"""Run an existing human differential suite with isolated build/log outputs.

Uses the checked-in MinGW build recipe, never its shared %TEMP% outputs. Requires
32-bit MinGW C++, Wine on Linux, an unpacked reference executable and its DLLs.
Game assets and compiler packages remain local and are never written by this runner.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--suite", choices=("control", "controller", "playernet", "remote", "combat"), required=True)
    parser.add_argument("--compiler", type=Path, required=True)
    parser.add_argument("--cxxflag", action="append", default=[])
    parser.add_argument("--original", type=Path, required=True)
    parser.add_argument("--game-dir", type=Path, required=True, help="isolated directory of reference DLLs")
    parser.add_argument("--wine-prefix", type=Path)
    parser.add_argument("--output-root", type=Path, default=ROOT / "build/campaign_control_legacy")
    args = parser.parse_args()
    if os.name == "nt":
        parser.error("This runner currently invokes the Linux MinGW cross compiler and Wine")
    for path in (args.compiler, args.original):
        if not path.is_file(): parser.error("Missing prerequisite: " + str(path))
    if not (args.game_dir / "LS3DF.dll").is_file(): parser.error("--game-dir must supply LS3DF.dll and its dependencies")
    args.output_root.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=args.suite + "-", dir=args.output_root.resolve()))
    print("Output:", output, flush=True)
    recipe = (ROOT / "_re/harness/human" / ("build_" + args.suite + ".bat")).read_text()
    variables = {}
    for line in recipe.splitlines():
        if line.startswith("set WRAP"):
            key, value = line[4:].split("=", 1)
            variables[key] = value
    commands = []
    for index, line in enumerate(x for x in recipe.splitlines() if x.startswith("g++ ")):
        line = line.removesuffix(" 2>&1")
        for key, value in variables.items(): line = line.replace("%" + key + "%", value)
        tokens = shlex.split(line.replace("\\", "/"))[1:]
        tokens = [x.replace("%SRC%", str(ROOT / "src")).replace("%TEMP%", str(output)).replace("\\", "/") for x in tokens]
        if any("%" in x for x in tokens): raise ValueError("unhandled recipe variable")
        command = [str(args.compiler.resolve()), "-static", *args.cxxflag, *tokens]
        commands.append(command)
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        result = subprocess.run(command, cwd=ROOT / "_re/harness/human", stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True, errors="replace")
        (output / ("compile-" + str(index) + ".log")).write_text(result.stdout)
        if result.returncode: raise SystemExit("FAIL: compile; see " + str(output))
    executable = output / ("human_" + args.suite + "_test.exe")
    environment = dict(os.environ)
    environment["HD2_ORIGINAL_EXE"] = "Z:" + str(args.original.resolve()).replace("/", "\\")
    environment["HD2_GAME_DIR"] = "Z:" + str(args.game_dir.resolve()).replace("/", "\\")
    if args.wine_prefix: environment["WINEPREFIX"] = str(args.wine_prefix.resolve())
    environment.setdefault("WINEDEBUG", "-all")
    command = ["wine64", str(executable)]
    commands.append(command)
    (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
    result = subprocess.run(command, cwd=output, env=environment, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True, errors="replace")
    (output / "execute.log").write_text(result.stdout)
    print(result.stdout[-2000:])
    if result.returncode: raise SystemExit("FAIL: execute; see " + str(output))
    print("PASS:", args.suite, flush=True)


if __name__ == "__main__":
    main()
