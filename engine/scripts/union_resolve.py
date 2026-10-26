#!/usr/bin/env python3
"""union_resolve.py — mechanical keep-both resolution for the audit-trail files
(worklog.md, diff-to-fix/SUMMARY.md) after a parallel-lane merge conflict.

The parallel lanes append entry blocks to both files; the union resolution is
always: keep BOTH sides, drop the conflict markers, keep exactly one `---`
separator between the sides when both sides are worklog entry blocks.

Usage: python3 scripts/union_resolve.py <file> [--summary]
"""
import sys


def resolve(path):
    lines = open(path).read().split('\n')
    out = []
    state = 'normal'
    for i, l in enumerate(lines):
        if l.startswith('<<<<<<<'):
            state = 'head'
            continue
        if l.startswith('=======') and state == 'head':
            state = 'tail'
            # keep ONE blank separation line
            if out and out[-1].strip() != '':
                out.append('')
            continue
        if l.startswith('>>>>>>>'):
            state = 'normal'
            continue
        out.append(l)
    open(path, 'w').write('\n'.join(out))
    # verify
    leftovers = [i + 1 for i, l in enumerate(open(path).read().split('\n'))
                 if l.startswith('<<<<<<<') or l.startswith('>>>>>>>') or
                 (l.startswith('=======') and len(l.strip()) == 7)]
    return leftovers


if __name__ == '__main__':
    for p in sys.argv[1:]:
        left = resolve(p)
        print(f"{p}: resolved, leftover markers: {left or 'NONE'}")
