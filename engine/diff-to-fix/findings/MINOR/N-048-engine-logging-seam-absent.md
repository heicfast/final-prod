# MINOR N-048 — Engine-level logging seam absent: the og has a three-layer log architecture (Log.mo shared EPMLOG + PubLog ring + Logger.dll async) wired into engines; the core logs nothing outside the CLI

**Severity:** MINOR
**Module:** logging — new module row (infrastructure)
**Parameters affected:** P35 (error recovery — diagnostics), P46 (documentation of the contract — no logging contract exists), P54 (module boundaries — no logging seam), P68 (documentation accuracy)
**Corpus references:** `re-workspace/reversed/ghidra-mo/Log_decompiled.c` — 64 KiB shared staging mapping `EPM_{1E956C11-…}` (:159-176), named mutex `EPMLOG_{1BFCF65F-…}` (:193), `<exe-dir>\EPMLOG.log` (:604-621/:723-743), **10 MB truncate-rollover** (:71-73), line format `[%04d-%02d-%02d %02d:%02d:%02d:%03d] (T:%d/P:%d) msg\r\n` (:699), per-disk dump `<exe-dir>\HD%03d.dpt` (:1428-1479), fsclog.dll forwarder (:1758/:2063); `re-workspace/reversed/ghidra-misc/PubLog_decompiled.c` — InitLog(file, LOGLEVEL, LOGTYPE), LOGTYPE 1 = **2 MB ring** mapping `DRW_{1E956C11-…}` (:119-135), CDirectLog default `./log/DefaultLog.txt` (:371-381); `re-workspace/reversed/ghidra-partition/Logger_decompiled.c` — singleton async queue + worker (:1630-1696/:1776/:1815), UTF-16 append under LockFileEx (:1943-1950), 10 MB cap → keep-second-half compaction (:1962-2015); engine adoption census (ghidra-searchdll/*_imports.txt): FATSearchDLL (ReleaseTrace), NTFSSearchDLL + RAWSearchDLL (LogInfo) import PubLog; Btrfs/EXT4/HFSPlus/ReFS/Zfs/exFAT import none; PartitionRecoveryD.mo logs scan START/END + clone read errors (PRD:21962, :22121, :20075)
**Core references:** audited `7cf6754` — `src/app/rrecov.cpp:122-143` (app-only `--log` FILE* → vfprintf; 12 references, all in rrecov.cpp); src/core/** contains no logging API and emits no log lines (grep-verified)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The og runs a three-layer logging architecture — a cross-process shared-memory staging log (EPMLOG.log with 10 MB rollover), a PubLog 2 MB ring for the UI-family binaries, and an async-queue Logger.dll with compaction — and the engine DLLs that log (FAT/NTFS/RAW SearchDLLs + PartitionRecoveryD's scan START/END brackets) write through it. The core has no engine-level logging at all: the only logging is the CLI's `--log` vfprintf, so an engine misbehavior in the field is undiagnosable from artifacts the reference product would have produced.

## What the reference does

See citations — the three layers, their size caps, line formats, mutexes,
and the per-binary adoption census are decoded in
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §E.

## What our core does

`--log` at the app layer only; engines are silent. The session JSON
summary is the closest diagnostic artifact (and a good one), but it is a
report, not a log — no scan-bracket events, no per-error trails.

## Gap

Falsifiable: no function in src/core/** emits a log line through any
logging API (the API does not exist); the og's engines emit scan
START/END with disk/method/range/duration and read errors.

## Impact

Field diagnosability only — no recovery-behavior divergence. The og's
support flows (BcdLog.txt, EPMLOG.log, HD%03d.dpt disk dumps) let support
reconstruct what a scan did; the core cannot. MINOR (infrastructure gap,
documented here so the parity docs stop claiming nothing about it).

## Reproduction / verification

Corpus-derived (all three layers decoded; imports census from the corpus's
own *_imports.txt files). Core grep-verified. Evidence:
`evidence/decompiled-quotes/s5-image-fscheck-persistence.md` §E.

## Fix plan (for the engineering team)

1. Decide the seam: recommended minimal — a `cr::log` header (levels
   Error/Warn/Info; sink = std::function registered by the host, default
   null) in include/cr/cr_log.h; engines call it at scan START/END,
   checkpoint, repair-plan boundaries, and read-error sites (the og's
   event set).
2. The CLI wires `--log` into the sink (reusing rrecov.cpp:122-143).
3. Keep it lock-free/single-writer per scan (no shared mapping needed for
   parity — the og's cross-process mapping serves its multi-process UI,
   which the core does not have; register the difference).
4. Tests: `LogSinkReceivesScanBrackets`, `LogSilentByDefault` (null sink,
   zero overhead path).

## Tests to add

The two named tests.

## Related findings

M-040 (BcdLog.txt — the boot-repair layer's log), M-042 (checkpoint
events ride the same seam), N-043 (progress watchdog — the heartbeat lane
could share the sink).

## Open questions

What FATSearchDLL's ReleaseTrace actually emits (event set undecoded —
only the import was census-verified).
