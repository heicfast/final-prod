# T5 evidence — seeded named-feature fixture, captured on real Windows

**Canonical green run**: [44356754104](https://github.com/heicfast/final-prod/actions/runs/44356754104) (commit `6544953e`, 2026-11-18, windows-latest, conclusion **success**).

## What the run proved (each claim = artifact in this directory)

| Claim | Evidence |
|---|---|
| The generator builds the full 24-row fixture matrix on a real 2 GB GPT/NTFS VHDX (diskpart-created, letter X:) | `manifest.jsonl` (line 1 `_meta`: fixture named-feature-v1 v1.0.0, salt 0x5EEDF1AF; 24 rows: 2 live, 22 deleted incl. 4 tree-delete dirs, 20 EXACT + 2 COLLISION-SET; unicode row `Names/üñíçødé-名前- файл.txt` intact with SHA-256) |
| **Determinism**: two generations on the same volume produce byte-identical manifests (modulo the `_meta` line, which records root + wall time by design) | lane log (the workflow's `Compare-Object` gate — the run is green, so the comparison passed); the generator's content is PRNG-seeded from a salted fold of each file's relative path |
| The deletion matrix applied exactly, with both flushes | `apply-log.json`: flush_before `flushed (letter X)`; 19 actions all `ok` (13 file deletes, 1 recreate-from-stash with SHA verification, 4 tree deletes, 1 stash removal); flush_after `write-through probe ok; volume-cache best-effort (letter X)` |
| The verifier's own logic is proven | `verify-selftest.txt`: 5/5 (good/missing/live-leak/bad-hash/extra) |
| The seeded-damaged disk image exists | CI artifact `named-feature-fixture` of run 44356754104 (VHDX, 100 MiB on disk): **SHA-256 `fb9f4835889207da202d71af264bebcb09ea42b5bbdf81b4d714fe82d3bd`** — too large to commit; regenerable deterministically by re-dispatching the lane; 90-day artifact retention |

The `lane-transcript.txt` is the full PowerShell transcript of the run.

## Run history (failures root-caused, never masked)

| Run | Commit | Result | Root cause / action |
|---|---|---|---|
| [44354166977](https://github.com/heicfast/final-prod/actions/runs/44354166977) | aabc1ffa | failure | PowerShell unrolls an EMPTY `byte[0]` function return into zero pipeline output → `$bytes = $null` on `Sizes/empty.bin` → `WriteAllBytes` ArgumentNullException. Fix: comma operator on the return (commit 00abe98a). |
| [44354788432](https://github.com/heicfast/final-prod/actions/runs/44354788432) | 00abe98a | failure | `Write-VolumeCache` second invocation fed `DriveLetter ':'` while the identical first call succeeded. Fix: regex-based letter derivation + piped-volume form + per-action logging (commit a80899b7). |
| [44355793332](https://github.com/heicfast/final-prod/actions/runs/44355793332) | a80899b7 | failure | Same cmdlet, same second-invocation failure regardless of call form → after-flush made non-fatal by construction (WriteThrough probe + best-effort cmdlet), evidence-first logging (commit 43516f8a). |
| [44356352147](https://github.com/heicfast/final-prod/actions/runs/44356352147) | 43516f8a | **success** | All 19 actions ok, self-test 5/5, VHDX produced; apply-log truthfully recorded `flush-failed` (File.Open 4-arg overload takes FileShare, not FileOptions). Probe fixed (commit 6544953e). |
| [44356754104](https://github.com/heicfast/final-prod/actions/runs/44356754104) | 6544953e | **success** | The canonical evidence run captured here — flush_after clean. |
| [44370686000](https://github.com/heicfast/final-prod/actions/runs/44370686000) | 5e57d8b2 | fixture ok / engine-flow red | The seeded VHDX ground truth built clean; the engine-flow job (scan + reconstruct + verify) failed — see the next rows. |
| [44391734922](https://github.com/heicfast/final-prod/actions/runs/44391734922) | bed4d22c | fixture ok / engine-flow red | **The F-01 closed-loop's first full red, root-caused from artifacts to THREE defects**: (1) ENGINE — the NTFS volume root `.` directory was sanitized into a literal `_` path component, nesting every reconstructed path under `_\` (F-01b: `.`/`..` are reference names, transparent in the plan walk); (2) FIXTURE — the v2 recreate's cached `File.Copy` lost its data on the unflushable volume cache (the VHDX reads zeros at v2's extent; the stash twin survived) → the recreate is now WriteThrough; the probe's post-deletion create had eaten `Docs/report.docx`'s freed MFT record → the probe record is now allocated BEFORE any deletion; the probe and the stash are now declared manifest rows (I4 accountability exact); (3) FIXTURE — the fixture seeded at `X:\fixture` so volume-relative paths carried a `fixture/` component the verifier had to strip → the fixture now seeds at the volume root `X:\` (manifest `rel` == volume-relative path). Engine label defect also found+fixed: `(VHDX, MBR)` on a GPT disk — `classify_lba0` needs LBA1 (`EFI PART` at +0x200) but the caller read only 512 bytes; the og's 0x1000-byte read restored. |

## Fixture design v1.1 (post-44391734922)

The generator's matrix now carries 26 rows (24 + the declared `.nffx-flush-probe` EXACT row + the declared `.nffx/draft-v2.txt` stash row), the apply step allocates the probe's MFT record before any deletion, recreates v2 through a WriteThrough stream, and the lane seeds at the volume root. The verifier is unchanged (I1–I5); the manifest now accounts every recoverable byte the engine can honestly find.

## How Wave 5 consumes this

1. Re-dispatch the lane (or reuse the artifact VHDX — verify its SHA-256 first).
2. Scan the VHDX with the product, run the named feature (deleted-only, structure-preserving reconstruction) into an output dir.
3. `python tools/validation/named-feature/verify-reconstruction.py --manifest worklog/VALIDATION/fixtures/named-feature/manifest.jsonl --out <output> --report <report>` — the 5 invariants (deleted-only, exactness, collision-set, accountability, containment) are the byte-for-byte gate; evidence lands in `worklog/VALIDATION/`.
