# s10-reverification-sweep.md — the session-10 board-truth pass (every active finding re-verified against HEAD `84a1832`)

> **Purpose.** The user's session-10 directive: "remove the fixed things."
> Every finding that was active at the session-9 close (76: 7 SEVERE /
> 28 MAJOR / 35 MINOR / 5 INFO, + 1 tracked-open) was re-verified against
> the current tree by four parallel verification agents, each reading the
> finding file + the current source first-hand; the lead re-verified every
> load-bearing verdict (the N-124 fix text, the M-100 engine half, the
> cr_scan.h false comment) before any board mutation. Landings in scope of
> the delta: `095a329` + `dd225da` (session-list empty-vs-missing),
> `9e5dedc` (M-081 carve), `b92ebef` (S-041 school-1 carve), `a972d29`
> (header doc-comments). Nothing else touched `src/`, `include/`, or
> `tests/` between the findings' filing bases (`35adcb9` / `ff879f9`) and
> HEAD.
>
> **Scope pin (this session's prompt):** two repositories only — core +
> corpus. The six driver-repo findings (M-086, M-087, M-089, N-107, N-127,
> and M-088's driver half) were NOT re-verified this session (no
> rda-driver reads per the prompt's scope pin); they stand as filed at
> driver `323e75a`. M-088's CORE-side half WAS re-verified (workflows +
> docs live in this repo) — STILL-ACTIVE, nothing moved.


## Push-time reconciliation (the moving-target SOP executing)

While this sweep ran, four concurrent fix lanes landed: `26da616` (N-094),
`f6e303e` (S-042), `dd0ffd5`/`ce2405a` (M-071), `9daa787`+`b500058`
(M-090). My STILL-ACTIVE verdicts for those four stand as the record at
`84a1832` (every one was re-verified first-hand at that tree — the fix
lanes' own RED-test reproductions corroborate the verdicts) and are
SUPERSEDED at the rebase tip. **Board at the rebase tip: 70 active
(6 SEVERE · 26 MAJOR · 33 MINOR · 5 INFO) / 255 FIXED-files / 2
tracked-open (OQ-S4-3, OQ-S8-1).** The N-094 conflict resolution merged
this sweep's drift record into the FIXED file's addendum (the :14
"9 verbs" headline residual is flagged there for the next doc pass).
This evidence file is NOT retro-edited: the verdict table below is the
truth at `84a1832`, the audited commit of this sweep.

## Board mutations performed this sweep

| Finding | Action | Evidence |
|---|---|---|
| **N-124** | **→ FIXED/** (the only fully-fixed active) | transport.md:133-136 carries the SUPERSEDED annotation (landed `2025554`); code re-verified cr_transport.cpp:563-573 + D-DRV-1/D-DRV-2. Residual: driver-install.md same-class sweep NOT done (follow-up note, driver lane's next pass). |
| **M-100** | Re-scoped (addendum) | The finding's "grep: absent" core-ref was FALSE AT FILING — `6d0f16c` (M-066) landed the engine enumeration pre-filing (cr_preview.h:114-149 ArchiveEntry + cr_preview.cpp:696-737 CD walk). Residual = the ABI/CLI exposure only (no cr_ui_archive_list, no entries field in cr_ui_metadata, no CLI verb). Stays MAJOR/active, re-scoped. |
| **M-079** | Addendum | NEW false comment at cr_scan.h:236-239 ("the CLI/win layer reads it from the volume entry" — no code does; cr_enumerate.cpp:245 fills the listing row only). |
| **N-094** | Addendum | The verb-count drift COMPOUNDED: docs/CORE-API-CURRENT.md now carries three contradictory counts (9 at :14 / 10 at :319 / 11 at :37) + the stale dispatch cite. |
| **N-102** | Addendum | A THIRD uncorrected "37 toggles" instance at include/cr/carve/cr_carve.h:88 (a972d29 fixed cr_rawsigns.h + cr_signatures.h but missed this one); + this finding's own cited filename (M-024-rawconfig-toggles-absent.md) never existed — the real file is M-024-no-runtime-format-filter-surface-vs-og-rawconfig-ini.md. |
| **N-099** | Addendum | Erratum E51 still carries the wrong MDB ctor cite ":21316-21318" (real: :16539-41) — the E63-for-WPG correction never propagated to E51. |

## Verdict table (all 76 actives at session open)

### SEVERE (7)

| ID | Verdict | Current evidence (HEAD 84a1832) |
|---|---|---|
| S-040 | STILL-ACTIVE | 11 verbs, no `image` verb (rrecov.cpp:3039-3060); cr_ui.h 81 exports, reader-only; GENERIC_READ at cr_image_win.cpp:37; R-93 stands. |
| S-041 | STILL-ACTIVE (scoped rollup current) | School 1 LANDED (b92ebef): FMT_CHM/ONE/MNY/EMF/SWF/SWF_CWS/3DS/3DS_NCSD at cr_signatures.h:179-188, validators cr_analysers.cpp:3742-3920, rows cr_registry.cpp:933-990, tests :2980/:3043. School 2/3 ALL ABSENT (grep FMT_HTML/XML/SVG/LZH/TS/M2TS/MTS/MOBI/JP2/VCF/GPX/HIK/OSP/POS/AEP/CHFP/ACCDB + needles BOOKMOBI/RIFX/xpacket/0x6a502020/BEGIN:VCARD → 0 hits). The finding's own SCAN-4 record + addendum #2 child map are CURRENT — remaining scope = school 2/3 exactly as stated. |
| S-042 | STILL-ACTIVE | validate_wmf_placeable at cr_analysers.cpp:3436-3464: invented inch gate :3443-3445, checksum u16@+18 vs XOR(words 0..8) :3450-3455, bounded extent :3460; no hmf/reserved/Size*2+22 gates. addendum re-pin :3444 exact. |
| S-043 | STILL-ACTIVE | Zero orientation/rotate/flip in cr_thumbnail.cpp (all 1,061 lines); orientation is metadata-string-only (cr_preview.cpp:355, cr_ui.h:1184). |
| S-044 | STILL-ACTIVE | cr_ui.cpp:2017 + rrecov.cpp:1712 — the only gate is `autosave_dir[0] != 0`; no results-found gate, no already-saved gate; the double-save falsification visible in rrecov.cpp source order (:1703 explicit save, :1712 autosave, no interlock). |
| S-045 | STILL-ACTIVE | thumbnail_format_supported: jpeg/png/bmp/gif/tiff/psd/pnm/hdr only (cr_thumbnail.cpp:83-86); webp/heic sniffed-and-named refusals :201-228/:1040-1044; tests pin refusals (test_thumbnail.cpp:124-128). No libheif/libwebp/LibRaw/pdfium vendoring. |
| S-047 | STILL-ACTIVE | No TS row/validator/sync-scanner (FMT_MPEGPS :384-393 / FMT_MPEGES :394-403 only; 0x47-sync grep → 0 hits). |

### MAJOR (28 → 27 active after N-124's class-mate stayed; count unchanged here)

| ID | Verdict | Current evidence |
|---|---|---|
| M-071 | STILL-ACTIVE | NODE_FLAG_JUNK_FILTERED: 3 hits in 2 files (query gate cr_query.cpp:119; setter cr_filterfolder.cpp:107,110) — rrecov.cpp ZERO (print_tree :819-860, recover-all :1961-1986 unfiltered; SafeWriter::enqueue cr_safewriter.cpp:102-106 none). Stale comment cr_scan.cpp:1196-1201 stands. |
| M-072 | STILL-ACTIVE | path_substring/state_mask grep → 0; deleted_only still bool (cr_query.h:73); prefix-only (cr_query.cpp:160). |
| M-073 | STILL-ACTIVE | ScanConfig (cr_scan.h:185-327) field-by-field: no caller folder-filter input. |
| M-074 | STILL-ACTIVE | cr_destio.cpp:21-24 collapse stands; cr_error.h 0x5005 family unchanged; test_destio_win.cpp:220 pins UNC → plain UNWRITABLE. (cr_transport.cpp:1040-1042 PathIsNetworkPathW = the N-013 overlap-arming path, NOT destination classification — do not misread as fixed.) |
| M-075 | STILL-ACTIVE | In-engine checker only; the only CreateProcess in src/ is the BitLocker unlock (cr_bitlocker.cpp:62); D-90/R-65 unchanged. |
| M-076 | STILL-ACTIVE | cr_ui_disk_info (cr_ui.h:444-455) no TRIM; health-facet-only stands (cr_ui.h:1299); no `list --trim`. |
| M-077 | STILL-ACTIVE | Caps {4 MiB, 4M units, 256} unchanged (cr_preview.h:108-112); no 100 MiB eligibility / 15 s cap anywhere. |
| M-078 | STILL-ACTIVE | turn_off/TurnOff grep → 0; command builder unlock-only (cr_bitlocker.cpp:338). |
| M-079 | STILL-ACTIVE (+addendum: new false comment cr_scan.h:236-239) | No fill site (5 grep hits: decl, ABI copy, 2 naming reads, record copy); fill_identity_card :1847-1862 fills not-the-label. |
| M-080 | STILL-ACTIVE | max_pixels 64MP / max_read 4 MiB (cr_thumbnail.h:61-62); EXIF fast path fires only >max_pixels (:951-969); test anchor mis-pin stands (test_thumbnail.cpp:131-138). |
| M-082 | STILL-ACTIVE | reghive FVE-only (cr_reghive.h:102-108); IsBootVolume/PartitionBootInfo/EASEUS_HMG → 0; cr_bootfix.cpp caller-path (:126). mom-capabilities.md:154-160 addendum acknowledges, no disposition. |
| M-083 | STILL-ACTIVE | Structural: no consumer surface exists → the promotion cannot fire; grade grep over thumbnail path → 0. |
| M-084 | STILL-ACTIVE | validate_mdb :3543-3557 — "Standard Jet DB\0" only; ACE DB/accdb grep → 0. |
| M-085 | STILL-ACTIVE | uncompress_strip {1,8,32946,32773} (cr_thumbnail.cpp:658-699); LZW(5) refused :698. |
| M-086 | NOT RE-VERIFIED (driver-repo; this session's two-repo scope pin) | Stands as filed at driver 323e75a. |
| M-087 | NOT RE-VERIFIED (driver-repo) | Stands as filed at driver 323e75a. |
| M-088 | STILL-ACTIVE (core-side half) | Pin split stands (release.yml:291 v0.2.4 vs installer.yml:118 v0.2.3); CI-CURRENT.md:59-61 "ONE pin source" + :145; 3-lane docs; PREBUILT-ARTIFACTS.md deleted-action/workflow cites (:36-37,54-59,63-64,73-74,81-82,127-128,148-153); README:125-132; RELEASE-NOTES:5-12; DRIVER-RELEASE:54,86,105-106,139,210; DRIVER-CONTRACT:26; CI-LOCAL:65-67; CI-SCENARIOS:16,25-27. Only docs/PACKAGING.md:18 is correct. |
| M-089 | NOT RE-VERIFIED (driver-repo) | Stands as filed at driver 323e75a. |
| M-090 | STILL-ACTIVE | validate_dmg :2996-3031 — bzip2 EOS walk :3006, EOS+10 sizing :3016; koly grep → 0; FMT_DMG cap 0xBEBC200 at cr_registry.cpp:718 (row :712-721; the :709-711 comment still asserts the refuted mirror claim). |
| M-091 | STILL-ACTIVE | 64 MiB stand-in at cr_analysers.cpp:3553; "undecoded" comment :3541-3542; row faithful (cr_registry.cpp:884-893). |
| M-092 | STILL-ACTIVE | FMT_HTML/u16-keys/DOCTYPE needles → 0. |
| M-093 | STILL-ACTIVE | FMT_XML/FMT_SVG/<?xm needles → 0. |
| M-096 | STILL-ACTIVE | FMT_MOBI/BOOKMOBI → 0. |
| M-098 | STILL-ACTIVE | FMT_HIK/private_stream → 0. |
| M-099 | STILL-ACTIVE | FMT_AEP/RIFX/xpacket → 0. |
| M-100 | STILL-ACTIVE, RE-SCOPED (addendum) | Engine half EXISTS (M-066/6d0f16c); residual = ABI/CLI exposure (cr_ui.h 81 exports, no archive-list; no verb; cr_ui_metadata :1180-1190 no entries field). |
| M-101 | (not in sweep batches — verified via S-041 school-2/3 absence + N-113) STILL-ACTIVE | No Versions grouping / Tags Extracted branch (grep → 0; the NTFS walk dedups multi-version siblings). |
| M-102 | STILL-ACTIVE | cr_ui.h zero thumbnail symbols; 81 exports (doc sites consistent). |

### MINOR (35 → 34 active: N-124 moved to FIXED)

| ID | Verdict | Current evidence |
|---|---|---|
| N-090 | STILL-ACTIVE | Plain *.crsf no auto_ filter (cr_ui.cpp:391-441 Win / :459-463 POSIX); label[32]; save_unix u64; no device-type field (cr_ui.h:881-892). |
| N-091 | (not re-verified in batches — doc-registration finding; PARITY-DELTAS registration state unchanged per N-096/N-125 greps) STILL-ACTIVE | Load-progress formula delta unregistered; autosave divergence registered only as interpretation. |
| N-092 | STILL-ACTIVE | No WMF/EMF/ICO branch in sniff() (:201-228 → Unknown :227); no cache (grep → 0). |
| N-093 | STILL-ACTIVE | MID :705 {14, 64<<20}; GZ :802 {18, 512<<20}; PSP :782-788 {32, 128<<20}; SIT :792-798 {16, 512<<20} — all four unchanged. |
| N-094 | STILL-ACTIVE (+addendum: triple count drift) | -h block :2997-3034 lists 10; dispatch :3039-3060 has 11 (sessions :3043). |
| N-095 | STILL-ACTIVE | All five items verbatim-stale (REFERENCE-DRIVER-BEHAVIOR:91-92,:329; PARITY-CORE-REC:125; PARITY-ARCH/UI 1,489; carve.md:546-548; transport.md:6; ABI-CONTRACT:479,:519). |
| N-096 | STILL-ACTIVE | Constants defined (cr_rda.h:50-52), zero issuance; DRIVER-UTILIZATION:64 MATCH verdict stands; no PARITY-DELTAS row. |
| N-097 | STILL-ACTIVE | PARITY-OG-CORE-INVENTORY:55 auto.txt row verbatim; :30 lumps it OOS-UI. |
| N-098 | STILL-ACTIVE | No always-fresh sentence (cr_ui.h:467-471; ABI-CONTRACT greps); behavior still always-fresh (cr_ui.cpp:1649-1653). |
| N-099 | STILL-ACTIVE (+addendum: E51 cite) | FIXED/S-007:215-223 verbatim-stale. |
| N-100 | STILL-ACTIVE | cr_node.h:216 only carrier; :210-212 rationale stands; nflags stops at bit-8 (cr_records.cpp:173,:260-268); comment rewrite not applied. |
| N-101 | PARTIALLY-FIXED (~90% absorbed; stays active for residuals) | Fixed: §8 counts, EuShare row, EFIBoot note, Eaolog/BuildRaw/DelFat rows, GetDriverInfo row, session-9 header rewalk (24e93ad, ea59128, 167dbe3). Residual: :16 header literal "78/204" (annotated below), §5 :100 cell "113/114; libvmdk ×1" (superseded by §8/§9, unedited). |
| N-102 | PARTIALLY-FIXED (+addendum) | Src halves fixed (a972d29); FIXED/M-024:6,15 stands; NEW cr_carve.h:88 instance. |
| N-103 | STILL-ACTIVE | All four doc sites byte-identical since ff879f9 (REFERENCE-API-GAP:222,:376; PLAN-API-EXPANSION:81; PARITY-OG-CORE-INVENTORY:90 two-binary misattribution). |
| N-104 | STILL-ACTIVE | ldm.md:1 dangling table.md parenthetical; CAPABILITY-MATRIX:43 HFS+ row stale vs cr_hfsplus_engine.cpp:514+. |
| N-105 | STILL-ACTIVE | auto_session_name label+timestamp only (cr_crsf.cpp:363-404); no folder-scan flag exists. |
| N-106 | PARTIALLY-FIXED (core deliverable landed) | SURFACE-INVENTORY:86 row exists (24e93ad); CROSS-REF:553. Over-claim note: "sixth-source note added to N-071's row" — no such note in N-071's file (recorded, not re-opened). |
| N-107 | NOT RE-VERIFIED (driver-repo) | Stands as filed at driver 323e75a. |
| N-108 | PARTIALLY-FIXED (headline stands) | docs/PARAMETERS.md:78-79 rows still wrong by 2-4x (WINDOW 1 MiB/OVERLAP 4096 at cr_carve.cpp:35-36; usersigns 48B/256/12/1024/80 at cr_usersigns.h:40-47); trail annotation + rawsigns comment fixed (7922b5e, a972d29). |
| N-111 | STILL-ACTIVE | jP-family/0x6a502020/jp2c → 0. |
| N-112 | STILL-ACTIVE | -lh method gates → 0; (E4 52 5C 7B now correctly binds FMT_ONE per b92ebef — corroborates). |
| N-113 | STILL-ACTIVE | All five text-school probes → 0. |
| N-115 | STILL-ACTIVE | office branch first-entry-name gate only (cr_analysers.cpp:846-856); no rels split/ODF/EPUB/iWork/HWP/CDR/metadata (grep → 0). |
| N-116 | STILL-ACTIVE | Byte-sort by full path (cr_ui.cpp:446 Win/:468 POSIX); CLI re-sorts by path (rrecov.cpp:960); save_unix carried (:2818) but never orders. |
| N-117 | STILL-ACTIVE | PLAN-API-EXPANSION:40 W1b shape; ABI-CONTRACT:479,:519 cites (real: :853-895/:897-916). |
| N-118 | STILL-ACTIVE | R-52/R-54/R-56/R-31 rows stale (REFERENCE-API-GAP:225,:261-265,:365-404); R-37 now doubly stale post-M-066 (folded to the addendum class). |
| N-119 | STILL-ACTIVE | D-88 (PARITY-DELTAS:2915-2934) unchanged; trigger fired, never re-adjudicated. |
| N-120 | PARTIALLY-FIXED | EuShare + corpus-pin fixed; I-010:10 D-26 over-claim + FileRepair wrong-register pointer (SURFACE-INVENTORY:98 → CAPABILITY-MATRIX §3 which registers only Cloud/NAS/remote/BitLocker-derivation/signing/.rsf-import) stand. |
| N-121 | STILL-ACTIVE | No ESP/GPT discovery (cr_bootfix.cpp:174-199 caller path; rrecov.cpp:2471-2472 flag-required); no D-90 row. |
| N-122 | STILL-ACTIVE | Patch-only nvram (cr_bootfix.cpp:279-294; cr_nvramfix.cpp:239-242 stop-on-missing pinned by test_nvramfix.cpp:417/:435); no D-90 create/delete row. |
| N-123 | STILL-ACTIVE | cli.md:16-17 "exactly 7 commands" stands; exit 4 missing from the module's exit-code row. |
| N-124 | **FIXED → moved** | See board mutations above. |
| N-125 | STILL-ACTIVE | R-16 unanchored (REFERENCE-API-GAP:205,:365); no PARITY-DELTAS row; no engine classification. |
| N-126 | STILL-ACTIVE | Category enum (cr_query.h:34-44) has no Unsaved; grep → 0. |
| N-127 | NOT RE-VERIFIED (driver-repo) | Stands as filed at driver 323e75a. |

### INFORMATIONAL (5 — all verified as still-accurate records)

| ID | Verdict | Notes |
|---|---|---|
| I-026 | Record stands | DRIVER-RELEASE §7 (:178-185) posture unchanged. Drift: the finding's mechanism cite (.github/actions/install-driver/action.yml:44-64) now dangles (the composite action was deleted — M-088 cluster). |
| I-027 | Record stands | Register intact; optional fold-ins not landed (by design — optional). |
| I-028 | Record stands | All five superset behaviors verified in code. Cite drift: the header is include/cr/preview/cr_thumbnail.h (the finding's src/-prefixed path never existed). |
| I-029 | Record stands | DRIVER-CONTRACT:131-153 + cr_rda.h:119-142 unchanged. Cite defect: the finding's `include/cr/win/rda_ioctl.h:224-225` names a file that never existed in core — the constants live in cr_rda.h (the :224-225 matches the DRIVER repo's file). |
| I-030 | Record stands | modules/ LS at HEAD (28 files): no table.md, no reghive.md, no apply.md; ldm.md:1 dangling parenthetical (same as N-104). |

## Totals after the sweep

Session open: 76 active (7 SEVERE / 28 MAJOR / 35 MINOR / 5 INFO) + 1
tracked-open. N-124 (MINOR) moved to FIXED → **75 active (7 SEVERE /
28 MAJOR / 34 MINOR / 5 INFO) / 251 FIXED / 1 tracked-open** (OQ-S4-3,
execution-bound, unchanged). 6 findings not re-verified (driver-repo
scope pin: M-086, M-087, M-089, N-107, N-127, M-088-driver-half) — they
stand as filed at driver 323e75a; the core-side halves of M-088 WERE
verified.

## Line-number re-pins recorded (for the next fixing session)

The carve findings' cr_analysers.cpp cites drifted ~+55-60 lines from the
b92ebef insertions (validate_mdb :3484→:3543; validate_wmf_placeable
:3383→:3436 — S-042's own addendum re-pin :3444 is exact); cr_registry.cpp
(FMT_MID :691→:705, FMT_GZ :781→:802, FMT_PSP :766→:782-788, FMT_SIT
:776→:792-798, FMT_DMG :695-707→:712-721, MPEG rows :370-389→:384-403);
cr_ui.cpp (autosave leg :1999-2019→:2009-2037; list sort :428/:450→:446/:468);
rrecov.cpp (autosave :1700-1718→:1708-1729; CLI sort :948→:960; -h block
:2985-3022→:2997-3034; dispatch :3027-3048→:3039-3060).
