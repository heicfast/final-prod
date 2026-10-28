#!/usr/bin/env python3
"""verify_trail_shas.py - the N-135 optional guard: every commit-like hex
token cited in the diff-to-fix gate docs must resolve in the object store.

The audit trail's phantom-SHA class (pre-rebase lane SHAs cited as fix
commits - N-135 item 3, the S11 wave's ledger repairs) becomes mechanically
detectable. Curated exclusions (each a documented non-core-SHA class):

  - tokens starting 1800/1801/1405/1402/1403/1401/1400 - decompile
    addresses (Ghidra's FUN_1800xxxxx / DRWUI 140xxxxx regions)
  - >= 10-digit decimal tokens - CI run IDs (e.g. 40549660074)
  - 8-hex small constants (ioctls, magic keys, wire values)
  - the corpus repo's own commit SHAs (03ad46c, be0cfe9 - the baselines
    cited as "corpus X" throughout; they resolve in the CORPUS repo)
  - the driver repo's SHAs (323e75a, f569836 - "driver X" cites; they
    resolve in heicfast/rda-driver, outside this repo's core+corpus grant)
  - 204137b - session-4's pre-rebase core lineage, documented as the
    historical baseline in README's session blocks

Exit 0 = every surviving token resolves (or is a known cross-repo cite);
exit 1 = live phantom cites remain (listed).

Advisory tool, deliberately NOT a CI gate (N-135's Resolution records the
decision): the curation set grows with every cross-repo cite class, so a
gate would churn; the S12/audit lanes run it per-session as the acceptance
check of the finding's "every SHA cited in diff-to-fix/ resolves".

Usage: python3 scripts/verify/verify_trail_shas.py [--all-history]
       --all-history: also scan diff-to-fix/evidence/ (the dated
       historical records - informational; the gate docs are the
       acceptance surface).
"""

import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent

GATE_DOCS = [
    "diff-to-fix/README.md",
    "diff-to-fix/SUMMARY.md",
    "diff-to-fix/CROSS-REF-MATRIX.md",
    "diff-to-fix/FIXED-LEDGER.md",
    "diff-to-fix/API-DELTA.md",
    "diff-to-fix/CORPUS-ERRATA.md",
    "diff-to-fix/PARAMETERS.md",
    "diff-to-fix/SCAN-ENGINE-DELTA.md",
    "diff-to-fix/SURFACE-INVENTORY.md",
]

# cross-repo commits the trail legitimately cites (resolve in their own
# repos; the corpus is the ground-truth repo, the driver is outside the
# core+corpus grant - both named in the trail's scope declarations)
CROSS_REPO = {
    "03ad46c",  # corpus baseline (pre-be0cfe9)
    "be0cfe9",  # corpus baseline
    "323e75a",  # rda-driver v0.2.4 (the session-8/9 driver audits)
    "f569836",  # rda-driver (API-DELTA's driver base)
    "204137b",  # session-4 pre-rebase core lineage - the documented
    # historical baseline in README's session blocks
    "6b50bb8",  # rda-driver v0.2.3 tag (SUMMARY's upstream narrative)
    "46b81573", # session-11's pre-rebase trail lineage (README's baseline)
}

DECOMPILE_PREFIXES = ("1800", "1801", "1405", "1402", "1403", "1401", "1400")


def looks_like_constant(tok: str) -> bool:
    """Small constants (ioctls, magic keys, wire values) cited as hex."""
    if len(tok) != 8:
        return False
    try:
        v = int(tok, 16)
    except ValueError:
        return False
    return v < 0x10000000 or v in (0xFFFFFFFF, 0xFFFFFFFE)


def is_run_id(tok: str) -> bool:
    return len(tok) >= 10 and tok.isdigit()


def resolves(tok: str) -> bool:
    r = subprocess.run(["git", "cat-file", "-t", tok], cwd=REPO, capture_output=True)
    return r.returncode == 0


def main() -> int:
    scan_all = "--all-history" in sys.argv
    files = list(GATE_DOCS)
    if scan_all:
        files += [str(p.relative_to(REPO)) for p in (REPO / "diff-to-fix/evidence").rglob("*.md")]
    bad = {}
    checked = 0
    for rel in files:
        path = REPO / rel
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        # only backticked tokens (the citation form); bare in-prose hex
        # (line numbers, offsets) is not a commit cite
        for m in re.finditer(r"`([0-9a-f]{7,40})`", text):
            tok = m.group(1)
            # repair annotations legitimately NAME the phantom they
            # repaired - skip lines that carry a repair marker
            line_start = text.rfind("\n", 0, m.start()) + 1
            line_end = text.find("\n", m.end())
            line = text[line_start:line_end if line_end > 0 else len(text)]
            if any(kw in line for kw in ("phantom", "pre-rebase", "re-pinned",
                                         "mis-paste", "for the record",
                                         "magic constant", "as-recorded")):
                continue
            if tok in CROSS_REPO:
                continue
            if tok.startswith(DECOMPILE_PREFIXES):
                continue
            if is_run_id(tok) or looks_like_constant(tok):
                continue
            checked += 1
            if not resolves(tok):
                bad.setdefault(rel, []).append(tok)
    print(f"checked {checked} commit-cited tokens across {len(files)} gate docs")
    if bad:
        for f, toks in sorted(bad.items()):
            print(f"PHANTOM {f}: {sorted(set(toks))}")
        return 1
    print("all cited tokens resolve (or are the documented cross-repo set)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
