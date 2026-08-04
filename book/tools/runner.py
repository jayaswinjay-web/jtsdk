#!/usr/bin/env python3
"""Run JTS GO snippets against the official interpreter and capture stdout."""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
JTS_EXE = os.path.join(ROOT, "bin", "win32", "jts.exe")
SCROLLS = os.path.join(ROOT, "scrolls")


def run(source, cwd=None, scrolls_dir=SCROLLS):
    """Execute a JTS snippet and return its stdout as a list of lines (stripped)."""
    fd, path = tempfile.mkstemp(suffix=".jts", prefix="jtsbook_")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as fh:
            fh.write(source)
        env = dict(os.environ)
        if scrolls_dir:
            env["JTS_SCROLLS"] = scrolls_dir
        proc = subprocess.run(
            [JTS_EXE, path],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=120,
            cwd=cwd,
            env=env,
        )
        out = proc.stdout.rstrip("\n").split("\n")
        if out == [""]:
            out = []
        err = proc.stderr.rstrip("\n")
        return out, err, proc.returncode
    finally:
        try:
            os.remove(path)
        except OSError:
            pass


def main():
    # Read a JTS file path from argv and print its output.
    if len(sys.argv) < 2:
        print("usage: runner.py snippet.jts [expected...]")
        sys.exit(1)
    with open(sys.argv[1], encoding="utf-8") as fh:
        source = fh.read()
    out, err, code = run(source)
    if err:
        print("STDERR:", err)
    print("\n".join(out))
    print("exit:", code)


if __name__ == "__main__":
    main()
