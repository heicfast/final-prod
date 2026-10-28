# MINOR N-133 — engine-side on-disk volume labels never decoded/surfaced: the ext4 half-decode is dead code, NTFS $Volume and the FAT-family boot labels are never read — and the LANDED M-079 fix's image-stem fallback (rrecov.cpp:331) bakes in the refuted "the og has no image-scan naming semantics" premise

**Severity:** MINOR (a display/datum-layer gap with no recovery impact; the user-visible fill-site gap is M-079's MAJOR — this finding owns the engine-decode absence, the dead ext4 decode, and the premise correction)
**Module:** ntfs / fat / exfat / ext4 (engine label decode) → session (the label datum's only consumers)
**Parameters affected:** B11-adjacent (node fields — the volume node's label), D12-adjacent (session persistence — record-2 volumeName's engine-side source), Q68 (documentation — M-079's premise correction)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c:647-676` (the volume-init walk reads $Volume and extracts the label) + `FUN_18003f3a0` at :50963-50980 (the $VOLUME_NAME type-0x60 attribute extractor with the wide→narrow conversion — instruction-decisive); `re-workspace/artifacts/SearchDLL/ext4-hfs-engine-algorithms-v2.md` §2 step 5 (:45): `strncpy(label, s_volume_name, 0xF) — volume label for the UI`. Full decode chain: `evidence/decompiled-quotes/s10f-engine-volume-labels.md`.
**Core references:** audited `deea0241` (the f43b747 M-079 fix landed mid-session; cites re-verified at this tree). The landed M-079 fix: `src/app/rrecov.cpp:331-362` (`fill_volume_label` — `--label` override > the image FILENAME STEM for image targets > GetVolumeInformationW for volume targets), with the comment at :329-332 repeating the refuted premise verbatim ("the og has no image-scan naming semantics"). ext4: `src/core/ext4/cr_ext4_engine.cpp:200-201` decodes `s_volume_name` into `Volume::volume_name` (`include/cr/ext4/cr_ext4_engine.h:79`) with ZERO consumers (the census: the only two `volume_name` hits are the decode and the declaration). NTFS: no $Volume label read (`cr_mftwalk.cpp:218/:247` are the metafile filter lists). FAT/exFAT: no boot-label decode. The label's only core carriers are caller-supplied: `include/cr/orchestrator/cr_scan.h:239` (`volume_label[32]`), `fill_identity_card` `src/ui/cr_ui.cpp:1850-1866` (fills model/serial/firmware/wwn/interface — not the label).
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** M-079 (FIXED at `f43b747` this session's window, ledger row carries the S10-F addendum note) correctly verified no fill site existed and landed one — but the landed image-target leg derives the label from the image FILENAME STEM on the strength of the fix plan's premise ("the og has no image-scan label semantics"), which the NTFS/ext4 engine decodes above refute. No other trail item covers the engine-side label surface.

## Summary

The og's engines decode the scanned volume's on-disk label themselves — NTFS from $Volume's $VOLUME_NAME attribute (instruction-decisive), ext4 from s_volume_name (artifact-documented) — and surface it to the volume node/UI; this works identically for image scans. The core still never decodes an engine-side label for any filesystem: the ext4 engine's existing decode (cr_ext4_engine.cpp:200-201) remains dead code with no consumer, and NTFS/FAT/exFAT have no decode at all. The M-079 fix that landed mid-session (f43b747) fills the live-volume half correctly (GetVolumeInformationW) but implements the image half as the image FILENAME STEM — on the strength of the premise this finding refutes (the og's engines decode the on-disk label, which works identically for images).

## What the reference does

NTFS: at volume init (NTFSSearchDLL_decompiled.c:647-676, the quick controller's volume-init slot), the engine reads $Volume and calls FUN_18003f3a0 (buffer, out, type) — the extractor looks up the type-0x60 ($VOLUME_NAME) attribute, takes its value at +0x18, and converts it wide→narrow into the label buffer; the label then feeds the volume node's name field (the :675-743 emission), with a stored-copy fallback at +0x440. ext4: the Ext4Volume ctor strncpy's s_volume_name (0xF bytes) into the volume object's label "for the UI" (algorithms-v2 §2 step 5). Both decodes work from the volume's own metadata — no OS mount required, image scans included.

## What our core does

No engine decodes its volume label. The ext4 engine decodes s_volume_name into `Volume::volume_name` (cr_ext4_engine.cpp:200-201) but nothing reads it — dead code. The only label carriers are `ScanConfig.volume_label[32]` (caller-supplied, cr_scan.h:239), the ABI card's `volume_label_utf8` (caller-supplied), and `fill_identity_card` (cr_ui.cpp:1850-1866) which fills every identity field EXCEPT the label (M-079's gap).

## Gap

Falsifiable claims: (a) an ext4 image with `s_volume_name = "myvol"` — the og's UI volume display shows "myvol" (engine-decoded); no core API, JSON field, or CLI output can ever produce "myvol" from that image (the only decode is dead code). (b) At the landed tree, an ext4 image named `backup.img` with `s_volume_name = "myvol"` produces the auto-save name `auto_backup_...crsf` (the image stem) where the og's session would carry `myvol` (the engine-decoded label) — the landed fallback invents a label the og does not use for image scans.

## Impact

- The engine-layer label datum is unreachable in the core — the ABI card and auto-save naming depend on caller-supplied values (M-079) with no engine fallback for image scans (the CLI's primary target class).
- The dead ext4 decode is a maintenance trap (reads as if surfaced; isn't).
- M-079's image-target leg, AS LANDED (f43b747), invents a filename-derived label where the og uses the on-disk label — the divergence is now in the tree, not just in a plan.

## Reproduction / verification

- Decompiled evidence (strength 2): the NTFS $VOLUME_NAME extraction chain quoted at `evidence/decompiled-quotes/s10f-engine-volume-labels.md` §1.1 (FUN_18003f3a0 read first-hand at :50963-50980 — the 0x60 attribute type + the wide→narrow conversion).
- Corpus documentation evidence (strength 3): ext4 algorithms-v2 §2 step 5.
- Core first-hand: the volume_label/volume_name grep census (evidence §2) at `44f043a5`.

## Fix plan (for the engineering team)

1. Surface the ext4 decode: after `validate_superblock`, copy `Volume::volume_name` into the scan context; extend `fill_volume_label` (rrecov.cpp:331) with an engine-decode precedence leg for image targets — engine on-disk label (when the engine yields one) BEFORE the image-stem fallback (which becomes the registered last-resort for label-less images). Replace the false-premise comment at :329-332.
2. Add the NTFS decode: in the $Volume handling (alongside the metafile filter at cr_mftwalk.cpp:218), read the $VOLUME_NAME (type 0x60) $DATA and convert UTF-16→UTF-8 (the og converts wide→narrow; the core's u16 carrier is the faithful equivalent — N-016b's naming precedent applies).
3. Add the FAT-family decode: the 11-byte boot label (FAT12/16/32 +0x2B, exFAT +0x80), same fallback wiring. HFS+: the volume-name key (-2) catalog record, same wiring.
4. Tests: `VolumeLabel.Ext4FromSuperblock`, `VolumeLabel.NtfsFromVolumeRecord`, `VolumeLabel.FatFromBootSector`, `VolumeLabel.CallerValueWins` (the override direction), each fail-before.
5. M-079 coordination: the ledger row carries the S10-F addendum note (the image-stem leg is the registered divergence until this finding's fix lands); the corrected precedence is engine-label > image-stem.

## Tests to add

- `VolumeLabel.Ext4FromSuperblock` (fail-before: dead decode).
- `VolumeLabel.NtfsFromVolumeRecord`.
- `VolumeLabel.FatFromBootSector` / `VolumeLabel.ExfatFromBootSector`.
- `VolumeLabel.EngineFallbackOnlyWhenCallerEmpty`.

## Related findings

- M-079 (FIXED f43b747 — its image-stem leg is the divergence this finding owns; the ledger row carries the S10-F addendum note).
- N-016b precedent (the U+FFFD name-substitution rule for the label's UTF-16→UTF-8 path).

## Open questions

None. (The FAT/HFS+ label offsets in fix-plan step 3 are the standard well-known offsets; an implementation pass should re-verify them against the corpus boot-sector schemas before landing — one grep each in the fat/exfat/hfsplus deep-dives.)
