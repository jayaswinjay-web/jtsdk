#!/usr/bin/env python3
"""Generate the JTS GO Tutorials book chapters from the tutorial source files.

Each tutorial (tutorials/NN_*.jts) becomes one chapter in book/tutorial-html/.
The leading comment block is turned into prose; everything after it is the
program source, rendered as a highlighted code block by build.py at PDF time.
"""
import html as html_mod
import os
import re

ROOT = os.path.dirname(os.path.abspath(__file__))
TUTORIAL_DIR = os.path.join(os.path.dirname(ROOT), "tutorials")
OUT_DIR = os.path.join(ROOT, "tutorial-html")

BACKTICK = re.compile(r"`([^`]+)`")


def codeify(text):
    return BACKTICK.sub(lambda m: "<code>%s</code>" % html_mod.escape(m.group(1)), text)


def escape(text):
    return html_mod.escape(text, quote=False)


def chapter_from_tutorial(path):
    with open(path, encoding="utf-8") as fh:
        raw = fh.read().split("\n")

    name = os.path.basename(path)
    stem = name[:-4]  # strip ".jts"
    num = re.match(r"(\d+)", stem).group(1)

    lines = iter(raw)
    header = []
    for line in lines:
        if line.startswith("#"):
            header.append(line[1:].lstrip())
        else:
            break
    rest = [line for line in lines]

    while rest and not rest[0].strip():
        rest.pop(0)
    code = "\n".join(rest).rstrip() + "\n"

    title = "Tutorial %s" % num
    paragraphs = []
    run_cmd = None
    challenge = None
    for line in header:
        if not line.strip():
            continue
        if line.lower().startswith("tutorial ") and ":" in line:
            title = line.split(":", 1)[1].strip()
            continue
        if line.lower().startswith("goal:"):
            paragraphs.append("<p><strong>Goal:</strong> %s</p>" % codeify(line.split(":", 1)[1].strip()))
            continue
        if line.lower().startswith("try it:"):
            run_cmd = line.split(":", 1)[1].strip()
            continue
        if line.lower().startswith("challenge:"):
            challenge = line.split(":", 1)[1].strip()
            continue
        paragraphs.append("<p>%s</p>" % codeify(line.strip()))

    out = []
    out.append('<h1 id="tut%s">Tutorial %s: %s</h1>' % (num, int(num), escape(title)))
    out.append("<p>Source file: <code>tutorials/%s</code> — a complete, runnable program. "
               "Every tutorial in this book is a file in the repository's "
               "<code>tutorials/</code> folder.</p>" % escape(name))
    out.extend(paragraphs)
    if run_cmd:
        out.append("<p><strong>Run it:</strong> <code>%s</code></p>" % escape(run_cmd))
    out.append("<pre><code>%s</code></pre>" % escape(code))
    if challenge:
        out.append('<p class="challenge"><strong>Challenge:</strong> %s</p>' % codeify(challenge))
    return "\n".join(out) + "\n"


def main():
    os.makedirs(OUT_DIR, exist_ok=True)
    files = sorted(f for f in os.listdir(TUTORIAL_DIR) if f.endswith(".jts"))
    for name in files:
        html = chapter_from_tutorial(os.path.join(TUTORIAL_DIR, name))
        out_path = os.path.join(OUT_DIR, name[:-4] + ".html")
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write(html)
        print("Wrote", os.path.relpath(out_path, ROOT))


if __name__ == "__main__":
    main()
