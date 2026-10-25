# Executed evidence — session 9: M-053 baseline-regression proof (before/after)

**Session:** diff-fix session 9 (this leg). Baseline pulled at `c231f2f`
(audit-trail lineage). Corpus `03ad46c` (unmoved, matches every prior
session's pin).

## Environment

- Linux container, g++ 13.x, CMake 4.4.4 (pip --user install; `/home/z/.local/bin`).
- Configure: `cmake --preset linux-gcc` (RelWithDebInfo + ASan, the CI-parity lane).
- Runner scope: **full ctest set** (`ctest` in `build/linux-gcc`) — 518 entries:
  517 gtest cases inside `cr_tests` + `cr_ui_abi_check` (the plain-C ABI
  consumer proof).

## Fail-before chain (executed)

1. **HEAD `c231f2f`, pre-fix:** `ctest` → 517/518 pass; `cr_ui_abi_check`
   SEGVs: `AddressSanitizer: SEGV on unknown address 0x8 ... #0 main
   tests/ui_abi_check.c:576` (READ; `rows == NULL` → `rows[0].name` at
   struct offset 8).
2. **Root cause via standalone repro** (session scratch,
   `scripts/repro_name_filter.c` outside the repo): the same fixture +
   lifecycle flow with flushed prints shows `scan_run FAILED: scan bad
   magic` — `fat::scan_fat` → `FatTable::load` → `fat_validate_(p)` gate at
   `src/core/fat/cr_chain.cpp:164` → `fat_table_validate` Flavor::Fat16
   media-word branch (`cr_chain.cpp:194-199`, `u32@FAT[0] == 0xFFFFFFF8`
   exact) rejects the fixture whose FAT[0]/FAT[1] were never written.
   The og-parity validator is CORRECT (M-015, corpus CFAT16
   `FUN_1800041b00` :54052-141); the fixture is not a valid FAT16 volume.
3. **Regression point proven by worktree builds:**
   - `63d9067` (M-015's parent): `ctest -R cr_ui_abi_check` → **100% passed** (0.23 s)
   - `6dc1e32` (M-015 landing): `ctest -R cr_ui_abi_check` → **0% passed** (SEGV)
   - The landing updated `test_fat_volume.cpp:138-141` and
     `test_orchestrator.cpp:75-77` with the canonical media pair but missed
     `tests/ui_abi_check.c:build_fat16()` — the ported twin fixture.

## Pass-after (executed)

1. Fixture fix: `build_fat16()` writes `FAT[0]=0xFFF8, FAT[1]=0xFFFF` into
   both copies (4 `put16` lines, matching the C++ builders).
2. Standalone repro with the same fix: `files_found=2 deleted_found=1`;
   match-all query `total=2`; deleted_only `total=1`;
   name_substring="doc" `total=1, rows[0].name="DOC.TXT", strcmp==0` —
   exactly the assertions at `ui_abi_check.c:574-576`.
3. **Full ctest: 518/518 pass (28.7 s)** — including the torn-FAT1 repair
   block (S-008), which now runs against a valid volume: the torn sector
   still fails the whole-table validator, the clean mirror still promotes
   (M-015 copy-retry path), the rawfixer still restores F8 FF at FAT1 head
   (asserted in-test).

## Notes

- No engine code changed; this is a pure fixture repair + audit-trail filing.
- Every "suite green" claim in this session's worklog names its runner scope.
