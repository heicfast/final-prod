# s3-jpeg-eoi-encoder-correction — S-018 premise refuted (first-hand, diff-fix session 3)

**Corrects:** `findings/SEVERE/S-018-jpeg-eoi-policy-og-salvages-truncated-core-rejects.md`
(the finding's "What the reference does" premise) and the
`grep -c "0xd9" → 0` reproduction line in its Reproduction section.

## The grep artifact

The finding's corpus premise rests on a literal-text grep for `0xd9` over the
decompiled analyser bodies. Ghidra renders the byte constant `0xD9` as the
SIGNED CHAR literal `-0x27` when the decompiled compare operates on `char`:

```c
/* RAWSearchDLL_decompiled.c :39750 (inside FUN_180033a90, the MODERN
 * JpegAnalyzer body the finding cites as :39489-41110) */
for (; param_2 <= pcVar16; pcVar16 = pcVar16 + -1) {
    if ((*pcVar16 == -1) && (pcVar16[1] == -0x27)) {   /* *(-1) == 0xFF; -0x27 == 0xD9 */
        iVar7 = ((int)pcVar16 - (int)param_2) + 2;     /* size to the EOI pair, +2 */
        break;
    }
}
uVar21 = (uint)(iVar7 != 0);                            /* EOI-found flag */
```

Five `-0x27` (FF D9) sites exist inside the modern body (:39489-41110 rel. lines
217, 231, 262, 480, 501 — including the shared backward-EOI helper ending at
`joined_r0x000180034516`), zero in the legacy dispatcher body (:57080-57567,
which the finding also cites — that half of the claim stands).

## The accept path is EOI-gated in the modern analyser

The size-accumulation accept path the SOF0 arithmetic feeds is explicitly
gated on the EOI scan result:

```c
/* :39760 */
if (uVar21 == 0) goto LAB_1800342b6;   /* no EOI found -> early return,
                                        * NO size accumulation this pass */
```

`LAB_1800342b6` is the function epilogue (`__security_check_cookie` + `return`,
:39854). With no FF D9 pair in the scanned buffer the modern analyser does not
accumulate a size — the opposite of the finding's "neither generation requires
or even checks for FFD9".

## What remains genuinely open (moved to tracked-open)

1. The og analyser is STREAMED (pending-continuation model, :10114): an
   early return in one buffer pass is not necessarily a final reject; whether
   a no-EOI JPEG is eventually accepted through continuation (or the
   `ctx+0x128 = 0x7800000` 120 MiB default at :39757) is unresolved.
2. The legacy path (FUN_18004b170 :57080+, the FileJPGAnalyser fallback) has
   no EOI reference itself, but it DISPATCHES to JFIF/Exif/generic
   sub-analysers (`FUN_18004bbd0` / `FUN_18004b3b0` / `FUN_18004c1e0`,
   :57200 vicinity) whose accept criteria are not decoded. If any of the
   three accepts without an EOI, the finding's salvage claim could hold
   through the FALLBACK, not through the primary analyser.
3. The modern analyser's SOF0 gate has its own quirks the core does not
   share (e.g. the `400 < height` minimum-height gate at the LAB_180033d7e
   block) — orthogonal to S-018, recorded here for the next audit wave.

## Consequence for S-018's classification

The finding's divergence direction ("og salvages tail-truncated JPEGs; the
core's EOI requirement is a superset strictness") is NOT established by the
cited evidence; the primary analyser is EOI-gated exactly like the core's
validator (by a different mechanism — a BACKWARD scan for the last FF D9 in
the buffer vs our forward marker walk; which policy admits more is itself an
open question). Loosening `validate_jpeg` on the refuted premise would be a
premise-driven regression; per the session SOP the finding is reclassified
INVALID (premise refuted first-hand) and the residual questions are filed
as a tracked-open item with the exact decode list above.
