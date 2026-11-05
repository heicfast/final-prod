# S55-D3 context sheet — the grading deep-read subgates (FUN_1401bc5a0 / FUN_14038c470) + the QSDE gauge score formula

**Task:** S55-D3 (grading deep-read subgates decode + gauge formula). READ-ONLY audit; no file in either repo modified.
**Corpus:** `easeus-drw-research` @ `be0cfe9`; decompile = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (2,381,187 lines; cited as `D:` below). SearchDLL decompiles under `re-workspace/reversed/ghidra-searchdll/`. Raw binary `og-codebase/DRWUI.exe` (ImageBase 0x140000000; `.text` file-off 0x400).
**Core:** `core_recovery` @ HEAD `1fccb0a4`; `src/core/tree/cr_grading.cpp` + `include/cr/tree/cr_grading.h`.
**Method:** every DRWUI claim below was read first-hand this session (function bodies + callees one level deep); the one control-flow ambiguity (the pre-check's net grade) was settled by disassembling the shipped binary (objdump on the extracted `FUN_14038c470` bytes, file offset 0x38b870).

---

## (a) The FUN_1401bc5a0 + FUN_14038c470 decode

### a.1 What FUN_1401bc5a0 actually is — the extent-claimed bitmap probe

`FUN_1401bc5a0(mgr, node, addr)` (D:322676-322715) is a thin wrapper, not a grading rule itself:

```
D:322690  if (param_2 == 0) → garbage return
D:322701  if (param_3 == -1)  FUN_1401bc410(mgr,node,&addr,base,off,&unit,&key8)   // resolve the node's absolute extent address
D:322706  dev = FUN_1401b6b90(mgr)                                                  // singleton → vt+0x78 → current device object
D:322711  return FUN_14035abe0(dev, node, addr, unit)
```

The callee chain, one level deep:

- **`FUN_1401bc410`** (D:322630-322671) — the address resolver: `FUN_140088570(node)` → the node's FS object (D:322645); `idx = vt+0x30(FS)` (D:322647); `kind = vt+0xa0(FS)` (D:322648); `kind ∈ {1,4} → idx -= 2` (D:322649-651); `FUN_1401bb580(node,&base,&unit,&off)` (D:322653); **`addr = base + off + idx*unit`** (D:322663); out `key8 = (addr + (addr>>63 & 7)) >> 3` (D:322666) — the **÷8 bitmap key**.
- **`FUN_1401bb580`** (D:322031-322064) — the run-info resolver: `off = vt+0x68(device-for-node)` (D:322044-322047 — the volume data-start offset); `unit`/`base` are filled by the FS object's **`vt+0x30(FS, node, &base, &unit)`** (D:322052-322054) with a `vt+0x10(node,…)` fallback (D:322058-322061). **The `unit` out-param is the OQ's `local_b8[0]`** — the run stride (sectors-per-unit of the node's data run), not a device constant.
- **`FUN_1401b6b90`** (D:319457-319466) — `FUN_1401b19d0()` (global singleton) → `vt+0x78()` → the current device object (the same chain the reader resolution uses: `FUN_1401b4fd0` at D:318484-318485 is byte-identical).
- **`FUN_14035abe0`** (D:575161-575178) — `if (*(dev+0x10) == 0) → 0` (D:575170); else `return FUN_1404ec7d0(*(dev+0x10), addr) != 0` (D:575174) — i.e. **test one bit in the device object's +0x10 bitmap**.
- **`FUN_1404ec7d0`** (D:820130-820143) — the bit test: first the in-memory direct test `FUN_1404eccd0()` (D:820137; Ghidra elided the args), then, only if that is 0, the delegate object at bitmap+0x60 via `vt+0x20(delegate, bit)` (D:820138-820140).
- **`FUN_1404eccd0`** (D:820282-820296) — the raw bit test: `idx = bitmap[10] (+0x50 base) + bit`; bounds-checked against `bitmap[2] (+0x10 size-in-bits)`; `return data[idx>>3] & 1<<(idx&7)`.

**The bitmap class** (all in D:819957-820320): `FUN_1404ec520` (D:819998-820087) allocates it — bit count at +0x10, byte size at +0x08, data at +0x00; **file-backed mode** via `CreateFileW`/`DeviceIoControl 0x900c4`/`CreateFileMappingW`/`MapViewOfFile` when a path is configured (D:820023-820064), else a zeroed heap block (D:820067-820083); in-memory threshold at +0x68 = 0x5f400000 bits (ctor `FUN_1404ecd20`, D:820312-820318). **`FUN_1404ec740`** (D:820092-820125) is the set/clear function (`param_3==0 → clear, else set`, `count` bits from `base+start`).

**Who SETS the bits** (the semantic): `FUN_1401c79a0` (D:331079-331105) — called from the quick-scan populate walk — marks, for every node inserted into the result tree, the node's **data-run extents** into the ctx+0xb0 bitmap: `FUN_1404ec740(ctx+0xb0, (runStart*unit+off+base)>>3, 1, count>>3)` (D:331093-331097). The walk `FUN_1401c7b20` (D:331110-331178) then **skips entries whose key bit is already set** (D:331146-331148 — the dedup) and marks each processed node `FUN_140089680(node,1)` (+0x74 bit 0, D:331149). The FS engines use sibling bitmaps the same way (exFAT run extension probing at D:943009-943015; cluster-claim setters at D:179875/180046/180317).

**Answer to OQ item 1:** `FUN_1401bc5a0` = **"is this node's extent address already claimed/marked in the device-side extents bitmap?"** — an addr-map membership test against the "sectors already claimed by processed nodes" bitmap (device object +0x10), not a device-liveness probe.

### a.2 FUN_14038c470 — the deep-read grade validation, full decode

`FUN_14038c470(node, addr, bForced, bMarkBad)` (D:606291-606421). Signature notes: the decompiler prints `void`, but both callers read the return (D:331283) — **the return value is the "previous-sector tail-zero" bool (`local_1098`)**.

**Entry guards:**
1. `node == 0` → return (D:606318).
2. `FUN_1401c5da0(node) != 0` → return (D:606318) — the **merge-linked bit** (+0x73 bit 2; getter D:330210-330214; setter `FUN_1400895b0` D:134149-134166 which also forces grade ≥ 2 when setting it, D:134160-134162). Overlap-merged nodes are exempt — they were auto-graded 2 by the merge.
3. Reader resolution (D:606319-606331): `FUN_1401b54f0(cfg)` (current device) → `FUN_1401b4fd0(cfg)` (fallback) → if cfg-flag `FUN_1401b50a0` (D:318507-318522) → `FUN_1401b50f0(cfg,node)` (per-node device). None → return.

**Setup** (D:606332-606354): `zeros32` = 32 zero bytes; `ff32` = 32 × 0xFF (D:606343-606347); `buf` = 4096 zero bytes (D:606348-606352); **`sectorSize = vt+0x28(reader)`** (D:606353); `newGrade (local_1094) = 1` (D:606354).

**Sub-gate S0 — the three-part pre-check (bForced == 0 only)** (D:606355-606361):

```
A = FUN_1401bc5a0(cfg, node, addr)   // extent claimed in the device bitmap   (D:606357)
B = FUN_1401b3550(node)              // bit 7 of node+0x72                     (D:606358)
C = FUN_1403895a0(node)              // bit 2 of node+0x74                     (D:606359)
if (!A || !B || C) → do the reads    // (D:606358-606359, the goto)
else FUN_140089630(node, 0)          // (D:606360) — see the byte-level note below
```

**Byte-level control-flow note (settled this session from the shipped binary):** after `FUN_140089630(node,0)` at +0x1a8 the code is `jmp +0x354` (disasm of `DRWUI.exe` file-off 0x38b870+0x1ad: `e9 a2 01 00 00`) — it jumps **past the read block to the shared tail**, where `FUN_140089630(node, local_1094)` runs with `local_1094` still 1 (initialized at +0x137: `movl $0x1, 0x34(%rsp)`; the tail is `cmpl $0x1,0x34(%rsp)` → optional `FUN_140389570` → `call FUN_140089630(node, 0x34(%rsp))` → `call FUN_140389540(node,1)`). Because the og's grade setter is monotonic (see a.4), a 0 written at +0x1a8 is immediately overwritten by the 1 at the tail. **Net effect of the A∧B∧¬C path: grade 1 (Average), graded-bit set, NO sector read** — the `set_grade(node,0)` at D:606360 is transient and invisible. (This corrects M-134's item 3 and the OQ's "→ grade 0" reading; the pre-check is a *skip-the-read cache*, not a downgrade.)

**Sub-gate S1 — the tail-sector read** (addr ≠ 0; D:606393-606411): `reader->vt+8(buf, addr−1, 1)` (D:606396) — read one sector at `addr−1`. If the read succeeds:
- last 16 bytes (`buf[sectorSize−0x10 .. sectorSize)`, D:606398-606399) **all zero** → `local_1098 = 1` (the return value) and, if unforced, **grade 2 (High)** (D:606401-606404), then fall into S2;
- tail non-zero → if unforced, grade stays 1 (D:606407-606409); fall into S2;
- read failure → grade stays 1; fall through.

**Sub-gate S2 — the header-sector read** (unforced only; `LAB_14038c6d7`, D:606364-606392): re-zero the buffer (D:606370-606374); `reader->vt+8(buf, addr, 1)` (D:606377) — read one sector at `addr`. If the read succeeds:
- **first 32 bytes all zero → grade 0 (Low)** (D:606379-606383);
- else **first 4 bytes == FF FF FF FF → grade 0 (Low)** (D:606385-606389).
- otherwise the grade from S1 survives (2 if the tail was zero, else 1).
(Forced mode skips S2's grade effects: at `LAB_6d7`, `param_3 != 0` → `set_grade(node, local_1094)` + early return, D:606366-606369 — the forced probe only reports the tail-zero bool. addr == 0 unforced runs S2 at address 0; addr == 0 forced returns immediately.)

**Tail** (D:606413-606417): if final grade == 1 ∧ `bMarkBad` → `FUN_140389570(node,1)` (set **bit 1 of +0x74** — the "content unknown/bad" flag; setter D:605051-605056, getter `FUN_1401c5dc0` D:330219-330223); `set_grade(node, final)`; `FUN_140389540(node,1)` — set the **graded bit** (+0x73 bit 3; setter D:605041-605046, getter `FUN_1401d0030` D:337205-337209).

**Grade-transition table (unforced):**

| condition | grade |
|---|---|
| entry guard: merge-linked bit set | unchanged (returns early) |
| A ∧ B ∧ ¬C (pre-check) | **1** (no read; graded bit set) |
| tail sector (addr−1) last-16 zero | **2** |
| header sector (addr) first-32 zero | **0** |
| header sector (addr) first-4 = 0xFF | **0** |
| reads fail / content inconclusive | **1** |

(All writes go through the monotonic setter — a.4 — so a pre-existing grade 2 can never be lowered, and grade 0 only lands on nodes that were ≤ 1.)

### a.3 The sub-gate inputs (OQ items 2, 3, 4, 5, 6)

- **Bit 7 of node+0x72 (gate B; `FUN_1401b3550`, D:317506-317510):** the **quick-scan extend-map registration bit**. Set by `FUN_140638400` (D:1037338-1037368): when a node passes the controller's `vt+8` check and its `+0x50` key is not yet in the ctx's map → store the node as current, **`*(node+0x72) |= 0x80`** (D:1037359), insert the key. It is **persisted** in the node serialization (writer D:133357-133358; reader D:133393 and D:133401). Its two consumers: the QSDE split (D:331164-331170 — clear → `QuickScanDeleteExtend` +0x1B8++, set → the unlogged ninth counter +0x1BC++) and gate B here. Not a deletion-state bit; it marks nodes registered into the quick-scan "extend" map (the same population QSDE counts the complement of).
- **Bit 2 of node+0x74 (gate C; `FUN_1403895a0`, D:605061-605065):** the **"original drive-letter path recovered" bit**. Set at exactly two sites, both in path-scanning functions that walk wide strings for `':' (0x3a)` followed by `'\' (0x5c)` — i.e. `X:\`-style paths — and then call `FUN_140629120(path, node)`: D:1028061 and D:1034509 (`*(node+0x74) |= 4`). It is clone-copied with the other flags (D:133508-133511). Gate C set → the read happens even when A and B would skip it.
- **`vt+0x28` (OQ item 4):** `GetSectorSize` on the reader device — a **forwarding getter** (`FUN_1400d04e0`, D:183606-183618): `if (*(this+0x28) != 0) return vt+0x28(*(this+0x28)); return 0x200;` — **device geometry with a fixed 512 fallback** when no underlying device is attached. Used as `sectorSize` (S1's tail offset `sectorSize−0x10`) and implicitly as the read unit (the 4096-byte buffer accommodates a 4K sector).
- **`vt+0xb8` / `vt+0x100` (OQ item 5):** slots on the node's FS-controller object (`FUN_140088570(node)` = node+0x30; the controller classes live in the SearchDLLs — `IFileSystemSearchController`/`IQuickFileSystemSearchController`/`CNTFSQuickSearch`, NTFSSearchDLL_decompiled.c:256/292/311/354-361). Call-site semantics, fully decoded:
  - `vt+0xb8()` (no-arg bool): **≠ 0 → grade 0** (D:605901-605907 in the walk; D:605610-605625 in the type-3 insert path).
  - `vt+0x100()` (no-arg int): the **FS code field**. `== 0xa8` → grade 0 (D:605902) and blocks the fixer-module auto-High (D:585049); `!= 0xa8` enables the fixer-index map insert (D:605611-606616) and the auto-High (D:585050-585052); `== 0x23` additionally selects the ≤ `0x77fffff` (~120 MB) size arm of the quick-scan size rule `FUN_1401c76e0` (D:330974-330999: code 0x23 → size ≤ 0x77fffff; any other code → size ≥ one cluster, the divisor at global+0x70). **0x23 is the default**: the `CFileSystemNode` ctor writes `*(this+0x1f4) = 0x23` in DRWUI (D:134418) and identically in every SearchDLL (NTFSSearchDLL:39787, FAT:33549, exFAT:19915, ReFS:6393, Btrfs:11273, HFSPlus:17216, EXT4:13014, Zfs:39414); the field is copied node→node (NTFSSearchDLL:40397). **No writer of 0xa8 was found in any decompiled module** — the 0xa8 producer is the one residual unknown (it must be set inside un-decompiled vtable implementors or a runtime path; the partition-layer code space `FUN_14008b040` (+0x1f0, window [0x24,0x37], D:135456-135460) is a *different* field).
  - The 0xa8/0x23 partition-stride coincidence in the OQ is **disproved as coincidence-by-construction**: 0x23 is the ctor default of a real code field, 0xa8 a rare value of the same field.
- **`local_b8[0]` (OQ item 6):** the **run-unit multiplier** — the `unit` out-param of the FS object's `vt+0x30(FS, node, &base, &unit)` inside `FUN_1401bb580` (D:322054), i.e. the node's data-run stride (sectors per unit). It scales the address key (`base + off + idx*unit`); the core's `kDeviceUnit = 2^40` stand-in (cr_grading.h:75) changes only key scaling, not collision sets — the OQ's suspicion is confirmed.

### a.4 The grade field and setter (needed for every transition above)

- **Grade = bits 5-7 of node+0x73**; getter `FUN_140089670` = `*(node+0x73) >> 5` (D:134209-134213); default `0x20` (grade 1) in both ctors (CFileNode D:132984; CFileSystemNode D:134418-region; the engines emit grade 1 only — NTFSSearchDLL:39242/39779 per the s4 evidence).
- **Setter `FUN_140089630`** (D:134188-134204): `g==0` → clear **unless current grade is 2** (sticky); `g!=0` → set only if `current < g` (monotonic max). This is why the pre-check's transient 0 cannot survive the tail's 1, and why a pre-existing High can never be demoted by the deep read.

### a.5 When the deep read runs (callers, both verified)

1. **Lazily, at UI render time** — `FUN_14038ac90` (the per-node completion grader, D:605686-605815) is called only from `FUN_1401d7110` (D:340868), which is called only from the two Qt render sites: the file-list "state" column (D:1162488; the "Waiting..."/"Average"/"Low"/"High" string mapping at D:1162489-1162530+) and a file-detail pane (D:1478894). `FUN_1401d7110`'s gate: `grade == 1 ∧ graded-bit clear` (D:340862-340867) — one-shot per node. Inside `FUN_14038ac90`, the deep read fires only for: FS kind ∉ {3 NTFS, 10 ReFS, 0xb Btrfs} (D:605736-605737) ∧ extension **NOT** in the global 169-key table (D:605740-605743, the S-058 polarity) ∧ no same-type twin found in the address map (D:605774-605784) ∧ config gate `cfg+0x15a ∨ ctx+0x178` set (D:605789-605797, `FUN_1401c5e10` = `*(cfg+0x15a)`, D:330238-330242) — then `FUN_14038c470(node, addr, 0, 0)` (D:605798) + graded bit (D:605800). **In-table extensions take the else branch that does nothing and never sets the graded bit** (D:605804-605807) — they stay Average and remain re-run-eligible forever (a benign og quirk, see (e)).
2. **Forced, during quick-scan populate** — `FUN_1401c7d80` (D:331183-331311) at D:331283: for grade-1 **NTFS** nodes with `0 < addr < device-size`, `FUN_14038c470(node, addr, 1, 0)`; the returned tail-zero bool ∧ the size rule `FUN_1401c76e0` (D:330974-330999) tick the assistant **rawCount** (+0x1A0, D:331278/:331289).
3. **NOT on the background thread** — the "Recoverability analyze" walk `FUN_14038b0e0` (thread job registered D:605665-605675, log "Recoverability analyze begin." D:605664) does the fixer-index rule, the vt+0xb8/vt+0x100 grade-0 gate, the raw-device size rule and the overlap merges — **it never calls FUN_14038c470**.

### a.6 The surrounding walk rules (for the matrix)

- `FUN_14038b0e0` (D:605822-605964): NTFS type-3 nodes: `vt+0xb0(FS,&idx,…)` ∈ [0,9) → grade 2 + `DAT_141be5818++` (D:605894-605899); `vt+0xb8()≠0 ∨ vt+0x100()==0xa8` → grade 0 (D:605901-605907); raw-device flag `vt+0xe8()` → size < 0x400 → grade 2 else grade 0 + graded bit (D:605912-605923); else → the overlap accounting `FUN_14038ba60`.
- `FUN_14038ba60` (D:605969-606113): deleted (state 3) nodes → the +0x20 address map + `DAT_141be5840++` (D:606028); existing nodes → overlap vs the +0x20 map: same type → mark both merge-linked, tick 5838/5828 by the located-twin bit (`FUN_140389520`, +0x73 bit 4, D:606048-605053/:606084-606090), other's grade ≠ 2 → grade 2 + `DAT_141be5818++` (D:606091-606095).
- `FUN_14038bf40` (D:606118-606216): the final sorted two-map merge — same logic, `DAT_141be5818++` at D:606190-606191.
- `FUN_14038a470` insert hook (D:605434-605541): bit-6-of-+0x72 guard (D:605465); `FUN_140089170(node)==0 → grade 0` (D:605466-605469); kind 2 ∧ state 3 → `DAT_141be5840++` (D:605471-605474); category 0xc → grade 2 + 5830/5828 (D:605475-605480); FS kind ∈ {0xb, 10} → grade 2 + 5830/5828 (D:605481-605497 — S-061); the 20-key member set (ctx+0x188) split: found ∧ located-twin-bit clear ∧ FS≠3 → 5820++, found ∧ located-twin-bit set → 5830++ (D:605499-605524); not-found → plain enqueue.
- The 20-key member set is built in the grading-ctx ctor from `.rdata 0x1413ca6bc-0x1413ca824` (D:605218-605296); the global `DAT_1425e7980` map additionally seeds up to 2000 entries (9-byte stride) from `.data 0x141bb0500` when not yet built (D:605297-605305).
- The "existing-file fast grade" at insert (D:339891-339909): bit-6 nodes with size ≥ 1 → grade 2 + `DAT_141be5828++` + `DAT_141be5830++`.

---

## (b) The QSDE gauge score formula (N-183's subject)

### b.1 The counter globals (all first-hand)

| global | role | increment sites | reset |
|---|---|---|---|
| `DAT_141be5818` | **matched** | D:585051 (fixer-module auto-High: FS≠0 ∧ `vt+0x100≠0xa8` ∧ grade≠2 → grade 2), D:605899 (NTFS fixer-index rule), D:606093 & D:606191 (the two overlap merges) | D:605331 |
| `DAT_141be5828` | **matched** | D:605479/605488/605495 (insert-hook auto-High classes), D:606053/606089/606186 (merges, located-twin side) | D:605330 |
| `DAT_141be5838` | **matched** | D:606050/606086/606183 (merges, non-located side) | D:605329, D:605423 |
| `DAT_141be5820` | **total** | D:605518 (insert hook: member-set ∧ located-twin clear ∧ FS≠3) | D:605334, D:605424 |
| `DAT_141be5830` | **total** | D:605478/605487/605494 (insert-hook auto-High classes), D:605522 (member-set ∧ located-twin set) | D:605333 |
| `DAT_141be5840` | **total** | D:605473 (insert hook: kind 2 ∧ state 3), D:606028 (walk: deleted nodes entering the +0x20 map) | D:605332 |

### b.2 The composition and the score

`FUN_1401d6ff0(ctx, &matched, &total)` (D:340824-340847):

```
matched, total = FUN_14036e900(quickEngine, &matched, &total)   // the quick-scan engine's own counters (D:340831-340832)
log "Quick scan matched=%d, total=%d"                            // D:340833
if (total < matched) matched = total                             // D:340834-340836  (clamp #1)
matched += DAT_141be5838 + DAT_141be5828 + DAT_141be5818         // D:340837-340839
total   += DAT_141be5820 + DAT_141be5830 + DAT_141be5840         // D:340840-340842
if (total < 100 && *(ctx+0x10) == 0) matched = 0                 // D:340843-340845  (the small-result heuristic)
```

`FUN_14021b030()` (D:374451-374481) — the score:

```
obj = FUN_1401b7c70(FUN_1401b3ec0());  if (obj == 0) return 0;   // D:374460-374463
FUN_1401d6ff0(obj, &matched, &total);                             // D:374468
log "Recovery ability matched=%d, total=%d"                       // D:374469
if (total == 0) return 0;                                         // D:374470-374472
if (total < matched) matched = total;                             // D:374474-374476  (clamp #2, post-addition)
score = (longlong)(((double)matched / (double)total + 0.05) * 10.0)   // D:374477
```

**The formula, exactly:** `score = trunc10((matched/total) + 0.05)` — a double division, +0.05, ×10, truncation to integer. Output range 0..10 (matched==total → 10; matched==0 → trunc(0.5) = 0). The 0.05 is a rounding bias so that e.g. matched/total = 0.155..0.249… maps to 2. The og never shows the gauge for tiny results (total < 100 ∧ ctx+0x10 clear → matched = 0 → score 0).

**Population:** `total` accumulates only (i) the quick-scan engine's own total, (ii) the member-set existing-file ticks (5820), (iii) the auto-High class ticks (5830: category 0xc, FS 0xb/10, member-set located-twin), (iv) the deleted/lost class (5840). `matched` accumulates the engine matched, the merge/auto-High matched classes (5838/5828), and the NTFS fixer-index + overlap-merge ticks (5818). It is **not** "all files" and **not** "count(grade==High)".

### b.3 The QSDE field — verified end-to-end (the pointer-scaling trap)

`FUN_1401c6f10(undefined8 *param_1)` (D:330758) logs `"analyze QuickScanDeleteExtend = %d"` reading `*(undefined4 *)(param_1 + 0x37)` (D:330927-330928) — with `param_1` an `undefined8*`, **`+0x37` units = byte +0x1B8**. The increment site is D:331166 (`*(int *)(param_1 + 0x1b8) += 1` when bit 7 of +0x72 is clear, in `FUN_1401c7b20`'s quick-scan walk, D:331110-331178); the else-counter is byte +0x1BC (D:331169); the ctor `FUN_1401d0260` (D:337399-337433, also `undefined8*`) zeroes exactly the pair `param_1+0x37 (byte 0x1B8)` and `(longlong)param_1+0x1bc` (D:337428-337429). **N-183's "+0x1b8" is correct.** (Same scaling for the other log lines: rawCount = byte +0x1A0 (D:330915-330916) — ticked by the forced deep-read path at D:331278/:331289; LPExist +0x1A4, LPDelete +0x1A8, QSExist +0x1AC, QSDel +0x1B0, DeepScan +0x1B4, D:330917-330926.)

---

## (c) Core-vs-og rule matrix

| og rule (cited) | core status (cited) |
|---|---|
| Deep read S1: tail-sector (addr−1) last-16-zero → High (D:606396-606405) | **ABSENT** — no reader in `GradeContext` (cr_grading.h:77-87); registered non-adoption cr_grading.cpp:6-10 (M-134) |
| Deep read S2: header-sector first-32-zero → Low; first-4 = 0xFF → Low (D:606377-606389) | **ABSENT** — same (M-134) |
| Pre-check skip-cache A∧B∧¬C → skip read, grade stays 1, graded bit (D:606355-606361 + disasm) | **ABSENT** (folds into M-134; net-grade-1 semantics now portable) |
| Merge-linked exemption (+0x73 bit 2 → early return, D:606318) | **PARTIAL** — core sets overlap and grade 2 together (`set_overlap()` at cr_grading.cpp:293-294; cr_node.h:288-292), but has no read to exempt |
| sectorSize = vt+0x28, 512 default (D:606353; D:183606-183618) | **ABSENT** — no sector-size anywhere in the grading pass |
| Monotonic setter, grade-2 sticky (D:134188-134204) | **PORTED** — cr_node.h:280-286 |
| Grade default 1 (D:132984; engines :39242/:39779) | **PORTED** — cr_node.h:329 |
| size < 1 → Low (D:605726-605729) | **PORTED** — cr_grading.cpp:181-183 |
| category 0xc (email) → High (D:605731-605734; category = node+0x70>>2, D:133560-133564) | **PORTED** — cr_grading.cpp:186-191 (category from extension, not the node byte — equivalent surface) |
| existing (bit 6 of +0x72, size ≥ 1) → High at insert + 5828/5830 ticks (D:339898-339908) | **PARTIAL** — Active→High ported (cr_grading.cpp:194-197); the 5828/5830 gauge ticks absent |
| raw/carve: size < 0x400 → High else Low (D:605912-605921) | **PORTED** — cr_grading.cpp:200-203 |
| NTFS fixer-index ∈ [0,9) → High + 5818++ (D:605894-605899) | **PARTIAL** — High ported (cr_grading.cpp:208-215) but ticks `quick_scan_delete_extend` (the wrong counter — N-183 (c)) |
| vt+0xb8()≠0 ∨ vt+0x100()==0xa8 → grade 0 (D:605901-605907); 0xa8 blocks auto-High (D:585049) | **ABSENT** — the "UNDECODED - not adopted" register cr_grading.cpp:204-206 (now decoded; see DC-3) |
| ReFS/Btrfs insert auto-High + 5830/5828 (D:605481-605497) | **ABSENT** — S-061 (open) |
| Twin/overlap: same extent-end key + same type → both merge-linked + High; 5838/5828/5818 ticks (D:605769-605784, D:605969-606216) | **PARTIAL** — twin pass cr_grading.cpp:277-296 (flags + grade 2, no counter ticks, no deleted/existing map split); gate polarity inverted (S-058) |
| Twin key: base + off + (idx−2 for kinds 1/4)·unit (D:605752-605768, D:322663) | **PARTIAL** — cr_grading.cpp:130-142 (M-133: family set wrong; unit = 2^40 stand-in) |
| Forced quick-scan deep probe + size rule (code 0x23 → ≤0x77fffff; else ≥1 cluster) → rawCount (D:331283, D:330974-330999) | **ABSENT** — no forced probe; `raw_count` = Origin::Carved only (cr_grading.cpp:249-251) |
| Gauge population = member-set classes + engine counts (b.1/b.2) | **ABSENT** — matched = count(grade==High) over ALL files; total = total_file_count (cr_grading.cpp:319-332) |
| Score `trunc((m/t + 0.05)*10)` (D:374477) | **ABSENT** — no formula (N-183 (b)) |
| Small-result total < 100 → matched = 0 (D:340843-340845) | **PORTED** — cr_grading.cpp:333-335 |
| QSDE = per-node bit-7-clear count (+0x1B8, D:331164-331166) | **MIS-MAPPED** — core ticks it on the NTFS fixer-index rule (cr_grading.cpp:212-213, N-183 (c)) |
| Lazy per-selection re-grade = full per-node tree (D:340852-340873 → D:605686) | **PARTIAL** — `regrade_node` re-runs `solo_rules` only (cr_grading.cpp:338-347; M-134) |

---

## (d) Verdicts

**OQ-S42-3 — RESOLVED.** All six recorded questions now have first-hand answers: (1) `FUN_1401bc5a0` = the device-side **claimed-extents bitmap membership probe** (addr-map membership — the OQ's first guess was right in kind); (2) bit 7 of +0x72 = the **quick-scan extend-map registration bit** (setter D:1037359, persisted D:133393/133401 — neither deletion-state nor scan-phase); (3) bit 2 of +0x74 = the **drive-letter-path-recovered bit** (setters D:1028061/D:1034509); (4) `vt+0x28` = **GetSectorSize — device geometry, 512 default** (D:183606-183618); (5) `vt+0xb8` = a controller bool that forces grade 0; `vt+0x100` = the **FS code field** (0x23 default in every FS ctor; 0xa8 = the grade-0 class; 0x23 also caps the size rule at 0x77fffff) — call-site semantics fully decoded, the 0xa8 *producer* and the `vt+0xb8` *implementor body* remain the only un-decoded residue (they live in SearchDLL vtables; next probe: dump `IFileSystemSearchController`'s vtable from the DLL binaries and read slot 23/32); (6) `local_b8[0]` = the **run-unit stride** from the FS object's vt+0x30 run-info (D:322054) — the core's 2^40 stand-in is structurally sound. Additionally, the byte-level disassembly **corrects the recorded "→ grade 0" outcome of the pre-check to "→ grade 1 (Average), no read"** — the D:606360 set_grade(0) is transient. M-121's fix-plan step 4 and M-134's step 4 can now be executed as decoded.

**N-183 — STILL ACCURATE, all three sub-divergences confirmed and now fully quantified.** (a) The population is the member-set classes + engine counts (b.1/b.2), not all files; (b) the score formula is exactly `trunc((matched/total + 0.05) * 10)` with a total==0 → 0 guard and a post-addition matched ≤ total clamp (D:374477, D:374474-374476) — the core has none; (c) the QSDE mapping is wrong in the core (`/* DAT_141be5818 */` at cr_grading.cpp:213 labels the gauge-matched global, while real QSDE = assistant +0x1B8 ticking on the bit-7-clear population, D:331164-331166). The field-offset question that could have overturned (c) was checked and survives the `undefined8*` scaling trap (b.3). One refinement: the og's "matched" is **not** grade-High count even within its population — it is the ticked classes (fixer-index, merges, auto-High classes, engine matched), so N-183's fix-plan step 1 ("keep matched = grade-High within that accounting") is an approximation, not the og's arithmetic.

**M-134 — STILL ACCURATE in its core claim (the content-read grading and the full-tree re-grade are absent; the core re-runs solo rules only), with two refinements from this decode:**
1. The pre-check's recorded outcome "grade 0 without reading" (M-134 item 3) is **wrong at the net-effect level**: the shipped binary jumps to the shared tail after the transient set_grade(0), and the tail's set_grade(local_1094=1) + monotonic setter make the net grade **1 (Average)**. The pre-check is a read-skip cache, not a downgrade. (M-134's gap statement — "the og's grading flips to Low on zeroed content / High on tail-zero" — is unaffected; those are S1/S2.)
2. The timing picture is now precise: the deep read runs **lazily at UI render** (grade-query of an ungraded Average node — D:1162488/D:1478894 → D:340868) and **forced at quick-scan populate** for grade-1 NTFS nodes (D:331283); the background "Recoverability analyze" thread (D:605822) never calls it. M-134's phrase "the completion pass invokes it at :605798" should read "the lazy per-node grader invokes it". The deep read is also config-gated (cfg+0x15a ∨ ctx+0x178, D:605789-605797) — a gate neither M-134 nor the core model has.

---

## (e) New divergence candidates (both-side evidence, proposed severity)

**DC-1 — the forced quick-scan deep probe + the size rule + the rawCount tick (proposed MINOR, new finding or M-134 annex).**
Og: `FUN_1401c7d80` forces `FUN_14038c470(node, addr, 1, 0)` for grade-1 NTFS nodes during quick-scan populate (D:331277-331283); the returned tail-zero bool, ANDed with the size rule `FUN_1401c76e0` (FS code 0x23 → size ≤ 0x77fffff ≈ 120 MB; any other code → size ≥ one cluster via the global +0x70 divisor; D:330974-330999), ticks the assistant **rawCount** (+0x1A0, D:331278/:331289) — the counter the og logs as "analyze rawCount" (D:330915-330916). Core: no forced probe, no size rule, and `raw_count` counts `Origin::Carved` only (cr_grading.cpp:249-251) — a different population entirely. Impact: the assistant's rawCount stat and the grade-1-NTFS content verification both diverge; the affected surface is one assistant counter + the quick-scan-phase grade for NTFS nodes. MINOR (stats surface; the grade effect overlaps M-134's population).

**DC-2 — the vt+0xb8 / FS-code-0xa8 grade-0 gate (proposed MINOR, blocked-on-evidence).**
Og: `vt+0xb8() != 0 ∨ vt+0x100() == 0xa8` → grade 0 on the NTFS walk (D:605901-605907) and the type-3 insert path (D:605610-605625); `vt+0x100() == 0xa8` additionally blocks the fixer-module auto-High + 5818 tick (D:585047-585053). Core: no FS-code concept anywhere (the register at cr_grading.cpp:204-206 says "UNDECODED - not adopted"). Impact: on volumes whose controller reports the 0xa8 class (or the vt+0xb8 flag), the og demotes everything to Low and the core leaves Average/High — but the 0xa8 producer is unresolved (no writer in any decompiled module), so the affected population cannot yet be sized. MINOR, with a tracked-open residue: dump the `IFileSystemSearchController` vtable (slots 0xb8/0x100) from the SearchDLL binaries and find the 0xa8 writer before adopting.

**DC-3 — the gauge "matched" arithmetic is class-ticked, not grade-counted (N-183 refinement, fold into N-183's fix plan).**
Og: matched = engine-matched (clamped) + 5838 + 5828 + 5818 — each a *tick at rule sites* (b.1), not a grade==High census; several tick sites fire regardless of the final grade (e.g. the fixer-index rule ticks 5818 before the vt+0xb8/0xa8 gate can force grade 0 at D:605899→D:605902; the existing-fast-grade ticks 5828/5830 at D:339906-339907). Core: matched = count(grade==High) over all files (cr_grading.cpp:325-332). Even after N-183's planned member-set population fix, a grade-census matched will disagree with the og's tick arithmetic in the auto-High-then-demoted and merge corners. Fold into N-183 (its fix plan step 1 needs this correction).

**DC-4 (note, not a finding) — the in-table extension nodes never get the graded bit.**
Og: the in-table else branch (D:605804-605807) does not set the graded bit, so `FUN_1401d7110` re-enters the full `FUN_14038ac90` walk for them on every UI grade query (grade stays Average; the walk is a repeated no-op). Core: `regrade_node` is grade==Average-gated and idempotent (cr_grading.cpp:338-347) — behaviorally equivalent output, cheaper. Register as a deliberate non-adoption note (perf quirk, no grade divergence).

---

## Residual unknowns (for the next probe)

1. The **0xa8 producer** and the **vt+0xb8 implementor** — inside the SearchDLL controller vtables (`IFileSystemSearchController`/`IQuickFileSystemSearchController`; class names from `re-workspace/rtti_classes.json`, construction sites NTFSSearchDLL_decompiled.c:256/292/311/354-361). Probe: dump the vtables from the DLL binaries (RTTI-located), read slots 0xb8 (23) and 0x100 (32).
2. `FUN_14036e900` — the quick-scan engine's own matched/total source (D:340832; not yet read — it feeds the gauge's base terms).
3. The exact disk-layout meaning of `addr` vs `addr−1` (the tail read's `addr−1` is likely the last sector of the preceding allocation or the file's header sector depending on the resolver's 0/1-basis; the grade semantics are unaffected).

---

## (f) S55-D3 re-verification pass (second independent session; append-only)

**Method of this pass:** every load-bearing claim above was re-derived first-hand from the decompile (all functions in (a)/(b) re-read, including the full callee chains `FUN_1401bc410`/`FUN_1401bb580`/`FUN_1401b6b90`/`FUN_14035abe0`/`FUN_1404ec7d0`/`FUN_1404eccd0`, the walk `FUN_14038b0e0`, the merges `FUN_14038ba60`/`FUN_14038bf40`, the insert hook `FUN_14038a470`, the gauge pair `FUN_1401d6ff0`/`FUN_14021b030`, and the flag setters/getters `FUN_140089630`/`FUN_1400895b0`/`FUN_140089570`/`FUN_1401d0000`/`FUN_1401c5d80`/`FUN_1401c5e10`/`FUN_140389520/540/570/5a0`/`FUN_1401b3550`/`FUN_1401d0030`), and the binary disasm of the pre-check was re-run. **All claims in (a)-(e) reproduce.** The three previously-cited-but-unverified items were re-read and also reproduce: the sector-size forwarding getter `FUN_1400d04e0` (D:183606-183618: underlying device's vt+0x28, else constant 0x200); the bit-7 setter `FUN_140638400` (D:1037358-1037362: `*(node+0x72) |= 0x80` + key-map insert when the controller's vt+8 accepts the node); the existing-file fast grade (D:339891-339909: bit-6 ∧ size ≥ 1 → grade 2 + 5828/5830, size < 1 → grade 0). The disasm re-run (objdump, file-off 0x38b870): gate sequence `call 0x1bb9a0`(A)/`0x1b2950`(B)/`0x3889a0`(C), then `xor edx,edx; call 0x88a30` (set_grade(node,0)) at 0x38ba0e-0x38ba18, then **`e9 a2 01 00 00` → 0x38bbc4** (the shared tail: `cmpl $0x1,0x34(%rsp)` → optional `FUN_140389570(node,1)` at 0x38bbe1 → `mov 0x34(%rsp),%edx; call 0x88a30` at 0x38bbe6-0x38bbf2 → `call 0x388940` at 0x38bc01). The pre-check's grade-0 is transient; net grade 1. **Confirmed byte-for-byte.**

### f.1 New refinements from this pass (added to the record, none overturning)

1. **The return value is mode-dependent.** From the disasm: the forced-mode completion returns `local_1098` (`movzbl 0x30(%rsp),%eax; jmp 0x38bc08` at 0x38baf4); **every unforced completion returns constant 1** (`mov $0x1,%al` at 0x38bc06 before the epilogue), as does the merge-linked early-out (0x38b8c7) and the reader-resolution failure paths (0x38b8af returns 0 only for node==0 / no-reader). The sole return-value consumer (:331283) always calls forced mode, so the rawCount logic is unaffected — but a port must not read the tail-zero bit from an unforced call.
2. **The 5818 tick's guard asymmetry.** The NTFS fixer-index tick (D:605899) is unconditional *inside* the index∈[0,9) branch — it fires even when the grade was already 2 and before the vt+0xb8/0xa8 grade-0 gate can run (already noted in DC-3) — while both merge ticks guard on `other-grade != 2` (D:606091-606095, D:606188-606192). Double-counting a node that is both fixer-index-promoted and merge-promoted is impossible (mutually exclusive branches), but a node promoted by the fixer-index rule then demoted by the 0xa8 gate stays counted in matched — the og's matched is a tick-at-rule-event ledger, further from any grade census than N-183's fix plan assumes.
3. **FUN_140389570 (bit 1 of +0x74) is dead at both deep-read call sites** (grep: only D:605798 and D:331283 call `FUN_14038c470`, both with param_4 = 0). The "content-unknown" bit is never set through this function in the shipped paths; its getter `FUN_1401c5dc0` gates the stats populator (D:331260-331263) on a bit nothing sets here. Register as deliberate non-adoption (with DC-4).
4. **QSDE walker mechanics sharpened** (D:331110-331178): the run-registration into the ctx+0xb0 bitmap (`FUN_1401c79a0`, D:331157) happens only when the node's device `vt+0xe8() == 0` (non-carve devices — D:331154-331158); carved-origin nodes are counted but their runs are never registered, so they cannot suppress later overlaps; the +0x1c8 tick is conditional on `FUN_140089020(node)` (D:331150-331153) and +0x198 accumulates sizes (D:331171-331172). QSDE itself (+0x1b8) is therefore a bitmap-deduped per-node count over the quick-scan engine list, split by bit 7 — not reachable from the core's `quick_scan_delete_extend` tick site at all.
5. **`FUN_14036e900` partially resolved** (D:587395-587400): it forwards to `FUN_14044b010(engineObj+0xb8, &matched, &total)` on the global engine object (`FUN_14036eb60` = `&DAT_1425e7540`, D:587460-587464) — the engine's own counters at +0xb8; `FUN_14044b010`'s internals remain unread (residual #2 stands, now one level shallower).

### f.2 Verdicts of this pass (unchanged, now double-confirmed)

OQ-S42-3 stays **RESOLVED**; N-183 stays **STILL ACCURATE** (with the DC-3 arithmetic refinement); M-134 stays **STILL ACCURATE in its core claim** with the two refinements in (d) — the second session's independent disasm removes any remaining doubt about the pre-check's net-grade-1 correction. DC-1..DC-4 stand as filed; f.1-3 adds the dead-bit note to DC-4's register.
