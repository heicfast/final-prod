#!/usr/bin/env python3
"""tools/doc_truth_gate.py - M-088: the CI-shape doc-truth gate.

Two assertions the consolidation's drift proved necessary:

1. FloorVersion single-floor discipline: every live `-FloorVersion vX.Y.Z`
   pin across .github/workflows/*.yml must (a) be exactly the set
   docs/CI-CURRENT.md declares, and (b) all carry the SAME version. The
   installer lane shipped at v0.2.3 for an era because no gate compared
   the pins (M-088's first defect).

2. No deleted-lane citation without a historical marker: the composite
   action (.github/actions/install-driver) and the three deleted workflows
   (driver-install.yml / core-install.yml / staged-scenarios.yml) plus the
   deleted api-benchmark lane may appear in docs ONLY next to an explicit
   historical marker (historical / pre-consolidation / deleted / previous /
   git history / was retired) - a bare current-tense citation is the
   PREBUILT-ARTIFACTS §2/§5 defect class (an engineer sent to a file that
   does not exist).

Exit 0 = truth holds; exit 1 with named violations otherwise.

Licensed under the MIT license. See LICENSE.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORKFLOWS = ROOT / ".github" / "workflows"
DOCS = ROOT / "docs"
README = ROOT / "README.md"

DELETED_SURFACE = re.compile(
    r"driver-install\.yml|core-install\.yml|staged-scenarios\.yml|"
    r"actions/install-driver|api-benchmark\.yml|comparative-bench\.yml"
)
MARKERS = re.compile(
    r"historical|pre-consolidation|deleted|previous|git history|"
    r"was retired|historically|was dropped|superseded",
    re.IGNORECASE,
)


def check_floorversion():
    """Every -FloorVersion pin in live workflows: one set, one floor."""
    pins = []
    for wf in sorted(WORKFLOWS.glob("*.yml")):
        text = wf.read_text(encoding="utf-8")
        for m in re.finditer(r"-FloorVersion\s+(v[0-9]+\.[0-9]+\.[0-9]+)", text):
            pins.append((wf.name, m.group(1)))
    if not pins:
        print("doc_truth_gate: FAIL: no -FloorVersion pins found in any workflow")
        return False
    versions = {v for _, v in pins}
    ok = True
    if len(versions) != 1:
        print(
            f"doc_truth_gate: FAIL: FloorVersion pins disagree: "
            f"{[(f, v) for f, v in pins]} (one floor per dependency - "
            f"docs/CI-CURRENT.md's contract)"
        )
        ok = False
    floor = next(iter(versions))
    ci_current = (DOCS / "CI-CURRENT.md").read_text(encoding="utf-8")
    declared = set(re.findall(r"-FloorVersion\s+(v[0-9]+\.[0-9]+\.[0-9]+)", ci_current))
    if declared != versions:
        print(
            f"doc_truth_gate: FAIL: CI-CURRENT.md declares {sorted(declared)} "
            f"but the workflows pin {sorted(versions)}"
        )
        ok = False
    if ok:
        print(
            f"doc_truth_gate: FloorVersion discipline holds: "
            f"{len(pins)} pin(s) at {floor} "
            f"({', '.join(sorted({f for f, _ in pins}))})"
        )
    return ok


def check_deleted_citations():
    """Deleted lanes may be cited only with a historical marker nearby."""
    ok = True
    files = sorted(DOCS.glob("*.md")) + [README]
    for doc in files:
        lines = doc.read_text(encoding="utf-8").splitlines()
        for i, line in enumerate(lines):
            if not DELETED_SURFACE.search(line):
                continue
            window = " ".join(lines[max(0, i - 2) : i + 1])
            if not MARKERS.search(window):
                rel = doc.relative_to(ROOT)
                print(
                    f"doc_truth_gate: FAIL: {rel}:{i + 1} cites a deleted "
                    f"CI surface in the present tense without a historical "
                    f"marker: {line.strip()[:100]}"
                )
                ok = False
    if ok:
        n = sum(
            len(DELETED_SURFACE.findall(l))
            for doc in files
            for l in doc.read_text(encoding="utf-8").splitlines()
        )
        print(
            f"doc_truth_gate: deleted-lane citations clean "
            f"({n} mention(s), all historically marked)"
        )
    return ok


def main():
    ok = check_floorversion()
    ok = check_deleted_citations() and ok
    if not ok:
        return 1
    print("doc_truth_gate: green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
