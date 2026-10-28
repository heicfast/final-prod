# FIXED-LEDGER.md — the index of the 259 resolved findings (files retired to git history)

> **What this is:** the user-mandated purge (session SA-10B / L-A) removed
> the 259 per-finding files under `findings/FIXED/` from the working tree
> (250 at the session-9 board-truth pass + the parallel fix lanes' five
> landings since: S-042 f6e303e, S-044 9496f3c, M-071, M-090 771e2ec,
> N-094 26da616 — counts re-verified at purge time; the A-005/A-006
> renumber pair retires BOTH files, the renumber note lives in the
> A-005-named file's heading, a deliberate historical record).
> **Nothing is lost:** every file is recoverable from git history at the
> commit that removed it, and this ledger keeps every ID navigable —
> severity, module, the fixing commit where the file records it, and the
> history path of the retired file.
>
> **How to recover a retired file:**
> `git log --oneline --follow -- 'diff-to-fix/findings/FIXED/<slug>.md'`
> then `git show <last-add-commit>:diff-to-fix/findings/FIXED/<slug>.md`.
> The RESOLVED-INDEX.md (retired alongside) and SUMMARY.md's session
> blocks carry the resolution narrative; the per-file Resolution sections
> live in history.

**Counts:** 281 resolved findings (280 rows at `e56baac` — the top line said 279, the filesystem is the truth — + M-100 closed-as-covered at the session-10 lane-B push) (the batch-C-conform 276 `0872ac2`/`64a3b18` + **S-041 resolved-as-umbrella by the SA-10 lane** — the purge's 255 + every mid-flight landing through batch C + the convention conformances + S-041/N-101) · 98 with a fix-commit recorded in-file ·  *(the dated counts snapshots below are the parallel lanes' push-window history, kept per the 64a3b18 sweep)*
**Counts:** 276 resolved findings (255 at the purge commit + the mid-flight parallel landings + the session-38 waves: N-116 `632d21e`, N-117/N-118/N-103 `954c386`, N-095/N-108/N-119 `e1e5124`, batch C N-104/N-097/N-102/N-123/N-101/N-120 fixed + N-099/N-106 FIXED-ALREADY `13245c0`; S-043 `b3938c8` moved its file into the re-created FIXED/ dir BEFORE this convention was readable - its row is pending, the next audit lane retires the file) · 98 with a fix-commit recorded in-file ·
**Counts:** 279 resolved findings (…prior waves…; the session-38 registration batch D adds N-091/N-098 fixed + N-105 FIXED-ALREADY) · 98 with a fix-commit recorded in-file ·

**Counts:** 270 resolved findings (255 at the purge commit + the mid-flight parallel landings absorbed at rebase: M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e`, N-117/N-118/N-103 `954c386`, N-095/N-108/N-119 `e1e5124` session-38 doc-truth batches A+B; S-043 `b3938c8` moved its file into the re-created FIXED/ dir BEFORE this convention was readable - its row is pending, the next audit lane retires the file) · 98 with a fix-commit recorded in-file ·

**Counts (268, the prior window's snapshot):** 255 at the purge commit + the mid-flight parallel landings absorbed at rebase: M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e`, N-117/N-118/N-103 `954c386`, N-095/N-108/N-119 `e1e5124` session-38 doc-truth batches A+B.

**Counts (265, the batch-A snapshot):** 255 at the purge commit + M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e` session-38, N-117/N-118/N-103 `954c386` session-38 doc-truth batch A.

**Counts (264, the closing-fix lane's snapshot):** 255 at the purge commit + M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e` session-38; S-043 and M-080 re-created FIXED/ before the convention was readable - their files retired to history and their rows added by the closing-fix lane's reconciliation.
**Counts:** 279 resolved findings (276 at the batch-C conform `0872ac2` + **S-041 resolved-as-umbrella by the SA-10 lane**; the purge's 255 + every mid-flight landing through batch C — N-116, N-117/N-118/N-103, N-095/N-108/N-119, N-104/N-097/N-102/N-123/N-101/N-120, N-099/N-106 — + S-043/M-080/M-085/S-047 conformed + S-041/N-101) · 98 with a fix-commit recorded in-file ·

**Counts:** 279 resolved findings (276 at the batch-C conform `0872ac2` + **S-041 resolved-as-umbrella by the SA-10 lane**; the purge's 255 + every mid-flight landing through batch C — N-116, N-117/N-118/N-103, N-095/N-108/N-119, N-104/N-097/N-102/N-123/N-101/N-120, N-099/N-106 — + S-043/M-080/M-085/S-047 conformed + S-041/N-101) · 98 with a fix-commit recorded in-file ·

**Counts:** 279 resolved findings (276 at the batch-C conform `0872ac2` + **S-041 resolved-as-umbrella by the SA-10 lane**; the purge's 255 + every mid-flight landing through batch C — N-116, N-117/N-118/N-103, N-095/N-108/N-119, N-104/N-097/N-102/N-123/N-101/N-120, N-099/N-106 — + S-043/M-080/M-085/S-047 conformed + S-041/N-101) · 98 with a fix-commit recorded in-file ·
**Ledger generated:** session SA-10B (2026-10-29), tree `62074bf`-lineage.

| ID | Severity | Module | Fix commit (as recorded in-file) | Retired file (history path) |
|---|---|---|---|---|
| A-001 | ACUTE |  | — | `findings/FIXED/A-001-gpt-type-guids-wrong-ldm-gpt-dynamic-disks-fail.md` |
| A-002 | ACUTE |  | 1cb6a43 | `findings/FIXED/A-002-a001-fix-corrupted-ldm-gpt-guids-real-dynamic-disks-fail.md` |
| A-003 | ACUTE |  | — | `findings/FIXED/A-003-fix-part-writes-at-disk-lba0-slice-offsets-never-rebased.md` |
| A-004 | ACUTE |  | — | `findings/FIXED/A-004-eventrange-fires-through-config-sink.md` |
| A-005 | ACUTE |  | 180010720 | `findings/FIXED/A-005-hfsplus-btree-record-span-inverted.md` |
| A-006 | ACUTE |  | — | `findings/FIXED/A-005-rawfixer-4kn-stack-buffer-overflow.md` |
| A-007 | ACUTE |  | bddf4df | `findings/FIXED/A-007-extents-overflow-descriptors-off-by-two.md` |
| A-008 | ACUTE |  | — | `findings/FIXED/A-008-zfs-child-dir-zapobj-offset.md` |
| A-010 | ACUTE |  | — | `findings/FIXED/A-010-asf-carve-truncated-to-header-object.md` |
| A-011 | ACUTE |  | 18002e2b0 | `findings/FIXED/A-011-sqlite-header-endianness.md` |
| I-001 | INFORMATIONAL |  | — | `findings/FIXED/I-001-ntfs-metafile-filter-known-list-vs-blanket-dollar-skip.md` |
| I-002 | INFORMATIONAL |  | — | `findings/FIXED/I-002-fat12-bad-cluster-canonicalized-vs-og-raw-follow.md` |
| I-003 | INFORMATIONAL |  | — | `findings/FIXED/I-003-btrfs-core-exceeds-og-deep-scan-and-mirror-probing.md` |
| I-006 | INFORMATIONAL |  | — | `findings/FIXED/I-006-ext4-nanoseconds-superset-core-preserves-og-drops.md` |
| I-007 | INFORMATIONAL |  | — | `findings/FIXED/I-007-ext4-hfsplus-btrfs-core-reads-more-timestamps-than-og.md` |
| I-008 | INFORMATIONAL |  | — | `findings/FIXED/I-008-ldm-raid5-four-rotations-dormant.md` |
| I-009 | INFORMATIONAL |  | — | `findings/FIXED/I-009-carve-probing-and-block-io-supersets.md` |
| I-010 | INFORMATIONAL |  | — | `findings/FIXED/I-010-scope-exclusions-and-supersets-registered.md` |
| I-011 | INFORMATIONAL |  | 4a4e4c78 | `findings/FIXED/I-011-corpus-errata-session2-register.md` |
| I-012 | INFORMATIONAL |  | — | `findings/FIXED/I-012-refs-ads-gap-resolved-shared-limitation.md` |
| I-013 | INFORMATIONAL |  | — | `findings/FIXED/I-013-refs-host-os-runtime-gate-superset.md` |
| I-014 | INFORMATIONAL |  | — | `findings/FIXED/I-014-ext4-hfsplus-supersets-and-matches-register.md` |
| I-015 | INFORMATIONAL |  | — | `findings/FIXED/I-015-preview-architecture-register-og-socket-protocol-vs-core-reader.md` |
| I-016 | INFORMATIONAL |  | — | `findings/FIXED/I-016-engine-factory-contract-register-and-fs-code-spaces.md` |
| I-017 | INFORMATIONAL |  | — | `findings/FIXED/I-017-slice-layer-deliberate-supersets.md` |
| I-018 | INFORMATIONAL |  | — | `findings/FIXED/I-018-nvme-route-byte-parity-match.md` |
| I-019 | INFORMATIONAL |  | — | `findings/FIXED/I-019-epm-tool-ops-scope-register.md` |
| I-020 | INFORMATIONAL |  | — | `findings/FIXED/I-020-corpus-doc-corrections-s5.md` |
| I-021 | INFORMATIONAL |  | — | `findings/FIXED/I-021-session5-scope-and-deliberate-delta-registers.md` |
| I-022 | INFORMATIONAL |  | — | `findings/FIXED/I-022-session5-core-superset-registers.md` |
| I-023 | INFORMATIONAL |  | — | `findings/FIXED/I-023-drwcfg-persistent-config-channel-scope-register.md` |
| I-024 | INFORMATIONAL |  | — | `findings/FIXED/I-024-partitionrecoveryd-generator-sets-and-dat-persistence-register.md` |
| M-001 | MAJOR |  | — | `findings/FIXED/M-001-ntfs-record-size-rejects-where-og-clamps.md` |
| M-002 | MAJOR |  | — | `findings/FIXED/M-002-ebr-torn-chain-discards-walked-volumes.md` |
| M-005 | MAJOR |  | — | `findings/FIXED/M-005-carve-bitmap-gating.md` |
| M-007 | MAJOR |  | — | `findings/FIXED/M-007-ntfs-hard-link-fan-out-emits-n-per-record-vs-og-one.md` |
| M-008 | MAJOR |  | — | `findings/FIXED/M-008-ntfs-attribute-list-chase-scope-and-depth-superset.md` |
| M-009 | MAJOR |  | — | `findings/FIXED/M-009-rawfixer-read-path-bypasses-64kib-envelope.md` |
| M-010 | MAJOR |  | — | `findings/FIXED/M-010-ext4-btrfs-invalid-utf8-names-destroyed.md` |
| M-011 | MAJOR |  | 63d9067 | `findings/FIXED/M-011-fat-table-size-windows.md` |
| M-012 | MAJOR |  | cfae986 | `findings/FIXED/M-012-fat-mirror-mode-divergences.md` |
| M-013 | MAJOR |  | 18001eba0 | `findings/FIXED/M-013-exfat-deleted-set-checksum-dual-form-and-oracle.md` |
| M-014 | MAJOR |  | 180002f90 | `findings/FIXED/M-014-exfat-structural-entry-gates-missing.md` |
| M-015 | MAJOR |  | 6dc1e32 | `findings/FIXED/M-015-fat-table-validation-absent.md` |
| M-016 | MAJOR |  | — | `findings/FIXED/M-016-ldm-tocblock-candidates-diverge.md` |
| M-017 | MAJOR |  | 140510a0e | `findings/FIXED/M-017-lost-partition-scoring-model-divergent.md` |
| M-018 | MAJOR |  | — | `findings/FIXED/M-018-junk-filter-filterfolder-absent.md` |
| M-019 | MAJOR |  | 1800168b0 | `findings/FIXED/M-019-zfs-multivdev-fallback-and-checksum-verify-missing.md` |
| M-020 | MAJOR |  | 8dbfc2d2 | `findings/FIXED/M-020-btrfs-profile-machinery-and-default-subvol.md` |
| M-021 | MAJOR |  | 18000beb0 | `findings/FIXED/M-021-carve-video-repair-wiring-absent.md` |
| M-022 | MAJOR |  | 9af8014 | `findings/FIXED/M-022-refs-file-value-grammar-divergent.md` |
| M-023 | MAJOR |  | — | `findings/FIXED/M-023-filter-model-categories-and-coverage.md` |
| M-024 | MAJOR |  | — | `findings/FIXED/M-024-no-runtime-format-filter-surface-vs-og-rawconfig-ini.md` |
| M-025 | MAJOR |  | 0e838a3 | `findings/FIXED/M-025-carve-read-model-8mib-pingpong-sector-dispatch-pending-streaming.md` |
| M-026 | MAJOR |  | 7feb79b | `findings/FIXED/M-026-refs-deleted-recovery-model-stale-msb-page-sweep-missing.md` |
| M-027 | MAJOR |  | 7feb79b | `findings/FIXED/M-027-refs-object-identity-dedup-and-recycle-bin-special-casing-missing.md` |
| M-028 | MAJOR |  | — | `findings/FIXED/M-028-preview-caps-bypass-and-provenance-misattribution.md` |
| M-029 | MAJOR |  | f388cad | `findings/FIXED/M-029-query-model-sort-paging-bucket-ids-server-side-push.md` |
| M-030 | MAJOR |  | 63a9167 | `findings/FIXED/M-030-driver-utilization-winpe-gate-handle-ladder-chunking-offline.md` |
| M-031 | MAJOR |  | — | `findings/FIXED/M-031-refs-progress-one-terminal-tick.md` |
| M-032 | MAJOR |  | — | `findings/FIXED/M-032-slice-tail-read-clamp-vs-refusal.md` |
| M-033 | MAJOR |  | 180042350 | `findings/FIXED/M-033-bitlocker-in-stack-decrypt-vs-pre-unlock.md` |
| M-034 | MAJOR |  | — | `findings/FIXED/M-034-no-nas-remote-device-layer.md` |
| M-035 | MAJOR |  | — | `findings/FIXED/M-035-engine-side-filefilter-validation-missing.md` |
| M-036 | MAJOR |  | — | `findings/FIXED/M-036-nvme-health-field-coverage-units.md` |
| M-037 | MAJOR |  | — | `findings/FIXED/M-037-ext4-dir-block-parse-resilience-inverted.md` |
| M-038 | MAJOR |  | — | `findings/FIXED/M-038-zfs-sa-layout-hardcoded.md` |
| M-039 | MAJOR |  | 180012040 | `findings/FIXED/M-039-cross-engine-occupancy-bitmap-missing.md` |
| M-040 | MAJOR |  | 94c9dda | `findings/FIXED/M-040-boot-chain-repair-family-absent.md` |
| M-041 | MAJOR |  | 03ad46c | `findings/FIXED/M-041-damage-audit-check-phase-missing.md` |
| M-042 | MAJOR |  | — | `findings/FIXED/M-042-session-resume-lost-revalidation-missing.md` |
| M-043 | MAJOR |  | — | `findings/FIXED/M-043-ntfs-content-proof-and-backup-boot-missing.md` |
| M-044 | MAJOR |  | 180002580 | `findings/FIXED/M-044-udf-recognition-family-missing.md` |
| M-045 | MAJOR |  | — | `findings/FIXED/M-045-fat-boot-gate-family-diverges.md` |
| M-046 | MAJOR |  | — | `findings/FIXED/M-046-4kn-ldm-lba-and-exfat-plan-hardcodes.md` |
| M-047 | MAJOR |  | — | `findings/FIXED/M-047-session-stats-roundtrip-asymmetry.md` |
| M-048 | MAJOR |  | FFFE0000 | `findings/FIXED/M-048-user-extensible-signature-db-absent.md` |
| M-049 | MAJOR |  | — | `findings/FIXED/M-049-error-model-divergence-and-parity-claim.md` |
| M-050 | MAJOR |  | — | `findings/FIXED/M-050-session-node-model-roundtrip-gaps.md` |
| M-051 | MAJOR |  | FFFFFFFF | `findings/FIXED/M-051-raw-carve-provenance-fields-unpersisted.md` |
| M-052 | MAJOR |  | — | `findings/FIXED/M-052-fat-exfat-recycle-bin-pairing-missing.md` |
| M-053 | MAJOR |  | 180041b00 | `findings/FIXED/M-053-c-abi-consumer-fixture-stale-since-m015.md` |
| M-054 | MAJOR |  | d487928 | `findings/FIXED/M-054-exfat-allocation-bitmap-bit-cluster-off-by-two.md` |
| M-055 | MAJOR |  | 3b5397a | `findings/FIXED/M-055-winlayer-uncompilable-destio-nesting-transport-namespace.md` |
| M-056 | MAJOR |  | — | `findings/FIXED/M-056-tiff-family-size-basis-mix.md` |
| M-057 | MAJOR |  | — | `findings/FIXED/M-057-app-layer-uncompilable-cmd-apply-undeclared-e.md` |
| M-058 | MAJOR |  | — | `findings/FIXED/M-058-readpipe-cache-failure-fallback-use-after-free.md` |
| M-060 | MAJOR |  | — | `findings/FIXED/M-060-devsign-install-paths-diverge-config.md` |
| M-061 | MAJOR |  | — | `findings/FIXED/M-061-devsign-uninstall-silent-failure-unexercised.md` |
| M-062 | MAJOR |  | — | `findings/FIXED/M-062-devsign-install-ps1-start-failure-exit-zero.md` |
| M-063 | MAJOR |  | — | `findings/FIXED/M-063-devsign-environment-matrix-hvci-undocumented.md` |
| M-064 | MAJOR |  | 6b50bb8 | `findings/FIXED/M-064-driver-deployer-versioned-verified-contract-absent.md` |
| M-065 | MAJOR |  | — | `findings/FIXED/M-065-no-default-user-sign-set.md` |
| M-066 | MAJOR |  | — | `findings/FIXED/M-066-preview-archive-enumeration-fs-origin-missing.md` |
| M-067 | MAJOR |  | b07ee9d | `findings/FIXED/M-067-user-sign-abi-truncates-s039-widths.md` |
| M-068 | MAJOR |  | 5e7390b | `findings/FIXED/M-068-composite-double-start.md` |
| M-069 | MAJOR |  | — | `findings/FIXED/M-069-staged-scenarios-never-green.md` |
| M-070 | MAJOR |  | — | `findings/FIXED/M-070-stage-disk-param-type-coercion.md` |
| M-071 | MAJOR |  | 69507cb | `findings/FIXED/M-071-m018-regression-junk-gates-lost.md` |
| M-080 | MAJOR | preview | 5ccc0da | `findings/FIXED/M-080-thumbnail-budget-starves-photos.md` |
| M-083 | MAJOR | preview/abi | (this landing) | `findings/MAJOR/M-083-thumbnail-grade-promotion-absent.md` |
| M-102 | MAJOR | abi/preview | (this landing) | `findings/MAJOR/M-102-thumbnail-abi-export-absent-engine-landed.md` |
| M-081 | MAJOR |  | 9e5dedc | `findings/FIXED/M-081-crw-misattributed-mrw-prd-gate.md` |
| M-090 | MAJOR |  | 180053de0 | `findings/FIXED/M-090-dmg-footer-walk-and-cap-diverge-from-og.md` |
| M-088 | MAJOR | ci/packaging (the FloorVersion pin split + the CI-shape doc cluster) | (this landing, DFX-38) | `findings/FIXED/M-088-floorversion-pin-split-and-stale-ci-docs.md` (full Resolution preserved in the retired file; the landing record + the doc_truth_gate evidence in worklog.md DFX-38/M-088) |
| N-001 | MINOR |  | — | `findings/FIXED/N-001-STALE-DUPLICATE-REMOVED.md` |
| N-002 | MINOR |  | — | `findings/FIXED/N-002-fat-alternate-eoc-families-mark-chains-broken.md` |
| N-003 | MINOR |  | — | `findings/FIXED/N-003-exfat-entry-set-checksum-enforced-beyond-og.md` |
| N-004 | MINOR |  | — | `findings/FIXED/N-004-N-006-linuxfs-minor-divergences.md` |
| N-007 | MINOR |  | 18000ff60 | `findings/FIXED/N-007-I-004-refs-detection-strictness-and-version-superset.md` |
| N-008 | MINOR |  | FFFFFFFF | `findings/FIXED/N-008-MAJOR-M-005-carve-zip-padding-and-target-policy.md` |
| N-009 | MINOR |  | 0a68937 | `findings/FIXED/N-009-I-005-grouped-minors-and-supersets.md` |
| N-010 | MINOR |  | — | `findings/FIXED/N-010-fat-bad-sentinel-swallowed-by-reserved-range.md` |
| N-011 | MINOR |  | 367561d | `findings/FIXED/N-011-ntfs-ads-emitted-as-child-nodes-vs-og-drops.md` |
| N-012 | MINOR |  | e7deda8 | `findings/FIXED/N-012-ntfs-compressed-sparse-flag-source-and-mask-mismatch.md` |
| N-013 | MINOR |  | — | `findings/FIXED/N-013-dest-overlap-check-optional-and-first-extent-only.md` |
| N-014 | MINOR |  | — | `findings/FIXED/N-014-stage-checkpoint-not-atomic-previous-destroyed-on-crash.md` |
| N-015 | MINOR |  | — | `findings/FIXED/N-015-writer-no-disk-full-pre-check-or-fast-fail.md` |
| N-016 | MINOR |  | 56cce87 | `findings/FIXED/N-016-orchestrator-emit-boundary-events-double-call-bug.md` |
| N-017 | MINOR |  | — | `findings/FIXED/N-017-cancel-watcher-coverage-inconsistent-carve-vs-orchestrator.md` |
| N-018 | MINOR |  | — | `findings/FIXED/N-018-ubsan-not-enabled-in-any-ci-lane.md` |
| N-019 | MINOR |  | 5d6da5f | `findings/FIXED/N-019-missing-per-parameter-regression-tests-13-scenarios.md` |
| N-020 | MINOR |  | 56cce87 | `findings/FIXED/N-020-documentation-drift-five-stale-claims.md` |
| N-021 | MINOR |  | 315532800 | `findings/FIXED/N-021-fat-invalid-date-yields-nonzero-filetime-vs-og-zero.md` |
| N-022 | MINOR |  | 180041de0 | `findings/FIXED/N-022-fat-mirror-consult-on-bad-vs-og-primary-served.md` |
| N-023 | MINOR |  | ba55913 | `findings/FIXED/N-023-fat-exfat-gate-minors-group.md` |
| N-024 | MINOR |  | e7deda8 | `findings/FIXED/N-024-ntfs-gates-and-fields-minors-group.md` |
| N-025 | MINOR |  | fffffff8 | `findings/FIXED/N-025-ldm-geometry-minors-group.md` |
| N-026 | MINOR |  | 180012390 | `findings/FIXED/N-026-refs-page-and-gate-minors-group.md` |
| N-027 | MINOR |  | — | `findings/FIXED/N-027-carve-writer-minors-group.md` |
| N-028 | MINOR |  | 3928f33 | `findings/FIXED/N-028-hfsplus-sink-and-fork-minors-group.md` |
| N-029 | MINOR |  | — | `findings/FIXED/N-029-utf8-replacement-residuals.md` |
| N-030 | MINOR |  | 18000d670 | `findings/FIXED/N-030-rawsigns-confidence-field-dead-in-og-core-invents.md` |
| N-031 | MINOR |  | — | `findings/FIXED/N-031-carve-dispatch-and-stride-deltas.md` |
| N-032 | MINOR |  | — | `findings/FIXED/N-032-health-transports-csmi-usb-bridges-missing.md` |
| N-033 | MINOR |  | — | `findings/FIXED/N-033-analyser-registry-exports-consumed-by-fs-engines-no-core-abi.md` |
| N-034 | MINOR |  | 180004610 | `findings/FIXED/N-034-fat-lost-boot-gate-strictness-constants.md` |
| N-035 | MINOR |  | — | `findings/FIXED/N-035-lost-candidate-extent-data-area-anchored.md` |
| N-036 | MINOR |  | 180011e90 | `findings/FIXED/N-036-refs-datarun-recognizer-and-container-translation-divergence.md` |
| N-037 | MINOR |  | 18000e020 | `findings/FIXED/N-037-refs-runlist-granularity-validation-missing.md` |
| N-038 | MINOR |  | — | `findings/FIXED/N-038-hfsplus-fork-selection-predicate-byte-29.md` |
| N-039 | MINOR |  | — | `findings/FIXED/N-039-ntfs-usnjrnl-harvest-gate-mask-and-bounds.md` |
| N-040 | MINOR |  | 63a9167 | `findings/FIXED/N-040-abi-doc-truthfulness-defects.md` |
| N-041 | MINOR |  | — | `findings/FIXED/N-041-dead-code-rdaclient-and-volume-letter-to-disk.md` |
| N-042 | MINOR |  | — | `findings/FIXED/N-042-cli-health-temperature-fractional-digit-wrong.md` |
| N-043 | MINOR |  | 140577100 | `findings/FIXED/N-043-progress-liveness-watchdog-absent.md` |
| N-044 | MINOR |  | — | `findings/FIXED/N-044-cli-deep-flag-means-whole-disk-not-deep-stage.md` |
| N-045 | MINOR |  | — | `findings/FIXED/N-045-cache-budget-8-vs-4-mib.md` |
| N-046 | MINOR |  | 140643870 | `findings/FIXED/N-046-no-geometry-refresh.md` |
| N-047 | MINOR |  | — | `findings/FIXED/N-047-grouped-minors-subvol-naming-volume-node-attrbit.md` |
| N-048 | MINOR |  | — | `findings/FIXED/N-048-engine-logging-seam-absent.md` |
| N-049 | MINOR |  | 63a3a77 | `findings/FIXED/N-049-lost-record-field-richness.md` |
| N-050 | MINOR |  | 180002180 | `findings/FIXED/N-050-recognition-contract-boot-only.md` |
| N-051 | MINOR |  | 6D626577 | `findings/FIXED/N-051-mkv-doctype-discrimination-absent.md` |
| N-052 | MINOR |  | — | `findings/FIXED/N-052-heic-brand-relabel-absent.md` |
| N-053 | MINOR |  | — | `findings/FIXED/N-053-iiu-magic-labeled-rw2-vs-og-tiff-ep.md` |
| N-054 | MINOR |  | 494a5546 | `findings/FIXED/N-054-raf-jpeg-magic-strictness-both-directions.md` |
| N-055 | MINOR |  | — | `findings/FIXED/N-055-smart-thresholds-0xd1-never-read.md` |
| N-056 | MINOR |  | — | `findings/FIXED/N-056-windestio-temp-leak-on-failed-rename.md` |
| N-057 | MINOR |  | — | `findings/FIXED/N-057-tablelayout-ebr-dead-enum-and-diskmap-debt.md` |
| N-058 | MINOR |  | — | `findings/FIXED/N-058-session-load-validation-deltas.md` |
| N-059 | MINOR |  | 1800061f0 | `findings/FIXED/N-059-btrfs-zfs-session-extras-unpersisted.md` |
| N-060 | MINOR |  | — | `findings/FIXED/N-060-save-session-progress-cancel-absent.md` |
| N-061 | MINOR |  | — | `findings/FIXED/N-061-engine-runlog-and-auto-naming-absent.md` |
| N-062 | MINOR |  | — | `findings/FIXED/N-062-memory-monitor-absent.md` |
| N-063 | MINOR |  | 1800173f0 | `findings/FIXED/N-063-carve-default-naming-scheme-invented.md` |
| N-064 | MINOR |  | 97382b7 | `findings/FIXED/N-064-dict-password-dictionary-absent.md` |
| N-065 | MINOR |  | — | `findings/FIXED/N-065-writer-destination-policy-collision-format-and-mkdir-gates.md` |
| N-066 | MINOR |  | — | `findings/FIXED/N-066-session-identity-card-weaker-fields-and-no-boot-snapshot.md` |
| N-067 | MINOR |  | — | `findings/FIXED/N-067-exfat-upcase-table-validation-skipped.md` |
| N-068 | MINOR |  | — | `findings/FIXED/N-068-zip-eocd-anchor-and-lfh-sanity-gates-missing.md` |
| N-069 | MINOR |  | — | `findings/FIXED/N-069-cab-lowercase-mscf-registry-key-missing.md` |
| N-070 | MINOR |  | — | `findings/FIXED/N-070-carve-overlap-scoring-and-two-generation-fallback-missing.md` |
| N-071 | MINOR |  | — | `findings/FIXED/N-071-device-enumeration-breadth.md` |
| N-072 | MINOR |  | — | `findings/FIXED/N-072-btrfs-rescan-cache-and-zfs-label-polling-missing.md` |
| N-073 | MINOR |  | 7f03c4b | `findings/FIXED/N-073-ext4-catchbuffer-shifted-window-salvage-missing.md` |
| N-074 | MINOR |  | a4053c4 | `findings/FIXED/N-074-doc-drift-group-2-five-stale-claims.md` |
| N-075 | MINOR |  | — | `findings/FIXED/N-075-event-bus-query-signals-and-pull-progress-convention.md` |
| N-076 | MINOR |  | — | `findings/FIXED/N-076-fsfamily-reserved5-anchor-missing.md` |
| N-077 | MINOR |  | b7d4e22 | `findings/FIXED/N-077-deserialize-tree-props-leak-on-early-break.md` |
| N-078 | MINOR |  | — | `findings/FIXED/N-078-session-carve-note-truncation-loads-as-ok.md` |
| N-079 | MINOR |  | — | `findings/FIXED/N-079-ntfs-recycle-bin-scope-name-prefix-matched-everywhere.md` |
| N-080 | MINOR |  | — | `findings/FIXED/N-080-descriptor-offset-transposition-model-serial.md` |
| N-081 | MINOR |  | — | `findings/FIXED/N-081-filterfolder-pool-overread.md` |
| N-082 | MINOR |  | — | `findings/FIXED/N-082-recycle-fixture-overread.md` |
| N-084 | MINOR |  | — | `findings/FIXED/N-084-devsign-no-timestamp-server.md` |
| N-085 | MINOR |  | — | `findings/FIXED/N-085-devsign-powershell-window-posture.md` |
| N-086 | MINOR |  | — | `findings/FIXED/N-086-rawsigns-table-compiled-in-vs-file-loaded.md` |
| N-087 | MINOR |  | — | `findings/FIXED/N-087-fat83-name-oem-bytes-identity-mapped.md` |
| N-088 | MINOR |  | — | `findings/FIXED/N-088-m028-iinwindowstream-propagation-and-mov-drift.md` |
| N-089 | MINOR |  | — | `findings/FIXED/N-089-sqlite-fcpevent-subtype-relabel-missing.md` |
| N-094 | MINOR |  | — | `findings/FIXED/N-094-cli-help-sessions-verb-missing.md` |
| OQ-RS | ? |  | — | `findings/FIXED/OQ-RS-refs-session3.md` |
| OQ-Z | ? |  | — | `findings/FIXED/OQ-Z-E-FLT-session3-resolved.md` |
| OQ-SA-1 | ? |  | — | `findings/FIXED/OQ-SA-1-anchor-datum-fields.md` |
| OQ-T1 | ? |  | — | `findings/FIXED/OQ-T1-ntfs-secondary-claim-map-semantics.md` |
| OQ-SA-2 | ? |  | 1463221 | `findings/FIXED/OQ-SA-2-carve-family-decode-candidates.md` |
| OQ-T2 | ? |  | — | `findings/FIXED/OQ-T2-T5-fat-mirror-and-exfat-boot-checksum.md` |
| OQ-S003 | ? |  | 180003ea0 | `findings/FIXED/OQ-S003-journal-fallback-salvage.md` |
| OQ-S3-JPEG | ? |  | — | `findings/FIXED/OQ-S3-JPEG-fallback-and-continuation-accept.md` |
| OQ-SA-3 | ? |  | — | `findings/FIXED/OQ-SA-3-euuserdata-consumption-mode.md` |
| OQ-T6 | ? |  | — | `findings/FIXED/OQ-T6-T7-zfs-anchors-and-ldm-raid5-rotation.md` |
| OQ-T8 | ? |  | 18000cf60 | `findings/FIXED/OQ-T8-T9-carve-dispatch-and-calcpartition-scoring.md` |
| OQ-S2M-1 | ? |  | — | `findings/FIXED/OQ-S2M-1-ntfs-grading-locus.md` |
| OQ-S2M-2 | ? |  | — | `findings/FIXED/OQ-S2M-2-bonus-setter-zfs-pool-name-btrfs-anchor.md` |
| OQ-S4-1 | ? |  | — | `findings/FIXED/OQ-S4-1-grading-extension-table.md` |
| OQ-S4-2 | ? |  | — | `findings/FIXED/OQ-S4-2-raw-naming-hook-consumption.md` |
| OQ-S5-1 | ? |  | — | `findings/FIXED/OQ-S5-1-checkversion-consumer.md` |
| OQ-S6-1 | ? |  | — | `findings/FIXED/OQ-S6-1-partitionrecoveryd-generator-set-semantics.md` |
| OQ-S6-2 | ? |  | — | `findings/FIXED/OQ-S6-2-btrfs-rc4-prga-target.md` |
| OQ-S6-3 | ? |  | — | `findings/FIXED/OQ-S6-3-craw-reader-f4-boolean-and-anchor-b.md` |
| OQ-S20-1 | ? |  | — | `findings/FIXED/OQ-S20-1-bitlocker-fve-kdf-interop.md` |
| RESOLVED | ? |  | 3678cbf | `findings/FIXED/RESOLVED-INDEX.md` |
| S-001 | SEVERE |  | — | `findings/FIXED/S-001-ntfs-overwritten-deleted-emitted-not-destroyed.md` |
| S-002 | SEVERE |  | — | `findings/FIXED/S-002-hfsplus-attributes-tree-decmpfs-not-walked.md` |
| S-003 | SEVERE |  | 180010110 | `findings/FIXED/S-003-hfsplus-journal-not-replayed.md` |
| S-004 | SEVERE |  | — | `findings/FIXED/S-004-ext4-journal-salvage-descriptor-referenced-only.md` |
| S-005 | SEVERE |  | — | `findings/FIXED/S-005-ext4-btrfs-zfs-names-byte-lifted-mojibake.md` |
| S-006 | SEVERE |  | 00BAB10C | `findings/FIXED/S-006-lost-partition-probes-missing-linux-hfs-btrfs-zfs-refs.md` |
| S-007 | SEVERE |  | D0CF11E0 | `findings/FIXED/S-007-carve-missing-common-formats.md` |
| S-008 | SEVERE |  | — | `findings/FIXED/S-008-rawfixer-subset-and-not-wired-prescan.md` |
| S-009 | SEVERE |  | e30b84d | `findings/FIXED/S-009-MAJOR-M-006-diskmap-surface-and-fuzz-ci.md` |
| S-010 | SEVERE |  | — | `findings/FIXED/S-010-rawfixer-write-path-no-volume-lock.md` |
| S-011 | SEVERE |  | — | `findings/FIXED/S-011-exfat-tz-offset-applied-vs-og-ignored.md` |
| S-012 | SEVERE |  | 180017190 | `findings/FIXED/S-012-ntfs-single-pass-guid-dump-and-rescue-missing-destroy-mis-scoped.md` |
| S-013 | SEVERE |  | 180021c60 | `findings/FIXED/S-013-exfat-fat-over-32mib-dropped-1-cluster-truncation.md` |
| S-014 | SEVERE |  | FFFFFFFF | `findings/FIXED/S-014-bitlocker-fveautounlock-wrong-path-no-offline-hive.md` |
| S-015 | SEVERE |  | 1800205b0 | `findings/FIXED/S-015-rawsigns-min-max-units-bytes-vs-kib.md` |
| S-016 | SEVERE |  | — | `findings/FIXED/S-016-errorrange-event-dead-snapshot-error-map-fed-after-emits.md` |
| S-017 | SEVERE |  | — | `findings/FIXED/S-017-ftyp-family-signatures-unreachable-mp4-mov-m4a-heic.md` |
| S-018 | SEVERE |  | 18004bbd0 | `findings/FIXED/S-018-jpeg-eoi-policy-og-salvages-truncated-core-rejects.md` |
| S-019 | SEVERE |  | 180004ce0 | `findings/FIXED/S-019-lost-bitlocker-fve-partition-probe-missing.md` |
| S-020 | SEVERE |  | 9af8014 | `findings/FIXED/S-020-refs-attribute-layer-recovery-gap-resident-inline-and-multi-stream.md` |
| S-021 | SEVERE |  | 4A4E4C78 | `findings/FIXED/S-021-hfsplus-journal-deleted-record-harvest-missing.md` |
| S-022 | SEVERE |  | 180045510 | `findings/FIXED/S-022-ntfs-indx-directory-entry-second-source-missing-in-core.md` |
| S-023 | SEVERE |  | 140089630 | `findings/FIXED/S-023-no-post-scan-grading-engine-crecoverability-unimplemented.md` |
| S-024 | SEVERE |  | 00458350 | `findings/FIXED/S-024-health-grading-layer-missing.md` |
| S-025 | SEVERE |  | 7fd00e0 | `findings/FIXED/S-025-ext4-journal-reparenting-missing.md` |
| S-026 | SEVERE |  | 7fd00e0 | `findings/FIXED/S-026-ext4-meta-bg-gdt-resolution-missing.md` |
| S-027 | SEVERE |  | 180001b00 | `findings/FIXED/S-027-ext4-backup-superblock-lost-volume-scan-missing.md` |
| S-028 | SEVERE |  | — | `findings/FIXED/S-028-btrfs-xattr-recovery-missing.md` |
| S-029 | SEVERE |  | 1800566d0 | `findings/FIXED/S-029-lost-partition-apply-table-write-path-missing.md` |
| S-030 | SEVERE |  | 140178250 | `findings/FIXED/S-030-disk-image-scan-targets-missing.md` |
| S-031 | SEVERE |  | 140c41aa0 | `findings/FIXED/S-031-carved-files-never-marked-deleted.md` |
| S-032 | SEVERE |  | 180012d00 | `findings/FIXED/S-032-archive-carve-inner-entry-inventory-absent.md` |
| S-033 | SEVERE |  | — | `findings/FIXED/S-033-exfat-root-dentry-type-constants-shifted-real-bitmap-skipped.md` |
| S-034 | SEVERE |  | 180006b40 | `findings/FIXED/S-034-zfs-ring-txg-history-not-walked.md` |
| S-036 | SEVERE |  | — | `findings/FIXED/S-036-devsign-install-ps1-elevation-guard-crash.md` |
| S-037 | SEVERE |  | — | `findings/FIXED/S-037-carve-fh11-cpt-formats-missing.md` |
| S-038 | SEVERE |  | — | `findings/FIXED/S-038-app-installer-surface-absent.md` |
| S-039 | SEVERE |  | — | `findings/FIXED/S-039-user-signs-caps-reject-og-euuserdata.md` |
| S-042 | SEVERE |  | f6e303e | `findings/FIXED/S-042-wmf-placeable-grammar-contradicts-og.md` |
| S-043 | SEVERE | preview | d0787e9 (base) + the closing-fix riders | `findings/FIXED/S-043-thumbnail-exif-orientation-not-applied.md` |
| S-044 | SEVERE |  | 7b7e095 | `findings/FIXED/S-044-autosave-gates-missing.md` |
| S-047 | SEVERE | carve | a3abfbe | `findings/FIXED/S-047-mpeg-ts-m2ts-analyser-absent-sync-scanners-decoded.md` |
| M-084 | MAJOR | (mid-flight landing) | 16475b8 | `findings/FIXED/M-084-mdb-accdb-second-signature-missing.md` |
| M-085 | MAJOR | preview | (the parallel lane) | `findings/FIXED/M-085-tiff-lzw-refused.md` |
| M-091 | MAJOR | (mid-flight landing) | 16475b8 | `findings/FIXED/M-091-mdb-page-chain-size-walk-decoded-bounded-extent-stands-in.md` |
| M-100 | MAJOR | preview | closed-as-covered — `6d0f16c` (M-066+N-088, an ancestor of the finding's own baselines; the session-10 lane-B verification: `evidence/s10-verification-pass-b.md`) | `findings/MAJOR/M-100-archive-browse-inside-preview-absent.md` (retired by the session-10 lane-B push; never lived in FIXED/ — the capability predated the filing) |
| N-092 | MINOR | (mid-flight landing) | 5ed862f | `findings/FIXED/N-092-thumbnail-refusal-matrix-and-cache.md` |
| N-124 | MINOR | (mid-flight landing) | c75755d | `findings/FIXED/N-124-transport-module-rows-superseded-by-ddrv-wave.md` |
| N-116 | MINOR | (session-38 minor-first lane; the Resolution + RED->GREEN proof live in the retired file and worklog.md's S38 block) | 632d21e | `findings/FIXED/N-116-session-list-row-ordering-diverges-og-time-sort.md` |
| N-117 | MINOR | (session-38 doc-truth batch A; doc-only per the audit's own plan — Resolution in history at 954c386) | 954c386 | `findings/MINOR/N-117-w1abc-adjacent-doc-drift-plan-shape-and-cite-lines.md` |
| N-118 | MINOR | (session-38 doc-truth batch A; doc-only — Resolution in history at 954c386) | 954c386 | `findings/MINOR/N-118-reference-api-gap-rows-stale-after-w1-landings.md` |
| N-103 | MINOR | (session-38 doc-truth batch A; doc-only — Resolution in history at 954c386, incl. the first-hand EURawImg/EUImgRepairFile import-table verification) | 954c386 | `findings/MINOR/N-103-thumbnail-api-rows-stale.md` |
| N-095 | MINOR | (session-38 doc-truth batch B; doc-only — Resolution in history at e1e5124) | e1e5124 | `findings/MINOR/N-095-doc-truth-batch-stale-texts.md` |
| N-108 | MINOR | (session-38 doc-truth batch B; doc-only — Resolution in history at e1e5124; the rawsigns comment + SCAN-ENGINE-DELTA halves FIXED-ALREADY by parallel lanes) | e1e5124 | `findings/MINOR/N-108-product-docs-stale-constants.md` |
| N-119 | MINOR | (session-38 doc-truth batch B; doc-only — Resolution in history at e1e5124; D-88 re-adjudicated against the EuShare decode re-read first-hand) | e1e5124 | `findings/MINOR/N-119-d88-nas-premise-stale-eushare-decode-answers.md` |
| N-104 | MINOR | (session-38 doc-truth batch C; doc-only — Resolution in history at 13245c0) | 13245c0 | `findings/MINOR/N-104-doc-truth-defects.md` |
| N-097 | MINOR | (session-38 doc-truth batch C; doc-only — the FVE-snapshot security datum registered) | 13245c0 | `findings/MINOR/N-097-auto-txt-misclassified-fve-key-snapshot.md` |
| N-102 | MINOR | (session-38 doc-truth batch C; doc-only — the propagation note flipped) | 13245c0 | `findings/MINOR/N-102-m024-toggle-count-half-propagated.md` |
| N-123 | MINOR | (session-38 doc-truth batch C; doc-only — the 11-verb recount + exit 4) | 13245c0 | `findings/MINOR/N-123-cli-module-commands-row-stale-11-verbs.md` |
| N-101 | MINOR | (session-38 doc-truth batch C; one live residue fixed + the rest FIXED-ALREADY with reproductions — Resolution at 13245c0) | 13245c0 | `findings/MINOR/N-101-surface-inventory-arithmetic-and-unregistered-surfaces.md` |
| N-120 | MINOR | (session-38 doc-truth batch C; live residues fixed + the rest FIXED-ALREADY with reproductions — Resolution at 13245c0) | 13245c0 | `findings/MINOR/N-120-audit-trail-pointer-and-arithmetic-drifts.md` |
| N-099 | MINOR | (FIXED-ALREADY — classified session 38: E55 + the live WPG/ctor re-pins carry the corrections; reproduction in the Resolution at 13245c0) | (parallel session-8/SCAN-4 lanes; E55) | `findings/MINOR/N-099-batch1-citation-truth-pass.md` |
| N-106 | MINOR | (FIXED-ALREADY — classified session 38: the SI:86 disposition row; reproduction in the Resolution at 13245c0) | (parallel session-8 wave-2 lane) | `findings/MINOR/N-106-getdriverinfo-disposition.md` |
| N-091 | MINOR | (session-38 registration batch D; D-108/D-109/D-110 landed — Resolution in history at the batch D commit) | (this session) | `findings/MINOR/N-091-parity-deltas-registration-gaps.md` |
| N-098 | MINOR | (session-38 batch D; the contract sentence + mirrors + the `EnumerationRefreshContractPinned` pin, RED→GREEN proven) | (this session) | `findings/MINOR/N-098-enum-refresh-contract-sentence.md` |
| N-105 | MINOR | (FIXED-ALREADY — classified session 38: D-106 is the demanded deferral record; reproduction in the Resolution) | (parallel S-044 lane's rider) | `findings/MINOR/N-105-autosave-folder-suffix-naming.md` |

## Go-forward convention (established by this purge, per the user instruction)

Fix lanes resolving a finding: (1) record the Resolution section in the
finding file, (2) append the row to this ledger (ID / severity / fix commit /
retired path), (3) `git rm` the file — do NOT re-create `findings/FIXED/`.
The active board stays the only thing in the findings tree; history is the
archive; this ledger is the index. A finding moved into a re-created
`findings/FIXED/` directory violates the convention and the next audit
lane will retire it again.

| S-041 | SEVERE | carve (the batch-2 umbrella) | school-1 `b92ebef` + M-081 `9e5dedc` + M-090 `771e2ec` + M-084/M-091 `16475b8` | `findings/SEVERE/S-041-s007-batch2-carve-remainder.md` (FIXED-as-umbrella, SA-10 — school 1 landed as code `b92ebef`; CRW/MRW/LowDoc/HLP `9e5dedc`; DMG `771e2ec` + MDB `16475b8` + MTS/M2TS `a3abfbe` resolved mid-flight; the still-open school-2/3 formats child-filed: M-092, M-093, M-096, M-098, M-099, N-111, N-112, N-113, N-115; the ONE/3DS walk follow-ups owned by M-106. The S-007 precedent: the umbrella closes when every remainder has its own owner. Full Resolution record in the retired file.) |
