# OQ-S6-3 — CRAW reader semantics: the +0xF4 boolean and the anchor-B tail offset (session/record-4 layer)

**Filed by:** session-6 deep-continuation audit (agent B session-format read)
**Status:** OPEN — needs the CRAW reader decompile (`FUN_140c40d80` cited by the corpus artifact)
**Blocking:** weakens/strengthens the tail-anchor sub-item of M-041 and the RAW payload field set of N-050

## The precise questions

1. **What is the u32 boolean at +0xF4 of the carved-file record (2,431/12,147 = 1 in the sample)?** (ifileproperties-part3:88.) Hypotheses: (a) a "tail-confirmed" flag (the file's tail anchor was validated); (b) a truncated/salvaged marker; (c) an inline-vs-extent data flag. The distribution (20%) does not obviously select any of these.
2. **Is the +0x94 anchor-B (tail offset) a reader-consumed field or write-only provenance?** If the reader consumes it (e.g., to bound reads or verify tails), the core's folding of the tail anchor into extent length loses behavior; if write-only, M-041's fix need not add a separate field.
3. **Does the reader's `FUN_140c40d80` validate the analyser checksum (+0x56) on load**, or is it informational-only? (Affects whether the core should persist a validator verdict for session-integrity checking.)

## The corpus artifacts that must be examined next

- `SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md` (the field table + the reader function name `FUN_140c40d80`).
- The CRAW/RAWSearchDLL decompile tree under `re-workspace/reversed/` at `FUN_140c40d80` — read the +0xF4/+0x94/+0x56 consumers.

## Why it was not resolved this session

The reader-side semantics require the decompile walk; the artifact documents the writer-side field census only.

## Related findings

M-041 (carve emission — the validation-payload/tail-anchor items), N-050 (node-field gaps — the RAW payload field set).
