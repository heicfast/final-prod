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

5. PARITY-DELTAS D-number uniqueness (N-191/N-207, the born-collided-seat class):
   every D-<n> ROW heading in docs/PARITY-DELTAS.md (the "### D-<n>" and
   "**D-<n>" line-start forms) must be unique - two lanes landing registry
   rows on the same D-seat (D-118/D-135/D-136/D-137 at the hot-window
   crossings) breaks every D-number cross-reference downstream.

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

# N-154: the in-source "filed as <ID>" citation guard.
FILED_AS_CITE = re.compile(r"filed\s+as\s+(?:\*\*)?((?:A|S|M|N|I)-\d+|OQ-[A-Z0-9]+-\d+)")

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
LEDGER_ROW_ID = re.compile(
    r"^\|\s+((?:A|S|M|N|I)-\d+|OQ-[A-Z0-9]+-\d+)\s+\|", re.MULTILINE
)
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


def derive_abi_counts():
    """The tree's own truth: exports, error codes, event types."""
    hdr = (ROOT / "include" / "cr" / "cr_ui.h").read_text(encoding="utf-8", errors="replace")
    exports = sum(1 for l in hdr.splitlines() if "CR_UI_API" in l and "#define" not in l)
    events = len(set(re.findall(r"\bCR_EVT_[A-Z_]+\b", hdr)))
    err = (ROOT / "include" / "cr" / "cr_error.h").read_text(encoding="utf-8", errors="replace")
    codes = len(set(re.findall(r"0x5[0-9a-fA-F]{7}", err)))
    return exports, codes, events


def check_abi_count_claims():
    """N-150: the ABI count sites must state the tree's derived numbers.

    The export count is derived from cr_ui.h (CR_UI_API lines minus
    #defines), the error-code count from cr_error.h's distinct 0x5-codes,
    the event count from the CR_EVT_* census. Every count claim in the
    three API gate docs must match - the "81 exports as of <landing>"
    snapshot phrasing is how stale counts hid (the A-ABI-3 lesson);
    historical mentions (audit-time / counted at / was N) are exempt.
    """
    exports, codes, events = derive_abi_counts()
    ok = True
    claims = [
        (re.compile(r"(\d+) exported functions"),
         "export count", exports),
        (re.compile(r"(\d+) exports\b"),
         "export count", exports),
        (re.compile(r'(\d+)\s*`?extern "C" CR_UI_API`? definitions'),
         "export count", exports),
        (re.compile(r"error codes?\s*\*\*(\d+) \+ OK\*\*"),
         "error-code count", codes),
        (re.compile(r"(\d+)\+OK error codes"),
         "error-code count", codes),
        (re.compile(r"ABI event types?\s*\*\*(\d+)\*\*"),
         "event-type count", events),
    ]
    hist = re.compile(
        r"audit-time|counted at|was \d+|historical|drift|the PARITY-FIX|"
        r"claims? \d+|docs say", re.IGNORECASE)
    docs = [DOCS / "API.md", DOCS / "CORE-API-CURRENT.md", DOCS / "API-SURFACE-AUDIT.md"]
    for doc in docs:
        if not doc.is_file():
            continue
        lines = doc.read_text(encoding="utf-8", errors="replace").splitlines()
        for i, line in enumerate(lines):
            if hist.search(line):
                continue
            for pat, label, truth in claims:
                m = pat.search(line)
                if not m:
                    continue
                if int(m.group(1)) != truth:
                    print(
                        f"doc_truth_gate: FAIL: {doc.name}:{i + 1} claims "
                        f"{m.group(1)} ({label}) but the tree derives {truth} "
                        f"- refresh the count (the A-ABI-3 class)"
                    )
                    ok = False
    if ok:
        print(
            f"doc_truth_gate: ABI count sites match the tree "
            f"(exports={exports}, error codes={codes}, event types={events})"
        )
    return ok


def check_filed_as_citations():
    """Every in-source "filed as <ID>" cite resolves to a ledger row.

    An in-source audit cite that names a nonexistent finding number sends
    the next auditor chasing a phantom (the cr_log.cpp N-079 lesson,
    N-154 item 1 - the wrong-subject half of that class stays human-
    verified: the early ledger rows carry blank module cells, so a
    module-matching heuristic would be guesswork).
    """
    ok = True
    n_cites = 0
    ledger = set(LEDGER_ROW_ID.findall(
        (DIFF_TO_FIX / "FIXED-LEDGER.md").read_text(encoding="utf-8", errors="replace")
    ))
    for pat in ("src/**/*.cpp", "src/**/*.h", "include/**/*.h", "include/**/*.cpp"):
        for src in sorted(ROOT.glob(pat)):
            lines = src.read_text(encoding="utf-8", errors="replace").splitlines()
            for i, line in enumerate(lines):
                m = FILED_AS_CITE.search(line)
                if not m:
                    continue
                n_cites += 1
                if m.group(1) not in ledger:
                    print(
                        f"doc_truth_gate: FAIL: {src.relative_to(ROOT)}:{i + 1} "
                        f'cites "filed as {m.group(1)}" but the ledger has no '
                        f"such row (the phantom-finding class)"
                    )
                    ok = False
    if ok:
        print(
            f"doc_truth_gate: in-source filed-as citations clean "
            f"({n_cites} cite(s), all resolve to ledger rows)"
        )
    return ok



def check_bench_ioctl_codes():
    """N-195: every CTL literal used by bench/ scripts exists in cr_rda.h.

    The phantom-zero class (0x60002440 vs the contract's 0x6002440): a bench
    that calls an extended IOCTL with an extra zero digit can never exercise
    the path it exists to verify, and a green-looking run is impossible. The
    contract header (include/cr/win/cr_rda.h) is the single source of truth
    for every IOCTL code this repo may issue; any 0x6xxxxxxx-form literal in
    the bench family must byte-match one of its constants.
    """
    header = (ROOT / "include" / "cr" / "win" / "cr_rda.h").read_text(
        encoding="utf-8", errors="replace"
    )
    contract = {m.lower() for m in re.findall(r"0x6[0-9A-Fa-f]{6,}", header)}
    ok = True
    n_literals = 0
    for pat in ("bench/**/*.ps1", "bench/**/*.cs", "bench/**/*.cpp"):
        for src in sorted(ROOT.glob(pat)):
            text = src.read_text(encoding="utf-8", errors="replace")
            for i, line in enumerate(text.splitlines(), 1):
                # Skip comment lines that document the phantom class itself.
                if re.match(r"\s*(#|//|\*)", line):
                    continue
                for m in re.finditer(r"0x6[0-9A-Fa-f]{6,}", line):
                    n_literals += 1
                    if m.group(0).lower() not in contract:
                        print(
                            f"doc_truth_gate: FAIL: {src.relative_to(ROOT)}:{i} "
                            f"issues IOCTL {m.group(0)} which no constant in "
                            f"include/cr/win/cr_rda.h defines (the phantom-zero "
                            f"class, N-195)"
                        )
                        ok = False
    if ok:
        print(
            f"doc_truth_gate: bench IOCTL literals match the contract "
            f"({n_literals} literal(s), all in cr_rda.h)"
        )
    return ok


def check_parity_delta_numbering():
    """N-191/N-207: every PARITY-DELTAS D-row heading is unique."""
    rows = []
    p = DOCS / "PARITY-DELTAS.md"
    if not p.exists():
        print("doc_truth_gate: PARITY-DELTAS.md missing")
        return False
    for i, line in enumerate(p.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        # M-157's gate repair: the heading form must cover BOTH `### D-<n>`
        # AND `## D-<n>` (the S84 born-collided D-152s shipped as `##`-form
        # rows the `###`-only pattern never counted - three lanes claimed
        # the same seat under the gate's eye)
        m = re.match(r"^(?:#{2,3}|\*\*)\s*D-(\d+)\b", line)
        # N-207: an ADDENDUM heading is the deliberate continuation of its
        # parent row (the D-74 supersession form) - not a new seat claim
        if m and "ADDENDUM" not in line:
            rows.append((int(m.group(1)), i))
    seen, dupes = {}, []
    for num, line in rows:
        if num in seen:
            dupes.append((num, seen[num], line))
        else:
            seen[num] = line
    if dupes:
        for num, first, second in dupes:
            print(
                f"doc_truth_gate: PARITY-DELTAS duplicate D-{num}: "
                f"rows at :{first} and :{second} (N-191/N-207 class)"
            )
        return False
    print(f"doc_truth_gate: PARITY-DELTAS D-numbering unique ({len(rows)} rows)")
    return True


# N-211: public-header contract phrases that a landed finding has retired.
# A header comment asserting a retired contract is the doc-truth class the
# markdown checks cannot see (the gate scanned diff-to-fix/ only). Each
# entry: (the retired phrase, the finding that retired it). The phrase is
# searched case-sensitively over include/cr/**/*.h.
RETIRED_HEADER_CONTRACT_PHRASES = [
    ("never auto-saves", "N-211"),
    ("a cancelled run never auto-saves", "N-211"),
]

# N-211 rider (the S83 lane): the SAME retired-contract class in the CLI's
# autosave prose. The header landings re-truthed both headers but missed
# rrecov.cpp's completion-window comment ("cancelled runs return before
# this point, so ONLY completed runs auto-save" - stale since S-057's
# cancel-path call). These phrases are searched over src/app/rrecov.cpp
# alongside the header scan so the third site's class cannot recur either.
RETIRED_CLI_CONTRACT_PHRASES = [
    ("cancelled runs return before this point", "N-211"),
    ("ONLY completed runs auto-save", "N-211"),
]

# The surfaces the two phrase families scan: the public headers (the
# original N-211 gate) plus the CLI's own autosave contract comments
# (the S83 rider - rrecov.cpp owns the third site).
HEADER_PHRASE_SURFACES = [("include", "*.h", RETIRED_HEADER_CONTRACT_PHRASES),
                          ("src/app", "rrecov.cpp", RETIRED_CLI_CONTRACT_PHRASES)]


def check_header_contract_phrases():
    """Public headers must not assert contracts a landed finding retired.

    The N-211 class: cr_scan.h's autosave_dir comment claimed a cancelled
    run never auto-saves while the CLI (S-057's landing) auto-saves the
    stop window - a host integrator reading the header would drop the
    user's partial results. The finding's fix re-truthed the comment; this
    check keeps the retired phrase (and its family) out of every public
    header so the drift cannot silently recur while the gate runs.
    """
    ok = True
    hits = 0
    for surface_dir, surface_glob, phrases in HEADER_PHRASE_SURFACES:
        for header in sorted((ROOT / surface_dir).rglob(surface_glob)):
            try:
                lines = header.read_text(encoding="utf-8", errors="replace").splitlines()
            except OSError:
                continue
            for i, line in enumerate(lines):
                for phrase, finding in phrases:
                    if phrase in line:
                        rel = header.relative_to(ROOT)
                        print(
                            f"doc_truth_gate: FAIL: {rel}:{i + 1} asserts the "
                            f"retired contract phrase '{phrase}' (retired by "
                            f"{finding} - the header-contract drift class; "
                            f"re-truth the comment against the landed behavior)"
                        )
                        hits += 1
                        ok = False
    if ok:
        print("doc_truth_gate: header-contract phrases clean (N-211 class)")
    return ok


def main():
    ok = check_floorversion()
    ok = check_deleted_citations() and ok
    ok = check_cited_scripts_exist() and ok
    ok = check_finding_id_uniqueness() and ok
    ok = check_filed_as_citations() and ok
    ok = check_abi_count_claims() and ok
    ok = check_parity_delta_numbering() and ok
    ok = check_bench_ioctl_codes() and ok
    ok = check_header_contract_phrases() and ok
    if not ok:
        return 1
    print("doc_truth_gate: green")
    return 0


if __name__ == "__main__":
    sys.exit(main())
