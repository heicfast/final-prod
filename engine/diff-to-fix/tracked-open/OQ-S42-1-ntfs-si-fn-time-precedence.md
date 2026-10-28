# OQ-S42-1 — NTFS $STANDARD_INFORMATION vs $FILE_NAME time precedence: which attribute's times does the og emit (and when)?

**Origin:** S42 continuation lane, timestamp focus battery (Agent B's context sheet, residue #5)
**Status:** RESOLVED (session-48: SI-preferred/FN-fallback decoded at NTFSSearchDLL :5336-5356 — the og loads FN times then overwrites with SI when the attribute parses; the core's has_si rule is semantically identical; MATCH, no divergence)

## The precise question

The og's NTFS engine emits per-file timestamps from `$STANDARD_INFORMATION` (SI) and/or `$FILE_NAME` (FN) — the two attribute blocks carry different times for the same record (SI is the user-visible set maintained by the OS; FN is the name-record set, often older or fresher than SI after deletion). Which one does the engine serialize into IFileProperties' +0x25/+0x2d/+0x35, and under what condition does it switch (deleted records? resident-FN-only orphans? empty SI)?

The core pins its own precedence at `src/core/ntfs/cr_mftwalk.cpp:868-877`: `props->times = has_si ? si_times : best->times` with a comment asserting it "mirrors the time-precedence rule" — but `diff-to-fix/modules/ntfs.md:52-54` records that the og's precedence was **never explicitly decoded** ("no observable contradiction in corpus; note only"). The asserted parity is therefore unverified: if the og prefers `$FILE_NAME` times (some recovery products do, because SI is zeroed by some deleters), deleted-file dates diverge per record.

## Why it was not resolved this session

The decode requires reading the NTFSSearchDLL decompile's attribute-serialization order (`NTFSSearchDLL_decompiled.c` — the $SI/$FN attribute walkers around the props fill sites) and cross-checking the shipped `.rsf` samples' timestamp populations against fixture records with known SI/FN divergence. The session's timestamp battery covered the wire census, per-FS decode, UTC/local, and the deleted-record carriers; the precedence question is the one undecoded seam. Per the no-hallucination rule it is tracked rather than asserted.

## Next probe (exact)

1. `re-workspace/reversed/ghidra-ntfs/NTFSSearchDLL_decompiled.c` — locate the IFileProperties time setters (the serializer sites for +0x25/+0x2d/+0x35; the SaveScan part1 census gives the getters' vtable slots +0x50/+0x58/+0x60 as the anchor) and read the attribute branch order around them.
2. Cross-check with the real `.rsf` samples (the corpus's three shipped sessions): select deleted NTFS records where the fixture's SI/FN would differ and compare the persisted values against an independent ntfs-3g dump of the same records.
3. If the og prefers FN: file the divergence (deleted-file timestamps wrong in core for SI/FN-divergent records — SEVERE candidate on the timestamp axis). If SI-first like the core: close with a MATCH addendum citing the decode.

## Consumers

S-057 (the restoration finding consumes whichever times the props carry), the ntfs module row B9, any future session's timestamp battery.

---

## Resolution (session-48 lane, 2026-10-29 — RESOLVED: **SI-PREFERRED, FN-FALLBACK — the og and the core MATCH, proven by decode**)

**Status:** RESOLVED (the precedence rule decoded first-hand; the core's asserted parity is
now verified, not assumed; no divergence to file).

**The decode (NTFSSearchDLL_decompiled.c:5336-5356, the props time-population site):**

```c
/* 1. the $FILE_NAME-sourced times load FIRST (the fallback): */
local_84 = FUN_1800437f0(fn);   /* creation (props +0x25) */
local_7c = FUN_1800437b0(fn);   /* write     (props +0x2d) */
local_74 = FUN_180043770(fn);   /* access    (props +0x35) */
/* 2. find $STANDARD_INFORMATION (attribute type 0x10): */
lVar10 = FUN_1800430b0(rec, 0x10, 0, NULL, NULL);
if (lVar10 != 0 && FUN_180044fb0(lVar10, size)) {   /* found AND parses */
    /* 3. OVERWRITE with the $SI-sourced times: */
    local_84 = FUN_180045090(si);   /* creation */
    local_7c = FUN_180045040(si);   /* write     */
    local_74 = FUN_180044ff0(si);   /* access    */
    /* + the DOS-flags merge (FUN_180044f60 — the N-012 record) */
}
```

The props fill then carries the three locals to IFileProperties +0x25/+0x2d/+0x35
(:5417-5419, the second props-fill site; the same locals also feed the :707-709 site).
The rule is UNCONDITIONAL on record state — no deleted-record gate, no orphan gate: SI
wins whenever the attribute is present and parses; $FILE_NAME is the fallback otherwise
(SI-absent or SI-unparseable records).

**Core comparison (re-read at HEAD):** `cr_mftwalk.cpp:868` — `props->times = has_si ?
si_times : best->times`, where `has_si` is set ONLY when `parse_standard_information(a,
si) == OK` (:95-100). **Semantically identical: SI-preferred-when-parses, FN-fallback.
MATCH.** The N-012 comment block (one line below) already carried this rule's
attribute-flags half with the FUN_180044f60 cite; this decode completes the TIME half at
the same site — the "mirrors the time-precedence rule" assertion is now decode-backed.

**The condition seam (recorded for precision):** the og's overwrite gate is
`FUN_180044fb0(si, size)` — an SI-body parse validation; the core's is
`parse_standard_information(...) == OK`. Both are "the SI attribute parses"; a
record whose SI is present but corrupt falls to FN times in BOTH implementations.

**Consumers closed:** S-057 — the restoration carries SI-when-parses times, parity
confirmed; the ntfs module row B9 — precedence now decode-anchored MATCH; the SEVERE
candidate (FN-preferred divergence) — NOT filed, refuted by decode. Cross-check probe #2
(.rsf samples) not required: the decompile is decisive and the rule is state-unconditional.
