# OQ-S42-2 — Btrfs nsec handling in the og: does the engine fold sub-second precision or carry seconds only?

**Origin:** S42 continuation lane, timestamp focus battery (Agent B's context sheet, residue #7)
**Status:** RESOLVED (session-48: the og FOLDS nsec — FUN_180006ba0's paired sec+nsec/100 fold decoded; core parity exact on atime/ctime/mtime; the otime conversion is the sole superset, D-59b anchored)

## The precise question

The og's Btrfs engine reads the inode timespec fields at atime@0x70 / ctime@0x7C / mtime@0x88 / otime@0x94 (`BtrfsSearchDLL_decompiled.c:4430-4446`) but **otime is never converted and +0x94 is zeroed** (the trail's btrfs module rows). The inode timespec's on-disk layout is `{sec: i64, nsec: i32}` — the question is whether the og's conversion reads and folds the nsec half into the emitted FILETIME (sub-second precision) or truncates to whole seconds like its ext4 sibling does (`_gmtime32_s` with `wMilliseconds=0`, the D-59a registration).

The core folds nsec: `src/core/btrfs/cr_btrfs_engine.cpp:873-880` via `unix_to_filetime_ns`. If the og truncates, the core is an I-006-family precision superset (benign, registerable); if the og folds, they match. Either way the trail currently has **no og-side nsec cite for Btrfs** (D-59b covers only the 3-vs-4 attribute count), so the parity claim for Btrfs sub-second precision is unverified.

## Next probe (exact)

`re-workspace/reversed/ghidra-btrfs/BtrfsSearchDLL_decompiled.c:4430-4446` — read the timespec load: a single `i32/i64` load at each field address followed by a seconds-only epoch fold (`*10000000` style) means truncation; a paired sec+nsec load with an nsec/100 addend into the FILETIME means folding. One read resolves it.

## Consumers

The btrfs module row B9, I-006's superset family register (this would join or be refuted), S-057's restoration finding (precision of what gets restored).

---

## Resolution (session-48 lane, 2026-10-29 — RESOLVED: the og FOLDS nsec; core parity exact on the three converted fields; the otime conversion is the sole superset)

**Status:** RESOLVED (the question answered decisively by the one-read probe the OQ defined;
the parity claim for Btrfs sub-second precision is now decode-anchored).

**The probe's answer:** `FUN_180006ba0` (BtrfsSearchDLL_decompiled.c:4429-4441, the inode
timespec converter) is a **paired sec+nsec load with the nsec/100 fold** — the og FOLDS
sub-second precision:

```
out+0x4a (ctime) = (u32)(in+0x84)/100 + ((i64)(in+0x7c) + 0x2b6109100) * 10000000
out+0x52 (mtime) = (u32)(in+0x90)/100 + ((i64)(in+0x88) + 0x2b6109100) * 10000000
out+0x5a (atime) = (u32)(in+0x78)/100 + ((i64)(in+0x70) + 0x2b6109100) * 10000000
```

`0x2b6109100` = 11644473600 = the 1970→1601 epoch delta; `nsec/100` = ns→100ns units
(sub-100ns truncated). **Truncation is REFUTED** — the hypothesis that Btrfs behaves like
the ext4 sibling (`_gmtime32_s`, wMilliseconds=0, D-59a) is wrong: the og is internally
inconsistent between its ext4 and Btrfs engines, exactly as the module doc's partial note
(:83) suspected.

**Core comparison (re-read at HEAD):** `unix_to_filetime_ns` (cr_types.h) is the SAME
formula — `(sec + 11644473600) * 10000000 + nsec/100` — and the core applies it at the
SAME offsets (INO_ATIME 0x70/+8, INO_CTIME 0x7c/+8, INO_MTIME 0x88/+8,
cr_btrfs_engine.cpp:873-878). **B9's sub-second precision verdict: MATCH** — not a
superset, byte-equivalent arithmetic on all three converted fields.

**The sole superset (bounded precisely):** the core ALSO converts otime/crtime
(INO_otime 0x94/+8, :880) — the og's converter never touches +0x94 (zeroed, per the
trail's standing note). This is I-007's 4-of-4 family, already registered as
**D-59b** — this decode ANCHORS that registration with the og-side proof: the superset
is the otime FIELD only; the nsec PRECISION is parity, not superset.

**Consumers updated:** B9 (btrfs.md) → MATCH on precision + the registered otime
superset; I-006's superset family — Btrfs does NOT join (it was never a precision
superset; only ext4 is, via D-59a); I-007/D-59b — confirmed, now decode-anchored;
S-057 (restoration precision) — UNAFFECTED, the restored times match the og to the
100ns on atime/ctime/mtime.


---

## Convergence addendum (S47 INFO-first lane, 2026-10-29 — pushed second per SA-9; both lanes' Resolutions kept)

**Status:** CONVERGENT-MATCH (this lane's independent decode reached the identical verdict first-hand: FUN_180006ba0 :4435-4441, the three-pair `(nsec/100)+(sec+0x2b6109100)*10^7` fold, unix_to_filetime_ns byte-equivalent; this file's one-read probe executed by two lanes with the same result)
**This lane's unique product:** the end-to-end PIN the audit lane's prose decode lacks — `Btrfs.MtimeNsecFoldsLikeTheOg` (tests/unit/test_btrfs_volume.cpp): the shared fixture's hello.txt inode now carries mtime nsec 123456789 and the surfaced modified time must equal `(11644473600+1600000000)*10^7 + 123456789/100` exactly; `Btrfs.EndToEndWalk` re-verified green on the fixture change (make_inode grew the mtime_nsec parameter, default 0 — every other fixture byte-identical). The whole-second "simplification" hazard is now test-guarded, not just documented. btrfs.md's B9 rows carry both lanes' notes (the auto-merge kept both).
