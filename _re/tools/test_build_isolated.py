"""Process-level regression checks for the build wrapper; no MSVC or game data needed."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

import build_isolated

FAKE_BUILD = r'''
import json, pathlib, sys, tempfile, time
root = pathlib.Path.cwd()
out = pathlib.Path(tempfile.gettempdir()) / "ls3df_build"
out.mkdir(parents=True, exist_ok=True)
(out / "actor.obj").write_text(root.name)
(root / "started.json").write_text(json.dumps({"temp": str(out)}))
while not (root / "release").exists():
    time.sleep(0.02)
assert (out / "actor.obj").read_text() == root.name, "consumed another worktree's object"
(root / "result.json").write_text(json.dumps({"temp": str(out)}))
sys.exit(7 if "--fail" in sys.argv else 0)
'''


class IsolatedBuildTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="hd2-isolation-test-")
        self.addCleanup(self.scratch.cleanup)

    def tree(self, name):
        root = Path(self.scratch.name) / name
        (root / "ls3df").mkdir(parents=True)
        (root / "ls3df/build.py").write_text(FAKE_BUILD)
        return root

    def start(self, root, *args):
        # Import the real wrapper while giving it an independent fake worktree.
        command = [sys.executable, "-c",
                   "import sys; from pathlib import Path; import build_isolated; "
                   "sys.exit(build_isolated.run('ls3df', sys.argv[2:], Path(sys.argv[1])))",
                   str(root), *args]
        child = subprocess.Popen(command, cwd=Path(build_isolated.__file__).parent,
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        self.addCleanup(self.stop, child)
        return child

    @staticmethod
    def stop(child):
        if child.poll() is None:
            child.kill()
        child.communicate(timeout=10)

    def wait_started(self, root, child):
        deadline = time.monotonic() + 15
        while not (root / "started.json").exists():
            if child.poll() is not None or time.monotonic() >= deadline:
                self.fail("fake build did not start: " + str(child.communicate(timeout=1)))
            time.sleep(0.02)
        return json.loads((root / "started.json").read_text())

    def finish(self, root, child, expected=0):
        (root / "release").touch()
        stdout, stderr = child.communicate(timeout=15)
        self.assertEqual(child.returncode, expected, stdout + stderr)

    def test_concurrent_worktrees_cannot_share_objects(self):
        before = {key: os.environ.get(key) for key in ("TEMP", "TMP", "TMPDIR")}
        left, right = self.tree("left tree"), self.tree("right tree")
        first, second = self.start(left), self.start(right)
        a, b = self.wait_started(left, first), self.wait_started(right, second)
        self.assertNotEqual(a["temp"], b["temp"])
        self.assertTrue(Path(a["temp"]).is_relative_to(left))
        self.assertTrue(Path(b["temp"]).is_relative_to(right))
        self.finish(left, first)
        self.finish(right, second)
        self.assertEqual(before, {key: os.environ.get(key) for key in before})

    def test_same_worktree_rejects_even_different_engine_variant(self):
        root = self.tree("busy tree")
        first = self.start(root)
        self.wait_started(root, first)
        second = self.start(root, "--debug")
        stdout, stderr = second.communicate(timeout=15)
        self.assertEqual(second.returncode, 1, stdout + stderr)
        self.assertIn("Build output is locked", stderr)
        self.finish(root, first)

    def test_failure_code_and_lock_release(self):
        root = self.tree("failing tree")
        child = self.start(root, "--fail")
        self.wait_started(root, child)
        self.finish(root, child, expected=7)
        self.assertFalse((root / "build/isolated/ls3df/build.lock").exists())
        child = self.start(root)
        self.finish(root, child)


if __name__ == "__main__":
    unittest.main()
