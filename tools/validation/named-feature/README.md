# named-feature fixture — seeded ground truth for validation standard T5

Session: cross-repo harvest (2026-11-19). This directory establishes the
**seeded validation fixture** required by the session's Phase 3 standard:
a deterministic NTFS disk image whose every file is recorded
(path, size, timestamps, attributes, SHA-256) **before deletion**, so every
later "works" claim (starting with the named feature — reconstructed files
with directory structure preserved, deleted only) is proven by
byte-for-byte comparison against the manifest.

## Components

| File | Purpose |
|---|---|
| `generate-fixture.ps1` | Deterministic generator: builds the fixture tree on any NTFS root, sets fixed timestamps/attributes, emits `manifest.jsonl` (line 1 = `_meta`, then one JSON object per file). Content is PRNG-derived from a salted hash of each file's relative path — same tree, same bytes, every run, every machine. |
| `apply-deletions.ps1` | Applies the deletion matrix exactly (file deletes, whole-tree deletes, the double-delete recreate dance), with volume flushes before and after (the testlab lesson: flush before delete = lazy-writer guard, flush after = metadata flush). Writes `apply-log.json`. |
| `verify-reconstruction.py` | The byte-for-byte judge: compares a reconstruction output directory against the manifest — deleted-only invariant, exact-path + SHA-256 checks, collision-set checks, live-leak detection, containment. `--self-test` proves the verifier itself. |
| `.github/workflows/named-feature-fixture.yml` | Dispatch-only Windows lane: builds the fixture on a real 2 GB GPT/NTFS VHDX, proves determinism (two generations, byte-identical manifests), applies deletions, runs the verifier self-test, detaches, uploads the VHDX + manifest + logs as artifacts. |

## The fixture matrix (what it exercises)

| Case | Files | Exercises |
|---|---|---|
| Live siblings | `Docs/readme.txt`, `Photos/keep.jpg` | deleted-only invariant (live files must NOT appear in output) |
| Plain deleted file | `Docs/report.docx`, `Docs/archive-2025.zip` | basic reconstruction |
| Deep nesting | `Docs/Old/sub/deep.bin` (tree-delete), `Deep/a/b/c/d/e/nested-deep.txt` (file-delete under live dirs) | path reconstruction depth |
| Whole deleted tree | `Photos/vacation/**` (dir + children + inner dir) | deleted-parent chains (sequence-bump rule pseq+1), children recovered under their original parent path |
| Unicode + spaces | `Names/file with spaces.txt`, `Names/üñíçødé-名前- файл.txt` | name fidelity through the whole chain |
| Size classes | `Sizes/empty.bin` (0 B), `Sizes/resident.txt` (512 B, resident $DATA), `Sizes/onerecord.bin` (4 KiB), `Sizes/nonresident.bin` (2 MiB) | resident vs non-resident extraction |
| Attributes | `Attributes/hidden-note.txt` (Hidden), `Attributes/readonly-diary.txt` (ReadOnly) | attribute recording + force-delete |
| Double delete (same name) | `Collisions/draft.txt` created → deleted → recreated with different content → deleted | same-name collision handling in a deleted-only output |

## Declared outcomes (what "correct" means)

- Every `deleted=true` row with `expect.output_rel` MUST appear in a
  reconstruction at exactly that path with exactly that SHA-256 (class
  EXACT; the fixture volume is fresh, so deleted extents stay contiguous —
  no reuse churn is introduced on purpose).
- NO `deleted=false` row may appear (path OR content hash).
- The **double-delete** row pair is allocator-dependent (the second
  `draft.txt` may or may not reuse the first one's MFT record): the
  manifest declares `expect.mode = "collision-set"` — the verifier accepts
  1 or 2 outputs under `Collisions/` whose stem starts with `draft`, every
  output's SHA-256 must be one of the pair's hashes, distinct outputs must
  carry distinct hashes. The STRICT ordering/suffix contract
  (` (2)`, ` (3)`, ... appended before the extension, ordered by MFT record
  number) is fixed for the engine implementation and pinned by synthetic
  unit tests in Wave 5 — natural-deletion fixtures cannot deterministically
  control record assignment.
- All output path components must be valid Windows names (no trailing
  dots/spaces, no reserved device names, no `<>:"/\|?*` or control chars)
  and every resolved path must stay inside the output root
  (junction/symlink containment).

## Operational lessons baked in (cited)

- **Double flush** around deletion (HALIM-FEATURES §2.19, Run-TestLab.ps1
  lesson) — `Write-VolumeCache` before (lazy-writer guard) and after
  (metadata flush).
- **No remount between delete and scan** (same source: remount writes
  System Volume Information / indexer files that reuse freshly freed
  directory clusters and destroy deleted entries on FAT-family volumes;
  on NTFS it risks cluster churn) — the lane detaches the VHDX only after
  the flush and the image is scanned by later lanes without an
  intermediate mount-and-write.
- Deterministic content via salted path-hash PRNG seeds (same discipline
  as `engine/bench/seed/generate_seed.ps1`, SEED_VERSION 2.0.0).

## Usage (Wave 5 validation lane will do exactly this)

```powershell
# on a Windows host with an attached NTFS volume X: (or the VHDX from the lane artifact)
powershell -File tools/validation/named-feature/generate-fixture.ps1 -Root X:\fixture -ManifestOut D:\work\manifest.jsonl
powershell -File tools/validation/named-feature/apply-deletions.ps1 -Root X:\fixture -ManifestOut D:\work\manifest.jsonl -LogOut D:\work\apply-log.json
# ... scan X: with the product, reconstruct deleted-only to D:\out ...
python tools/validation/named-feature/verify-reconstruction.py --manifest D:\work\manifest.jsonl --out D:\out --report D:\work\verify-report.json
```

Licensed under the MIT license. See LICENSE.
