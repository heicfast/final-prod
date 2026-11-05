# S42 — the DefaultLog.txt real-run decode: the Ldq read-parameter layer (M-119's machinery, first-hand at the transport entry) + the bad-sector skip constants + the write-failure observable

**Lane:** S42 (session-42 continuation audit) · **Tree:** core `869da779`-lineage · **Corpus:** `be0cfe9`.
**Sources:** `og-codebase/log/DefaultLog.txt` (573 lines, read in full this session — the S12 walk dispositioned the tree-builder half; the three classes below had ZERO trail coverage: `rg 'Ldq|bad sector:|ReadAndWriteDataRunList'` over diff-to-fix/ → 0 hits at this lane's open) + `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c`.

## 1. The Ldq read-parameter layer — M-119's machinery decoded at its entry (with 393 real-run firings)

`DefaultLog.txt` carries **393 occurrences** of `----Ldq:Read sector parameter error = %I64d----`
(the printed values alternate 73 with {7, 15, 23, 31, 39, 47, 55, 63, 71} — all odd, the
og's own scan layers driving the read API out-of-parameter on a BASIC disk). The emitter
is the og's core disk-read entry (DRWUI_decompiled.c :180139-180160, the `FUN_1400cc...`
logical-disk-query family — "Ldq" = logical-disk query):

```c
/* :180139-180151 — the parameter-validation ladder (verbatim structure) */
if (((param_4 < 0) || (param_5 < 0)) ||                       /* negative start or count */
    (0x100000000 / sector_size <= param_5) ||                 /* count >= 4 GiB of sectors */
    (device_size <= param_4)) {                               /* start at/beyond device end */
    LogInfo("----Ldq:Read sector parameter error = %I64d----");
    goto LAB_1400cc2d5;                                        /* REFUSE (error out-param set) */
}
if (device_size < param_4 + param_5) {
    param_5 = device_size - param_4;                          /* CLAMP the over-long read */
}
```

**This is M-119's site family, first-hand at the transport entry** — the S12 lane decoded
the clamp+zero-fill+succeed semantics via the CBasicPartitionDevice RTTI
(DRWUI:183633-183666) and one DRWUI.log:638 firing; this session's decode adds: (a) the
full parameter-validation ladder (three REFUSE classes before the CLAMP class: negative
start, negative count, count ≥ 4 GiB-of-sectors, start ≥ device size); (b) the zero-fill
branch at :180157-180160 (`LogInfo("...Read sector error in %d")` + `memset(param_3, 0,
count × sector_size)` — the zero-fill-succeed on the read-error path); (c) the real-run
evidence that the og's own scan layers issue out-of-parameter reads ROUTINELY (393
refusals in one captured run — the refusal-with-log is normal operation, not an error
state). **M-119's addendum lands in its finding file** — the fix plan's clamp ladder gains
the three refuse-classes and the 4 GiB-of-sectors count ceiling.

## 2. The bad-sector skip machinery — the constants decoded (:179930-180080)

The four `bad sector:%I64d, count:%d` lines in DefaultLog (sectors 509011968/509017088,
607037440/607038464; counts 8192/2048 — the skip ranges on a disk with real dead spots)
come from the og's skip loop:

- **Initial read span 2 MiB** (`uVar10 = 0x200000`), the skip step = 2 MiB/sector_size sectors.
- **Backoff: the read size DOUBLES every 3 iterations, capped at 64 MiB** (`uVar3 =
  uVar10 * 2; if (0x4000000 < uVar3) uVar10 = 0x4000000` — 0x4000000 = 64 MiB).
- **Inner per-granularity skip when read > 4 MiB** (`0x400000` threshold — the loop
  stepping down by the granularity uVar11, default 1 sector, configurable via the device's
  +0x18 field).
- **CRC-error trigger**: `local_94 == 0x163` (ERROR_CRC, 355) — the bad-sector class.
- **UI message at > 0x28000 skipped sectors** (163,840 sectors = 80 MiB at 512 B):
  `LogInfo("Send bad sector message.")` + the `FUN_14026bf00(msg, 1, 0x10, ...)` UI event
  + the once-only gate at +0xf2.
- **Skip-list recording**: `FUN_1404ec740(badmap, start/granularity, 1, (gran-1+span)/gran)`
  — the skipped ranges recorded into the device's bad-sector map.
- **Loop breaks**: cancel flag (+0x1e), device end, or 1 GiB total skipped
  (`0x40000000 < local_68`).

**The core's counterpart** (`include/cr/storage/cr_retry.h` + `cr_retry.cpp`): bisection
down to sector granularity, Skip mode zero-fills only failing sectors, ReadErrorMap
recorded and preserved into scan results/sessions, the BadSectorTip user-facing state
(FB-O03). **Verdict: mechanism-divergent (skip-forward-with-backoff vs bisection),
outcome-equivalent** (both isolate to sector granularity, both zero-fill, both record,
both surface the state). The og's constants (2 MiB initial / ×2-per-3-steps / 64 MiB cap
/ 4 MiB inner threshold / 80 MiB message gate) are transport-internal tunables — REGISTER
(register-class datum for the storage module; no finding: the user-observable — one dead
spot never aborts a scan, the skip state surfaced, the failures recorded — matches).

## 3. The recovery-write failure observable

`Failed to write file! CFileRecoverWorker::ReadAndWriteDataRunList  1515` — one real-run
firing: the og's recovery worker logging a per-file write failure with function+line
(`Failed to write file! %s %d`, strings :35729). The core's SafeWriter records per-item
failures with per-extent honesty (the S-052 landing's per-extent failure model) and
surfaces them in the recovery report — the observable class (per-file failure logging at
recovery time) is covered semantically; the og's function+line log format is a log-channel
convention datum (the N-048 log-census family). No finding; registered as the writer
module's real-run error-observable datum.

## Verdict summary

| Observable | Class | Disposition |
|---|---|---|
| Ldq parameter errors (393×) | the og's read-boundary semantics — M-119's machinery | **M-119 addendum** (the validation ladder + the real-run corroboration) |
| bad sector:N, count:M (4×) | the skip machinery's logging | **Register** (storage module — the constants decoded, outcome-equivalent to the core's bisection) |
| Failed to write file! … 1515 (1×) | the recovery-write failure log | **Register** (writer module — the N-048 log-census family) |
| The tree-builder trace | S12-dispositioned (S-048/M-103/S-051 + UI OOS) | unchanged |
