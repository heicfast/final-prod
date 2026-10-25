# MINOR N-003 — exFAT entry-set checksum enforced on live sets; the og engine does not verify it at all

**Severity:** MINOR
**Module:** exfat
**Parameters affected:** B4 (grading), D15 (confidence grading model), B3 (false-positive/strictness)
**Corpus references:** decompiled `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c` — agent-B search for ROL15/ROL31 accumulator patterns returned **no dentry-checksum loop** (verified absence, B-exfat.md §3.4); structural validation quote :1846–1875
**Core references:** `src/core/exfat/cr_exfat_quick.cpp:303–311` (live-set checksum), `:407` (PartialRisk downgrade), `include/cr/exfat/cr_exfat_engine.h:70` (`validate_entry_sets = true` default) — audited commit `67193b6`
**Status:** CLOSED — FIXED in `fb44073` (verified independently by the session-2 audit: ExfatScanFlags::validate_entry_sets default now false at include/cr/exfat/cr_exfat_engine.h:78 — og parity by default, honest strictness one explicit flag away (D-47 registered); tests SetChecksumMismatchGradesExcellentInOgProfile (default) + SetChecksumMismatchMarksDamaged (honest-mode pin) green in the 313/313 ASan suite; see Resolution)

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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for `fix: N-003`)
**Verified by:** fail-before/pass-after — with the header flip reverted,
`ExfatEngine.SetChecksumMismatchGradesExcellentInOgProfile` fails (the
torn-checksum file grades PartialRisk under the old default-true); with
the fix, the og-profile test passes AND the honest-mode pin
(`SetChecksumMismatchMarksDamaged`, now explicit `validate_entry_sets =
true`) still passes — both profile behaviors proven. Suite 295/295.
**Resolution notes:** default flipped to `false` per the fix plan (the
verified-absence decompile search holds: the og has NO entry-set checksum
loop); the strictness remains available behind the explicit honest-mode
flag with keep-and-downgrade semantics (never drop). The deviation
direction is registered in docs/PARITY-DELTAS.md as D-47 (default OFF =
og parity; the deliberate strictness exists only behind the explicit
flag). The old default-true test was updated to the honest-mode pin
rather than deleted; the og-profile behavior gets its own test.
