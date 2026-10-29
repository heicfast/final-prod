#!/usr/bin/env python3
"""S68 coverage-gap census: which corpus artifacts are never cited anywhere in the
core repo's audit evidence base (diff-to-fix/, docs/, worklog*, scripts/)?
Output: a gap report of corpus files with zero citations — the deep-dive targets
for what prior sessions missed."""
import os, re, json, collections

CORPUS = "/home/z/my-project/audit/easeus-drw-research"
CORE = "/home/z/my-project/audit/core_recovery"

# 1. Enumerate corpus artifacts (files that could be cited)
citeable = []
for root, dirs, files in os.walk(CORPUS):
    dirs[:] = [d for d in dirs if d not in (".git",)]
    for f in files:
        p = os.path.join(root, f)
        rel = os.path.relpath(p, CORPUS)
        citeable.append(rel)

# 2. Read the core evidence base: diff-to-fix/ (all), docs/, worklog.md, worklog/,
#    scripts/, src/ comments (for good measure), tests/
core_texts = []
for base in ["diff-to-fix", "docs", "worklog", "scripts"]:
    b = os.path.join(CORE, base)
    if not os.path.isdir(b):
        continue
    for root, dirs, files in os.walk(b):
        dirs[:] = [d for d in dirs if d not in (".git",)]
        for f in files:
            try:
                with open(os.path.join(root, f), "r", encoding="utf-8", errors="replace") as fh:
                    core_texts.append(fh.read())
            except Exception:
                pass
for wf in ["worklog.md", "AUDIT.md", "AUDIT-NTFS.md", "AUDIT-FAT.md", "AUDIT-LINUX-FS.md",
           "AUDIT-ARCH.md", "AUDIT-PIPELINE.md", "AUDIT-FALLBACKS.md"]:
    p = os.path.join(CORE, wf)
    if os.path.exists(p):
        core_texts.append(open(p, encoding="utf-8", errors="replace").read())

blob = "\n".join(core_texts)
print(f"core evidence text: {len(blob)/1e6:.1f} MB across {len(core_texts)} files")

# 3. For each corpus file, count citations of distinctive parts of its name
#    S68 refinement: only ANALYTICAL artifacts — .md docs in artifacts/,
#    architecture/, raw-evidence/ docs, top-level docs. Icons/binaries excluded.
never, cited = [], collections.Counter()
for rel in citeable:
    if not rel.endswith(".md"):
        continue
    if not (rel.startswith("re-workspace/artifacts/") or rel.startswith("architecture/")
            or rel.startswith("re-workspace/raw-evidence/") or "/" not in rel):
        continue
    name = os.path.basename(rel)
    # distinctive stem: e.g. 'rawsearchdll--family-brance.md' -> 'family-brance'
    stem = name
    for ext in (".md", ".csv", ".json", ".txt", ".ini", ".xml", ".log", ".py", ".c", ".cpp", ".rsf"):
        if stem.endswith(ext):
            stem = stem[: -len(ext)]
    if len(stem) < 6 or stem in ("README", "worklog", "env"):
        continue  # too generic
    n = blob.count(stem)
    if n == 0:
        never.append(rel)
    else:
        cited[rel] = n

print(f"\nciteable corpus files: {len(citeable)}; distinct stems checked: {len(never)+len(cited)}")
print(f"NEVER cited anywhere in core evidence base: {len(never)}")
print("\n=== NEVER-CITED corpus artifacts (the deep-dive targets) ===")
by_dir = collections.defaultdict(list)
for rel in sorted(never):
    d = os.path.dirname(rel)
    by_dir[d].append(os.path.basename(rel))
for d in sorted(by_dir, key=lambda x: -len(by_dir[x])):
    print(f"\n[{d}] ({len(by_dir[d])} files)")
    for f in sorted(by_dir[d]):
        print(f"  {f}")

# 4. Also: low-citation (< 3 hits) .md docs
print("\n=== LOW-CITED (<3 mentions) corpus .md docs ===")
low = [(rel, cited[rel]) for rel in cited if cited[rel] < 3 and rel.endswith(".md") and "re-workspace/artifacts" in rel]
for rel, n in sorted(low, key=lambda x: x[1]):
    print(f"  {n:2d}x {rel}")
