# Evidence — S-041 batch-2 decode resolutions + absence sweep (session 8)

Corpus `be0cfe9`; core `35adcb9`. All decompile reads performed or re-verified
first-hand this session; the raw-binary disassembly ground truth for CHM and the
key-284 registration was produced with objdump against `og-codebase/RAWSearchDLL.dll`.

## A. CHM — the GUID-compare "conflict" resolved (retdec print artifact)

**FileCHMAnalyser::Analyse** = `FUN_180016b40`. Ghidra:
`re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:17705-17740`;
retdec: `ci-multitool/searchdll/RAWSearchDLL/retdec_decompiled.c:22633-22643`
(the ":22635 region" the trail flagged). Registration: key `0x46535449` ("ITSF")
→ FileCHMAnalyser node idx `0x0E` (ghidra `:2488-2499`; census `:97`).

**objdump ground truth (VA 0x180016b40-0x180016c14, quoted from the session's
disassembly pass):**

```asm
180016b57: cmp DWORD PTR [rdx],0x46535449        ; 'ITSF' @ +0x00
180016b5d-8e: (materialises 8 GUID dwords on stack:)
        [rbp-0x30]=0x7c01fd10 [rbp-0x2c]=0x11d07baa [rbp-0x28]=0xa0000c9e [rbp-0x24]=0xece622c9
        [rbp-0x20]=0x7c01fd11 [rbp-0x1c]=0x11d07baa [rbp-0x18]=0xa0000c9e [rbp-0x14]=0xece622c9
180016b97: cmp DWORD PTR [rdx+0x8],0x60          ; hdr-length field == 0x60
180016b9d: mov rax,QWORD PTR [rdx+0x18]
180016ba1: cmp rax,QWORD PTR [rbp-0x30]          ; == 0x11D07BAA7C01FD10
180016ba7: mov rax,QWORD PTR [rdx+0x20]
180016bab: cmp rax,QWORD PTR [rbp-0x28]          ; == 0xECE622C9A0000C9E
180016bb1: mov rax,QWORD PTR [rdx+0x28]
180016bb5: cmp rax,QWORD PTR [rbp-0x20]          ; == 0x11D07BAA7C01FD11
180016bbb: mov rax,QWORD PTR [rdx+0x30]
180016bbf: cmp rax,QWORD PTR [rbp-0x18]          ; == 0xECE622C9A0000C9E
180016bc5: cmp DWORD PTR [rdx+0x60],0x1fe        ; size-chunk magic
180016bce: cmp DWORD PTR [rdx+0x70],0x0
180016bd4: cmp DWORD PTR [rdx+0x74],0x0
180016bda: mov DWORD PTR [r8+0x18],0xe           ; result->type = 14
180016be2: mov rax,QWORD PTR [rdx+0x68]          ; size = u64 @ +0x68
180016be6: mov QWORD PTR [r8+0x10],rax           ; result->size (raw copy)
180016bea: mov eax,0x1                           ; accept
```

The two GUIDs are **canonical ITSF v3** (little-endian at +0x18 and +0x28):
- GUID1 = bytes `10 FD 01 7C AA 7B D0 11 9E 0C 00 A0 C9 22 E6 EC` = {7C01FD10-7BAA-11D0-9E0C-00A0C922E6EC}
- GUID2 = bytes `11 FD 01 7C AA 7B D0 11 9E 0C 00 A0 C9 22 E6 EC` = {7C01FD11-7BAA-11D0-9E0C-00A0C922E6EC}

The compares are genuine 64-bit `mov rax,[mem]; cmp rax,[stack-const]` pairs. Ghidra's
full-constant rendering is byte-exact correct; **retdec's `0x7c01fd10` / `-0x5ffff362`
at :22635 are truncated print artifacts** (sign-extended low dwords of the qwords) —
the artifact, not the code, created the batch-2 blocker. The trail's claimed GUIDs
(`799C39D5`, `D0ED6BCE`) have **zero occurrences** in the binary (full .text sweep,
both decompiles). No min/max bounds; no version/timestamp/langid checks; size u64@+0x68
copied verbatim.

## B. Key 284 (0x11C) = FreeHand 11, not MPEG-TS

Registration site `FUN_180002a50`: ghidra `:1951-1962`, retdec `:5654-5665`,
asm `0x1800035a1-0x1800035ea`:

```asm
1800035c1: lea rax,[rip+0x68238]   # 0x18006b800   ; RTTI .?AVCFH11Analyser@BruteForce@@
1800035c8: mov QWORD PTR [rbx],rax
1800035cb: mov DWORD PTR [rbx+0x8],0x1d           ; format id 29
1800035d7: mov DWORD PTR [rbp+0x48],0x11c         ; key = 284
1800035e5: call 0x180006c80                       ; std::map::operator[]
1800035ea: mov QWORD PTR [rax],rbx                ; map[284] = CFH11Analyser node
```

`CFH11Analyser::Analyse` (`FUN_180029f30`, ghidra `:32612-32634`): u64@+4 ==
0x02001401_1C040002, u32@+0xC == 0x011C1400, u16@+0x22 == 0x3131 ("11");
type-only accept (no size). Grammar = Macromedia FreeHand 11 (`.fh11`).

MPEG-TS: `CMTSAnalyser` (idx 0x76) and `CM2TSAnalyser` (idx 0x74) are constructed
and pushed to the brute-force list (`0x1800053ae-0x180005488`) with **no magic-key
map entry** — list-layer frame-sync walkers, the TAR posture. Census `:99`'s
"(ICO sizes, MTS/ADTS sync)" gloss on the 0x11c row is a misattribution →
corpus erratum **E53** (filed this session). Several census chain attributions in
the numeric-key window are shifted by one slot; the construct→insert re-verification
rule (validated 3-for-3 on EBML→MKV, ITSF→CHM, MRM→MRW) additionally pinned
`map["SQLi" 0x694C5153] = CFCPEvtAnalyser` — FCPEvt keys on "SQLite format 3\0"
(15-byte memcmp at `0x180067eb0`, page-size/version gates at `FUN_18002e2b0`).

## C. Census roster rows for the absent set (verbatim)

`re-workspace/artifacts/SearchDLL/rawsearchdll--analyser-registry-census.md`:

- `:59` — `| `<?xm` / UTF8-BOM`<` / `<svg` | 0x6d783f3c, 0x3cbfbbef, 0x6776733c | XML (+BOM variant) / SVG | HTML/SVG chain |`
- `:78` — `| `E4 52 5C 7B` | 0x7b5c52e4 | **LZH arc** | LZH chain |`
- `:97` — `| `ITSF` | 0x46535449 | **CHM** (InfoTech Storage File) | FileCHMAnalyser |`
- `:105` — `@BruteForce` namespace — 24 classes: … CFCPEvt, CFH11, … CMTS/CM2TS (via the MTS/M2TS gloss at :99) …
- `:107` — `@RawSpeed` namespace — 20 classes: … HTML, New3DS (Nintendo 3DS!), … One (OneNote!), …
- `:109` — `@RawFormat_xjq` namespace — 8 classes: … JP2, M1V, MOBI …
- `:111` — `@Brance` namespace — 22 classes: CRW (Canon RAW), … LZH, … SVG, X3F, OfficeTagFromXML …
- `:113` — First-generation `File*Analyser` family — 46 classes: … FileSWF, … FileEMF, … FileMNY (MS Money), … File3DS, … FileONE, FileHTM, FileLZH, FileFCPEVENT (Final Cut Pro!), FileCRW …
- `:115` — Standalone classes: AEPAnalyser, CHFPAnalyser, CHIKAnalyser, COSPAnalyser, CPOSAnalyser, CGPXAnalyser, CVCFAnalyser …

## D. Core absence sweep (patterns → results)

Swept `src/core/carve/` + `include/cr/carve/` (+ full-tree sanity on key magics), core `35adcb9`:

| Searched | Patterns | Result |
|---|---|---|
| CHM/ITSF | `\bCHM\b`, `ITSF` | 0 hits |
| MNY | `MNY` | 0 |
| EMF | `\bEMF\b` | 0 |
| ONE / E4 52 5C 7B | `0x7b5c52e4`, byte-pattern regex, onenote | 0 |
| MPEG-TS / 284 / 0x11C | `mpegs?[-_ ]?ts`, `\bMTS\b`, `\b284\b`, `0x11c`, `m2ts\|transport stream` | 0 as MPEG-TS (the only 0x11C usage is FMT_FH11, key bytes 1C 01 00 00, cr_registry.cpp:641-650 — S-037's landing; a key-collision nuance, not an MPEG-TS analyser) |
| 3DS | `\b3[dD][sS]\b`, `new3ds` | 0 |
| SWF / FWS / CWS | `\bswf\b`, `FWS`, `CWS` | 0 |
| JP2 / jp2c | `\bjp2\b`, `jp2c` | 0 |
| MOBI / BOOKMOBI | `\bmobi\b`, `bookmobi` | 0 |
| LZH / -lh | `\blzh\b`, `-lh[0-5]` | 0 |
| HTML carve | `\bhtml\b`, `<html`, `FileHTM` | 0 |
| SVG | `\bsvg\b`, `<svg` | 0 |
| FCPEvt | `fcpevt\|fcpevent\|final cut` | keyed row absent; only the N-089 SQLite subtype relabel note ("page-math+fcpevent", cr_analysers.cpp:1202-1247, cr_carve.cpp:434) |
| OfficeTagFromXML | `officetag\|office_tag` | 0 |
| CRW | `\bcrw\b`, `HEAPCCDR` | 0 |
| VCF / GPX | `\bvcf\b`, `vcard`, `\bgpx\b`, `<gpx` | 0 |

FormatId enum (`include/cr/carve/cr_signatures.h:23-173`): 90 ids including
FMT_COUNT — none for any format above. Full-tree sanity `ITSF|BOOKMOBI|jp2c|OfficeTag`
across src/include/tests: 0 hits.
