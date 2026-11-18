"""Compile/run COOP-005 isolated preparation authority behavior.

Native Python: --compiler gcc supplements the required target check.
Windows Python in an x86 MSVC environment (or --vcvars / HD2_VCVARS32):
  --compiler msvc runs the intended target compiler. No game libraries/data used.
Each invocation creates a unique ignored output directory and records commands/logs.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from legacy_source import generate

ROOT = Path(__file__).resolve().parents[3]
SOURCES = ROOT / "_re" / "harness" / "campaign_preparation"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", choices=("gcc", "msvc"), required=True)
    parser.add_argument("--vcvars", default=os.environ.get("HD2_VCVARS32"))
    parser.add_argument("--output-root", type=Path, default=ROOT / "build" / "campaign_preparation")
    args = parser.parse_args()
    if args.compiler == "msvc" and os.name != "nt":
        parser.error("Run --compiler msvc using Windows Python and the intended x86 MSVC toolchain")
    if args.vcvars and not Path(args.vcvars).is_file():
        parser.error("--vcvars/HD2_VCVARS32 must name an existing batch file")
    args.output_root.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.compiler + "-", dir=args.output_root.resolve()))
    (output / "environment.json").write_text(json.dumps({
        "compiler": args.compiler, "vcvars": args.vcvars if args.compiler == "msvc" else None,
        "python": os.sys.version, "platform": os.sys.platform,
        "sources": str(SOURCES), "output": str(output)
    }, indent=2) + "\n")
    generate(ROOT, output)
    commands = []
    print("Output:", output, flush=True)

    def run(name, command, expect_failure=False, use_vcvars=False):
        commands.append({"name": name, "command": command, "expected_failure": expect_failure})
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        actual = command
        if use_vcvars and args.vcvars:
            # Generated only under this invocation's private output directory.
            # vcvars is a local setup file; no shell interpolation of source data.
            if any(c in str(args.vcvars) for c in '\r\n"%'):
                raise ValueError("vcvars path contains unsupported batch metacharacters")
            batch = output / (name + ".bat")
            batch.write_text('@echo off\ncall "' + str(args.vcvars) +
                             '"\nif errorlevel 1 exit /b %errorlevel%\n' +
                             subprocess.list2cmdline(command) + '\nexit /b %errorlevel%\n')
            actual = ["cmd", "/d", "/c", str(batch)]
        result = subprocess.run(actual, cwd=output, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, errors="replace", check=False)
        (output / (name + ".log")).write_text(result.stdout, encoding="utf-8")
        if (result.returncode != 0) != expect_failure:
            print(result.stdout)
            raise SystemExit(f"FAIL: {name}: unexpected exit {result.returncode}; see {output}")
        print(f"PASS: {name} (exit {result.returncode})", flush=True)

    if args.compiler == "msvc":
        flags = ["/nologo", "/std:c++17", "/EHsc", "/W4", "/WX", "/permissive-",
                 "/Zc:__cplusplus", "/arch:IA32", "/MT", "/O2"]
        # Compiler banner/version is emitted by a successful probe, not a failing
        # cl-without-input command. _M_IX86/sizeof(void*) assertions check target.
        run("header", ["cl", *flags, "/Bv", "/c", str(SOURCES / "header_only.cpp"),
                       "/Foheader.obj"], use_vcvars=True)
        run("behavior", ["cl", *flags, str(SOURCES / "behavior.cpp"),
                         str(ROOT / "src" / "game" / "campaign_preparation.cpp"),
                         "/Febehavior.exe", "/link", "/MACHINE:X86", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO"], use_vcvars=True)
        run("execute", [str(output / "behavior.exe")])
        run("legacy-parity", ["cl", *flags, "/I" + str(output), str(SOURCES / "legacy_parity.cpp"),
                              str(ROOT / "src" / "game" / "campaign_preparation.cpp"),
                              "/Felegacy-parity.exe", "/link", "/MACHINE:X86", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO"], use_vcvars=True)
        run("legacy-execute", [str(output / "legacy-parity.exe")])
    else:
        compiler = shutil.which("g++")
        if not compiler:
            raise SystemExit("g++ is required for the supplementary native check")
        flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-pedantic"]
        run("version", [compiler, "--version"])
        run("header", [compiler, *flags, "-c", str(SOURCES / "header_only.cpp"), "-o", "header.o"])
        run("behavior", [compiler, *flags, str(SOURCES / "behavior.cpp"),
                         str(ROOT / "src" / "game" / "campaign_preparation.cpp"), "-o", "behavior"])
        run("execute", [str(output / "behavior")])
        run("legacy-parity", [compiler, *flags, "-I", str(output), str(SOURCES / "legacy_parity.cpp"),
                              str(ROOT / "src" / "game" / "campaign_preparation.cpp"), "-o", "legacy-parity"])
        run("legacy-execute", [str(output / "legacy-parity")])
    print("PASS: standalone header, preparation behavior and original-rule parity")
    print("Commands and compiler diagnostics:", output)


if __name__ == "__main__":
    main()
