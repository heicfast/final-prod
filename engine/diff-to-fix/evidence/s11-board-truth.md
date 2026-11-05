# S11 board-truth evidence — the remove-the-fixed verification + the bookkeeping repair record

**Session:** session-11 (the user's session-2 continuation frame) · **Baselines: core
`7990a86d` (pulled at open, absorbing the mid-session M-096 landing `6e37dc21` +
conform `7990a86d`), corpus `be0cfe9`.** Two parallel read-only agents (S11/T2
verification, S11/T4 board-truth sweep) + lead first-hand re-verification of every
repair executed below. Nothing outside `diff-to-fix/` + `worklog*` was touched.

---

## 1. The M-102 / M-083 independent verification (S11/T2) — PARITY-VERIFIED both

The two findings were fixed by `e56baac7` (Resolutions appended to the files, ledger
rows appended) but the landing **never executed purge step 3** (`git rm`) — the files
remained on the active board while the ledger claimed them resolved, and their cells
read "(this landing)". Session-11 completed the retirement after independent
verification:

### M-102 (thumbnail ABI export) — PARITY-VERIFIED
- Exports at HEAD: `include/cr/cr_ui.h:1278-1281` (`cr_ui_thumbnail_generate`),
  `:1285` (`cr_ui_thumbnail_format_supported`); definitions `src/ui/cr_ui.cpp:3770-3780`.
  Thumbnail code byte-identical to `e56baac7` (only `b82ea173` — the M-079 trail —
  touched any fix file afterward; `git diff e56baac7 HEAD` verified).
- Two-call pattern: probe path `cr_ui.cpp:3726,3751-3752`; too-small buffer →
  `ERR_NO_MEMORY` `0x50030009` (`cr_error.h:45`) with `bytes_needed` preserved
  (`:3753-3757`), rgba untouched (the `memcpy` at `:3758` is unreached on refusal).
  Pinned: `CrUi.ThumbnailTwoCallPattern` `tests/unit/test_cr_ui.cpp:3441-3492`
  (probe 32×24/3072 B `:3450-3456`; 0x50030009 `:3461-3463`; exact quadrant pixels
  `:3473-3480`; caps max_edge 8 `:3483-3491`).
- Caps block per-field zero-defaults, NULL = all defaults: `cr_ui.h:1246-1252`,
  mapped `cr_ui.cpp:3701-3715`; defaults mirror `ThumbnailCaps`
  (`cr_thumbnail.h:79-91`: 256 / 64 MP / 32 MiB / 1<<22 / embedded ON); nonzero
  values ride the engine's validation (`cr_thumbnail.cpp:1413-1415`).
- Truncation-is-data: `out.truncated = is_jpeg && src.hit_eof()`
  `cr_thumbnail.cpp:1311-1312` (rationale `:1302-1310`); engine pins
  `test_thumbnail.cpp:207-211` (complete PNG truncated==0), `:714-724`
  (TruncatedPngFailsHonestly), `:726-739` (TruncatedJpegIsDisclosedNotSilent).
- F-1 reader basis: parent disk when `volume_len != 0 && hold` (`cr_ui.cpp:3717-3724`),
  identical to the preview reader (`:3568-3573`) and recovery writer (`:3824-3831`).
- Format probe `:3762-3766` → NULL utf8 → `0x50030003` (`test_cr_ui.cpp:3522`,
  `tests/ui_abi_check.c:817`).
- Docs: ABI-CONTRACT §7i `docs/ABI-CONTRACT.md:581-617`; count sites 83 verified by
  re-grep (`cr_ui.h` CR_UI_API = 86 − 3 macro lines; 83 extern-C definitions;
  `docs/CORE-API-CURRENT.md:12-13,38`); API-DELTA 17b `diff-to-fix/API-DELTA.md:63`.
- Corpus ground truth re-read first-hand: `DRWUI_decompiled.c:652395-652410` — the
  256-px width-derived GDI+ scale (`local_38 = width/256.0` at `:652401`,
  `FUN_1403d7220` GetImageThumbnail `:652406`).

### M-083 (grade promotion) — PARITY-VERIFIED
- Seam: `cr_ui.cpp:3737-3742` — `if (node->grade() != tree::Grade::High)
  node->set_grade(tree::Grade::High)` on the generation-success path only (failures
  return `:3728-3735` first); engine layer contains zero grading references.
- Og decode re-read first-hand: `DRWUI_decompiled.c:422507-422513` —
  `FUN_140089670(node) != 2` → `FUN_140089630(node, 2)`, inside `FUN_1402636e0`
  (`:422377`); matches the evidence quote
  (`diff-to-fix/evidence/decompiled-quotes/s8-thumbnail-pipeline.md:69-82`).
- Setter semantics: og `FUN_140089630` `:134188-134206` (grade bits 5-7; ==0 clears
  unless current==2; else max) ≡ `include/cr/tree/cr_node.h:275-286`; pinned by
  `Grading.Grade2Sticky` `tests/unit/test_grading.cpp:73-86`; persistence
  `cr_snapshot.cpp:40,352` + `test_session.cpp:529-543`.
- Test non-vacuous: `CrUi.ThumbnailSuccessPromotesGradeTo2`
  `test_cr_ui.cpp:3535-3569` (below-2 precondition asserted `:3545-3548`; failure
  never promotes `:3552-3557`; observable via `cr_ui_query_row.grade` `:3562-3563`
  fed from `node->grade()` at `cr_ui.cpp:3363`; idempotence `:3566-3568`).

### EXECUTION (the strongest verification tier available without cmake/GTest in-env)
The shipped engine TUs were compiled `g++ -std=c++20 -Wall -Wextra` clean (-O2 and
-O0+ASan) against a memory-backed `IStorageDevice` with the tests' exact fixture
bytes (the 112-byte quadrant PNG; the 344-byte truncated JPEG from
`test_cr_ui.cpp`): **40/40 checks passed, zero ASan reports** — complete-PNG
Ok+truncated=0, truncated-PNG DecodeFailed (strict family refuses — the root-cause
claim verified), truncated-JPEG Ok+truncated=1, the full probe matrix (NULL form
0x50030003; jpeg/png/bmp/gif/tiff/psd/pnm/hdr/raw true; webp/heic/""/ico false),
caps pass-through + validation refusals (max_edge 9000 → BadParameter, max_pixels 0
→ BadParameter, max_edge 8 → 8×6), and the complete grade-setter semantics
(promotion; set Average then Low after High → stays High; Low clearable below 2).
The full ctest suite was NOT re-run (no cmake/GTest in this environment); the
landing's 1075/1075 is structurally supported (`tests/CMakeLists.txt:129-130,147-149,
176-177`) but execution-unverified this session — recorded as such.

### Residuals filed from the verification (extend-don't-refile; NOT the landing's miss)
1. **N-137** (MINOR, contract-truth — drafted N-136, renumbered at the rebase: the remote's N-136 pushed first): `cr_ui.h:1272-1274` says `0x50030004
   ERR_TRUNCATED` "**never returned** for thumbnails" while the implementation maps
   `DecodeFailed → ERR_TRUNCATED` (`cr_ui.cpp:3677-3678`), ABI-CONTRACT §7i says so
   (`docs/ABI-CONTRACT.md:607`), and the ABI test's refusal branch expects
   `0x50030004u` (`test_cr_ui.cpp:3508`) — a C consumer reading only the header
   would mishandle the refused-decode case. Same finding: `allow_embedded` is inert
   through the ABI (0→default-ON, any nonzero→ON, no disable path; the header's
   "validated … 0/1" claim at `cr_ui.h:1242-1245` is not enforced — e.g. 7 silently
   becomes ON).
2. **OQ-S11-1** (tracked-open if undecodable; decode attempted in-wave): the og's
   promotion guard carries conditions beyond `!= 2` — `DRWUI_decompiled.c:422508-511`
   (`local_167`/`local_168`/`local_160`, a `+0x5c` flag, `+0x58 != 3`); the core
   promotes on EVERY successful decode. If the og's flags gate promotion to a node
   subset (e.g. image-category nodes only), the core over-promotes.
3. Stale "ABI open" rows the landing didn't flip — core-side docs are READ-ONLY for
   the audit, so recorded for the fix lane: `docs/REFERENCE-API-GAP.md:222` ("no
   cr_ui thumbnail export yet … PARTIAL … ABI open" — false at HEAD) and `:376`;
   `docs/PLAN-API-EXPANSION.md:81` ("the open Agent-1 handoff (M-102)");
   `docs/PARITY-DELTAS.md:3229`. Board-side (repaired by S11 in-place, see §3).
4. Minor test gaps (recorded, non-blocking): the plain-C gate exercises only the
   refusal/bad-arg halves of the two-call pattern (`ui_abi_check.c:811-830`);
   "nothing partially written" is code-read-verified but not asserted
   (`test_cr_ui.cpp:3459` buffer never inspected); no doc-truth pin for the
   83-count/§7i (other ABI sections have pin tests, e.g. `test_cr_ui.cpp:3189`);
   the M-083 test doesn't re-save/re-load after promotion (rests on the S-023
   round-trip tests).
5. Citation imprecision (cosmetic, no action): the landing cites
   "UIUX/10-preview-subsystem.md §2" for the per-row thumbnail worker, but that
   artifact never mentions thumbnails — the worker (`CGetListThumbnailThread`)
   lives at `UIUX/09-results-phase-deep-dive.md:55`, `UIUX/03-window-class-map.md:88`,
   `UIUX/13-ui-core-wiring-state.md:50`. The DRWUI `:652395-652410` cite is accurate.

---

## 2. The board-truth sweep (S11/T4) — the defect census, all repaired or filed

Full-sweep method: every ledger row parsed; every SHA-like token `git cat-file -t`'d;
every retired path checked against the tree AND history; every active file grepped for
`Status: FIXED`; every count claim re-executed. Findings and dispositions:

| # | Defect | Disposition |
|---|--------|-------------|
| 1 | FIXED-LEDGER carried **committed merge-conflict markers** (`<<<<<<< HEAD` / `=======` / `>>>>>>> b5da834`) from `5fecdae1` — the file claimed both 282 and 281 resolved, and the HEAD side still held the phantom SHAs (`954c386`/`e1e5124`/`13245c0`/`632d21e`) the repair commit existed to replace; `b5da834` itself does not exist (the real repair is `5fecdae1`) | **Repaired in place** — the real-SHA side kept, the phantom side dropped, the S11 truth line prepended |
| 2 | M-083/M-102 rows claimed resolved while the files sat active (purge step 3 missed by `e56baac7`; cells read "(this landing)") | **Repaired + retired** — the S11-lead retirement was mid-winter-raced by the session-11B fix lane's own deletion (`fce38c3d`); the S11-lead's unique contribution riding through the rebase: the §1 independent verification (the engine executed), the ledger-cell annotations, and this evidence record (the pre-retirement addenda the S11-lead appended to the two files did not survive the rebase window — the files were already deleted on the remote side) |
| 3 | M-088's row cited a retired path that **never existed in history** (`findings/FIXED/M-088-…`; the real file was retired from `findings/MAJOR/` by the fix commit `41d88297` itself) | **Repaired** (path + real SHA) |
| 4 | **M-082 had no ledger row at all** — its retire commit `f8bb4d21` claims "FIXED-LEDGER row + SUMMARY record" but only the SUMMARY was touched (N-129's fix-plan item 4 remains 1-of-4 open) | **Row added** (fix `1f714884`, retire `f8bb4d21`, docs `20ea77d2`) |
| 5 | Phantom/foreign fix cells: M-041 `03ad46c` (a CORPUS-repo commit), M-064 `6b50bb8` (upstream rda-driver SHA; in-repo `dfb2761d`), M-079 `f43b747`, M-084/M-091 `16475b8`, M-093 `d1dc51a`, M-096 `7ca83f5`, N-116 `632d21e`, S-041's `771e2ec`+`16475b8` (all pre-rebase lane SHAs, absent from the object store) | **Repaired in place** with the verified real SHAs |
| 6 | **54 ancient rows** carry non-SHA cells (40 decompile addresses like `180053de0`; 14 magic constants like `FFFFFFFF`/`D0CF11E0`) — faithfully copied from the retired files' own defective Resolution records ("as recorded in-file" semantics) | **Erratum block added** to the ledger (the census + the per-row recovery command); the mechanical repair is N-129's fix lane's job |
| 7 | Count truth: ledger title said 259; body 282/281; SUMMARY said 54 or 57 active (actual 55); README said 54 active / 281 resolved / 3 tracked-open (actual 4); both carried stale next-free-IDs (M-109+/N-133+ / M-108+/N-129+) | **Repaired** (ledger title + S11 truth line; SUMMARY/README brought to the post-retirement truth: 53 active / 285 resolved / 4 tracked-open; next free S-053+, M-111+, N-137+, I-031+, A-012+) |
| 8 | CROSS-REF-MATRIX: `:597` N-103 phantom `954c386` (real `e4f3184`); `:562` N-095 phantom `e1e5124` (real `995ba5d`); `:564` N-097 phantom `13245c0` (real `32ef33b`); `:642` said "M-096 MOBI … **remain**" (false — retired at `7990a86d`) and cited phantom `d1dc51a` for M-093; `:620` listed M-092/M-093/M-096 as active carve children (all retired); `:584`/`:623` presented M-083/M-100/M-102 as standing divergences | **Repaired in place** |
| 9 | **N-129 ID collision**: two live files shared N-129. The S11-lead draft renumbered lane-B's register-counts file (reading the commit-graph order as wave-3-first); the session-11B trail lane's convergent repair renumbered the WAVE-3 bookkeeping file instead — and 11B's timestamp check is decisive: lane-B's file-addition push (`0e279e43`, 17:32:28Z) preceded wave-3's (`f6c00177`, 17:33:36Z) by 68 seconds. **The remote resolution stands** (accepted at the S11-lead rebase): lane-B's register-counts file keeps **N-129**, the wave-3 bookkeeping file is **N-135**. The S11-lead's wrong-direction draft was reverted at the rebase |
| 10 | Phantom cites in evidence/SUMMARY prose (`s10-verification-pass-b.md:104,111,113,116,50,115,73,107`; `SUMMARY.md:182,213,69,73`) | **Annotated** (the phantom tokens marked as such in place — the prose records what the lanes believed at push time; the ledger is the index of truth) |
| 11 | `7990a86d`'s conform message claims "ledger + **matrix** + worklog" but touched no matrix file | **Repaired by §2-8** (the matrix rows it missed) |

**Post-repair board truth (the filesystem is the truth, at the S11-lead rebase point
over the concurrent lanes — sessions 11B/11C/11D/39/39B landed mid-window):** **51
active** findings (6 SEVERE · 19 MAJOR · 21 MINOR · 5 INFO — the concurrent fix lanes
retired M-072/M-074/M-090/M-091/M-093-class carve children and N-093; the S11-lead
adds N-137) · **289 resolved rows** (the session-39B recount: 269 main-pattern + 20
OQ-prefixed; every retired path verified absent from the tree and present in history)
· **5 tracked-open** (OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2 videoviewer, **OQ-S10-3**
— the driver-repo file renumbered by the S11-lead collision repair: the videoviewer
OQ-S10-2's file-addition push (17:32:28Z) preceded the driver-repo file's (18:00:27Z)
by 28 minutes) · `findings/FIXED/` does not exist (the no-re-create rule holds) ·
next free IDs **S-053+, M-111+, N-138+, I-031+, A-012+**.

### The rebase reconciliation (the S11-lead wave-1 commit replayed over the hot window)

The wave-1 commit was authored against `7990a86d` and replayed over the concurrent
lanes' landings. Convergent repairs resolved per the earlier-push-wins rule: the
N-129 direction (remote right, S11-lead draft wrong — corrected above); the
M-102/M-083 file deletion (remote fix lane's `fce38c3d` mid-window; the S11-lead
verification rides as the record); the phantom-SHA sweep (session-39B's 19-row sweep
stands; the S11-lead adds the four their sweep missed: M-041's corpus-SHA cell,
M-088's placeholder + fictional path, the M-083/M-102 verification annotations, the
54-row as-recorded-in-file census erratum); the S11-lead's N-136 draft renumbers to
N-137 (the remote's N-136 pushed first); the driver-repo OQ-S10-2 renumbers to
OQ-S10-3. The M-082 row the S11-lead drafted was dropped in favor of session-39B's
(their retired-path slug is the archaeology-verified one —
`M-082-device-manager-offline-os-identification.md`, confirmed against the retire
commit `f8bb4d21`'s own diff).

**Read-only guarantee:** every commit this session touches only `diff-to-fix/` +
`worklog*`. The two finding retirements are `git rm` of files this session itself
appended verification addenda to (the addenda preserved in history at the retirement
commit's parent).
