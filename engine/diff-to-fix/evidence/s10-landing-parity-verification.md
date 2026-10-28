# s10-landing-parity-verification.md — the session-10 fix-wave verification pass (nine landings independently verified against their findings' og-decodes, the corpus, and the current tree)

**Lane:** session-10 continuation audit (wave 3) · **Date:** 2026-10-29 ·
**Audited tree:** `2beb555` · **Corpus:** `be0cfe9` · **Method:** three
parallel verification agents (session/preview family; decoder/carve family;
reghive/CLI/doc family), each reading the finding file (recovered from
history where the purge convention retired it), the landed code, and the
corpus/decompile citations first-hand; the lead re-verified every
load-bearing PARITY-DIVERGENT/CONCERNS claim before filing (the
test-doctrine line, the gap-board staleness, the phantom-SHA object-store
checks).

## Verdict table

| Landing | Fix commit (real) | Verdict | Basis (first-hand) |
|---|---|---|---|
| **S-044** auto-save gates | 7b7e095 | **PARITY-VERIFIED** | All three og gates (not-import / results-found / not-already-saved) on BOTH surfaces (cr_ui.cpp:2030-2031 + rrecov.cpp:1732-1733); cancelled runs structurally cannot auto-save; 5 tests landed incl. the cancelled-run and empty-scan cases; the og trigger sites re-read at DRWUI :615166/:616764 (2 of 3 first-hand; the strictest 2-of-3 form adopted); ABI contract text + D-105/D-106 registered |
| **N-116** session-list ordering | 9445627 | **PARITY-VERIFIED** | save_unix DESC + path-bytes ASC tie-break (cr_ui.cpp:2848-2852); CLI twin (rrecov.cpp:992-998); contract text updated (cr_ui.h:876-883); the og comparator re-read first-hand (DRWUI :432112-432142, SYSTEMTIME→FILETIME descending); truncation upgraded to newest-prefix; 3 tests landed. Cosmetic: the CLI tie-break compares UTF-16 vs the ABI's UTF-8 bytes — identical under the N-061 ASCII sanitizer |
| **M-080** thumbnail budget | 5ccc0da | **CONCERNS → filed M-108** | Both fix-plan mechanics landed (byte-oversize embedded-fast-path leg; default raised 4→32 MiB; the ordinary-photo class fixed + tested). BUT the addendum's designated re-pin did not land: the og's 200 MB eligibility gate (DRWUI :422428) not adopted; the 32–200 MB band without embedded thumbs still refused; the pinning test still cites the refuted "no input cap" doctrine (test_thumbnail.cpp:187, lead-verified); no D-row |
| **M-085** TIFF LZW | fb7cf30 | **PARITY-VERIFIED** | Every claimed semantic present (MSB-first packing :763, early-change width growth :752/:813-819, KwKwK :821-828, slot-free first-code-after-Clear :832-835, table-full stop :836-839, 16-bit honest refusal); the fixture independently decoded with a from-scratch reference — the early-change variant reproduces the quadrant pins exactly, the deferred variant fails (the width-growth point is the load-bearing choice); TiffVariants/6 passes. Residuals are bookkeeping (the phantom edee513 SHA; no ledger row) and scope-adjacent (bps 1..7 LZW refused by the pre-existing header gate — GDI+ decodes them; out of M-085's landed scope) |
| **S-042** WMF validators | f6e303e | **PARITY-VERIFIED** | Gate-for-gate, offset-for-offset transcription of FUN_1800264d0 (re-read at RAWSearchDLL :29823-29838: checksum word@+20 = XOR words 0..9, zero dword@+16, hmf==0, std dword@+22, tail word@+38, size = Size@+28×2+22; the std branch's params gate); the invented inch-gate/checksum@+18 convention gone (grep-verified); fixtures regenerated from the spec-derived canonical layout with negative tests pinning BOTH wrong conventions rejected |
| **M-090** DMG koly walk | 9daa787 | **PARITY-VERIFIED** | The walk is a faithful rendering of FUN_180053de0 (re-derived term-for-term from the decompile: 512-byte chunk steps, the 8-zero tail gate, aligned startswith probe → chunk+0x200, the past-0x2800 previous-chunk full search → koly+0x200); the row cap = the ctor's own 0xC800000 (:49439, lead-verified lineage); the EOS walk demoted to the documented no-koly fallback; 3 tests landed. Residuals: the no-koly fallback extent still diverges (the finding's own sanctioned bounded-extent heuristic — documented); ledger row carries `180053de0` (the function name) as the fix SHA |
| **M-082** offline OS profile | 1f71488 | **PARITY-VERIFIED** | The CPartitionBootInfo walk ported corpus-exact (the 8-field CurrentVersion walk, the SystemRoot `_strlwr`+(c+0x9F)<0x1A gate, `\DosDevices\<x>:` with the 12-byte decode, Select-aware ControlSet with %03d + the 001 fallback — the %03d re-verified against the .mo binary at offset 0x7bf7c; ProductOptions tri-classification); C++-API-only surface = the plan's own staging (D-107); 5 tests landed. Edge: the landed code gates the whole SYSTEM pass on a valid SystemRoot where the og fills product_type regardless — an unpinned edge |
| **M-071** junk gates | ce2405a | **PARITY-VERIFIED** | Both rrecov.cpp gates byte-exact to the 69507cb form (:825-828, :1987-1990); the cr_ui bulk walks gated identically (:3745, :3886); the explicit-selection semantic pinned by test (junk stays recoverable when individually selected); the stale cr_scan.cpp comment corrected to name every gated surface truthfully; fail-before executed + the Windows CI junk-gate probes landed |
| **N-117** W1b plan/doc cites | e4f3184 | **PARITY-VERIFIED** | The W1b row names the landed cancel_flag shape with the divergence flagged; §7f cite exact; §7g cite contains the decl but overshoots 9 lines (N-129 item 6) |
| **N-118** gap-board rows | e4f3184 | **CONCERNS → folded into N-129** | The four named rows (R-52/R-54/R-56/R-31 primary) flipped with landing cites; but the second table's R-31 (:379) contradicts the primary (:225) — lead-verified; the step-3 finding-ID sweep not run (R-37 carries no M-100 anchor) |
| **N-103** thumbnail rows + heif | e4f3184 | **PARITY-DIVERGENT → folded into N-129** | The heif attribution (:90) and the primary R-28 (:222) fixed and corpus-verified (EUImgRepairFile imports get_thumbnail; EURawImg does not — re-read in ghidra-repair); but the finding's explicitly-cited SECOND site (:376) still reads "no dedicated op" — the Resolution's "the two R-28 rows" claim is false as written; :376 was in no hunk of the fix commit |

## Board mutations from this wave

- **M-108 filed** (MAJOR — the M-080 residual: the 200 MB anchor not
  adopted; the 32–200 MB band; the test-doctrine mis-pin; the unregistered
  bound).
- **N-129 filed** (MINOR — the bookkeeping truth batch: N-103's missed
  :376 site + the overclaiming Resolution; N-118's :379 self-contradiction;
  the five phantom SHAs edee513/771e2ec/dd0ffd5/95122bd/954c386; the
  ledger's missing rows + the 180053de0 mis-paste + the re-created FIXED/
  dir; the §7g overshoot; the .cpp matrix comment).

## The phantom-SHA root cause (recorded for the convention)

The fix lanes wrote resolution records against their pre-rebase local
SHAs; the push-time rebases (this lane's and the parallel lanes' — the
worklog carries at least three rebase windows this session) invalidated
them. The real post-rebase SHAs: M-085 → fb7cf30, M-090 → 9daa787, M-071
→ ce2405a, M-082 → 1f71488, the doc batch → e4f3184. Rule proposal
(N-129's fix plan item 3): cite SHAs only post-push, or cite by
title+first-line match.

## Independent full-suite run

The decoder-family agent built the tree fresh and ran the full suite at
HEAD: **1005/1005 PASSED** (linux-release). No build regressions from the
wave.
