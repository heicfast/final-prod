# MINOR N-207 — the PARITY-DELTAS born-collided-seat batch: NINE additional duplicate D-numbers (D-105/D-106/D-107/D-135/D-136/D-137) + the stale N-045 cache cite — discovered by N-191's new gate invariant

**Severity:** MINOR
**Module:** trail registry (docs/PARITY-DELTAS.md — the deliberate-divergence register's numbering; the N-191 collision class, discovered extension)
**Parameters affected:** Q46/Q68-class (documentation accuracy of the registry; D-number cross-reference integrity)
**Corpus references:** n/a (core-side registry bookkeeping — the same class as N-191)
**Core references:** discovered at HEAD during the N-191 fix: docs/PARITY-DELTAS.md rows at :3481/:3383/:3522/:3508 (D-105/D-107 x2/D-106 duplicates), :3953/:3994 (D-135), :3969 (D-136), :4060 (D-137); modules/storage.md:71's stale D-75 cite (the N-045 cache row is D-72 at :2219); the D-74 addendum false-positive class at :2195
**Status:** READ-ONLY FINDING — no fix applied at filing (fixed in the same session by the N-191 landing's extension — see Resolution)

## Summary

N-191's fix plan added the duplicate-D-number invariant to tools/doc_truth_gate.py.
Its FIRST RUN against the live registry surfaced nine MORE born-collided D-seats the
audit had not mapped (beyond N-191's own D-118 pair): D-105 x2, D-106 x2, D-107 x3,
D-135 x3, D-136 x2, D-137 x2 — every one the same hot-window racing-lane failure mode
(two lanes appending registry rows without checking the seat map). The same sweep
surfaced one stale live citation: modules/storage.md's N-045 cache-budget row cite
says D-75 where the row has been D-72 since the registry's own D-72-collision errata.

## Gap

Falsifiable: the gate's first-run output (quoted in the N-191 Resolution) listed the
nine duplicate pairs/triples; `rg "^(###|\*\*)\s*D-(105|106|107|135|136|137)\b"
docs/PARITY-DELTAS.md` returned 12 rows before the repair.

## Impact

Registry cross-reference integrity — the same class as N-191, multiplied by nine:
any finding or doc citing D-105/D-106/D-107/D-135/D-136/D-137 was ambiguous between
two or three different deliberate divergences.

## Reproduction / verification

Executed class: the gate run itself (tools/doc_truth_gate.py check 5, first
execution this session, output preserved in the N-191 Resolution and the worklog).

## Fix plan

Fold into the N-191 landing (the same renumber pass): earlier-push-wins on every
collided seat (file-position order as the checkable tiebreak, noted in the registry
header), the later rows renumbered to the next free seats with renumber notes at
every live citation site.

## Resolution

**Status:** FIXED
**Fixed in commit:** (the S57 lane's N-191/N-207 landing — one commit, one push)
**Verified by:** tools/doc_truth_gate.py check 5 green: 135 unique D-rows, zero duplicates, zero ADDENDUM false-positives; the nine renumbered rows each carry an inline renumber note naming the seat's keeper; the stale storage.md D-75 cite re-truthed to D-72.
**Resolution notes:** The seat map after the repair: D-105 = the S-044 auto-save config-field rider (the embedded-fast-path orientation row -> D-133); D-106 = the N-105 Folder-suffix deferral (the missing-member recovery policy row -> D-138); D-107 = the M-082 offline-OS layering rider (the S-047 TS null-packet row -> D-132; the overlap-mapping conservative-families row -> D-139); D-135 = the N-163 sparse-first registration (the N-177 damaged-marker compute row -> D-128; the N-090 session-list row -> D-129 with its a/b/c sub-letters); D-136 = the S-060 metadata-restoration translation (the N-165 FAT out-of-volume stance row -> D-130); D-137 = the S-050 file-repair scope exclusion (the N-169 EUUserData merge row -> D-131); D-75 = the M-033 BitLocker FVEK row (the N-070 carve validator-chain row -> D-140). Live citations updated at every site (modules/{ext4,session,fat,carve,ldm,storage}.md, docs/ABI-CONTRACT.md, the six finding files, the FIXED-LEDGER rows). The gate's ADDENDUM exclusion (the D-74 supersession continuation form) is documented in the check itself. Discovered-and-filed per the mandate's §8 during the N-191 verification — announced in the landing commit's message.
