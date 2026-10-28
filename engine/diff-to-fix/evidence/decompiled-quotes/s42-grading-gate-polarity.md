# s42 evidence — the grading gate polarity decode (S-058 / S-061 / M-133 / M-134 / N-183's shared worksheet)

**Corpus:** `easeus-drw-research` @ `be0cfe9`; decompile = `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` (all line numbers re-read first-hand this session, 2026-10-29).
**Core:** `core_recovery` @ `b26017a7` (src/include byte-identical to the S12 live-verification point `25e408b8` — git-diff empty).

## §1 The comparator pair (the polarity's foundation)

```c
:324923  bool FUN_1401bebe0(undefined8 a, undefined8 b) {
:324925      char cVar1 = FUN_1401bec20(a, b);
:324926      return cVar1 == '\0';                    /* = !(a==b) = INEQUALITY */
:324929  }
:324931  bool FUN_1401bec20(longlong *a, longlong *b) {
:324933      return *a == *b;                          /* iterator node-pointer equality */
:324935  }
```

## §2 `FUN_140256d00` is `std::set::find` (the MSVC idiom, verbatim)

```c
:413990  undefined8 *FUN_140256d00(container, out, key) {
:414003      FUN_140256de0(container, &lower, key);      /* lower_bound (the _Tree descent
                                                            FUN_140256ed0 :414059-414083) */
:414004      end = FUN_1401be4c0(container, tmp);
:414005      if (FUN_1401bec20(&lower, end) == '\0') {   /* lower != end */
:414009          if (FUN_140060710(comp, key, *lower_elem) == '\0')  /* !(key < *lower) */
:414011              return lower;                        /* FOUND */
:414015      return end;                                 /* NOT FOUND */
```

`FUN_1401be4c0` = `end()` (:324474-324487: constructs the head-based iterator into the out-slot). `FUN_140256ed0` (:414059-414083) is the classic left/right tree descent — `lower_bound`.

## §3 The control case calibrates the direction (:585885-585905)

```c
:585888  local_78 = find(set, slot1, key);
:585889  local_80 = end(set, slot2);
:585890  cVar1 = FUN_1401bec20(local_78, local_80);   /* EQUALITY: TRUE iff find==end iff NOT FOUND */
:585891  if (cVar1 == '\0') { local_c8 = 1; }          /* FOUND → set c8 */
         else { … memcmp(name, const1/const2, 4) → maybe c8 = 1 … }
```

The whitelist idiom `found-in-set || name-matches-consts` reads correctly ONLY with `FUN_1401bec20 == 0 ⇔ FOUND`. Both comparators' directions are fixed simultaneously by this case.

## §4 The grading gate (:605736-605807)

```c
:605736  fs = FUN_140088770(node);                       /* FS kind */
:605737  if (fs != 3 && fs != 10 && fs != 0xB) {        /* NOT NTFS/ReFS/Btrfs → generic */
:605738      FUN_1403895c0(key, node);  FUN_140389400(key);   /* ext extract + toupper */
:605740      local_68 = FUN_140256d00(&DAT_1425e7980, s1, key);   /* find */
:605741      local_70 = FUN_1401be4c0(&DAT_1425e7980, s2);       /* end  */
:605742      cVar1 = FUN_1401bebe0(local_68, local_70);   /* TRUE iff FOUND */
:605743      if (cVar1 == '\0') {                        /* NOT IN TABLE → run the branch */
             …absAddr (:605752-756: kind∈{1,4}→−2)…
             …twin map +0x20 lookup → same 6-bit type → twin grade 2 (:605774-784)…
             …else → FUN_14038c470 deep read (:605798)…
             …FUN_140389540(node,1) mark-graded (:605800)…
:605804      } else { /* IN TABLE → nothing (cleanup only) */ }
```

**Conclusion:** the deep-validation branch runs for extensions **NOT in the 169-key set**. The s5 evidence file's §5 item 4 ("In-table (find != end) → deep-validation branch") is an **erratum** — it misread `FUN_1401bebe0` (inequality) as the equality test. The `""` corroboration: the og's runtime set contains `""` (init loop `:605297-605305`, 2000 iterations over the zero-padded table) → dot-less files are in-table → they skip the branch; the core's `EXPECT_FALSE(in_table(""))` produces the same skip — the two implementations agree on dot-less files only.

## §5 The deep read `FUN_14038c470` (:606291-606419) + the re-grade (:340852-873)

- Skip if overlap flag set (:606318); reader chain `FUN_1401b54f0/4fd0/50f0` (:606319-331); `sectorSize = vt+0x28()` (:606353); default grade 1 (:606354).
- addr==0: read 1 sector @0 → first 32 bytes all-zero → grade 0; first 4 bytes 0xFF → grade 0 (:606364-392).
- addr>0: read 1 sector @(addr−1) → last 16 bytes (sectorSize−0x10) all-zero → grade 2 / raw-flag (:606393-411).
- Mark graded (:606416-417); returns the tail-zero bool (consumed by the assistant rawCount rule :331283).
- Re-grade `FUN_1401d7110` (:340852-873): gate = grade==1 ∧ graded-bit clear → re-runs the FULL `FUN_14038ac90` tree (twins + table + deep read).

## §6 The device-walk + insert-hook rules (:605885-927, :605481-497)

- NTFS (:605893-907): fixer-index `vt+0xb0` ∈ [0,9) → grade 2 + `DAT_141be5818++` (:605899 — the gauge-matched global, NOT QSDE); `vt+0xb8()!=0 || vt+0x100()==0xa8` → grade 0 skip (:605901-907).
- Raw device `vt+0xe8` (:605912-921): size<0x400 → 2 else 0, mark-graded.
- kind ∉ {3} path: `kind∈{1,4} → offset−2` (:605924-927; the completion-pass twin at :605752-756). Kind space = FS codes (arch/04 §3.2 + engine-matrix :27): 0 FAT12, 1 FAT16, 2 FAT32, 3 NTFS, 4-5 EXT2/3, 7 exFAT, 8 HFS, 9 EXT4, 10 ReFS, 0xB Btrfs, 0xD ZFS → **shifted set = {FAT16, EXT2-family}**.
- Insert hook `FUN_14038a470` (:605481-497): device kind 0xB → grade 2 + 5830/5828 (:605485-489); kind 10 → grade 2 + 5830/5828 (:605491-496) — the ReFS/Btrfs insert rule (S-061).
- The member set (this+0x188, 20 lowercase keys, .rdata 0x1413ca6bc-824): doc pdf mov ppt xls cr2 arw dng cr3 mp4 orf raf docx pptx xlsx heic nef jpeg jpg mp3 — the gauge-candidate population (:605502-513: found → gauge counters; not-found → plain enqueue).
- Gauge (:340824-847): matched = quick counts + 5838/5828/5818; total = quickTotal + 5820/5830/5840; total<100 → matched=0. Score (:374451-481): `int((matched/total + 0.05) * 10)`.
- QSDE (:331164-170): the assistant field +0x1b8, incremented when bit 7 of +0x72 clear (the +0x1bc else-counter unlogged).

## §7 Core-side pins (at b26017a7)

- The inverted gate + the s5-carrying comment: `cr_grading.cpp:272-276`; the pinned test `test_grading.cpp:166-187`.
- The ReFS/Btrfs early-return: `cr_grading.cpp:217-219`; header note `cr_grading.h:20-27`.
- The twin-key family: `cr_grading.cpp:130-142` (fat_family = FAT12/16/32/exFAT).
- The QSDE tick on the NTFS rule: `cr_grading.cpp:207-216` (`/* DAT_141be5818 */` comment).
- The solo-only re-grade: `cr_grading.cpp:337-346`; the non-adoption registration `cr_grading.cpp:6-9`.
