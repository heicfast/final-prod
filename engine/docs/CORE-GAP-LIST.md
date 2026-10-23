# CORE-GAP-LIST — production gate register

> Reconciled against `core_recovery` HEAD `ee23472` and RE corpus `03ad46c`.
> Driver reference: `7e88e38` (the requested read-only v0.2.0 contract remains
> the compatibility baseline; the checked-out driver HEAD is newer). UI contract
> reference: `d773991`. This register supersedes stale open/closed labels in the
> previous revision. MATCH means implementation and regression evidence are at
> HEAD; PARTIAL / WRONG / MISSING remain release-blocking unless explicitly
> marked non-reference scope. No status is inferred from a worklog claim alone.

## 1. Reference engines and recovery pipeline

| ID | Status | Core surface | Evidence at HEAD / closure required |
|---|---|---|---|
| GAP-NTFS | MATCH | NTFS live/deleted/remnant trees, attribute lists, hard links, ADS, sparse runs, $MFTMirr, recycle remap | `test_ntfs_volume.cpp`, `test_runlist.cpp`; corpus `ntfs-engine-internals.md`, `ntfs-engine-algorithms-v2--deleted-viability.md`, `zfs-vdev--mftmirr--calcpartition.md`; recycle closure `632edc4`. |
| GAP-P1-1 | MATCH | NTFS native compressed $DATA | LZNT1 extent envelope and bounded decoder landed in `464d8e5`; tests in `test_ntfs_volume.cpp`. XPRESS/LZX/WOF is not claimed: the RE corpus has no evidence that the reference core implements it. |
| GAP-P1-2 | MATCH | Bounded lzjb decode | `632edc4`; truncated-source regression in `test_zfs_volume.cpp`. |
| GAP-FAT | MATCH | FAT12/16/32, LFN, deleted chains, mirrored FAT, cancellation | `3267d26`; `test_fat_volume.cpp`; corpus `fat-exfat-engine-algorithms-v2.md`. |
| GAP-EXFAT | MATCH | exFAT live/deleted entry sets, checksum and allocation grading | `test_exfat_volume.cpp`; corpus `exfat-engine--deep-dive.md`. |
| GAP-EXT4 | MATCH | ext2/3/4 metadata, extents, deleted/orphan and journal salvage | `test_ext4_volume.cpp`; corpus `remaining-engines--ext4-hfs-refs-btrfs-zfs.md`. |
| GAP-HFS | MATCH | HFS+/HFSX catalog and extents B-trees, forks, orphan sweep | `test_hfsplus_volume.cpp`. Attributes B-tree/journal replay are not registered as parity gaps because the corpus explicitly does not evidence them in the reference engine. |
| GAP-BTRFS | MATCH | Btrfs mirrors, chunk mapping, subvolumes, inline/compressed extents | `test_btrfs_volume.cpp`; corpus `refs-btrfs-zfs-engine-algorithms-v2.md`. |
| GAP-ZFS-MULTIVDEV | PARTIAL | ZFS multi-vdev DVA resolution | `cr_zfs_engine.cpp` rejects nonzero DVAs and labels mirror/raidz report-only. Corpus `zfs-vdev--mftmirr--calcpartition.md` proves vdev-tree geometry parsing but does not yet establish the reference's full mirror/RAIDZ reconstruction semantics. Closure requires evidence-backed topology resolution and seeded multi-vdev tests; no parity claim is permitted. |
| GAP-REFS | MATCH | ReFS MSB chain, checkpoints, directory/object tables, data runs, deleted harvest | `test_refs_ldm.cpp`, `test_refs_volume.cpp`; corpus `refs-engine--deep-dive.md`. Containers/security/ADS are not evidenced reference-core behavior and are not claimed. |
| GAP-LDM | MATCH | LDM database, VBLK graph, simple/spanned/striped/mirrored/RAID-5 mappings, cross-disk resolver | `test_ldm.cpp`; corpus `LdmManager.md`, `module-internals-wave2--ldm-reglib-ctbfile.md`. |
| GAP-LOST | MATCH | Lost-partition discovery, GPT two-step salvage, virtual-volume reader | `3267d26`; `test_lostpart.cpp`, `test_slice_devmap.cpp`; corpus `PartitionRecoveryD.md`, `Partition.md`. |
| GAP-CARVE | MATCH | Signature registry, grammar validators, merge/dedup, endian handling | `test_carve.cpp`; corpus `rawsearch-carving--deep-dive.md`, `rawsearchdll--analyser-registry-census.md`. |
| GAP-SESSION | MATCH | CRSF save/load/resume/checkpoint, identity and stage-mask truth | `be8875f`, `cd4bb41`; `test_session.cpp`, `test_pause_resume.cpp`; corpus `rsf-format--scan-session.md`. |
| GAP-WRITER | MATCH | Overlap refusal, directories, collisions, post-write hashing, JSON and salvaged outcomes | `3267d26`; `test_writer_dirs.cpp`, `test_report.cpp`; corpus `04-recovery-workflow.md`. |
| GAP-HEALTH | MATCH | ATA/NVMe health, TRIM/SSD interpretation | `test_nvme_health.cpp`; corpus `diskhealth--smart-companion.md`. USB bridges without a supported passthrough remain honest `ERR_HEALTH_UNAVAILABLE`, not simulated data. |
| GAP-REPAIR | MATCH | RawFixer analyze/dry-run/apply/undo, 4Kn and rollback | `test_rawfixer.cpp`; corpus `rawfixer-fs-repair.md`, `rawfixer-v2--method-inventory.md`. |
| GAP-ORCH | MATCH | Quick/deep/carve orchestration, monotonic progress, cancellation, ETA, error map and threading | `test_orchestrator.cpp`, `test_telemetry.cpp`; corpus `05-scan-engine-architecture.md`. |
| GAP-G-02 | MATCH | Immutable mid-scan snapshots and value-driven recovery | `fc656d2`; `test_snapshot.cpp`, TSan lane. |
| GAP-G-01 | MATCH | `cr_ui` versioned C ABI and opaque service handles | `e79d23f`, MSVC guard `e8da7ef`; `include/cr/cr_ui.h`, `tools/cr_ui_abi_check.c`. |

## 2. CLI, scoring and fallback correctness

| ID | Status | Item | Evidence / closure required |
|---|---|---|---|
| GAP-P1-5 | MATCH | Elevated-token diagnosis replaces administrator guesses | `837a543`; Windows token probe and four call sites. |
| GAP-BENCH-1 | MATCH | Keep control requires active-tree hits | `837a543`; `scripts/bench_score.py`. |
| GAP-BENCH-2 | MATCH | Precision consumes each result at most once | `837a543`; per-result matching. |
| GAP-BENCH-3 | MATCH | Frozen numeric enum decoding | `837a543`; scorer and CLI CSV schema agree. |
| GAP-BENCH-4 | MATCH | Reference RSF contamination guard | `837a543`; comparison workflow snapshots pre-run state. |
| GAP-BENCH-5 | MATCH | Comparison actually uses `\\.\RDA` | `837a543`; service `rda` plus hard transport assertion. |
| GAP-CLI-COMMANDS | MISSING | Required command names `save`, `load`, `verify`, `repair` | `rrecov.cpp` currently exposes `list/scan/tree/recover/health/fix/bench`; `scan --out/--resume` and `tree --in` are not the named commands. Implement real session validation/load/save semantics and `repair` as the consent-gated RawFixer command, preserving `fix` compatibility. |
| GAP-CLI-TESTS | MISSING | Executable CLI contract tests for every command | No CLI tests are registered in CTest or the Windows workflow. Add success/failure/stdout-grammar/exit-code tests, including DLL smoke and every named command. |

## 3. CI, documentation and release gates

| ID | Status | Item | Evidence / closure required |
|---|---|---|---|
| GAP-CI-1 | MATCH | Windows temporary paths | `0a28a23`; later Windows runs reached the subsequent MSVC issue rather than these three path failures. |
| GAP-CI-2 | PARTIAL | All workflows green at final HEAD | Run `39991984255` (`fc656d2`) passed fully. Runs for `79f5751` and `74f42c0` failed Windows; `e8da7ef` addresses the C-consumer MSVC flags. Final HEAD run `39995054859` was still in flight at reconciliation. Watch every job to completion and fix failures without masking. |
| GAP-CI-UBSAN | MISSING | Full UBSan suite | ASan and TSan lanes exist; no UBSan configure/build/test lane exists. Add a non-sanitizer-mixed UBSan build with halt-on-error and no suppressed findings. |
| GAP-W5-CONTRACTS | MATCH | Ground-truth corrections | Cache-coalesced reads, fixup scope, viability boundary, resident-size correction and LDM history are present in `ground-truth-contracts.md`. |
| GAP-W5-REGISTRY | MATCH | F11/F15 corrections | `error-fix-registry.md` carries the corrected `name_N` cap and capability-report wording. |
| GAP-W5-TODO | MATCH | Shipped TODO/FIXME/HACK sweep | No markers in shipped core/docs. Remaining hits are historical session prompts and the third-party `scripts/ldm_reference.c` reference source; they are not unresolved core work items. |
| GAP-DOCS-FINAL | WRONG | Final documentation still describes pre-closure reality | `CAPABILITY-MATRIX.md` says NTFS compression MISSING; README shows only the v0.1 scope and a future UI; roadmap lacks the landed C ABI/snapshot surface; `UI-READINESS-PLAN.md` remains written as a plan. Update all final docs and record every deliberate divergence in `PARITY-DELTAS.md`. |
| GAP-BENCH-FINAL | PARTIAL | Final same-seed benchmark evidence | `BENCHMARK-REPORT.md` contains measured spot evidence plus NOT-COMPARABLE parameters, not a final full-matrix run at the release commit. Run the Windows seeded/reference workflows at the final commit; retain honest NOT-COMPARABLE reasons and resolve any BELOW before release. |
| GAP-W6-WIN7 | MATCH | Win7 target pins, PE stamps and import allowlist | `74f42c0`; CI gate present. Final green proof remains under GAP-CI-2. |
| GAP-W6-RELEASE | MISSING | Production tag and complete artifact bundle | Release workflow stages only `rrecov.exe`, docs and PDB. It must add `cr_ui.dll`, static libraries, headers, SHA256SUMS and notes linking the three gate documents, then tag the next semver with signing disabled. |

## 4. Closure order

1. Publish this register before implementation changes.
2. Close CLI and UBSan/contract-test gaps with regression coverage.
3. Reconcile final docs and deliberate-delta records.
4. Run Linux, Windows, fuzz, sanitizer and benchmark workflows to completion.
5. Resolve ZFS multi-vdev only to the level proven by RE evidence and real tests; never substitute a simulated topology or an unsupported-parity claim.
6. Package and tag only after every release-blocking row is MATCH and final HEAD CI is green.
