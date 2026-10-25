# MINOR N-007 / INFORMATIONAL I-004 — ReFS detection strictness (signature width) and version-gate superset

Two small ReFS detection divergences filed together.

---

## N-007 — Page-signature matching width

**Severity:** MINOR · **Module:** refs · **Parameters:** B1, C19
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c`
:11689–11720 (agent-B verified): signature dispatch selects the string
("SUPBMSB+ReFSFSRS" / "CHKPSUPBMSB+ReFSFSRS" / "MSB+ReFSFSRS") but the comparison is
**first-DWORD only** (`*param_2 == *(int*)pcVar4`).
**Core references:** `src/core/refs/cr_refs_detect.cpp:39–50` — full-width `memcmp` (16/20/12
bytes) per signature — audited commit `67193b6`.

**Gap:** a metadata page whose first 4 bytes match a ReFS signature but whose later
signature bytes are corrupt is accepted by the og (then validated by the ref-parser) and
rejected outright by the core. Corrupt-page tolerance only; the core's probes then find no
ReFS at all on volumes where every page has late-byte corruption (rare).

**Fix plan:** compare first 4 bytes for page *discovery*, keep the full memcmp as a
*grading* signal (page flagged `signature_partial`, volume still opened). Files:
`src/core/refs/cr_refs_detect.cpp`.
**Tests:** `Refs_Partial_Signature_Page_Detected` (first-4 match + corrupt tail → found,
flagged).

---

## I-004 — Version-gate superset (1.x–3.x accepted; og gates 3.4+/1.x)

**Severity:** INFORMATIONAL (deliberate, documented in code) · **Module:** refs ·
**Parameters:** B1, D15
**Corpus references:** `re-workspace/artifacts/SearchDLL/refs-engine--deep-dive.md` §3
(engine distinguishes ReFS versions/layouts; og gate = major==3 && minor>3 on the
checkpoint body + a separate major==1 path, per the corpus re-validation notes).
**Core references:** `src/core/refs/cr_refs_engine.cpp:359–364, :384–387` — accepts
major 1..3 with full shape validation; comment documents "wider recovery scope, same
structure walk" — audited commit `67193b6` (includes concurrent GAP-S6-3 fix `7797bd6`:
checkpoint-body minor gate + dynamic body offset).

**Gap:** ReFS 2.x and 3.0–3.3 volumes are refused by the og's gate and attempted by the
core. Recovery-positive superset; recorded so parity benches don't count 2.x/3.0–3.3 yield
against the og.

**Fix plan:** none needed — register in `docs/parity/PARITY-DELTAS.md` if not already a
row.

---

**Related findings:** A-001 (partition-table side of detection), the ReFS module file.
**Open questions:** none.


## I-004 Resolution

**Status:** ACKNOWLEDGED (deliberate, recovery-positive superset — preserved)
**Verified by:** the code comment documents "wider recovery scope, same
structure walk"; the og gate (major==3 && minor>3 + major==1 path) is
corpus-documented; the rationale holds. Registered this session: PARITY-DELTAS
D-48b (first row; parity benches must not count ReFS 2.x / 3.0–3.3 yield
against the og). No code change — future work must PRESERVE the superset.
