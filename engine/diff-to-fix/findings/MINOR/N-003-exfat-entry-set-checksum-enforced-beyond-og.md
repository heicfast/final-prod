# MINOR N-003 — exFAT entry-set checksum enforced on live sets; the og engine does not verify it at all

**Severity:** MINOR
**Module:** exfat
**Parameters affected:** B4 (grading), D15 (confidence grading model), B3 (false-positive/strictness)
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c` — agent-B search for ROL15/ROL31 accumulator patterns returned **no dentry-checksum loop** (verified absence, B-exfat.md §3.4); structural validation quote :1846–1875
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:303–311` (live-set checksum), `:407` (PartialRisk downgrade), `include/cr/exfat/cr_exfat_engine.h:70` (`validate_entry_sets = true` default) — audited commit `67193b6`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og exFAT engine validates dentry triples **structurally** (type classes via
`(t−X)&0x7F`, reserved-field zeros, first-cluster gate) and contains no SetChecksum
verification anywhere in the decompiled engine. Our core additionally validates the 16-bit
entry-set checksum on live sets and downgrades mismatches to `Confidence::PartialRisk`
(files are kept — grading-only strictness). Live files with torn checksums (crash-during-write,
bit rot) display worse recovery chances than the og would show, and count in `sets_damaged`
telemetry.

## What the reference does

Agent-B's verified-absence search across `exFATSearchDLL_decompiled.c` (1,299 functions) for
the ROL15/ROL31 checksum accumulator patterns found no entry-set checksum loop; the dentry
validator at :1846–1875 (quoted in `evidence/decompiled-quotes/exfat-dentry-validation.md`)
performs only structural checks. Deleted and live dentries pass through the same
`(t−X)&0x7F` class tests.

## What our core does

`src/core/exfat/cr_exfat_quick.cpp:303–311`:

```cpp
/* entry-set checksum (live sets only - deleted sets have
 * checksum over live values anyway) */
bool set_ok = true;
if (ctx.flags.validate_entry_sets && in_use) {
    u16 chk = entry_set_checksum(buf.data() + i * 32, 1 + 1 + name_entries);
    if (chk != fe->set_checksum) {
        set_ok = false;
        ++ctx.stats->sets_damaged;
    }
}
```

and `:407`: `conf = set_ok ? tree::Confidence::Excellent : tree::Confidence::PartialRisk;`

## Gap

Falsifiable claim: for a live exFAT file with a wrong stored SetChecksum, the og grades the
file normally (no checksum input exists) and our core grades it `PartialRisk`.

## Impact

- Grading divergence only (result-set membership identical — the file is still emitted and
  still recovers its bytes).
- User-visible: "Recovery Chances" column shows a worse grade for a file that is actually
  intact — mild trust erosion, no data loss.

## Reproduction / verification

- Corpus-derived: verified-absence search (documented method + pattern list in B-exfat.md
  §3.4); structural validator quote.
- Core-derived: code quotes.
- Executed: unit-level — mutate `set_checksum` byte on a live entry in a synthetic image →
  assert `Confidence::PartialRisk` (current) — the og-side expectation (no downgrade) is
  decompile-derived.

## Fix plan

1. Default `validate_entry_sets` to `false` for og-parity scan profiles, keeping `true`
   available for honest-mode scans (flag already exists — wire a scan-profile preset).
2. When enabled (honest mode), keep the downgrade-to-PartialRisk behavior (never drop).
3. Files: `include/cr/exfat/cr_exfat_engine.h` (default), orchestrator scan-profile wiring
   (`src/core/orchestrator/cr_scan.cpp` engine flags block).
4. Register the deviation direction in `docs/parity/PARITY-DELTAS.md` whichever default is
   chosen.

## Tests to add

- `test_exfat_volume.cpp::Live_Set_Bad_Checksum_Grades_Excellent_In_Og_Profile`: with the
  og-parity profile, mutated checksum → `Confidence::Excellent` (fails before, passes after).
- `test_exfat_volume.cpp::Live_Set_Bad_Checksum_Downgrades_In_Honest_Profile`: with
  `validate_entry_sets=true` → `PartialRisk`, node still emitted (pins current behavior).

## Related findings

- OQ-T5 (boot-checksum predicate — the other exFAT checksum parity question).

## Open questions

None.
