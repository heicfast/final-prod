# Evidence: MID/GZ og ctor bounds vs our registry caps (N-093 — merged at the push-time reconciliation)

**Corpus:** `easeus-drw-research` @ `be0cfe9`
**Source:** `re-workspace/reversed/ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c`
**Verified:** first-hand by the session-9 lead (sed reads at the cited lines).

## MID (FileMIDAnalyser, format id 38 = 0x26) — ctor at RC:21110-21127

```c
// Address range: 0x180014ea0 - 0x180014f1a
int64_t function_180014ea0(void) {
    int64_t result = function_1800020c0(288, v1, v1, v1);
    if (result == 0) { return 0; }
    int64_t * v2 = (int64_t *)result;
    function_18000b520(v2, v1, v1, v1);
    *v2 = (int64_t)&g54;
    *(int64_t *)(result + 224) = 0;
    *(int64_t *)(result + 216) = 14;          // min = 14
    *(int64_t *)(result + 272) = 14;          // min (second slot)
    *(int64_t *)(result + 280) = 0x989680;    // max = 10,000,000 (10 MB)
    *(int32_t *)(result + 24) = 38;           // format id
    return result;
}
```

## GZ (FileGZAnalyser, format id 31 = 0x1F) — ctor at RC:20650-20666

```c
// Address range: 0x1800143a0 - 0x180014400
int64_t function_1800143a0(void) {
    int64_t result = function_1800020c0(280, v1, v1, v1);
    if (result == 0) { return 0; }
    int64_t * v2 = (int64_t *)result;
    function_18000b520(v2, v1, v1, v1);
    *v2 = (int64_t)&g49;
    *(int64_t *)(result + 224) = 0;
    *(int64_t *)(result + 216) = 20;           // min = 20
    *(int64_t *)(result + 272) = 0x17d78400;   // max = 400,000,000 (400 MB)
    *(int32_t *)(result + 24) = 31;            // format id
    return result;
}
```

(Note the two ctor layouts: the MID-family analyser stores min at both +216
and +272 with max at +280; the GZ-family stores min at +216 and max at +272.
Both patterns were cross-checked against the MDB/WPG ctors by the D-agent —
different analyser subfamilies carry different FileInfo member offsets; the
decoded min/max VALUES are what matter here.)

## Our registry rows (core @ 35adcb9, `src/core/carve/cr_registry.cpp`)

```cpp
691:    {FMT_MID, "mid", {'M', 'T', 'h', 'd'}, 4, 0, 14, 64ull << 20, 800, validate_mid, "MIDI"},
781:    {FMT_GZ,  "gz",  {0x1F, 0x8B},         2, 0, 18, 512ull << 20, 600, validate_gz, "gzip"},
```

## The deltas

| Format | Field | Og | Ours | Ratio/effect |
|---|---|---|---|---|
| MID | min | 14 | 14 | match |
| MID | max | 0x989680 = 10,000,000 (10 MB) | 64 MiB (67,108,864) | 6.7x larger cap |
| GZ | min | 20 | 18 | ours accepts 18-19-byte headers the og rejects |
| GZ | max | 0x17D78400 = 400,000,000 (400 MB) | 512 MiB (536,870,912) | 1.34x larger cap |
