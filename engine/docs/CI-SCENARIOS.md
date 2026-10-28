> **Superseded matrix (2026-10-28):** the staged-scenarios S-* truth
> matrix now lives in `.github/scenarios/benchmark-matrix.json` and is
> executed by the test-benchmark lane (`.github/workflows/
> test-benchmark.yml` + `.github/scripts/Run-Benchmark.ps1`) through BOTH
> the CLI and the cr_ui API, on simulated fixed-VHDX disks (MBR+GPT,
> FAT32/NTFS/exFAT, dual-partition). This document's scenario philosophy
> (ground truth = seed manifest + deletion semantics, never tool output;
> extra finds recorded, never failures) carries over verbatim; the seed
> manifest itself is now `bench/seed/generate_seed.ps1`'s deterministic
> 23-format corpus instead of the 11-file stage-disk manifest.

# CI-SCENARIOS — the staged-scenario truth matrix

> **Superseded matrix (2026-10-28):** the staged-scenarios S-* truth
> matrix now lives in `.github/scenarios/benchmark-matrix.json` and is
> executed by the test-benchmark lane (`.github/workflows/
> test-benchmark.yml` + `.github/scripts/Run-Benchmark.ps1`) through BOTH
> the CLI and the cr_ui API, on simulated fixed-VHDX disks (MBR+GPT,
> FAT32/NTFS/exFAT, dual-partition). This document's scenario philosophy
> (ground truth = seed manifest + deletion semantics, never tool output;
> extra finds recorded, never failures) carries over verbatim; the seed
> manifest itself is now `bench/seed/generate_seed.ps1`'s deterministic
> 23-format corpus instead of the 11-file stage-disk manifest.
>
> **Lane truth (M-088):** three of the consumer workflows named in the
> historical table below were DELETED by the consolidation
> (`driver-install.yml`, `core-install.yml`, `staged-scenarios.yml`) — the
> current consumers are `checks.yml` (the refusal contract's driverless
> probes), `test-benchmark.yml` (the install chain + the scenario
> matrix), and `installer.yml` (the S-038 gates). The table is preserved
> as the historical contract record; read every row as pre-consolidation
> history, not as a live lane.

> What each prebuilt-consumption workflow proves (PLAN-CI-REBUILD §3.4). The
> matrix is versioned with the scenario matrix
> (`.github/scenarios/benchmark-matrix.json`, seeded by
> `bench/seed/generate_seed.ps1`); every scenario runs on
> windows-2022 with the REAL prebuilt driver + REAL prebuilt core (no mock,
> no from-scratch build — docs/PREBUILT-ARTIFACTS.md) and uploads an evidence
> artifact that IS the acceptance proof.

Ground truth derivation: the seed manifest defines name/size/category/attrs
and the `deleted` flag; the staging script generates deterministic content
(per-name FNV-seeded LCG — identical bytes every run), records per-file
SHA256 at stage time (`staged-truth.json`), and executes the scripted
deletion (`Remove-Item -Force` — the recorded recycle-bypassing method).
The **expected-recoverable set** = deleted files with size > 0 (the FS
metadata entry — MFT record / dirent — survives the deletion; zero-byte
files are the documented exclusion: no data extent to prove recovery by
hash). The expected set is derived from the manifest + deletion semantics,
never from either tool's output.

| ID | Filesystem | Staging | Deletion | What it proves |
|---|---|---|---|---|
| S-DEL-NTFS | NTFS | 256 MB fixed VHD, MBR, full format | Remove-Item -Force on the deleted subset | deleted-file recovery truth on the primary FS: every expected file appears in the scan's tree AND recovers with bytes identical to the staged seed (SHA256) |
| S-DEL-FAT32 | FAT32 | same | same | the FAT family's deleted-dirent + chain recovery |
| S-DEL-EXFAT | exFAT | same | same | exFAT deleted-dentry recovery |
| S-FMT-NTFS | NTFS | seeded, then quick-format | the format itself | post-quick-format recovery (the og's formatted-drive scenario): the FS metadata is rebuilt by the format, so recovery routes through the deep/carve layers |
| S-DRV-REFUSE | n/a | driver absent → installed → stopped | n/a | the refusal contract (pre-consolidation: executed in driver-install.yml; now checks.yml's driverless ABSENT probes + test-benchmark's READY facet assertions): ABSENT (exit 4, `0x500B0001`, no scan output) → READY (exit 0, every facet green, scan proceeds past the gate) → NOT_STARTED (exit 4, `0x500B0002`) |

## The evidence artifacts (the acceptance proof per scenario)

- `staged-truth.json` — the ground truth: disk number, filesystem,
  deleted list, expected-recoverable set, per-file SHA256.
- `scan-log.txt` / `tree.json` — the scan's own output and observed set.
- `expected-vs-observed.json` — per-file expected/found/size comparison.
- `recover-log.txt` / `recovered/` — the recovery run to a fresh destination.
- `hash-verification.json` — per-file recovered-vs-seed SHA256 verdicts.
- `env-manifest.json` — runner image, MSVC version, resolved driver+core
  tags, the convention document reference.

Verdict: PASS requires every expected file found AND hash-verified. Extra
finds are informational, never failures (carve may emit more than the
expected set — superset is honest).

## Scope honesty

The matrix covers the engines the core ships for the staged filesystems and
the deletion mode the manifest records. Ext4/HFS+/Btrfs/ZFS/ReFS staged
scenarios are NOT in v1: the runners format those filesystems only via
third-party tooling, and staging them truthfully (mkfs-class formatting with
deterministic seeds) is follow-up work — recorded here as the matrix's
documented boundary, not a silent gap. The S-IMG-CREATE scenario (image
verb) is blocked on PLAN-API-EXPANSION Wave 4 and enters the matrix when
the verb lands.
