# MAJOR M-139 — The HIK discriminator start-code byte is wrong: the og probes 00 00 01 BC (private_stream_2); the core probes 00 00 01 BD (the M-098 landing's transcription error, in the finding's own title)

**Severity:** MAJOR
**Module:** carve (HIK / Hikvision DVR video analyser)
**Parameters affected:** A1 (detection correctness — the HIK class keys on the wrong stream id), B1 (false-positive rate — BD-led streams falsely HIK-stamped), B19 (magic bytes / signature constants), A25 (the probe's grammar)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c` FUN_1800272a0 :30412-30484 (read first-hand this session: the discriminator constant `-0x43ff0000` at BOTH probe sites :30438 and :30452; the id stamp 0x92; the name prefix DAT_18006b428 = UTF-16LE "HIK" = `48 00 49 00 4B 00`, byte-dumped from the og binary); cross-checked `re-workspace/reversed/ci-multitool/RAWSearchDLL/retdec_decompiled.c` function_1800272a0 (the same -0x43ff0000 twice; `-0x42ff0000` appears nowhere in either decompile — grep-verified)
**Core references:** audited `34d6cf10` (HEAD at filing) — `src/core/carve/cr_analysers.cpp` validate_hik :6072-6136 (the skip-end probe `bd[3] != 0xBD` at :6115, the direct-path probe `sc[3] == 0xBD` at :6118; the block comment :6072-6075 asserting "00 00 01 BD ... the private_stream_1 PES" at both sites); landed by c0efacc3 (M-098+N-156)
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The M-098 landing ported the og's HIK probe with one transcribed byte: the
og discriminates the HIK PES by the start code **00 00 01 BC**
(private_stream_2, the constant -0x43ff0000 = 0xBC010000 in LE bytes), but
the core probes **00 00 01 BD** (private_stream_1). Every real og-HIK
candidate (BC-led) is rejected by the core's HIK analyser and falls to the
generic MPEG-PS grammar (different id, different name, a different walk);
every BD-led stream the core stamps "HIK" is a file the og would never
HIK-stamp. The landed finding's own title ("HIK private_stream_1
discriminator") carries the error.

## What the reference does

FUN_1800272a0 (the full quotes in
`evidence/decompiled-quotes/s50-landing-divergences.md` §2):

```c
if (*param_2 == -0x45ff0000) {                    /* 00 00 01 BA - the pack */
  bVar7 = (*(byte *)((longlong)param_2 + 0xd) & 7) + 0xe;    /* pack length */
  ...
  if (*(int *)(uVar6 + (longlong)param_2) == -0x44ff0000) { /* 00 00 01 BB - system header */
    ...
    if (*(int *)((ulonglong)uVar5 + (longlong)param_2) == -0x43ff0000) {
      /* -0x43ff0000 as u32 = 0xBC010000 = LE bytes 00 00 01 BC */
      ...                                        /* the BE-u16 PES length walk */
      *(undefined4 *)(param_3 + 0x18) = 0x92;    /* the HIK format id      */
      ... FUN_18000f4b0(..., &DAT_18006b428);    /* name prefix "HIK"     */
```

The direct path (the else-branch at :30452) uses the SAME -0x43ff0000
constant. Constant arithmetic: 00 00 01 BD would be 0xBD010000 =
**-0x42ff0000**, which appears nowhere in the decompile. The landing's own
BA/BB decodes use this exact arithmetic (its "-0x45ff0000 = BA" and
"-0x44ff0000 = BB" are correct), which is what makes the BD reading of
-0x43ff0000 a transcription error rather than a different-decompile
disagreement: retdec independently shows -0x43ff0000 twice.

## What our core does

`validate_hik` (cr_analysers.cpp:6072-6136 at the audited tree):

```cpp
u8 bd[6];
if (!rd(s, hit + p, bd, sizeof(bd)))
    return r;
if (bd[0] != 0 || bd[1] != 0 || bd[2] != 1 || bd[3] != 0xBD)
    return r; /* private_stream_1 required at the skip end */      <- :6115
...
} else if (sc[0] == 0 && sc[1] == 0 && sc[2] == 1 && sc[3] == 0xBD) {
    pos = pack_len; /* the direct private_stream_1 after the pack */ <- :6118
```

The block comment (:6072-6075) teaches "00 00 01 BD - the private_stream_1
PES" for both paths. Everything else in the landing is parity (verified
this session: pack length (buf[0xD]&7)+0xE, the LE-u32 BB skip with the
<0x40000001/<0x400001 bounds, the BE-u16 PES length +6, the structural
walk with the 0x1A advance and the 0x40000000 cap, the L"HIK" prefix, the
row precedence FMT_HIK before FMT_MPEGPS).

## Gap

Falsifiable: on a Hikvision stream whose post-pack (or post-system-header)
PES start code is 00 00 01 BC, the og stamps id 0x92 with the "HIK" prefix;
the core's validate_hik returns not-ok. On a stream with 00 00 01 BD there,
the core stamps HIK where the og's probe declines (falls through to its
other grammars).

## Impact

The HIK population (CCTV/DVR forensic recovery — a named use case of the
og's analyser set): every genuine HIK file loses its format classification,
its "HIK" name prefix and its PES-length-based extent in the core (recovered
only if the generic MPEG-PS grammar happens to catch it, under a different
id/name/walk); conversely the core emits false HIK classifications for
BD-led streams the og does not treat as HIK. Both directions are
user-observable in the results tree (format column, names, and extents).
Content is largely preserved via the generic MPEG-PS grammar — which is why
this is MAJOR (classification/naming/extent attribution) and not ACUTE/SEVERE
(mirrors the M-081 CRW/MRW key-attribution precedent).

## Reproduction / verification

Decompiled-evidence class, both sides read first-hand this session:
`evidence/decompiled-quotes/s50-landing-divergences.md` §2 (the og quotes
with line numbers + the retdec cross-check + the constant arithmetic), the
sweep record `evidence/executions/s50-landing-parity-sweep.md` row 6 (the
DIVERGENCE verdict on the M-098 landing; the N-156 half verified PARITY).

## Fix plan (for the engineering team)

1. Architectural: none — two byte constants + the comment.
2. Files: `src/core/carve/cr_analysers.cpp` (validate_hik), the block
   comment; the test fixtures.
3. Algorithm/constants: change both probes from `0xBD` to `0xBC` (the
   skip-end `bd[3] != 0xBC` and the direct-path `sc[3] == 0xBC`); re-truth
   the comment ("00 00 01 BC - the private_stream_2 PES" at both sites);
   fix the landed finding's title reference in any doc that quotes it.
4. Tests (`tests/unit/test_carve.cpp`, the M-098 HIK family):
   - `Carve.HikDiscriminatorIsPrivateStream2` — a BC-led pack+PES fixture
     accepted, a BD-led twin rejected, both RED before the fix.
   - `Carve.HikDirectPathAcceptsBcOnly` — the direct-path form.
   - keep/adjust the existing HIK fixtures to the BC form.
5. Contract/doc: the carve.md module row + the FIXED/M-098 Resolution gain
   the erratum note (the title's "private_stream_1" is the transcription
   error; the decode is -0x43ff0000 = BC).
6. Acceptance: the two new tests green; the HIK fixtures carv as HIK; no
   other MPEG-PS-family test regresses (the generic PS grammar is untouched).

## Tests to add

The two named tests above, both RED before by construction (the current
code probes BD).

## Related findings

M-098 (FIXED — the landing this rode), M-081 (the CRW/MRW key-attribution
precedent), S-049 (the naming contract the "HIK" prefix participates in).

## Open questions

None — two decompilers and the constant arithmetic agree.
