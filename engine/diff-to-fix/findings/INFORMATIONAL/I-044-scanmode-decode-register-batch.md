# INFORMATIONAL I-044 — the scan-mode decode register: the auto-advance state machine, the hardwired gates, the host-side chunk skip, and two corpus citation corrections

**Severity:** INFORMATIONAL
**Module:** orchestration / scan-mode semantics (the "how scanning works" focus battery)
**Parameters affected:** #31 Scan lifecycle (the decode record); #68 Documentation accuracy (the two citation corrections)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c:567875-567892` (the state-queue push: state 1 pushed unconditionally when +0x3d/+0x3f clear); `:568079-568175` (the ScanThread state loop; telemetry events 0-3); `:903252-903380` (the controller factory: 0=CDeletedRecovery, 1/5=CGenericRecovery, 4=CRecycleBinRecovery, 6/7=CCloudRecovery); `:947721-947722` (settings+0x60 init 1 / settings+0x6c init 0); `:917015-917040` (`FUN_140581500` — the searcher-set **creation** site, correcting the arch/04 §3.3 + D-36 citation of `FUN_140580720`); `:919487-919505` (`FUN_140585150`: the all-allocated-chunk memcmp skip — host-side, 4 MiB granularity); `:913047-913110` (the scanext4/scanzfs-gated quick factory — the shipped product has `scanzfs` absent: ZFS recognition compiled out)
**Core references:** `src/core/orchestrator/cr_scan.cpp:1170-1275` (the always-on quick→deep→carve order); `include/cr/orchestrator/cr_scan.h:186-189` (`run_quick/run_deep/run_carve` default true); `diff-to-fix/SCAN-ENGINE-DELTA.md` rows C1/Q1/A1/A5 (the dispositions this register refines) — at audited core `b26017a7`
**Status:** READ-ONLY FINDING — no fix applied (register entry)

## Summary

This register records the scan-mode-semantics decode products that refine existing dispositions without changing their verdicts: (1) the quick→deep auto-advance is an **engine-side state machine** (`CDoScanPartition::DoScan` pushes states 0 and 1 unconditionally; the scan thread pops them; the recoverability grade only selects the coaching text) — the C1 row's "UI-layer state machine; N/A" rationale was wrong even though the verdict (net-parity via the core's default-on stages) stands; (2) the celebrated "settings+0x60 RAW gate" and "settings+0x6c deep gate" are hardwired constants with no runtime writer in the shipped DRWUI — the gates never gate; (3) the advanced phase's host-side sweep skips only fully-allocated 4 MiB chunks (memcmp vs an 0xff mask) before fanning each chunk out to every controller; (4) two citation corrections: searcher-set creation is `FUN_140581500` (not `FUN_140580720`, which is the run phase), and the shipped product has ZFS recognition compiled out at all three `scanzfs` gate sites (a sharpening of the D-50/I-016 superset registration).

## What the reference does

Each item verified first-hand this session at the cited lines (see the worksheet, `diff-to-fix/evidence/decompiled-quotes/s42-scanmode-state-machine.md`): the state-queue push and pop; the controller-per-state factory; the two settings DATs' init and the absence of writers; the memcmp chunk skip; the creation-site function; the shipped `config/version.ini`'s `scanext4=1` and absent `scanzfs`.

## What our core does

The core's stage model (quick→deep→carve, all default-on, CLI opt-outs) is the correct parity translation of the og's unconditional auto-advance — the decode confirms the design rather than contradicting it. No core change is proposed by this register.

## Gap

None behavioral — the dispositions' verdicts stand; the rationale/citation corrections are recorded so future sessions do not re-derive them wrongly.

## Impact

Record-keeping only. The two citation corrections matter for any future session quoting arch/04 §3.3 or D-36.

## Reproduction / verification

All sites read first-hand this session (worksheet §1-§5). The ZFS-shipped-off fact was verified against the og-codebase's `config/version.ini` directly.

## Fix plan (for the engineering team)

1. Annotate `diff-to-fix/SCAN-ENGINE-DELTA.md` C1 with the corrected rationale (engine-side state machine; net-parity via default-on stages) and A1/A5 with the hardwired-constants note; correct the `FUN_140580720`→`FUN_140581500` creation citation in the row and in CORPUS-ERRATA (the corpus doc arch/04 §3.3 carries the same number).
2. No code change.

## Tests to add

None.

## Related findings

N-181 (the RAW-first ordering — the one behavioral residue of this decode), N-182 (the deep-range resolver), D-36/D-50/I-016 (the registered deltas sharpened here).

## Open questions

Device-type 6's identity (rides N-182's open question).
