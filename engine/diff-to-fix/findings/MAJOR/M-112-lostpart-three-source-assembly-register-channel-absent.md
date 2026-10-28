# MAJOR M-112 — The og's lost-partition candidate assembly consults THREE sources (mbr / filesystem / register); the register source — the Explorer MountPoints2\CPC\Volume history walk, decoded engine-side in PartitionRecoveryD — has no core counterpart, and the S-051/S-052 fix plans build targets from the partition table alone

**Severity:** MAJOR
**Module:** lost-partition (candidate assembly), orchestration (whole-disk target model)
**Parameters affected:** B1 (detection correctness — the register channel finds partitions whose on-disk remnants are gone), A25/A30 (candidate assembly/scoring inputs), O37 (stage model — the whole-disk flow's candidate feed), P5 (orphan/post-format reconstruction completeness)
**Corpus references:** `og-codebase/log/DRWUI.log:637-643` (the real-run observation: "add lost partition from mbr start=2048, total=122277823" → "add lost partition from filesystem start=2048..." → "add lost partition from register start=2048..." ×3 → "Insert lost partition... Found lost partition(NTFS)"); `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:921820-921890` (FUN_140588820 — the three-source candidate assembly, each source emitting its own `FUN_140162e00("add lost partition from <source> start=%I64d, total=%I64d\r\n")` line: mbr via FUN_140589070, filesystem via FUN_140588ba0, register via FUN_1405895d0); `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:922335+` (FUN_1405895d0 — the DRWUI-side register walk: `RegOpenKeyExA(HKCU, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2\\CPC\\Volume")` + `RegEnumKeyA` loop); `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:8930-8965` (the ENGINE-side register walk: the same key opened with `RegOpenKeyA`, up to 1000 subkeys enumerated, each parsed by FUN_18000f5b0); `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c` FUN_18000f5b0 (the per-subkey decode: `RegQueryValueExA(hkey, "Data")` REG_BINARY > 0xe7 bytes, gate `data[0xc4]=='#'` && `data[0xe6]=='#'`, 0x20 hex chars copied from `data+0xc6`, parsed `%I64X` into the byte start, `start % bytes_per_sector == 0` gate, `start / bps` sector conversion); `re-workspace/reversed/ghidra-searchdll/HFSPlusSearchDLL_decompiled.c:454-461` (the CPartitionVirtualVolumeDevice usage context — the engine-side device family the candidates feed)
**Core references:** audited `c7fa527c` — `src/core/lost/cr_candidates.cpp` (the discovery: windowed byte-scan + boot-sig/evidence-point probes — the filesystem channel's counterpart; no registry input anywhere in `src/core/lost/`); `src/core/reghive/` (the hive parser family — `cr_reghive.cpp` is read-only for FVEAutoUnlock harvest (S-014), `cr_offlineos.cpp` is the M-082 DeviceManager walk; neither touches MountPoints2, and both target OFFLINE hives, not the live HKCU); grep-verified: no occurrence of "MountPoints" or "CPC\\Volume" anywhere in `src/` or `include/`; the whole-disk target model: `diff-to-fix/findings/SEVERE/S-051-...md` fix plan step 1 ("build scan targets from the partition table... The lost-partition discovery stays the `--whole-disk` extra") and `S-052`/`M-104` (the same family's target/branch shape)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's whole-disk lost-partition flow assembles its candidate set from three distinct sources before scoring/dedup: the partition table ("from mbr"), the filesystem-probe scan ("from filesystem"), and the Windows Explorer volume-mount history ("from register" — the `MountPoints2\CPC\Volume` registry walk, present BOTH in DRWUI and engine-side in PartitionRecoveryD.mo). The core's discovery implements only the filesystem-probe channel; the table channel is planned into S-051's fix (per-table-volume targets), and the register channel — which uniquely recovers partitions whose boot-sector/filesystem remnants are destroyed but whose historical placement Windows still remembers — has no core counterpart, no finding, and no registration.

## What the reference does

FUN_140588820 (DRWUI_decompiled.c:921820-921890) assembles lost-partition candidates in three waves, each appending to the same candidate container and emitting its own diagnostic:

1. **from mbr** — `FUN_140589070` fills the candidate list from the partition table (gated on flags at `*param_1+0x6ec` / `param_1+0x29` — the scan-options bits); each entry logs `add lost partition from mbr start=%I64d, total=%I64d`.
2. **from filesystem** — `FUN_140588ba0` (gated on `*param_1 != 0`) fills from the filesystem-probe scan; entries log `add lost partition from filesystem ...`.
3. **from register** — `FUN_1405895d0` fills from the Explorer mount history; entries log `add lost partition from register ...`.

The register source (decoded engine-side — PartitionRecoveryD_decompiled.c:8930-8965): `RegOpenKeyA(HKCU, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MountPoints2\\CPC\\Volume")`, then up to **1000** subkeys enumerated by `RegEnumKeyA`. Each subkey (named for a mounted volume GUID) is parsed by FUN_18000f5b0:
- `RegQueryValueExA(subkey, "Data", ...)` — the REG_BINARY shell cache blob, required > 0xe7 (231) bytes;
- validation gates: `data[0xc4] == '#'` and `data[0xe6] == '#'` (the blob's field delimiters);
- a 0x20-byte (32-char) ASCII-hex field copied from `data+0xc6` — the volume's historical placement encoding;
- parsed with `sscanf("%I64X")` into a byte offset; gated on `offset % bytes_per_sector == 0`; converted to a sector start (`offset / bps`);
- the parsed record is fed into the candidate container (the dedup/splice machinery the M-017 family decoded — `FUN_1800108f0`/`FUN_18000aed0` insertion).

The real-run log (DRWUI.log:637-643) shows the full chain firing in the field — including the register source contributing three records for the same placement (multiple historical mount records for one volume) that the downstream dedup collapses.

The value of channel 3 over channels 1-2: the registry remembers volume placements **even after the partition is deleted and its boot sector overwritten** — the exact scenario lost-partition recovery exists for, where neither the table (channel 1) nor the filesystem probe (channel 2) has anything to find.

## What our core does

`src/core/lost/cr_candidates.cpp` implements the windowed byte-scan with boot-signature dispatch and evidence-point probes (the filesystem channel), plus the M-017 content scoring and the M-042 revalidation. The table channel arrives via S-051's planned fix (per-table-volume scan targets + the +500 table-match scoring already landed). There is no registry-history input: the reghive family (`cr_reghive.cpp`, `cr_offlineos.cpp`) parses offline hives for FVEAutoUnlock and the offline-OS profile, and nothing in the tree references `MountPoints2`/`CPC\Volume` (grep-verified). S-051's fix plan step 1 builds whole-disk targets "from the partition table" only; its acceptance and tests enumerate table-derived targets exclusively.

## Gap

Falsifiable claim: at `c7fa527c`, no code path in `src/` derives a lost-partition candidate (or a scan target) from the Explorer `MountPoints2\CPC\Volume` registry history, while the og feeds such candidates into its assembly (decompiled in both DRWUI and PartitionRecoveryD) and demonstrably fires the walk in a real scan session. A deleted partition with a destroyed boot sector on a disk whose Windows previously mounted it: the og proposes it (register channel); the core cannot.

## Impact

- **Recovery completeness (B1):** the register channel is the only source that can place a partition whose on-disk signature is gone. Disks previously mounted by the same Windows installation (the overwhelmingly common case for the "I deleted a partition" customer) lose this recovery path in the core.
- **S-051/S-052/M-104 fix correctness:** the whole-disk family's fix plans, as written, will land a two-channel assembly and be considered done — the third channel would remain invisible because no finding owns it until now.
- **Candidate scoring inputs (A30):** the og's dedup/AdjustPartitionInfoOrder family (M-017's decode) consumes the merged three-source set; the core's model consumes one source. The M-017 landing is not wrong, but its input set is a subset.

## Reproduction / verification

Corpus-derived + decompile-verified (this session, first-hand):
1. `og-codebase/log/DRWUI.log` lines 637-643 — the three-source chain observed in a real scan (quoted in `evidence/decompiled-quotes/s12-lostpart-register-channel.md`).
2. `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:921820-921890` — FUN_140588820 read in full: the three waves, their gates, their per-source format strings.
3. `re-workspace/reversed/ghidra-mo/PartitionRecoveryD_decompiled.c:8930-8965` + FUN_18000f5b0 — the engine-side walk and the per-record "Data"-blob decode quoted in the evidence file.
4. Core-side greps: `grep -rn "MountPoints\|CPC" src/ include/` — zero hits; `src/core/lost/` read for the candidate-source census.

## Fix plan (for the engineering team)

1. Architectural: add a **candidate-source seam** to the lost-partition discovery: the discovery currently owns the filesystem channel only. Introduce a `CandidateSource` feed (list of `{start_lba, sector_count, provenance}` records) so hosts and the win layer can contribute table- and register-derived records into the same container the byte-scan fills, upstream of the M-017 scoring and the AdjustPartitionInfoOrder-equivalent dedup. The register walk itself is a **win-layer reader** (live HKCU access is an OS-integration concern, like `volume_extent`): implement `win::mountpoints2_volume_history()` returning the parsed placements, and have the orchestrator's whole-disk flow (the S-051 target builder) merge them as register-provenance candidates/targets.
2. Files: `src/win/cr_transport.cpp` (or a new `src/win/cr_mountpoints.cpp`) for the registry walk; `include/cr/win/cr_transport.h` (or its own header) for the export; `src/core/lost/cr_candidates.cpp` (the source seam + provenance field on `Candidate`); `src/core/orchestrator/cr_scan.cpp` (the whole-disk target assembly merges the three channels); `src/ui/cr_ui.cpp` only if the ABI needs to expose the history explicitly (not required if the orchestrator consumes the win-layer reader directly on Windows).
3. Algorithm: the walk is fully decoded above — HKCU `Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\CPC\Volume`, enumerate ≤1000 subkeys, per subkey read the REG_BINARY `Data` value, gate `size > 0xe7`, `data[0xc4]=='#'`, `data[0xe6]=='#'`, parse the 32-hex-char field at `data+0xc6` as `%I64X` byte offset, require sector alignment, convert to sectors. Provenance-tag the records `register`; the existing dedup/containment rules (M-017/M-042 family) collapse duplicates exactly as the og's FUN_1800108f0/FUN_18000aed0 insertion does. **Fix-time probe (one decompile read):** the sector-count derivation — FUN_18000f5b0 computes `local_c0[2] = parsed/bps` from the same hex field; confirm whether the 0x20-char field encodes start only (with the total from a second field or the sibling value) by re-reading the two `%I64X` outputs of FUN_180010b70 at the call site; the core should adopt the verified field split, not a guess.
4. Tests: (a) unit — a synthetic `MountPoints2\CPC\Volume` hive fixture (the reghive parser can read an offline NTUSER.DAT carrying the key) with two volume subkeys, one clean, one failing each gate (short Data, missing '#', misaligned start); (b) integration — a whole-disk fixture where a partition's boot sector is zeroed but the registry history places it: the register channel proposes the candidate, the scan targets it, files are found (this is the channel's unique-value pin); (c) dedup — register + filesystem proposing the same placement yields ONE candidate (the real-log shape).
5. Contract/doc: `docs/CORE-API-CURRENT.md` gains the win-layer function; `modules/lost-partition.md` row for the source model; the S-051 finding's fix plan gains the channel-merge step (addendum, this session).
6. Acceptance: the boot-sector-zeroed fixture recovers its files through the register channel alone; the three-source merge shows the og's real-log candidate count shape (mbr=1, filesystem=1, register=3 → one partition scanned once).

## Tests to add

- `LostPart.MountPoints2WalkParsesPlacements` — the synthetic-hive fixture: clean record parsed, each gate failure rejected (fails before, passes after).
- `LostPart.RegisterChannelFindsZeroedBootPartition` — the unique-value integration pin.
- `LostPart.ThreeSourceDedupSingleCandidate` — the real-log shape.
- `LostPart.RegisterRecordsAreRevalidated` — the M-042 revalidation applies to register-provenance records too (stale placements drop).

## Related findings

- S-051 (whole-disk scan targets — the fix plan's assembly gains the channel merge), S-052/M-104 (the same flow's branch/naming family), M-017 (the scoring/dedup the merged set feeds), M-042 (revalidation over the persisted records), N-131 (the persisted-record family)

## Open questions

The `Data`-blob field split for the sector total (fix-time probe named in step 3 — the start field is proven, the total's exact source needs the FUN_180010b70 call-site re-read; the og's own log shows both start and total emitted, so the total IS derived in the walk).
