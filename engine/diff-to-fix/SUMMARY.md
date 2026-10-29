# SUMMARY.md — executive summary (the S87 MAJOR-first 20+ fix lane on top; the prior lanes below, newest-first)

## Session-87 S87 fix-lane delta (2026-10-30 — the user's fix mandate ["fix all the audit findings(priority to major first) one by one... Aim for atleast 20+ fixes in this session, put fixed findings in fixed directory. Update all docs , counts and all properly"]; opened at core `ddab68b4` [clean tree], corpus `be0cfe9` throughout; five racing windows absorbed and reconciled at the pushes; prompt saved token-redacted at worklog/session-diff-fix-s87-prompt.md)

- **M-107 CLOSED (the session's headline): the eleven-member
  enumeration complete, the finding moved to FIXED/.** This lane's
  unique landings: **member 6 (WPD)** — the version-table walk decoded
  first-hand (FUN_180041f50 + the 36-byte .rdata table at 0x18006ef40,
  the raw dump; convergent with the racing S84/89 lane's landing —
  this lane's five-test battery rides as the verification: the
  over-cap 72 MiB carve, the unknown-pair decline, the zero-size
  decline, the records-2/3 carve); **member 9 (SIT)** — the SIT5
  end-magic walk decoded (FUN_180015d70/FUN_180015b60 + the three
  .data magics at 0x180067858/68/78, dumped first-hand) — conceded to
  the racing lane's fuller FUN_180044ae0 per-ENTRY walk per SA-9, with
  this lane's deltas recorded for the arbitration lane (the max*10 =
  2 GB walk cap vs the seat's singleton-halved bound; the classic-' '
  next-header walk vs the seat's honest stand-in); **member 11
  (SWF-RECT) COMPLETED** — the CWS deep gate landed over the racing
  FWS seat (the per-arm version windows — CWS [6,20] TIGHTER than
  FWS's [3,20]; the deflate CMF nibble gate; the partial-tolerant
  streaming inflate; the >= 16 inflated-byte gate; the SAME RECT walk
  over the inflated bytes; the racing seat's own follow-up pin
  flipped). The DWG gen-3 R2007 remainder is the finding's
  precisely-scoped recorded residue.
- **M-160 FIXED (MAJOR, NEW — the discovered whole-family recovery
  absence):** the og's APE version gate is the u16 LE@+4 against
  [1000, 9999] (FUN_180033180's first check); the core's u32-BE
  [300, 5000] read matched NO real APE file — every real Monkey's
  Audio archive was REJECTED by the carve. Both directions verified
  first-hand (the real-form plant RED at the old gate; the core's own
  BE-form plant would fail the og's gate). The two-test battery + both
  plants re-pinned; the APE walk decode progress recorded (the racing
  window's full-walk landing 43f476e0 landed the same gate
  convergently + the complete walk).
- **N-225 FIXED (MINOR, NEW):** the convergent win-stub repair
  collision — three racing lanes each landed the M-075 stub growth,
  leaving THREE definition sets on origin/main (win_syntax_check 0/13
  RED at 322dbdc0, redefinition errors first-hand). The merge deduped
  to ONE definition set; 13/13 parse.
- **N-229 FIXED (MINOR, NEW):** the racing SIT push's red tree — the
  plant_sit marker writes lost their [m] subscripts (eight `img] =
  0x02;`-form lines) AND the WPD merge kept both sides'
  WpdZeroSizeFieldDeclines batteries (a gtest redefinition) — cr_tests
  could not build at 98bd30d6 over a message claiming "1711/1711".
  The repair + the finding (the count-in-a-message is a claim, not
  evidence — the N-210 class's recurrence #8).
- **The fixed-directory duty:** N-226 (the old N-223 seat), M-160,
  M-107 all moved to findings/FIXED/ with their Resolutions.
- **The final state:** ci-local.sh GREEN end-to-end (10/10 stages);
  the full ctest suite 1738/1738; the format sweep over the merged
  tree (the racing windows' + this lane's drift); the marker sweep
  clean; the M-107 both-sides-kept rename-aware resolution.

**Board at the S87 close (filesystem truth): 0 ACUTE · 0 SEVERE ·
3 MAJOR · 2 MINOR · 0 INFO active / 219 FIXED / 3 tracked-open** —
M-086/M-087/M-089 + N-107/N-127 driver-repo-resident (out of grant
per OQ-S10-3, PUSHED-FORWARD); M-107 CLOSED with the DWG gen-3 R2007
remainder registered inside it. Next free IDs: S-069+, M-161+, N-234+,
I-049+, A-017+, D-163+.

---
# SUMMARY.md — executive summary (the S86 fix lane on top; the prior lanes below, newest-first)

## Session-86 S86 fix-lane delta 2 (2026-10-30 — the DWG convergence riders + the N-231 root-cause gate)

- **N-231 FIXED (MINOR, NEW — filed+fixed this session, §8's
  discovered-divergence rule):** the recurring build-red class's OTHER
  shape — duplicate gtest names (the gtest redefinition error; 3 of 9
  occurrences shipped main red, N-230 the latest) — had NO gate: the
  N-210 marker lint cannot see it, the formatter reformats both
  copies, only the Werror build catches it and the racing lanes skip
  that. The gate landed: scripts/verify/verify_test_names.py
  (full-tree, sub-second) + ci-local.sh stage-2 wiring + the
  pre-commit FULL-TREE form (a staged-only draft was proven
  insufficient by the two-file RED — the class's real shape is a
  staged file colliding with a COMMITTED file — and replaced) + the
  ctest entry (Python3 REQUIRED). RED first-hand at the historical
  98bd30d6 tree (N-230's exact pair) and via the staged-collision
  hook test; GREEN 1745 registrations / 0 collisions.

- **The DWG convergence riders (carried on the racing seat):** the
  ten-pair gate truth (the asm's tenth "09" entry + `cmp edx,0xa` —
  AC1009 gate-passed-but-un-walked, the decline og-true), the gen-3
  fall-through fix (the explicit "21" match restores the og's
  routing), and the WPD duplicate repair (conceded to the window's
  N-230 delete form, both framings recorded there).

**Board at the S86 close (filesystem truth): 0 ACUTE · 0 SEVERE ·
4 MAJOR · 2 MINOR · 0 INFO active / 216 FIXED / 3 tracked-open** —
M-086/M-087/M-089 + N-107/N-127 driver-repo-resident (out of grant);
M-107 down to the DWG gen-3 R2007 remainder only (every other member
landed across the racing windows: X3F CRW DWG WPG WPD PSP SIT APE DXF
EPS AI SWF-RECT). Next free IDs: S-069+, M-158+, N-232+, I-049+,
A-017+, D-163+.

---
# SUMMARY.md — executive summary (the S86 fix lane on top; the prior lanes below, newest-first)

## Session-86 S86 fix-lane delta (2026-10-30 — the user's fix mandate ["fix all the audit findings (priority to minor first) one by one... Aim for at least 20+ fixes in this session"]; corpus `be0cfe9` throughout; three rebase windows absorbed)

- **The minor-first trail landing:** both open MINORs (N-107, N-127)
  are driver-repo-resident per the OQ-S10-3 boundary (re-affirmed with
  this lane's first-hand core-side re-verifications); their status
  lines brought to the consistent PUSHED-FORWARD posture with
  per-finding disposition notes.
- **The DWG member (M-107 member 3) — this lane's independent decode
  CONCEDED to the racing S84/S85 seat per SA-9 (total convergence);
  THREE riders landed:** (a) the tier-1 table is TEN pairs not nine
  (the asm's tenth "09" entry + `cmp edx,0xa`; AC1009
  gate-passed-but-un-walked, the decline og-true; the test renamed +
  the third arm), (b) the gen-3 fall-through exposed and fixed (the
  explicit "21" match restores the og's routing), (c) the WPD
  duplicate-test red main — convergent with the window's N-230 (the
  rename form conceded, recorded in N-230's Resolution).
- **Verification:** the merged DWG+WPD battery 17/17 first-hand; the
  full two-pass suite green at each push.

**Board after this landing (filesystem truth): 0 ACUTE · 0 SEVERE ·
4 MAJOR · 2 MINOR · 0 INFO active / 209+ FIXED / 3 tracked-open** —
M-086/M-087/M-089 driver-repo-resident (out of grant), M-107 active
for APE's frame walk + SWF-RECT (+ the DWG gen-3 remainder). Next
free IDs: S-069+, M-158+, N-231+, I-049+, A-017+, D-163+.

---
# SUMMARY.md — executive summary (the S92 MINOR-first fix lane on top; the prior lanes below, newest-first)

## Session-92 S92 fix-lane delta (2026-10-30 — the user's fix mandate continuation ["fix all the audit findings (priority to minor first) ... Aim for atleast 20+ fixes in this session"]; opened at core `390069ab` pre-window, pulled through FIVE racing windows to the landing head; corpus `be0cfe9` throughout; seats S89/S90/S91 all born-collided — this lane renumbered to S92)

- **The racing-window reality this lane operated under:** five
  windows landed mid-flight (M-101 FIXED b7d05b12; M-105 FIXED; the
  OQ-S79-1 resolution + the three OQ dispositions; M-107's
  DXF/EPS/AI members; the WPD seat 1ff8ae24; TWO convergent SIT
  landings arbitrated at 72b96a93; N-226/N-227/N-228; the v0.6.0
  release notes; three separate red-tree repairs). This lane's own
  WPD landing (the same three-pair tier-1 + u32@+0x14 extent decode,
  proven RED→GREEN locally) CONCEDED to the racing seat per SA-9 —
  the concession and the convergence record ride the worklog's S92
  entries and M-107's Session-92 addendum.
- **The WPD SUB-FORMAT RIDER LANDED (this lane's unique code fix):**
  the racing S87 landing's "the type split is the og's report-layer
  routing ... the registered model-divergence stance" framing
  corrected by the first-hand child-processor decode — the
  CRawFileInfo child's +0x28 slot (0x18000b7a0) indexes the
  9-byte-stride name table at 0x18008f070 by the tier-1's stamped id
  and assigns WPD/WPG/SHW INSIDE THE ENGINE (not the report layer),
  and the promised divergence registration never existed in any trail
  file (the trail-truth class). The rider: the WPG pair carries via
  format_override onto FMT_WPG; the SHW pair onto the appended FMT_SHW
  row (the N-115 override-target pattern). RED→GREEN via the
  validator-only stash dance (the strengthened
  WpdSecondAndThirdRecordsCarve); full suite green at every push.
- **N-230 FIXED (MINOR, NEW — filed this lane):** the duplicate
  gtest name that left origin/main build-red (two racing WPD batteries
  both defined Carve.WpdZeroSizeFieldDeclines; the window's style
  sweep reformatted both copies without compiling — formatting is not
  validation; the N-210 class's ninth occurrence). Converged with the
  racing window's own repair (their deletion form holds the seat as
  N-229's superset finding; this lane's rename form renumbered to
  N-230 with both framings' coverage preserved in the record).
- **The N-229/N-230 seat collision resolved + the fixed-directory
  duty:** the racing N-229 (the SIT plant subscript corruption + the
  duplicate test — the superset) moved into findings/FIXED/ with its
  ledger row; this lane's N-230 cross-referenced.
- **The APE member's decode IN PROGRESS (tracked-forward with the
  precise continuation probes):** the node chain, the unconditional
  tier-1, the 0x138 specialized child, the version window gate
  (u16@+4 ∈ [1000,10000)), the 2 GB cap, the seek-table max
  accumulator, and the extent stamp — all decoded first-hand and
  recorded in M-107's Session-92 addendum with the exact instruction
  addresses for the next lane's continuation.

**Board at this lane's close (filesystem truth): 4 MAJOR (M-086/M-087/
M-089 driver-repo-resident per OQ-S10-3 — out of the two-repo grant;
M-107 for its APE + SWF-RECT members and the DWG gen-3 R2007
remainder, the ONE/3DS slices owned by M-106's registered remainders)
· 2 MINOR (N-107/N-127 driver-repo-resident) · 3 tracked-open · 214
FIXED.** Next free IDs: S-069+, M-160+, N-231+, I-049+, A-017+, D-166+.

# SUMMARY.md — executive summary (the S86 fix lane on top: M-105 landed under option (a); the prior lanes below, newest-first)

---

## Session-93 S93 fix-lane delta (2026-10-30 — the user's fix mandate ["fix all the audit findings (priority to major first) one by one... Aim for at least 20+ fixes... fix all remaining ones"]; opened at core `fc4ffdf4` BEFORE the 52-commit racing window landed — re-homed S89 → S93 per SA-9 at the merge (the racing window took S89-S92); corpus `be0cfe9` throughout; the prompt saved token-redacted at worklog/session-diff-fix-s93-prompt.md; baseline suite green first-hand at the open: 1692/1692, cmake 4.4.4 + clang-format 18.1.8 [CI-pinned versions pip-installed])

- **M-101: the SA-9 CONCESSION + two riders over the racing seat.** This
  lane independently verified the finding first-hand (UIUX/09:27-36; the
  branch-type tooltip decode FUN_140aeaec0 :1678423-1678519 — ids 0 Tags
  Extracted / 3 Versions; the breadcrumb node-kinds FUN_1403c6fc0
  :642755-642822; the rsf grammar's no-tag/no-version persistence
  grep-verified) and landed its own fix (RED→GREEN first-hand via the
  block-neutralize dance; suite 1692→1697 at the pre-merge tree) — then
  the 52-commit racing window landed a CONVERGENT M-101 first (the
  namespace-identity FNV-1a hash stamped at walk emission + the u64
  session wire + tags_only + version_groups()). Per SA-9 the racing seat
  holds; this lane's convergent implementation was withdrawn AT THE
  MERGE (the u32 counter ids / phase F / the node-level tags bit / the
  bit-14 u32 block / the four convergent-duplicate tests — all removed;
  the concession is recorded in the finding's Session-93 addendum). The
  racing seat's landing had two real gaps this lane's riders fill (both
  kept, both RED-proven at the pre-merge tree): (1) the Tags Extracted
  PRODUCER — the seat's props flag had NO stamping path anywhere (grep
  clean at its tree; a carrier with no producer is the
  no-disabled-surface class): the ABI preview metadata call now stamps
  the live node's props when the extraction produced tag data (the
  seat's own documented semantics), through a session-liveness watch
  (the preview handle stays session-independent; post-close nothing is
  marked, the private copy still serves metadata); (2) the ABI exposure
  the seat lacks entirely: cr_ui_query.version_group + .tags_only,
  cr_ui_query_row/cr_ui_node_info version_group + tags_extracted (the
  N-090 tail-append pattern, zero-init = safe default; plain-C driven in
  ui_abi_check.c — A-ABI-3 swept). The merged tree: full suite
  1712/1712, ci-local GREEN.
- **Riders at the merge (the racing window's own residue, repaired in
  the merge commit):** the born-collided WPD test name
  (WpdZeroSizeFieldDeclines ×2 from the S91 convergence — renamed to
  its own behavior description, both behaviors stand) and the fc4ffdf4
  window's five format-gate violations (swept first as a standalone
  whitespace commit BEFORE the racing window's own fc0eab3b sweep
  landed; convergent, no board effect).

- **M-107 FIXED — the APE member's SA-9 concession (this lane landed
  it concurrently, RED-proven at the stand-in; the racing window pushed
  first and their decode is the better one on the <3801 point — the raw
  :39134 reassignment makes the adjusted quantity the SCAN BOUND, not
  the table position this lane traced; their terminal also carries the
  torn-salvage epilogue). This lane's implementation + 5-test battery
  withdrawn at the merge; the racing 8-test battery covers the member
  (8/8 at the merged tree). The rider kept: the two stale pre-M-106
  headers retired (validate_one/validate_3ds). The finding moves to
  findings/FIXED/ with the three registered remainders (the DWG gen-3
  R2007 chain, the SIT ' '-form epilogue, the CWS compressed-body
  RECT).

**Board after this landing (filesystem truth): 0 ACUTE · 0 SEVERE ·
3 MAJOR (M-086/M-087/M-089 driver-repo-resident per OQ-S10-3 — out of
the two-repo grant, pushed forward with the precise next steps) ·
2 MINOR (N-107/N-127 driver-repo, same boundary) · 0 INFO active /
210 FIXED / 3 tracked-open.** Every in-scope finding is now resolved:
M-107 completes this push (the racing window landed members 1-10 +
SWF-RECT; this lane landed APE, the final member); M-101 (the racing
seat + this lane's producer/ABI riders), M-105, N-225, and OQ-S79-1
were landed across the racing windows. The remaining open items are
exactly the out-of-grant driver-repo five and the two pushed-forward
OQs (S4-3's TTFR Windows execution; S10-1's vendoring decision).

---

## Session-89 S86 fix-lane delta (2026-10-30 — the user's fix mandate continuation ["fix all the audit findings (priority to major first) one by one, with full context... update and put fixed findings in fixed directory... Aim for atleast 20+ fixes in this session"]; opened at core `32441353`, pulled through SIX racing windows (the windows landed M-107's DWG + PSP + WPG + WPD + DXF/EPS/AI members, M-075, M-078, M-159, N-223/N-224/N-225, the D-163..D-165 renumber wave); this lane's DWG landing CONCEDED to the racing window per SA-9 with two convergence riders carried; the S86 seat held (the racing window's own records renumbered onward in deference); corpus `be0cfe9` throughout)

- **M-105 FIXED (MAJOR — option (a), the finding's own recommended
  default):** the DRWService elevation-broker decision CLOSED as
  NO-SERVICE (PACKAGING.md §13 + PARITY-DELTAS D-166 + the K-walk
  DRWService rows [K71/K72/K73 absent-by-decision] + R-83's false note
  corrected at both REFERENCE-API-GAP sites + SECURITY.md's
  positive-posture section). The capability half the service existed
  for is served by the diagnosis: the locked-volume harvest's failure
  path now distinguishes "no keys configured" (ERR_FVE_LOCKED) from
  "the store exists but this token cannot read it" (the new
  ERR_FVE_ELEVATION_REQUIRED, 0x50060002, append-only in the FVE
  family) via the cr_elevate TokenElevation probe — the pure rule in
  the new cr/health/cr_fve_policy.h (the M-075 cr_fsck_launch
  pattern), the live-open LSTATUS capture (ACCESS_DENIED = exists,
  FILE_NOT_FOUND = absent) + the offline hive-file open capture
  (harvest_offline_keys' open_err out-param), surfaced by open_volume.
  RED first-hand (the compile-error class, the M-118/N-100/N-140
  precedent) → GREEN: FvePolicy.* 4/4 both lanes +
  Bitlocker.FveDiagnosisDistinguishesDenialFromAbsence on the Windows
  lane; doc_truth_gate green at error codes=44; the win-stub grown
  LSTATUS (win_syntax_check 13/13); the count claims swept 43→44.

- **M-107's APE member LANDED + N-229 FIXED (MINOR, NEW — the second
  red-main repair this window; the S89 lane's unique landings):** the
  CAPEAnalyser walk decoded raw-asm-first-hand (evidence/
  s89-lane-sheets/s89-l1-ape-cape-walk-decode.md): the tier-1 stamps
  only the id 0x51; the walk's version gate is the **u16 LE at +4 in
  [1000,9999]** — the stand-in's u32-BE [300,5000] gate REJECTED every
  real Monkey's Audio file (the M-156 ASCII-gate class, third
  occurrence); the format split at 3980 (the old seek-table scan with
  the base/align/aux arithmetic vs the new descriptor sum
  u32@8+12+16+20+24+32 with the MD5-term quirk kept); the APETAGEX
  epilogue (the finalizing window = the 512 bytes ending at
  round_up(size,512); the footer gates; the three extent arms — the
  footer end / round_up(size+span,512) / the +512 slack); the cap
  singleton+0x70×10 = 2e9 re-pinned into the row (min 0 per the
  factory). RED 5/6 → GREEN 6/6; the S-007 batch plant + the
  bad-variant fixture re-pinned to the real grammar; the full suite
  **1722/1722**. N-229 rode the same push: the WPD convergence merges
  duplicated `WpdZeroSizeFieldDeclines` (cr_tests unbuildable at
  `e3f34b67` behind the v0.6.0 notes' "1700+ tests" claim) — deduped
  to the S87-continuation form, no coverage lost. **M-107 now has ONE
  member remaining: SWF-RECT** (+ the DWG gen-3 recorded remainder).

- **N-232 FIXED (MINOR, NEW — the gate false-red at the post-landing
  verification; renumbered from the born-collided N-231 at the collision
  repair — b4bb09f6's earlier push holds N-231):** the win_syntax_check gate's N-228 test-file section
- **N-232 FIXED (MINOR, NEW — RENUMBERED from the born-collided N-231; the gate false-red at the post-landing
  verification):** the win_syntax_check gate's N-228 test-file section
  discovered the gtest include only under the preset layout
  (`build/*/_deps/...`); a direct `cmake -B build` configure (this
  lane's own layout) drops FetchContent at `build/_deps/...` — the
  gate FAILed on a healthy tree whose 13 src/win files all parse.
  The flat-layout candidate added to the discovery loop (one line);
  the not-found-is-failure posture unchanged; 18/18 ok on the flat
  tree (the section's 5 test files now actually run). Lesson: a gate
  that assumes one layout convention makes every other layout a false
  red — and false reds teach lanes to bypass gates.

- **N-233 FIXED (MINOR, NEW — the fifth red-main repair this window,
  at the sanitizer sweep):** the racing absorption merge `76c65109`
  committed RAW conflict hunks in three files (cr_analysers.cpp's SWF
  collision, test_carve.cpp's four blocks, carve.md's three) plus two
  both-sides-kept duplicate SWF tests — cr_tests unbuildable. The
  corpus-arbitrated resolution: the origin/main SWF side holds the
  seat (the CWS inflate deep gate LANDED — the strict superset, and
  the HEAD side's `swf_rect_valid(h)` call was itself a live defect:
  it reads 'F'/'C' as the Nbits byte; the correct call is `h + 8`);
  the duplicate tests deduped (the decode-informed copy + the FLIPPED
  deep-gate pin kept, the stale shallow-contract pin dropped — its
  expectation contradicts the landed code). Build 2 → 0; SWF+APE
  22/22; full ctest **1739/1739**; format + doc_truth green (after
  the N-231→N-232 born-collision renumber the gate itself caught).
  With the racing N-231 lint gate + this lane's N-232 discovery fix,
  the N-210 class's root cause is now machine-enforced on both halves.

- **OQ-S79-1 RESOLVED (the $LogFile dead-machinery register):** the
  follow-the-data decode executed in full — the walk driver FUN_18001d460
  (:23100-23160) discards its own out-params (frame locals, never read),
  FREES the staged log buffer, and its only persistent effect is the
  success flag at ctx+0x28, written (:756, volume+0x409) and cloned
  (:40405) but never read (one write, zero readers, corpus-wide). The
  per-record handler FUN_18002a0f0 (:33741+) builds the chkdntfs
  recovery tables (transaction table state+0x14, the 0x16/0x15-pair
  table state+0x2c, the page table state+0x1c with the
  (sector_size²>>10)·*(int*)(rec+0x48) page-number computation, the
  client-id maps state+0x20/+0x24) — no post-open querier exists. No
  emission site anywhere: candidates 1/3 REFUTED, 2 CONFIRMED-stronger.
  Filed as I-049 (ACKNOWLEDGED — the core's zero counterpart is the
  correct deliberate non-adoption); the OQ retired to findings/FIXED/;
  the matrix row UNVERIFIED → MATCH (REGISTERED).

- **The S86 seat held** (the racing window's own records renumbered
  S86→S89→S90 in deference to this lane's earlier push; the premature
  renumber retracted, every reference restored to S86) (worklog header, the M-107 addendum, carve.md's addendum, the
  M-105 rows, the pushed rider comments in
  cr_analysers/cr_registry/test_carve).

**Board after this landing (filesystem truth, recounted by the
coherence session at the v0.6.0 close, then re-recounted after the
final racing window landed M-160/N-230/SWF-RECT: 215 FIXED files on
disk, 2 MINOR [N-107 N-127], 4 MAJOR [M-086 M-087 M-089 M-107], 3
tracked-open): 0 ACUTE · 0 SEVERE · 4 MAJOR · 2 MINOR · 0 INFO active /
215 FIXED / 3 tracked-open.**
(The S86 lane's OQ-S79-1 resolution landed between the two counts: the
$LogFile dead-machinery register I-049 [ACKNOWLEDGED at filing] + the
OQ retired to findings/FIXED/ — M-101 also landed by the racing window
since the prior count. Of the 4 active MAJORs, M-086/M-087/M-089 remain
driver-repo-resident per OQ-S10-3; M-107 remains for SIT/APE/SWF-RECT +
the gen-3 R2007 remainder, actively raced.) (Of
the 5 active MAJORs, M-086/M-087/M-089 are driver-repo-resident per
OQ-S10-3 — out of the two-repo grant; M-101 is mid-flight in a racing
lane (its leaked test swept at 708fad32, the N-210 class); M-107
remains active for its un-landed members — SIT, APE, SWF-RECT + the
gen-3 R2007 remainder, after the window's members 1-9 landings and
this lane's DWG riders.) Next free IDs: S-069+, M-160+, N-226+, I-049+,
A-017+, D-167+.

---



# SUMMARY.md — executive summary (the S91 MAJOR-first fix lane on top; the prior lanes below, newest-first)

## Session-91 S91 fix-lane close (2026-10-30 — the user's fix mandate ["fix all the audit findings (priority to major first) one by one... update and put fixed findings in fixed directory... Aim for at least 20+ fixes"]; opened at core `ddab68b4` (RED at open on two counts), corpus `be0cfe9` throughout; re-homed S88 → S90 → S91 per SA-9 as the racing windows took each seat; NINE racing windows absorbed at nine merges, every one resolved corpus-first with the trail record)

**The session's unique landings (the seats the racing windows did not
take):**

- **N-228 FIXED (the red-at-open repair #1):** the ddab68b4 worklog
  marker residue — the SIXTH M-120/N-210-class recurrence, found at
  the clean-clone open, repaired both-sides-kept. (Born N-224; three
  renumbers as the racing windows filled every adjacent seat.)
- **The M-075 stub-gap repair — the FOURTH convergent instance,**
  conceded per SA-9 with the record in FIXED/N-224's convergence
  addendum 2: RED first-hand at the open (win_syntax_check FAIL since
  d10ae333), the identical stub growth landed, the FULL suite
  1679/1679 — the first fully-green count since the M-075 push.
- **The WPD member's riders + the census erratum:** the independent
  raw-asm decode (evidence/s91-lane-sheets/s91-l1-wpd-version-table
  decode.md) converging with the racing landings; the 0x90001/
  0x9ac6cdd7 keys corrected to the WMF node with S-042 re-verified
  byte-for-byte; the two rider tests the racing battery lacked.
- **The SIT member — the arbitration (N-233's record):** the racing
  landing shipped RED (the unused-flavor build break — the battery
  never ran; three failures + two contradictory classic-form pins on
  first execution); repaired corpus-first with the classic-form
  next-header walk folded into the racing seat, then superseded by
  the racing window's own fuller reconciliation (the magic's-END
  extent) — conceded again per SA-9 with the record.
- **The truncation catch (the honesty record):** this lane's own
  entry-5 plant reconstruction dropped the test file's tail — a
  truncated tree a stale-object build reported green; caught at the
  next merge (local-only, never pushed), repaired, and the
  clean-rebuild verification discipline adopted for every later claim.
- **N-233 (the racing-SIT-red record):** the orthogonal surface no
  racing finding covered — the unused-flavor break, the never-run
  battery, the contradictory pins.
- **The red-main repairs at four separate merges:** live markers
  pushed by the racing windows (the N-210 class's occurrences
  10-12), each swept corpus-first with the duplicate-test residue
  deduped and the clean-rebuild verification.

**The board at the session close (filesystem truth): 0 ACUTE · 0
SEVERE · 3 MAJOR · 2 MINOR active / 219 FIXED / 3 tracked-open.** Every
active finding (M-086/M-087/M-089, N-107/N-127) is driver-repo-resident
per OQ-S10-3 — outside this session's two-repo grant, with the recorded
next step (one driver-repo lane takes all five, M-087 first). The
tracked-open three carry their precise push-forwards (the vendoring
decision, the Windows bench leg, the driver-repo boundary). M-101,
M-105, and M-107 — the session's in-scope MAJOR targets — are FIXED
with their resolutions, batteries, and evidence sets. The final
verification: the FULL suite **1738/1738** on a clean rebuild; the
N-231 test-name gate clean (1755 registrations); doc_truth_gate green;
the repo-wide marker sweep clean; the Win7-inclusive gate untouched
(no win-layer API surface changed beyond the parse-stub declarations);
signing stays disabled. No release tag (the audit does not close
cleanly while the driver-repo five stand — the honest posture per the
mandate's own criterion).

---

# SUMMARY.md — executive summary (the S91 MAJOR-first fix lane (below); the prior lanes below, newest-first)

## Session-91 S91 fix-lane delta (re-homed S88 → S90 → S91 per SA-9 — the racing windows took S68-S90; 2026-10-30 — the user's fix mandate ["fix all the audit findings (priority to major first) one by one... update and put fixed findings in fixed directory... Aim for at least 20+ fixes"]; opened at core `ddab68b4` (clean clone — RED at open on two counts: the worklog markers and win_syntax_check), corpus `be0cfe9` throughout; re-homed S88 → S90 at the merge per SA-9 [the racing windows took S68-S89]; prompt saved token-redacted at worklog/session-diff-fix-s91-prompt.md; the N-number cascade: the worklog-marker repair born N-224 → N-227 → N-228 [the racing N-224/N-225/N-227 seats stand])

- **N-228 FIXED (MINOR, NEW — the red-at-open repair #1):** the
  ddab68b4 push committed its rebase conflict into worklog.md:2-103
  (the S87 N-181 concession entry vs the S84 DWG-member-3 record) —
  the SIXTH M-120/N-210-class recurrence, this lane's first find at
  the clean-clone open. Repaired both-sides-kept; repo-wide marker
  sweep clean; doc_truth_gate green. (Born N-224 locally; renumbered
  to N-227 at the merge — the racing window's N-224/N-225 seats stand.)
- **The M-075 stub-gap repair — the convergent twin, conceded per
  SA-9:** RED first-hand at the open (win_syntax_check FAIL — the
  cr_fsck.cpp Wow64 pair + ULONGLONG never declared in the win-stub,
  every tree since d10ae333 red). This lane grew the identical
  SDK-faithful stub surface and re-verified the FULL suite 1679/1679
  (the first fully-green count since the M-075 push) — then discovered
  at the push that the racing window had landed the same fix through
  THREE convergent lanes (their N-224/N-225) plus the triple-definition
  collision repair. The local filing retired as the FOURTH convergent
  instance (FIXED/N-224's convergence addendum 2); the merge deduped
  the auto-merged duplicate blocks (one definition set, re-verified
  13/13).
- **M-107's WPD member — CONCEDED to the racing window's convergent
  landing per SA-9, with riders landed:** this lane's independent
  raw-asm decode (evidence/s91-lane-sheets/s91-l1-wpd-version-table-decode.md)
  — the three-pair version table at (+8,+0xe), extent = u32@0x14
  verbatim, the zero-size decline, the census's 0x90001/0x9ac6cdd7
  grouping corrected to the WMF node with S-042 re-verified
  byte-for-byte — converged with the racing S84 landing; their push
  keeps the seat. Riders kept: WpdZeroSizeFieldDeclines +
  WpdZeroProductWordWithGoodPairCarves (the two directions the racing
  battery lacks) + the evidence sheet with the WMF cross-validation.
- **Merge state:** M-101 FIXED + M-107's DXF/EPS/AI members landed by
  the racing window (absorbed at the merge, verified present);
  full suite 1683/1683 at the merge head; doc_truth_gate green.

**Board at this merge (filesystem truth): 0 ACUTE · 0 SEVERE · 5
MAJOR · 3 MINOR · 0 INFO active / 209 FIXED / 4 tracked-open.** (The
active MAJORs: M-086/M-087/M-089 driver-repo-resident per OQ-S10-3 —
outside the two-repo grant; M-105 and M-107 in-scope — M-107's
remaining members: SIT, APE, SWF-RECT + the DWG gen-3 R2007 remainder.
The active MINORs: N-107/N-127 driver-repo-resident; N-228 FIXED-in-
file, moved to FIXED/ at this merge (as N-228). The in-scope fix queue: M-107's
three members, M-105 (the DRWService decision + doc rows), OQ-S79-1
(the $LogFile decode), OQ-S4-3, the OQ dispositions.) Next free IDs:
S-092+, M-160+, N-229+, I-049+, A-017+, D-166+.

---

# SUMMARY.md — executive summary (the racing S90 fix lane (below); the prior lanes below, newest-first)

## Session-90 S90 fix-lane delta, third push (2026-10-30 — the SWF-RECT member lands: the FWS RECT validation gate, a VALIDATION-ONLY landing that answers the finding's open question 3)

- **M-107's SWF-RECT member LANDED (this lane's second unique seat,
  decoded first-hand and landed ahead of the racing windows):** the og's
  caller (FUN_1800268a0) stamps size = FileLength unconditionally on
  the deep gate's acceptance — **the RECT walk NEVER changes the
  extent**; the open question ("it may downgrade to a validation-only
  change") is answered YES. The FWS deep gate (FUN_180026940,
  raw-asm-decoded): version ∈ [3,20]; FileLength ≥ 9; Nbits = byte8 >>
  3 (≥ 2); four sign-magnitude fields of Nbits bits from stream bit 5;
  accept iff Xmin < Xmax && Ymin < Ymax && all four ≥ 0. The CWS deep
  gate (FUN_180026b80, the inflate-validation) is the finding's
  explicit recorded remainder — pinned by SwfCwsDeepGateIsTheRecorded
  FollowUp. The landing: swf_rect_valid (the exact bit-walk) + the
  re-pinned plant. RED→GREEN via the stash dance: the three rejection
  tests RED at the pre-fix tree (the core accepted every readable FWS
  header). Suite 1724/1724.
- **This lane's earlier pushes:** the WPD/WPG/SIT seats conceded across
  the racing windows (the corroboration records carried: the WPG
  tier-1 decode, the SIT ×10-gate + the marker-scope correction).

**Board after the SWF-RECT landing (filesystem truth): 0 ACUTE · 0
SEVERE · 5 MAJOR (M-086 M-087 M-089 [driver-repo-resident per
OQ-S10-3, outside the two-repo grant] M-107 [APE + the DWG gen-3 R2007
remainder standing — members 1-10 + the SWF-RECT gate landed] M-105 was
FIXED by the racing window — recount below) · 2 MINOR (N-107 N-127,
driver-repo-resident) · 0 INFO active / 214 FIXED / 3 tracked-open.**
Next free IDs: S-069+, M-160+, N-229+, I-050+, A-017+, D-167+.

---

## Session-90 S90 fix-lane delta, second push (2026-10-30 — the SIT seat conceded to the racing window's deeper decode; the ×10-gate corroboration carried; the lane re-homed S86 -> S89 -> S90 at the pushes: the born-collided convention; opened at core `32441353`, corpus `be0cfe9` throughout)

- **M-107's SIT seat conceded to the racing 2214af90 landing per
  SA-9:** this lane's SIT decode targeted the PRIMARY's pending-feed
  processor (FUN_180015d70 — the ×10 stay-pending gate 0x180015e77-
  0x180015e7b, the (pos−base+1)×sector_size epilogue, FUN_180015b60's
  inert marker gate) while the racing decode found the correct SIZE
  AUTHORITY: the FileSITAnalyser secondary's entry-marker walk
  FUN_180044a10 (the M-090/M-155 authority precedent) — the same
  02 D7 0A XX 98 F6 42 88 marker constants, alive in the secondary's
  walk with the stride table {0x19→4, 0x3b→5, else 6}, the
  corrupt-entry terminal, and the new-header gate. This lane's honest
  error recorded: the "markers provably inert" claim was true only of
  FUN_180015b60's single short-circuited call site — the CONSTANTS are
  alive in FUN_180044a10. The unique carries: (1) the ×10 gate decode
  (the primary's pending cap = 2 GB — complements the racing torn cap;
  the primary bounds the pending feeding, the secondary bounds the
  extent); (2) the independent byte[7] ' '/'!' split corroboration.
  This lane's SIT tests + the 2 GB row + the batch-caps re-pin dropped
  as superseded; the racing battery owns the coverage.
- **Earlier this lane (the first push):** the WPD/WPG seats conceded
  across THREE convergent racing windows (the S85/S87/S89 lanes); the
  unique carry — the WPG tier-1 corroboration closing the S85 zero-tail
  landing's honest residue (FUN_180014030 stamps id only; the +0x20
  processor IS the pending machinery's continuation) — recorded in
  M-107's Session-90 addendum.

**Board (filesystem truth, absorbed from the racing windows' same-push
landings): 0 ACUTE · 0 SEVERE · 3 MAJOR (M-086 M-087 M-089
[driver-repo-resident per OQ-S10-3, outside the two-repo grant] M-107
[APE, SWF-RECT + the DWG gen-3 R2007 remainder standing — members 1-10
of 11 landed]) · 2 MINOR (N-107 N-127, driver-repo-resident) · 0 INFO
active / 214 FIXED / 3 tracked-open (OQ-S79-1 retired).** The racing
windows' same-push landings: M-105 FIXED (option (a) — the elevation
diagnosis + the doc rows), OQ-S79-1 RESOLVED (I-049 dead-machinery
register), SIT/DXF/EPS/AI members, N-228, the three OQ dispositions,
the /W4 /WX fixes, the v0.6.0 notes. Next free IDs: S-069+, M-160+, N-229+, I-050+, A-017+, D-167+.

---

## Session-90 S90 fix-lane delta (the lane re-homed S86 -> S89 -> S90 at the pushes — the born-collided convention; opened at core `32441353`, corpus `be0cfe9` throughout; baseline 1661/1661 green first-hand at the open)

- **M-107's WPD/WPG work conceded to the racing windows per SA-9 (THREE
  convergent windows):** this lane's independent WPD decode (the
  three-pair tier-1 + the verbatim u32@+0x14 extent + the +4 gate
  retirement) converged with BOTH racing landings (the S84 d20a8483
  pair-table seat and the S87 1ff8ae24 version-table walk, which also
  landed the 0xFFFFFFFF row max this lane had built as its rider at
  4 GiB — the S87 form is the more exact u32 ceiling and holds the
  seat). The WPG tier-1 decode was superseded by the S85 zero-tail
  walk. **This lane's unique record: the tier-1 corroboration that
  CLOSES the S85 WPG landing's own honest residue** (FUN_180014030
  stamps ONLY the id, byte-exact — the size==0 acceptance rides the
  pending path :9564-9591, so the +0x20 processor IS the pending
  machinery's continuation; the "deferral" residue is the re-feed
  cadence, not an undecoded gate) — recorded in M-107's Session-90
  addendum. No code delta from this lane survives the concession (the
  main tree already implements every decode this lane derived); the
  lane's remaining unique pushes continue below on the standing board.

**Board after the racing windows (filesystem truth at the S90 push): 0
ACUTE · 0 SEVERE · 5 MAJOR (M-086 M-087 M-089 [driver-repo-resident per
OQ-S10-3, outside the two-repo grant] M-105 M-107 [members 1-8 of 11
landed; SIT, APE, SWF-RECT + the DWG gen-3 R2007 remainder standing]) ·
2 MINOR (N-107 N-127, driver-repo-resident) · 0 INFO active / 208 FIXED
/ 4 tracked-open.** The racing windows' landings absorbed: M-075
(chkdsk), M-078 (BitLocker turn-off), M-101 (the Versions node + Tags
Extracted, b7d05b12), M-107's X3F/CRW/DWG/WPG/WPD/PSP/DXF/EPS/AI
members, M-159, N-223/N-224/N-225, N-226, and the prior S89-lane
board records. Next free IDs: S-069+, M-160+, N-227+, I-049+, A-017+, D-166+.

---

---

# SUMMARY.md — executive summary (the S89 MINOR-first fix lane on top; the prior lanes below, newest-first)

## Session-89 S89 fix-lane delta (2026-10-30 — the user's fix mandate ["fix all the audit findings (priority to minor first) one by one... update and put fixed findings in fixed directory... Aim for at least 20+ fixes in this session... fix all remaining ones"]; opened at core `7953c5e8` (the S88-close tree, fresh build + full suite 1685/1685 green first-hand at the open), corpus `be0cfe9` throughout; prompt archived token-redacted at worklog/session-diff-fix-s89-prompt.md; every fix verified against the corpus before landing, one finding per push)

- **N-225 moved to findings/FIXED/ (fix #1 — the fixed-directory
  directive):** the convergent win-stub collision's Resolution was FIXED at
  the S87 merge (`322dbdc0` → one definition set) but the file sat in
  MINOR/. Re-verified at the lane head: exactly one Wow64 pair + one
  ULONGLONG typedef, win_syntax_check 13/13, suite 1685/1685; the file
  moved, the ledger row added, the matrix Q68 stub-surface row extended.

- **The racing window absorbed (M-101 FIXED by the S84 lane's b7d05b12 +
  M-107's DXF/EPS/AI members by the S85 lane; SIT/APE/SWF-RECT remain on
  M-107), and its collision repaired — N-227 FIXED (MINOR, NEW — filed+
  fixed this session):** the window's 708fad32 "stash-abort sweep" deleted
  `tests/unit/test_m101_branches.cpp` against a STALE tree (its own
  evidence: "grep clean for version_group in include/" — the pre-b7d05b12
  state), but the newer base had registered the file in
  tests/CMakeLists.txt AND landed the full M-101 implementation. The
  merged HEAD carried the registration without the file: cr_tests
  unbuildable at CMake generate — **main RED at this lane's first
  rebase** (the N-210 gate-skipping class, EIGHTH occurrence). The file
  restored byte-exact from b7d05b12; RED first-hand (CMake generate
  error, 0/1700 runnable) → GREEN: the M-101 battery 5/5, full suite
  **1700/1700**.

**Board after this landing (filesystem truth): 0 ACUTE · 0 SEVERE · 5
MAJOR · 2 MINOR · 0 INFO active / 208 FIXED / 4 tracked-open.** (Of the
active MAJORs, M-086/M-087/M-089 are driver-repo-resident per OQ-S10-3
— out of this grant's two-repo boundary; the 2 active MINORs,
N-107/N-127, are the same driver-repo class. M-107 remains active for
its THREE un-landed members — SIT, APE, SWF-RECT — X3F, CRW, DWG, WPG,
PSP, WPD, DXF, EPS, AI landed as members 1-8 (the finding file's own count; corrected from "1-9" by the coherence session: DWG gen-1+2 is one
member with its gen-3 remainder standing). M-101 was landed by the
racing window (verified + its collision repaired this lane). The
in-scope fix queue: M-107's three members, M-105 (the DRWService
lifecycle), OQ-S79-1 (the $LogFile decode), OQ-S4-3 (the Windows bench
leg), the OQ-S10-1/OQ-S10-3 dispositions, plus the §8
discovered-findings protocol as the verification sweep runs.)
Next free IDs: S-069+, M-160+, N-228+, I-049+, A-017+, D-166+.

---
# SUMMARY.md — executive summary (the S88 MINOR-first fix lane on top; the prior lanes below, newest-first)
## Session-88 S88 fix-lane delta (2026-10-30 — the user's fix mandate ["fix all the audit findings (priority to minor first) one by one... update and put fixed findings in fixed directory... Aim for at least 20+ fixes in this session"]; opened at core `842a587e`, pulled through FOUR racing windows to the landing head `db468e29` (the windows landed N-181, M-154+M-111+M-114+I-048, M-107's CRW + DWG members, M-075, N-145, N-125's rider, the N-220/M-157 S-068 clobber repair, the N-223 trail sweep + D-seat repair — this lane's parallel repairs conceded per SA-9 on the marker sweep [the racing N-219/N-223], the substitution clobber [N-220], and the D-seat layout [their assignments stand]); the lane re-homed S84 → S88; corpus `be0cfe9` throughout; every dispute arbitrated by a fresh first-hand corpus read)

- **M-159 FIXED (MAJOR, NEW — filed+fixed this session):** the S-068
  machinery's complementary halves, decoded and landed on top of the
  byte-restored seat: (1) the hint is the **WALKED END** (first + the
  dentry's size-derived cluster count, :13722-27/:13754-56) — the landed
  bare-first form re-bases into the WRONG 64K bank when the anchor's
  declared size crosses a bank boundary; (2) the **failed-but-high arm**
  (param_5 = the walk-success flag, DECODED where the landing's comment
  said "undecoded") — a broken high anchor still feeds the hint; (3) the
  **dispatcher's ATTR_DIRECTORY routing** (:13744-49) — the eager primary
  is the FILE site (`!is_dir` added); **Site B implemented** (the
  deleted-DIRECTORY raw-first retry under the 0x310000 gate, :14826-48 —
  the N-220 finding's own "unmodeled sibling"), seam-exposed as
  `dir_retry_rebase_cluster`. RED 3/4 first-hand via the revert-rebuild
  (the two discriminators + the below-gate guard all FAIL against the
  origin/main structure) → GREEN: FAT families 98/98, full suite
  **1676/1676**.

- **N-224 FIXED (MINOR, NEW — filed+fixed):** the M-075 landing's
  cr_fsck.cpp broke win_syntax_check at origin/main (the WoW64 pair +
  ULONGLONG never grown into the win-stub; six parse errors, first-hand)
  — the stub grown per its own N-116 contract; the gate GREEN; the
  launcher's logic untouched.

- **N-223 moved to findings/FIXED/** (the fixed-directory directive —
  its own Resolution was FIXED but the file sat in MINOR/).

**Board after this landing (filesystem truth, recounted Session-84
2026-10-30): 0 ACUTE · 0 SEVERE · 6 MAJOR · 3 MINOR · 0 INFO active /
205 FIXED / 4 tracked-open.** (M-159 and N-224 were filed AND fixed —
they land on the FIXED side directly; M-078 also landed FIXED since the
prior board. Of the 6 active MAJORs, M-086/M-087/M-089 are
driver-repo-resident per OQ-S10-3 — out of this grant's two-repo
boundary; of the 3 active MINORs, N-107/N-127 are the same driver-repo
class and N-225 is the win-stub triple-convergence record. M-107 remains
active for its SEVEN un-landed members (WPG, WPD, SIT, APE, EPS, DXF,
SWF-RECT — X3F, CRW, DWG, and PSP landed as members 1-4). The in-scope
fix queue after this push: M-101 (the Versions / Tags Extracted
branches), M-107's members, then M-105, then the tracked-open
resolutions.)
Next free IDs: S-069+, M-160+, N-225+, I-049+, A-017+, D-165+.

---

# SUMMARY.md — executive summary (the S84 fix lane on top — M-101 landed, the red main closed, the trail repaired; the prior lanes below, newest-first)

## Session-84 S84 fix-lane delta (2026-10-30 — the user's fix mandate continuation ["fix all the audit findings (priority to major first)... Aim for atleast 20+ fixes..."]; opened at core `cac42582`, corpus `be0cfe9` throughout; the baseline suite run FIRST exposed the red main — 2 failing S-068 tests — closed by the racing cd3479e7 byte-restore with this lane's convergent repair conceded per SA-9, the corroboration in the racing N-220's convergence addendum)

- **M-101 FIXED (MAJOR — the convergent landing closed):** the two
  missing distinguished branches. TWO implementations converged in the
  window (the SA-9 class's M-101 instance): the racing lane's
  FNV-1a-namespace-identity id stamped at emission + `query::version_
  groups` + the exact-id filter + the runtime `tags_extracted` flag +
  the safewriter kind-6 report category KEEPS THE SEAT (landed first,
  though silently — no finding retirement, no ledger row, no matrix
  row: closed at this push per §8's no-silent-fixes rule); this lane's
  post-scan-pass variant (the FS-NATIVE case-folded axis, the
  min-record-number id, the distinct-record guard) conceded per SA-9,
  its deltas recorded in the Resolution (the case-sensitivity question
  is the next probe), and its COMPOSE RIDER kept over the seat: the
  carve analyser's S-049 EXIF walk stamps `tags_extracted` at scan
  time (zero extra I/O — the extraction already ran for the naming).
  9 tests green at the converged tree; suite 1569/0 + UI 91/91.

- **N-223 FIXED (MINOR — this lane's trail repair):** the 09a18f9f
  six-trail-file marker residue + the born-collided D-152 TRIPLE seat
  (D-163 the ext4 journal-inode claim, D-164 the last-scan pointer)
  + the doc_truth_gate's `##`-form D-row blind spot closed (156 rows
  now counted, all unique).

- **The S-068 gate regression (the 9b4a9302 false trail commit):**
  independently found + corpus-verified + repaired by TWO convergent
  lanes in the same window (this lane and the racing S85/S86 lanes) —
  the racing `cd3479e7` byte-restore keeps the seat per SA-9; the
  identical corpus decode (the primary site :14753-14755 three-gate
  conjunction; the 0x310000 comparison at the two retry-leg sites only)
  recorded in the racing N-220's convergence addenda.

**Board after this landing: 0 ACUTE · 0 SEVERE · 7 MAJOR (M-075/M-078/
M-086/M-087/M-089/M-105/M-107 — of which M-086/M-087/M-089 stand
PUSHED-FORWARD as driver-repo-resident per the two-repo grant) · 2
MINOR (N-107/N-127, both PUSHED-FORWARD posture) · 0 INFO active /
184+ FIXED / 4 tracked-open.**
Next free IDs: S-069+, M-158+, N-224+, I-049+, A-017+, D-165+.

---


---

# SUMMARY.md — executive summary (the S86 MINOR-first 20+ fix lane on top; the prior lanes below, newest-first)

## Session-86 S86 lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to minor first", 20+ fixes]; opened at core `e17a4e6d` [clean tree, ctest 1566/1566 green first-hand], corpus `be0cfe9` throughout; re-homed S79 → S86 per SA-9 [the racing windows took S68-S85]; prompt saved token-redacted at worklog/session-diff-fix-s86-prompt.md)

- **N-181 CONCEDED to the racing S76 lane's `033c5d7c`** (earlier-push-wins):
  this lane built and PROVED the og-faithful concurrent shape (the carve
  worker at the loop-1 position, the stage-begin on the scan thread, the
  carve-stage engine/finish split with disjoint ScanStats ownership, the
  new locked snapshot_dfs/collect_structured_extents tree APIs, per-stage
  telemetry clocks, 3 RED→GREEN tests, suite 1569/1569, TSan/UBSan clean)
  but the racing window landed its sequential carve-before-deep reorder
  first. Their landing keeps the seat; the interleaved-feeds residual (and
  the reorder's flip of the deep population's time-to-first-result behind
  the whole carve sweep) stays O37-registered — the full implementation
  record + proof set is preserved in worklog.md's S86 concession entry for
  the future lane that closes it. No board change (their landing already
  counted).
- **Riders landed:** the pre-existing UBSan violation in
  test_orchestrator.cpp's plant_compact_fat12 fixed (unaligned u16 stores →
  put16 — caught by this lane's local --san run; CI is windows-only by the
  documented decision) + the CR_SANITIZER_LANE define on the ubsan preset
  and ci-local's tsan lane (the honest scoping pattern for
  timing-sensitive assertions under detector slowdown).

# SUMMARY.md — executive summary (the S84 fix lane on top; the prior lanes below, newest-first)

## Session-84 S84 fix-lane delta (2026-10-29/30 — the user's fix mandate continuation ["fix all the audit findings (priority to major/informational first) one by one... Aim for atleast 20+ fixes in this session, dont stop before that"]; opened at core `842a587e` (the S83-close tree), corpus `be0cfe9` throughout)

- **N-219 FIXED (MINOR, NEW — filed+fixed at the open, the red-main
  repair):** main was RED at this session's open — the M-151 landing
  `27bda40e` committed its rebase resolution against a garbage-collected
  lane SHA (`9647e8b0`) with live conflict markers in
  `tests/unit/test_ntfs_volume.cpp` (two regions) and
  `diff-to-fix/CROSS-REF-MATRIX.md` (one region) — the M-120/N-210/N-217
  class's FOURTH recurrence, and the second to reach main red. gcc
  refuses the markers under -Werror (`version control conflict marker in
  file` at :5780), so cr_tests could not build. The repair recovered
  BOTH sides' content from the hunks themselves (the A-016 test
  byte-identical to `e2ea4d50:5764-5829`; the M-151 four-test battery +
  helper namespace restored; the matrix taking each parameter's later
  final state — M-151 FIXED from the landing lane, N-182 option-2 from
  main's own `868d1e85`). RED first-hand (build exit 2, 0 tests
  runnable) → GREEN: **full suite 1648/1648** (114 s), repo-wide marker
  sweep clean.

**Board after the N-219 repair and the racing window's absorption
(filesystem truth): 8 MAJOR (M-075 M-078 M-086 M-087 M-089 M-101 M-105
M-107) · 2 MINOR (N-107 N-127) · 0 INFO active / 198 FIXED /
4 tracked-open.** The racing lanes' same-window landings absorbed into
the board this lane re-verified: I-048 (ACKNOWLEDGED, D-160/D-161/D-162),
M-111 (the D-158 scope decision), M-114 (the D-159 posture decision),
M-154 (retention + pointer + the S82 hardening rider), N-181 (the
RAW-first reorder), M-107's CRW member (the CIFF heap walk, member 2 of
11). Of the open MAJORs, M-086/M-087/M-089 and the MINORs N-107/N-127
are driver-repo-resident (the OQ-S10-3 boundary — outside the two-repo
grant). Next free IDs: S-069+, M-157+, N-220+, I-049+, A-017+,
D-163+.
# SUMMARY.md — executive summary (the S84 MAJOR-first fix lane on top — the N-221 leak landing + the marker-repair concession + the MAJOR queue; the prior lanes below, newest-first)

## Session-84 S84 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major/informational first, one by one, 20+ fixes, fixed findings into findings/FIXED/, update all docs and counts"]; opened at core `27bda40e` — main RED at open; the racing S85 window landed the marker repair (N-219) mid-flight, conceded per SA-9; corpus `be0cfe9` throughout)

- **N-221 FIXED (MINOR, NEW — this lane's unique catch at the baseline
  ASan run):** the N-216 landing's `SizeCapHidesOversizeNode` lambda
  returned `tree.add_file(root, pr)` directly, never releasing its three
  FileProperties caller-side refs — LSAN at exit: 1170 bytes / 6
  allocations, exit 1 behind a green 5/5 (the FilterFolder battery) and
  a green full-suite gtest summary. The leak survived the racing
  window's marker repairs (a file none of them touched) — RED
  re-verified first-hand at `eeddec57` before the fix, GREEN post-fix
  (exit 0, LSAN clean). Born N-219 locally; renumbered to N-221 at the
  landing (the racing S85 lane took N-219/N-220 — the M-153
  born-collided-seat precedent, the renumber note in the file).

- **The marker-repair seat conceded per SA-9:** this lane's independent
  both-sides-kept reconstruction of the 27bda40e conflict (the A-016
  test + the M-151 battery) converged with the racing window's
  c7037bf3/f18ba226/1d7d31a6 landings; the convergence addendum is in
  their N-219 file. Two riders this lane's verification contributed on
  top of the concession: the matrix row's trailing `||` table-syntax
  break + stale title repaired (the run-growth probe's own row), and
  the N-221 leak above.

- **M-101 FIXED (MAJOR, the two missing branches):** the Versions
  node (the NTFS namespace-identity hash — two records for one
  pathname = versions of one file, the torn-overwrite population;
  persisted as nflags bit-14, queryable per-group and enumerable via
  query::version_groups()) and Tags Extracted (the runtime flag the
  host's preview pass stamps; NOT persisted — the og persists no tag
  state either, the finding's own defer note). The three landed
  branches regression-pinned unchanged. Honest trail note: the code
  rode the gate-repair commit b7d05b12 through an add -A mishap; the
  trail lands in its own commit.

- **M-078 FIXED (MAJOR, the destructive consent twin):** the
  CBitlockerTurnOffOperation endpoint — `win::bitlocker::turn_off` (the
  dual Disable-BitLocker / manage-bde -off chain, unlock()'s own
  duality; the og's exact verb undecoded, honestly registered), the
  `cr_ui_turn_off_bitlocker` ABI export #93 (request-phase progress; OK
  means accepted, not plaintext — the OS decrypt continues in the
  background, D-165), the CLI `unlock --turn-off C:` verb (the
  three-site lock). D-16's legitimate-unlock-only register untouched:
  decrypt, never bypass, the consent pin is a test
  (TurnOffIsTheConsentedDestructiveTwin). The 3-test seam battery +
  the export count sites refreshed (the A-ABI-3 class swept).

- **M-075 FIXED (MAJOR, the escalation endpoint):** the check ladder's
  fix rung landed — the pure grammar (the two postures of the guidance
  string, cross-platform/Linux-tested), the win-layer launch (elevated,
  synchronous with the documented 15-minute bound, WoW64-native via the
  redirection disable that realizes the og's IsWow64Process branch, the
  loud refusal on absent volumes), the `cr_ui_fsck_volume` ABI export
  #92 (all five count sites refreshed — the A-ABI-3 class swept in the
  same change), the `fix --chkdsk C:` CLI leg (the three-site lock),
  D-90 rewritten to the superseded posture, R-65 closed to PRESENT. The
  og's exact 5 arguments to WinChkdsk.exe stay undecoded (honestly
  registered; the direct chkdsk.exe form is the corpus-grounded base).

- **The racing window's same-window closures absorbed at the rebase:**
  M-111 (D-158 scope decision), M-114 (D-159 posture decision), M-154
  (the lifecycle + the hardening rider), I-048 (ACKNOWLEDGED, D-155
  through D-157), N-145 (the in-flight journal), N-178 (the pause CLI
  legs), N-181 (the RAW-first start order), M-107's CRW member (member
  2 of 11), N-220 (the fifth-recurrence class record) — all verified
  present in FIXED/ at this lane's rebase head.

**Board after M-101 (filesystem truth at this push): 0 ACUTE ·
0 SEVERE · 5 MAJOR · 2 MINOR · 0 INFO active / 199 FIXED / 4
tracked-open** (the 8 active MAJORs: M-075/M-078/M-086/M-087/M-089/
M-101/M-105/M-107; the 2 active MINORs: N-107/N-127 — of which
M-086/M-087/M-089 + N-107/N-127 are driver-repo-resident per OQ-S10-3
and out of this repo's scope). Next free IDs: S-069+, M-158+, N-222+,
I-049+, A-017+, D-160+.

---

## Session-84 S84 lane (this lane) — the audit-trail-restoration delta (2026-10-29)

**The third convergent S84 lane's unique landings** (the code repair and
the matrix marker sweep were conceded to the racing windows per SA-9 —
`cd3479e7`/`eeddec57`'s N-220 for the code, `15c59ee7`'s N-219 sweep for
the matrix markers; this lane's independently-derived two-site arbitration
agrees with both: BOTH occurrences of `0x310000 < DAT_1800728c8` in
FATSearchDLL_decompiled.c — :14834 + :30184 — are RETRY-leg gates; the
:14754 pre-walk re-base carries no big-volume gate; the S82 open
question 2 is closed by that arbitration):

- **The S-068 finding file restored** from the `ad1bcb9e` tree — the
  gutted S80 verification addendum + Resolution (the `9b4a9302` damage
  had replaced them with text naming a `test_fat_high16.cpp` that never
  existed on main) — plus the repair addendum closing the gate question.
- **modules/fat.md's S80 landing addendum restored** (the damage had
  replaced it with the wrong-gate variant note) + the repair note.
- **The matrix's residual rollback rows re-landed**: the A-016 MATCH row
  (deleted by the damage, absent from the N-219 sweep) and the N-145
  stale DIVERGENCE row → MATCH (the S81 landing's recorded state).
- **The nested marker blocks resolved in FIXED-LEDGER.md, I-048's
  finding file, docs/PARITY-DELTAS.md, and worklog.md** (the
  0fac15de/309f7af8/09a18f9f residue — the marker class's sixth
  recurrence, five live blocks on main at this lane's open; the only
  carrier left is M-120's sanctioned evidence quote).
- **N-220's convergence addendum + the ledger row** (the racing filing
  had no ledger row): the full rollback record — the permanent note that
  `9b4a9302`'s "No code change rides this commit" claim cannot be
  trusted for the diff it pushed (a 143-line code rewrite, a finding
  gutting, a matrix rollback).

RED→GREEN first-hand at this lane's repaired tree (before the racing
windows were found): the two RED tests green, FAT family 94/94, full
suite 1657/1657; the suite re-verified green after every rebase.

- **M-107's DWG member LANDED (member 3 of 11 — this lane's first
  finding fix after the restoration):** the three-generation walk
  (BruteForce::CDWGAnalyser, FUN_18002f320; the factory's initial scan
  position 0x15 — the constant the prior lanes never decoded; evidence:
  evidence/s84-lane-sheets/s84-l1-dwg-walk-decode.md). Mode 1
  (R12-R2000): count u32@+0x15, 9-byte records {index gate, seeker@+1,
  size@+5}, extent = max(seeker+size), the index-mismatch decline, the
  device-end salvage. Mode 2 (R2004-R2018): the 108-byte section map at
  +0x80 XOR the MSVC-LCG keystream, six validation gates, extent =
  u64@+0x34 + 0x6C. Mode 3 (AC1021/R2007): the recorded remainder (the
  bounded extent stands in for it alone). The version set tightens to
  the og's exact tables {12,13,14,15,18,21,24,27,32}. RED→GREEN
  first-hand via the stash dance (4 walk tests RED at the stand-in
  tree); the carve suite 279/279.

**Board after the DWG member (filesystem truth): 0 ACUTE · 0 SEVERE ·
8 MAJOR active (M-075/M-078/M-086/M-087/M-089/M-101/M-105/M-107 — M-107
at members 1-3 of 11 landed, 8 remain) · 2 MINOR · 0 INFO / 4
tracked-open.**


---

# SUMMARY.md — executive summary (the S79 MAJOR-first fix lane on top — the M-073 seat conceded to the racing window per SA-9 and re-verified; the prior lanes below, newest-first)

## Session-79 S79 fix-lane delta (2026-10-29 — the user's fix mandate continuation ["fix all the audit findings (priority to major first) one by one... Aim for atleast 20+ fixes in this session, dont stop before that"]; opened at core `e17a4e6`, corpus `be0cfe9` throughout; baseline suite green re-run first-hand at the open [1484/1442/0]; this lane's own M-073 implementation was CONCEDED to the racing window's landing at the push rebase per SA-9 — the seat's `cr_folderfilter` landing verified first-hand instead, and the verification CAUGHT TWO REAL DEFECTS, filed + fixed this session)

- **A-016 FIXED (ACUTE, NEW — filed+fixed this session at the convergent
  verification)** — the M-073 landing's folder-filter path walk treated
  the NTFS volume-root "." node (MFT record 5, $FILE_NAME ".", reparented
  to the Found root) as a path SEGMENT: every caller prefix turned
  incompatible, the "." directory detached WITH the whole volume, and a
  filtered NTFS scan returned an EMPTY tree with near-zero counters —
  wrong recovery results on the dominant Windows filesystem, invisible to
  the landing's FAT-only test battery (the FAT engines emit no "." node).
  The fix: the transparency rule in `prune_children` — a directory whose
  key is "." recurses with the PARENT's path (never a segment, never
  evaluated, never detached; "." is a forbidden user name on Windows so
  the rule cannot misfire). RED first-hand at `5291076`
  (`NtfsVolume.FolderFilterRootDotIsTransparent`: kept_in_scope 0) →
  GREEN (readme.txt/deleted.dat kept through the "." node, the $-meta
  siblings correctly pruned, a partial prune never the whole volume); the
  landing's 13 FolderFilter tests stay green.

- **N-218 FIXED (MINOR, NEW — filed+fixed this session)** — both detach
  paths (`detach_child` — the landing's primitive — and
  `detach_subtree`) subtracted FullNode intermediates from `live_files_`,
  which only `add_file` ever ticks (N-166's documented contract): a
  detached subtree holding an intermediate folder (the session-loaded
  marker=2 shape under the prunable roots) WRAPPED the unsigned counter
  (18446744073709551615 in the reproduction), corrupting the mid-stage
  telemetry samples. Both paths now subtract non-full nodes only.
  RED (`NodeTree.DetachChildLiveCounterCountsOnlyAttachedFiles`) → GREEN
  (0, node_count 1).

- **M-073 CONCEDED-VERIFIED (the seat)** — this lane's convergent
  `cr_scopefilter` implementation was dropped at the rebase per SA-9
  (the racing window's `cr_folderfilter` landing took the seat); the
  landing's corpus ground (DRWUI :567668-567690, UIUX/31:142) re-read
  and its 13 tests + the full suite verified first-hand on the merged
  tree — the verification is what surfaced A-016/N-218 above. The
  lane's trail block is recorded at the finding file's history.

**Board after this landing (filesystem truth): 0 ACUTE · 1 SEVERE (S-068)
· 17 MAJOR · 10 MINOR · 1 INFO active / 176 FIXED / 4 tracked-open.**
(A-016 and N-218 were both filed AND fixed this session — they land on the
FIXED side directly; the 10 active MINORs are the S81 board's set.)
Next free IDs: S-069+, M-156+, N-219+, I-049+, A-017+, D-151+.

---

# SUMMARY.md — executive summary (the S83 fix lane on top — the M-156 X3F correction; the prior lanes below, newest-first)

## Session-83 S83 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first", 20+ fixes]; opened at core `2d614cf2` (the S82-close tree: the racing lanes had repaired the red main, landed S-067, M-073, N-211, N-214, N-215 and the X3F four-lane arbitration while this lane's first composite was being built — the push rejection on the moved remote was the save; the composite was withdrawn unpushed per the S82 lane's own precedent), corpus `be0cfe9` throughout)

- **M-156 (NEW, filed+fixed this session — the X3F surface's FIFTH
  touch):** the four-lane arbitration left TWO defects standing. (1)
  **The live bug:** the landed ASCII version gate rejects every real
  Sigma X3F file — real files carry the BINARY u32 version 0x00020001,
  the gate demanded an ASCII digit + point; real RAWs never carved at
  all. The og's chain never jointly rejects a readable FOVb (the modern
  tier-1 FUN_180040080 accepts the binary shape; the legacy tier-1
  FUN_180053590 accepts any readable header). (2) **The refuted
  mechanism:** N-217's dispatcher claim ("vtbl[+0x40] consults the
  secondary FIRST — the Brance count-walk never contributes") does not
  fire for the X3F node: the consult requires the node class to
  override +0x40 with the real getter (FUN_180013700), which exactly 8
  node classes do — the X3F node AND the DMG node (the M-090 precedent
  the lanes leaned on) both carry the return-0 STUB at +0x40,
  byte-exact from .rdata. The magic-key path provably wires the MODERN
  count-walk for binary versions (the node's binary gate → the factory
  → the 0x120 walker with the 100 MiB cap → the +0x20 feeding state
  machine → extent = SECd + 16 + count*12). Evidence:
  `evidence/s83-lane-sheets/s83-l1-x3f-chain-raw-asm-decode.md`
  (instruction-address cites against the corpus binary — the prior
  lanes' Ghidra line cites are CI-generated and unverifiable from the
  corpus).

- **The landing:** the version gate is GONE (the union acceptance); the
  version shape routes the generation (binary → the modern count-table
  walk; anything else → the legacy fixed-tail walk, the four-lane
  convergent semantics preserved; torn → the legacy salvage); the row
  bound takes the og walker's own cap constant (0x6400000 = 100 MiB).
  Tests: the five legacy-path pins unchanged + five new pins incl.
  `X3fRealSigmaBinaryVersionCarves` (RED at the open tree — the
  headline pin — and GREEN post-fix) and `X3fChainRoutesByVersionShape`
  (the router itself). The X3F battery 10/10; the full suite green.
  Honest residue recorded in M-156: the +0x20 feed-loop caller and the
  registry container merge are undecoded — the binary-with-directory
  extent (count-walk vs legacy constant) disagreement is RECORDED with
  its decisive next probe, not papered over.

- **M-152 FIXED (MAJOR, ext4 — this lane's second landing):** the
  orphan-reserved skip is GONE. Corpus re-verified first-hand from the
  raw asm (the filter at 0x180005f5d-0x180005f87: four content gates,
  NO dtime/links read); the orphan-active population (unreachable-but-
  intact inodes — torn parents, unlinks without link decrements) now
  surfaces under the deleted root with its extents, the honest Active
  state, and the new orphan_inodes counter. The live-claim dedup and the
  mode-gate hardening pinned; the journal-inode claim registered
  (PARITY-DELTAS D-163/D-153). RED→GREEN first-hand via the skip-line
  reintroduction. Suite 49/49 ext4 + full suite green.

- **M-107's CRW member LANDED (member 2 of 11 — this lane's third
  landing):** the CIFF heap size walk, raw-asm decoded first-hand
  (evidence/s83-lane-sheets/s83-l2-crw-ciff-walk-decode.md): the CRW
  child walker searches zero-tailed sectors for CIFF directory entries
  {u16 0x2807, u32 len ∈ {0xa8→+0x46, 0xb2→+0x3c, 0xe4→+0x32}} with the
  walker's OWN 50 MiB cap; no entry → the cap salvage. validate_crw
  replaces the blind 64 MiB stand-in; both registry rows carry the og's
  cap constant. RED→GREEN via the stash dance; the carve suite 174/174.

**Board after this landing: 0 ACUTE · 0 SEVERE · 15 MAJOR · 10 MINOR ·
1 INFO active / 177 FIXED (M-156 + M-152 [convergent, conceded] + M-107's
X3F and CRW members this lane; the racing window's S-067/S-068/M-073/
M-113/M-149/M-150/N-211/N-213/N-214/N-215/N-216/N-182/A-016/N-218/M-151/
N-178 absorbed) / 4 tracked-open.** Next free IDs: S-069+, M-157+,
N-219+, I-049+, A-017+, D-151+.

# SUMMARY.md — executive summary (the S80 parity fix lane on top; the prior lanes below, newest-first)

## Session-80 S80 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first", 20+ fixes]; opened at core `04fb5e88` — main was RED at open; the racing M-155 lane `6d74ec2c` + the S81 lane `9508b90b`/`567694e7`/`3dfb2a5a`/`59a431d3` landed mid-session and were absorbed at the rebases; corpus `be0cfe9` throughout)

- **N-217 FIXED (the red-main repair, convergent with M-155)** — the ed501caa
  push's committed conflict markers (the M-120/N-210 class's third recurrence,
  plus a false "suite 1570/1570" claim). This lane's decisive contribution: the
  SCAN-DISPATCHER decode (RAWSearchDLL :9850-9876, :9900-9935) — the secondary
  (File*Analyser) is called FIRST with the result object, the Brance primary
  is the 2-arg second-chance; M-155's X3F resolution (the S71 FileX3FAnalyser
  walk, SECd + 0x4C, the count never read) confirmed by dispatch evidence
  rather than precedent. The registry census's "modern first" prose recorded
  as an erratum (the M-107 Session-80 addendum). This lane's own initial
  "unified chain" hypothesis was corrected by the decode before any code
  seesaw.

- **S-067 FIXED (SEVERE — the exFAT chain walker's out-of-range FAT-entry
  clamp)** — `ExfatFat::next()` clamps every entry ≥ cluster_count to EOC
  (the og's walk-loop clamp at exFATSearchDLL:41348-41351; the count's
  provenance: volume-context +0x18 = the boot +0x5C field verbatim,
  RAWSearchDLL:36750 — both re-verified first-hand); the loop gains the og's
  `c < 0xFFFFFFF6` bound (:41315). Tests: the finding's 3-cluster mid-chain
  fixture shape, RED 3/3 → GREEN 3/3 via the stash dance
  (OutOfRangeFatEntryEndsChainCleanly / ExactClusterCountEntryClampsToEoc /
  ReservedFFF6EntryEndsChainCleanly); the EXFAT-2/EXFAT-3 families re-pinned;
  exFAT suite 44/44. The damaged-FAT semantics family is now three-for-three.

- **S-068 FIXED (SEVERE, fat — the SEVERE tier closes; the S80
  diff-to-fix lane)** — the og's deleted-FAT32 high-16 re-base: the
  scan-global last-good first-cluster hint (DAT_1800728c8, :13753-13758)
  + the pre-walk low-16 re-base of torn-high-word deleted dentries
  (`hint_high | dentry_low16`, the < 0x10000 && < hint gates,
  :14753-14755) + the repaired-pair emission (the :14840-14842
  write-back semantics). The adoption criterion is the og's
  walk-confirmation shape: the re-based cluster's own FAT entry must be
  live — FREE never adopts (the raw walk decides, GAP-S6-2 intact on
  the raw path). The assumed provenance (never Excellent) +
  FatScanStats.high16_rebiased surface the population. Tests RED
  first-hand (stash dance): DeletedHigh16ZeroRebasesFromHint /
  DeletedHigh16RebaseFallsBackToRawWalk /
  LastGoodHintUpdatesOnlyOnFileWalks / FAT16NoRebase; suite 1604/1604.
  Trail: findings/FIXED/S-068 + the S80 verification addendum, matrix
  MATCH, modules/fat.md S80 addendum.

- **M-149 FIXED (MAJOR, lost-partition)** — the ghost-score two-pass
  form restored (the M-017 convergence debt): the +500 table-match
  bonus is NUMERATOR-only (the og's max loop :842210-842240 has no
  bonus walk), the count==0 edge is the og's 0.0 floor
  (CalcPartition.log's zero row), and a matched leader legitimately
  exceeds 1000 permille (the og's raw double >1.0 — PARITY-DELTAS
  D-151 registers the representation). Seven stale empty-candidate
  pins re-truthed to the zero floor (the finding's own inflation
  class). Tests RED first-hand (stash dance):
  ScoringTableMatchBonusIsNumeratorOnly / ScoringZeroCountScoresZero /
  the MatchesLogRows re-pin; suite **1624/1624**. Trail:
  findings/FIXED/M-149, matrix MATCH, modules/lost-partition.md S80
  addendum.

**Board after this landing: 0 ACUTE · 0 SEVERE · 13 MAJOR · 10 MINOR ·
1 INFO active / 179 FIXED / 4 tracked-open.** Next free IDs:
S-069+, M-156+, N-218+, I-049+, A-016+, D-150+.

# SUMMARY.md — executive summary (the S81 MINOR-first fix lane on top; the prior lanes below, newest-first)

## Session-81 S81 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to minor first", 20+ fixes, fixed findings into findings/FIXED/]; opened at core `04fb5e88`, corpus `be0cfe9` throughout; the S81 lane executed the identical M-155 repair independently before pulling — the convergence addendum in the M-155 finding records it; the seat conceded to the racing `6d74ec2c` per SA-9; this block re-landed after the N-217 lane's rebase dropped the S80-parallel blocks)

- **N-211 FIXED (MINOR, orchestrator/api-docs):** the `autosave_dir`
  contract comments re-truthed at BOTH headers — `cr_scan.h`'s "a
  cancelled run never auto-saves" and the twin at `cr_ui.h:636` the new
  gate caught (the shim's own comment, whose code has stop-window-saved
  since S-057). The og truth now stated: the field fires on completion
  AND on the stop window (DRWUI :1549399-421 → FUN_140395e50, gates
  :612571-587 no stopped-check — corpus re-verified this session). The
  guard: `doc_truth_gate.py`'s `check_header_contract_phrases()` — RED
  pre-fix (the cr_ui.h catch), GREEN post-fix.

**Board (filesystem truth at this push): 0 ACUTE · 2 SEVERE · 17 MAJOR ·
10 MINOR · 1 INFO active / 173 FIXED / 4 tracked-open.** Next free IDs:
M-156+, N-218+, I-049+, A-016+, D-150+.
# SUMMARY.md — executive summary (the S80 fix lane on top; the prior lanes below, newest-first)

## Session-80 S80 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to severe first, one by one, 20+ fixes"; opened at core `04fb5e88` — RED at open, see M-155; corpus `be0cfe9` throughout])

- **M-155 FIXED (MAJOR, NEW — filed+fixed this session)** — **main was
  RED at open**: the `ed501caa` push (the S71 X3F landing) committed
  unresolved merge conflict markers in `src/core/carve/cr_analysers.cpp`
  (`validate_x3f`) and in the M-107 finding file, with a false
  "suite 1570/1570" claim in the commit message (the tree cannot
  compile; the N-210 gate was not run before that push). Resolved
  corpus-first: FUN_1800535b0 (FileX3FAnalyser) re-read in full —
  SECd + 0x4C in all three found-branches, loop-exit salvage, the count
  never read; the M-090 DMG precedent establishes the File*Analyser
  secondary as the size-walk authority (the S78 lane's
  FUN_1800404a0 count-walk decode — the Brance streaming callback —
  superseded). The S71 body's own coordinate bug fixed (the absolute
  probe limit mixed with relative offsets: reads past the device end on
  small volumes fell to the torn path even with SECd in range —
  `Carve.X3fExtentIsSecdPlusFixedTail` RED against the ed501caa body).
  The S78 test trio replaced by corpus-true pins. Suite **1578/1578**
  at the repair; N-210 lint clean.

- **M-150 FIXED (MAJOR, session)** — the count pre-allocation bomb
  gate: deserialize_tree's count field is bounded by the wire length
  (count > (len-4)/10 refused instantly with ERR_SESSION_FORMAT — each
  node consumes ≥ 10 wire bytes; the og's per-node factory reader is
  bounded by the bytes present, rsf-record4:16,26). No count-scaled
  allocation ever happens on the load path — the old value-initialized
  by_id(count) touched up to 34.4 GB at the u32 max (the M-131-class
  torn-write read-side complement: pagefile thrash on Windows,
  OOM-kill on Linux overcommit). Tests RED first-hand (stash dance):
  HostileCountBombRefused / CountAtWireBoundFailsCleanlyBeyondIt /
  TornTailGarbageCountStillFailsCleanly; suite **1627/1627**. Trail:
  findings/FIXED/M-150, matrix MATCH, modules/session.md S80 addendum.

- **M-151 FIXED (MAJOR, ntfs)** — the truncated-$MFT run-growth probe:
  per run, 4 KiB windows probed past the declared end, consecutive
  records validated (FILE magic + fixup_validate with N-213's pre-gate
  inside), the run length grown by the validated bytes (cluster floor);
  termination per the og (first invalid / failed read / next run's
  start / device end). A runlist that LOADS but under-declares has its
  tail annexed — the recently-written MFT-tail records become walkable.
  The mirror population unaffected (the S-063 fixtures' holes made
  physical, re-pinned). Tests RED first-hand (stash dance):
  TruncatedRunlistGrowsToValidRecords / RunGrowthPartialWindowCounts
  ValidPrefix; + RunGrowthStopsAtFirstInvalidRecord /
  MirrorFallbackUnaffectedByGrowthPass; NTFS suite 95/95; suite
  **1634/1634**. Trail: findings/FIXED/M-151, matrix MATCH,
  modules/ntfs.md S80 addendum.

- **M-154 FIXED (MAJOR, session/orchestrator)** — the auto-session
  lifecycle: retention-one (the previous auto session named by the
  pointer is deleted before every auto-save — both autosave bands; the
  unbounded accumulation ends), the pointer (session::LastScanInfo with
  the og's [Info] four fields, the last_scan.info sidecar —
  PARITY-DELTAS D-164 registers the location convention; the HKLM
  mirror dropped), and the pointed resume
  (cr_ui_session_last_autosave_path, PathFileExistsA-validated). The
  auto_-name retention guard = the registered safety superset. Tests:
  AutosaveRetentionDeletesPreviousAndPointerNamesIt (RED: BUILD-absent)
  / ManualSaveSurvivesRetention / DeadPointerDoesNotBlock + the C
  consumer check; suite **1651/1651**. Trail: findings/FIXED/M-154,
  matrix MATCH, modules/session.md S80 addendum, PARITY-DELTAS D-164.

**Board after this landing: 0 ACUTE · 2 SEVERE · 18 MAJOR · 11 MINOR ·
1 INFO active / 170 FIXED / 4 tracked-open** (M-155 filed+fixed same
session; the +1 MAJOR is M-155 itself, closed on filing).


# SUMMARY.md — executive summary (the S69 continuation audit lane on top; the prior lanes below, newest-first)

## Session-82 S82 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first, 20+ fixes, one finding / one verified fix / one push"]; opened at core `04fb5e88` on the RED tree, re-based at `d37840c4` after the racing lanes' X3F/S-067/N-211/N-215 landings were absorbed; corpus `be0cfe9` throughout)

- **The X3F arbitration conceded after first-hand dispatcher
  verification:** this lane initially reached the OPPOSITE (composite,
  modern-primary-first) arbitration from the census artifact's protocol
  prose and its own reads of both walks; the racing lanes' dispatcher
  decode (:9849-9876, :9900-9935) was re-verified directly in the corpus
  before conceding — the secondary File*Analyser runs FIRST with the
  result object, and FUN_1800535b0's never-decline stamp means the
  Brance count-walk never contributes to a carved X3F extent. The
  wrong-order composite was withdrawn pre-push (the push rejection was
  the save); the convergence record + the census-prose erratum
  confirmation are in N-217's S82 addendum (findings/FIXED/). No
  code change by this lane on the X3F surface.
- **S-068 — CONCEDED to the racing `ad1bcb9e` (the third independent
  implementation of the same finding this window):** this lane's variant
  (the walked-END hint, the 0x310000-gated re-base, the contiguous
  completion at the re-based anchor, four tests RED->GREEN on a sparse
  0x320000-cluster fixture) was withdrawn pre-push per SA-9; the S82
  convergence addendum in findings/FIXED/S-068 records the two open
  decode deltas (the hint's value; the 0x310000 gate's placement) with
  their one-volume synthetic discriminators for the next audit. The
  SEVERE tier is closed either way.
- **The lane's remaining queue (the user's order):** the MAJOR tier next
  (M-149 the scoring fold, M-150 the count bomb, M-152 the ext4 orphan
  skip, M-151 the NTFS run growth, M-154 the autosave retention, M-107's
  remaining members [CRW next], M-075/M-078/M-101/M-105/M-111/M-113/
  M-114), then the MINOR queue (N-216/N-145/N-178/N-181/N-182), then
  I-048's registers — one finding, one verified fix, one push. Already
  landed by the racing lanes this window (not re-fixed here): S-067,
  S-068, N-211, N-213, N-214, N-215, M-073, X3F.

## Session-69 S69 lane delta (2026-10-29 — the NINTH continuation audit lane of the user's second-session mandate [remove-the-fixed + go-deeper + mark-all-remaining, focus: sector numbers / scanning / accurate detection / calculation / mathematics / no stuck or freeze / local database / state management]; the S68 seat conceded to the racing citation-census lane per SA-9 — this lane executed the S61 seat's never-landed plan, a disjoint slice; opened at core `3799dee2`, findings verified there and re-verified LIVE at the rebase head `271f5d06` after absorbing the S71..S79 fix windows [M-118/M-135/M-136/M-143/N-140/N-212/N-131/N-200/N-210/OQ-S52-1 landed by the racing lanes — zero of this lane's filings collided], corpus `be0cfe9` throughout)

- **The liveness battery (28/28 at the open pin):** every then-active
  finding's core-side claim re-verified first-hand — 23 core-side STILL +
  5 driver-repo ACTIVE-OPEN (OQ-S10-3); zero falsely-active, zero
  phantom-fixed. The racing fix lanes retired M-118/M-135/M-136/M-143/
  N-140/N-200 + OQ-S52-1 in the same window — all correctly in FIXED/.
  Evidence: `evidence/s69-lane-sheets/liveness-battery.md`.

- **Four new findings + one register filed** (all corpus cites read
  first-hand by the lead; all core cites re-verified LIVE at `271f5d06`):
  **M-154** (MAJOR — the auto-session lifecycle: the og deletes the
  previous auto .rsf via the LastScanInfo.ini pointer before writing the
  new one [DRWUI:442815-442833], keeps exactly one live auto session,
  writes the 4-field pointer + an HKLM `SOFTWARE\EASEUS\DRW` mirror
  [:443060-443086], resumes pointed [:443092-443127]; the core never
  prunes — unbounded accumulation, no pointer); **N-214** (MINOR — the
  lost-partition ext probe's journal-indicator XOR gate: PRD:1273-1293
  rejects torn SBs with exactly one journal indicator set; core's
  probe_ext accepts them); **N-215** (MINOR — R-20's mechanism
  mis-attribution: the og's "Unimportant files" bucket is the
  usefullostfiles.xml extension gate at tree-build [DRWUI:635764-635910],
  not the M-018 junk-name filter); **N-216** (MINOR — version.ini's
  `MaxFileSize ` cap surface [GiB units, default 1 TiB, two config
  objects, consumer undecoded] has no core counterpart);
  **I-048** (INFO — three registers: FileFilter.xml Selected=2 dead
  data; the remote-tunable summary-family groups; the og's 32-bit
  lost-partition ext size vs its own engine's 64-bit).

- **Two addenda to active findings:** M-151 gains the truncated-$MFT
  family's INITIAL-bound site (FUN_18000ecb0, NTFS:11274-11277 — the
  device-end-derived record count when no declared size exists);
  N-182 gains the deep-range resolver's full first-hand decode
  (FUN_14035a6d0, DRWUI:575024-575170 — the <300 MB rule is a backward
  extension to LBA 0; the neighbor extension is bounded to ONE following
  FAT16-family entry; the device-type-6 open question narrowed to
  CDiskDeviceType's +0x20 field, data-driven, ctor default 4).

- **MATCH confirmations:** the FAT DOS-timestamp invalid class (A-014,
  re-verified against FAT:32135/:32144/:32151 first-hand); the auto-session
  naming (N-061, byte-parity against all three shipped samples); the
  RawConfig gating semantics; the FileFilter membership data; the autosave
  trigger gates (S-044/S-057); the racing lanes' OQ-S52-1 landing verified
  LIVE. The prior-coverage spot checks (S55-D1's EUUserData cites, M-129's
  gates, the log-channel dispositions) all re-confirmed.

- **The not-reached register (honest):** the engine-side cancel-pause poll
  census and the quick→advanced transition trigger were not decoded (the
  subagent infrastructure timed out twice; the lead re-scoped to the
  resolver decode + the three deep dives). The MaxFileSize consumer and
  the APPDATA subfolder constant remain undecoded (N-216's open
  questions). No UNVERIFIED finding was left untracked: every candidate
  either filed with evidence, registered INFO, or carried in an active
  finding's open questions.

**Board at this lane's filing: 32 active — 2 SEVERE (S-067/S-068, the
racing S79 lane's) · 17 MAJOR (16 racing + M-154) · 12 MINOR (9 racing +
N-214/N-215/N-216) · 1 INFO (I-048) / 169 FIXED / 4 tracked-open
(OQ-S79-1 new).** Next free IDs: S-069+ (unused — this lane filed none),
M-155+, N-217+, I-049+, A-016+, D-150+.

# SUMMARY.md — executive summary (the S78 MINOR-tail fix lane on top [re-homed s70->s76->s77->s78 per SA-9: the racing windows took S70-S77]; the prior lanes below, newest-first)

# SUMMARY.md — executive summary (the S79 MAJOR-first fix lane on top; the prior lanes below, newest-first)

## Session-79 S79 fix-lane delta (2026-10-29 — the user's fix mandate ["fix all the audit findings, priority to major first, 20+ fixes"]; opened at core `e17a4e6d`, corpus `be0cfe9` throughout; every finding independently re-verified against the corpus and the running code before any change; baseline suite green first-hand [1581/1581 post-M-073]; the lane's board re-truth at open: 12 MAJOR / 7 MINOR active [the S78 block's "8 MINOR" was an off-by-one against the disk] / 168 FIXED / 4 tracked-open, of which M-086/M-087/M-089 + N-107/N-127 are driver-repo-resident per OQ-S10-3 and out of this repo's scope)

- **M-073 FIXED (MAJOR, orchestration/query/ABI/CLI)** — the scan-time
  folder filter (the og's PushFolderFilter, UIUX/31:142 re-verified
  first-hand): ScanConfig.filter_folders + the cr_folderfilter prune at
  every stage boundary BEFORE the checkpoint emit (out-of-prefix folders
  never reach a session/checkpoint/snapshot/query/recovery; counters
  decrement to the pruned tree) + ResultTree::detach_child + the ABI
  append + the CLI --filter-folder + the REC_SCAN_FILTER round-trip.
  Carve/pathless roots exempt (the registered scoping decision); composes
  with M-018's junk pass. 15 tests RED (wiring-disabled) -> GREEN.

- **M-113 FIXED (MAJOR, boot-repair/reghive)** — the recovered-OS
  write-side surgery landed core-side: reghive::HiveEditor (the offline
  hive VALUE editor - rewrite-never-create, inline/cell-reuse/new-hbin
  semantics, seq+checksum discipline) + reghive::surgery
  (rewrite_mounted_devices [12-byte DosDevices family, no-partial-write
  guarantee], repoint_bcd_device_element [the {9dea862c} 23000003 walk;
  REG_SZ wide bytes - the finding's REG_BINARY corrected by the decode],
  the bootstat contract [offset 10, byte 1]). Two finding annotations
  corrected by the first-hand decode (the BCD write type; the
  SOFTWARE-hive leg is a READ). R-69 flipped PRESENT->PARTIAL (truthful).
  11 tests RED (compile-error class) -> GREEN; suite 1607/1607.

**Board after this landing (reconciled over the racing S69 audit lane's
filings + the N-144/X3F landings absorbed at the push-rebase): 0 ACUTE ·
2 SEVERE · 15 MAJOR · 12 MINOR · 1 INFO active / 171 FIXED / 4
tracked-open** (M-086/M-087/M-089 + N-107/N-127 driver-repo-resident per
OQ-S10-3). Next free IDs: S-068+, M-155+, N-217+, I-049+, A-016+, D-150+.


