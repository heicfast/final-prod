# SEVERE S-062 — The ONE chunk-chain walk publishes the last-valid-chunk end on the size-gate breaks where the og publishes 2×device flat — a first-chunk decline carves nothing (the M-106 landing's own regression)

**Severity:** SEVERE
**Module:** carve (ONE / OneNote analyser, the M-106 chunk-chain walk)
**Parameters affected:** A27 (fragment handling / reconstruction strategy), B2 (recovery correctness — the extent), P2 (false-negative class: a file the og recovers, the core drops)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` FUN_180044e30 :52316-52358 (read first-hand this session; the comma-operator decode below); the FileInfo ctor's 500,000,000 clamp FUN_180016080 :17191-17209; cross-checked against `re-workspace/reversed/ci-multitool/RAWSearchDLL/retdec_decompiled.c` function_180044e30 (independently shows the same exit shape)
**Core references:** audited `34d6cf10` (HEAD at filing) — `src/core/carve/cr_analysers.cpp` validate_one :5768-5795 (the size-gate break at :5776-5777, the cursor bookkeeping at :5778-5780, the 500M clamp at :5781-5782; the loop's og-decode comment at :5744-5758 carrying the mis-decode "The og's size is the cursor at the chain's break point"); landed by c98eb7a7 (M-106)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The M-106 landing ported the og's ONE chunk-chain size walk but mistranscribed one exit-value family. In the og's walker, the comma expressions set the published size (`uVar5`) to **2×device** at the moment the primary size field is read — so when the chain breaks on a size-gate failure (both size fields 0, or a declared size > 2×device), the og publishes the flat **2×device** extent (then the 500M FileInfo clamp). The core instead keeps the **last fully-valid chunk's end** — which is **0 when the first chunk declines**, meaning the og recovers a large span where the core carves nothing. Both the function comment and the exit behavior diverge.

## What the reference does

The og's walker (FUN_180044e30, quoted in full in
`diff-to-fix/evidence/decompiled-quotes/s50-landing-divergences.md` §1):

```c
uVar1 = *(longlong *)(puVar2 + 0x70) * 2;          /* 2 x device size */
while (((((uVar5 = uVar4, uVar4 <= uVar1 &&        /* (A) uVar5 = cursor  */
          ... read window ...
         (*(longlong *)(puVar2 + 4) == 0x7853b1ae4da7d88c)) &&   /* chunk GUID */
        ((*(int *)(puVar2 + 0xc) == -0x2c69d630 &&
         ((uVar3 = *(ulonglong *)(puVar2 + 0xc4), uVar5 = uVar1, uVar3 != 0 ||   /* (E) */
          (uVar3 = *(ulonglong *)(puVar2 + 100), uVar3 != 0)))) &&
        (uVar3 <= uVar1))) {                       /* (G) size <= 2x-device */
  uVar4 = uVar4 + uVar3;
}
*(ulonglong *)(param_2 + 0x10) = uVar5;            /* the published size */
```

The comma semantics: (A) sets `uVar5` = the current cursor on every
iteration; (E) — executed the moment the primary size field is loaded —
sets `uVar5` = 2×device. Exit table:
- both size fields 0 (E's `!= 0` tests both fail) → **uVar5 = 2×device**
- size > 2×device (G fails) → **uVar5 = 2×device**
- window read miss / GUID mismatch / cursor > 2×device → **uVar5 = the cursor**
- normal exhaustion → uVar5 = the cursor

The retdec cross-check independently shows `v5 = v2;` (v2 = 2*device)
immediately before both size-gate breaks — two decompilers agree. The
published value then rides the og FileInfo ctor's 500,000,000 clamp
(FUN_180016080 :17203).

## What our core does

`validate_one` (cr_analysers.cpp:5768-5795 at the audited tree):

```cpp
u64 sz = u64_from_bytes_le(c + 0xc4);
if (sz == 0)
    sz = u64_from_bytes_le(c + 0x64); /* the fallback field */
if (sz == 0 || sz > dev2)
    break; /* the 0<size<=2x-device gate */       <- keeps `size` = last-valid end
pos += sz;
size = pos; /* the end of this valid chunk - the break keeps it */
```

On the two size-gate failure exits the core's published size is the end of
the last fully-valid chunk — **0 when the first chunk declines** (a
zero-size-field or over-2×device first chunk), so `r.end = hit + 0` and no
useful extent is emitted. The block comment above the loop claims "The og's
size is the cursor at the chain's break point (uVar5 - the end of the last
FULLY-VALID chunk)" — the comma-decode refutes this for exactly these two
exits (the cursor-carried exits — read miss, GUID mismatch, normal
exhaustion — DO match).

## Gap

Falsifiable: on a ONE candidate whose first chunk's both size fields are 0
(or whose first chunk declares a size > 2×device), the og's walker publishes
min(2×device, 500,000,000) as the extent; the core publishes 0. On a
mid-chain size-gate break the og publishes min(2×device, 500M) where the
core publishes the accumulated valid prefix.

## Impact

The corrupt-first-chunk population: a OneNote file whose chunk-0 size field
is zeroed or garbage (a common corruption shape — the chunk header sits in
the damaged zone) is **recovered by the og and dropped by the core** —
silent missed recovery on exactly the population the carve layer exists
for. The mid-chain population: extent-size divergence between the two
engines (the og's flat 2×device can be shorter or longer than the core's
valid-prefix). The landed comment also mis-teaches the next reader.

## Reproduction / verification

Decompiled-evidence class (both sides read first-hand this session; the
full quotes with line numbers in
`evidence/decompiled-quotes/s50-landing-divergences.md` §1, the retdec
cross-check noted there): the og at RAWSearchDLL_decompiled.c:52316-52358,
the core at cr_analysers.cpp:5768-5795 (audited `34d6cf10`). The
landing-parity sweep that surfaced this: 
`evidence/executions/s50-landing-parity-sweep.md` row 9.

## Fix plan (for the engineering team)

1. Architectural: none — one exit-value correction inside the existing walk.
2. Files: `src/core/carve/cr_analysers.cpp` (validate_one), the M-106
   block comment.
3. Algorithm: publish `min(2*dev2, 500000000)` on the two size-gate break
   exits (both-fields-zero and size>2×device) — i.e. on those breaks set
   `size = std::min(dev2, 500000000ull)` before falling out; keep the
   cursor-carried value for the read-miss/GUID/exhaustion exits exactly as
   today. Re-truth the block comment (drop "the cursor at the chain's break
   point" in favor of the exit table).
4. Tests (in `tests/unit/test_carve.cpp`, the M-106 family):
   - `Carve.OneFirstChunkZeroSizePublishesTwoDeviceClamped` — a ONE fixture
     whose chunk-0 has both size fields 0, on a small device (e.g. 4096-byte
     probe window); expect the extent = min(2×device, 500M), RED before.
   - `Carve.OneMidChainZeroSizeKeepsOvershoot` — a 2-chunk fixture whose
     chunk-1 size field is 0; expect extent = min(2×device, 500M) (og
     semantics), RED before (today it keeps the chunk-0 end).
   - `Carve.OneGuidMismatchStillKeepsCursor` — the read-miss/GUID exits keep
     the last-valid cursor (guards against over-applying the patch).
5. Contract/doc: the M-106 row in `diff-to-fix/modules/carve.md` and the
   FIXED/M-106 Resolution gain the exit-table correction note.
6. Acceptance: the three tests green; the block comment carries the
   two-exit exit table; no change on valid-chain fixtures (the existing
   M-106 tests stay green).

## Tests to add

Listed in step 4 above — the three named tests, the first two RED before
the fix by construction.

## Related findings

M-106 (FIXED — the landing this regression rode); S-059 (the same
"extent/refusal vs content-preserving write" consequence family); the
M-107 stand-in members (the ONE walk replaced a 64 MiB stand-in — this
finding is about the REPLACEMENT's exit values, not the stand-in).

## Open questions

None — the comma-operator decode is unambiguous and cross-checked by two
decompilers.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; S58 lane)
**Verified by:** `Carve.OneFirstChunkZeroSizePublishesTwoDeviceFlat` (RED pre-fix: no ONE record at all - the 0-size extent; GREEN: the flat 2x-device extent, device-clamped by the probe limit), `Carve.OneMidChainOversizeChunkPublishesTwoDeviceFlat` (the mid-chain (G)-gate leg: 2 valid chunks then an oversize declaration -> the flat extent, not the 8 KiB valid prefix), the re-pinned over-2x sub-case of `Carve.OneChunkChainHostileVariantsSalvageAndRefuse` (the M-106 landing had pinned the divergent nothing-carves outcome in error - now pins the og's flat extent); the cursor-carried exits (broken-GUID salvage, fallback-field, exhaustion) stay green via the existing M-106 family; full suite 1353/1311/0-fail.
**Resolution notes:** One exit-value correction inside validate_one: the two size-gate breaks (both-fields-zero, size>2x-device) now set `size = dev2` before falling out, per the og's comma-operator decode (FUN_180044e30 :52335's `uVar5 = uVar1` sits between the primary-size load and the != 0 tests - re-read first-hand this session; the retdec cross-check independently shows `v5 = v2` before both breaks). The cursor-carried exits (window miss, GUID mismatch, exhaustion) keep the last-valid-chunk end exactly as before. The block comment re-truthed to the four-row exit table (the M-106 "cursor at the chain's break point" claim dropped - it was true only for the cursor-carried family). The 500M FileInfo clamp and the probe-limit device clamp unchanged.
