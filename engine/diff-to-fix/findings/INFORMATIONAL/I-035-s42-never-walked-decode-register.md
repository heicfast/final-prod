# INFORMATIONAL I-035 — The S42 never-walked decode register: SearchResult.txt (the QA-gated full-result-list dump), the DRWUI user-mode bad-sector skip policy (the D-38/FB-S01 og-side record enrichment), and the failed-files HTML fragment constants

**Severity:** INFORMATIONAL (deliberate-difference / decode-register posture)
**Module:** audit-trail (never-walked corpus decode register, the I-031/I-032 lineage)
**Parameters affected:** P46/P68 (documentation accuracy — the og-side records these enrich), P60 (bad-sector handling — the og-side model correction), P16-equivalent (filter model — SearchResult's flags)
**Corpus references:** `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (all first-hand this session): `:388969` (`GetPrivateProfileIntA("register","SearchResult",0,"%s/Config.dat")` — the QA gate), `:388972` (`sprintf("%s/log/SearchResult.txt")` + `fopen(…,"wb")`), `:388974-389045` (the dump loop over the two result maps `param_1+0x40` existing / `param_1+0x58` lost, per entry: the full path string, `",folder:0|1"`, `",deleted:0|1\r\n"` — the deleted bit is `node+0x89` bit 1); the bad-sector cluster: `:179826-179899` (`FUN_1400cb580`, the `\\.\PhysicalDrive` read wrapper — the trigger `if ((iVar5 == 2) && (0xb < *(int *)(param_1+0xa8)))` at `:179891-179895`: **12+ bad reads in one call engages the skip routine**), `:179912-180075` (`FUN_1400cb7e0` — the skip routine: `"Start skip bad sectors"`, initial chunk `0x200000` **2 MiB**, allocation `0x4000000` **64 MiB**, adaptive doubling `uVar10*2` capped at `0x4000000`, error code **0x163**, `CancelIo`, sector-granular boundary refinement, the toast `0x28000 < param_1[0x14]` (**163,840 skipped sectors → "Send bad sector message."** + event 0x10), the total cap `0x40000000 < local_68` (**1 GiB → "Break skip bad sectors, ReadSize = %I64d"**), `"Skip bad sectors Finish."`), `:180578-180600` (`FUN_1400cc800` — the cached bad-range short-circuit over the `DAT_1425e71b0` linked list, returning 0x163 without issuing the read), `:820090-820113` + `:819998-820059` (the sector-map bit-setter + its **file-backed 1-bit-per-sector bitmap** via `CreateFileW` + `DeviceIoControl 0x900c4` + `CreateFileMappingW`/`MapViewOfFile`), `:180140-180160` (the ReadThreadData/`Ldq:Read sector error` logging context); the direct-open sites `:176835`/`:176879`/`:177099`/`:177642`/`:178061`/`:178805` (`\\\\.\\PHYSICALDRIVE%d`); the user copy `:1551139` ("Program is trying to skip bad sectors to find data.").
**Core references:** audited `869da779`. The retry layer: `include/cr/storage/cr_retry.h` (`RetryingDevice` — per-policy retries, bisection to sector granularity, Skip-mode zero-fill of only failing sectors, `ReadErrorMap` preserved) — the registered deliberate equivalent (D-38, `docs/PARITY-DELTAS.md:891-894`; the module note `modules/storage.md:28`); `AUDIT-FALLBACKS.md:88` (FB-S01's "og has no transport-wide retry at all"); no SearchResult.txt equivalent (grep-negative over src/); the report fragment constants' note rides N-155.
**Status:** READ-ONLY REGISTER — no fix applied (deliberate-difference records + decode enrichment; the one truth repair to FB-S01's wording is fix-lane work)

## Summary

Three never-walked corpus decode surfaces, recorded so no future lane re-derives them: (a) the og's QA-gated **SearchResult.txt** dump — a registration-state-gated full-result-list export (path + folder flag + deleted flag per entry, two passes over the existing/lost maps) that is the ONE list-export verb in the binary (R-58's "no CSV/list-export verb is documented" stays true at the product level, but the binary-level statement needed this register note); (b) the og's **user-mode bad-sector skip policy** in DRWUI's own `\\.\PhysicalDrive` reader — a rich adaptive skip/isolation engine (12-read trigger, 2→64 MiB adaptive chunking, sector-granular boundary refinement, a cached bad-range short-circuit, a 163,840-sector toast threshold, a 1 GiB total cap, a file-backed 1-bit-per-sector map) that the trail's og-side records (D-38/FB-S01) never decoded — FB-S01's "og has no transport-wide retry at all" is true only of the driver wire, not this user-mode layer; (c) the failed-files HTML report's fragment constants (N-155's companion datum). The core's registered mechanism (`RetryingDevice`, D-38) covers the behavior class deliberately-differently — no capability gap is filed; the register enriches the og-side record and corrects the two register claims.

## What the reference does

(Quotes verbatim in `evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md` §3-§4.)

**SearchResult.txt:** gated by `Config.dat`'s `[register] SearchResult` (default 0 — QA builds only); on scan completion, the results container dumps every entry of both result maps to `log/SearchResult.txt` as `<full path>,folder:0|1,deleted:0|1\r\n`. This is a binary-level list-export verb R-58's grep-negative never surfaced (it is not a product surface — the register records it so the "ONLY corpus-documented results-export surface" claim carries the QA-gate qualifier).

**The user-mode skip policy:** DRWUI's direct `\\.\PhysicalDrive` read wrapper counts bad reads per call; at 12+, `FUN_1400cb7e0` takes over: it reads in 2 MiB chunks (doubling every 3 steps, capped at 64 MiB), classifies error 0x163 as the bad-sector marker, cancels outstanding IO per failed chunk (`CancelIo`), refines chunk boundaries down to sector granularity marking readable spans, records every skipped sector in a file-backed 1-bit-per-sector bitmap, short-circuits future reads that overlap known-bad ranges (the `DAT_1425e71b0` list — no read issued), toasts the user after 163,840 skipped sectors, and hard-stops skipping after 1 GiB total. The user-visible copy: "Program is trying to skip bad sectors to find data."

## What our core does

`RetryingDevice` (D-38's registered deliberate difference): per-`RetryPolicy` retries, bisection to sector granularity, Skip mode zero-filling only the failing sectors, `ReadErrorMap` carried into results/sessions and the disk-map bad overlay (S-009). The mechanism class is covered; the og's specific parameters (the 12-read trigger, the 2→64 MiB adaptive chunking, the 1 GiB cap, the 163,840 toast threshold) are NOT adopted — and the register records them so the delta is knowable rather than silent. No SearchResult.txt equivalent exists (the core's `tree --csv/--json` exports are the product-level superset; the QA dump is an og diagnostics channel, deliberately not ported). The HTML fragment constants are N-155's follow-up datum.

## Gap

Deliberate-difference records, no capability gap. The one actionable truth repair: FB-S01's and D-38's og-side wording ("og has no transport-wide retry at all" / "the og mechanism is per-engine and UI-mediated") should carry the decoded user-mode layer's parameters — the og DOES have a transport-wide user-mode skip engine, with specific constants, in DRWUI's PhysicalDrive reader.

## Impact

Informational: future lanes reading D-38/FB-S01 get the full og-side picture; the SearchResult.txt register closes the last binary-level export verb; no product behavior changes.

## Reproduction / verification

Executed this session: the `sed -n` reads of every cited line range (the gate, the dump loop, the trigger, the skip routine's constants, the short-circuit, the bitmap file-backing) — transcripts in the evidence file §3-§4; grep-negatives for the core's absence of the QA dump; the D-38/FB-S01 wording read at `docs/PARITY-DELTAS.md:891-894` / `AUDIT-FALLBACKS.md:88`.

## Fix plan (for the engineering team)

1. D-38 addendum (doc-only): append the decoded og-side parameters (the 12-read trigger, 2→64 MiB adaptive chunk, 0x163, sector-granular refinement, cached bad-range short-circuit, 163,840-sector toast, 1 GiB cap, the file-backed bitmap) with the :179826-180075 cites; note the layer is DRWUI's user-mode PhysicalDrive reader, distinct from both the driver wire and the engine CatchBuffer.
2. FB-S01 wording repair (doc-only): "no transport-wide retry at the DRIVER wire; the og's user-mode DRWUI reader implements its own adaptive skip (I-035's register)".
3. R-58 qualifier (doc-only): "no PRODUCT list-export verb; the QA-gated SearchResult.txt dump exists in the binary (I-035)".
4. Acceptance: the three register sites tell the decoded truth.

## Tests to add

None (deliberate-difference register).

## Related findings

N-155 (the HTML report this register's item (c) companions); D-38/FB-S01 (the records items (b) enriches); S-009 (FIXED — the bad overlay the ReadErrorMap feeds); N-156 (the register-truth class); I-031/I-032 (the register lineage).

## Open questions

Which scan flow consumes the `0x1400cb` PhysicalDrive reader cluster (the `ReadThreadData`/`Ldq` logging context suggests the smart-partition/whole-disk scan reader, distinct from the MOM `Device.mo` layer the transport module decoded) — UNDETERMINED; a caller-trace of the class's read entry at `:180196` would close it. No product decision rides on it (the core's mechanism is registered deliberate-different either way).
