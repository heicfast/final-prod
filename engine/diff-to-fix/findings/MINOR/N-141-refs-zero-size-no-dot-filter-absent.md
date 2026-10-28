# MINOR N-141 — the og's ReFS zero-size∧no-dot record filter (a junk-suppression gate) is absent in the core: rows the reference suppresses are emitted as recoverable results

**Severity:** MINOR (false-positive-side divergence on a narrow row class; the core shows MORE than the reference, never less — no data loss, but junk rows the og deliberately suppresses appear as recoverable files)
**Module:** refs (ReFS engine row-emission filter)
**Parameters affected:** B3 (false-positive rate — rows the og filters as noise), B4-adjacent (the noise population's grading), D15-adjacent (the og's implicit confidence boundary: zero-size + extension-less = noise)
**Corpus references:** `re-workspace/reversed/ghidra-searchdll/ReFSSearchDLL_decompiled.c:8317-8337` (`FUN_18000ba90`: the `local_e8 < 1` size gate — the record's FileValues size at +0x30 via the `c060` read at `:8617` — combined with a `memchr(name, 0x2e, len)` dot-scan: within the zero-size class, records whose name contains no '.' take the filtered path); the M-022 OQ-3 lineage (the dot-prefilter's first decode — this walk pins it as a shared-processor gate with the size condition)
**Core references:** audited `46b81573` — `src/core/refs/cr_refs_engine.cpp:1101-1103` (the emission gate checks name emptiness and record-shape only — no size∧dot condition anywhere in the emit path; zero-size extension-less rows are parsed, graded and emitted)
**Status:** READ-ONLY FINDING — no fix applied
**Prior audit check:** M-022's OQ-3 registered the og's dot-prefilter as decoded-but-unported; this walk completes the decode (the dot condition is the SECOND half of a size-first gate) and registers the core-side absence as its own finding.

## Summary

The og's ReFS engine filters a noise class before emission: records whose
size field is zero AND whose name contains no '.' (the metadata remnants and
index residue that carry neither content nor an extension) are dropped
server-side. The core's ReFS engine has no equivalent filter — those rows
are emitted as graded recoverable files, so a core scan of a remnant-heavy
ReFS volume shows junk entries the og's user never sees.

## What the reference does

`FUN_18000ba90` (`:8317-8337`): the size read (`c060` → FileValues@+0x30)
gates the class (`local_e8 < 1`); within it, the name is scanned for a '.'
(memchr 0x2e with the trailing-suffix walk); the no-dot branch takes the
filtered path. The decode detail: the dot-scan is NOT a general filter —
it only applies to the zero-size class (a zero-size record WITH an extension
survives; a nonzero-size record without one survives).

## What our core does

No size∧dot condition at the emission gate (`:1101-1103` checks name
emptiness and `val_len` shape only) nor downstream (grading is
content-integrity-based). Zero-size extension-less rows are emitted.

## Gap

Falsifiable: a ReFS fixture with (a) a zero-size no-dot record, (b) a
zero-size record WITH an extension, (c) a nonzero-size no-dot record. og:
(a) suppressed, (b) and (c) emitted. core: all three emitted. The result
sets differ by exactly class (a).

## Impact

Result-noise parity on remnant-heavy volumes (post-format ReFS scans are
dominated by exactly this residue class); the false-positive-rate parameter
diverges in the core's permissive direction. No loss risk.

## Reproduction / verification

Decompiled + source (strength 2/3): the gate re-read first-hand at
`:8317-8337`; the core gate read at `46b81573`. Full walk:
`evidence/decompiled-quotes/s11-zfsrefs-function-walk.md` §3 C2.

## Fix plan (for the engineering team)

1. In `cr_refs_engine.cpp`'s emission path, add the og's compound gate AFTER
   the S-053 fix lands (S-053 removes the name-empty drop; this filter then
   applies to the survivors): if the parsed size is zero AND the name
   contains no U'.', skip the row. Keep the og's exact semantics — the
   filter is size-first, dot-second; do NOT filter zero-size rows that
   carry an extension, and do NOT filter nonzero rows.
2. Pin the boundary in a comment (the M-022 OQ-3 lineage + this finding).
3. Tests: `Refs.ZeroSizeNoDotRowSuppressed` (class (a) absent), 
   `Refs.ZeroSizeWithExtensionEmitted` (class (b) present),
   `Refs.NonZeroNoDotEmitted` (class (c) present) — the three-way boundary
   is the regression risk.
4. Acceptance: the three-class fixture yields identical row sets to the
   og-behavior oracle; the boundary tests pin both false-filtering
   directions.

## Tests to add

- `Refs.ZeroSizeNoDotRowSuppressed`
- `Refs.ZeroSizeWithExtensionEmitted`
- `Refs.NonZeroNoDotEmitted`

## Related findings

- S-053 (the same emission gate's opposite direction — fix both in one
  pass; the ordering note in the fix plan)
- M-022 (the dot-prefilter's OQ-3 lineage this completes)

## Open questions

None — the gate's compound semantics are decoded on both sides.
