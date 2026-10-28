# MINOR N-090 — rda-driver stale session-code counts: the served 19-code set described as "15" or "18" across 14+ sites

**Severity:** MINOR
**Module / Surface:** transport (the rda-driver wire contract's documentation surface)
**Parameters affected:** P47-adjacent (the session-protocol command set — the count/range prose), the wire-contract docs
**Corpus references:** `artifacts/PartitionRecovery/eudcpepm-euedkepm--hook-drivers.md` (the EUDCPEPM custom-IOCTL table: 15 documented commands 0x800C2AC8-0x800C2B38); the G28 decode (rda-driver worklog) that found the discovery + RRWR families
**Core references (rda-driver @ 323e75a):** `src/shared/inc/rda_ioctl.h:175-201` (the 19 `RDA_SESSION_CODE_*` defines — the ground truth), and the stale sites enumerated below
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** expanded on the SCAN-2f driver-lane handoff D3 ("the '15/18-command' prose vs the 19-code set is now stale in seven driver-repo sites incl. docs/ground-truth-contracts.md §5.2") — this session's full-text pass found 14+ stale sites (the "seven" undercounted), with a per-site nuance the handoff lacked

## Summary

The driver serves a 19-command session protocol (19 explicit `RDA_SESSION_CODE_*`
defines, matched explicitly in `RdaIsSessionCode` since G28), but the repo's
prose still describes it as a 15-command or 18-command protocol in more than
a dozen places — including the header that defines the 19 codes, a test that
CHECKs all 19, and the reference-verification script that asserts all 19.
The nuance: sites that describe **the og's EUDCPEPM reference set** as "15
commands (0x800C2AC8-0x800C2B38)" are CORRECT (the corpus documents 15);
sites that describe **our served set** with 15 or 18 are stale. The routing
code and the checks are correct — this is documentation drift only, but it
is exactly the drift class that misleads a future session auditing the wire
contract.

## What the reference does

The corpus's hook-drivers artifact documents EUDCPEPM's custom IOCTL table
as 15 commands spanning 0x800C2AC8-0x800C2B38 (normalized). The binary
additionally dispatches a discovery code (0x800C2AC0) and the RRWR filter
family (0x800C2B3E/B40/B42) that the artifact missed — the G28 decode —
for a true reference-dispatched set of 19.

## What our implementation does

`rda_ioctl.h:183-201` defines 19 codes (DISCOVERY AC0 through RRWR_READ
B42); `rda_ioctl.c:40-60` matches all 19 explicitly. The stale sites
(verified first-hand this session):

**Sites describing OUR served set as 15 (stale, range also wrong):**
- `src/shared/inc/rda_ioctl.h:11-12` — the header's own file-map comment
  ("15-command copy-session protocol") sits in the file that defines 19;
- `src/driver/inc/rda_p.h:13` — "copy-session engine (15-command protocol)";
- `src/driver/rda_registry.c:14` — "serve the 15-command copy protocol";
- `README.md:31` — "the 15-command copy-session protocol … preserved
  byte-semantically";
- `README.md:42` — "IoctlBase-normalized 15-command session engine
  (`0x800C2AC8 … 0x800C2B38`)" — count AND range omit AC0 + RRWR;
- `docs/architecture.md:19` — the merge table's our-driver column:
  "Parameters\FilterMode role + 15-command session protocol";
- `docs/architecture.md:72` — "IoctlBase-normalized `0x800C2AC8 … 0x800C2B38`
  (15 commands)";
- `docs/requirements.md:56` — "EUDCPEPM protocol …: IoctlBase-normalized
  15 commands 0x800C2AC8-0x800C2B38" (the requirement as implemented is 19);
- `docs/ground-truth-contracts.md:194` — "custom range served:
  0x800C2AC8 … 0x800C2B38 (15 commands)" — "served" is our set.

**Sites describing OUR set as 18 (stale by one):**
- `src/shared/inc/rda_ioctl.h:154` — "decoded the FULL command set (18
  codes)" directly above the 19-row table;
- `tests/unit/test_compat.c:229` — "G28: the full 18-command set" above 19
  CHECKs;
- `scripts/verify/verify_reference_binaries.py:30` — "all 18 command codes"
  above the 19-code assertion tuple (:214-232).

**Structural:**
- `src/driver/rda_ioctl.c:6-7` — the routing header comment still
  describes the pre-G28 range `0x800C2AC8..0x800C2B38` (the code below it
  matches all 19 explicitly);
- `docs/ground-truth-contracts.md` §5.2 — the command table carries 15
  rows (the reference set); the discovery + RRWR rows exist in code and in
  the G28 note but not in the table.

**Correct-as-written (no change needed):** the EUDCPEPM-reference bullets —
`docs/ground-truth-contracts.md:11`, `:55`, and `rda_ioctl.h:11`'s EUDCPEPM
bullet — where "15" describes the corpus's documented reference set.
`worklog.md:20/:77` are historical entries (state-at-time records).

## Gap

The repo's authoritative wire-contract documents (the header, README,
architecture, requirements, ground-truth contracts) state the wrong served
count/range in 12+ reader-facing places while the code serves 19. A future
audit or integration session reading the docs would probe 15 or 18 codes
and mis-assess the contract.

## Impact

Documentation-only (no behavioral defect: routing, tests, and verification
all handle 19). The cost is audit/integration friction and the
truthfulness of the contract docs the core repo's DRIVER-CONTRACT.md
cross-references.

## Reproduction / verification

1. `grep -c "#define RDA_SESSION_CODE" src/shared/inc/rda_ioctl.h` → 19.
2. The per-site reads quoted above (all verified first-hand this session).
3. `rda_ioctl.c:42-43` — the G28 comment itself states the prior range
   "missed the discovery code (0x800C2AC0) and the RRWR family
   (0x800C2B3E/B40/B42)".
4. The D3 handoff record: SCAN-ENGINE-DELTA.md:327-329.

## Fix plan (for the engineering team)

1. One docs-and-comments pass over the rda-driver repo (no code change).
2. Files: `src/shared/inc/rda_ioctl.h`, `src/driver/inc/rda_p.h`,
   `src/driver/rda_registry.c`, `src/driver/rda_ioctl.c`,
   `tests/unit/test_compat.c`, `scripts/verify/verify_reference_binaries.py`,
   `README.md`, `docs/architecture.md`, `docs/requirements.md`,
   `docs/ground-truth-contracts.md`.
3. The change: our-set prose becomes "19" with the full range
   `0x800C2AC0 … 0x800C2B42` (noting the non-contiguity — matched
   explicitly per G28); reference-set prose keeps "15 (the corpus's
   documented set; +discovery/RRWR found in-binary)". §5.2's table gains
   the four missing rows (AC0 discovery, B3E/B40/B42 RRWR) marked as
   in-binary-found, or a pointer note to the G28 list.
4. Test: none functional; optionally a docs-consistency grep gate
   (`grep -n "15-command\|18-command\|18 codes" ...` returns empty) in the
   lint lane.
5. Contract updates: note the correction in the core repo's
   `docs/DRIVER-CONTRACT.md` cross-reference if it cites a count (it
   already says 19 — verify only).

## Tests to add

- `Lint.NoStaleSessionCodeCounts` (grep gate) — fails on the current tree,
  passes after.

## Related findings

- N-091 — the same repo's other doc-vs-code truthfulness defects.
- The G28 landing (rda-driver worklog) — the code-side fix this doc pass
  completes.

## Open questions

None.
