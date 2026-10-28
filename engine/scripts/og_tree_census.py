#!/usr/bin/env python3
"""scripts/og_tree_census.py - the og-codebase tree census (N-160's repair).

I-034's original census cited two scripts that were never committed
(`s41_never_cited_census.sh` / `s41_ogfile_census.py`) — the census was not
reproducible from its citation. This script is the committed, reproducible
form: it walks the corpus's `og-codebase/` tree and prints the depth census
the registers cite, in the registers' 1-based depth convention
(root = depth 1; "top+1" = 1-based depth <= 2; "top+2" = 1-based depth <= 3).

Usage (run from the corpus checkout, or pass the path):
    python3 og_tree_census.py [path-to-easeus-drw-research]

Verified numbers at corpus `be0cfe9` (re-executed 2026-10-29, the S56 lane,
cross-checked against `find` semantics: maxdepth 2 -> 555 children;
maxdepth 3 -type f -> 750; mindepth 4 -type f -> 720):
    entries_total=1538 (68 dirs + 1470 files) · entries_top1=555 ·
    files_top2=750 · files_deeper=720. The top_dirs_direct line counts
    DIRECT entries per first-level directory (not recursive).

Licensed under the MIT license. See LICENSE.
"""

import os
import subprocess
import sys


def git_commit(root: str) -> str:
    try:
        out = subprocess.run(
            ["git", "-C", root, "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, timeout=10, check=False,
        )
        return out.stdout.strip() if out.returncode == 0 else "unknown"
    except Exception:
        return "unknown"


def main() -> int:
    corpus = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
    og = os.path.join(corpus, "og-codebase")
    if not os.path.isdir(og):
        og = corpus if os.path.basename(corpus) == "og-codebase" else ""
    if not og:
        print(f"og_tree_census: no og-codebase/ under {corpus!r}", file=sys.stderr)
        return 2

    entries_total = dirs_total = files_total = 0
    entries_top1 = files_top2 = files_deeper = 0
    top_dirs: dict[str, int] = {}

    for dirpath, dirnames, filenames in os.walk(og):
        rel = os.path.relpath(dirpath, og)
        # 1-based depth of this directory itself ("." == the root, depth 0).
        dir_depth = 0 if rel == "." else rel.count(os.sep) + 1
        if rel == ".":
            for d in dirnames:
                top_dirs[d] = 0
        else:
            entries_total += 1
            dirs_total += 1
            if dir_depth <= 2:
                entries_top1 += 1
            top_parent = rel.split(os.sep)[0]
            if top_parent in top_dirs:
                top_dirs[top_parent] += 1
        for _f in filenames:
            entries_total += 1
            files_total += 1
            # A file's own 1-based depth is its containing dir's depth + 1
            # (root files sit at depth 1).
            file_depth = dir_depth + 1
            if file_depth <= 2:
                entries_top1 += 1
            if file_depth <= 3:
                files_top2 += 1
            else:
                files_deeper += 1

    print(f"corpus_commit: {git_commit(corpus)}")
    print(f"entries_total: {entries_total}")
    print(f"dirs_total: {dirs_total}")
    print(f"files_total: {files_total}")
    print(f"entries_top1: {entries_top1}")
    print(f"files_top2: {files_top2}")
    print(f"files_deeper: {files_deeper}")
    print("top_dirs_direct: " + ", ".join(f"{d}={n}" for d, n in sorted(top_dirs.items(), key=lambda kv: -kv[1])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
