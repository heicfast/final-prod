# OQ-S4-1 — The CRecoverability 169-entry extension table (blob at 0x141bb0500) was located but not dumped

**Status:** OPEN (session 4) · **Module:** drwui/grading · **Parameters:** P15 (confidence grading model — deep-validation extension gate)
**Audited commit:** `63a9167` · corpus `03ad46c`

## The precise question

S-023's decision tree gates the generic-path deep validation on the file's extension
being present in a **169-entry static table whose first key is the literal "Raw"**
(lookup at `DRWUI_decompiled.c:605297-605304`; table @ `.rdata` 0x141bb0500). The
table's remaining 168 entries were NOT dumped this session — the S-023 fix plan's
step 3 requires the exact entry set before the gate can be byte-matched.

## Corpus artifacts to examine next

1. `re-workspace/reversed/ghidra-drwui/DRWUI_decompiled.c` around 0x141bb0500 — dump
   the 169 entries (Ghidra: the table is a sorted string-map blob; `strings` + the
   decompile's DAT_ references will enumerate it).
2. Cross-check against `og-codebase/FileFilter.xml`'s 1,481 distinct extensions —
   the grading table is plausibly a subset (the "deep-validatable" formats).

## Acceptance

RESOLVED when the 169 entries are enumerated with extraction evidence; fold into
S-023's fix plan step 3 and re-pin its extension-gate test.
