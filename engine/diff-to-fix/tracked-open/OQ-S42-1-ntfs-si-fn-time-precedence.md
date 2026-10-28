# OQ-S42-1 — NTFS $STANDARD_INFORMATION vs $FILE_NAME time precedence: which attribute's times does the og emit (and when)?

**Origin:** S42 continuation lane, timestamp focus battery (Agent B's context sheet, residue #5)
**Status:** OPEN (this session) — the core asserts parity without og decode

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
