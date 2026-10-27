# CI-SCENARIOS — the staged-scenario truth matrix

> What each prebuilt-consumption workflow proves (PLAN-CI-REBUILD §3.4). The
> matrix is versioned with the seed manifest
> (`.github/scenarios/seed-manifest.json`); every scenario runs on
> windows-2022 with the REAL prebuilt driver + REAL prebuilt core (no mock,
> no from-scratch build — docs/PREBUILT-ARTIFACTS.md) and uploads an evidence
> artifact that IS the acceptance proof.

## The workflows

| Workflow | Consumes | Proves |
|---|---|---|
| `driver-install.yml` | latest rda-driver release + latest core release | the driver install chain works from the prebuilt release (checksums → test-sign → service → RUNNING) AND the refusal contract (below) |
| `core-install.yml` | latest core release | the core release is fetchable/verifiable/stageable by convention; the (b)-class non-disk surface works; the ABSENT refusal is the standing regression probe |
| `staged-scenarios.yml` | both, tags pinned by the setup job's recorded manifest | the truth matrix (below) |

## The scenario matrix (staged-scenarios.yml)

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
| S-DRV-REFUSE | n/a | driver absent → installed → stopped | n/a | the refusal contract, executed in driver-install.yml: ABSENT (exit 4, `0x500B0001`, no scan output) → READY (exit 0, every facet green, scan proceeds past the gate) → NOT_STARTED (exit 4, `0x500B0002`) |

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
