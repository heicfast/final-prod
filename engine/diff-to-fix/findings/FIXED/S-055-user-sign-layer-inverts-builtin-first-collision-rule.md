# SEVERE S-055 — the user-sign layer probes BEFORE the built-in raw-sign table: the M-048/D-104 "built-in-first" collision rule is inverted in code, flipping 8 overlap formats to device-end extents

**Severity:** SEVERE
**Module:** carve (user-signature channel)
**Parameters affected:** carving validator logic (28), detection correctness (1), false-positive rate (3), merge/dedup strategy (29), sort/model of the carve channel
**Corpus references:** og has NO user-sign carve channel (verified: zero EUUserData references in RAWSearchDLL_decompiled.c — 80,615 lines swept; sole consumer = DRWUI's CHK repair chain, `fopen("%s/EUUserData","r")` DRWUI:1005387/:1005412, errata CORRECTIONS.md #9 = E-A/E-B); reference layering among the og's OWN layers: grammar analysers claim positions before the raw-sign table (RAWSearchDLL dispatch, SCAN-ENGINE-DELTA F-rows)
**Core references:** `src/core/carve/cr_carve.cpp:725-808` (the user-sign block), `:830` (the built-in probe it precedes), comment `:727-733`, `include/cr/carve/cr_usersigns.h:85-87`, `tests/unit/test_carve.cpp:6983-7018` (the pin test that only covers grammar preemption); default set `data/userdata-defaults.json` (D-104, shipped 111 keys/190 signs); audited at core `8d0d88b8`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The in-code contract says a user sign colliding with a BUILT-IN raw sign "fires only when
the built-ins did not match — built-in-first". The code does the opposite: the user-sign
probe (`usersigns::match_at`, :738-740) runs in the same loop iteration BEFORE
`idx.probe_header` (:830, the 139-format built-in table) and `continue`s on hit (:805), so
the built-in table is never consulted at a user-sign hit. In the shipped default-on
configuration (userdata-defaults.json), the 8 formats present in BOTH sets — **amr, cap,
class, cpl, flac, msf, pdb, wpl** — flip channels at their magics: instead of a bounded
raw-sign candidate (min/max size gates, tail pairing, table confidence) the core emits a
user-sign record whose extent runs to DEVICE END, confidence 600, tag 0xFFFE0000.

## What the reference does

The og's scan engine has no user-sign input at all (E-A/E-B, re-verified this session by
lane C first-hand: zero references in RAWSearchDLL; the file's only runtime consumer is
DRWUI's CHK repair chain). Within the og's own layers, the grammar analysers claim a
position first and the raw-sign table probes unclaimed positions — the built-in-first
ordering the core's own M-048 landing comment specified for its superset channel. The
og's raw-sign table rows carry per-format min/max sizes and tail signs (raw_signs.json,
139 ext; read at engine init, RAWSearchDLL :23836-23850) — i.e. the bounded grammar the
collision rule exists to preserve.

## What our core does

`src/core/carve/cr_carve.cpp` — the loop over unclaimed positions:

```c
725      if (emitted_at.count(start_abs))
726          continue; /* a grammar-validated file starts here */
727      /* M-048: the user-extensible signature layer ... before the built-in raw-sign
730       * table (a user sign for an already-probed position is a duplicate by
731       * construction; a user sign colliding with a BUILT-IN fires only when the
732       * built-ins did not match - built-in-first, the collision rule the finding
733       * specifies). */
738      if (usersigns::active_count() != 0) {
739          if (const carve::usersigns::UserSign* us =
740                  usersigns::match_at(win + off, want - off)) {
741              const u64 len = ctx.total - start_abs;   /* extent to DEVICE END */
...
769              rsig.ext = us->ext;
...
803              ctx.sign_hits.fetch_add(1, std::memory_order_relaxed);
804              emitted_at.insert(start_abs);
805              continue;                               /* built-in never runs here */
806          }
807      }
...
830      idx.probe_header(win + off, want - off, matches);   /* the built-in table */
```

The comment (:727-733) and the header contract (cr_usersigns.h:85-87: "built-ins were
already probed by the earlier layers at the same stride") state built-in-first; the code
executes user-first. The pin test `CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition`
(test_carve.cpp:6983-7018) only exercises GRAMMAR-layer preemption (it plants a full valid
PNG — a grammar format, not a raw-sign-table format), so the raw-sign collision is
untested and the inversion survives.

## Gap

At any position where a default-set user sign and a built-in raw sign both match, the
user sign wins and the built-in's bounded candidate is never produced. Falsifiable: a
FLAC header at offset X on a default-config scan → core emits one user-sign record
`FLAC_000001.FLAC` with `len = total - X` (device end), tag 0xFFFE0000; the bounded
built-in flac candidate (min/max gates + tail pairing) is absent from the tree.

## Impact

- Wrong emission shape for the 8 overlap formats in the DEFAULT configuration: oversized
  device-end extents replace bounded candidates; the recovered "file" is the real header
  plus the entire remainder of the disk.
- Every grammar-REJECTED common-format magic in free space (truncated JPG/MZ/OLE — all
  present in the default DB) becomes a device-end user-sign file (the grammar layer
  rejects, then the user-sign layer accepts the same bytes).
- Confidence/grading bypass: the user-sign fixed 600 / PartialRisk path replaces the
  table's per-format confidence.
- Downstream: dedup-by-extent-class (F9) sees a different extent class; the S-049 namer
  names it via the user ext; sessions persist the bogus size.

## Reproduction / verification

Verified first-hand this session (lead auditor): the code block order read directly at
cr_carve.cpp:725-830 (quotes above) — the user-sign probe at :738 precedes
`idx.probe_header` at :830 in the same iteration, with `continue` at :805. The overlap
set computed by the lane: EUUserData keys ∩ built-in kFormats exts = {amr, cap, class,
cpl, flac, msf, pdb, wpl} (userdata-defaults.json vs cr_rawsigns_data.cpp). D-104's
registration text (docs/PARITY-DELTAS.md:3172-3210) registers the channel as a superset
but records no ordering divergence; the "gated by the same format-toggle rules" claim is
the separate S-056 defect.

## Fix plan (for the engineering team)

1. Reorder the layers at the collision point: run `idx.probe_header` (:830) BEFORE the
   user-sign probe, and let the user-sign block fire only when `matches.empty()` after
   the disabled-format filter — matching the M-048 comment's specified contract. Keep
   grammar-layer preemption exactly as is (emitted_at :725).
2. Files to modify: `src/core/carve/cr_carve.cpp` (move the :738-808 block after the
   :830-849 built-in best-pick, gated on no surviving built-in match); update the comment
   (:727-733 stays, now true).
3. Algorithm note: the zero-stride fast-skip (:817-829) must stay ahead of the user-sign
   probe only if the og's user-scan semantics require it — with the reorder, the skip can
   cover both layers (it already reasons about all header signs).
4. Tests (below): one per overlap format plus the grammar-rejected-magic case.
5. Contract: cr_usersigns.h:85-87 becomes true; D-104 needs no change (its text never
   claimed the inversion).
6. Acceptance criteria: on the default set, a FLAC (and each of the other 7 overlap
   formats) yields the bounded built-in candidate — same record as a scan with user signs
   disabled — and zero 0xFFFE0000-tagged records at those magics; the
   `UserSignatureYieldsToBuiltInAtSamePosition` test gains a raw-sign-layer leg.

## Tests to add

- `Carve.UserSignYieldsToRawSignTableAtSamePosition` (plants a flac magic that both
  layers match; asserts the bounded built-in record, not the device-end one) — fails today
- One parametrized variant per overlap format (amr/cap/class/cpl/flac/msf/pdb/wpl)
- `Carve.UserSignStillFiresWhenNoBuiltinMatches` (the channel stays functional)
- `Carve.GrammarRejectedMagicNotReclaimedByUserSign` when the default set is loaded
  (truncated-JPG class) — decide + pin the intended semantics (the og has no analog;
  the M-048 contract implies no reclaim)

## Related findings

- S-056 (the disabled-format bypass in the same block — fix in the same patch)
- N-171 (user-sign files never tick carved_found — same block's stats leg)
- D-104 / M-048 lineage (the channel's registration and landing)

## Open questions

None on the divergence; one product question recorded for the fix lane: whether
grammar-REJECTED magics should remain reclaimable by user signs after the reorder (the
M-048 contract implies no; the test list pins whichever the team decides).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-055+S-056 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `Carve.UserSignYieldsToRawSignTableAtSamePosition` (the finding's falsifiable case: a FLAC header at a stride on the default configuration — RED pre-fix with the device-end 0xFFFE0000 record, GREEN post-fix with the bounded built-in candidate tag 0xFFFF0000+35, conf 940, zero user-tagged records), `Carve.UserSignOverlapFormatsAllYieldToBuiltin` (all 8 overlap formats, table-driven from the built-in sign array — each planted magic yields a built-in record and no user record; the cap magic is claimed by the higher-confidence pcap sibling row, conf 1000 vs 910 — the best-pick's own rule, pinned as any-built-in-tag), `Carve.UserSignBuiltinFloorGateSuppressesUserSignToo` (a built-in magic below the format's min_size floor yields NOTHING — the built-in's match settles the stride, no user reclaim), `Carve.GrammarRejectedMagicNotReclaimedByUserSign` (the no-reclaim decision pinned: a truncated-JPG magic rejected by the grammar layer produces no user-sign record), `Carve.UserSignStillFiresWhenNoBuiltinMatches` (the channel stays functional: an unclaimed DEADBEEF magic still emits), `CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition` + `CarveEngine.UserSignatureProbedAtStride` (the pre-existing pins stay green). Suite 1192/1192 green (linux-release).
**Resolution notes:** CONFIRMED-DIVERGENCE, re-verified first-hand: the corpus layering re-read (the og's grammar-claims-then-table-probes order; EUUserData zero-references in RAWSearchDLL per E-A/E-B) and the code order at the current HEAD (user-sign probe at :884 preceding probe_header at :978 with continue at :953). The fix is the finding's own step 1: the loop body reordered — built-in arm (probe_header + M-024 filter + best-pick/tail/emission) runs first and sets builtin_matched; the user-sign arm runs only when no built-in survived the disabled-format filter (exactly the "matches.empty() after the filter" gate the finding specified — a built-in match whose format is disabled lets the user arm fire). The N-031 zero-stride fast-skip now guards the BUILT-IN arm only: user signs anchor up to kMaxOffset+48 bytes out (the og's zip sign sits at +526), so the user arm still probes zero-head strides (the finding's step 3 question answered — the skip does NOT cover both layers; the pre-fix order never let it gate user signs either, so no behavior was lost). The no-reclaim product question (the finding's open item) DECIDED + PINNED: a grammar-REJECTED start is not reclaimed by the user arm — rejected_at (memory-bounded at 2^20 entries) threaded beside emitted_at; the built-in table still probes rejected positions (the og's layers claim by emission only). D-104's registration text gains the landing notes (the ordering it described is now the code's truth; the no-reclaim rule registered as the core-side rule with no og analog). Two test-magic corrections discovered during verification: CAFEBABE is the built-in class magic (a claimed magic — unusable as an "unclaimed" fixture), and cap's D4C3B2A1 sign is shared with the higher-confidence pcap row — both documented in the test comments.
