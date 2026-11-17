"""Run a legacy build with worktree-local temporary files and an exclusive lock.

Usage: python _re/tools/build_isolated.py {game,check,link-check,ls3df,rw-data,ls3drf} [build arguments ...]
Use Windows Python for MSVC builds (also when Python runs under Wine).
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
TOOLS = {
    "game": "_re/tools/build_game.py",
    "check": "_re/tools/build_check.py",
    "link-check": "_re/tools/link_check.py",
    "ls3df": "ls3df/build.py",
    "rw-data": "_re/tools/build_rw_data.py",
    "ls3drf": "ls3drf/build.py",
}


def run(tool, arguments, root=ROOT):
    # check and link-check intentionally share objects, and must share the lock.
    family = "check" if tool == "link-check" else tool
    variant = "debug" if tool in ("ls3df", "ls3drf") and "--debug" in arguments else "default"
    base = root / "build" / "isolated" / family
    base.mkdir(parents=True, exist_ok=True)
    lock = base / "build.lock"
    try:
        lock.mkdir()
    except FileExistsError:
        print("Build output is locked: " + str(lock), file=sys.stderr)
        print("Check owner.json and stop its build before removing a stale lock.", file=sys.stderr)
        return 1
    owner = lock / "owner.json"
    try:
        owner.write_text(json.dumps({"pid": os.getpid(), "tool": tool,
                                     "arguments": arguments, "worktree": str(root)}, indent=2))
        temp = base / variant / "temp"
        temp.mkdir(parents=True, exist_ok=True)
        env = os.environ.copy()
        for key in ("TEMP", "TMP", "TMPDIR"):
            env[key] = str(temp)
        print("Build temporary root: " + str(temp), flush=True)
        # subprocess argv preserves paths with spaces; never change the caller's environment.
        result = subprocess.run([sys.executable, str(root / TOOLS[tool]), *arguments],
                                cwd=root, env=env)
    except BaseException:
        # An interrupted compiler can still have descendants. Keep the lock for inspection.
        print("Build interrupted; output remains locked at " + str(lock), file=sys.stderr)
        raise
    else:
        owner.unlink()
        lock.rmdir()
        return result.returncode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tool", choices=TOOLS)
    parser.add_argument("arguments", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    arguments = args.arguments[1:] if args.arguments[:1] == ["--"] else args.arguments
    return run(args.tool, arguments)


if __name__ == "__main__":
    sys.exit(main())
