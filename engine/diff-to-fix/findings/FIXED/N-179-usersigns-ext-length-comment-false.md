# MINOR N-179 (renumbered TWICE per earlier-push-wins: filed N-154 [consumed by the S42-main bookkeeping batch], then N-161 [consumed by the S12 GPT-span-underflow finding]) — the S-039 cap-annotation set carries one false data claim: the og's longest EUUserData extension is 8 ("manifest"), not 4 ("epub", "hqx")

**Severity:** MINOR
**Module:** carve (user-extensible signature DB — the S-039 caps family)
**Parameters affected:** A68 (documentation accuracy — in-code comment truth), C19-adjacent (the caps' stated corpus grounding)
**Corpus references:** `og-codebase/EUUserData` (the stock user-signature JSON, 111 keys / 190 signs) — key `"manifest"` (8 chars; `"mime": "text/cache-manifest"` family) and key `"class"` (5 chars) are the two entries longer than 4; measured this session: `max(len(k)) = 8`, the >4 set = `{"class", "manifest"}` exactly
**Core references:** re-verified at the S42-D merged tree (still live) — audited `869da779` — `include/cr/carve/cr_usersigns.h:46` — `constexpr u32 kMaxExtLen = 8;    /* the og's longest ext is 4 ("epub", "hqx") */`
**Status:** READ-ONLY FINDING — no fix applied. **Renumber note (twice):** filed as N-154 on the pre-merge branch (the S42-main lane's N-154 pushed first), renumbered N-161 at the S42-D merge (the S12 lane's N-161 pushed in the same window), now N-179. The defect is unchanged and re-verified live at each renumber. The defect is re-verified LIVE at the merged tree: `include/cr/carve/cr_usersigns.h:46` still carries the false "longest ext is 4" comment (grep at the merge). The parallel N-148 comment-truth batch (78e92955, all seven items) did not include this site.

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

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-179 landing — post-push SHA in the ledger row and worklog entry)
**Verified by:** `UserSigns.ManifestEightCharExtParses` (the og's longest stock key "manifest" at exactly 8 parses with its cache-manifest mime; "class" at 5 parses; a 9-char ext rejects with ERR_BAD_PARAMETER — the exact-fit boundary pinned from both sides) + the existing `UserSigns.ParseOgShippedExtremes`/`DefaultSetShipsAndLoads` stay green (the 111-key stock DB round-trip, 0 diffs). Suite green at the landing tree.
**Resolution notes:** Corpus re-verified first-hand this session: `python3 -c "import json; d=json.load(open('og-codebase/EUUserData')); ..."` → 111 keys / 190 signs, the length top `[(8,'manifest'), (5,'class'), (4,'xlsx'), (4,'pptx'), (4,'mpeg')]`, the >4 set exactly `{"class","manifest"}` — the finding's measurement reproduced exactly. The fix is comment-only as the finding's own plan specifies (step 2's exact wording adopted, extended with the boundary-test pointer): `kMaxExtLen = 8` stays (the exact fit — zero headroom, unlike the neighboring caps; the no-headroom fact is now stated), the false "longest ext is 4 ("epub", "hqx")" provenance replaced with the measured truth. No behavioral change (the finding's own "zero behavioral divergence" verdict — the cap admits every stock entry; the pin test now holds the boundary so a future caps-tightening pass cannot silently break og-stock-DB compatibility).
