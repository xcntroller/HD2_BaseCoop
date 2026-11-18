"""Configure an already downloaded portable MSVC/SDK and Windows Python for Wine.

Run with Linux Python from the worktree root. This downloads nothing and changes no
global Wine prefix or environment. See docs/build_baseline.md for prerequisites.
"""
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def windows_path(path):
    # Wine's default Z: mapping exposes the Unix root. Batch expansion of % or a
    # newline/quote in a path is unsafe even inside quoted SET statements.
    value = str(path.resolve())
    if any(char in value for char in ('%', '"', '\n', '\r')):
        raise ValueError("Unsupported character in batch-file path: " + value)
    return "Z:" + value.replace("/", "\\")


def configure(toolchain, python, output, msvc_version, sdk_version):
    msvc = toolchain / "VC" / "Tools" / "MSVC" / msvc_version
    sdk = toolchain / "Windows Kits" / "10"
    include = [msvc / "include"] + [sdk / "Include" / sdk_version / name
                                    for name in ("ucrt", "shared", "um", "winrt")]
    libs = [msvc / "lib/x86", sdk / "Lib" / sdk_version / "ucrt/x86",
            sdk / "Lib" / sdk_version / "um/x86"]
    bins = [msvc / "bin/Hostx64/x86", sdk / "bin" / sdk_version / "x64"]
    for path in [python, *include, *libs, *bins, bins[0] / "cl.exe", bins[0] / "link.exe"]:
        if not path.exists():
            raise FileNotFoundError(path)
    output.mkdir(parents=True, exist_ok=True)
    vcvars = output / "vcvars-x86.bat"
    lines = ["@echo off", 'set "INCLUDE=' + ";".join(map(windows_path, include)) + '"',
             'set "LIB=' + ";".join(map(windows_path, libs)) + '"',
             'set "PATH=' + ";".join(map(windows_path, bins)) + ';%PATH%"',
             'set "WindowsSdkDir=' + windows_path(sdk) + '\\"']
    vcvars.write_text("\n".join(lines) + "\n")
    # Embedded Python ignores the script directory unless it is explicitly added.
    runner = output / "run_script.py"
    runner.write_text("import os, runpy, sys\n"
                      "sys.argv = sys.argv[1:]\n"
                      "script = os.path.abspath(sys.argv[0])\n"
                      "sys.path.insert(0, os.path.dirname(script))\n"
                      "runpy.run_path(script, run_name='__main__')\n")
    launcher = output / "run-python.bat"
    launcher.write_text('@echo off\nset "HD2_VCVARS32=' + windows_path(vcvars) + '"\n"' +
                        windows_path(python) + '" "' + windows_path(runner) + '" %*\n')
    print("Configured x86 MSVC " + msvc_version + ", Windows SDK " + sdk_version)
    print("Launcher: " + str(launcher.resolve()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--toolchain", type=Path, default=ROOT / "build/tooling/msvc")
    parser.add_argument("--windows-python", type=Path, default=ROOT / "build/tooling/python/python.exe")
    parser.add_argument("--output", type=Path, default=ROOT / "build/tooling")
    parser.add_argument("--msvc-version", required=True)
    parser.add_argument("--sdk-version", required=True)
    args = parser.parse_args()
    configure(args.toolchain, args.windows_python, args.output, args.msvc_version, args.sdk_version)


if __name__ == "__main__":
    main()
