# S41/T9 evidence — the registry-key residue sweep (the never-filed N-026 territory)

> **S42-B merge note (2026-10-29):** the DBX finding this evidence drove was filed
> by the S41 lane as M-117 — an ID already consumed by the retired suppress_overlap
> row (the 0f5d57fb/9cc6a03e 2.5-minute race). The S42-B lane detected the collision
> and renumbered to M-118, but the parallel S42-main window FIXED the finding under
> M-117 before that renumber pushed; per SA-9 the fix-under-M-117 stands, the
> renumber was reverted, and the dual-ID is documented in FIXED/M-117's collision
> addendum. "M-117" below referring to the DBX analyser = the fixed finding; the
> suppress_overlap M-117 = the retired quadratic-walk row.

- **Task ID:** S41/T9 (session-41 continuation audit lane, the deep-comparison wave A)
- **Date:** 2026-10-29
- **Core HEAD:** `c7fa527c` · **Corpus HEAD:** `be0cfe9`
- **Method:** the s3 rawsearch-inventory's 57 MISSING-marked key rows re-verified against the
  core's current registry by a byte-level magic diff (both endiannesses, u16 and u32 forms,
  char-array magics parsed from cr_registry.cpp) — 53 keys landed, **4 remain absent**: the DBX
  triple (0x36464d4a / 0xfe12adcf / 0x2c3489f7) and the DXF "99" u16 key (0x3939). Both findings
  filed this wave (M-117, N-152). The never-cited-artifact census (23 artifacts never cited in the
  trail) drove the wave: the three RAWSearchDLL family artifacts (brance/rawspeed/xjq) were the
  entry point; their rosters were cross-checked against the enum (122 FMT_ members) and the registry.

## §1 The DBX registry bindings (RAWSearchDLL_decompiled.c :2192-2232, line-addressed)

```
2192:   local_res8 = (uint *)FUN_1800020c0(0x18);
2195:     *(undefined ***)local_res8 = IRawFileAnalyser::vftable;
2196:     local_res8[2] = 0xffffffff;
2197:     *(undefined ***)local_res8 = RawSpeed::DbxAnalyser::vftable;
2198:     local_res8[2] = 0x15;                      // format id 21 = DBX
2203:   local_res10[0] = 0x36464d4a;                 // "JMF6"
2204:   plVar3 = FUN_180006c80(param_1,local_res10); // map[key] = node
2206:   local_res8 = (uint *)FUN_1800020c0(0x18);
2211:     *(undefined ***)local_res8 = RawSpeed::DbxAnalyser::vftable;
2212:     local_res8[2] = 0x15;
2217:   local_res10[0] = 0xfe12adcf;                 // CF AD 12 FE — the OE DBX magic
2218:   plVar3 = FUN_180006c80(param_1,local_res10);
2220:   local_res8 = (uint *)FUN_1800020c0(0x18);
2225:     *(undefined ***)local_res8 = RawSpeed::DbxAnalyser::vftable;
2226:     local_res8[2] = 0x15;
2231:   local_res10[0] = 0x2c3489f7;                 // F7 89 34 2C — the marked variant
2232:   plVar3 = FUN_180006c80(param_1,local_res10);
```

Three consecutive nodes, one class, three magic keys. (The next node at :2234-2246 is the
FileZIP+ISpecial node under PK 0x4034b50 — the window boundary that confused the first read.)

## §2 The DBX probe / FileInfo / size pass

**FUN_180016740 (:17509-17562) — the format-id probe's identity table:**

```
local_70 = 0x36464d4a;  uStack_6c = 0x100003;         // "JMF6" + 03 00 01 00
local_58 = 0xfe12adcf;  uStack_54 = 0x6f74fdc5;       // OE GUID family, gen 1
local_50 = 0x11d1e366;  uStack_4c = 0xc0004e9a;
local_48 = 0xfe12adcf;  uStack_54' = 0x6f74fdc6;      // gen 2
local_38 = 0xfe12adcf;  ... 0x6f74fdc7;               // gen 3
local_28 = 0xfe12adcf;  uStack_24 = 0x26fe9d30;       // the fourth GUID variant
         local_20 = 0x11d21a8f;  uStack_1c = 0x6000bfaa;
local_88 = 0x2c3489f7;  uStack_84 = 0x6deef0f7;       // the marked 16-byte pattern
         local_80 = 0xd18922af;  uStack_7c = 0xb0f0ff00;
...
if ((*param_2 == 0x10000336464d4a) ||
   ((*param_2 == 0x6deef0f72c3489f7 && (param_2[1] == -0x4f0f00ff2e76dd51))))
LAB_18001684f:  *(undefined4 *)(param_3 + 0x18) = 0x15;   // stamp id 21
```

(The GUID loop iterates the 4 variants at 8-byte stride — `*param_2 == GUID.lo && param_2[1] == GUID.hi`.)

**FUN_180016880 (:17586-17600) — DBXRawFileInfo ctor:** 0x120-byte object, id 0x15, ctor slots
0x1b=0xe / 0x22=0x14 / u16@+0x23=0x100 (base-CRawFileInfo fields; full decode = fix-plan step 1).

**FUN_180016950 (:17622-17690) — the size/name pass:**

```
if (*(char *)(param_1 + 0x119) != '\0') {
    if ((*param_2 == 0x6deef0f72c3489f7) && (param_2[1] == -0x4f0f00ff2e76dd51))
      *(undefined1 *)(param_1 + 0x118) = 1;          // the marked flag
    *(undefined1 *)(param_1 + 0x119) = 0;
}
if (*(char *)(param_1 + 0x118) == '\0') {
    *(ulonglong *)(param_1 + 0x10) = (ulonglong)*(uint *)((longlong)param_2 + 0x7c);
    // PLAIN FORM: carve size = u32 at +0x7C (the DBX self-declared size)
}
else if (8 < *(int *)(param_5 + 0x14)) {
    iVar2 = FUN_180042b30(param_2,param_3);          // the marked-form block computation
    memset(local_158,0,0x104);                        // 260-byte name buffer (+NUL = 272 slot)
    ... strlen walk, assign into param_1+0x20 (the name field) ...
    *(longlong *)(param_1 + 0x10) =
         (param_4 - *(longlong *)(param_1 + 8)) * (longlong)param_3 + (longlong)iVar2;
}
```

## §3 The byte-level registry diff (the 57 MISSING rows vs the current core)

Script: `scripts/s41_missing_keys.py` (this lane's scratch; logic recorded here for reproduction —
parse `cr_registry.cpp`'s `{FMT_X, "ext", {magic...}}` rows into byte sequences; for each
MISSING-marked inventory row, test the key in LE/BE at u16/u32 width plus the magic column's
literal form). Result: 53 of 57 present (CRW II/MM, X3F, XML/SVG variants, OGG, RM, MXF, FH11,
AI/EPS, DWG, GIF, MIDI, RTF, PSP, SIT, ONE, WPG, WPD, WMF both, TTF, OLE, LowDoc both, ICO, CUR,
HLP, FORM/AIFF, CPT7/8/9, mscf, AU, CHM, MRM, %!, 1F 8B, MDB 0x0100, HTML family, MM 3DS, BM,
MS MNY, BZh1, MPEG both, DXF "  0\r", APE — all landed across S-007/S-037/M-081 and the
subsequent waves). **Absent: 0x36464d4a, 0xfe12adcf, 0x2c3489f7 (the DBX triple → M-117) and
0x3939 (the DXF "99" key → N-152).**

The og's `raw_signs.json` (139 extensions, parsed with json this session) contains no dbx/jmf/
outlook entries — the JSON layer does not backstop the analyser's absence (in either implementation).

## §4 The DXF "99" binding and probe (RAWSearchDLL_decompiled.c)

**Registry (:2722-2733, the u16 map — FUN_18000ac20 insert):**

```
2722:   puVar1 = (undefined8 *)FUN_1800020c0(0x18);
2727:     *puVar1 = BruteForce::CDXFAnalyser::vftable;
2729:     *(undefined4 *)(puVar1 + 1) = 0x19;         // format id 25 = DXF
2732:   local_60[0] = 0x3939;                          // u16 key "99"
2733:   plVar2 = (longlong *)FUN_18000ac20(param_1,local_60);  // u16 map insert
```

**Probe FUN_1800376f0 (:42240-42290):**

```
local_50 = 0xd302020;      uStack_4c = 0x4345530a;   // "  0\r" + "\nSEC"
local_48 = 0x4e4f4954;     uStack_44 = 0x20200a0d;   // "TION" + "  \r\n"
if ((*param_2 == 0x4345530a0d302020) && (param_2[1] == 0x20200a0d4e4f4954))
    goto stamp_0x19;                                   // the canonical 16-byte pattern
else if (((short)*param_2 == 0x3939) && (*(char *)((longlong)param_2 + 2) == '9')) {
    // "999" comment header
    if (*(char *)((longlong)param_2 + 3) == '\n') { _Buf2 = &local_40; _Size = 0xe; }
    else {
        if ((*(char *)((longlong)param_2 + 3) != '\r') ||
            (*(char *)((longlong)param_2 + 4) != '\n')) goto no_match;
        _Size = 0x10;
    }
    // local_40 = 0x0a302020 ("  0\n"), local_3c = 0x54434553 ("SECT"),
    // local_38 = 0xa4e4f49 ("ION\n"), local_34 = 0x2020 ("  ") — the LF-tolerant shifted pattern
```

## §5 The never-cited-artifact census (the wave's entry point)

23 corpus artifacts carry zero citations anywhere in diff-to-fix/ + docs/ + worklog (census script:
basename grep across the audit surface). The three RAWSearchDLL family artifacts
(`rawsearchdll--family-brance.md` / `-rawspeed.md` / `-xjq.md`) led to the roster cross-check; the
never-cited list is recorded here for the next lanes (the disposition of the remaining 20 —
runtime-families, UIUX chrome, Languages, Licensing — rides I-034):

```
Languages/language-packs--family-census.md            Licensing/euinapp--ie-bootstrapper.md
PartitionRecovery/burn--imapi2-disc-writer.md         PartitionRecovery/eudcpepm-euedkepm--hook-framework-urgent-cache.md
SearchDLL/rawsearchdll--family-brance.md              SearchDLL/rawsearchdll--family-rawspeed.md
SearchDLL/rawsearchdll--family-xjq.md                 UIUX/14-cdetailpage-slots-viewmodes.md
UIUX/16-shell-internals-ai-layer.md                   UIUX/22-rtti-class-census.md
UIUX/23-euinapp-ie-emulation-bootstrapper.md          UIUX/24-dynamic-ground-truth-ingestion.md
UIUX/32-repair-item-rows-decode.md                    UIUX/33-objectname-loop-closure.md
UIUX/34-navigatebar-bottomarea-chrome.md              UIUX/dynamic-ground-truth--splash-captured.md
UIUX/dynamic-ground-truth--window-census-verification.md
runtime-families/{d3d-graphics-shims,imagemagick-modules,misc-windows-shims,network-libraries,qt5-core-stack,qt5-quick-qml-stack}.md
```

The Brance/RawSpeed/xjq roster cross-check against the enum: every member either landed, is an
active finding, or rides M-107's stand-in residue — EXCEPT Dbx (→ M-117). The DXF-999 key came
from the s3 key-diff, cross-checked against the u16 dual-registry table.
