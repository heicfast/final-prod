# S50 evidence — the 74-finding liveness battery (the remove-the-fixed ask)

**Lane:** S50 continuation audit lane (2026-10-29). **Pins:** core `93461483`
(the S50 claim commit's tree = code-identical to `0d7b1d9d`, the S48-close HEAD;
`git diff 659f0f6c..HEAD -- src include` verified EMPTY at battery run — no code
fix landed inside the S45–S50 window), corpus `be0cfe9`.
**Method:** four parallel battery batches (A: ACUTE+SEVERE, B: MAJOR 1–14,
C: MAJOR 15–28, D: MINOR+INFO), each finding re-verified against the current
tree first-hand (cited lines re-read, absence-claims re-grepped, per-file
`git log` checked for post-filing fixes); the load-bearing claims re-verified
by the lead before this sheet was written (HIK/ONE/CSL/DXF/EXIF/D-118/clone/
N-142/OQ-1/OQ-2 both-sides reads — see the finding files and the sibling
evidence sheets).

## The battery verdicts

### ACUTE + SEVERE (10)

| ID | Verdict | Key evidence (current tree) |
|---|---|---|
| A-014 | **LIVE** | cr_fat_wire.h:123-142 gates month/day only; hour/min/2s flow unvalidated into the epoch math (time=0xC000 → hour 24); no FAT-side boundary test exists (only the exFAT M-014 gate test) |
| S-040 | **LIVE** | repo grep `image_create|create_image|cr_ui_image` → 0 hits; rrecov verb table :3476-3500 has 12 verbs, no image producer; cr_image_win.cpp:37 GENERIC_READ-only |
| S-050 | **PARTIAL** | leg 2 (false registration) fixed at filing (SURFACE-INVENTORY:98 re-truthed); leg 1 (capability) live: `src/core/repair/` = RawFixer only; CAPABILITY-MATRIX §3 still has no repair-line disposition |
| S-055 | **LIVE** | cr_carve.cpp:884-956 user-sign block precedes the built-in probe in the same iteration (`continue` on hit at :953); the :873-883 comment still claims built-in-first; the pin test plants only the grammar-layer leg |
| S-056 | **LIVE** | cr_carve.cpp:1319-1340 builds `trial_disabled`/`rawsign_disabled` only; no `usersign_disabled`; the user-sign block references neither map |
| S-057 | **LIVE** | cr_ui.cpp:2191-2212: the cancel path returns before the completion auto-save block; :2193-2200 comment still mis-states the og; CLI mirror :1854-1868/:1941-1943 unchanged |
| S-058 | **LIVE** | cr_grading.cpp:275 `if (!extension_in_grading_table(...)) continue;` — the inverted gate; test_grading.cpp:166-187 still pins the WRONG polarity |
| S-059 | **LIVE** | cr_runlist.cpp:190-191 `if (r.sparse) continue;`; Extent (cr_node.h:114-119) has no logical offset; safewriter :403-447 cumulative walk + ERR_TRUNCATED at :440-444 |
| S-060 | **LIVE** | DestIo (cr_safewriter.h:40-63) exposes no time/attribute op; repo grep SetFileTime/SetFileAttributes/utimensat → 0 hits |
| S-061 | **LIVE** | cr_grading.cpp:217-219 early return sets no grade for ReFS/Btrfs; the disproven comment premise verbatim |

### MAJOR (28)

LIVE 24: M-073 (ScanConfig :186-354 no filter input), M-075 (chkdsk grep 0),
M-078 (`manage-bde -unlock` only at cr_bitlocker.cpp:338), M-101 (no
version-group/tag axis in cr_query.h:128-167), M-105 (CreateService grep 0),
M-107 (all 11 stand-in sites with caps/comments at cr_analysers.cpp:4406-5783),
M-111 (no live-FS adapter), M-113 (boot surgery grep 0; cr_bootfix still
cell-repoint + NVRAM), M-114 (SR purge grep 0, no D-row), M-115 **PARTIAL**
(only the comment-cite leg landed via N-148 78e92955; decode-gating defect
fully live at cr_ui.cpp:4014-4029; the existing test pins the DIVERGENT
behavior), M-118 (no CandidateSource seam in cr_lostpart.h:56-74),
M-119 (cr_slice.cpp:46-49 read-at strict + :28-32 create strict), M-123
(cr_table_parse.cpp:29-70 has no reserved/position/usable gates), M-124
(`mbr_type_to_kind` :116-120 no case 0x85), M-126 (ETA grep 0; M-125's
landing verified to NOT cover it — the query family is percent-only),
M-127 (visited-vector linear scans at cr_chain.cpp:429-447/:475-491 +
cr_exfat_quick.cpp:91-107), M-128 (hex_nibble accepts both cases
:77-91; no PARITY-DELTAS row), M-129 (all-or-nothing parse verbatim
:194-221), M-130 (no shape gates in the dentry walk :140-254), M-132
(backup-probe asymmetry :316-324/:339-353 intact), M-133 (twin-key
FAT-family shift :130-142, the header comment self-documents the wrong
family), M-134 (GradeContext carries no reader; solo-only regrade
:338-347), M-135 (zero-fill+continue at safewriter :498-503; no cause
taxonomy in cr_report.h), M-136 (recovery hands the raw device at
rrecov.cpp:2323/cr_ui.cpp:4115-4118), M-137 (naming/cap/Quick/512 legs
all live at cr_scan.cpp:2048-2067, cr_candidates.cpp:759-760).

CANNOT-VERIFY (driver-repo halves; core-side halves verified SOUND) 3:
M-086 (core release.yml:291 -FloorVersion v0.2.4 consumption present),
M-087 (corpus contrast re-verified first-hand: og regdrv.reg:34-36 correct
UTF-16LE hex(7)), M-089 (core phantom-zero prose tail already swept c169332e
+ pinned test_cr_ui.cpp:3395-3399). Tracked at OQ-S10-3 (still OPEN).

### MINOR (32) + INFORMATIONAL (4)

LIVE 25: N-090, N-131, N-140, N-144, N-145, N-150, N-158, N-159, N-161,
N-163, N-165, N-166, N-167, N-169, N-171, N-174, N-175, N-176, N-177, N-178,
N-180, N-181, N-182, N-183, N-184 — every defect site re-read live; the
battery notes line drift at cr_carve.cpp/cr_scan.cpp/cr_ui.cpp/rrecov.cpp
(later landings above the sites) and zero post-filing fix commits per file.

PARTIAL 4: N-154 (item 2 fixed via N-164/D-116; items 1+4 live; item 3
half-landed — reghive.md+apply.md exist, tree.md/err.md still missing),
N-160 (items 4/5/7 materially moved; 1/2/3/6 live), N-170 (item 4 fixed via
N-179; 1/2/3/5/6/7/8 live — the pre-erratum EUUserData architecture still
in the header/docs), N-185 (code resolved at filing per its own text; the
owed N-157 erratum pointer not landed).

FIXED-ALREADY **1** — **N-142** (the battery's one falsely-active finding):
the fields it asks for (firmware[32]/wwn[32]/interface_desc[32]/
boot_snapshot[512]) landed with N-066's `4d3f0409` (2026-10-26 — three days
BEFORE N-142's filing) and exist at HEAD at cr_records.h:112-122 +
cr_records.cpp:809-862 + the Session.BootSnapshotRoundTrip test
(test_session.cpp:1402-1415). N-142's own "grep-verified: no firmware/WWN/
interface strings anywhere in the session layer" claim was FALSE at its own
audited commit c7fa527c (verified `git show c7fa527c:include/cr/session/
cr_records.h`). → **retired this session** as a false-active duplicate
(convergent with N-066), Resolution + ledger row landed; the optional
VolumeSessionInfo-exposure sliver noted there.

CANNOT-VERIFY 2: N-107, N-127 (driver-repo halves; core-side doc halves
verified truthful — DRIVER-CONTRACT.md:131/:197 carry the correct 19-code
truth and the admin-gate caveat). REGISTER-STANDS 4: I-035 (S-050 owner
still active), I-036 (default-load CLI-only posture unchanged; D-104's
imprecision stands as filed), I-040 (ext4 i_size_high conditional verbatim;
the proposed one-row registration still not landed — the FAT width member
was registered separately as D-119), I-041 (stage-mask stance re-affirmed
open by c4b57ff1's own repair).

### tracked-open re-verified (6)

OQ-S10-1 (vendoring plan — unchanged), OQ-S10-3 (driver-repo pair —
unchanged), OQ-S4-3 (the TTFR half — no committed value; unchanged),
OQ-S42-1/OQ-S42-2/OQ-S42-3 — **all three RESOLVED this session** by the
decompile probes (see the sibling evidence sheets + the disposition sections
appended to the OQ files).

## Summary

**74 active checked → 63 LIVE · 6 PARTIAL · 1 FIXED-ALREADY (N-142, retired)
· 5 CANNOT-VERIFY (all driver-repo: M-086/M-087/M-089/N-107/N-127, tracked
OQ-S10-3) · 4 REGISTER-STANDS (INFO by design). Zero phantom-fixed. Zero
false retirements.** The board's active set is honest at the pin; the only
falsely-active finding was N-142 (filed on a stale grep by the S12 lane over
a tree that already contained the N-066 fix).
