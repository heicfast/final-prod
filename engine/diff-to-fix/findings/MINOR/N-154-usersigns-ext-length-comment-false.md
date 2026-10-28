# MINOR N-154 — the S-039 cap-annotation set carries one false data claim: the og's longest EUUserData extension is 8 ("manifest"), not 4 ("epub", "hqx")

**Severity:** MINOR
**Module:** carve (user-extensible signature DB — the S-039 caps family)
**Parameters affected:** A68 (documentation accuracy — in-code comment truth), C19-adjacent (the caps' stated corpus grounding)
**Corpus references:** `og-codebase/EUUserData` (the stock user-signature JSON, 111 keys / 190 signs) — key `"manifest"` (8 chars; `"mime": "text/cache-manifest"` family) and key `"class"` (5 chars) are the two entries longer than 4; measured this session: `max(len(k)) = 8`, the >4 set = `{"class", "manifest"}` exactly
**Core references:** audited `869da779` — `include/cr/carve/cr_usersigns.h:46` — `constexpr u32 kMaxExtLen = 8;    /* the og's longest ext is 4 ("epub", "hqx") */`
**Status:** READ-ONLY FINDING — no fix applied

## Summary

The S-039 hardening pass annotated every user-signature cap with the og's
observed maximum. The extension-length annotation misstates that maximum:
it claims 4 (citing "epub"/"hqx") where the og's stock database ships
`manifest` (8) and `class` (5). The cap itself (`kMaxExtLen = 8`) is
correct — it is exactly what lets the og's own `manifest` entry parse — so
there is zero behavioral divergence; the defect is the false provenance
comment, which would mislead any future caps audit into "correcting" the
cap down to 4 and rejecting the og's own stock DB (the S-039 failure mode
inverted).

## What the reference does

The og's EUUserData (verified by direct JSON parse this session):
111 top-level extension keys, 190 signs total. The length distribution's
top: `manifest` (8), `class` (5), then the 4-char family (`xlsx`, `pptx`,
`mpeg`, …). The og parser has no separate ext-length cap visible in the
corpus decode — the shipped data is the ground truth the S-039 caps were
derived from.

## What our core does

`include/cr/carve/cr_usersigns.h:46` sets `kMaxExtLen = 8` with the
comment `/* the og's longest ext is 4 ("epub", "hqx") */`. The value is
right (8 ≥ the true maximum 8 — an exact fit, no headroom, unlike the
neighboring caps which all carry explicit headroom); the comment is wrong.

## Gap

Falsifiable: parse `og-codebase/EUUserData`, compute `max(len(key))`. It
is 8 (`manifest`), not 4. The comment claims 4.

## Impact

No recovery-results impact (the cap admits every stock entry — the
111/111-key parse parity was re-verified this session with 0 diffs).
The impact is audit-trail integrity: the S-039 family's annotations are
cited as corpus ground truth in `cr_usersigns.h` and in the retired
finding's record; one false claim in the set undermines the set's use as
a derivation source, and a future "tighten the caps" pass reading the
comment would break og-stock-DB compatibility (manifest entries would
fail to parse — a silent capability regression).

## Reproduction / verification

Executed this session (session-42 config-asset deep-diff lane, re-verified
first-hand by the lead): `python3 -c "import json; d=json.load(open('og-codebase/EUUserData')); print(sorted(((len(k),k) for k in d), reverse=True)[:5])"`
→ `[(8, 'manifest'), (5, 'class'), (4, 'xlsx'), (4, 'pptx'), (4, 'mpeg')]`.
Core comment read directly at `include/cr/carve/cr_usersigns.h:46`.

## Fix plan (for the engineering team)

1. No architectural change.
2. Modify `include/cr/carve/cr_usersigns.h:46` — the comment only:
   `/* the og's longest ext is 8 ("manifest"); "class" is 5 — 8 = exact fit, no headroom */`
   (the neighboring caps' headroom convention makes the no-headroom fact
   worth stating: a 9-char ext in a user DB is a legitimate reject, and
   `manifest` at exactly 8 is the boundary case a test should pin).
3. No algorithm or constant change — `kMaxExtLen = 8` stays.
4. Tests to add: one pin in the existing user-signs test family —
   `UserSigns.ManifestEightCharExtParses` (a fixture entry with ext
   `manifest` and ext `12345678` parse; a 9-char ext rejects with the
   caps error) — fail-before/pass-after only on the comment-truth axis if
   the comment is promoted to a static_assert-adjacent test comment; the
   behavioral halves already pass.
5. Contract/doc updates: none (no public surface).
6. Acceptance criteria: the comment reads 8/"manifest"; the pin test
   exists; the S-039 family's other annotations re-spot-checked
   (kMaxSignBytes 44 hqx/img/eps — verified true this session;
   kMaxEntries 111 — true; kMaxSignsPerEntry 11 mpeg — true;
   kMaxOffset 526 — true; kMaxMimeLen 73 — true; only the ext claim was
   false).

## Tests to add

- `UserSigns.ManifestEightCharExtParses` (the exact-fit boundary pin)
- `UserSigns.NineCharExtRejected` (the boundary's reject side)

## Related findings

- S-039 (FIXED — the caps family this annotation belongs to; its own
  decode table carried the same "longest = 4" claim, corrected here)
- N-148 (the comment-truth batch — same class; N-154 filed separately
  because N-148's seven-item scope closed before this was found)

## Open questions

None.
