# MINOR N-142 — the dropped thumbnail variant's dead wiring survives in the tree: CrPreview::src_session / src_node_id are written at preview-open (cr_ui.cpp:3581-3582) and read NOWHERE — the promotion they were built for runs off cr_ui_thumbnail_generate's own arguments

**Severity:** MINOR
**Module:** abi/preview (the UI layer's preview handle) — modules/abi.md, modules/preview.md
**Parameters affected:** A54 (module boundaries — a dead cross-layer field pair), A69-adjacent (rebase-leftover hygiene), A43 (ownership and lifetimes — the fields hold borrowed pointers whose lifetime story is now misleading)
**Corpus references:** n/a — core-side hygiene defect introduced by the session-10/11 fix-window rebase (no og counterpart is claimed; the og's promotion site is DRWUI :422498-518, cited by M-083's retired file and ABI-CONTRACT §7i).
**Core references:** audited `743d7b8d` — `src/ui/cr_ui.h`-side struct at `src/ui/cr_ui.cpp:880-889` (`struct CrPreview { ... CrSession* src_session = nullptr; u64 src_node_id = 0; };`, the fields declared under an M-083 comment that promises "the thumbnail success path promotes THIS node's grade to 2"); the only writes at `:3581-3582` (`p->src_session = s; p->src_node_id = node_id;` inside the preview-open path); **zero reads** — `grep -n "src_session\|src_node_id" src/ui/cr_ui.cpp` returns exactly the declaration + the two writes (verified this session); the actual promotion runs in `thumbnail_generate_impl` (`:3750-3755`, landed by e56baac7/740d9b56) off its OWN `sess`/`node_id` arguments (`cr_ui_thumbnail_generate(dev, sess, node_id, ...)` at `include/cr/cr_ui.h:1278`).
**Status:** READ-ONLY FINDING — no fix applied
**Renumber note (session-11B push-time reconciliation):** filed as N-137 by this lane; renumbered **N-142** — the parallel S11 lane's N-137 (the thumbnail ABI header contradiction) pushed first and keeps the ID per the SA-9 rule. The two findings are adjacent (both are thumbnail-ABI-wave residues) but distinct: that one is the header's contract truth; this one is the dropped variant's dead struct wiring.
**Prior audit check:** newly discovered this session (the S11B/T4 landing-parity verification of the 740d9b56/e56baac7 thumbnail wave — residual R4). The parallel fix lane's own reconciliation (fce38c3d) records that "this lane's preview-handle variant of the same surface was dropped in the rebase resolution (duplicate of the landed design)" — but the drop was incomplete: the variant's CrPreview fields survived.

## Summary

The session-10/11 thumbnail ABI race produced two designs: the preview-handle variant (which stashed the originating session/node on the CrPreview struct so a later thumbnail call could find them) and the node-addressed variant (cr_ui_thumbnail_generate takes dev/sess/node_id directly). The node-addressed variant landed first and stands; the preview-handle variant was dropped at rebase — but its struct fields and their two write sites were left behind. They are dead: written at every preview-open, read by nothing. The M-083 comment above them describes a mechanism that no longer runs through them.

## What the reference does

Not applicable (no og counterpart; this is core-internal hygiene). The og's promotion site is the DRWUI thumbnail work item (:422498-518), already ported correctly by the landed design.

## What our core does

Every `cr_ui_preview_open`-class call allocates a `CrPreview`, sets `p->src_session = s` and `p->src_node_id = node_id` (:3581-3582), and never consults them again — the promotion path that motivated them (per the :883-886 comment) runs entirely off `cr_ui_thumbnail_generate`'s own arguments. The fields hold a borrowed `CrSession*` whose comment implies a lifetime contract ("the preview owns a private copy — the session may close independently" at :3575 makes the borrow safe) — but since nothing reads them, the contract is vestigial and misleading to the next maintainer.

## Gap

Falsifiable: `grep -n "src_session\|src_node_id" src/ui/cr_ui.cpp` yields exactly 4 hits — the two declarations (:887-888) and the two writes (:3581-3582). No read exists. The promotion at :3750-3755 does not dereference either field.

## Impact

No behavioral defect — the fields are written and ignored, and the landed promotion works. The cost is maintainability and truth: a dead borrowed-pointer pair that reads as load-bearing (the M-083 comment), a misleading lifetime comment, and a rebase-leftover that the next refactor could wrongly preserve or worse, wrongly wire. MINOR per the cosmetic/hygiene band, but it is exactly the residue class the trail's rebase reconciliations exist to catch.

## Reproduction / verification

Executed (protocol §6.1, static): the grep census above, run at `743d7b8d` this session; the fce38c3d commit message read first-hand (the drop record); the promotion site read at :3750-3755 with its own argument names. Cross-checked against the T4 agent's independent finding (same 4 hits).

## Fix plan (for the engineering team)

1. No architectural change — delete the dead pair or (if a future snapshot-promotion feature is planned) re-purpose deliberately.
2. Files: `src/ui/cr_ui.cpp` (:883-889 the struct fields + their comment; :3581-3582 the two writes).
3. Change: remove `src_session`/`src_node_id` and the writes; rewrite the M-083 comment at :883-886 to state the truth (the promotion runs off `cr_ui_thumbnail_generate`'s own arguments — ABI-CONTRACT §7i).
4. Tests: none required (dead-code removal); the existing `CrUi.ThumbnailSuccessPromotesGradeTo2` (test_cr_ui.cpp:3535-3569) and the preview suite pin the behavior — both must stay green (they never touched the fields).
5. Contract/doc: none (no ABI change).
6. Acceptance: the grep census returns 0 hits for both names; the full cr_ui suite green; no size change in any ABI struct (CrPreview is internal).

## Tests to add

None (the fix is deletion; the existing promotion tests are the pin).

## Related findings

M-083 / M-102 (retired — the landed design this residue came from), N-135 (the rebase-leftover bookkeeping class), I-028 (the thumbnail superset register — unaffected).

## Open questions

None.
