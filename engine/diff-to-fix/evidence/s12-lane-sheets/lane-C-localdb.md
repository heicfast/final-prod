# LANE C — LOCAL DATABASE / EUUserData FAMILY / USER-SIGNATURE SYSTEM / PER-SCAN STATISTICS
Deep-dive audit (read-only). Core = `/home/z/audit/core_recovery` (8d0d88b8 content). Corpus = `/home/z/audit/easeus-drw-research` (be0cfe9).
Scratch file: this document. Every cite below was read first-hand this session unless explicitly marked "(registered)".

---

## 0. ERRATUM VERIFICATION (done first, as instructed)

Corpus erratum CORRECTIONS.md #9 (= core CORPUS-ERRATA E-A/E-B, echoed in PARITY-DELTAS D-104 and
SCAN-ENGINE-DELTA F2/OQ-SA-3) claims: (a) EUUserData's sole consumer is DRWUI's CHKRepairFileSign,
never the RAW carving engine; (b) GetRawUserInfoMap = per-scan carve-statistics map (insert-or-
increment + searcher-init clear + DRWUI post-scan report reader). **VERIFIED FIRST-HAND, BOTH
DIRECTIONS — the erratum is CORRECT and no errata-erratum exists against it** (checked
diff-to-fix/CORPUS-ERRATA.md E-A/E-B/E-S11#4 rows and all later sessions' rows; nothing overturns
them; my independent decodes agree on every point, with several new details below):

- `rg EUUserData` over RAWSearchDLL_decompiled.c (80,615 lines): **0 hits**. RAWSearchDLL's only
  runtime-loaded sign data: `raw_signs.json` (ifstream, :23836) + `config/RawConfig.ini`
  (GetPrivateProfileSectionA "Filter", :13133-13137).
- Whole-og-codebase binary string scan (ASCII + UTF-16LE): `EUUserData` appears ONLY in DRWUI.exe
  (1 ASCII hit, the `%s/EUUserData` format) and unins000.dat (installer file-list). **No writer
  anywhere; no other reader.**
- DRWUI :1005387 builds `%s/EUUserData`, `_wfopen_s(..., "r")` at :1005412 (mode string verified at
  DRWUI.exe 0x1418922f8 = "r") — **read-only text mode**.
- GetRawUserInfoMap (:72213-72221) returns `FUN_180001350() + 0x90`; the singleton getter
  (:65-100) initializes +0x90 via a map ctor and +0x78 with ids 1..169 — matches E-S11 #4's layout.
- FUN_18000ed00 (:11376-11531): insert-or-increment on a `std::map<std::wstring, int64>`
  (existing key → `plVar10[8] = plVar10[8]+1` :11466; new key → value 1 :11525-11526). 36 direct
  E8 call sites in the binary (decompile shows ~60 call lines); callers pass
  `FUN_180001350()+0x60` so the map lands at singleton+0x90 (e.g. :11594 RIFF/AEP analyser,
  :34064 MP4-family). **Key = format NAME wstring** (uppercase ext-shaped, e.g. L"AEP", L"ZIP",
  L"MDB" — hardcoded per analyser; the id→name table at RAWSearchDLL.dll .data 0x18008f070,
  9-byte stride, 169 names, 0x9c="AEP", 0x27="MDB", 0x48="ZIP"), **value = detection count**.
- FUN_18000beb0 = `CRAWSearchController` vtable slot +0x68 (RTTI verified via binary: vftable
  0x180066e38, slot 0x180066ea0; class `.?AVCRAWSearchController@@`): resets singleton +0x60=1,
  +0x70=200000000, +0x68=0 and **clears the +0x90 map** (:9047-9051, `FUN_180001500` = map
  erase-all) at every searcher init → per-scan lifetime.
- DRWUI report reader = FUN_1401227e0 (:227457; body :227524-227587): GetModuleFileNameA +
  _splitpath → LoadLibraryA("<exedir>RAWSearchDLL.dll") :227528-532, GetProcAddress
  ("GetRawUserInfoMap") :227538, call :227543, iterate map, format each value `%lld`
  (:227561, node+0x40 = the count), build the `Trigger_Raw` → … → `FileSystem` report tree
  (:227573-584). Called from the scan-completion UI path (bottomarea.cpp:0x358 handler :1098863-
  1098891 and two more sites :1313115/:1320974) — post-scan, one-way, live memory.

---

## 1. CONTEXT SHEET

### 1.1 The og's EUUserData — full producer/consumer map

**File** (`og-codebase/EUUserData`, 16,891 B, CRLF, pretty-printed): 111 parsed ext keys, **112 raw
ext blocks / 191 raw sign strings** (the `asf` key appears TWICE — alphabetically with mime
`application/vnd.ms-asf`, and again at file end with mime `asf`; both carry the same 1 sign
`0,3026B2758E66CF11A6D900AA0062CE6C`). Measured extremes: max offset 526 (zip `526,504B537058`),
max sign 44 B (img), max 11 signs/entry (mpeg), longest ext 8 (`manifest`), longest mime 73 (pptx
OOXML). 26 signs carry non-zero offsets (0..526, DECIMAL); 12 signs contain LOWERCASE hex digits
(mpeg ×11, img ×1 — see LC-3). No negative offsets, no odd hex lengths, no empty signs arrays.

**Producer**: none at runtime. The file ships with the install; the only file operation on it in
any binary is DRWUI's `fopen(path, "r")`. The corpus artifact's claim "the UI's custom-format flow
feeds this JSON" (config-files/userdata-remoteconfig §1) has **no binary support** (no writer
exists). Hand-editing the install-tree file is the only extension path.

**Consumer chain (DRWUI, all first-hand)**:
1. Static init: FUN_140003cf0 (:846-852) constructs the process singleton `DAT_1425e7fc8` via
   FUN_140602e40 — **once per process**. (A reload entry point FUN_140606a90 :1005311 =
   load+parse-again exists but has no in-decompile caller — see OQ-4.)
2. FUN_140602e40 (:1002658-1002757) = CHKRepairFileSign ctor (vftable :1002677; RTTI
   `.?AVCHKRepairFileSign@@`, vtable at 0x141899958 = {FUN_140603040 dtor, **FUN_140603820 Match**,
   FUN_140604210 query}, binary-verified): allocates a 0x40-B DB object {+0x00 std::string
   (file contents), +0x20 map<offset,…> (by-offset), +0x30 map<ext,…> (by-ext)}, then load
   (FUN_140606ab0 :1005324, the `%s/EUUserData` fopen "r" + fseek/ftell/fread :1005387-1005487)
   → parse (FUN_140606e80 :1005497) → per-ext-object FUN_1406072a0 (:1005687) → per-sign
   FUN_140607930 (:1005936). The ctor finally scans the by-offset map and stores the MAX OFFSET
   at this+0x08 (:1002712-1002754).
3. Parse semantics (FUN_1406072a0 + FUN_140607930):
   - member iteration over ext objects; only `_stricmp(name, "signs") == 0` acts
     (:1005818) — **"mime" and every other member are IGNORED** (the og never reads mime);
     member-name compare is case-insensitive.
   - "signs" value must be an array (type 6, :1005825); elements must be strings (type 4,
     :1005844); each split by **","** (DAT_14187f320, binary-verified) via FUN_140605d30;
     **exactly 2 tokens required** — otherwise the sign is SILENTLY SKIPPED (:1005986
     `if (tokens != 2) goto cleanup`); offset = **`atoi` (DECIMAL)** :1005991; the hex token kept
     as TEXT.
   - insert into by-offset map (key = offset int, value = set of {hex-text, ext}) and by-ext map
     (key = ext string) — insert-or-append (:1005994-1006108).
4. Match = virtual slot +0x08 = FUN_140603820 (:1003137-3259): if an expected-ext string is
   supplied (from the file's own ext through an alias map, FUN_14044a510 :721402; the by-ext
   lookup FUN_140603280 :1002847 also splits expected ext on ';'), the walk is restricted to that
   ext's offsets; main loop walks ALL by-offset nodes (skipping offset==0), tail pass re-checks
   the offset==0 node (:1003225-3256). Per sign (FUN_140603aa0 :1003264-3525):
   - **bounds guard: `filesize < offset + hexSTRlen` → skip** (:1003371; standalone variant
     FUN_140603f70 :1003545) — i.e. the og demands offset + 2×signbytes of file, DOUBLE the
     needed length (see LC-5);
   - reads hexlen/2 bytes at buffer+offset and reformats them as **UPPERCASE hex text**
     (FUN_140604090 :1003584-100650, `sprintf_s(L"%02X")`; the only wide %02X in DRWUI.exe is at
     file off 0x165bfba, zero %02x) then **memcmp against the stored sign TEXT** (:1003452,
     :1003555) → **case-sensitive: lowercase-hex signs never match** (LC-3);
   - on match: writes the sign's ext into the out string, then invokes the CHKRepairAdapterMgr
     (this+0x18) first adapter (vtable+0) with (buffer, size, offset) :1003384-3417.
5. File-level flow: FUN_14044a8b0 (:721520-721667) — the "*****MatchedFile:%s,FileSign:%s" /
   "xxxxxxxxxx NotMatchedFile:%s,FileSign:%s" checker (:721627/:721651): identified-ext vs the
   file's actual ext (`_stricmp` :721620, alias second-chance FUN_1404f80f0 :721645); matched →
   state updates + counter (param_1+0x1f8) + FUN_14044a720 (:721469, size/adapter pipeline).
   FUN_140451bd0 (:726632-726716) on identification: counters, and if the new name < 0xFFA chars
   builds **"%s.%s"** (or `"%s_%s.%s"` with a prefix) and **RENAMES the .chk file to the
   identified extension** (:726699-726708, FUN_1400890a0) — `.chk` literal verified at
   DRWUI.exe 0x141553e40. This is the "CHK repair" feature: raw-carved unknown `.chk` fragments
   are IDENTIFIED (and renamed) from EUUserData signs.
6. Adapter registry (FUN_14060bb40 :1008873+): CHKTSAnalyze keyed "47" (:1008918),
   CHKPERepair keyed "4D5A" (:1008968), CHKOffice504B0304 keyed "504B0304" (:1009019, +300
   field), CHKOfficeD0CF11E0A1B11AE1 keyed "D0CF11E0A1B11AE1" (:1009069), CHKTifAnalyze under
   several keys (:1009113-1009407, incl. a second CHKTSAnalyze). Note the explicit **"tif"
   exclusion** in FUN_140451740 (:726583) on this path.
   The CHK repair file reader (FUN_1404a1fe0 :772528-558) buffers 0x9000 (36 KiB) — or
   0x200000 (2 MiB) for large files — so the offset-512/526 signs ARE reachable.

### 1.2 The og's GetRawUserInfoMap — per-scan statistics map (verified in §0)

Live, process-wide `std::map<std::wstring,int64>` at RAWSearchDLL singleton+0x90; fed
insert-or-increment by ~36 analyser detection sites; keyed by format NAME; cleared at every
CRAWSearchController init (vtable+0x68); read one-way post-scan by DRWUI via LoadLibrary +
GetProcAddress into the `Trigger_Raw`/`FileSystem` report tree. **Never an engine input; never
persisted** (no rsf record carries it — the corpus rsf grammar, byte-confirmed per CORPUS-ERRATA
E-note, has session-state/volume/tree records only; the report builder reads the LIVE map).
Resume semantics unstudied (OQ-2).

### 1.3 The og's raw_signs.json — the ENGINE's sign DB (contrast object)

139 exts; schema `{header:{signs:["off,HEX,conf"]}, tail:{…}, min, max}` (min/max in KiB; conf
third token is DEAD data in the og — N-030); sign offsets 0..46 only; loaded read-only via
ifstream at engine init (RAWSearchDLL :23836-3850, FUN_18001ea40; per-ext header/tail parse
FUN_18001f460 :24232; per-sign split FUN_18001e510 :23522). config/RawConfig.ini `Filter` section
= per-ext enable/disable (IAnalyserFilter, :13133-13208). **EUUserData plays no part here.**

### 1.4 The core — surface-by-surface map

| og surface | core surface | verdict |
|---|---|---|
| EUUserData read-only load (install-tree, per-process) | `usersigns::parse` (cr_usersigns.cpp:137-224) + CLI default load of `data/userdata-defaults.json` beside rrecov.exe (rrecov.cpp:1228-1264) + `--user-signs` (rrecov.cpp:1265-1293) + ABI `cr_ui_register_user_signs` (cr_ui.cpp:4509-4516) | present as a **carve-feed superset** (D-104 registered); default-on is CLI-app-level only (LC-10) |
| EUUserData → CHK repair identify/rename chain | nothing (no CHK/repair-file consumer) | **absent — owned by S-050** (file-repair line, SEVERE, open) + D-104 FOLLOW-UPS handoff; NOT refiled here (LC-9 addendum detail) |
| sign grammar "offset,HEX" decimal offset | parse_sign decimal, ≤4 digits, ≤1024 (cr_usersigns.cpp:94-121) | MATCH (both decimal) |
| multi-sign OR semantics | match_at first-match-wins over all signs (cr_usersigns.cpp:252-261) | MATCH (OR) |
| hex case: UPPERCASE-only text memcmp | case-insensitive nibble decode (:77-91) | **DIVERGENCE LC-3** |
| bounds guard offset+hexSTRlen (2×) | correct `offset+len > avail` (:255) | **DIVERGENCE LC-5** (core superset) |
| unknown members ignored; mime ignored; per-sign skip; `_stricmp` "signs" | strict: unknown key / dup mime / odd hex / token≠2 / trailing garbage / empty-signs → whole-file ERR_BAD_PARAMETER; exact-case "signs" (:174-221) | **DIVERGENCE LC-4** |
| duplicate ext keys (asf ×2) — og semantics unresolved | MERGE: both blocks' signs active; per-block mime backfill (:150-216) → og file verbatim parses as 191 signs | **DIVERGENCE LC-6** / OQ-1 |
| no writer (read-only file) | no writer (install_set replaces the in-memory set; nothing writes the JSON) | MATCH |
| GetRawUserInfoMap per-extension stats map | CarveStats = 13 scalars (cr_carve.h:27-57); carved_found=cs.validated (cr_scan.cpp:2184); no per-format aggregation | **absent — owned by N-140** (MINOR, open); new rider LC-8 |
| searcher-init clear of stats | fresh CarveStats per carve_slice call | present (per-scan lifetime ✓) |
| stats map → "Trigger_Raw"/"FileSystem" report tree | nothing (report carries category totals) | owned by N-140 |
| raw_signs.json runtime load (139 formats) | compiled-in generated table (cr_rawsigns_data.cpp, 139/199/62) | registered D-103 (content-identical, posture divergence) |
| RawConfig.ini per-ext toggles | flags.disabled_formats → trial_disabled (:1118-1126) + rawsign_disabled (:1128-1131, applied :836-844) | present for grammar+raw-sign layers; **NOT for user signs (LC-2)** |
| user-sign channel as scan input | user-sign carve layer (cr_carve.cpp:727-808): 512-stride probe, emit ext+mime, extent-to-device-end, conf 600, tag 0xFFFE0000, F9 dedup, emitted_at claim | **superset by design (D-104); collision order + gating broken (LC-1, LC-2)** |
| mime (dead in og) | parsed + surfaced: carve_note (cr_carve.cpp:789-793) → session-persisted (cr_records.cpp:238 write, :445-454 read) + ABI row | superset, INFO |
| rsf: no stats persisted | session v2 persists scalar carved_found (cr_records.h:137) | superset scalar; per-format absent (N-140) |

---

## 2. DIVERGENCE CANDIDATES

### LC-1 — user-sign layer probes BEFORE the built-in raw-sign table: the M-048/D-104 "built-in-first" collision rule is inverted in code
- **Severity: SEVERE** (wrong emission shape/channel for overlapping formats in the shipped
  default-on configuration; spec-vs-code, since og-parity for the channel itself is conceded by
  D-104).
- **Og:** no user-sign channel at all (E-A verified; SCAN-ENGINE-DELTA F2). Reference collision
  semantics exist only among the og's own layers.
- **Core (code):** cr_carve.cpp:738-740 probes `usersigns::match_at` at the stride and
  `continue`s on hit (:805) — **before** `idx.probe_header` (:830, the 139-format built-in table)
  in the same iteration. Grammar-layer claims still win (emitted_at :725).
- **Core (spec):** the in-code comment says the opposite — "a user sign colliding with a BUILT-IN
  fires only when the built-ins did not match — built-in-first, the collision rule the finding
  specifies" (cr_carve.cpp:730-733); cr_usersigns.h:85-87 repeats it ("built-ins were already
  probed by the earlier layers at the same stride"); the test
  `CarveEngine.UserSignatureYieldsToBuiltInAtSamePosition` (test_carve.cpp:6983-7018) claims to
  pin it but only exercises GRAMMAR-layer preemption (plant_png :125-150 plants a full valid PNG
  claimed by the FMT_PNG validator; PNG is not a raw-sign-table format) — the raw-sign-layer
  collision is untested.
- **Concrete behavioral difference:** with the default-on og DB
  (userdata-defaults.json, 190 signs), the 8 formats that exist in BOTH the default set and the
  built-in raw-sign table — **amr, cap, class, cpl, flac, msf, pdb, wpl** (computed: EUUserData
  keys ∩ kFormats exts) — flip channels at their magics: instead of the bounded raw-sign
  candidate (min/max sizes, tail pairing, table confidence) the core emits a user-sign record
  with extent to DEVICE END (len = ctx.total - start_abs, :741), confidence 600, tag 0xFFFE0000.
  Additionally, every grammar-REJECTED common-format magic in free space (truncated JPG/MZ/OLE…
  all present in the default DB) becomes a device-end user-sign file. Both the built-in table's
  bounds and its tail pairing are silently bypassed.
- **PARITY-DELTAS check:** D-104 registers the channel as a superset but does NOT register an
  ordering divergence; its text ("a user sign for an already-probed position is a duplicate by
  construction" is not in D-104 but in the code comment; D-104 only claims gating parity — see
  LC-2) — no row covers the inversion. N-140/N-143/S-050 do not touch it.
- **ID: LC-1.**

### LC-2 — the disabled-formats filter does not gate the user-sign layer (D-104's "same format-toggle rules" claim is false in code)
- **Severity: SEVERE** (user-observable: turning a format OFF does not stop it being carved via
  the user-sign channel in the default-on configuration).
- **Og:** RawConfig.ini `Filter` removes analysers from the working registry (FUN_180006b30
  removal loop, registered M-024) — the og's only sign sources are all inside that gate.
- **Core:** `flags.disabled_formats` gates the grammar trial rows (`trial_disabled`,
  cr_carve.cpp:438-439, setup :1118-1126) and the built-in raw-sign formats (`rawsign_disabled`,
  :836-844, setup :1128-1131). The user-sign block (:738-808) has **no disabled-format check** —
  occupancy/bitmap/free-map gates apply (they precede it), the format toggle does not.
- **Concrete difference:** host disables "cpl" (or "jpg", "exe", …) → built-in/raw-sign/grammar
  emissions for it stop, but the default user-sign set still carves it (tag 0xFFFE0000).
  D-104's registration text explicitly claims "the carve pass remains gated by the same
  format-toggle/occupancy/bitmap rules as every other carve source" (PARITY-DELTAS.md:3206-3207)
  — the format-toggle half is not implemented.
- **PARITY-DELTAS check:** D-104's claim is contradicted by the code — this is a
  registration-vs-implementation defect, not covered elsewhere (N-149 is the carve filter
  exempt-set on the og side; distinct).
- **ID: LC-2.**

### LC-3 — hex case sensitivity: 12 of the og's own signs are DEAD in the og but LIVE in the core
- **Severity: MAJOR** (default-scan output differs from the og's own DB semantics; the core
  carves formats the og's consumer can never match).
- **Og:** FUN_140604090 converts file bytes to UPPERCASE hex (`%02X`, only wide %02X in
  DRWUI.exe at 0x165bfba; zero %02x) and memcmps against the stored sign TEXT
  (DRWUI :1003452/:1003555) → any sign containing lowercase hex never matches. The shipped
  EUUserData contains **12 such signs** (mpeg ×11 — e.g. `0,000001b321000100`; img ×1 — the
  mixed-case 44-byte sign). They are dead configuration in the og.
- **Core:** `hex_nibble` accepts both cases (cr_usersigns.cpp:77-91) and compares decoded BYTES
  (:257) → all 12 signs match. With the default-on set, the core carves MPEG streams (and the
  img magic) the og's own signature DB could never fire even in its own consumer.
- **PARITY-DELTAS check:** not registered; D-104/M-048 do not discuss case semantics.
- **ID: LC-3.**

### LC-4 — parser strictness vs the og's tolerant loader (forward-compat availability)
- **Severity: MAJOR** (edge/availability: a valid-for-the-og file is wholesale-rejected by the
  core).
- **Og:** unknown members in ext objects are IGNORED (the FUN_1406072a0 loop acts only on
  `_stricmp "signs"` :1005818 — note "mime" itself is never read); member names case-insensitive;
  malformed signs (token count ≠ 2) are silently SKIPPED (:1005986); odd-length hex loads but
  never matches (converted length (hexlen>>1)*2 ≠ hexlen, :1003452 guard); `atoi` even
  mis-parses gracefully ("0x10," → offset 0).
- **Core:** unknown key → whole-file ERR_BAD_PARAMETER (cr_usersigns.cpp:206-207); duplicate
  "mime" → reject (:204); odd hex → reject (:110); any malformed sign → reject (:194-195);
  trailing garbage → reject (:220-221); empty signs array → reject (:201-202); "signs" key is
  exact-case strcmp (:174). All-or-nothing.
- **Concrete difference:** a hand-edited or versioned EUUserData with one bad sign or one extra
  member (e.g. a future `"size"` field) loads (partially) in the og and is REJECTED ENTIRELY by
  the core — the CLI then proceeds with zero user signs (degrade) or `--user-signs` aborts
  (rrecov.cpp:1285-1289). Note the honest-rejection doctrine is a deliberate core posture
  (S-039 lineage) — the divergence is the og's tolerance, worth registering rather than
  "fixing" silently.
- **PARITY-DELTAS check:** S-039/M-067 registered the CAPS, not the strictness-vs-tolerance
  semantics; no row covers it.
- **ID: LC-4.**

### LC-5 — the og's 2× bounds guard (files shorter than offset+hexSTRlen never match)
- **Severity: MINOR** (edge: the og over-rejects short .chk fragments; the core is a superset).
- **Og:** guard `filesize < offset + hexstrlen` (DRWUI :1003371, :1003545) while reading only
  hexlen/2 bytes — a fragment of length in [offset+signbytes, offset+2·signbytes) is never
  tested (e.g. the 16-byte asf sign at 0 requires ≥32 bytes).
- **Core:** `if (u64(s.offset) + s.len > avail) continue` (cr_usersigns.cpp:255) — the correct
  bound.
- **PARITY-DELTAS check:** unregistered. Core superset on short files.
- **ID: LC-5.**

### LC-6 — duplicate-ext-key semantics (the shipped "asf" duplication)
- **Severity: MINOR** (quirk: the og file itself is ambiguous; core merges).
- **Og:** unknown — the CJsonObject-family member storage (FUN_140077180/FUN_14007dca0) decides
  first-vs-last-vs-merge for the duplicated `asf` block; not decoded (OQ-1). The RAW file has 112
  ext blocks / 191 sign strings.
- **Core:** MERGE — both blocks parse, both signs install; the per-block mime backfill
  (cr_usersigns.cpp:212-215) assigns each block's mime to its own signs. The og file verbatim
  parses as **191 signs** (vs the shipped userdata-defaults.json's pre-deduped 111/190 — parsed
  content identical, I-033's "111/111 keys, 190/190 signs" is the deduped view; the test
  DefaultSetShipsAndLoads pins 190).
- **ID: LC-6.**

### LC-7 — stale pre-erratum claims in the core tree (comment/doc-truth batch)
- **Severity: MINOR** (doc-truth; some rows actively mislead future sessions).
- Items (all contradict E-A/E-B/D-104, all read this session):
  1. cr_usersigns.h:8 — "The og's carve layer consults it alongside raw_signs.json" (the
     REFUTED pre-E-A reading).
  2. cr_usersigns.h:86-87 — "built-ins were already probed by the earlier layers at the same
     stride" (false — see LC-1).
  3. cr_usersigns.h:42-43 — kMaxEntries comment says "the og ships 111" but the cap is enforced
     against TOTAL SIGNS (out.signs.size(), cr_usersigns.cpp:183) — the og ships 190/191 signs;
     256 is 1.34× headroom, not 2.3×.
  4. cr_usersigns.h:46 — "the og's longest ext is 4 ('epub','hqx')" — actually 8 ("manifest";
     the cap itself is correct).
  5. PARITY-OG-CORE-INVENTORY.md:84 — "GetRawUserInfoMap feeds the RAW scan-setup trigger
     surface" (E-B refuted: it is scan OUTPUT, never setup input).
  6. PARITY-DELTAS.md D-52 (:1176-1177) — groups GetRawUserInfoMap with EUGetAnalyserList/Map/
     Multimap as "raw-dual-registry seeds" (it is the stats map, not a registry view and seeds
     nothing).
  7. PARITY-DELTAS.md D-60f (:1838) — "the engine reads RawConfig/FileFilter/raw_signs/
     EUUserData only" (the ENGINE never reads EUUserData; DRWUI does).
  8. SCAN-ENGINE-DELTA.md C4 (:89) — "… + EUUserData MIME" in the og classification chain (no
     binary consumer reads the mime member).
- **Corpus-side residue (same batch, for the maintainers):** config-files/userdata-remoteconfig
  §1 still claims "the UI's custom-format flow feeds this JSON" (no writer exists) and
  rawsearch-carving--deep-dive §3 item 2 still lists "EUUserData magic/MIME DB — secondary
  typing" among the ENGINE's sign sources — both are the claim-side of CORRECTIONS #9 and have
  not yet been folded into the artifact texts (the errata protocol's "fold when next touched").
- **ID: LC-7** (fold into the N-148 comment-truth-batch class or file standalone).

### LC-8 — user-sign emissions are invisible in carved_found (runtime + session-persisted undercount)
- **Severity: MINOR** (stats accuracy, distinct from N-140's missing per-extension surface).
- **Og:** n/a (no channel). **Core:** the user-sign path ticks only `ctx.sign_hits`
  (cr_carve.cpp:803); `stats.carved_found = cs.validated` (cr_scan.cpp:2184) and the
  session-persisted v2 `carved_found` (cr_records.h:137) therefore EXCLUDE every user-sign file.
  With the default-on set (78 extensions carried by NO stock source — test
  OgAbsentFromStockExtensionsCarried), the headline carved count undercounts by the entire
  user-sign channel.
- **PARITY-DELTAS check:** N-140 owns the per-extension aggregation surface; this is the scalar
  undercount — unregistered.
- **ID: LC-8.**

### LC-9 — the CHK repair chain (the og's ACTUAL consumer): decode-detail addendum for S-050 (do NOT refile)
- **Severity: INFO (coverage confirmed; detail new).**
- The chain's absence in core is owned by S-050 (file-repair line, SEVERE, open — its corpus
  references already name CHKRepairFileSign/CORRECTIONS #9) and D-104's FOLLOW-UPS handoff.
  This lane's first-hand additions for a future S-050 addendum: the rename-to-identified-ext
  semantics ("%s.%s" / "%s_%s.%s", DRWUI :726699-706), the adapter registry keyed by hex prefix
  ("47"=TS, "4D5A"=PE, "504B0304"=Office-zip, "D0CF11E0A1B11AE1"=Office-OLE, TIFF keys,
  :1008918-1009407), the 36-KiB/2-MiB match read buffer (:772543-553), the explicit "tif"
  exclusion (:726583), and the parser quirks LC-3/LC-5 (which apply to the CHK identify flow).
- **ID: LC-9.**

### LC-10 — the "engine-init default load" is CLI-app-level only (D-104/inventory phrasing imprecise)
- **Severity: INFO.**
- D-104 (PARITY-DELTAS.md:3194-3196) and PARITY-OG-CORE-INVENTORY.md:84 say the default set is
  "loaded at engine init"; PREBUILT-ARTIFACTS.md:106 correctly says "beside rrecov.exe". The
  only load site is rrecov.cpp:1228-1264 (the CLI app); the engine/orchestrator and the cr_ui
  ABI path have NO default load — an ABI host gets user signs only via
  cr_ui_register_user_signs (cr_ui.cpp:4509). So "default-on" is the CLI product's posture; the
  library posture is default-off. Rider on LC-7.
- **ID: LC-10.**

---

## 3. OPEN QUESTIONS

1. **OQ-1 (decides LC-6):** the og's duplicate-key semantics for the shipped `asf` block —
   first-wins, last-wins, or merge? Resolvable by decoding DRWUI's member-name storage:
   FUN_140077180 (member-name vector build) + FUN_14007dca0 (the parse) in
   ghidra-drwui/DRWUI_decompiled.c, plus the underlying container (CJsonObject-family std::map vs
   list) — the deciding artifact is the decompile of those two functions (no binary run needed).
2. **OQ-2:** does a RESUMED og scan under-report the stats map (searcher re-init clears it
   mid-session, so the post-scan report counts only post-resume detections)? Resolvable from the
   DRWUI resume flow: the call sites of the CRAWSearchController init (vtable+0x68) during
   session-load/resume vs the report reader's timing (FUN_1401227e0 callers :1098891/:1313115/
   :1320974) in ghidra-drwui.
3. **OQ-3:** is the `Trigger_Raw`/`FileSystem` report tree persisted to disk (a report file) or
   UI-only? Resolvable by decoding FUN_1400ed3f0/FUN_1400ed2a0 (the merge into param_1's report
   object) and that object's save path in ghidra-drwui.
4. **OQ-4:** is FUN_140606a90 (the EUUserData load+parse-again entry, DRWUI :1005311) ever
   called (i.e., can the DB be re-read without a process restart)? No in-decompile caller —
   needs a binary xref sweep for pointers to 0x140606a90 in DRWUI.exe (same method used for
   FUN_18000beb0's vtable slot this session).
5. **OQ-5 (minor):** the exact target of the LAB_140451950 callback thunk (the per-file check
   wrapper called via FUN_140452620, DRWUI :726841) — completes the CHK chain's argument flow;
   the chain is already RTTI-pinned through the singleton vtable+0x08 so this is cosmetic.

---

## 4. WORKLOG / method notes

- Erratum verified before any filing (§0); CORPUS-ERRATA read end-to-end for errata-errata
  against E-A/E-B — none found; E-S11 #4 (singleton layout) re-verified and extended (the
  singleton's stats map keying by FORMAT NAME via the 9-byte-stride id→name table at
  RAWSearchDLL.dll 0x18008f070 — 169 names dumped).
- vtable/RTTI pins done against the shipped binaries (DRWUI.exe CHKRepairFileSign vtable
  0x141899958 → Match = FUN_140603820; RAWSearchDLL.dll CRAWSearchController vtable 0x180066e38
  → init/clear = FUN_18000beb0 at slot +0x68) — the decompile alone had missed both xrefs.
- Overlap checks vs active findings: N-140 (stats surface — owns the per-extension aggregation
  gap; LC-8 is the distinct scalar undercount), N-143 (two-step dispatch — untouched), S-050
  (owns the CHK-repair/file-repair absence; LC-9 is an addendum), S-040/S-048 untouched,
  M-073..M-117 unrelated, I-026..I-034 (I-033 owns the data-asset byte-identity claims — my
  191-vs-190 raw-sign nuance is LC-6, not a refutation of I-033 whose claim is about the
  deduped shipped file), D-103/D-104 read in full (LC-1/LC-2 are defects AGAINST D-104's own
  registered rules, not new parity gaps).
- Files touched: only this scratch file.
