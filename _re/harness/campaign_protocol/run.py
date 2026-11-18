"""Build/run COOP-006 portable or intended MSVC x86 conformance in private outputs."""
import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]
SOURCES = Path(__file__).resolve().parent


def check_fixtures():
    fixtures = SOURCES / "fixtures"
    for path in fixtures.glob("*.hex"):
        frame = bytes.fromhex(path.read_text())
        magic, major, minor, kind, flags, size = struct.unpack("<IHHHHI", frame[:16])
        assert (magic, major, minor, flags) == (0x50433248, 1, 0, 0), path
        assert 1 <= kind <= 19 and size == len(frame) - 16, path
    # Independent scalar fixtures, calculated from the published wire schema.
    ack = struct.pack("<HHQQQ", 1, 0, 7, 0x1122334455667788, 5)
    header = struct.pack("<IHHHHI", 0x50433248, 1, 0, 3, 0, len(ack))
    assert bytes.fromhex((fixtures / "3.hex").read_text()) == header + ack
    # SnapshotAck eight u64 fields; max-width identities must not narrow to x86.
    ack = struct.pack("<8Q", (1 << 64) - 1, (1 << 64) - 1, 3, 13, 4, 8, 10, 6)
    header = struct.pack("<IHHHHI", 0x50433248, 1, 0, 14, 0, len(ack))
    assert bytes.fromhex((fixtures / "26.hex").read_text()) == header + ack


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", choices=("gcc", "msvc"), required=True)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--vcvars", default=os.environ.get("HD2_VCVARS32"))
    parser.add_argument("--output-root", type=Path, default=ROOT / "build" / "campaign_protocol")
    args = parser.parse_args()
    if args.compiler == "msvc" and os.name != "nt":
        parser.error("Use Windows Python with the intended MSVC x86 toolchain")
    if args.sanitize and args.compiler != "gcc":
        parser.error("--sanitize requires GCC")
    if args.vcvars and not Path(args.vcvars).is_file():
        parser.error("--vcvars/HD2_VCVARS32 must be an existing setup batch")
    check_fixtures()
    args.output_root.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.compiler + "-", dir=args.output_root.resolve()))
    print("Output:", output, flush=True)
    (output / "environment.json").write_text(json.dumps({
        "compiler": args.compiler, "sanitize": args.sanitize, "vcvars": args.vcvars,
        "python": os.sys.version, "platform": os.sys.platform, "output": str(output)
    }, indent=2) + "\n")
    commands = []

    def run(name, command, setup=False):
        commands.append({"name": name, "argv": command})
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        actual = command
        if setup and args.vcvars:
            if any(c in str(args.vcvars) for c in '\r\n"%'):
                raise ValueError("unsupported batch expansion in vcvars path")
            batch = output / (name + ".bat")
            batch.write_text('@echo off\ncall "' + str(args.vcvars) +
                             '"\nif errorlevel 1 exit /b %errorlevel%\n' +
                             subprocess.list2cmdline(command) + '\nexit /b %errorlevel%\n')
            actual = ["cmd", "/d", "/c", str(batch)]
        env = os.environ.copy()
        if args.sanitize:
            env["ASAN_OPTIONS"] = "detect_leaks=1"
            env["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
        result = subprocess.run(actual, cwd=output, env=env, text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                errors="replace", check=False)
        (output / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        print(result.stdout, end="", flush=True)
        if result.returncode:
            raise SystemExit(f"FAIL: {name}, exit {result.returncode}; see {output}")

    cpp = str(ROOT / "src" / "net" / "campaign_protocol.cpp")
    harness = str(SOURCES / "conformance.cpp")
    if args.compiler == "msvc":
        flags = ["/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", "/permissive-",
                 "/Zc:__cplusplus", "/arch:IA32", "/MT", "/O2"]
        run("header", ["cl", *flags, "/Bv", "/c", str(SOURCES / "header_only.cpp"), "/Foheader.obj"], setup=True)
        run("compile", ["cl", *flags, cpp, harness, "/Feconformance.exe", "/link",
                        "/MACHINE:X86", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO"], setup=True)
        run("execute", [str(output / "conformance.exe"), str(SOURCES / "fixtures")])
    else:
        compiler = shutil.which("g++")
        if not compiler:
            raise SystemExit("g++ is required")
        flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic"]
        if args.sanitize:
            flags += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        run("version", [compiler, "--version"])
        run("header", [compiler, *flags, "-c", str(SOURCES / "header_only.cpp"), "-o", "header.o"])
        run("compile", [compiler, *flags, cpp, harness, "-o", "conformance"])
        run("execute", [str(output / "conformance"), str(SOURCES / "fixtures")])
    print("PASS: independent scalar fixtures, standalone header and eleven conformance suites")


if __name__ == "__main__":
    main()
