#!/usr/bin/env python3
"""ps_sanity.py - crude structural sanity for PowerShell files: brace/paren
balance with single/double-quoted strings and # comments stripped."""
import re
import sys

def strip_ps(text):
    out = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '#':
            while i < n and text[i] != '\n':
                i += 1
            continue
        if c == '"':
            i += 1
            while i < n:
                if text[i] == '`':
                    i += 2
                    continue
                if text[i] == '"':
                    i += 1
                    break
                i += 1
            out.append('""')
            continue
        if c == "'":
            i += 1
            while i < n:
                if text[i] == "'":
                    if i + 1 < n and text[i + 1] == "'":
                        i += 2
                        continue
                    i += 1
                    break
                i += 1
            out.append("''")
            continue
        out.append(c)
        i += 1
    return ''.join(out)

ok = True
for f in sys.argv[1:]:
    s = strip_ps(open(f, encoding='utf-8-sig').read())
    b = s.count('{') - s.count('}')
    p = s.count('(') - s.count(')')
    status = 'OK' if (b == 0 and p == 0) else 'IMBALANCE'
    print(f'{f}: braces {b:+d}, parens {p:+d} -> {status}')
    if b or p:
        ok = False
sys.exit(0 if ok else 1)
