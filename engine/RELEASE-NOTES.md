# core_recovery v0.4.0-parity

**Scope of this release:** the complete local recovery core with the
parity audit **closed** — every filed finding in `diff-to-fix/` is
dispositioned (FIXED with tests / FIXED-ALREADY / INVALID /
SUPERSEDED; the board is empty, tracked-open 1: OQ-S4-3's live-harness
perf measurement). The parity loop — audit (`easeus-drw-research`
corpus as ground truth) → fix → prove → re-audit — is closed at this
tag, and this release additionally carries the post-fix **review
session**: every fix-lane landing and collision was independently
re-verified, invalid removals reverted, hallucinated claims corrected,
and the audit trail reconciled (`diff-to-fix/SUMMARY.md` is the board;
`docs/PARITY-DELTAS.md` the 97-row deliberate-divergence register).

## New since v0.3.0-parity (headline capabilities)

- **Damage-audit check phase** (M-041): the og's FileSystemCheck as a
  read-only pre-scan stage — FAT (DBR ceilings, hole/duplicate scans,
  pairwise copy compare, dir-tree audit incl. the FAT cross-link 0x1002
  gate, orphan sweep) + NTFS ($MFT/$MFTMirr proofs, version gate,
  $Bitmap structural, MFT×$Bitmap cross-check), taxonomy 0x1001..0x100e
  in numeric parity, never aborts the scan; `scan --check` CLI +
  `pre_scan_check` ABI + REC_FS_CHECK session record.
- **Recycle-bin $I/$R pairing on every engine** (M-052 + N-079): the
  shared model across NTFS/FAT/exFAT — original name/size/time
  restoration, the og's four bin-root set, bin-scope guards (out-of-bin
  `$…` files stay raw).
- **ZFS uberblock-ring history** (S-034): every txg state walked
  newest-first with cross-state identity dedup — files deleted inside
  the ring window are visible, matching the og.
- **Boot-chain repair** (M-040 stages a+b): BCD partition-offset surgery
  (`cr/bcdfix`) + the UEFI `Boot####` NVRAM updater (`cr/nvramfix`), both
  plan/execute/undo with default-deny consent; boot.ini/chkdsk
  registered dispositions (D-90).
- **User-extensible signature database** (M-048): `--user-signs` — the
  og's EUUserData JSON shape, ABI registration surface, built-in-wins
  collision rule.
- **Carve efficiency** (M-025 + M-039): the window-aware read view
  (measured 9.9× fewer device reads) and the cross-engine occupancy gate
  (structured extents suppress carve re-reads).
- **Driver-utilization parity** (M-030): the WinPE environment gate, the
  terminal volume-OFFLINE quiesce step with mount-point restore, the
  200-disk enumeration bound, and transport-error surfacing on fallback.
- Plus: Btrfs chunk profiles/default-subvolume + re-scan cache (M-020,
  N-072), ext4 backup-superblock rescue (S-027), the grading engine
  (S-023), health grading (S-024), disk-image targets (S-030), UDF
  recognition (M-044), archive inner-entry inventories (S-032), the
  BitLocker password-dictionary aid (N-064, consent-gated), overlap
  scoring + naming parity (N-070/N-063/N-065), and the review-session
  corrections (the M-041 FAT cross-link gate, the N-064/D-80 dead-module
  enforcement, the `--check` CLI half the canonical landing had lost).

## What is in the bundle

```
bin/    rrecov.exe            the CLI (list / list-carve-formats / scan /
                              tree / recover / health / fix / apply / bench)
        cr_ui.dll             the UI C-ABI shim (78 exports; bind in-process)
        z.dll, lz4.dll, zstd.dll   vcpkg-built runtime (shipped binaries)
        vcruntime140*.dll, msvcp140*.dll, ucrtbase.dll + api-ms-win-crt-*  app-local CRT
lib/    cr_ui.lib             import library (link against cr_ui.dll)
        cr_core.lib, cr_win.lib    static core + platform layers
include/                      the C/C++ surface (cr/cr_ui.h + full internal tree)
docs/                         the truth documents (see below)
SHA256SUMS                    every file in the bundle
```

Every shipped PE carries the **Windows 7 SP1 loader contract** (OS/subsystem
version 6.1, stamped post-link) and passed the **import allowlist gate**
(`tools/verify_pe_um.py`; Windows-8-only APIs fail the release by design).

## Documentation (new in this release)

- `docs/API.md` — the API reference (CLI grammar + exit codes, the full
  cr_ui export inventory by lifecycle group, the C++ core layers, the
  events/threading contract, the error model, stability classes).
- `docs/CONNECTION-GUIDE.md` — building, binding, the device connections
  (RDA grammar / direct handles / images / BitLocker), threading rules,
  and the consent model.
- `docs/FORMATS.md` — every format read/written: engines, carve layers,
  the .crsf record inventory, report schema, repair artifacts.
- `docs/PARAMETERS.md` — every tunable (CLI flags, ScanConfig fields,
  engine constants, caps, budgets) with defaults, bounds and governing
  D-row citations.
- Updated: `README.md` (current scope/status/docs index),
  `docs/CAPABILITY-MATRIX.md` (per-row truth),
  `docs/PARITY-DELTAS.md` (97 rows, zero collisions, errata notes),
  `docs/ABI-CONTRACT.md`, `RELEASE-NOTES.md` (this file).

## Validation at this tag

- **924 tests** green on Linux (885 pass + 4 environment-skipped in the
  core suite + 35 cr_ui ABI tests), plain + **ASan** + **UBSan** +
  **TSan** lanes (zero warnings — the event-sink threading contract is
  pinned by test), MSVC `/W4 /WX` + the C consumer + DLL smoke on the
  Windows lane, clang-format-18 gate clean, the win-layer parse gate,
  libFuzzer smoke campaign (15 targets; the S-030 image harness build
  break that reddened the fuzz lane since it landed is fixed at this tag).
  CodeQL deferred (if: false - recorded, not silent).
- **Win7 import gate green on every shipped PE.**
- **The parity audit trail**: 223 dispositioned findings in
  `diff-to-fix/findings/FIXED/` (each with a Resolution citing its
  tests), the cross-reference matrix at `MATCH`, and the review
  session's independent verification of the fix landings recorded in
  `worklog.md`.
- Driver-side benchmarks: the API-benchmark lane (alignbench) carries
  the measured MATCH/EXCEEDS verdicts; see
  `docs/BENCHMARK-REPORT.md`.

## Truth documents

- `diff-to-fix/SUMMARY.md` — **the board** (every finding's disposition
  and the session history; the authoritative close-out record).
- `docs/PARITY-DELTAS.md` — deliberate divergences with rationale
  (D-01…D-102, reconciled this release).
- `docs/CAPABILITY-MATRIX.md` — per-capability DONE/PARTIAL/MISSING.
- `docs/CORE-GAP-LIST.md` — the closure register.
- `docs/ground-truth-contracts.md` — the anti-hallucination anchor.
- `docs/error-fix-registry.md` — reference defects catalogued and fixed.

## Known honest gaps (registered, not hidden)

NTFS XPRESS/LZX compressed $DATA (GAP-P1-1 — no corpus surface exists;
the og is LZNT1-only too), ReFS container-map translation (N-036), ZFS
multi-vdev topologies (single-vdev only), HFS+ attributes items the
corpus itself leaves UNVERIFIED, and OQ-S4-3 (the 4-vs-7-MiB chunking
relative-perf measurement — needs exactly one live Windows harness
run). Nothing silently degrades: unimplemented surfaces return explicit
errors.
