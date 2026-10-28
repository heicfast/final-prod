#!/usr/bin/env python3
"""tools/doc_truth_gate.py - M-088: the CI-shape doc-truth gate.

Four assertions the consolidation's drift proved necessary:

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

3. No phantom script citations (N-160): every `scripts/<name>.py|sh|ps1`
   cited anywhere under diff-to-fix/ (top level, modules/, the five active
   severity dirs, findings/FIXED/, tracked-open/ - evidence/ excluded as
   the frozen proof set) must exist on disk, UNLESS the citing line (or its
   preceding line) self-identifies as historical documentation of a
   phantom/removed script (the I-034 lesson: a census cited two scripts
   that were never committed, so the census was not reproducible from its
   own citation).

4. Finding-ID uniqueness (N-160, the M-117 collision class): the title ID
   of every active finding file (findings/{ACUTE,SEVERE,MAJOR,MINOR,
   INFORMATIONAL}/ + tracked-open/) must be unique among active files and
   must not already exist as a FIXED-LEDGER row; and ledger rows sharing
   an ID must all cite the SAME findings/FIXED/<slug>.md (the convergent-
   record convention keeps both lanes' rows for one retirement - same
   slug, two rows - while two DIFFERENT slugs under one ID is the true
   collision the S41 lane raced into M-117).

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
DIFF_TO_FIX = ROOT / "diff-to-fix"
SCRIPTS = ROOT / "scripts"

DELETED_SURFACE = re.compile(
    r"driver-install\.yml|core-install\.yml|staged-scenarios\.yml|"
    r"actions/install-driver|api-benchmark\.yml|comparative-bench\.yml"
)
MARKERS = re.compile(
    r"historical|pre-consolidation|deleted|previous|git history|"
    r"was retired|historically|was dropped|superseded",
    re.IGNORECASE,
)

# N-160: the phantom-citation and collision guards.
SCRIPT_CITE = re.compile(r"(?<![\w/.-])(scripts/[\w][\w.-]*\.(?:py|sh|ps1))\b")
PHANTOM_MARKERS = re.compile(
    r"phantom|does not exist|do not exist|nonexistent|never committed|"
    r"never existed|was never|was not committed|was removed|was deleted|"
    r"never landed",
    re.IGNORECASE,
)
# Citations of ANOTHER repo's scripts (the driver repo is out of this tree
# per the two-repo grant) are cross-repo cites, not phantoms.
CROSS_REPO_MARKERS = re.compile(
    r"driver repo|rda-driver|driver-repo|out-of-tree|out of this repo",
    re.IGNORECASE,
)
FINDING_TITLE_ID = re.compile(
    r"^#\s+(?:ACUTE|SEVERE|MAJOR|MINOR|INFORMATIONAL)\s+((?:A|S|M|N|I)-\d+)\b"
)
OQ_TITLE_ID = re.compile(r"^#\s+(OQ-[A-Z0-9]+-\d+)\b", re.MULTILINE)
LEDGER_ROW_ID = re.compile(r"^\|\s+((?:A|S|M|N|I)-\d+|OQ-[A-Z0-9]+-\d+)\s+\|")
LEDGER_SLUG = re.compile(r"findings/(?:FIXED)/[\w.-]+\.md")


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


def _gate_scan_files():
    """The N-160 scan scope: diff-to-fix/ minus the frozen evidence set."""
    files = sorted(DIFF_TO_FIX.glob("*.md"))
    files += sorted((DIFF_TO_FIX / "modules").glob("*.md"))
    for sev in ("ACUTE", "SEVERE", "MAJOR", "MINOR", "INFORMATIONAL", "FIXED"):
        files += sorted((DIFF_TO_FIX / "findings" / sev).glob("*.md"))
    files += sorted((DIFF_TO_FIX / "tracked-open").glob("*.md"))
    return files


def check_cited_scripts_exist():
    """Every scripts/*.{py,sh,ps1} cited under diff-to-fix/ exists on disk.

    Lines (or their preceding line) that self-identify as documentation OF a
    phantom/removed script are exempt - the audit trail legitimately quotes
    dead citations when it documents their deadness - and so are citations
    of another repo's scripts (the driver repo, out of this tree per the
    two-repo grant; the M-086 class).
    """
    ok = True
    n_cites = 0
    for doc in _gate_scan_files():
        lines = doc.read_text(encoding="utf-8", errors="replace").splitlines()
        for i, line in enumerate(lines):
            for m in SCRIPT_CITE.finditer(line):
                n_cites += 1
                cited = m.group(1)
                if (ROOT / cited).is_file():
                    continue
                window = " ".join(lines[max(0, i - 1) : i + 1])
                if PHANTOM_MARKERS.search(window) or CROSS_REPO_MARKERS.search(window):
                    continue
                rel = doc.relative_to(ROOT)
                print(
                    f"doc_truth_gate: FAIL: {rel}:{i + 1} cites a script that "
                    f"does not exist on disk: {cited} (the phantom-citation "
                    f"class - commit the script or cite the inline method)"
                )
                ok = False
    if ok:
        print(
            f"doc_truth_gate: script-citation census clean "
            f"({n_cites} citation(s), all resolve)"
        )
    return ok


def check_finding_id_uniqueness():
    """Active finding IDs unique, un-retired; ledger ID->slug injective.

    Convergent ledger rows (two lanes recording one retirement) are legal
    only when every row for an ID cites the SAME findings/FIXED/<slug>.md.
    """
    ok = True
    active: dict[str, Path] = {}
    retired: dict[str, Path] = {}
    for sev in ("ACUTE", "SEVERE", "MAJOR", "MINOR", "INFORMATIONAL"):
        for f in sorted((DIFF_TO_FIX / "findings" / sev).glob("*.md")):
            head = f.read_text(encoding="utf-8", errors="replace")[:400]
            m = FINDING_TITLE_ID.search(head)
            if not m:
                continue
            fid = m.group(1)
            if fid in active:
                print(
                    f"doc_truth_gate: FAIL: finding ID {fid} claimed by two "
                    f"active files: {active[fid].name} and {f.name} "
                    f"(the M-117 collision class)"
                )
                ok = False
            else:
                active[fid] = f
    # N-191 (the second N-188): title IDs must also be unique across the
    # RETIRED files - two different findings under one ID in findings/FIXED/
    # is the same M-117 class even when only one carries a ledger row.
    for f in sorted((DIFF_TO_FIX / "findings" / "FIXED").glob("*.md")):
        head = f.read_text(encoding="utf-8", errors="replace")[:400]
        m = FINDING_TITLE_ID.search(head)
        if not m:
            continue
        fid = m.group(1)
        if fid in active:
            print(
                f"doc_truth_gate: FAIL: {fid} is claimed by both an active "
                f"file ({active[fid].name}) and a retired file ({f.name})"
            )
            ok = False
        elif fid in retired:
            print(
                f"doc_truth_gate: FAIL: {fid} is claimed by two retired "
                f"files: {retired[fid].name} and {f.name} (the M-117 "
                f"collision class)"
            )
            ok = False
        else:
            retired[fid] = f
    for f in sorted((DIFF_TO_FIX / "tracked-open").glob("*.md")):
        head = f.read_text(encoding="utf-8", errors="replace")[:400]
        m = OQ_TITLE_ID.search(head) or FINDING_TITLE_ID.search(head)
        if not m:
            continue
        fid = m.group(1)
        if fid in active:
            print(
                f"doc_truth_gate: FAIL: tracked-open ID {fid} also claims "
                f"{active[fid].name} (the M-117 collision class)"
            )
            ok = False
        else:
            active[fid] = f

    ledger_slug: dict[str, set] = {}
    ledger = DIFF_TO_FIX / "FIXED-LEDGER.md"
    for line in ledger.read_text(encoding="utf-8", errors="replace").splitlines():
        m = LEDGER_ROW_ID.match(line)
        if not m:
            continue
        fid = m.group(1)
        slug_m = LEDGER_SLUG.search(line)
        if slug_m:
            ledger_slug.setdefault(fid, set()).add(slug_m.group(0))
    for fid, slugs in sorted(ledger_slug.items()):
        if len(slugs) > 1:
            print(
                f"doc_truth_gate: FAIL: ledger rows for {fid} cite different "
                f"finding files: {sorted(slugs)} (the M-117 collision class - "
                f"convergent records must cite one slug)"
            )
            ok = False
    for fid in sorted(active):
        if fid in ledger_slug:
            print(
                f"doc_truth_gate: FAIL: {fid} is both an active finding "
                f"({active[fid].name}) and a FIXED-LEDGER row - the board "
                f"cannot hold both states for one ID"
            )
            ok = False
    if ok:
        n_rows = len(ledger_slug)
        print(
            f"doc_truth_gate: finding-ID uniqueness holds "
            f"({len(active)} active, {len(retired)} retired, "
            f"{n_rows} ledger-resolved, zero collisions)"
        )
    return ok


def main():
    ok = check_floorversion()
    ok = check_deleted_citations() and ok
    ok = check_cited_scripts_exist() and ok
    ok = check_finding_id_uniqueness() and ok
    if not ok:
        return 1
    print("doc_truth_gate: green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
