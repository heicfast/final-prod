# core_recovery v0.3.0-parity

**Scope of this release:** the complete local recovery core, parity-audited
against the reverse-engineering corpus (`easeus-drw-research`) — all
filesystem engines (NTFS, FAT12/16/32, exFAT, ext2/3/4, HFS+/HFSX, Btrfs,
ZFS, ReFS), full LDM dynamic-disk reconstruction, grammar-aware carving
(30-format registry), lost-partition recovery, RawFixer-class transactional
repair, durable sessions (.crsf), the safe recovery writer, and the
in-process UI binding surface (`cr_ui.dll` — the og's C-export factory
wire, corpus DRWUI-exe §9.2). Out of scope by decision: remote/network
recovery only (NAS redirector, cloud).

## What is in the bundle

```
bin/    rrecov.exe            the CLI (scan / tree / recover / health / fix / bench)
        cr_ui.dll             the UI C-ABI shim (57 exports; bind in-process)
        cr_ui.pdb, rrecov.pdb symbols for crash triage
        z.dll, lz4.dll, zstd.dll   vcpkg-built runtime (shipped binaries)
        vcruntime140*.dll, msvcp140.dll, ucrtbase.dll + api-ms-win-crt-*  app-local CRT
lib/    cr_ui.lib             import library (link against cr_ui.dll)
        cr_core.lib, cr_win.lib    static core + platform layers
include/                      the C/C++ surface (cr/cr_ui.h + full internal tree)
docs/                         the truth documents (see below)
SHA256SUMS                    every file in the bundle
```

Every shipped PE carries the **Windows 7 SP1 loader contract** (OS/subsystem
version 6.1, stamped post-link) and passed the **import allowlist gate**
(`tools/verify_pe_um.py` — kernel32/advapi32 functions must be Win7-safe;
Windows-8-only APIs like `WaitOnAddress` fail the release by design).

## Parity highlights (evidence in docs/)

- **Recycle-bin remap**: 26/26 name recall on the real-volume benchmark
  (was 0/26 before the `$I`/`$R` token-convention fix). — BENCHMARK-REPORT §1
- **NTFS compressed $DATA** now decompresses (LZNT1, MS-XCA vector-proven);
  compressed streams no longer surface as raw compressed bytes. — GAP-P1-1
- **GPT two-step salvage** (primary-entries + backup-header) matches the
  decompiled reference chain; primary-damaged volumes recover.
- **ReFS**: dual-checkpoint max-clock selection, versioned layouts (0x94/0xA8),
  next-chain threading — engine tests + regressions.
- **Writer honesty**: `salvaged` outcome for zero-filled items, per-item error
  collection, destination-overlap refusal.
- **EXCEEDS the og** in: deleted-file risk grading (four-tier $Bitmap
  analysis), session integrity (per-record CRC32, skip-forward),
  cancellation granularity, report schema. — PARITY-DELTAS D-14/D-01/D-38/D-41

## UI binding surface (new in v0.3)

`cr_ui.h` is a C11 header: opaque handles, `cr_ui_*` exports returning
`cr::Err`-space u32s, ABI-versioned structs, zero-init-valid configs,
thread-local last-error. Scan lifecycle (pause/cancel/resume/telemetry,
auto-checkpoints), mid-scan immutable snapshots (G-02), query/filter/sort/
paging (G-12), preview, recovery (subtree + rename), report, sessions.
A plain-C consumer (`tests/ui_abi_check.c`) proves the full lifecycle on
every CI lane. `rrecov tree --json` emits the frozen G-12 projection.

## Validation at this tag

- 229/229 tests on Linux, plain + **ASan** (leak-detect) + **TSan** (the scan fans
  engine work over the thread pool; the UI progress bridge is single-flight
  and monotonic under TSan); **230/230 on MSVC** `/W4 /WX` (the 230th is the
  Windows-only WinDestIo contract suite — the drive-letter mkdir defect it
  caught is fixed in this tag).
- **Win7 import gate green on every shipped PE** (rrecov.exe 145, cr_tests.exe
  250, cr_ui.dll 125 imports — all XP-/Vista-era or older; the Windows-8-only
  `WaitOnAddress` family fails the gate by design and is self-tested to keep
  biting).
- libFuzzer smoke campaign green (preview short-window OOB class found by
  CI fuzzing was fixed with regression tests before this tag).
- CodeQL clean.
- Driver-side bench (rda-driver `2b1db5f`): **throughput parity achieved,
  +52% on hostile dispatch, 64/64 byte-identical reads** against the
  reference transport contract.

## Truth documents

- `docs/CORE-GAP-LIST.md` — the closure register (every row cites its fix).
- `docs/PARITY-DELTAS.md` — deliberate divergences with rationale (D-01…D-45).
- `docs/BENCHMARK-REPORT.md` — measured MATCH/EXCEEDS verdicts + the
  blocked-on register for the benchmark-machine runs.
- `docs/ground-truth-contracts.md` — the anti-hallucination anchor
  (§11 = the UI binding contracts and the frozen tree --csv/--json schemas).
- `docs/CAPABILITY-MATRIX.md` — per-capability DONE/PARTIAL/MISSING with
  honest remaining gaps (HFS+ journal replay, Btrfs tree-log recovery,
  ZFS multi-vdev reports, NTFS security-attribute surfacing).

## Known honest gaps (registered, not hidden)

See CAPABILITY-MATRIX. Nothing silently degrades: unimplemented surfaces
return explicit errors; the benchmark two-leg og comparison
(`comparison-ours.yml` / `comparison-easeus.yml`) requires the benchmark
machine and is wired, scoring-valid (GAP-BENCH-1..5 closed), and blocked
only on runner availability.
