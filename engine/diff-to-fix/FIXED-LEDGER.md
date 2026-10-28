# FIXED-LEDGER.md — the master index of resolved findings (264 finding rows + 20 resolved OQ rows at session-39 open; 259 at the SA-10B purge whose convention created this ledger)

> **What this is:** the user-mandated purge (session SA-10B / L-A) removed
> the 259 per-finding files under `findings/FIXED/` from the working tree
> (250 at the session-9 board-truth pass + the parallel fix lanes' five
> landings since: S-042 f6e303e, S-044 7b7e095e (the 9496f3c token was a pre-rebase phantom, repaired session 39), M-071, M-090 9daa787 (the 771e2ec token likewise),
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

**Counts (row-count truth, session-39B post-M-109/M-112 re-count, 2026-10-29): 300 resolved rows — 278 main-pattern (A/S/M/N/I) + 22 OQ-prefixed** (+I-030 this push) (the session-39 296-count verified against the filesystem; +N-134 this push), each row one retired file (no duplicate IDs; the A-005/A-006 renumber pair counts both IDs by design). The filesystem is the truth (the whole table re-counted twice this window — session-11B's 285/265+20 pass, then session-39's 287/267+20 after the M-072 row landing and the N-136/N-093 committed-marker repair at the table tail kept both rows; the prior top lines' 276–282 figures were push-window estimates that never re-counted the table — the drift is the N-129(numeric-defects) class applied to the ledger's own top line; the `b5da834` repair commit left the second occurrence of the conflict markers unswept — both sides preserved as dated history below).

**(history, HEAD side of the b5da834 conflict):** 282-claim — 280 rows at `e56baac` + M-100 (the batch-C-conform 276 `0872ac2`/`64a3b18` + S-041 resolved-as-umbrella by the SA-10 lane) · 98 with a fix-commit recorded in-file
**(history, b5da834 side):** 281-claim — the same lineage with the rebase-rewritten batch SHAs (e4f3184 / 995ba5d / 32ef33b)
**(history, session-39 re-count at `35bee726`, pre-rebase):** 284 rows = 264 main-pattern + 20 OQ — the session-39 open tree before the M-072/M-102/M-083/N-093/N-136 row landings; convergent with the session-11B re-count (the delta is exactly those five rows + the N-136 renumber). Session-39 also resolved the SECOND committed-marker occurrence at the table tail (the N-136/N-093 row collision a parallel lane merged with markers) — both rows kept, markers dropped.

**Counts:** 270 resolved findings (255 at the purge commit + the mid-flight parallel landings absorbed at rebase: M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e`, N-117/N-118/N-103 `e4f3184`, N-095/N-108/N-119 `995ba5d` session-38 doc-truth batches A+B; S-043 `b3938c8` moved its file into the re-created FIXED/ dir BEFORE this convention was readable - its row is pending, the next audit lane retires the file) · 98 with a fix-commit recorded in-file ·

**Counts (268, the prior window's snapshot):** 255 at the purge commit + the mid-flight parallel landings absorbed at rebase: M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e`, N-117/N-118/N-103 `e4f3184`, N-095/N-108/N-119 `995ba5d` session-38 doc-truth batches A+B.

**Counts (265, the batch-A snapshot):** 255 at the purge commit + M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e` session-38, N-117/N-118/N-103 `e4f3184` session-38 doc-truth batch A.

**Counts (264, the closing-fix lane's snapshot):** 255 at the purge commit + M-084/M-091 `16475b8`, N-092 `5ed862f`, N-124 `c75755d`, M-088 DFX-38, N-116 `632d21e` session-38; S-043 and M-080 re-created FIXED/ before the convention was readable - their files retired to history and their rows added by the closing-fix lane's reconciliation.
**Counts:** 280 resolved findings (276 at the batch-C conform `0872ac2` + **S-041 resolved-as-umbrella by the SA-10 lane**; the purge's 255 + every mid-flight landing through batch C — N-116, N-117/N-118/N-103, N-095/N-108/N-119, N-104/N-097/N-102/N-123/N-101/N-120, N-099/N-106 — + S-043/M-080/M-085/S-047 conformed + S-041/N-101) · 98 with a fix-commit recorded in-file ·

**Counts:** 280 resolved findings (276 at the batch-C conform `0872ac2` + **S-041 resolved-as-umbrella by the SA-10 lane**; the purge's 255 + every mid-flight landing through batch C — N-116, N-117/N-118/N-103, N-095/N-108/N-119, N-104/N-097/N-102/N-123/N-101/N-120, N-099/N-106 — + S-043/M-080/M-085/S-047 conformed + S-041/N-101) · 98 with a fix-commit recorded in-file ·

**Counts:** 280 resolved findings (276 at the batch-C conform `0872ac2` + **S-041 resolved-as-umbrella by the SA-10 lane**; the purge's 255 + every mid-flight landing through batch C — N-116, N-117/N-118/N-103, N-095/N-108/N-119, N-104/N-097/N-102/N-123/N-101/N-120, N-099/N-106 — + S-043/M-080/M-085/S-047 conformed + S-041/N-101) · 98 with a fix-commit recorded in-file ·
**Ledger generated:** session SA-10B (2026-10-29), tree `62074bf`-lineage.

### S11-lead erratum — the "as recorded in-file" census (quantifies the phantom-SHA class; complements the session-39 sweep)

The Fix-commit column is **"as recorded in-file"** — it copies what each retired
file's own Resolution recorded. The session-11-lead truth pass (a `git cat-file -t`
walk over every SHA-like token — the guard N-135's fix plan proposed, executed
audit-side) censused the class the session-39 sweep did not reach: **54 ancient rows
carry non-SHA cells** — 40 decompile addresses in image-base form (e.g. `180010720`)
and 14 magic constants / numeric literals (e.g. `FFFFFFFF`, `D0CF11E0`, `4a4e4c78`).
These were never commits; the real fix per row is recoverable via
`git log --all --oneline --grep '<ID>'`. Affected rows: A-005, A-011, M-013, M-014,
M-017, M-019, M-020, M-021, M-033, M-039, M-044, M-053, N-007, N-008, N-021, N-022,
N-025, N-026, N-030, N-034, N-036, N-037, N-043, N-046, N-050, N-051, N-054, N-059,
N-063, OQ-S003, OQ-T8, S-003, S-006, S-007, S-012, S-013, S-014, S-015, S-018,
S-019, S-021, S-022, S-024, S-027, S-029, S-030, S-031, S-032, S-034 (and I-011).
The mechanical backfill (the cat-file guard over every hex token in `diff-to-fix/`)
belongs to the fix lane; this census is the audit-side truth record.

> **Session-38 convergence note (2026-10-29, post-purge):** the M-071 row
> above carries a three-lane convergence (the landed fix-lane `ce2405a`,
> the DFX-38 lane's addendum, and this session-38 execution lane's
> complementary landing `63efde8` — the ONE merged Windows-CI step
> `CLI junk-filter contract (M-071 - tree + recover-all)` over the
> committed fixture `tests/data/images/junk-gate-fat16.raw`, the
> Linux-suite substrate proof
> `FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet`, and the
> explicit-selection pin). The retired file's full session-38 convergence
> addendum is preserved in git history (the `63efde8` parent's tree) and
> summarized in worklog.md's S38-M071-CONVERGE entry + SUMMARY.md's
> session-38 reconciliation delta.

| ID | Severity | Module | Fix commit (as recorded in-file) | Retired file (history path) |
|---|---|---|---|---|
| A-001 | ACUTE |  | — | `findings/FIXED/A-001-gpt-type-guids-wrong-ldm-gpt-dynamic-disks-fail.md` |
| A-002 | ACUTE |  | 1cb6a43 | `findings/FIXED/A-002-a001-fix-corrupted-ldm-gpt-guids-real-dynamic-disks-fail.md` |
| A-003 | ACUTE |  | — | `findings/FIXED/A-003-fix-part-writes-at-disk-lba0-slice-offsets-never-rebased.md` |
| A-004 | ACUTE |  | — | `findings/FIXED/A-004-eventrange-fires-through-config-sink.md` |
| A-005 | ACUTE |  | 604bd2cd (the 180010720 token was the FUN_ mis-paste — repaired session 39, the phantom-SHA sweep) | `findings/FIXED/A-005-hfsplus-btree-record-span-inverted.md` |
| A-006 | ACUTE |  | — | `findings/FIXED/A-005-rawfixer-4kn-stack-buffer-overflow.md` |
| A-007 | ACUTE |  | bddf4df | `findings/FIXED/A-007-extents-overflow-descriptors-off-by-two.md` |
| A-008 | ACUTE |  | — | `findings/FIXED/A-008-zfs-child-dir-zapobj-offset.md` |
| A-010 | ACUTE |  | — | `findings/FIXED/A-010-asf-carve-truncated-to-header-object.md` |
| A-011 | ACUTE |  | c8999987 (the 18002e2b0 token was the FUN_ mis-paste — repaired session 39) | `findings/FIXED/A-011-sqlite-header-endianness.md` |
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
| I-030 | INFORMATIONAL | audit-trail structure (the module map) | session-11B (the reghive.md + apply.md rows written; the table.md gap fixed earlier by N-104) | `findings/FIXED/I-030-module-row-structure-gaps.md` |
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
| M-017 | MAJOR |  | 48a6c099 (the 140510a0e token was the FUN_ mis-paste — repaired session 39) | `findings/FIXED/M-017-lost-partition-scoring-model-divergent.md` |
| M-018 | MAJOR |  | — | `findings/FIXED/M-018-junk-filter-filterfolder-absent.md` |
| M-019 | MAJOR |  | 1800168b0 | `findings/FIXED/M-019-zfs-multivdev-fallback-and-checksum-verify-missing.md` |
| M-020 | MAJOR |  | f830db90 (the 8dbfc2d2 token was the FUN_ mis-paste — repaired session 39) | `findings/FIXED/M-020-btrfs-profile-machinery-and-default-subvol.md` |
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
| M-041 | MAJOR |  | 612abc48 (the 03ad46c token was a CORPUS-repo commit, not a core fix — repaired session-11-lead) | `findings/FIXED/M-041-damage-audit-check-phase-missing.md` |
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
| M-064 | MAJOR |  | dfb2761 (the core-side FIXED move; the fix itself = rda-driver v0.2.4 323e75a, cross-repo — the 6b50bb8 token was a pre-rebase phantom, repaired session 39) | `findings/FIXED/M-064-driver-deployer-versioned-verified-contract-absent.md` |
| M-065 | MAJOR |  | — | `findings/FIXED/M-065-no-default-user-sign-set.md` |
| M-066 | MAJOR |  | — | `findings/FIXED/M-066-preview-archive-enumeration-fs-origin-missing.md` |
| M-067 | MAJOR |  | b07ee9d | `findings/FIXED/M-067-user-sign-abi-truncates-s039-widths.md` |
| M-068 | MAJOR |  | 5e7390b | `findings/FIXED/M-068-composite-double-start.md` |
| M-069 | MAJOR |  | — | `findings/FIXED/M-069-staged-scenarios-never-green.md` |
| M-070 | MAJOR |  | — | `findings/FIXED/M-070-stage-disk-param-type-coercion.md` |
| M-071 | MAJOR |  | 69507cb (+ the session-38 convergence landing: the Windows-CI execution proof, the committed Linux-proven fixture, the trail repairs) | `findings/FIXED/M-071-m018-regression-junk-gates-lost.md` |
| M-079 | MAJOR | session/CLI | 0b76757c (f43b747 was a pre-rebase phantom, repaired session 39) | `findings/MAJOR/M-079-cli-autosave-volume-label-unfilled.md` — **S10-F addendum (2026-10-29): the fix's image-target leg (the image-stem fallback) is a registered divergence** — the og's image-scan label is the ENGINE-side on-disk decode (NTFS $Volume $VOLUME_NAME; ext4 s_volume_name), not the filename; the engine-decode absence + the completion path = **N-133** (evidence: s10f-engine-volume-labels.md) |
| M-080 | MAJOR | preview | 5ccc0da | `findings/FIXED/M-080-thumbnail-budget-starves-photos.md` |
| M-083 | MAJOR | preview/abi | e56baac7 (thumbnail ABI two-call) | `findings/MAJOR/M-083-thumbnail-grade-promotion-absent.md` (deleted by the parallel fix lane fce38c3d; Resolution in-file — content recoverable there) — **session-11-lead independent verification: PARITY-VERIFIED, the engine layer executed standalone 40/40 checks zero-ASan (evidence/s11-board-truth.md)** |
| M-102 | MAJOR | abi/preview | e56baac7 (cr_ui_thumbnail_generate) | `findings/MAJOR/M-102-thumbnail-abi-export-absent-engine-landed.md` (deleted by the parallel fix lane fce38c3d; Resolution in-file — content recoverable there) — **session-11-lead independent verification: PARITY-VERIFIED (evidence/s11-board-truth.md; the residual header-contradiction filed as N-137)** |
| M-108 | MAJOR | preview (the TxtViewFiles text-eligibility model — the generated whitelist + the BOM-aware snippet + the extensionless early return; the count corrected 119→97 tokens/92 unique) | aac75bba | `findings/MAJOR/M-108-preview-text-eligibility-model-diverges.md` (retired to history; the Resolution rides the fix commit) |
| M-109 | MAJOR | preview (the 200 MB thumbnail eligibility anchor — M-080's refuted no-cap doctrine re-pinned; the 32–200 MB band served; M-112 discovered and fixed in the same test) | 5103ecd8 | `findings/MAJOR/M-109-m080-landing-stopped-at-32mib-og-200mb-anchor-not-adopted.md` (retired to history; the Resolution rides the fix commit) |
| M-112 | MAJOR | preview (NEW-FINDING §8: the StbioSource single-shot read vs the 4 MiB kMaxRead clamp — every photo-scale PNG was outofdata; the read loop) | 5103ecd8 | `findings/MAJOR/M-112-stbio-read-cb-single-shot-breaks-large-png-idats.md` (retired to history; the Resolution rides the fix commit) |
| M-110 | MAJOR | btrfs (the Synology @eaDir descent suppression — the EXACT-name decode, prefix corrected by binary disassembly; the node emitted empty) | b95c924b | `findings/MAJOR/M-110-btrfs-eadir-subtree-suppression-absent.md` (retired to history; the Resolution rides the fix commit) |
| M-081 | MAJOR |  | 9e5dedc | `findings/FIXED/M-081-crw-misattributed-mrw-prd-gate.md` |
| M-082 | MAJOR | (the offline Windows OS profile — the og DeviceManager's CPartitionBootInfo hive walk; FIXED at 1f714884, retired at f8bb4d21 whose "ledger row" claim never landed — the row added by the session-39 truth repair, the N-135 item-4 completion) | 1f714884 | `findings/MAJOR/M-082-device-manager-offline-os-identification.md` |
| M-090 | MAJOR |  | 9daa787 (the 180053de0 token was the FUN_ mis-paste — repaired session 39; the N-135 item-4 original) | `findings/FIXED/M-090-dmg-footer-walk-and-cap-diverge-from-og.md` |
| M-088 | MAJOR | ci/packaging (the FloorVersion pin split + the CI-shape doc cluster) | 41d88297 (the fix commit, which also retired the file — repaired session-11-lead) | `findings/MAJOR/M-088-floorversion-pin-split-and-stale-ci-docs.md` (retired directly from MAJOR/ — the originally-recorded FIXED/ path never existed in history; path corrected session-11-lead; the landing record + the doc_truth_gate evidence in worklog.md DFX-38/M-088) |
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
| N-054 | MINOR |  | b2f48c64 (the 494a5546 token was the FUN_ mis-paste — repaired session 39) | `findings/FIXED/N-054-raf-jpeg-magic-strictness-both-directions.md` |
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
| OQ-S8-1 | ? | mom-capabilities | (this session, S10-F) | `findings/FIXED/OQ-S8-1-cwindowsfs-live-fs-adapter-consumer.md` — **RESOLVED: the consumer is the og's user-visible in-app partition file browser** (the ExplorerPartition chain: DRWUI:438891/:1421116 → PartitionRecovery:8481-8492 → FileSystem.mo's vtable+0x38 → CWindowsFS) + two internal consumers (used-space; root-openability); converts to **M-111** (the capability finding) |
| OQ-S10-2 | ? | og-tree register | (this session, S10-F) | `findings/FIXED/OQ-S10-2-videoviewer-sfc-unknown-payload.md` — **RESOLVED: dead payload** (the decisive probe ran: zero consumers of the " cfs-"/FAN. container across every reversed binary + string table; hypotheses 1-2 refuted); registered via N-134 |
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
| S-031 | SEVERE |  | the N-015/S-031/msvc arc, converged at 26d842b6 + the residual 7514368a (the 140c41aa0 token was the FUN_ mis-paste — repaired session 39) | `findings/FIXED/S-031-carved-files-never-marked-deleted.md` |
| S-032 | SEVERE |  | 180012d00 | `findings/FIXED/S-032-archive-carve-inner-entry-inventory-absent.md` |
| S-033 | SEVERE |  | — | `findings/FIXED/S-033-exfat-root-dentry-type-constants-shifted-real-bitmap-skipped.md` |
| S-034 | SEVERE |  | 79ca47ba (the 180006b40 token was the FUN_ mis-paste — repaired session 39) | `findings/FIXED/S-034-zfs-ring-txg-history-not-walked.md` |
| S-036 | SEVERE |  | — | `findings/FIXED/S-036-devsign-install-ps1-elevation-guard-crash.md` |
| S-037 | SEVERE |  | — | `findings/FIXED/S-037-carve-fh11-cpt-formats-missing.md` |
| S-038 | SEVERE |  | — | `findings/FIXED/S-038-app-installer-surface-absent.md` |
| S-039 | SEVERE |  | — | `findings/FIXED/S-039-user-signs-caps-reject-og-euuserdata.md` |
| S-042 | SEVERE |  | f6e303e | `findings/FIXED/S-042-wmf-placeable-grammar-contradicts-og.md` |
| S-043 | SEVERE | preview | d0787e9 (base) + the closing-fix riders | `findings/FIXED/S-043-thumbnail-exif-orientation-not-applied.md` |
| S-044 | SEVERE |  | 7b7e095 | `findings/FIXED/S-044-autosave-gates-missing.md` |
| S-045 | SEVERE | preview (the TIFF-RAW half FIXED at 021ae079; the codec-bound halves — HEIC via libheif+libde265, WebP via libwebp, PDF via pdfium, video key-frames — spun out as tracked-open OQ-S10-1 with named vendoring dependencies; the row added by the session-39 truth repair — the file's 06cd56e4 retirement cites the phantom 3aba383, the real fix is 021ae079) | 021ae079 | `findings/SEVERE/S-045-thumbnail-format-coverage-video-raw-heic.md` (retired to history; Resolution at 06cd56e4) |
| S-047 | SEVERE | carve | a3abfbe | `findings/FIXED/S-047-mpeg-ts-m2ts-analyser-absent-sync-scanners-decoded.md` |
| M-084 | MAJOR | (mid-flight landing) | 8a7055ea (16475b8 was a pre-rebase phantom, repaired session 39) | `findings/FIXED/M-084-mdb-accdb-second-signature-missing.md` |
| M-085 | MAJOR | preview | fb7cf30 | `findings/FIXED/M-085-tiff-lzw-refused.md` |
| M-091 | MAJOR | (mid-flight landing) | 8a7055ea (16475b8 was a pre-rebase phantom, repaired session 39) | `findings/FIXED/M-091-mdb-page-chain-size-walk-decoded-bounded-extent-stands-in.md` |
| M-092 | MAJOR | carve (the HTML four-key chain) | eb1f116 | `findings/FIXED/M-092-html-carve-absent-u16-keys-and-endtag-walk-decoded.md` |
| M-093 | MAJOR | carve (the XML/SVG chain; "csl"+0x1cd decoded) | ca701711 (d1dc51a was a pre-rebase phantom, repaired session 39) | `findings/FIXED/M-093-xml-svg-carve-absent-four-u32-keys-one-chain.md` |
| M-096 | MAJOR | carve (the MOBI salvage gate + end-tag walk) | 6e37dc21 (7ca83f5 was a pre-rebase phantom, repaired session 39) | `findings/FIXED/M-096-mobi-carve-absent-bookmobi-gate-endtag-walk-decoded.md` |
| M-100 | MAJOR | preview | closed-as-covered — `6d0f16c` (M-066+N-088, an ancestor of the finding's own baselines; the session-10 lane-B verification: `evidence/s10-verification-pass-b.md`) | `findings/MAJOR/M-100-archive-browse-inside-preview-absent.md` (retired by the session-10 lane-B push; never lived in FIXED/ — the capability predated the filing) |
| M-072 | MAJOR | query/abi | bf17e55 | `findings/MAJOR/M-072-query-axes-path-substring-state-mask.md` |
| M-074 | MAJOR | writer/abi | 09b311d | `findings/MAJOR/M-074-destination-validation-codes-undifferentiated.md` |
| M-077 | MAJOR | preview | af5ca4d | `findings/MAJOR/M-077-preview-caps-defaults-absent.md` |
| N-092 | MINOR | (mid-flight landing) | 5ed862f | `findings/FIXED/N-092-thumbnail-refusal-matrix-and-cache.md` |
| N-124 | MINOR | (mid-flight landing) | c75755d | `findings/FIXED/N-124-transport-module-rows-superseded-by-ddrv-wave.md` |
| N-116 | MINOR | (session-38 minor-first lane; the Resolution + RED->GREEN proof live in the retired file and worklog.md's S38 block) | 9445627f (632d21e was a pre-rebase phantom, repaired session 39) | `findings/FIXED/N-116-session-list-row-ordering-diverges-og-time-sort.md` |
| N-117 | MINOR | (session-38 doc-truth batch A; doc-only per the audit's own plan — Resolution in history at e4f3184) | e4f3184 | `findings/MINOR/N-117-w1abc-adjacent-doc-drift-plan-shape-and-cite-lines.md` |
| N-118 | MINOR | (session-38 doc-truth batch A; doc-only — Resolution in history at e4f3184) | e4f3184 | `findings/MINOR/N-118-reference-api-gap-rows-stale-after-w1-landings.md` |
| N-103 | MINOR | (session-38 doc-truth batch A; doc-only — Resolution in history at e4f3184, incl. the first-hand EURawImg/EUImgRepairFile import-table verification) | e4f3184 | `findings/MINOR/N-103-thumbnail-api-rows-stale.md` |
| N-095 | MINOR | (session-38 doc-truth batch B; doc-only — Resolution in history at 995ba5d) | 995ba5d | `findings/MINOR/N-095-doc-truth-batch-stale-texts.md` |
| N-108 | MINOR | (session-38 doc-truth batch B; doc-only — Resolution in history at 995ba5d; the rawsigns comment + SCAN-ENGINE-DELTA halves FIXED-ALREADY by parallel lanes) | 995ba5d | `findings/MINOR/N-108-product-docs-stale-constants.md` |
| N-119 | MINOR | (session-38 doc-truth batch B; doc-only — Resolution in history at 995ba5d; D-88 re-adjudicated against the EuShare decode re-read first-hand) | 995ba5d | `findings/MINOR/N-119-d88-nas-premise-stale-eushare-decode-answers.md` |
| N-104 | MINOR | (session-38 doc-truth batch C; doc-only — Resolution in history at 32ef33b) | 32ef33b | `findings/MINOR/N-104-doc-truth-defects.md` |
| N-097 | MINOR | (session-38 doc-truth batch C; doc-only — the FVE-snapshot security datum registered) | 32ef33b | `findings/MINOR/N-097-auto-txt-misclassified-fve-key-snapshot.md` |
| N-102 | MINOR | (session-38 doc-truth batch C; doc-only — the propagation note flipped) | 32ef33b | `findings/MINOR/N-102-m024-toggle-count-half-propagated.md` |
| N-123 | MINOR | (session-38 doc-truth batch C; doc-only — the 11-verb recount + exit 4) | 32ef33b | `findings/MINOR/N-123-cli-module-commands-row-stale-11-verbs.md` |
| N-101 | MINOR | (session-38 doc-truth batch C; one live residue fixed + the rest FIXED-ALREADY with reproductions — Resolution at 32ef33b) | 32ef33b | `findings/MINOR/N-101-surface-inventory-arithmetic-and-unregistered-surfaces.md` |
| N-120 | MINOR | (session-38 doc-truth batch C; live residues fixed + the rest FIXED-ALREADY with reproductions — Resolution at 32ef33b) | 32ef33b | `findings/MINOR/N-120-audit-trail-pointer-and-arithmetic-drifts.md` |
| N-099 | MINOR | (FIXED-ALREADY — classified session 38: E55 + the live WPG/ctor re-pins carry the corrections; reproduction in the Resolution at 32ef33b) | (parallel session-8/SCAN-4 lanes; E55) | `findings/MINOR/N-099-batch1-citation-truth-pass.md` |
| N-106 | MINOR | (FIXED-ALREADY — classified session 38: the SI:86 disposition row; reproduction in the Resolution at 32ef33b) | (parallel session-8 wave-2 lane) | `findings/MINOR/N-106-getdriverinfo-disposition.md` |
| N-091 | MINOR | (session-38 registration batch D; D-108/D-109/D-110 landed — Resolution in history at the batch D commit) | (this session) | `findings/MINOR/N-091-parity-deltas-registration-gaps.md` |
| N-098 | MINOR | (session-38 batch D; the contract sentence + mirrors + the `EnumerationRefreshContractPinned` pin, RED→GREEN proven) | (this session) | `findings/MINOR/N-098-enum-refresh-contract-sentence.md` |
| N-105 | MINOR | (FIXED-ALREADY — classified session 38: D-106 is the demanded deferral record; reproduction in the Resolution) | (parallel S-044 lane's rider) | `findings/MINOR/N-105-autosave-folder-suffix-naming.md` |
| N-100 | MINOR | (session-38; the +0x60 carrier landed as nflags bit-9 with the sparse-zeroing rule — Resolution + RED/GREEN proof in history at the N-100 commit) | (this session) | `findings/MINOR/N-100-ntfs-0x60-total-clusters-not-persisted.md` |
| N-136 | MINOR | session/abi | fce38c3-lineage (renumbered from the N-128 ID collision per the SA-9 earlier-remote-push rule - the capability-matrix N-128 keeps the ID) | `findings/MINOR/N-128-session-list-cwd-dir-composes-prefixed-paths.md` (renamed N-136 in history) |
| N-134 | MINOR | register (the product-frame census) | FIXED-ALREADY at `b202f9ed` (the six rows pre-dated the filing by 6.5h; retired session-11B with the sfc addenda preserved) | `findings/FIXED/N-134-product-frame-register-completion.md` |
| N-093 | MINOR | (session-38; the og's four bound pairs carried — the pin + MID behavioral pair RED→GREEN; Resolution in history at the N-093 commit) | (this session) | `findings/MINOR/N-093-batch1-bounds-mid-gz.md` |
| N-111 | MINOR | carve | (session-39 fix lane — cite-by-title: "fix(carve): N-111 - the JP2 family lands") | `findings/FIXED/N-111-jp2-carve-absent-jp-signature-ftyp-brand-map-decoded.md` |
| N-142 | MINOR | ui (the dead CrPreview origin pair; rebase-leftover hygiene) | (this session — cite-by-subject "fix(ui): N-142 - the dead CrPreview origin pair removed") | `findings/FIXED/N-142-preview-handle-dead-wiring-rebase-leftover.md` |
| N-112 | MINOR | carve | (session-39 fix lane — cite-by-title: "fix(carve): N-112 - the LZH entry-chain walk lands") | `findings/FIXED/N-112-lzh-carve-absent-list-layer-lh-method-gate-entry-walk-decoded.md` |
| N-113 | MINOR | carve | (session-39 fix lane — cite-by-title: "fix(carve): N-113 - the text school lands") | `findings/FIXED/N-113-text-school-list-probes-absent-vcf-gpx-osp-pos-chfp-decoded.md` |

## Go-forward convention (established by this purge, per the user instruction; AMENDED session-39 per the session-39 user instruction "put fixed findings in fixed directory")

Fix lanes resolving a finding: (1) record the Resolution section in the
finding file, (2) append the row to this ledger (ID / severity / fix commit /
resolved path), (3) **session-39 amendment: `git mv` the file into
`findings/FIXED/`** — the user's session-39 instruction explicitly directs
resolved findings into the fixed directory, so `findings/FIXED/` is
re-created DELIBERATELY as the live resolved-file archive (rows below the
amendment carry the `findings/FIXED/` path; the pre-amendment rows keep
their history paths, recoverable via
`git log --oneline --follow -- 'diff-to-fix/findings/FIXED/<slug>.md'`).
The active severity folders stay the only thing carrying UNRESOLVED
findings; this ledger is the index. A finding moved into a re-created
`findings/FIXED/` directory violates the convention and the next audit
lane will retire it again.

| S-041 | SEVERE | carve (the batch-2 umbrella) | school-1 `b92ebef` + M-081 `9e5dedc` + M-090 `9daa787` + M-084/M-091 `8a7055ea` (the 771e2ec/16475b8 tokens were pre-rebase phantoms, repaired session 39) | `findings/SEVERE/S-041-s007-batch2-carve-remainder.md` (FIXED-as-umbrella, SA-10 — school 1 landed as code `b92ebef`; CRW/MRW/LowDoc/HLP `9e5dedc`; DMG `9daa787` + MDB `8a7055ea` + MTS/M2TS `a3abfbe` resolved mid-flight; the still-open school-2/3 formats child-filed: M-092, M-093, M-096, M-098, M-099, N-111, N-112, N-113, N-115; the ONE/3DS walk follow-ups owned by M-106. The S-007 precedent: the umbrella closes when every remainder has its own owner. Full Resolution record in the retired file.) |
| S-051 | SEVERE | orchestration (the whole-disk target set — the partition list as the scan-target basis; the CDoScanPartition flow) | `6ecd6bb4` (session 40) | `findings/SEVERE/S-051-wholedisk-scan-targets-absent-silent-empty.md` (retired to history; the Resolution rides the fix commit — the RUN 2 silent-empty verdict reproduced verbatim at session-40 open, then flipped to files=4/deleted=2 by the re-run probe; six whole-disk tests; the discovery no-double-scan reconciliation; matrix/module/counts updated in the same trail commit) |
