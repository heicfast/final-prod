#!/usr/bin/env python3
"""Resolve rebase conflicts keep-both: HEAD content first, incoming second,
markers dropped. Used by DFX7 when parallel legs touched the same files."""
import re, subprocess, sys

files = subprocess.run(["git","diff","--name-only","--diff-filter=U"],
                       capture_output=True, text=True).stdout.split()
print("conflicted:", files)
ok = True
for path in files:
    s = open(path).read()
    # split into segments by conflict markers
    pat = re.compile(r"<<<<<<< HEAD\n(.*?)\n?=======\n(.*?)\n?>>>>>>> [^\n]*\n",
                     re.S)
    n = 0
    def repl(m):
        global n; n += 1
        head, inc = m.group(1), m.group(2)
        if head and not head.endswith("\n"): head += "\n"
        if inc and not inc.startswith("\n"): inc = "\n" + inc
        if inc and not inc.endswith("\n"): inc += "\n"
        return head + inc
    s2 = pat.sub(repl, s)
    if s2 == s and "<<<<<<<" in s:
        print("UNRESOLVED:", path); ok = False; continue
    open(path,"w").write(s2)
    left = len(re.findall(r"^[<>=]{7}$", s2, re.M))
    if left: print("MARKERS LEFT:", path); ok = False; continue
    subprocess.run(["git","add",path])
    print("resolved:", path, f"({n} conflict(s))")
sys.exit(0 if ok else 1)
