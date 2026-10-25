# SEVERE S-019 — Lost BitLocker ("-FVE-FS-") partitions: probe family missing entirely (extends S-006)

**Severity:** SEVERE
**Module:** lost-partition
**Parameters affected:** B1 (detection correctness), B3 (probe set), A30 (candidate set), B8
**Corpus references:** `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:2731-2790` (the "-FVE-FS-" OEM literal built byte-by-byte at :2778-2785: `2d 46 56 45 2d 46 53 2d` = "-FVE-FS-"), `:2525-2526` + `:2604-2606` (pre-check hit → classify type `0xB`, skipping the FAT flavor ladder), `:721` (`BitlockerParser::vftable` — the dedicated deep-scan block parser), `:6463/:6558/:8664/:8720` (CompleteSearch/RegSearch/Fast1M/Fast generator sets that drive it); `og-codebase/PartitionRecovery/BitLockerLib.dll` + `bitlockerhelp.mo` (the recovery/decrypt chain per `re-workspace/artifacts/PartitionRecovery/mo-module-map.md:25,40`)
**Core references:** `src/core/lost/cr_candidates.cpp:229-249` (dispatch: NTFS / EXFAT / "EFI PART" / else-FAT — no BitLocker branch), `:243-248` (the else-FAT catch-all: a "-FVE-FS-" boot sector fails `fat::validate_boot` → `pts < 0` → `continue`, candidate dropped); `src/core/fat/cr_chain.cpp:261-319` (no path accepts an FVE boot sector)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

S-006 catalogued the missing Linux/FATX probe families but under-listed the og:
the og's lost-partition deep scan ALSO classifies and parses **BitLocker**
volumes — the boot-sector classifier recognizes the "-FVE-FS-" BitLocker
signature and assigns type 0xB, and PartitionRecoveryD.mo carries a dedicated
`BitlockerParser` block parser (paired with BitLockerLib.dll for recovery). Our
core's probe dispatch has no BitLocker branch at all: an FVE boot sector falls
into the FAT catch-all, fails the BPB validation, and the lost BitLocker
partition is silently invisible.

## What the reference does

1. The boot classifier pre-check `FUN_180004ce0`
   (PartitionRecoveryD_decompiled.c:2731-2790) builds the 8-byte literal
   "-FVE-FS-" on the stack (`local_40=0x2d('-'), 0x46('F'), 0x56('V'),
   0x45('E'), 0x2d, 0x46, 0x53, 0x2d` — :2778-2785) and compares it against
   the boot sector's OEM area (BitLocker volumes carry "-FVE-FS-2013.0.0"-style
   OEM labels at offset 3). On match, `FUN_180004610` classifies the candidate
   as type `0xB` and skips FAT classification entirely (:2604-2606).
2. `BitlockerParser` (vftable at :721) is one of the seven deep-scan block
   parsers (Win32/FAT-tables, Linux, exFAT, ExtX, FATX, NTFS, Bitlocker — full
   vftable census in `evidence/decompiled-quotes/s3-calcpartition-scoring.md`
   §6).
3. Downstream, `og-codebase/PartitionRecovery/BitLockerLib.dll` +
   `bitlockerhelp.mo` (mo-module-map :40) implement the recovery/decrypt side —
   the og can offer structured recovery of lost BitLocker partitions, not just
   their detection.

## What our core does

`src/core/lost/cr_candidates.cpp:229-249`: the discovery dispatch checks
`"NTFS    "`, `"EXFAT   "`, `"EFI PART"`, and treats everything else with a
0x55AA signature as a FAT candidate. A BitLocker boot sector (signature 0x55AA
present, OEM "-FVE-FS-…") reaches `probe_fat` (:246), where
`fat::validate_boot` (`src/core/fat/cr_chain.cpp:261`) rejects it (no coherent
FAT BPB: bps/SPC fields in an FVE header do not parse as FAT geometry) →
`pts < 0` → `continue` (:247-248). **No candidate is ever created.** There is
also no BitLocker block parser anywhere in the core (no "-FVE-FS-" constant
exists in `src/`).

## Gap

Falsifiable claim: a disk with a deleted BitLocker-encrypted partition (FVE
boot sector intact at the old partition start, outside the current table)
yields a lost-partition candidate of the og's deep scan (type 0xB,
BitlockerParser path) and zero candidates in our scanner — the partition and
its contents are invisible to every core pass except raw carve (encrypted
content — carve yields nothing usable).

## Impact

- BitLocker users with deleted/lost encrypted partitions get nothing from us
  where the og detects the volume and can engage its BitLocker recovery chain.
- Same user-observable class as S-006 (invisible lost partitions), for a
  family S-006 did not list.
- Detection is the gating step: without the candidate, no downstream BitLocker
  work (key probing, unlock) can even begin.

## Reproduction / verification

- Decompiled: the "-FVE-FS-" literal bytes + classification type 0xB
  (PartitionRecoveryD_decompiled.c:2778-2785, :2604-2606) and the
  `BitlockerParser::vftable` (:721) — first-hand quotes.
- Core: dispatch quote (cr_candidates.cpp:229-249) — deterministic: an
  "-FVE-FS-…55 AA" sector cannot match any branch and fails probe_fat.
- Executed RE probe (2026-10-25): `rg -a --byte-offset -o "\-FVE\-FS\-"
  PartitionRecoveryD.mo` → hits at byte offsets 172520 and 172536 (the
  comparison literal + the on-disk-format template); `rg -a -o "FVE-FS"
  Partition.mo` → byte offset 218217; `rg -a -c "FVE" BitLockerLib.dll` → 2
  sections. The og binaries carry the BitLocker boot-signature matching
  end-to-end.

## Fix plan (for the engineering team)

1. Add a `probe_bitlocker` branch to `cr_candidates.cpp` BEFORE the FAT
   catch-all: match OEM `"-FVE-FS-"` at offset 3 (8 bytes, case-sensitive per
   the on-disk format) on a 0x55AA sector; record `CandidateFs::BitLocker`
   (new enum value), size from the FVE header's volume-size fields where
   parseable, else device-tail fallback (same policy as other probes,
   `size_from_device_end` marking per GAP-S6-6).
2. Evidence points (A30, until content-based scoring lands per M-017): boot-sig
   +15, FVE header sanity (version/cluster-size fields) +25 — mirrors
   S-006's fix-plan point values.
3. Materialize/slice handling: the slice is opaque encrypted content; the
   orchestrator should surface the candidate (inventory correctness) even if
   no engine can walk it yet — a "locked BitLocker volume" result is what the
   og effectively offers pre-unlock.
4. Extend S-006's planned fixtures with a BitLocker case
   (`Lost_Bitlocker_Partition_Discovered`) + noise case (random OEM, no
   candidate).
5. Document in CAPABILITY-MATRIX: BitLocker lost-partition detection present,
   content recovery gated on BitLocker key handling (out of current scope).

## Tests to add

- `Lost_Bitlocker_Partition_Discovered`: fixture disk with an FVE-style boot
  sector at a non-table LBA → `discover()` yields one BitLocker candidate
  (fails before, passes after).
- `Lost_Bitlocker_Oem_Noise`: "FVE-FS-" string without valid FVE header
  shape/0x55AA → no candidate.
- EBR/table interplay: an FVE boot sector inside the extended partition region
  is still discovered (deep scan is table-independent).

## Related findings

- S-006 (same probe-set module; this finding adds the BitLocker family S-006
  did not list — the under-list is recorded here rather than editing S-006,
  per append-only discipline).
- M-017 (addendum)/M-017 (scoring/merge model — applies to any new family's candidates).

## Open questions

Whether the og's BitlockerParser recovers the FVE volume's *geometry* (size
extent) from the FVE header fields or from the surrounding table remnants is
not yet decoded (corpus artifact: `BitLockerLib.dll` decompile under
`ghidra-partition/BitLockerLib_decompiled.c`).

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's S-019 commit; see git log)
**Verified by:** fail-before/pass-after — with the probe/dispatch stashed back to the pre-fix tree, all three tests FAIL exactly as the finding's falsifiable claim predicts (`LostDiscovery.LostBitlockerPartitionDiscovered`: "the lost BitLocker partition must surface" — fve == nullptr, no candidate ever created; `LostBitlockerOemNoise`: the 55AA+OEM noise sector also produced no candidate, proving the whole family was dead; `LostBitlockerInsideExtendedRegionStillFound`: table-independent discovery also dead); with the fix, 3/3 PASS. Full suite 365/365 (linux-gcc ASan, 362 baseline + 3). clang-format gate clean; Windows lane: constants/logic only, no new imports (Win7-inclusive gate untouched).
**Resolution notes:** `probe_bitlocker` added to `cr_candidates.cpp` with the og's EXACT classifier gate (corpus re-verified first-hand this session: `FUN_180004ce0` builds `2d 46 56 45 2d 46 53 2d` on the stack and `memcmp`s it against boot+3 — PartitionRecoveryD_decompiled.c:2778-2785; on match `FUN_180004610` assigns type 0xB and skips FAT classification — :2604-2606). The dispatch branch runs BEFORE the FAT catch-all (an FVE VBR is BPB-less and died in `fat::validate_boot` pre-fix). Scoring per the fix plan: boot-sig +15 (the OEM gate + 55AA), fve-shape +25 when the version region (+0x0B..+0x13) carries printable digits/dots — the shape check only grades confidence, never rejects, because the og's gate is shape-free (pinned by `LostBitlockerOemNoise`: 55AA+OEM+binary-garbage version still surfaces a boot-sig-only candidate). Size: the FVE VBR volume-geometry decode remains corpus-UNDECODED (the finding's own open question — BitLockerLib's field reads) — device-tail fallback with the GAP-S6-6 `size_from_device_end` marker, the same honest policy as probe_refs. Downstream wiring: `FsFamily::BitLocker` (value 10, append-only like the S-006 values) + a `probe_family` OEM branch so encrypted content never falls through to the ext4-magic probe (a random 0xEF53 in ciphertext would mis-dispatch); the engine dispatch's default path surfaces the candidate as a locked volume with zero files — the og's pre-unlock behavior per the fix plan's inventory-correctness requirement. Session deserialize gate extended to `<= CandidateFs::BitLocker` (same append-only rule as S-006; old sessions stay valid). The signed-char 0xFF compare in the shape check uses unsigned char per the PROTOCOL signed-char rule (the S-018 lesson). CAPABILITY-MATRIX lost-partition row updated.
