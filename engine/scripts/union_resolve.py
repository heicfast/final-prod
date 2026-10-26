#!/usr/bin/env python3
"""union-resolve.py — resolve worklog.md (and similar append-log) merge
conflicts by keeping BOTH sides verbatim (the repo's union protocol),
dropping only the conflict markers. Handles multiple conflict hunks.
Usage: python3 union_resolve.py <file>"""
import sys

def main(path):
    s = open(path).read()
    lines = s.split('\n')
    out = []
    i = 0
    hunks = 0
    while i < len(lines):
        l = lines[i]
        if l.startswith('<<<<<<<'):
            hunks += 1
            # collect HEAD side until =======, then theirs until >>>>>>>
            i += 1
            head = []
            while i < len(lines) and lines[i] != '=======' and not lines[i].startswith('>>>>>>>'):
                head.append(lines[i])
                i += 1
            if i < len(lines) and lines[i] == '=======':
                i += 1
            theirs = []
            while i < len(lines) and not lines[i].startswith('>>>>>>>'):
                theirs.append(lines[i])
                i += 1
            if i < len(lines) and lines[i].startswith('>>>>>>>'):
                i += 1
            # union: head side then theirs side; avoid duplicating identical
            # trailing/leading blank boundaries
            def norm(block):
                while block and block[0] == '':
                    block.pop(0)
                while block and block[-1] == '':
                    block.pop()
                return block
            h, t = norm(head), norm(theirs)
            if h and t:
                out.extend(h)
                out.append('')
                out.extend(t)
            elif h:
                out.extend(h)
            else:
                out.extend(t)
            continue
        out.append(l)
        i += 1
    open(path, 'w').write('\n'.join(out))
    print(f"resolved {hunks} hunk(s) in {path} (union: both sides kept)")

if __name__ == '__main__':
    main(sys.argv[1])
