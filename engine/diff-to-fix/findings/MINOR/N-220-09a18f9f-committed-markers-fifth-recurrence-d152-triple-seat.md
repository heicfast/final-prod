# MINOR N-220 — the FIFTH committed-marker red-main recurrence (`09a18f9f`, the S82 registration batch): six trail files pushed with live conflict markers — plus the born-collided D-152 TRIPLE seat the gate's `###`-only pattern never saw

**Severity:** MINOR (the class convention — M-120 `db228b75`, N-210 `f3da99f1`, N-217 `ed501caa`, N-219 `27bda40e`, now `09a18f9f`; all repaired in the same session before any consumer pulled the broken trail; but the recurrence count is now five and the D-seat half shipped a triple collision the gate provably missed)
**Module:** audit-trail integrity (the M-120/N-210 class) + the PARITY-DELTAS registry
**Parameters affected:** Q68 (audit-trail truth), the D-seat uniqueness invariant (N-191/N-207), the doc_truth_gate's own coverage
**Corpus references:** none for the marker defect (a core-repo integrity defect)
**Core references:** found at core `cac42582` (origin/main HEAD at this lane's pull): unresolved `<<<<<<<`/`=======`/`>>>>>>>` blocks in `diff-to-fix/CROSS-REF-MATRIX.md` (the M-154 row block), `diff-to-fix/FIXED-LEDGER.md` (a NESTED double conflict at the I-048/M-152/N-145 vs I-048/M-111/M-114 row sets, plus duplicate N-145/N-178 rows), `diff-to-fix/findings/FIXED/I-048-three-registers-...md` (the Resolution section), `docs/PARITY-DELTAS.md` (TWO conflicts — a separator block and the D-155-combined vs D-160/161/162-renumbered nested block), and `worklog.md` (the Session-80-entry-10 vs Session-82-entry-6 block). Verified first-hand: `grep -rn '^<<<<<<< HEAD'` over the tree → 6 files (the sixth, `findings/FIXED/M-120`, is the intentional evidence quote — excluded). The D-seat half: `grep '^## D-152' docs/PARITY-DELTAS.md` → 2 hits plus the legitimate `### D-152` at :4218 — THREE sections under one seat.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The S82 registration batch push (`09a18f9f`) committed unresolved conflict markers into six trail files — the fifth red-trail window of the M-120/N-210 class. The interleavings were semantic, not line-noise: the matrix's M-154 row existed in both its stale DIVERGENCE-MAJOR form (HEAD) and its landed MATCH form (incoming); the ledger carried a nested double conflict stacking two different row sets; PARITY-DELTAS carried the superseded D-155-combined I-048 registration (HEAD) against the renumbered D-160/D-161/D-162 split rows (incoming); the worklog interleaved two lanes' session entries.

Beneath the markers, the push also left a born-collided D-152 TRIPLE seat: N-145's journal-location row (`### D-152`, :4218 — the first landing, keeps the seat), the ext4 sweep's two-guards section (`## D-152`, :4656 — M-152's residue, its mode-gate half duplicating D-153), and the last-scan pointer location (`## D-152`, :4680 — M-154's registration). The doc_truth_gate's D-row pattern `^(?:###|\*\*)\s*D-(\d+)` matches only `###`-form headings — both colliding sections shipped as `##`-form rows the gate never counted, which is why the gate printed "PARITY-DELTAS D-numbering unique (154 rows)" over a file carrying three D-152s.

## The repair (executed this session)

1. **Matrix:** the M-154 row conflict resolved to the incoming MATCH row (the landed fix is the newer truth; the stale DIVERGENCE row dropped with the markers).
2. **Ledger:** the nested double conflict resolved to the union of both sides' distinct rows — the incoming side's I-048 (S69 form), M-111, M-114 rows kept; the HEAD side's M-152/N-145 duplicates (already present above the block) dropped; the pre-existing duplicate N-145 and N-178 rows deduped.
3. **I-048 finding:** the Resolution conflict resolved to the incoming (S82 registration) side.
4. **PARITY-DELTAS:** both conflicts resolved — the separator kept; the D-155-combined I-048 section dropped in favor of the renumbered D-160/D-161/D-162 rows it was superseded by.
5. **D-seat repair (the 67d2bf25 convention):** N-145's row keeps D-152 (the first landing); the ext4 two-guards section renumbered **D-163** with its mode-gate half removed as D-153's duplicate (the journal-inode claim is the seat's content, cross-referenced from M-152's Resolution); the last-scan pointer section renumbered **D-164**. Cross-references swept: `modules/ext4.md` (D-163/D-153), `modules/session.md` (D-164), M-152's Resolution (D-163), `SUMMARY.md`'s M-152/M-154 rows (D-163/D-153, D-164).
6. **Gate hardening:** the doc_truth_gate D-row pattern widened to `^(?:#{2,3}|\*\*)\s*D-(\d+)` — the `##`-form blind spot that admitted the triple collision is closed; the gate re-run green over 156 rows (the two renumbered rows now counted).
7. **Worklog:** the entry block resolved both-sides-kept (the Session-80-entry-10 record and the Session-82-entry-6 record both preserved verbatim — the worklog is append-history and neither lane's entry was forfeited).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S84 fix lane, 2026-10-30)
**Verified by:** `grep -rn '^<<<<<<< HEAD'` over the tree → only `findings/FIXED/M-120` (the intentional evidence quote); `python3 tools/doc_truth_gate.py` → green with "PARITY-DELTAS D-numbering unique (156 rows)" (was 154 — the two `##`-form rows invisible before the pattern fix); the D-163/D-164 cross-references resolve (`grep -n 'D-163\|D-164'` over diff-to-fix/ and docs/ → every reference lands on a real section); the full suite unaffected (trail-only change).
**Resolution notes:** The both-sides-kept resolution preserved every lane's record — the stale HEAD rows that were genuinely superseded (the matrix's pre-fix M-154 DIVERGENCE row, PARITY-DELTAS's D-155 combined registration) were the ONLY deletions, and both deletions are cases where the same lane's newer form exists in the same push's incoming side. The recurrence root cause is unchanged since N-210: a lane pushing without running the marker gate (`scripts/lane-push.py`'s checks or `ci-local.sh`) before push. The new hazard this recurrence adds — the gate's own pattern blind spot — is closed at the gate itself so the next `##`-form collision cannot ship green.
