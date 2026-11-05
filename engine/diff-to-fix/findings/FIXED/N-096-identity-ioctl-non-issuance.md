# MINOR N-096 — Identity IOCTL non-issuance: the og client queries GET_DISK_CONTEXT_MODE per device open; the core never issues any identity IOCTL

**Severity:** MINOR
**Module:** transport
**Parameters affected:** S35-adjacent (error recovery — the informational mode check), P42-adjacent (the client half of the identity surface), Q68 (the disposition text)
**Corpus references:** `re-workspace/artifacts/PartitionRecovery/epmntdrv-internals-v2--completion-engine-identity-ioctls.md` §2.3 (GetDiskContextMode returns the per-open mode dword); the og client's actual usage: `Device_decompiled.c:52598` — "User mode issues only `0x60002408`" (4-byte mode out), as registered at `docs/REFERENCE-DRIVER-BEHAVIOR.md:95`
**Core references:** audited `35adcb9`. `include/cr/win/cr_rda.h:50-61` defines all three identity IOCTL constants (`kIoctlGetProcName/kIoctlGetFileObjName/kIoctlGetDiskContextMode`); **zero client-side issuance** — the constants are documentation-only (the ARCH-DEAD-2 register: "Per-constant Grep: 0 uses outside cr_rda.h"). The core's open path (`src/win/cr_transport.cpp:474-510`) opens the grammar path and reads, never issuing the mode query.
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og's user-mode client issues `GET_DISK_CONTEXT_MODE` (0x60002408) on every device open — a per-handle mode probe (0 = the chosen layer's driver is `\Driver\Disk`, 1 otherwise) it uses informationally. The core's transport defines the constant but never issues any identity IOCTL — the only client the og actually exercises on this family is unmirrored.

## What the reference does

The og client opens the device and queries the mode dword (Device_decompiled.c:52598 — the sole identity IOCTL user mode issues), keeping an informational per-handle record of how the disk stack resolved.

## What our core does

The constants exist in `cr_rda.h` (wire parity complete); no issuance anywhere (grep-verified via the ARCH-DEAD-2 register, re-confirmed this session).

## Gap

The mode-check parameter is unmirrored: zero issuance vs the og's per-open query. Falsifiable: instrument an og run — one 0x60002408 per open; instrument the core — zero.

## Impact

None known functionally — the og's use of the mode value is informational (`Device_decompiled.c:52598-52604`); no behavioral branch decoded depends on it. The `DRIVER-UTILIZATION.md:64` "MATCH" row overstates the parity (the wire is matched; the client usage is not).

## Reproduction / verification

Corpus-derived (the decompile site + REFERENCE-DRIVER-BEHAVIOR:95); core first-hand (the constant census + the open path read + the ARCH-DEAD-2 register). Performed by this session's driver-docs lane; re-confirmed by the lead.

## Fix plan (for the engineering team)

1. Either issue the query at open (`cr_transport.cpp`'s open path — a 4-byte-out `DeviceIoControl` after CreateFileW, result stored on the handle or logged at trace level), or register the non-issuance as a deliberate client-side delta in PARITY-DELTAS (the D-DRV-5 lineage) and downgrade the DRIVER-UTILIZATION row's wording from MATCH to "wire-matched, issuance deliberately omitted".
2. If issuing: assert the 4-byte out in a unit test via the win-stub seam.

## Tests to add

- `Transport.ModeQueriedOnOpen` (win-stub: the IOCTL fires once per open) — only if fix option 1 is chosen.

## Related findings

- D-DRV-5 (the driver-family register this extends).
- The S7/driver lane (the two DRIVER-REPO-ITEM handoffs from this session's verification are recorded in the worklog — READ-length semantics nuance and the disk.sys-comparison field reading; they belong to that repo, not this one).

## Open questions

Whether any og branch beyond the informational read consumes the mode value — undecoded; the fix plan's option 2 is safe regardless.


---

## Session-11B addendum (2026-10-29): still-active, with a partial-rehabilitation nuance

The identity family (kIoctlGetProcName/GetFileObjName/GetDiskContextMode = 0x60002400/04/08, cr_rda.h:50-52) is still issued zero times — the open paths (cr_transport.cpp:472+) CreateFileW and probe geometry without any 0x600024xx. **Nuance:** the EXTENDED family (kIoctlGetInfo/GetVersion, 0x6002440/48) IS now issued at cr_transport.cpp:449/:452 — a different code family from this finding's identity trio, but it partially rehabilitates the "wire-matched" framing: the transport now talks to the driver beyond open/read. The finding's scope (the identity family) is untouched. DRIVER-UTILIZATION.md:64's MATCH-row overstatement stands.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-096 landing — recorded post-push in the ledger row)
**Verified by:** the three registration sites read back current: PARITY-DELTAS carries the new D-DRV-6 row (the deliberate non-issuance with the rationale and the informational-only finding); DRIVER-UTILIZATION's control-IOCTL row now reads "MATCH (wire) / OMITTED (client issuance — deliberate, D-DRV-6)" — the overstated MATCH is gone; cr_rda.h's three constants re-verified unchanged (the wire parity they document stays intact).
**Resolution notes:** The finding's fix-plan option 2 executed (the register-the-delta path): the og's sole client-side identity IOCTL (the per-open GET_DISK_CONTEXT_MODE 4-byte mode probe) has NO decoded behavioral consumer anywhere in the corpus — its result is an informational per-handle record — so issuing it in the core would add a per-open round trip whose output nothing consumes. The deliberate lighter client posture is now registered as D-DRV-6 in PARITY-DELTAS (beside its D-DRV-5 lineage), and the DRIVER-UTILIZATION row's wording is downgraded from MATCH to the wire-matched/issuance-omitted truth the finding demanded. Option 1 (issuing the query) stays available to any future consumer that needs the mode value — the constants and semantics remain documented in cr_rda.h at wire parity. Doc-only registration; no code change (the finding's own impact analysis: "none known functionally").
