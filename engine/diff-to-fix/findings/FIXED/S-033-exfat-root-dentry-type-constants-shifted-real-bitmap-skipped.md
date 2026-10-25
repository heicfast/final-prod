# SEVERE S-033 — exFAT root-dentry type constants shifted vs. the real format: the real allocation-bitmap dentry (0x83) is skipped as "volume label", the UPCASE entry (0x81) is parsed as the bitmap

**Severity:** SEVERE
**Module:** exfat
**Parameters affected:** B1 (detection correctness — wrong-structure interpretation on every real volume), A25 (engine algorithm steps — the allocation-bitmap consumption), S35 (error recovery — the og's upcase integrity check unreachable), C11 (fixture truth — engine tests built on the shifted constants)
**Corpus references:** `re-workspace/artifacts/SearchDLL/fat-exfat-engine-algorithms-v2.md` §1 ("exFAT's on-disk **allocation bitmap** … is consumed directly by `CQuickSearchAndBuildBitmap` — quick scan only visits allocated clusters + the FAT") and §3 row 1 ("`(t-3)&0x7F==0` | 0x03 / 0x83 | … **volume-label triple**: entry itself, +0x20 must be 0x81 (TX), +0x40 must be 0x82 (label) — the root's opening sequence"); the byte facts of that row are the real Win-formatted root order **[0x83 bitmap][0x81 upcase][0x82 label]**; `re-workspace/artifacts/SearchDLL/fat-exfat-engine-internals.md:63-64` ("Up-case table / integrity: volume validation via boot + upcase-table checks (`CExFatVolumeAnalyer`)"); `re-workspace/reversed/ghidra-searchdll/exFATSearchDLL_decompiled.c:2076/1923` (`(t-0x40)&0x7f` / `(t+0x3f)&0x7f` stream/name dispatches — the v2 table's byte-level tests are decompile-verified; its row LABELS are garbled but its bytes corroborate the real-root triple); RTTI `.?AVCQuickSearchAndBuildBitmap@@` / `.?AVCExFatVolumeAnalyer@@` in `og-codebase/exFATSearchDLL.dll`
**Core references:** `include/cr/exfat/cr_exfat_wire.h:53-55` (`TYPE_BITMAP = 0x01 /* 0x81 */`, `TYPE_UPCASE = 0x02 /* 0x82 */`, `TYPE_VOLUME_LABEL = 0x03 /* 0x83 */` — all three shifted), `:117-124` (`BitmapEntry` — the FIELD layout `first_cluster@+0x14, data_length@+0x18` is the real **0x83 bitmap** layout, keyed on the wrong type byte), `:128-131` (comment: upcase/label structs deleted "the engine skips both by TYPE byte only"); `src/core/exfat/cr_exfat_quick.cpp:289` (`if (t == TYPE_BITMAP && in_use)` — loads the "bitmap" from a 0x81 dentry), `:325` (`if (t == TYPE_UPCASE || t == TYPE_VOLUME_LABEL)` — the real 0x83 bitmap dies here), `:467-480` (deleted-file grading by bitmap reuse ratio — dead on real volumes), `:612-631` (S-009 free map gated on `have_bitmap` — never populates on real volumes); `tests/unit/test_exfat_volume.cpp:5` (fixture header comment "bitmap(0x81@50), upcase(0x82), label(0x83)"), `:238-272` (the root dentries planted with the shifted constants), `:920` (second fixture)
**Status:** NEW-FINDING — filed during the N-067 verification pass (session 11); the audit's own AUDIT-FAT row 11b recorded the og's 0x83/[+0x20]=0x81/[+0x40]=0x82 byte triple but read it as a "volume-label" anchor and did not connect it to the bitmap/upcase mapping.

## Summary

The real exFAT format (spec + every Win-formatted volume) keys the root dentries as: **allocation bitmap = 0x83, volume label = 0x82, up-case table = 0x81**. The core's `TYPE_*` constants are shifted: it keys the bitmap branch on 0x81 (the UPCASE entry's byte), skips 0x83 (the real bitmap) as "volume label", and treats 0x82 (the real volume label) as "upcase". The corpus itself carries the proof twice — the v2 artifact's "triple" row describes the real root opening [0x83][0x81][0x82] (its row labels are garbled, its bytes are decompile-anchored), and the v2 §1 statement that the og's quick scan "consumes the allocation bitmap directly" (`CQuickSearchAndBuildBitmap`). On every real exFAT volume the core's allocation-bitmap path never fires.

## What the reference does

`CQuickSearchAndBuildBitmap` consumes the on-disk allocation bitmap dentry (0x83) directly — the quick scan visits allocated clusters; `CExFatVolumeAnalyer` validates the upcase table (0x81) as part of volume integrity. The og's root walk also carries the [0x83][0x81][0x82] opening-triple check (decompile-anchored at `(t-3)&0x7F==0`).

## What our core does

The bitmap branch (`t == TYPE_BITMAP`, masked 0x01) parses the **0x81 UPCASE dentry** with the `BitmapEntry` layout — reading `first_cluster` from the upcase entry's reserved bytes @+0x14 and `data_length` from @+0x18 (the upcase entry's own FirstCluster field). On spec-conformant volumes (reserved zero-filled) the `first_cluster >= 2` gate fails, so the mis-parse is silently absorbed: `have_bitmap` stays false, deleted-file grading loses the reuse ratio (falls to the default), and the S-009 free map — gated on `have_bitmap` — never populates. The real 0x83 bitmap dentry is discarded at the `TYPE_VOLUME_LABEL` skip.

## Gap

Falsifiable claim: a spec-conformant exFAT image (Win-format root order [0x83 bitmap][0x81 upcase][0x82 label]) scanned by the core produces NO free map and NO bitmap-based grading, while the og's `CQuickSearchAndBuildBitmap` consumes the bitmap directly. The engine's own test fixtures plant the shifted order (bitmap@0x81, label@0x83), so every green test has been proving the wrong mapping — the fixtures and the engine are self-consistently wrong together.

## Impact

Real-volume correctness + capability: (1) exFAT free-cell disk maps stay "unknown" on real volumes (the S-009 surface, landed and tested, is dead in the field); (2) deleted-file confidence grading loses the allocation-reuse signal (the og's primary deleted-viability input on exFAT); (3) N-067's upcase integrity check is unimplementable until the mapping is fixed — the upcase entry currently never reaches an upcase branch; (4) the mis-parse reads reserved bytes of the upcase entry as a cluster pointer — harmless on zeroed-reserved volumes, a garbage-following hazard on corrupt/quirky ones.

## Reproduction / verification

Corpus: v2 §1 (bitmap consumed directly), v2 §3 row 1 bytes ([0x83][0x81][0x82] triple), internals :63-64 (upcase validated by CExFatVolumeAnalyer), RTTI strings. Spec: Bitmap 0x83 (FirstCluster@+0x14, DataLength@+0x18 — exactly the core's `BitmapEntry` layout), UPCASE 0x81 (TableChecksum@+0x04, FirstCluster@+0x18, DataLength@+0x1C), Volume Label 0x82 (LabelLength@+0x01, 11 u16 chars@+0x02 — matching the core's existing label fixture bytes). Fail-before executed this session: the real-order fixture on the pre-fix engine fails the S-009 free-map assertions and both deleted-grading assertions (`gone.dat` not Excellent, `over.dat` not Overwritten); pass-after with the corrected constants.

## Fix plan (for the engineering team)

1. `cr_exfat_wire.h`: reassign the constants to the real format — `TYPE_UPCASE = 0x01 (0x81)`, `TYPE_VOLUME_LABEL = 0x02 (0x82)`, `TYPE_BITMAP = 0x03 (0x83)`; fix the `BitmapEntry` comment to "(0x83)"; the field layout is already spec-exact for the real bitmap.
2. Rewrite the test fixtures' root dentries to the real Win-format order [0x83 bitmap][0x81 upcase][0x82 label] (both exFAT fixtures in test_exfat_volume.cpp; the label fixture's character layout is already real — only the type bytes move).
3. Reintroduce the `UpcaseEntry` struct (real layout: TableChecksum@+4, FirstCluster@+24, DataLength@+28) — required by N-067, and update the deleted-structs comment.
4. Test: the existing grading/free-map tests become the fail-before proof on the real-order fixture; add an explicit real-root-order free-map test if the S-009 hardening test does not already cover it.
5. Audit the AUDIT-FAT row-11b note: the "volume-label triple" label came from the corpus artifact's garbled row name; record the correction in the finding resolution, not by editing the audit.

## Tests to add

- Real-root-order fixture drives the existing `ExFAT` grading + S-009 free-map assertions (fail-before on the pre-fix engine)
- `ExFAT.RealRootBitmapDentryLoads` (explicit free-map-from-real-bitmap proof, if not already covered by the hardening test)

## Related findings

N-067 (upcase-table validation — unblocked by this fix); N-023 (FIXED — the exFAT gate minors; its fixtures carried the shifted constants, corrected here); S-009 (FIXED — the free-map surface this bug silences in the field); AUDIT-FAT §6 row 11b (the corpus row whose byte triple hides this mapping).

## Open questions

None — the mapping is quadruple-sourced (spec layout, corpus v2 bytes, corpus v2 §1 statement, real-format RTTI behavior) and the fail-before run reproduces the impact.

## Resolution

**Status:** INVALID
**Fixed in commit:** n/a — no code change (the local false-positive branch was reverted before push; the finding file is preserved as the investigation record)
**Verified by:** first-hand decompile read `exFATSearchDLL_decompiled.c:24380-24460` (the directory-walk validation switch) + re-derivation of the 32-byte dentry layouts + the og's triple anchor at `:1847`. Command evidence: the revert restored the pre-finding tree byte-exactly; `Exfat*` 26/26 green on the restored tree (the same tests that "failed" under the proposed rewrite — they failed because the FIXTURE was rewritten to a wrong order, not because the engine was wrong).
**Resolution notes:** The premise — "the real format keys bitmap=0x83 / upcase=0x81 / label=0x82, and the core's constants are shifted" — is REFUTED by the og's own directory-walk validation switch (decompile :24380-24460): **case 0x81** validates a dentry whose +1 byte is UNCHECKED, whose reserved bytes +2..+0x13 are zero-checked, with cluster@+0x14 and length@+0x18 — the ALLOCATION BITMAP layout (BitmapFlags@+1 free, Reserved(18)@+2, FirstCluster@+0x14, DataLength@+0x18); **case 0x82** validates reserved1@+1..+3 zero, the field at +4 UNCHECKED (the checksum), reserved2@+8..+0x13 zero, cluster@+0x14, length@+0x18 — the UPCASE layout (TableChecksum@+4 free). The og — a shipped engine that works on real volumes — keys BITMAP=0x81, UPCASE=0x82, VOLUME LABEL=0x83, exactly the core's original constants; the real-root opening triple [0x83 label][0x81 bitmap][0x82 upcase] at :1847 confirms the label-first order. The finding's UPCASE layout claim (FirstCluster@+0x18/DataLength@+0x1C) was a mis-derivation — those offsets with +4/+8/+0x14 fields sum past the 32-byte dentry; the correct UPCASE layout shares the bitmap's cluster@+0x14/length@+0x18 tail and differs in the head (TableChecksum@+4 vs BitmapFlags@+1). Root cause of the false positive: the session's spec recall was wrong AND the corpus artifact `fat-exfat-engine-algorithms-v2.md` §3's garbled row labels ("volume-label triple" for the 0x83 anchor, "allocation-bitmap dentry" for the 0x85 FILE row) invited the misreading — the artifact's BYTES were always right and its LABELS unreliable. The core's constants, `BitmapEntry` layout, and original fixtures were all correct. **Erratum fed back:** corpus row filed in CORPUS-ERRATA.md (the v2 §3 labels); the convergent evidence chain (deep-dive §2 "Allocation Bitmap (type 0x81)" + the M-054 lane's bit↔cluster fix) is the correct reading. N-067 proceeds on the TRUE constants (upcase = TYPE 0x02 / on-disk 0x82, TableChecksum@+4, cluster@+0x14, length@+0x18) with the decompile's upcase-shape gates as a bonus: the og's walk checks reserved1@+1..3 zero + reserved2@+8..+0x13 zero on the 0x82 dentry itself.
