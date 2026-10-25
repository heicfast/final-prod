# MAJOR M-036 — NVMe health: 4 spec fields dropped, data units unconverted (og multiplies by 512,000)

**Severity:** MAJOR
**Module:** health
**Parameters affected:** B11 (node fields), B13 (report schema), Q68 (doc accuracy)
**Corpus references:** DiskHealth_decompiled.c :5600-5615 (data-units → bytes conversion `x0x7d000` = 512,000); :13351-13375 (the NVMe route — byte-parity MATCH with ours, see I-018); health-log field surface per NVMe 1.x (host reads/writes, controller busy time, error log count present in the og's row set).
**Core references:** `src/core/health/cr_nvme.cpp:23-39` + `include/cr/health/cr_nvme.h:40-53` (host_reads/host_writes, controller_busy_time, num_error_log_entries absent; data_units reported raw). Audited commit `7fd00e0`.
**Status:** READ-ONLY FINDING — no fix applied
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `M-020` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.


## Summary

The og's NVMe health surface includes host-reads/writes counts, controller-busy-time, and error-log-entry counts, and converts data-unit counts to bytes (×512,000). Our parser drops those fields and reports data units unconverted. Reports diverge on every NVMe drive.

## What the reference does

The cited conversion and the row set are decoded; the route (IOCTL 0x2D1400, protocol {3,2,page 2}, 512B) is identical to ours — only the parsed surface differs.

## What our core does

Parses critical_warning, temperature, spare thresholds, percentage_used (u16 — N-009a), data_units read/written (raw counts), power cycles/hours, unsafe shutdowns, media errors. No host r/w, no busy time, no error-log count; no unit conversion.

## Gap

Four fields + one unit conversion. Falsifiable: byte-compare a health report on the same NVMe drive — og shows bytes and the extra fields; ours shows raw units without them.

## Impact

Report parity divergence on NVMe; derived stats (data written) are misleading in raw units for users.

## Reproduction / verification

Corpus-derived (cites first-hand). Core: struct read. UNVERIFIED-BY-EXECUTION (needs an NVMe host).

## Fix plan

1. Extend `NvmeHealthInfo` with host_reads, host_writes, controller_busy_time, num_error_log_entries (NVMe 1.x health-log layout — offsets 0x78..0x90 region).
2. Add a converted `data_bytes_read/written` pair (units × 512,000) alongside the raw counts (keep both; the raw count is the on-disk truth).
3. Fix percentage_used width (u16 vs byte) in the same pass — N-009a is the tracked MINOR; land it here.

## Tests to add

- `NvmeHealth.FullFieldSurface` — synthetic 512B health page; assert all fields parsed incl. the four new ones.
- `NvmeHealth.DataUnitsConverted` — assert bytes = units × 512,000.
- `NvmeHealth.PercentageUsedByteWidth` — pins the N-009a fix.

## Related findings

S-024 (grading layer — same module), N-009a (percentage_used width, folded here), I-018 (the route MATCH record).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this commit — see `git log` for "fix(M-036)")
**Verified by:** `NvmeHealth.FullFieldSurface` (the four dropped fields at their spec offsets — every fixture value distinct so a wrong offset cannot pass), `NvmeHealth.DataUnitsConvertedToBytes` (1111/2222 units and a 1e9-unit large count; raw counts preserved alongside), `NvmeHealth.PercentageUsedIsU8Width` (the folded N-009a pin, already green), `NvmeHealth.ShortBufferRejected` (the gate moved 176 → 192 with the surfaced last field). Fail-before: `FullFieldSurface` does not compile against the pre-fix header (host_reads et al. absent from NvmeHealthInfo — capability-absence), and `DataUnitsConvertedToBytes` fails on the pre-fix build (the fields read 0). Suite 485/485 Linux Release.
**Resolution notes:** Corpus re-verified first-hand this session: the ×512,000 conversion is exactly as cited (DiskHealth_decompiled.c :5589-5614 — the 5-byte LE chain multiplied by 0x7d000 with a full 64-bit __allmul, twice: data units read and written). The NVMe route itself was already a byte-parity MATCH (I-018). Fix: (1) NvmeHealthInfo gains host_reads (+64), host_writes (+80), controller_busy_time (+96) and num_error_log_entries (+176) — the NVMe 1.x health-log layout, all via the existing le128 (the og's 5-byte read is the same value for realistic counts; ours reads the full 128-bit field); the length gate moves to 192 bytes (the page's last field ends there). (2) The converted pair data_bytes_read/written = units × 512,000 lands in the pure parser (both raw counts kept — the raw count is the on-disk truth, the converted pair is what reports show, exactly like the og). (3) N-009a (percentage_used u8) was already landed in a prior session — its pin test re-verified here. (4) The report surface: the cr_ui C-ABI report appends the six new fields (host r/w, busy, error-log, converted bytes) at the struct tail with CR_UI_ABI_VERSION 1 → 2 per ABI-CONTRACT §9 (appended fields bump); the UI fill + the CLI health output now print bytes like the og plus the four surfaced fields. ABI-CONTRACT.md + ground-truth-contracts.md updated in the same change; the C consumer test compiles against the bumped macro unchanged.
