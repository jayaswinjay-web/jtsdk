#!/usr/bin/env python3
"""Build the JTS GO Programming Language book as a single PDF using WeasyPrint."""
import html as html_mod
import glob
import os
import re
import sys

import weasyprint

ROOT = os.path.dirname(os.path.abspath(__file__))
HTML_DIR = os.path.join(ROOT, "html")
CSS_FILE = os.path.join(ROOT, "css", "book.css")
OUT_PDF = os.path.join(ROOT, "JTS-GO-Programming-Language.pdf")

KEYWORDS = {
    "and", "or", "not", "if", "elif", "else", "end", "while", "for", "in",
    "to", "func", "return", "true", "false", "nil", "class", "self", "super",
    "extends", "new", "is", "del", "assert", "try", "catch", "throw",
    "finally", "yield", "break", "continue", "import", "append",
    "len", "print", "input", "type", "str", "int", "float", "bool", "number",
    "matrix", "tensor", "predict", "http", "request", "response", "server",
    "model", "set", "var", "list",
}


def highlight_code(text):
    """Very small tokenizer that wraps keywords/strings/numbers/comments in spans.

    Operates on raw (unescaped) code text and HTML-escapes every emitted token, so
    entities such as &#x27; or &quot; are never re-tokenized as comments or strings.
    """
    out = []
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == '"':
            start = i
            i += 1
            while i < n and text[i] != '"':
                if text[i] == "\\":
                    i += 2
                else:
                    i += 1
            if i < n:
                i += 1
            token = text[start:i]
            out.append('<span class="str">' + html_mod.escape(token) + "</span>")
            continue
        if ch == "#":
            start = i
            while i < n and text[i] != "\n":
                i += 1
            token = text[start:i]
            out.append('<span class="com">' + html_mod.escape(token) + "</span>")
            continue
        if ch.isalpha() or ch == "_":
            start = i
            while i < n and (text[i].isalnum() or text[i] == "_"):
                i += 1
            word = text[start:i]
            if word in KEYWORDS:
                out.append('<span class="kw">' + word + "</span>")
            else:
                out.append(html_mod.escape(word))
            continue
        if ch.isdigit():
            start = i
            while i < n and (text[i].isalnum() or text[i] in "._"):
                i += 1
            out.append('<span class="num">' + html_mod.escape(text[start:i]) + "</span>")
            continue
        out.append(html_mod.escape(ch))
        i += 1
    return "".join(out)


def process_code_blocks(fragment):
    """Replace <pre><code> blocks with highlighted versions."""
    pattern = re.compile(r"<pre(?P<attrs>[^>]*)><code>(?P<body>.*?)</code></pre>", re.DOTALL)
    def repl(m):
        body = m.group("body")
        body = html_mod.unescape(body)
        body = highlight_code(body)
        return "<pre" + m.group("attrs") + "><code>" + body + "</code></pre>"
    return pattern.sub(repl, fragment)


def collect_headings(fragment):
    headings = []
    for m in re.finditer(r"<h([12]) id=\"([^\"]+)\">(.*?)</h\1>", fragment, re.DOTALL):
        level = int(m.group(1))
        hid = m.group(2)
        title = re.sub(r"<[^>]+>", "", m.group(3)).strip()
        headings.append((level, hid, title))
    return headings


def build():
    files = sorted(glob.glob(os.path.join(HTML_DIR, "*.html")))
    # Exclude cover.html from chapter list
    files = [f for f in files if not f.endswith("cover.html")]
    if not files:
        print("No chapter files found in", HTML_DIR)
        sys.exit(1)

    toc_entries = []
    body = []
    toc_marker = None
    for f in files:
        with open(f, encoding="utf-8") as fh:
            fragment = fh.read()
        fragment = process_code_blocks(fragment)
        headings = collect_headings(fragment)
        if "<!--TOC-->" in fragment:
            toc_marker = len(body)
        body.append(fragment)
        toc_entries.extend(headings)

    toc_html = build_toc(toc_entries)
    if toc_marker is not None:
        body[toc_marker] = body[toc_marker].replace("<!--TOC-->", toc_html)

    with open(CSS_FILE, encoding="utf-8") as fh:
        css = fh.read()

    doc = """<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>The JTS GO Programming Language</title>
</head>
<body>
""" + "\n".join(body) + """
</body>
</html>
"""
    print("Rendering PDF with %d chapters..." % len(files))
    html_doc = weasyprint.HTML(string=doc)
    html_doc.write_pdf(OUT_PDF, stylesheets=[weasyprint.CSS(string=css)])

    # Render cover as separate PDF and prepend
    cover_html_path = os.path.join(HTML_DIR, "cover.html")
    if os.path.exists(cover_html_path):
        print("Rendering cover page...")
        import PyPDF2
        cover_pdf_path = os.path.join(ROOT, "_cover_tmp.pdf")
        weasyprint.HTML(filename=cover_html_path).write_pdf(cover_pdf_path)
        # Merge: cover + book
        merger = PyPDF2.PdfMerger()
        merger.append(cover_pdf_path)
        merger.append(OUT_PDF)
        merger.write(OUT_PDF)
        merger.close()
        os.remove(cover_pdf_path)
        print("Cover prepended.")

    print("Wrote", OUT_PDF)


def build_toc(entries):
    parts = ['<div class="toc"><h1>Contents</h1><ul>']
    for level, hid, title in entries:
        tag = "li"
        if level == 1:
            parts.append("</ul><ul>")
        parts.append(
            '<%s><a href="#%s">%s</a></%s>' % (tag, hid, title, tag)
        )
    parts.append("</ul></div>")
    return "\n".join(parts)


if __name__ == "__main__":
    build()
