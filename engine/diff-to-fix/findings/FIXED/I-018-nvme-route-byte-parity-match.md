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

## Resolution

**Status:** ACKNOWLEDGED (positive MATCH record)
**Verified by:** src/win/cr_health.cpp:252-278 re-read this session - IOCTL_STORAGE_QUERY_PROPERTY with STORAGE_PROTOCOL_SPECIFIC_DATA {ProtocolType Nvme, DataType LogPage, health page}, offset/length per the corpus cite - line-by-line identical route to DiskHealth_decompiled.c :13351-13375.
**Resolution notes:** No divergence: the NVMe health transport route is byte-parity MATCH. Recorded so the route is never "improved" without a corpus reason. The NVMe field-coverage/units item (M-036) remains its own finding.

**Status:** ACKNOWLEDGED (positive record — verified MATCH)
**Verified by:** this session: `src/win/cr_health.cpp:244-278`
`read_nvme_health` — IOCTL_STORAGE_QUERY_PROPERTY (0x2D1400) with
STORAGE_PROTOCOL_SPECIFIC_DATA {ProtocolType=ProtocolTypeNvme, DataType
log page, page 2 (health), length 0x1000} re-read directly; matches
DiskHealth_decompiled.c :13351-13375 byte-for-byte on the route.
**Resolution notes:** no code change. The field-surface difference above
the transport stays M-036 (MAJOR tier). This row upgrades any "corpus
silent" NVMe claims to verified MATCH.
