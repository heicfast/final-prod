# INFORMATIONAL I-018 — NVMe health transport route verified byte-parity MATCH (positive record)

**Severity:** INFORMATIONAL (positive verification record)
**Module:** health
**Parameters affected:** C22 (IOCTL codes), B1 (transport correctness)
**Corpus references:** DiskHealth_decompiled.c :13351-13375 — IOCTL_STORAGE_QUERY_PROPERTY 0x2D1400 with STORAGE_PROTOCOL_SPECIFIC_DATA {ProtocolType 3 (Nvme), DataType 2 (LogPage), RequestValue 2 (health page)}, offset 0x28, length 0x1000 → 512B out.
**Core references:** `src/win/cr_health.cpp:252-278` — identical route (protocol/data-type/page/length verified line-by-line this session). Audited commit `7fd00e0`.
**Status:** READ-ONLY RECORD
> **Integration note (2026-10-26):** filed by the session-3 *storage/census*
> parallel leg against baseline `7fd00e0`; renumbered from its local `I-013` during the multi-leg reconciliation (IDs `S-016..S-022`,
> `M-024..M-030`, `N-033..N-044`, `I-013..I-016` were taken by the legs that
> landed on main first). Cross-references to parallel-leg findings were
> rewritten in the same pass.
 — VERIFIED MATCH

## Summary

The NVMe health-log transport is byte-parity identical on both sides. This upgrades AUDIT-PIPELINE's "corpus silent" NVMe row to a verified MATCH and closes the F-A4-17 follow-up question. The field-surface difference above the transport is M-036.

## Action

Update AUDIT-PIPELINE §5's NVMe row with this cite. No code change.

## Related findings

M-036 (field surface), S-024 (grading), N-009a (percentage_used width).

## Open questions

None.
