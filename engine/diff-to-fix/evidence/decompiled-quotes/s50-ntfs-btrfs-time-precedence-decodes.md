# S50 evidence — the tracked-open time-precedence probes decoded (OQ-S42-1, OQ-S42-2)

**Lane:** S50 continuation audit lane. **Corpus pin:** `be0cfe9`
(`re-workspace/reversed/ghidra-searchdll/NTFSSearchDLL_decompiled.c`,
`BtrfsSearchDLL_decompiled.c`). **Core pin:** `93461483` (code-identical to
`0d7b1d9d`). Every quote below was read first-hand by the lead; the parallel
probe agent's decode was independently re-verified line-by-line before filing.

---

## 1. OQ-S42-1 — NTFS $STANDARD_INFORMATION vs $FILE_NAME time precedence

**DECISION: SI-FIRST with FN-fallback — the core's asserted parity is
CONFIRMED (MATCH); the core's SI size-validity gate is STRICTER than the
og's (72 bytes vs 40) — a benign hardening superset, registered here.**

### The og decode (FUN_180006db0, the per-record props builder)

The record's time slots are seeded from the selected $FILE_NAME attribute,
then OVERWRITTEN from $STANDARD_INFORMATION when one is found and passes a
size-validity gate:

```c
/* NTFSSearchDLL_decompiled.c :5338-5351 (read first-hand) */
5338:  local_84 = FUN_1800437f0(lVar7);           /* FN wrapper +0x20 -> creation */
5339:  local_7c = FUN_1800437b0(lVar7);           /* FN wrapper +0x28 -> modified  */
5340:  local_74 = FUN_180043770(lVar7);           /* FN wrapper +0x38 -> accessed */
5341:  local_8e = FUN_1800436b0(lVar7);           /* alloc size */
5342:  lVar7 = FUN_180043610(lVar7);              /* real size */
5343:  lVar10 = FUN_1800430b0(param_2,0x10,0,(int *)0x0,(longlong *)0x0);
                                                   /* attribute-type 0x10 = $SI lookup */
5344:  if (lVar10 != 0) {
5345:    uVar5 = FUN_180045480(lVar10);           /* attribute length */
5346:    bVar3 = FUN_180044fb0(lVar10,(ushort)uVar5);
5347:    if ((int)CONCAT71(extraout_var,bVar3) != 0) {
5348:      local_84 = FUN_180045090(lVar10);      /* SI wrapper +0x18 -> creation */
5349:      local_7c = FUN_180045040(lVar10);      /* SI wrapper +0x20 -> modified  */
5350:      local_74 = FUN_180044ff0(lVar10);      /* SI wrapper +0x30 -> accessed  */
5351:      uVar4 = FUN_180044f60(lVar10);         /* SI flags -> DOS attr bits     */
```

The getters (all trivial loads, read first-hand):

```c
/* :53520-53524 */ FUN_1800437f0 { return *(undefined8 *)(param_1 + 0x20); }  /* FN creation  */
/* :53511-53515 */ FUN_1800437b0 { return *(undefined8 *)(param_1 + 0x28); }  /* FN modified  */
/* :53502-53506 */ FUN_180043770 { return *(undefined8 *)(param_1 + 0x38); }  /* FN accessed  */
/* :54436-54440 */ FUN_180045090 { return *(undefined8 *)(param_1 + 0x18); }  /* SI creation  */
/* :54427-54431 */ FUN_180045040 { return *(undefined8 *)(param_1 + 0x20); }  /* SI modified  */
/* :54418-54422 */ FUN_180044ff0 { return *(undefined8 *)(param_1 + 0x30); }  /* SI accessed  */
```

The og's SI size-validity gate — valueOffset + 0x28 <= attribute length
(i.e. at least 40 bytes of SI value must fit):

```c
/* :54409-54413 (read first-hand) */
FUN_180044fb0(attr, len) {
  return (ulonglong)*(ushort *)(param_1 + 0x14) + 0x28 <= (ulonglong)param_2;
}
```

Final serialization into the IFileProperties slots (the SaveScan part-1
census's +0x25/+0x2d/+0x35):

```c
/* :5416-5420 */
5416:  plVar13 = FUN_180002f90();                    /* new CNTFSFileProperties */
5417:  *(undefined8 *)((longlong)plVar13 + 0x25) = local_84;   /* creation */
5419:  *(undefined8 *)((longlong)plVar13 + 0x35) = local_74;   /* accessed */
5420:  *(undefined8 *)((longlong)plVar13 + 0x2d) = local_7c;   /* modified */
```

**Decode:** SI wins whenever a $SI attribute exists and its resident value
spans at least 40 bytes; FN times survive only on records with no
size-valid $SI (the typical deleted-record case). There is NO
deleted-condition on the override itself — the deleted state is carried
separately (bVar2=8 at :5374-5380).

### The core side (read first-hand, current tree)

```cpp
/* src/core/ntfs/cr_mftwalk.cpp:868 */
props->times = has_si ? si_times : best->times;
```

`has_si` is set only when `parse_standard_information` succeeds
(cr_mftwalk.cpp:95-101), which requires the resident span AND a value of at
least **72** bytes:

```cpp
/* src/core/ntfs/cr_attributes.cpp:163-170 (read first-hand) */
u32 parse_standard_information(const AttrView& a, StandardInfo& out) {
    const u8* d = nullptr;
    u32 dlen = 0;
    u32 rc = resident_span(a, d, dlen);
    if (rc != OK) return rc;
    if (dlen < 72) return ERR_TRUNCATED;
    ...
```

### Verdict

- **Precedence: MATCH.** Both are SI-first with FN-fallback. The OQ's feared
  inversion (og preferring FN) is refuted by the decompile; the core's
  "mirrors the time-precedence rule" comment is now evidence-backed.
- **Informational nuance:** the og accepts $SI values >= 40 bytes
  (valueOffset+0x28<=len); the core requires >= 72 (the full v3 SI incl.
  quota/security fields). On corrupt records with a 40-71-byte SI the og
  emits SI times where the core falls back to FN times. The core's stricter
  gate is a benign hardening superset (the og's accepted values in that band
  are truncated garbage); registered in the I-045 finding, no code change
  owed.
- **Consumers closed:** S-057's restoration finding consumes whichever times
  the props carry — the carrier is now proven equivalent; modules/ntfs.md
  B9's "never explicitly decoded" note is discharged.

---

## 2. OQ-S42-2 — Btrfs nsec treatment

**DECISION: FOLD — the og folds the nsec half (nsec/100) into the FILETIME;
the core's `unix_to_filetime_ns` is byte-for-byte the same arithmetic (MATCH).
The core's pre-1601 refusal guard is a benign hardening superset.**

### The og decode (FUN_180006ba0, at exactly the audit-trail-cited :4430-4446)

```c
/* BtrfsSearchDLL_decompiled.c :4435-4443 (read first-hand) */
4435:    *(ulonglong *)(param_2 + 0x4a) =
4436:         (ulonglong)*(uint *)(param_3 + 0x84) / 100 +          /* ctime nsec / 100 */
4437:         (*(longlong *)(param_3 + 0x7c) + 0x2b6109100) * 10000000;
4438:    *(ulonglong *)(param_2 + 0x52) =
4439:         (ulonglong)*(uint *)(param_3 + 0x90) / 100 +          /* mtime nsec / 100 */
4440:         (*(longlong *)(param_3 + 0x88) + 0x2b6109100) * 10000000;
4441:    *(ulonglong *)(param_2 + 0x5a) =
4442:         (ulonglong)*(uint *)(param_3 + 0x78) / 100 +          /* atime nsec / 100 */
4443:         (*(longlong *)(param_3 + 0x70) + 0x2b6109100) * 10000000;
```

Each converted field is a paired (i64 sec; u32 nsec) load — sec at +0x7c with
nsec at +0x84 (ctime), sec +0x88/nsec +0x90 (mtime), sec +0x70/nsec +0x78
(atime). `0x2b6109100` = 11644473600 (the 1970->1601 epoch bias in seconds);
the emitted FILETIME = `nsec/100 + (sec + bias) * 10^7`. **The nsec half is
explicitly folded** (100-ns FILETIME ticks), truncated only below 100 ns.
The constant appears exactly three times in the whole file (grep-verified) —
these three sites only. The otime timespec (+0x94 sec, +0x9c nsec) is NOT
converted by this function (only wholesale struct copies elsewhere — the
I-007 registered 3-vs-4 asymmetry stands).

### The core side (read first-hand, current tree)

```cpp
/* include/cr/cr_types.h:56-60 */
inline u64 unix_to_filetime_ns(i64 unix_sec, u32 nsec) {
    constexpr i64 EPOCH_DELTA = 11644473600LL; /* 1970 -> 1601 */
    if (unix_sec <= -EPOCH_DELTA)
        return 0;
    return u64((unix_sec + EPOCH_DELTA) * 10000000LL) + u64(nsec) / 100u;
}
```

Applied to all four timespecs at src/core/btrfs/cr_btrfs_engine.cpp:873-880
(atime/ctime/mtime/crtime). The fold arithmetic — `(sec + bias) * 10^7 +
nsec/100` — is identical to the og's `nsec/100 + (sec + bias) * 10^7`.

### Verdict

- **Fold vs truncate: MATCH.** The og folds; the core folds; the exact
  addend matches. The OQ's premise ("no og-side nsec cite") is discharged —
  and corroborates modules/btrfs.md:80-84's own cite, which this decode
  re-verified first-hand at :4435-4443.
- **Informational nuance:** the core refuses pre-1601 times (`sec <= -bias`
  -> 0) where the og would wrap; benign hardening, registered in the I-046
  finding, no code change owed.
- **The crtime asymmetry is pre-registered:** the og emits 3 converted
  times; the core emits 4 (crtime from otime) — the I-007 INFO finding
  already owns this superset; no new finding.
