# MAJOR M-122 — The StbioSource single-shot read broke every photo-scale PNG decode (the retroactive filing of the trail's never-filed §8 discovery; renumbered THIRICE at the hot-window reconciliations: draft M-112 per the fix lane's ledger row [the ACTIVE S11B CSL finding owns M-112] → the S12-planned M-118 [the remote S12 lane's MountPoints2 owns it] → draft M-121 [the remote's Dbx-analyser renumber chain M-117→M-118→M-121 owns it] → M-122 per earlier-push-wins)

**Severity:** MAJOR
**Module:** preview (thumbnail engine)
**Parameters affected:** B1 (decode correctness), P47/P48 (throughput/latency under the band), Q69 (stub/stand-in hygiene — the single-shot read was a silent partial-read)
**Corpus references:** ghidra-drwui/DRWUI_decompiled.c ~:422425-429 (FUN_1402636e0 — the og's still-image eligibility gate `if (0xc800000 < lVar4)`); the og's decode path streams the file through its reader (the 200 MB anchor is the only size bound — no 4 MiB clamp exists on the decode read side)
**Core references (at the reconciled tree `4f8fa661`/origin-main):** `src/core/preview/cr_thumbnail.cpp:163-181` (the read loop as landed — verified standing at the reconciled HEAD this lane); the fix commit **`76e2d351`** ("fix(preview): M-109 + M-112 - the og's 200 MB thumbnail eligibility anchor adopted (the refuted no-cap doctrine re-pinned at all four sites), and the latent large-IDAT decode defect the band test exposed (the StbioSource single-shot read vs the 4 MiB clamp) fixed with the read loop")
**Status:** RESOLVED — fixed at `76e2d351` (with the M-109 anchor landing in the same commit); filed retroactively by the S42-B lane per the trail's §8 never-filed-finding duty (the S12 lane's Wave-1 claim, twice interrupted; see the renumber history and the Resolution below)

## Summary

The thumbnail engine's `StbioSource::read_cb` issued ONE `read_at` call per
stb-input request, but `PreviewReader::read_at` clamps any single read to
4 MiB (`kMaxRead`). Any PNG whose IDAT stream spans a >4 MiB request — every
photo-scale PNG once the M-109 landing raised the eligibility band to the og's
200 MB — returned a short read that stb_image treated as end-of-stream:
`outofdata`, decode failure. The defect was DISCOVERED by the M-109 landing's
own band test, fixed in the same commit by looping `read_at` until the
request is satisfied or the pipe returns 0, and then **recorded as a finding
in the fix commit's message and ledger row but never given a finding file** —
the ledger row that recorded it (rows :181-182 lineage) cited a path that
never existed and a phantom fix SHA (`5103ecd8`, in no ref). This file is the
retroactive artifact the trail's own SOP requires.

## What the reference does

The og's DRWUI thumbnail path gates a still image by FILE SIZE ONLY: the
`FUN_1402636e0` prologue reads the file length and skips the file when
`0xc800000 < lVar4` (the 200 MB anchor, DRWUI_decompiled.c ~:422425-429,
verified first-hand by the S42-B/W2 pass). Past that gate the og hands the
whole file to its image decode stack; no intermediate reader clamps or
fragments the request — the decode input is the file, streamed. (The corpus
carries no 4 MiB decode-read clamp anywhere in the DRWUI thumbnail chain; the
only chunking constants in the og's transport layer are the 4 MiB DEVICE /
7 MiB VOLUME scan-read chunks, M-030's decode, which live in the scan pipe,
not the preview reader.)

## What our core does

The core's `StbioSource` (the stb_image input adapter over the
`PreviewReader`) wrapped `read_at` in a single-shot call: one request, one
call, whatever came back was the whole answer. `read_at` — the same reader
the scan engines use — clamps a single transfer to 4 MiB. The M-109 landing
(`76e2d351`, the same commit) raised the eligibility anchor from the
pre-fix 32 MiB to the og's 0xC800000, which admitted exactly the
photo-scale PNGs (>4 MiB IDAT streams) that the clamp then truncated. The
band test caught it: a within-og-band fixture that decoded as a short
`outofdata`. The same commit's fix looped the read (standing verbatim at the
reconciled tree, cr_thumbnail.cpp:163-181, re-verified this lane):

```cpp
/* N-112 (discovered during M-109's band test): LOOP the read - ... */
u32 got = 0;
while (got < want) {
    const n = s->pr_.read_at(s->pos_ + got, data + got, want - got);
    if (n == 0) break;
    s->b_.spend_read(n);
    got += n;
}
```

## Gap

Falsifiable claim: before `76e2d351`, any thumbnail request for a PNG with a
compressed IDAT stream >4 MiB failed with a short-read `outofdata` under the
core, while the og — whose only gate is the 200 MB size anchor — decoded it.
The claim was EXECUTED: the M-109 band test exposed it during the landing
(the `Thumbnail.LargeStillImageWithinOgBand` regression proof), and the
S42-B/W2 pass re-ran the proof green at the pinned tree alongside
`Thumbnail.CapsDefaultsPinTheCorpusAnchors` and
`Thumbnail.OversizePastOgGateRefused`.

## Impact

User-observable: photo-scale PNGs (>4 MiB IDAT, well inside a 200 MB file)
produced no thumbnail and a failed preview — the exact class the M-080
thumbnail-budget family was filed to prevent. Multiplied by the M-109
band-raise: every within-band large PNG was affected until `76e2d351`. No
data loss (the scan/recovery paths never use StbioSource); a preview-only
correctness hole with a hard-to-diagnose symptom (short reads masquerading
as corrupt images).

## Reproduction / verification

1. **Executed (the landing's own band test):** the `76e2d351` diff introduced
   `Thumbnail.LargeStillImageWithinOgBand`, which plants a >4 MiB-IDAT PNG
   inside the og band and asserts a full decode; RED pre-fix (short read →
   `outofdata`), GREEN post-fix. Re-ran GREEN by the S42-B/W2 pass at the
   pinned tree, together with the whole `Thumbnail.*` family.
2. **Executed (the S42-B baseline):** the full `cr_tests` suite at the pin
   ran the loop-affected paths green (1091 pass / 42 env-skips / 1 unrelated
   fail — the N-157 red, not this finding's scope; the reconciled remote
   tree has since run 1104/0-fail/85-skip per the S42 lane's record and
   1215/1215 after N-152's landing per the 43-minors lane's record).
3. **Trail bookkeeping verification (the never-filed half):** the ledger rows
   :181-182 at the reconciled tree STILL cite fix commit `5103ecd8` — a SHA
   that exists in NO ref (`git rev-parse` + `git log --all` negative, the S12
   lane's probe, re-verified by S42-B at the reconciled tree) — and the path
   `findings/MAJOR/M-112-stbio-read-cb-single-shot-breaks-large-png-idats.md`,
   which never existed in any commit (the M-088 fictional-path class). The
   real fix commit is `76e2d351` (its message names both halves). This
   filing repairs both rows (see the trail repairs below).

## Fix plan (for the engineering team)

The behavioral fix is DONE (`76e2d351`); the remaining work is bookkeeping
the S42-B lane has executed trail-side (this filing + the ledger row
repairs) plus ONE code comment repair:

1. **The code comment** (`src/core/preview/cr_thumbnail.cpp:163`, verified
   standing at the reconciled tree): the read-loop comment says `N-112` —
   wrong on two counts: the finding was ledger-numbered M-112 at the landing
   (not N-112; N-112 is the LZH carve finding), and after this final renumber the
   correct cite is **M-121**. One-line comment repair for the next
   code-touching lane: `/* N-112 ... */` → `/* M-122 ... */`.
2. **The ID collision (the renumber this file executes):** the ACTIVE S11B
   finding `M-112-csl-sizefield-0x44c-branch-unported-unowned.md` owns M-112
   by the earlier-push-wins rule. The fix lane's row :182 claiming M-112 was
   the collision the S12 lane flagged; the S12 lane's planned renumber to
   M-118 was then itself consumed mid-window by the remote S12 lane's own
   MountPoints2 finding (its draft M-112 → **M-118**, an ACTIVE finding).
   Per the earlier-push-wins rule this filing renumbers to **M-121** (next
   free MAJOR: M-120 is the retired conflict-markers finding). The ACTIVE
   M-112-CSL file carries the collision note (S42-B/W1); the ACTIVE M-118
   (MountPoints2) is a different finding — any trail reference to "M-118" or "M-121" in
   a preview/thumbnail context means this file, M-122.
3. **Acceptance criteria:** the comment cites M-121; the ledger rows carry
   `76e2d351` + this file's FIXED/ path (M-122); no other file cites the phantom
   `5103ecd8` or the old M-112-as-Stbio usage (grep-clean trail-side; the
   `21b6c072` conform commit message is unrewritable history and carries the
   phantom token — the message-truth rule's standing note applies).

## Tests to add

Already present and green: `Thumbnail.LargeStillImageWithinOgBand` (the
>4 MiB IDAT regression proof), `Thumbnail.CapsDefaultsPinTheCorpusAnchors`,
`Thumbnail.OversizePastOgGateRefused`. No further test scoping is owed; any
future reader-clamp change to `read_at` re-runs the band family as its gate.

## Related findings

- **M-109** (the 200 MB anchor landing this defect rode — same commit, same
  band test); **M-080** (the refuted no-cap doctrine the anchor replaced);
- **N-148** (the comment-truth batch — RETIRED with all seven items landed,
  but its sweep predates this renumber: the cr_thumbnail.cpp:163 comment
  repair above is this finding's residual, not N-148's);
- **N-135** (the bookkeeping-truth batch — RETIRED; its phantom-SHA guard
  did not reach rows :181-182, this filing's trail repair does);
- **N-157** (the same fix-window's OTHER bookkeeping finding — the red-main
  test-truth class, convergent with this lane's independent reproduction).

## Open questions

None. The defect, its fix, and its evidence are all executed-verifiable; the
renumber and the phantom-SHA repairs are recorded trail-side by the S42-B
lane in the same wave as this filing.
