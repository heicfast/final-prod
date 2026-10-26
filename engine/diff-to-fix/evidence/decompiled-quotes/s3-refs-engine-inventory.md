# S3-REFS evidence — ReFSSearchDLL full-capability inventory (function-by-function map)

**Agent:** S3-REFS (session 3, read-only parity audit).
**Corpus artifact:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c`
(20,270 lines, 629 functions; indexes `ReFSSearchDLL_functions.txt`, `ReFSSearchDLL_imports.txt`).
All `:NNNN` citations below are line numbers in that .c file unless prefixed `core:`.
**Core audited at HEAD `34b3441`** (`src/core/refs/cr_refs_engine.cpp` 928 LOC,
`src/core/refs/cr_refs_detect.cpp` 84 LOC, `include/cr/refs/cr_refs_engine.h` 105 LOC,
`include/cr/refs/cr_refs_detect.h` 45 LOC).

Prior coverage built on (not re-derived here): N-007 (signature-width first-DWORD),
I-004 (version-gate superset), GAP-S6-3 fix `7797bd6` (checkpoint-body minor gate).

---

## 1. Class census (RTTI vftables, verified by grep)

Named classes: `RefsQuickSearchFiles` (:72), `RefsSearchController` (:99),
`refs::RefsPartitionInformation` (:264), `refs::RefsBasicDevice` (ref-count obj :141),
`refs::RefsSupperBlock`/`RefsSupperBlockImpl` (:931/:12918), `refs::RefsCheckPoint`/`Impl`
(:968/:13603), `refs::RefsDbr`/`RefsDbrImpl` (:548/:13523), `refs::RefsMetadataBlockHeader`/`Impl`
(:9363/:11670), `refs::RefsMetadataBlockReference`/`RefrenceImpl` [vendor typo preserved]
(:11676/:11705), `refs::RefsBaseDataImpl` + two `RefsBaseDataImplManager<T>` instantiations
(:8823/:11755/:11769), `refs::RefsNodeRecord`/`Impl` (:8815/:7837), `refs::RefsNodeHeader`/`Impl`
(:8349/:14902), `refs::RefsMSTreeHeaderDescImpl` (:8577), `refs::RefsFileValuesImpl` (:8599),
`refs::RefsDirectoryValues`/`Impl` (:9015/:9019), `refs::RefsFileProperties` (:2108/:9772),
runlist classes `IDataRunList` (:10118), **`CReFSDataRunList`** (:10130, non-resident),
**`CRefsResidentDataRunList`** (:10430, :10743 — resident/inline), and (shared family)
`CNoneResidentNTFSDataRunList` (:14461), `CFileNode` (:2282), `CFileSystemNode` (:6108),
`CSearchedPartitionInfo` (:165), `IFileProperties` (:14244),
`IQuickFileSystemSearchController` (:500), `ICompleteFileSystemSearchController` (:5436).

Corpus-doc reconciliation: `refs-engine--deep-dive.md` §1–§3 and
`remaining-engines--ext4-hfs-refs-btrfs-zfs.md` §ReFSSearchDLL match this census; the deep-dive's
"CRefsResidentDataRunList (resident — ReFS small files inline)" claim is **confirmed by
construction sites** :10430/:10743 (see §6.3). `refs-btrfs-zfs-engine-algorithms-v2.md` §1's
pseudo-code for the block-header parser matches `FUN_18000fc80` exactly (§3.4), except the doc's
"refs::RefsMetadataBlockRefrenceImpl::parse(page + 8, …)" should read **page+0x20** — the call at
:11715 is `FUN_18000ff60(puVar2,(undefined8 *)(param_2 + 8),param_3 - 0x20)` with `param_2` an
`int*`, i.e. byte offset +0x20 (ref area after the 0x20-byte page header), length `size-0x20`.
(Doc nit only; the core already reads +0x20-relative refs — session-1 C20 MATCH stands.)

## 2. Entry points, OS gate, controller split

| Capability | Og evidence |
|---|---|
| Exports `EUCreateReFSQuickSearcher` / `EUCreateReFSSearcher` | :54–78 / :83–113 (quick = `RefsQuickSearchFiles` vtable @ :72, 0x210-byte impl via `FUN_1800019c0`; complete = `RefsSearchController` @ :99) |
| **Host-OS gate**: refuses to run below Windows 6.2 (Win8) | `FUN_1800010d0` :37–49 — `GetVersion()`; `if (major < 6 || minor < 2) return 0;` (import census: `KERNEL32.DLL::GetVersion`, imports.txt:1) |
| Quick-scan result callbacks registered for tags **0x500 / 0x600 / 0x701** | `FUN_1800019c0` :593–616 (member-fn thunks at DAT_180002d30/0x30d0/0x32e0) |
| Quick-scan volume/root node emission (tag 0x600) | `FUN_1800041f0` :2044–2195: builds `refs::RefsFileProperties` (flags `\| 0x10` :2102), sets grade 10 via props-vtable+0x40 (:2139), copies the volume label (≤64 wchars, :2141–2167), emits with node tag 0x600 (:2169) through the sink at ctx+0x1e8 vtable+0x18 (:2170–2171) |
| Quick-scan counting/aggregation pass (files, deleted, sizes, 0x800/0x40-attr counts) | `FUN_18000a2c0` :7059–7202 (BFS with deque; skips directories via props+0x39 bit 0x10 :7119–7125; deleted bucket when node grade byte&3==2 :7137–7149; counts `u16@props+0x39 & 0x800` and `& 0x40` :7150–7155) |
| Quick vs complete open-path switch | `FUN_180003d50` :1878–1947 — `*(int*)(ctx+0x5c)==0` → quick chain `FUN_1800021e0` (:1899) else complete `FUN_180004bf0` (:1923) |
| Search entry (partition handed to engine) | `FUN_180003950` :1676–1761 — stores partition start on the device (:1692–1693), calls open (`FUN_180002130` :1755) then `FUN_180003d50` (:1757) |
| Already-scanned volume dedup (complete-search over overlapping candidates) | `FUN_180004ec0` :2634–2814 — global list `DAT_180022650` of {start, cluster-shift, chkp-id pair}; returns "seen" :2768–2775 |

## 3. Detection / boot gates

### 3.1 DBR (boot sector) — `FUN_180012390` `RefsDbrImpl::parse` :13511–13566
- Gate: buf≠0, size ≥ 0x200, `"ReFS"` u32 @+3 (`0x53466552` :13529), `"FSRS"` u32 @+0x10
  (:13530), **major u8 @+0x28 == 3 && minor u8 @+0x29 > 3** (:13530–13531) — ReFS 3.4+.
- Fields: u64 @+0x18 (→obj+0x10, sector count), u32 @+0x20 (bytes/sector), u32 @+0x24
  (sectors/cluster), u8 @+0x28/+0x29 (major/minor), u64 @+0x38 (→obj+0x28), u64 @+0x40
  (container size) :13532–13542.
- **Geometry gate** :13544–13546: bytes/sector ∈ {0x100,0x200,0x400,0x800,0x1000} AND
  `spc*bps == 0x1000 || == 0x10000` — **cluster size exactly 4 KiB or 64 KiB**.
- `major==1` path :13547–13550 (page clusters = 0x4000/bps, valid); `major==3` :13552–13561:
  obj+0x30 = spc (page clusters); minor==0 → container-clusters := 0x1000; else
  **obj+0x34 = u64@+0x40 / cluster** (container size in clusters — the object-id-table sweep
  bound, §6.5). ⇒ no 2.x, no 3.0–3.3 (see I-004 for the core's documented superset).

### 3.2 Quick-path open — `FUN_180002130` :832–868
Partition fs code **== 10** and detected flag (`*param_1==10 && param_1[0x17]!=0` :847);
copies sectors/cluster (+0x38 → +0xb4 :850), sector size from device vtable+0x28 (default 0x200,
:851–860), MSB identity default **0x4000** (:861), volume length (+0xa0 − +8 :862), then
`FUN_180011cd0(device, partition-info)` :863. Non-matching → full detect `FUN_180004980` :866.

### 3.3 Full detect — `FUN_180004980` :2410–2496
Reads 1 sector at offset 0 (:2440), `RefsDbr` parse (:2444); if valid copies geometry
(sector size :2458, spc :2459, **MSB-area offset = cluster*0x1e** :2460, major/minor
:2461–2462, container-clusters :2463, obj+0x28 :2464), then `FUN_180011cd0` :2465.
**Layout-generation tag** :2466–2477: `major==3` → minor <5 ⇒ gen 3; <0xb ⇒ gen 7; ≤0xc ⇒ 8;
else 9 (stored at ctx+0x1d8, pushed to the record-processor ctx at :2596/:2000). This tag gates
the record-grammar switch (§6.2) — the og runs **two attribute grammars** by ReFS minor version.
Marks the first cluster in the read-error/used-space map (:2478–2479).

### 3.4 Page/metadata header — `FUN_18000fc80` `RefsMetadataBlockHeaderImpl::parse` :11659–11737
Min size 0x20 (:11688); page-type 1/2/3 → signature strings
"SUPBMSB+ReFSFSRS" / "CHKPSUPBMSB+ReFSFSRS" / "MSB+ReFSFSRS" (:11689–11700); comparison is
**first-DWORD only** (`*param_2 == *(int*)pcVar4` :11701 — N-007); ref parsed from page+0x20
with length size−0x20 (:11715, `int*` arithmetic); header.size (+0x20) += ref.size (+0x30, set
to constant 0x30 in `FUN_18000ff60` :11874) (:11717); on valid ref: LCN-quad list, flags
byte@ref+0x22 (:11871), **next u64 @ref+0x28** (:11872–11873) copied into the
`RefsMetadataBlockReference` (:11718–11729).
`FUN_18000ff20`/`FUN_18000ff40` :11789–11808 — standalone "SUPB"(0x42505553)/"CHKP"(0x504b4843)
DWORD probes used by the complete-search probe (§3.6).

### 3.5 Superblock — `FUN_180011770` `RefsSupperBlockImpl::parse` :12899–12988
Parses as page-type 1 (:12930); **MSB-area cluster default 30** (obj+0x40 = 0x1e :12925),
overridable by a second block-ref entry (:12947–12949); self-descriptor at page+header.size:
16-byte volume GUID copied to obj+0x10/:12966–12970; next descriptor requires type==2
(:13773 → `*(int*)(desc+0x24)==2` :12973) and yields **the two checkpoint cluster numbers at
obj+0x28/obj+0x30** (:12974–12975) plus bounds checks (:12976–12983).

### 3.6 Checkpoint chain + complete-search probe
- Quick chain `FUN_1800021e0` :873–1094: reads boot cluster (:911–918), `RefsSupperBlock`
  (:931–934), checkpoint A at cluster*sb[5] (:948–970), checkpoint B at cluster*sb[6]
  (:987–1010); **newer sequence number wins** (`puVar7[2] < puVar6[2]` compare :1029–1031);
  chosen checkpoint copied out (:1032–1050); used-space marks for boot/SUPB/CHKP areas
  (:1051–1066); then the main scan `FUN_180004ec0` (:1067).
- `FUN_180012650` `RefsCheckPointImpl::parse` :13663–13810: page-type 2 (:13707); body at
  header.size (:13743); **body version gate major u16@body+4 == 3 && minor u16@body+6 > 3**
  (:13744–13745); **descriptor list offset 0x58 (body) if body-minor ≥ 0x0e else 0x44**
  (:13754–13757); count u32 @body+0x40 (:13761); each list entry = page-relative u32 offset →
  0x30-byte `RefsMetadataBlockRefrenceImpl` (:13764–13782, slot index stored at ref+0x2c
  :13785); valid only if all count entries parsed (:13805). (Core REFS-1/REFS-2/GAP-S6-3 all
  re-confirmed at these lines.)
- Complete-search probe `FUN_180010630` :12149–12230 (lost-partition side): reads 32 sectors at
  the candidate; requires first DWORD "SUPB" or "CHKP" (:12173–12176); then
  `FUN_180010c00`/`FUN_180010da0` (:12202–12228). Validation `FUN_180010820` :12235–12310
  re-reads the superblock at `[+0x98]+[+0x10]` and validates both checkpoint candidates via
  `FUN_180010a20` on sb[5]/sb[6] (:12290–12294). `FUN_180010390` :12016–12144 parses the DBR and
  fills `CSearchedPartitionInfo` with **fs code 10** (`*param_4 = 10` :12125 — C21), geometry
  (sector size :12094–12096 force-0x200-on-odd, spc :12115, MSB offset = spc*0x1e :12116,
  container-clusters :12118, serial :12117).
- Complete-open `FUN_180004bf0` :2501–2629: guard length ≥ 1 (:2525); reads a cluster at offset
  `ctx+0x170` (:2539–2541) → `RefsCheckPoint` parse (:2556) → **version + layout-gen tag from
  the checkpoint body** (:2571–2596) → `FUN_180004ec0` (:2597). NOTE: the +0x170 read-offset
  semantics (= `[+0xa0] − [+8]`, set in `FUN_180002130:862`) are **UNRESOLVED** — see
  scratch-report UNVERIFIED list.

## 4. Page I/O, cache, container translation

- `FUN_180011b50` :13093–13163 — read the page addressed by a block reference: block-size guard
  (< 0x8000000, :13113–13115); block number = first qword of the ref (:13119);
  **container translation when block ≥ cluster size** (:13117–13146): key =
  `((block/cluster) >> 1 − 2) * cluster` (:13122), map at device+0xf8; on hit, block =
  `(block & (cluster−1)) | mapped_base` (:13145). Read at sector-offset = shift*block
  (:13148–13152); insert into the page cache at device+0x108 (:13158).
- `FUN_180011e90` :13223–13267 — the same translation as a free function (used for datarun
  blocks): no map / block < cluster ⇒ identity (:13237–13242); otherwise the
  `((block/cluster)>>1)−2` key (:13244) and OR-rewrite (:13264).
- Page fetch + parse + cache for tree walks: `FUN_180003620` :1529–1643 (read :1577, record-list
  parse `FUN_18000ae70` :1591–1593, cache insert keyed by ref+0x10 :1616).
- System-directory/container map build: `FUN_180002a60` :1255–1381 — walks records with
  32-byte-ID-bearing keys of length 0xa0 or 0xe0 (ID halves at key+0x90/+0x98 or +0xd0/+0xd8,
  :1302–1309); **identity gate**: second ID half must equal ctx+0x148 (container-cluster count
  from the DBR, :1314); inserts `map[counter*identity] = first-ID-half` into ctx+0x188
  (:1336–1342). The map is handed to the device at `FUN_180003f40:1995–1999` and consumed by
  §4 translation.
- `FUN_180004820` :2335–2405 — post-walk pass over fetched block lists: translates every block
  through the same formula (:2355–2386) and marks `cluster*block .. +count` in the
  used-space/read-error map (`FUN_1800089f0`, :2385).

## 5. Tree walkers (page level)

- Main scan driver `FUN_180003f40` :1952–2039: walks the chosen checkpoint's three root
  descriptor lists (+0x78/+0x88/+8 → `FUN_180002a60` twice for the system-dir/container map,
  :1965–1993), moves the map onto the device (:1995–1999), walks the root tree
  (`FUN_180002770` :2001–2002), then **always runs the stale-page sweep** `FUN_180005330`
  (:2015) and the result flush `FUN_18000ef20` (:2016), and clears the page cache (:2020–2036).
- Root-tree walk `FUN_180002770` :1099–1250: per record, key type from the record value
  (:1147–1156): **0x600 → emit the volume/root node** (`FUN_1800041f0` :1151, §2);
  > 0x700 → treat as **0x701** (:1153–1155) and notify the controller sink (record + value
  passed at vtable+0x10, :1187–1208) — the system-identity notification path.
- Directory recursion `FUN_1800032f0` :1386–1524: enumerates a node's records; per record:
  **deleted-flag propagation** (record+0x5a := 1 :1463; +0x59 inherited from parent :1464–1466);
  dispatch on key type u16 (`FUN_18000b820` :8096–8106 = key[0]):
  - **0x10 → `FUN_18000cbd0`** (object/file-values records, §6.1) :1471–1473
  - **0x30 → subtype u16@key+2** (`FUN_18000b840` :8111–8121): **1 → `FUN_18000ba90`**
    (file entries, §6.2) :1481–1483; **2 → `FUN_18000c920`** (directory entries, §6.4) :1484–1486
  - directories (record+0x58) recurse (`FUN_1800032f0` :1489–1491).
- Node grammar: `FUN_180014380` :15333–15384 — at base+skip: entry-array offset u32@+0x10,
  count u32@+0x14, end bound u32@+0x20; entries = u16 offsets from base (:15354–15380).
  Node-header enumeration `FUN_1800144a0` :15389–15414 (`RefsNodeHeaderImpl`).
- Record grammar `FUN_18000b210` `RefsNodeRecordImpl::parse` :7815–7972 — entry:
  {u32 total@+0 (≤0x2000), u16 keyOff@+4, u16 keySize@+6, u32 type@+8, u16 valueOff@+0xa,
  u32 valueSize@+0xc}; bounds chain :7861–7896; key copied to obj+0x18 (:7897–7901), value to
  obj+0x30 (:7962–7967); the `FUN_18000b860` shape (:8126–8154: value size 0x30, name at
  value+u16@entry+0xa, typed header {type u8@name+0x22 ∈{1,2}, resident-size u8@name+0x23==8,
  size u32@name+0x24 ∈{4,8}}) stores the **four qwords at value+nameOff as an LCN-quad block
  reference** (obj list at +0x48, :7907–7960) — the *indirect attribute* recognizer.

## 6. Record processors — the object/attribute layer (ADS determination)

**Result model (og):** ONE `refs::RefsFileProperties` per object identity. Single data-stream
slot: stack props +0x62 := {u16 runlist-type-tag, `IDataRunList*}` (written by `FUN_18000e020`
:10185, `FUN_18000e490` :10454, `FUN_18000ea30` :10768) — copied to the heap object at +0x73
(`FUN_18000d970` :9798–9801; result-side contract `SaveScan/ifileproperties-part2--ext4-hfs-refs.md`
§3 "u32 hasRun → DataRunList +0x73"). A non-resident runlist, built once per object, **overwrites**
a resident one (`FUN_18000e020` runs after the attribute loop, `FUN_18000ba90:8478`). **No code
path emits a named stream as a separate recoverable result; there is no stream-name field in the
emission** (emission carries {object key, props} only — `FUN_18000ef20:10934–10939`,
`FUN_18000d970:10007–10018`). ReFS has no NTFS-style ADS on disk; what it has are per-object
*attribute records* (typed, named) — and both engines fold them into the object's single stream,
but with very different coverage (below).

### 6.1 Type-0x10 records — `FUN_18000cbd0` :9106–9263
Value = u32-offset chain of sub-records (:9163–9205):
- sub `{off==0xa8, u16@+4==0x28}` → **timestamps: 4×(u64,u64) pairs** copied from
  +0x18/+0x28/+0x38/+0x48 (:9172–9185).
- sub `{u16@+4==0x10, u16@+0xa==0x18}` → **name block**: name length u32@+0xc, UTF-16 name at
  sub+0x30 (:9187–9201). Single-char names of the root object (node id 0) matching the
  `DAT_1800199a8` char table are appended to the ctx special-name list (:9213–9247) — used at
  flush to tag the root row 0x600 (`FUN_18000ef20:10914–10933`).

### 6.2 Type-0x30 subtype-1 file entries — `FUN_18000ba90` :8233–8530
1. Inherit orphan/deleted flag from the record (+0x5a) and ctx layout-gen into props+0x38
   (:8309–8311); init props (`FUN_18000c2b0` :8312).
2. `FUN_18000c060` :8535–8636 — shape gate: key[0]==0x30 && key[2]==1, or (dead-tolerance)
   key[0]==0x40 && key[2]==0x8000 (:8558–8570 — unreachable from the page-walk dispatch §5);
   value parsed as `RefsMSTreeHeaderDescImpl` {count u32@v+0 (<0x1001), fileValuesOffset
   u16@v+4, …} (:8575–8592); at v+fileValuesOffset a `RefsFileValuesImpl` {u64@+0, u64@+8,
   u64@+0x18, u64@+0x30, u32@+0x20} fills props (:8597–8631); **global monotonically increasing
   file id** from `DAT_180021038` (:8620–8622); node id (record+0x68) → props+0x3a (:8618–8619).
3. Name: `FUN_18000c2b0` :8641–8756 — UTF-16 name from key+4, `(keySize−4)/2` wchars
   (:8673–8679), converted with `WideCharToMultiByte(CP_UTF8=0xfde9)` (:8690–8696) into the
   props name string (+0x42).
4. **Attribute loop** over the embedded node's children (`RefsNodeHeader` :8349–8352,
   `FUN_1800144a0` :8354–8358), dispatch per child (child key[0] u16 = `uVar16`, value size
   `uVar22`), with the **layout-gen grammar switch**:
   - `uVar16 == 0x178` → dir-flag bit 2 (:8369–8371) then datarun walk (below);
   - gen ≥ 8 && value==0x28 && key ≥ 0xb1 → dir-flags for 0x108 (bit 1) / 0xf8 (bit 3)
     (:8430–8438) then datarun walk;
   - value==0x10 && key==100(0x64) && name "$I…" → **`FUN_18000ea30`** (resident $I recycle
     metadata, :8374–8421);
   - value==0x10 && FileValues[1]==1 (props+0x10) → **`FUN_18000e490`** (resident inline
     content, :8422–8427);
   - record+0x58 (directory) → `FUN_18000d470` :8443–8446 else `FUN_18000c560` :8441.
5. Non-deleted records mark their runs in the used-space map (`FUN_1800089f0` :8451–8473);
   deleted records set the deleted bit in props (:8476).
6. `FUN_18000e020` (runlist build, §6.3) then `FUN_18000d970` (emit, §6.6) :8478–8481.

### 6.3 Data-run assembly
- `FUN_18000c560` :8761–8849 — attribute-value walker. **Gen gate**: gen<8 requires key 0x178;
  gen≥8 requires value size 0x28 && key ≥ 0x29 (:8784–8800). Sub-entries via `FUN_180014380`;
  **recognizer**: `byte@(sub+0xa) & 0x18 == 0x18` → datarun parser :8810–8813; else the record
  path `FUN_18000b210` → `FUN_18000cf30` :8815–8831.
- `FUN_18000c790` :8854–8934 — **run entry grammar**: flags u16@+8 with **0x40 = carries block
  reference** (required, :8880–8882), ref-part size u16@+0xa == 0x18 (:8883–8885), block u64@+0
  (:8888), offset u32@+0xc (:8887), length u32@+0x14 (:8886). **Flag 0x20 = physical/global —
  skip container translation; otherwise block is translated** via `FUN_180011e90` (:8893–8895).
  Runs appended {translated-block, offset, length} into the props run list (+0x6a, count +0x72)
  with **adjacent-run coalescing** (:8916–8931).
- `FUN_18000cf30` :9294–9505 — **indirect attributes**: record's own LCN-quad block reference
  (obj+0x48, :9333–9334) → page read → re-parsed as page-type 3 "MSB+ReFSFSRS" (:9363–9365) →
  MSTreeHeaderDesc at header.size (:9370–9397) → sub-entry loop: b860-shaped (named typed-value
  carrying its own block ref) → recurse (:9444–9463); datarun-shaped → `FUN_18000c790`
  (:9439–9442). **All runs merge into the same object's props.**
- `FUN_18000e020` :10084–10286 — build the `CReFSDataRunList` from the accumulated list (with
  re-coalescing :10156–10166), compute total size (:10179), store {tag, ptr} at props+0x62 and
  size at +0x32 (:10185–10186), then — gated on layout-gen ≠ 0 and device geometry
  (:10201–10204) — **granularity validation**: average run length in sectors is rounded up to a
  power-of-two band (:10223–10277) and **the whole runlist is discarded when it disagrees with
  the volume cluster size** (device+0x14) (:10278–10281).
- `FUN_18000e490` :10304–10524 — **resident (inline) content**: value {s32@+8, s32@+0xc} + node
  offsets (+0x5c/+0x60) + param_4 locate the inline bytes; block = loc / cluster (:10371–10372);
  block < 4 → += node MSB base (+0x70) (:10373–10374); container OR-rewrite (:10376–10399);
  builds a **`CRefsResidentDataRunList`** {block, span} with **resident byte offset at +0x48**
  (:10410–10436, vtable swap :10430), stores at props+0x62 (:10454).
- `FUN_18000ea30` :10615–10856 — "$I"-named variant of the same resident materialization
  (offset = value[3] + 8 + node offsets, :10679–10687; build :10723–10762; store :10768).
- Directory values `FUN_18000c920` :8939–9101 (subtype-2): `RefsDirectoryValuesImpl` from value
  {u64@+0, u64@+8, u32@+0x40, u64@+0x10, u64@+0x18, u64@+0x28, u64@+0x38} (:9034–9051);
  deleted flag from record+0x59 (:9062–9065); emit directly (:9070).

### 6.4 Directory-value walker — `FUN_18000d470` :9510–9635 / `FUN_18000d770` :9640–9722
Recursive page walk over directory records; same gen gate (:9661–9673); same
datarun/indirect-record dispatch (:9680–9702).

### 6.5 Stale-page (deleted-object) sweep — `FUN_180005330` :2819–3054
Runs after the live walks (call :2015). Sets the harvest flag in the processor ctx (:2859).
Reads the object-id-table allocated ranges (`FUN_180011f20` on device+0x108, :2860) — the
container-cluster-count-sized area; scans in 0x80-cluster windows (:2863–2909) for pages whose
first DWORD is **0x2b42534d "MSB+"** (:2904) **not already in the page cache** (i.e., not
reachable from the live checkpoint chain, :2910–2931); parses their records
(`FUN_18000ae70` :2932–2934) and processes them with the live dispatch (0x10/0x30-1/0x30-2,
:2942–2965) with **every record marked deleted (record+0x59 := 1, :2939)** → these become
deleted-file results through the normal emit path (deleted bit set at `FUN_18000ba90:8476`).

### 6.6 Emission, object-identity dedup, recycle-bin special-casing — `FUN_18000d970` :9727–10079
- Builds the heap `refs::RefsFileProperties` (0x8b) from the stack props (:9767–9802).
- **Recycle-bin routing** (:9805–9876): names starting **"$R"** (:9824–9829), **"S-"**
  (:9837–9843), or equal to **"System Volume Information"** (:9844–9861) go to the dedicated
  collection at ctx+0x40 (`FUN_18000f780` :9866) instead of the main map; flushed at
  `FUN_18000ef20:10881–10911`.
- **Object-identity keep-one**: when the props carry the identity flag (+0x2c bit0, :9904), an
  existing same-identity entry (map at ctx+0x10, searched by props+0x3a/node id :9905–9934)
  triggers name/priority comparison (`FUN_18000ac40` :9939) and a keep-one decision on
  {+0x43 priority, +0x41, +0x2f, +0x42} (:9942–9966) — the loser is destroyed (:10021–10027).
  Directory entries additionally tracked in the node map at ctx+0xa8 (:10015–10018).
- `FUN_18000e930` :10542–10610 — duplicate scan of the pending map (name-equality check).
- Flush `FUN_18000ef20` :10861–10972 — main map first (special-name 0x600 tagging :10914–10933;
  emit via ctx+0xa0 callback :10934–10939, loser destroyed :10941–10945), then the recycle-bin
  collection (:10881–10911).

## 7. Core-side capability map (MATCH / MISSING / DIVERGENT / REPORT-ONLY)

Core = `cr_refs_engine.cpp` (`scan_refs`) + `cr_refs_detect.cpp` (`detect_refs`). **The core is
a full recovery engine, not report-only**: `scan_refs` reconstructs the MSB chain, walks the
ObjectID/parent-child tables, emits files with names/timestamps/extents, harvests deleted rows
and orphan tables (`cr_refs_engine.cpp:604–920`). The report-only concern from the session-2
handoff is resolved — no capability-mismatch finding on that axis.

| Og capability (cite) | Core behavior (cite) | Verdict |
|---|---|---|
| Exports quick+complete controllers (:54–113); OS gate Win8+ (:37–49) | Single `scan_refs` + `detect_refs`; no host gate | **DIVERGENT-INFO** (I-011) |
| DBR gate: "ReFS"@+3, "FSRS"@+0x10, major==3&&minor>3 or major==1 (:13529–13531) | Same fields; accepts major 1..3 (:365–387) | **MATCH** modulo I-004 (superset, filed) |
| DBR geometry: bps ∈ {256,512,1K,2K,4K}; cluster ∈ {4K,64K} exactly (:13544–13546) | bps ∈ {512,4096}; spc any pow2 ≤ 128 (:369–374) | **DIVERGENT-MINOR** (N-029) |
| MSB area = cluster 30 + sb-overridable (:12925, :2460, :10373-74) | superblock probes cluster 30 + n−3/n−2 backups (:460–478) | **MATCH** (backup probing is core superset, tolerated) |
| SB → checkpoint pair at sb+0x28/+0x30 (:12974–12975); newer sequence wins (:1029–1031) | chkp offset u32@sup+0x70 → two clusters (:495–498); newer virtual clock at +0x60 wins (:523–526) | **MATCH** (session-1 A25; both select the newer checkpoint) |
| Checkpoint body: version re-gate, list @body+0x44/0x58 by body-minor, count@body+0x40, 0x30-byte refs with next-chains (:13744–13805) | identical gates and next-chain walk (:546–602) | **MATCH** (GAP-S6-3 fix re-verified) |
| Page sigs SUPB/CHKP/MSB+ first-DWORD, min 0x20, ref@+0x20 (§3.4) | Full-width memcmp (detect :39–53); engine checks page-sig DWORDs + volume sig (:111–119) | **MATCH at engine level**; detect-side = N-007 (filed) |
| Volume signature checked on every sig page (device+0xa8 compare, :13117–13145) | `volume_sig` from SB GUID XOR, checked on sig pages (:114–118) | **MATCH** |
| System-dir/container map + `((block/cluster)>>1)−2` translation, flag 0x20 skips (§4, §6.3) | `translate_cluster` top-nibble tag; **map never populated** (:72–82); no 0x20 equivalent | **DIVERGENT-MINOR** (N-030) |
| Records: 0x10 timestamps+names (§6.1), 0x30/1 files, 0x30/2 dirs (§5) | ROW markers 0x10/0x00010030/0x00020030 (engine :33–36, :804–847) | **MATCH** |
| File id = global counter (c060 :8620–8622) | `record_ref` = directory table id (:726) | MATCH-ish (D11 field semantics; not filed) |
| Names: UTF-16 key+4 → CP_UTF8 (c2b0 :8673–8696) | UTF-16 key+4 → u16string (:704–711) | **MATCH** (B10) |
| Timestamps: 4×16B from 0x10-records (cbd0 :9172–9185); dir values (c920 :9034–9051) | GeneralInfo MACE + dir-link value times (:267–280, :822–829) | **MATCH** (B9 raw 100ns; different row grammar documented in header :24–33) |
| **Attribute layer**: gen-gated grammars; all data-bearing attributes merged; indirect (paged) attributes chased; resident inline materialized; $I resident; runlist granularity validation (§6.2–6.3) | First `ATTR_DATA`(0x80) attribute only (`break` :759); val_len ≥ 0x28 required (:741); `parse_data_runs` embedded-node-only, rejects inner nodes (:293–310); 0xb0/0xc0 documented but unhandled (header :30–33); `resident_data` never populated (cr_node.h:69–74) | **DIVERGENT-SEVERE** (S-015) |
| Deleted model: stale-MSB-page sweep over the OID area, records force-marked deleted (§6.5); used-space marking for live files (:8451–8473) | Deleted = row flag 0x4 + ID2 evidence rows (:805–810, :848–875); allocator-bitmap grading (:653–691); **no stale-page sweep** | **DIVERGENT-MAJOR** (M-015) |
| Object-identity keep-one for duplicate names/hardlinks (d970 :9904–9966); recycle-bin $R/$I/S-/SVI special collection (d970 :9805–9876, ef20 :10881–10911) | One result per ROW_FILE row, no identity dedup (:805–810); no recycle-bin special-casing | **DIVERGENT-MAJOR** (M-016) |
| 0x80000020 (ID2) rows: **not handled** by the og page dispatch (§5 — only 0x10/0x30 reach processors; c060's 0x40/0x8000 acceptance is unreachable) | ID2 rows harvested as deleted-name evidence (:848–875) | **Core superset** (recovery-positive; noted, not filed) |
| Orphan/lost: stale-page sweep + root fallback via system-dir identity (§6.5, §4) | Orphan *tables* from the ObjectID map → lost tree (:902–920); root fallback = not-a-child table (:888–897) | **MATCH at feature level**; mechanism divergence in M-015 |
| Lost-partition probe: SUPB/CHKP DWORD probe + SB/CHKP validation (§3.6) | Not wired into candidate discovery | covered by **S-006** (lost-partition module) |
| Page cache: device map keyed by first LCN (:13158, :1616) | 256-entry cache keyed by first LCN, clear-all at 257 (:98–126) | **MATCH** (X51) |
| Read-error/used-space map marking (`FUN_1800089f0` call sites :2385, :8469) | Not present in the refs engine (writer-side concern) | INFO-level absence; folded into M-015 impact note |
| Quick-scan count/grade aggregation (a2c0 :7059–7202) | Single full walk; stats counters (:62–71) | **MATCH** at contract level (orchestrator-owned split) |

## 8. Doc-vs-decompile reconciliation (architecture/ + corpus docs)

1. `architecture/05-scan-engine-architecture.md` §5 "ReFSSearchDLL | OS-version gating, small
   engine (5 capa rules — much logic shared)" — confirmed: the OS gate is the host-Windows gate
   (`FUN_1800010d0:37–49`), **not** an on-disk version gate; the on-disk gates live in
   `FUN_180012390`/`FUN_180012650`. The doc's FS-code table "10 ReFS" is confirmed at
   `FUN_180010390:12125` (`*param_4 = 10`) and `FUN_180002130:847`.
2. `refs-engine--deep-dive.md` §3 "the engine distinguishes ReFS versions to pick the metadata
   layout" — confirmed and now precise: the layout-generation tag (3/7/8/9) is derived from the
   minor version at `FUN_180004980:2466–2477` / `FUN_180004bf0:2571–2596` and switches the
   attribute-record grammar (`FUN_18000c560:8784–8800`, `FUN_18000d770:9661–9673`).
3. `refs-btrfs-zfs-engine-algorithms-v2.md` §1 pseudo-code — matches `FUN_18000fc80` with the
   page+8 → page+0x20 nit (§1 above). Also its "header.size(+0x20) += ref.field(+0x30)" is
   confirmed but the ref "+0x30" field is a constant 0x30 set by the parser itself
   (`FUN_18000ff60:11874`), i.e. the page header is always 0x50 on valid pages.
4. `modules/refs.md` B7 row ("og's .mo analyser family covers them — CRefsAppDataRunItem") —
   the **scan DLL** itself has no ADS enumeration (§6); the .mo family reference is to
   `RefsFileSystemAnalyser.mo` (partition-side analyser, 1,752 classes per mo-module-map) and is
   **UNVERIFIED** for stream emission (see scratch report; next artifact = ghidra-mo
   RefsFileSystemAnalyser decompile). The DLL-side truth: one merged stream per object, with
   resident/inline and indirect-attribute content recovered (S-015).
5. `remaining-engines--ext4-hfs-refs-btrfs-zfs.md` §ReFSSearchDLL class list — all classes
   verified present; no contradictions.
6. `SaveScan/ifileproperties-part2--ext4-hfs-refs.md` §3 — "no ReFS volumes were scanned by the
   vendor QA" (no real-volume ground truth in corpus); the RefsFileProperties field table
   matches the decompile (`hasRun` at +0x73 ← props+0x62, `FUN_18000d970:9798–9801`).
   Its single `[INFERRED]` (objectId at +0x39) is consistent with the FileValues u32 path
   (`FUN_18000c060:8623` → `FUN_18000d970:9780`).
7. `CORPUS-ERRATA.md` — no ReFS-specific rows beyond E8/E9-style out-of-scope notes; nothing in
   the ReFS docs contradicts the decompile except the page+8 nit (§1) — logged here, not
   severe enough for an errata row.

## 9. Verification-strength note

Everything above is **decompiled evidence** (strength 2 per protocol §2): file:line citations
into the corpus decompile, cross-checked against the core source at HEAD `34b3441` read
first-hand. No executed comparison was possible this session (no ReFS volume fixture exists in
the corpus — the vendor QA scanned none, §8.6; the core's own fixtures are synthetic models).
Where the decompile alone cannot settle real-volume polarity (datarun flag bits, container-map
values, row-flag-0x4 existence), the finding text carries an explicit UNVERIFIED note with the
next probe.
