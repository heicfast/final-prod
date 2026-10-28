# CROSS-REF-MATRIX.md — module × parameter verdict matrix (the S46 MINOR-first fix lane on top; the S12 session-2 deep-dive lane below: sector math / scan-progress math / local database / state management / detection-calculation; the prior lanes below)

**Session-46 S46 lane rows (2026-10-29 — the user's MINOR-first fix directive; opened at core `659f0f6c` → `fa654673` claim, corpus `be0cfe9`; the parallel S45 lane holds the ACUTE/SEVERE side per the natural division):**

| Parameter (module · surface) | Verdict | Evidence / finding |
|---|---|---|
| carve · the OfficeTagFromXML five-branch typing tail (P1/P11/P19/P25/P65) | **MATCH** | **N-158 FIXED** (session-46 MINOR-first lane) — the five typed ids land (XMIND 0x86 / MMAP 0x87 / EDDX 0x8c / WFP 0x8d / DRP 0x91): the XMIND content probe decoded from disassembly (the 300-byte RAW window at record+30+strlen(name), the two needles — the 16-byte `xmlns="urn:xmind` + the 14-byte .data constant at RVA 0x93980 — disasm 0x180045110-0x180045666) + the four entry-name compares; the chain walk's NO-stop/last-match-wins semantics decoded and pinned (:18411-18450 — the N-115-era "map stop" comment corrected); the [Filter] coupling as the conservative row-level disabled_formats gate (PARITY-DELTAS D-134); 6 tests, 3 groups RED pre-fix; suite 1209 green — the Resolution in findings/FIXED/ carries the full record |
| writer+report · the failed-files report's tree-classification axis (P13/P11/P46/P68) | **MATCH** | **N-159 FIXED** (session-46 MINOR-first lane) — the og's recovery-log truth decoded and landed: the per-item `tree_class` axis (the six-valued root-role classification, DRWUI :642755-642822 re-verified first-hand with the three field getters pinned) + the `failed_bytes` totals split (the +0x268 accumulator, the enqueue-refusal exclusion documented as schema truth rule 5) + the three register repairs (R-58's "text log"→HTML report; API-DELTA row 21's "NOT IN CORPUS" refuted; D13's addendum) + additive-within-v1 per the schema doc's own contract; 4 tests RED-first; suite 1243 green |

**Session-12 S12 lane changes (2026-10-29 — the session-2 continuation audit per the user's second-session directive: remove-the-fixed + go-deeper with special focus on sector numbers, scanning, accurate detection, calculation/mathematics, no-stuck-or-freeze, the local database, state management; opened on core `869da779` (rebased to the window's closes), corpus `be0cfe9`; five deep-dive lanes' context sheets + three liveness batteries at `evidence/s12-lane-sheets/` + `evidence/s12-liveness/`; the findings push `f85def0f`):**

- **The remove-the-fixed liveness battery (directive 1):** all 49 then-active findings
  checked against the pinned tree — 46 STILL-ACTIVE, 2 PARTIAL (S-050 leg-2 done by its
  own addendum; N-135's items fixed with a doc-residual), **0 phantom-fixed**. The
  parallel windows' subsequent retirements (S-048, M-103, M-112-CSL, M-098, N-146,
  N-156, N-157, the whole I-026..I-034 INFO board) were absorbed at the renumber
  reconciliation — convergent with this battery's pins.
- **The renumber record (earlier-push-wins over three hot windows):** this lane's drafts
  collided with the parallel S42/S42-B/S42-C lanes' consumed seats — M-118..M-122
  (MountPoints2 / partition-boundary / DBX / Stbio-large-IDAT chains), N-154..N-160,
  I-037..I-039 — renumbered **my M +5 (→M-123..M-132), my N +7 (→N-161..N-178),
  I-037..039 → I-041..I-043** (single-pass simultaneous map, no cascade; the script's
  convention recorded at `worklog.md`'s S12 entry).
- **The 39 filings, by module × parameter (all first-hand verified — the lead re-read
  every load-bearing cite before filing):**
  - **ACUTE (2):** **A-013** (cli/session P32/P34/P35 — the `--resume`-without-`--out`
    `"wb"` truncate-before-read: the printed resume command destroys the session);
    **A-014 FIXED** (fat P9/P2 — `fat_datetime_to_unix_seconds` validated month/day only;
    hour≥24/min≥60/2s≥30 fabricated shifted plausible timestamps where the og zeroes
    the slot, FAT:32135-54 ⇔ cr_fat_wire.h — the time-word gate landed by the S45
    lane (862d79ef) with the S51 severe-first lane's convergence addendum (the
    per-slot zeroing pin + the 0x65C4 example-date erratum), findings/FIXED/A-014,
    MATCH).
  - **SEVERE (3):** **S-055** (carve P28/P1/P3 — FIXED session-51 → MATCH: built-in-first
    restored, the user arm fires only when no built-in survived the filter, the
    no-reclaim rule pinned, `findings/FIXED/S-055-*`); **S-056** (carve P16/P28 — FIXED
    session-51 → MATCH: match_at's disabled_exts predicate carries the removal-loop
    semantics, `findings/FIXED/S-056-*`); **S-057** (session/orchestration P36/P31/P32 — the og's stop path runs its
    own organize into the shared completion handler whose auto-save gates carry no
    stopped-check (DRWUI:1549399-21 → :339076-79 → :612572-87); the core never
    auto-saves a cancelled run, and its comment mis-states the og).
  - **MAJOR (10):** **M-123 FIXED** (S47 MAJOR-first fix lane, `10620f04` —
    findings/FIXED/M-123: FUN_1800078d0's full gate set landed in load_gpt_side
    [revision 0x10000, header_size==92, reserved==0, the mirrored position pair,
    usable-range bounds, entries outside the usable range — structural gates
    before the CRC, og order]; N-164's deliberate strictness kept; 9 tests,
    8 RED pre-fix; P1/P20 → MATCH); **M-124 FIXED** (S47 MAJOR-first fix lane, `dcb123dd` — findings/FIXED/M-124:
    the EBR link set {05,0F,**85**} — `case 0x85:` joined the Extended arm of
    mbr_type_to_kind per Partition_decompiled.c:1006; whole 0x85 chains walk,
    the bogus Other-volume at the link target gone; 3 RED-then-green tests,
    suite 1164/1164; P1/P26 → MATCH); **M-132 FIXED** (S47 MAJOR-first fix lane, `d746a75b` — findings/FIXED/M-132:
    the backup probe at device-last FIRST [FUN_180006e20:4567's fixed
    position], the declared field a registered fallback; the dead-LBA1
    recovery kept + registered as PARITY-DELTAS D-124b; P1/P35 → MATCH);
    **M-125 FIXED (S42 fix
    lane — findings/FIXED/M-125: the query-layer helper family
    `progress_display_remap/_percent/_publish` (FUN_1401af820/:950/:a60 verbatim,
    the constants re-verified against the corpus — the finding's test table
    carried 4 transcription errors, corrected per the corpus-wins rule); the
    engine's linear permille untouched + the regression pin; O38 → MATCH;
    PARITY-DELTAS D-118)** (orchestration P38 — the og's piecewise progress
    remap + the defensive 99.99 cap, landed as the optional display layer); **M-126** (orchestration P39 — the og's rate-EMA ETA model
    decoded; no core ETA surface exists; O39 premise superseded); **M-127**
    (fat/exfat P47-50 — the O(n²) visited-vector cycle guard vs the og's range tree;
    100×maximal-chain FAT32 ≈ 100+ s stall-class CPU); **M-128** (carve P1/P19 —
    og sign matching is UPPERCASE-hex text memcmp; 12 og-dead lowercase signs live in
    the core, unregistered); **M-129** (carve P23-adjacent — the og's tolerant
    EUUserData loader (skip-and-continue) vs the core's all-or-nothing reject);
    **M-130** (fat P1/P3/P26 — the dentry shape-validator family (ext-tail, size/chain
    XOR, dir, >9-repeat histogram gates + walk-stop semantics) unported); **M-131 FIXED-ALREADY
    (4f0fc160, the parallel A-013 landing — verified at c98eb7a7: the CliCkSink
    tmp+commit+rename reproduction; the sink never opens the final path for
    truncation; CrUi.CliResumeSinkNeverTruncatesTheSessionSource green;
    findings/FIXED/M-131)** (cli/session P34 — the in-run checkpoint's
    truncate-in-place shape, the N-014 class at the CLI layer — closed by the
    A-013 landing); **M-132** (table P1/P35 — GPT backup probe:
    og device-last always + full bail on dead LBA1; core probes declared backup_lba +
    recovers from dev-last — divergent both directions, unregistered).
  - **M-128/M-129 FIXED** (S47 MAJOR-first fix lane, `2b0184f3`/`55b2fe32` —
    findings/FIXED/: the user-sign hex-case superset REGISTERED as D-125
    [the og's %02X-only text compare leaves 11 of its own shipped signs
    dead; the core's byte-decode keeps all 190 live — no code change, 3
    pins, the census corrected 12→11 first-hand] + the EUUserData loader's
    og-tolerance semantics [unknown members skipped, _stricmp dispatch,
    malformed signs skip-and-continue, over-cap still atomic per S-039,
    D-126 registered, the verbatim corpus fixture ships as a test];
    P1/P3 detection + P23 error-semantics → MATCH);
  - **MINOR (18):** **N-161** (table — GPT entry span underflow, no first≤last gate);
  - **MINOR (18):** **N-161 FIXED (Session-44 minors lane — `emit_gpt_entries` first≤last gate
    + `DiskTable`/`ScanStats::gpt_invalid_entries` counter; RED→GREEN proven; findings/FIXED/N-161)**
    (table — GPT entry span underflow, no first≤last gate);
    **N-162** (fat/registry — the og's 32-bit `(c−2)×spc` wrap ×4 sites, core u64
    correct, unregistered D-56/D-68 member); **N-163** (ntfs — runlist
    truncate-and-keep vs core reject-whole-fragment; sparse-first superset);
    **N-164** (table/registry — the deliberate GPT strictness unregistered);
    **N-165** (fat — the "File overflows partition!" scan-time refusal vs the core's
    deferred per-read failure, unregistered stance); **N-166** (orchestration — live
    counters stage-boundary-only vs the og's ~2 s cadence; orchestration.md:210
    inaccurate); **N-167** (orchestration/query — no found-size aggregate, the og's
    "Files Found: N (X GB)" headline unrenderable); **N-168** (carve — the og's 2×
    bounds guard, core correct, unregistered); **N-169** (carve — duplicate `asf`
    block merge semantics, og side undecoded OQ); **N-170** (docs — 8 stale
    pre-erratum EUUserData/GetRawUserInfoMap claims + 2 corpus-side residues);
    **N-171** (carve/stats — user-sign files never tick carved_found, runtime +
    session undercount); **N-172** (fat — deleted-first-char `'#'` vs `'_'` + the
    unregistered LFN-checksum recovery superset; closes OQ-T2); **N-173** (fat —
    the 2106/2107 u32 clamp vs full conversion; exFAT sibling inconsistent);
    ~~**N-174**~~ **FIXED (S46)** (exfat — the `._` AppleDouble sink filter landed, the N-028 family's
    exFAT twin); ~~**N-175**~~ **FIXED (S46)** (fat — NTRes case folding landed); **N-176** (fat — the
    LFN illegal-char 8.3 fallback absent); **N-177** (ext4 — the size-exceeds-blocks
    damaged flag (FLAGS 0x200) not carried); **N-178** (cli/orchestration — no CLI
    pause verb + no engine pause notification, the landed ABI pair's missing legs).
  - **INFORMATIONAL (6):** **I-035** (S-050's CHK-chain decode addendum: rename
    formats, hex-prefix adapter registry, 36-KiB buffer, tif exclusion);
    **I-036** (the default-on user-sign posture is CLI-app-level only — D-104's
    "engine init" phrasing imprecise); **I-041** (stopped-vs-finished: the og's
    <98% heuristic vs the core's truthful stage mask — deliberate, register);
    **I-042** (paused-state observability: four og surfaces vs telemetry-only);
    **I-043** (state ownership: the og's unlocked master enum vs single-owner +
    mutexed telemetry — deliberate); **I-040** (og-defect non-adoption batch #2:
    ext4 i_size_high pollution, `_gmtime32_s` post-2038 — register beside D-58/D-59).
- **Verified-MATCH highlights recorded for the lanes (no finding, evidence in the
  sheets):** the LDM span/stripe/RAID-5 math clean post-S-052 (algebraically identical
  to `FUN_180086a90`); EBR base/rel chain math; NTFS runlist nibble/sign-extension/LCN
  accumulation; exFAT 64-bit cluster math; ZFS DVA `<<9`; the FAT/exFAT/ReFS/Btrfs
  grading bytes; the og's 14-state scan enum + pause machinery (the core's
  PausableToken = verified RE-parity); auto-save gates/naming; the cancel-keeps-results
  flow; no hang-input asymmetry beyond the registered D-14/D-18/N-043 classes (7-input
  hang trace table in lane B's sheet).
- **Board after this push (filesystem truth): 74 active — 2 ACUTE · 5 SEVERE ·
  28 MAJOR · 33 MINOR · 6 INFO.** Next free IDs: S-058+, M-133+, N-179+, I-044+, A-015+.

**Session-42-C S42-C lane changes (2026-10-29 — the THIRD continuation lane of the same user mandate, opened on the same pin `869da779`/`be0cfe9` before any window landed; rebranded twice at the reconciliations [the remote's S42 and S42-B lanes pushed first]; evidence: evidence/s42c-*.md):**
- **The three-window reconciliation (W1.5, executed twice):** the lane's drafts
  collided at every seat — **M-122** (the Stbio large-IDAT retroactive filing:
  draft M-112 [the ACTIVE CSL owns it] → M-118 [the remote S12's MountPoints2]
  → M-121 [the remote DBX chain M-117→M-118→M-121] → M-122), **I-038** (the
  diskhealth companion-tree register: draft I-035 → I-037, both consumed by the
  remote lanes' registers), **I-039** (the corpus-worklog register: draft
  I-036 → I-038, displaced seat-by-seat), and the red-main draft folded as the
  **N-157 corroboration addendum** (this lane's unique second-method
  reproduction: a clean `git worktree` build of `67de3276` runs
  `Carve.OoxmlRelsPathSplit` FAILED, pinning the N-115 Resolution's 0-fail
  claim false at the pushed tree).
- **preview row, retroactive cell:** **M-122** (B1/P47/P48 cells → MATCH at the
  landed code; the phantom-SHA ledger rows 181-182 repaired — the
  phantom `5103ecd8` → `76e2d351`, STILL standing after all three windows,
  repaired by this lane; the cr_thumbnail.cpp:163 `N-112`-cite comment verified still
  standing → the one live code-side residual, carried in its fix plan).
- **health row, register cell:** **I-038** (the diskhealth companion tree — 818
  files walked for the first time: 13 COVERED / 12 NOT-CORE-SCOPE / the 5
  never-dispositioned transport surfaces [6 vendor-NVMe CMD_TYPE routes, the
  USB-bridge quirk table, the AAM/APM setters, the Winmgmt mutation, the
  diskHealthIndex= contract] registered as D-82/I-027 extensions; the health
  module verified CLEAN — zero code changes proposed; M-076's retirement by the
  window leaves the module with NO active divergence; ACKNOWLEDGED-RETIRED).
- **cross-cutting row, register cell:** **I-039** (the corpus worklog
  cross-check — the first lane to read it: ~45 claimed surfaces verified
  dispositioned, zero contradictions; the FoxitRestore / NAS-LM-hash /
  build-identity datums registered; the Languages-count + retention-nag errata
  → CORPUS-ERRATA Session-42-C rows; the RawFixer 31-vs-33 docs drift queued;
  the core-side citation census: 232 files, zero behavior gaps;
  ACKNOWLEDGED-RETIRED).
- **Bookkeeping cells (the repairs that survived all three windows, executed by
  this lane):** the ledger's phantom-SHA rows 181-182 + the N-032/N-112/N-113
  dangling FIXED paths (all still standing at the third reconciliation); the
  trail-SHA guard's pre-rebase phantoms (`36c1d3e5`/`cd883754`) annotated —
  the guard re-run GREEN (515 tokens resolving; it was RED at two consecutive
  reconciled mains); the M-112-CSL collision note (the three-seat renumber
  recorded).
- **Landing-parity cells (the pin-time sweep, all three windows unaffected):**
  A-012, S-049, S-051+M-104, S-052, S-053+N-141, S-054, M-109+the-Stbio-loop,
  N-096, N-112, N-113, N-115+N-153, N-121, N-122, N-128, N-129, N-130+N-138,
  N-132, N-133, N-137, N-142, N-151 — **all VERIFIED-PARITY at the pin**,
  no code divergence (evidence/s42c-landing-parity.md).
- **Convergence cells:** this lane's 49/49-liveness battery at the pin is
  CONVERGENT with the remote second lane's (397be347: "49/49 STILL at 869da779,
  zero falsely-active" — two independent lanes, one verdict); the 1470-file
  asset census verdict likewise convergent; the asset-family walk + the
  corpus-worklog read + the diskhealth walk + the citation census remain this
  lane's unique product.



**Session-42 S42 continuation-lane changes (2026-10-29 — the user's continuing second-session directive at the post-fix-window board; audited `869da779`, corpus `be0cfe9`):**
the board-truth verification over the hottest fix window (the S12 sweep pre-dated S-049/S-051/S-052/S-053 + the N-wave landings — every at-risk finding re-checked LIVE at the lane tree, three stale-line cites noted, NO unretired fix found; `evidence/s42-board-verification.md`) plus the HFS+ residual resolutions:
- **hfsplus rows, cells updated**: N-038's field-name residual **RESOLVED** (+0x29 = `ownerFlags`, TN1150 byte-exact via the ushort-units decode — `FUN_180009940`'s `short *` record parameter mixes ushort-offset and byte-offset reads; all ten branch reads mapped; `FUN_18000f7b0`'s own FileSize label corroborates) and the S-002 decmpfs-completion residual **CLOSED core-superset** (the og's completion site wires size only — no mode-3/4 content branch anywhere in the SearchDLL; `evidence/decompiled-quotes/s42-hfs-forkfield-resolution.md`).
- **hfsplus row, new cell**: **N-155** (MINOR — the og's zero-logical-size fallback `dataFork.totalBlocks << 12` absent in the core; the deleted/journal-record population is where it bites; B2/D11 cells).
- **decode datum registered**: the og's journal-path record copy (`FUN_18000fe90`) is name-length-misaligned by design (fixed `keyStart+0xC` base) — deliberate non-parity, the D-row class.
- **tests/build-gate row, new cell — FILED AND CONVERGENT-RESOLVED intra-session**: **M-120** (MAJOR: main red — committed conflict markers in test_carve.cpp:8588-8600 from `1fb7603b`, whose message claims "suite 1096/1096 green"; the from-scratch audit-side build reproduced it: cr_core/cr_ui clean, cr_ui_tests 63/63, cr_tests uncompilable; the parallel repair `b00118b5` landed in the same window — both-sides-folded compare — and this lane EXECUTED-VERIFIED the post-repair tree: full suite 1104 PASSED / 85 Win-only SKIPPED / 0 FAILED; Q65/Q68 cells; evidence executions/s42-build-at-head.md).
- **storage/transport rows, addendum cells**: the DefaultLog.txt real-run decode (the three zero-coverage classes: 393 Ldq read-parameter refusals, 4 bad-sector skip lines, 1 recovery-write failure) — **M-119's addendum** (the full validation ladder first-hand at the Ldq entry :180139-180160: three refuse classes + the clamp + the error-path zero-fill; the real-run corroboration that the og's scans over-read routinely) + the bad-sector skip constants REGISTERED (2 MiB/×2-per-3/64 MiB/4 MiB inner/80 MiB message gate vs the core's bisection — outcome-equivalent) + the write-failure log observable registered (evidence s42-defaultlog-ldq-decode.md).
- **matrix hygiene**: the stale OQ-S10-2 lane-B cell annotated superseded (N-138's refutation is the truth row).
# CROSS-REF-MATRIX.md — module × parameter verdict matrix (the S42 continuation lane on top: the focus-battery filings — grading polarity, sparse export, timestamp restoration, ReFS/Btrfs grade, the reconstruction/lost-partition/scan-mode batches; the prior sessions below)

**Session-42 continuation lane changes (2026-10-29 — the user's second-session focus battery: file reconstruction / timestamp / recovery chances / partition recovery + API / preview / scan-mode semantics; audited core `b26017a7` (src+include byte-identical to the S12 live-verification point — the liveness sweep discharged by git-proof + SEVERE spot-probes, evidence/s42-liveness-sweep.md), corpus `be0cfe9`; the W4 executed build+suite at `evidence/executions/s42-build-suite-execution.md`):**

- **grading/query rows, new cells — the recovery-chances battery (4 SEVERE + 3 filed)**: the extension-table gate polarity — **S-058** (SEVERE: the og runs the twin/overlap/deep-validation branch for extensions NOT in the 169-key set — `FUN_1401bebe0` = inequality, control case :585888 calibrates the direction; the core's `if (!in_table) continue` at cr_grading.cpp:275 twin-flags the exact inverse population, pinned by test_grading.cpp:166-187; the s5 evidence's §5 polarity line is the erratum the inversion inherited — evidence/decompiled-quotes/s42-grading-gate-polarity.md); the ReFS/Btrfs insert-time grade — **S-061** (SEVERE: the og grades both High at insert with 5828/5830 gauge ticks, DRWUI:605481-497; the core's early return leaves them Average — cr_grading.cpp:217-219); the twin-key device family — **M-133 — FIXED (session-52 lane — findings/FIXED/M-133: the shift narrowed to {Fat16} exactly per DRWUI :605752-756/:605924-927; the ext2/3 leg not expressible in the core's kind space, registered OQ-S52-1; 6 tests, 3 RED pre-fix; suite 1290/1290)**; the deep-read validation + full-tree re-grade — **M-134** (`FUN_14038c470` decoded: first-sector zero/0xFF → Low, prev-sector tail-16-zero → High; `FUN_1401d7110` re-runs the whole tree; core re-runs solo rules only + no reader in GradeContext; sub-gates tracked as OQ-S42-3); the gauge/QSDE accounting — **N-183** (the 20-format member-set population + the `int((m/t+0.05)*10)` score + the QSDE counter mis-mapping).
- **writer/recovery rows, new cells — the file-reconstruction battery (2 SEVERE + 3 filed)**: the sparse-hole export semantics — **S-059** (SEVERE: the og writes zero chunks for holes and always produces full logical size — :875661-673/:875062-75; the core drops sparse runs at cr_runlist.cpp:190, has no per-extent logical offset (cr_node.h:109-120), left-shifts interior holes and refuses with ERR_TRUNCATED — cr_safewriter.cpp:440-444; the mftwalk comments assert a zero-fill the writer cannot perform); the timestamp/attribute restoration — **S-060** (SEVERE: the og restores DOS attributes + the mask-gated SetFileTime trio after every item — :874686-718, the only SetFileTime in the binary, refuting the N-009d row's premise; the core's DestIo has no metadata surface, zero restoration calls tree-wide); the read-failure semantics — **M-135** (the og truncates + numbers + logs the failure trio and keeps the partial; the core zero-fills-and-continues then deletes failed items wholesale; the six-cause taxonomy); the retry bypass — **M-136** (recovery hands the writer the raw device while the scan's Retry+Cache pipe exists unused — rrecov.cpp:2080-2082/cr_ui.cpp:4027 vs cr_scan.cpp:155-194).
- **lost-partition/orchestration rows, new cells — the partition-recovery battery**: the surfaced-shape batch — **M-137** (the "Lost Partition %d(%s)" template + per-scan counter at FUN_1403b9330/:634817-838, the "Unallocated" relabel :525588-98, the 10-target insertion cap + the per-candidate "Quick" registration :904229-240, the 512-byte fallback :904185-87, the Disk/Basic/Dynamic/MBR/GPT presentation rows, the CreateEnumPartitionObject seam — vs the core's `Candidate @ <LBA>` roots, no cap, always-deep, ERR_BAD_PARAMETER refusal; **M-104's open question 1 resolved by the template decode** — evidence/decompiled-quotes/s42-lostpart-surface-batch.md).
- **orchestration/carve rows, new cells — the scan-mode battery (the "how scanning works" directive)**: the auto-advance state machine DECODED — **I-044** (engine-side, unconditional, result-independent — the C1 row's "UI-layer" rationale corrected; the hardwired settings+0x60/+0x6c; the 4 MiB all-allocated chunk skip; the `FUN_140581500`-vs-`FUN_140580720` citation erratum; ZFS compiled out of the shipped og); the RAW-first start ordering — **N-181** (the og starts CRAWSearchController first, :916484-916509; the core's carve stage is strictly last); the deep-range resolver — **N-182** (`FUN_14035a6d0`'s type-6 sub-range/300 MB/neighbor-extension path; the core reuses the quick slice — D-36's wording qualified).
- **preview row, new cell**: the media metadata families — **N-184** (MP4+WAV only in extract_metadata; AVI/MKV/MOV/FLAC/MP3/M4A unsupported).
- **tests/docs rows, new cells — the executed-verification pass**: **N-185** (main RED at HEAD: the S42 opening commit's OoxmlRelsPathSplit re-pin is self-defeating — the expectation table uppercased, the tail-fold left lowercase; the commit's "1092/1092 green" claim refuted by the executed 1136/1093/1/42) and the doc-truth staleness batch — **N-180** (REFERENCE-API-GAP R-28/R-34 rows, ABI-CONTRACT §7i caps line, API-DELTA S3-W4a, AUDIT-PIPELINE WRITER-1, exfat.md B9 stale text, the s5 polarity erratum).
- **tracked-open +3**: OQ-S42-1 (NTFS $SI/$FN time precedence), OQ-S42-2 (Btrfs nsec), OQ-S42-3 (the grading deep-read sub-gates).

**Session-12 continuation lane changes (2026-10-29 — the second-session continuation deep audit, audited `c7fa527c`, corpus `be0cfe9`):**
the remove-the-fixed liveness sweep (55/55 adjudicated: 41 LIVE, 4 PARTIAL — N-132/N-135/N-138/I-030 residuals recorded in N-143 — 5 driver-repo N/A, 5 register; NO unretired fix found; the from-scratch build+suite EXECUTED green 1117/1117 — evidence/executions/s12-build-execution.md) plus three first-hand-verified new findings and the register/batch set:
- **writer/transport/ui rows, new cells**: the destination-overlap safety parameter — **S-054 FIXED at `a3741812`, CONVERGENT (session-42 independent verification: FIXED-ALREADY)** (was SEVERE: the N-013 multi-extent arming CLOBBERED at both production call sites by the unrelated M-036 commit `1ebd1bcc`; `volume_extents`/`set_destination_spans`/`destination_volume_letter` zero consumers at HEAD; the header contract still claims all-spans; the suite-green masks it; git-proven — evidence/decompiled-quotes/s12-destarming-regression.md). Restored verbatim per the 3b5397a1 shape — any resolvable path armed, ALL extents fed to `set_destination_spans`; the wiring FENCE lands (`DestArming.ProductionWiringCallsSpansApi` + `HeaderContractStillClaimsAllSpans`), fail-before proven by mutation. **Session-42 first-hand re-verification at `d10e4aa2`: the wiring is live at both sites (`rrecov.cpp:2112-2132`, `cr_ui.cpp:4056-4063`), the fence pair + `Report.MultiExtentSpannedVolumeRefused` green, suite 1094/1094 — F62/B8/R43/Q65 cells all MATCH; the finding retired to findings/FIXED/ (the no-delete mandate — the purge lane's deletion converged to the archive), board SEVERE 4 → 3.**
- **lost-partition/orchestration rows, new cells**: the candidate-source model — **M-118** (the og's THREE-source assembly mbr/filesystem/register decoded engine-side: the MountPoints2\CPC\Volume walk at PartitionRecoveryD:8930-8965 + FUN_18000f5b0's Data-blob field decode; the register channel absent in core; S-051's fix plan builds table-only targets; B1/A25/A30/O37 cells — evidence/decompiled-quotes/s12-lostpart-register-channel.md).
- **storage/writer/cli rows, new cells**: partition-boundary read semantics — **M-119** (the og's CBasicPartitionDevice read = clamp + zero-fill + SUCCESS, RTTI-confirmed at DRWUI:183633-183666 and observed firing in the real-run log DRWUI.log:638; the core's PartitionSlice hard-errors per-read and the CLI/UI opens REFUSE whole volumes on over-long table lengths; the discovery path already clamps — the internal inconsistency; F60/B6/E23/X51 cells — evidence/decompiled-quotes/s12-partition-read-clamp.md).
- **session row, new cell**: the variant-save device-descriptor record — **N-142** (the 76a672b0 GUID record's 5400-B volumeDevice blob: firmware/WWN/interface + the 512-B boot snapshot have no identity-card equivalent; D12 field parity — evidence/decompiled-quotes/s12-rsf-fifth-record.md).
- **board-truth cells**: **N-154** (bookkeeping batch #2: the N-079 ID collision in cr_log.cpp:30-34's comment vs FIXED-LEDGER:259; the GPT-salvage strictness unregistered vs ldm.md:53's plain-M; tree/err/reghive/apply module rows absent — I-030's remainder; the stale 1027-vs-1117 suite count) and **I-035** (register batch: RegLib's hive-WRITE half, DiskHealth's resident-monitoring product frame, the Explorer context-menu verb + the DiskHealth-artifact trayicon erratum).
- **S-050 addendum**: the fix plan's build order gains the video engine (the corpus's deepest-decoded repair algorithm + the only engine-side seam — the datarun-selection consumes tree extents), the eligibility-signature seam, and the output-location contract.

**Session-10 lane-B changes (2026-10-29 — the continuation deep audit, audited `84a1832`, RECONCILED at the second push):**
the second verification lane (convergent with s10-reverification-sweep.md; the unique
verdicts + the superseded-verdict reconciliation table at
`evidence/s10-verification-pass-b.md`) plus three deep-comparison lanes. NEW rows:
preview text-eligibility **M-108** (EXECUTED — the TxtViewFiles whitelist vs the
content sniff; BOM/UTF-16/Latin-1 false-negatives, printable-head false-positives;
renumbered twice at push — M-103→M-105→M-108 as the parallel lanes consumed the IDs);
gate-doc false-"landed" triplet **N-132** (API-DELTA rows 44-45 vs R-31; renumbered
from N-128 — the SA-10 lead lane's capability-matrix row owns it); register numeric
defects **N-129**; the undispositioned Fixo engine **N-130** (VideoFileRepairer.exe,
SIDECAR-OOS); the FS-native parent anchors **N-131** (the N-100 class; the full
field-parity table at evidence/s10-field-parity-table.md); tracked-open **OQ-S10-2**
(VideoViewer/sfc); erratum **E69** (part2's ReFS FLAGS contradiction — the
population-site read). BOARD MOVE: **M-100 resolved closed-as-covered** (FIXED-LEDGER
row — the `6d0f16c` capability predates the finding's own baseline; the one move the
parallel lanes missed). CONVERGENT with the remote SA-10 close: the S-050→S-051
collision repair (both lanes made the same repair). Executed evidence:
s10-preview-text-eligibility.md + s10-rsf-ground-truth.md (the real-.rsf container
grammar byte-confirmed — no erratum needed).

**SA-10B changes (2026-10-29, the B lane — the real-.rsf session mining; evidence: evidence/executions/s10b-real-rsf-session-mining.md):**
- **carve row, new cells**: tree-shape/naming parameters — **S-048** (the
  per-volume grouping — **FIXED, session-40 severe lane**: the Reconstructed root + the
  7 empty category skeleton folders + the lazy per-volume group children + the empty
  per-ext folders + the files directly under the volume child; the volume attribution is
  the tree position, the path-prefix query axis; the marker-2 session record; B8/A26/A42/
  D12/D13 cells → MATCH; full landing record in the finding's Resolution + the
  FIXED-LEDGER row),
  `Reconstructed/<volume>` per-volume grouping absent; B8/A26/A42/D12/D13
  cells), **S-049** (the `<PREFIX>_%06d.EXT` naming contract + the
  content-derived prefixes EXIF-Model/Software, ID3-title, PDF/DOCX-timestamp,
  ZIP-entry, SOF-dims — B10/A28/D12 cells; N-063's remainder resolved;
  **FIXED — session-40 severe lane**, the full landing record in the finding's
  Resolution + the FIXED-LEDGER row: per-format counters, the uppercase-ext
  default, MP4/MOV duration prefixes, PDF Title/Creator+date, DOCX date, WAV
  INAM — B10/A28/D12 cells → MATCH. **Convergent-DU with the S42 lane,
  reconciled at the merge**: the executed counter-key arbitration (TWO
  classes - the per-format branch-1 key shared by plain+dims+duration+
  date-alone rows at 96.7% union density, the per-string branch-2 key for
  every analyser string: ZIP families all 1-based dense, PDF TeX 1..453,
  DOCX jacko 1..6 across six dates), the DOCX creator+date composition, and
  the session-wide CarveNameCounters binding; +3 tests, suite 1229/1229).
- **ntfs/tree rows, new cells**: orphan-grouping parameters — **M-103**
  (`LostDir<record>` per-parent folders + the "File Path Lost" branch name;
  B5/B10/D12/A26 cells; the C2 "roles map 1:1" claim falsified for the lost
  branch).
- **mom-capabilities/product-frame row, new cell**: **S-050** (the file-repair
  line — 9 binaries, 8 artifacts, unimplemented AND untruthfully registered;
  A42/Q68/I-010 cells).
- Executed MATCH recorded: the raw_signs.json regen sync (139/199/62
  byte-identical; the carve row's C19-C21 cells).
- **The FIXED purge (L-A)**: findings/FIXED/ retired to history; all matrix
  cells citing FIXED IDs resolve via FIXED-LEDGER.md.

---
# (prior header) CROSS-REF-MATRIX.md — module × parameter verdict matrix (post-rebase with parallel leg)

**Dev-sign session changes (2026-10-27/28, SA lane — Phase 1, audited `dd82db9`):**
NEW module row **driver-install/dev-sign** (the packaging surface the corpus
documents via 02-kernel-driver-architecture.md §5 + regdrv.reg — prior
sessions never audited install/uninstall/signing posture):
executability **S-036** (installer crash — the cell is the live divergence),
service-configuration **M-060** (group/Parameters/binPath divergence),
uninstall-behavior **M-061**, install-error-semantics **M-062**,
environment-matrix/HVCI **M-063**, signature-lifetime **N-084**,
installer-interaction-model **N-085**, signing-posture **I-026** (deliberate,
MATCH-equivalent). The corpus's other installer parameters — copy destination
(`System32\Drivers`), service shape (Type=1/Start=3/ErrorControl=1),
product-orchestrated lifecycle — are **MATCH** (install.ps1/composite comply;
see the per-cell detail at the bottom). Evidence:
evidence/executions/devsign-run-40549660074/.

**Session-27 changes (2026-10-26, DFX27 — the MAJOR-head lane):** **M-035
FIXED** (engine-side FileFilter validation — the exFAT-only content-consistency
scorer, reject rule decompile-exact with the strict-< correction; scan_candidate
wiring + ResultTree::detach_subtree; lost-partition A30/B3 and query B16 rows
below; findings/FIXED/M-035). **M-058 NEW-FINDING FIXED** (build-integrity/
memory-safety: run()'s cache-failure fallback UAF — build_read_pipe extracted,
ASan fail-before executed; the orchestrator row's R1.6 pipe note). M-049 landed
first from the parallel DFX24 lane (convergence honored).

**Session-30 changes (2026-10-26, DFX30 — tracked-open tail lane):** OQ-FLT-1
RESOLVED (the 169-entry extension table = a 4-engine shared vocabulary, live in
FAT/exFAT/RAW, dead copy in NTFS, absent in 5 engines; the M-035 scorer is
exFAT-only — its Open-questions cell answered); OQ-Z5 RESOLVED (ZFS quick/
complete = the IQuick/ICompleteFileSystemSearchController plugin split, 16 vs
34 vtable slots — core orchestrator is the union, no action); OQ-Z2 RESOLVED
with NEW-FINDING **S-034** (SEVERE — the og walks EVERY uberblock in the
label-0 ring per txg through a per-txg VdevLoc::UberData registry; the core's
newest-txg-only premise REFUTED; D-05 re-verdicted; corpus erratum E45). The
zfs B/D cells: S-034 FIXED (DFX37) — MATCH with the registered emission-dedup divergence.

**Session-31 change (2026-10-26, DFX31 — MINOR tail-first lane):** **N-064**
FIXED — the Dict password-dictionary unlock aid: the generated 228-entry
table + the pure-core parse/attempt loop (cr::dict) + the win-layer
consent action (unlock_with_dictionary, the PowerShell/manage-bde
-Password plumbing) + the CLI `--bitlocker-dict` consent flag + the
cr_ui_unlock_volume_with_dictionary additive ABI export. F61 consent gate
structural; health/bitlocker F61/B1 cells -> MATCH. **N-071** FIXED —
the two-stack disk enumeration (cfgmgr32 disk-interface pass merged into
the numbered loop; residual stacks registered as D-79); storage B8/P42/A54
-> MATCH-with-D-79. **N-059** FIXED — the per-FS session extras
(Btrfs/ZFS/HFS+ secondary payload carriers + engine emission + the
nflags bit-6 append-only block); session D11/D12 cells -> MATCH; the
N-019/M-020 convergence break repaired in the same landing. **N-066**
FIXED — the identity card's record-3 parity (v4 tail: firmware/WWN/
interface + the boot snapshot; session D12/B9 -> MATCH) + **N-080**
(NEW-FINDING) — the descriptor offset transposition fixed (storage D11
axis -> MATCH).

**Session-30b changes (2026-10-26, DFX30-MAJOR-tail lane — this lane):**
**M-041 FIXED** — the og's FileSystemCheck parity module
(`include/cr/check/cr_fscheck.h` + `src/core/check/cr_fscheck.cpp`): the full
error taxonomy 0x1001..0x100e, the FAT pass in og phase order (DBR ceilings
341.3333333/0x100/>>(2), dead-zone + duplicate-ref scans, pairwise copy
compare, dir-tree audit, orphan sweep with the host decision), the NTFS pass
($MFT/$MFTMirr proofs, version gate {1.2, 3.0, 3.1}, $Bitmap structural,
the $MFT::$BITMAP-gated MFT×$Bitmap cross-check 0x1002/0x1003), the 8-event
callback surface, the never-abort wiring (ScanConfig::pre_scan_check after
repair), REC_FS_CHECK, the cr_ui ABI appends (union with the parallel
M-049 error-context append) + CLI `scan --check`. New module
row: `check-phase` (modules/check-phase.md); deliberate deltas registered as
D-87 (the undecoded slack term, the param superset, the partial-load refusal).
15 fail-before tests; suites 758/758 release + ASan. **N-079 CONVERGENT**:
this lane's leak repair (eb3cb6d) reconciled to the parallel wave's canonical
deque-pool implementation (first-landing-wins; findings/FIXED/N-079 is the
parallel lane's file — this lane's convergence record rides the worklog).

**Session-25 changes (2026-10-26, DFX25 — INFO tier closed into FIXED/, tracked-open resolution wave):**
the 22 acknowledged INFORMATIONAL findings (I-001..I-024) moved from
`findings/INFORMATIONAL/` to `findings/FIXED/` — the directory is now empty
and the `ack` cells below denote acknowledged-and-filed-in-FIXED deliberate
divergences (each file carries its ACKNOWLEDGED Resolution; pin tests green on
`716d121`, 737/737). No verdict changed — `ack` was and remains MATCH-by-
deliberate-divergence; only the filing location moved per the user directive.
Tracked-open dispositions recorded per-OQ in the session's worklog entries.
**Session-24 changes (2026-10-26, DFX24 — the disjoint MAJOR lane; DFX23 owns the MINOR queue
in the same window):** **M-049 FIXED** — the error-model row below (err+abi): the session-11
half (header truth + err_text coverage) + this session's close-out of the finding's registered
step 2, the engine-side error-context subset (the og's per-module error registrars, Error.md
census): the error-range event family carries the module/operation/retry-state triple
(`err_module`/`err_operation`/`err_attempts` on `cr_ui_event`, CR_ERRMOD_*/CR_ERROP_* ABI
constants pinned by static asserts), attributed per-range at failure time (cr::errmod thread
stamp + device-owner fallback for engine-internal pools); sessions keep the geometric trio
(wire-compat pinned both directions); D-68 updated. Fail-before RED executed at the retry-device
seam (5/6); suite 745/745.

**Session-22 changes (2026-10-26, DFX22 — build-integrity NEW-FINDING + the carve overlap/scoring wave):** **M-055**
(MAJOR) — the `src/win` platform layer was UNCOMPILABLE since `3b5397a`/`915bce3`/
`d8f6808` (four structural breaks; see the finding for the per-commit decomposition);
Q67 buildability now MATCH (win_syntax_check parse gate on the Linux lane — the
fail-before/pass-after proof is in the finding). The writer+report and quality rows
below carry the M-055 cells.

**Session-5 changes (2026-10-26, audited `7cf6754`):** the never-cited-artifact wave —
new findings **A-005 (ACUTE — rawfixer 4Kn stack-buffer overflow; the first ACUTE since
A-004), S-031, S-032, M-046..M-051, N-051..N-064, I-021, I-022**; OQ-S4-1 RESOLVED (the
169-entry grading table dumped byte-exact — S-023 addendum + evidence file); OQ-S4-2
RESOLVED-with-residual (the vtable+0x48 hook is a per-scan filter-config fetch, not a
namer — N-063 + corpus errata row 37); M-029/N-043 addenda (filterbar decode + liveness
evidence); corpus errata rows 37-40; three evidence files (grading-table dump, session-5
quotes pack, baseline 366/366 execution). New module coverage: `err` gains M-049 (via the
cross-cutting err/error-model row below); `win/elevation` + `destio` registered in I-021.

Legend: `M` = MATCH · `A-n`/`S-n`/`M-n`/`N-n`/`I-n` = finding ID (in
`findings/<SEVERITY>/`) · `NA` = NOT-APPLICABLE (reason in the module file) ·
`ok` = MATCH-with-note (see module file). Full per-parameter expansion in each
`modules/<module>.md`.

**Session-5b changes (2026-10-26, audited `7cf6754` rebased onto `c75f646`):** one NEW
module row — `mom-capabilities` (first-time coverage of the never-dispositioned
MOM/product surfaces: apply-write, boot-repair, imaging, check-phase, recognition,
session-persistence, logging, mom-tool-ops, mom-runtime) with findings
**S-029, S-030, M-040..M-045, N-048..N-050, I-019, I-020** (13 new — renumbered
past the storage/census leg's S-024..S-028/M-031..M-039/N-045..N-047/I-017/I-018;
no content overlap); existing rows gain: ntfs += M-043 (content-proof recognition
+ scanner backup-boot), fat += M-045 (boot-gate family — resolves S-006's FATX
remainder: no Xbox parser exists), session += M-042/N-049 (lostpart.dat resume
re-validation + record richness), lost-partition += S-029 (apply/table-write
path). OQ-S4-1 RESOLVED (the 169-entry grading extension table dumped — `.data`,
unsorted; S-023's fix-plan step 3 input complete); OQ-S4-2 RESOLVED (the RAW
naming hook = single-site "filter_generalraw" display-name mapping; M-029 §Gap
(b) corrected); S-019's fix verified in-source; corpus errata rows 37-42 filed;
one new tracked-open OQ-S5-1 (CheckVersion's consumer). The stale duplicate
fat/exfat row pair (lines 44-45 vs 46-47, an old parallel-leg merge artifact)
consolidated — the S-013/S-021-current rows survive (f210b7e precedent).

**Session-4 changes (2026-10-26, audited `63a9167`):** three NEW module rows —
`preview`, `query`, `transport` (first-time audits of modules no prior session
covered) — with findings **S-023, M-028, M-029, M-030, N-040..N-044, I-015, I-016**
(renumbered past the parallel S3/S4 legs' own S-016..S-022 / M-024..M-027 /
N-033..N-039 / I-013/I-014 — cross-references inside the parallel legs' cells for
THEIR IDs are untouched); `orchestrator` gains S-023 (grading); `abi` gains
N-040/N-041/I-016; `cli` gains N-044; `health` gains N-042; OQ-S2M-1 RESOLVED
(S-023); OQ-S2M-2 partial (NTFS bonus setters decoded); M-022/M-023 amended
(OQ-1/OQ-3 resolved; counts corrected); corpus errata rows 29-36 filed (after the
S3-CARVE leg's 22-28). A-004 + S-016 verified FIXED in-tree (`c33236f`) — ACUTE
tier empty since that commit. The stale duplicate `carve` row left by the parallel
legs' own merge was removed (their updated row with the S-015/S-017-fixed
dispositions is the surviving one).

**Session 2 + parallel-leg combined changes:**
- New finding IDs (session 2): S-010, S-011, M-007, M-008, M-009, N-011, N-012,
  N-013, N-014, N-015, N-016, N-017, N-018, N-019, N-020, N-021 (was N-010,
  renumbered to avoid conflict with parallel-leg's N-010), I-006, I-007.
- New finding IDs (session-2 MAIN-AUDITOR leg, renumbered post-rebase to avoid the
  collisions above): **A-004, S-012, S-013, S-014, M-011..M-021, N-023..N-029,
  I-009, I-010, I-011**, plus OQ-S2M-1/OQ-S2M-2 (tracked-open). One candidate
  RETRACTED by the leg's own re-verification (exFAT fixer checksum predicate —
  core MATCH; see modules/exfat.md addendum).
- New finding ID (parallel leg): N-010 (FAT BAD sentinel) — FIXED in same commit.
- Resolved (moved to `findings/FIXED/`): A-001, **A-002 (fix-induced LDM GUID
  regression — restored to the real Microsoft values; test fixtures
  de-self-referenced)**, S-001, S-004, S-005, S-008,
  M-001, M-006 (combined with S-009 — split tracked in N-020), N-002, N-003,
  N-010-theirs, OQ-T1.
- Acknowledged (filed in FIXED/ since session 25, with pin tests): I-001..I-005.
- S-009 partially FIXED (disk-map visualization via GAP-S6-11); the combined
  S-009/M-006 finding file remains in SEVERE/ pending split (tracked in N-020).

| Module \ Group | B | D | C | A | S | O | P | X | R | F | Q |
|---|---|---|---|---|---|---|---|---|---|---|---|
| ntfs | M-007, M-008 (S2); N-011 FIXED-ALREADY + N-012 FIXED (minors session 2026-10-26, D-52); I-001 ack; S-012 FIXED (two-pass: pass-1 membership + pass-2 dump walk + size-0 rescue; convergent dual-agent fix); ****M-043 FIXED (s10: backup-DBR fallback + MFT content proof — findings/FIXED/M-043)** (S5b: content-proof recognition + scanner backup-boot)** | M (S-001+S-012 fixed: membership keep), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M; N-021 (S2); **M-045 FIXED (s10: validate_boot any-512-multiple bps + numFATs + cluster-cap; classifier already N-034 — findings/FIXED/M-045)** (S5b: boot-gate family — resolves S-006's FATX remainder) | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M-013 FIXED (s16: the dual-form deleted-set checksum gate + the parked-set reconstruction oracle — FUN_18001eba0/FUN_1800203a0 decompile-exact; findings/FIXED/M-013), M-014 FIXED (s35: structural entry gates — FUN_180002f90/FUN_180029280/FUN_180003030 decompile-exact; findings/FIXED/M-014), M (S-011 + S-013 fixed), N-003 FIXED (premise recorded REFUTED by M-013), OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | **M-054 (S10 NEW-FINDING) FIXED — allocation-bitmap bit i vs cluster i+2 shift (grading + the S-009 producer wrong on real volumes; spec §7.1; producer pin test added)**; M (debt) |
| ntfs | **M-007 FIXED (session 11: single_link_per_record parity default + the repaired per-parent fan-out, D-65)**, M-008 (S2); N-011 FIXED-ALREADY + N-012 FIXED (minors session 2026-10-26, D-52); I-001 ack; S-012 FIXED (two-pass: pass-1 membership + pass-2 dump walk + size-0 rescue; convergent dual-agent fix); **~~M-043~~ **FIXED (s14, FIXED/M-043: content proof at probe; backup-fallback premise corpus-disproven)** (S5b: content-proof recognition + scanner backup-boot)** | M (S-001+S-012 fixed: membership keep), N-012 (S2) | M | M-008 (S2) | M (M-001 fixed), N-001 | NA | NA | M | M | M | M (debt), N-019 (S2) |
| fat | M; N-021 (S2); **M-045 (S5b: boot-gate family — bps/SPC/label/numFATs/cap/band-retry; resolves S-006's FATX remainder)** | M; N-021 (S2) | M | I-002 ack, OQ-T2 | M | NA | NA | M | M | M | M (debt) |
| exfat | M (S-011 + S-013 fixed), N-003 FIXED, OQ-T5 | N-003 FIXED; M (S-011 fixed) | M | M | M | NA | NA | M (S-013 windowed-FAT FIXED) | M | M | M (debt) |
| ext4 | S-004 FIXED, S-025 FIXED (two-pass reparenting, D-58/59 siblings), S-026 FIXED (meta_bg resolver, D-58), S-027 FIXED (backup-SB rescue, D-59); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks; S-027 fixed: backup-SB positions + s_block_group_nr) | S-004 FIXED, S-025 OPEN | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, **S-003 FIXED (s5 + closing-session verification: JIB layout corrected, gates, strict validator, fallback salvage)**, **S-021 FIXED (s5 + closing-session verification: attributes branch + marker derivation resolved)**, N-005 FIXED, N-038 FIXED, **A-007 FIXED**; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) **N-047 (S3) FIXED s22** (volume-node naming = registered partition-layer divergence, modules/hfsplus.md) |
| ext4 | S-004 FIXED, S-025 FIXED (journal reparenting: orphan map + iNode naming + claim interaction), S-026 FIXED (meta_bg resolver, D-58), S-027 FIXED (backup-SB rescue, D-59); **M-010**, I-006 (S2), I-007 (S2) | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 meta-group desc blocks; S-027 backup-SB positions + s_block_group_nr) | S-004 FIXED, S-025 FIXED (two-pass realized as harvest+claim checks; parent map + topo anchors) | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, **S-003 FIXED (s5)**, **S-021 FIXED (s5 — journal harvest)**, N-005 FIXED, N-038 FIXED; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) || btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010 FIXED (lossless mapping)**, I-007 (S2) | M; **M-010 FIXED**, I-007 (S2); **S-028 FIXED** (xattrs: DOSATTRIB bits + user.* records + the compression-property tag) | M | M | M | NA | NA | M | M | M | M |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2); S-034 **FIXED (S33b: every ring state walked newest-first + identity dedup; findings/FIXED/S-034)** | S-034 **FIXED (S33b: deleted-in-window files surface as Deleted nodes — RingWindowDeletedFileRecalled et al.)** | OQ-T6 | M | M | M (S-034: deleted-in-window recall; beyond-window via carve) | NA | M | M | M | M || refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved); **S-020 FIXED (s4)**; **M-026/M-027 FIXED (s4: stale-page sweep + identity keep-one + recycle routing)** | M | M | **M-022 FIXED (s4)** | M | NA | NA | M | M | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-031 FIXED (carved nodes flag Deleted - og FLAGS=0x20 const); S-007 (primary FIXED s5), M-005 | M | S-007, **N-008 FIXED s11** (sector-strided salvage + no clean-archive inflation + ZIP64 chain; gate half via N-068); M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s10)** (consent-gated apply module + `rrecov apply`; findings/FIXED/S-029) | M | NA | NA | M | M | M | M **N-035 (S2) FIXED s22** (data-area-anchored containment/overlap extents; both extents on Candidate; fail-before executed) |

| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s10)** (consent-gated apply module + `rrecov apply`; findings/FIXED/S-029) | M | NA | NA | M | M | M | M |

| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | **S-029 FIXED (s16: the apply/table-write path — consent-gated transactional writer, corpus IOCTL chain, undo; findings/FIXED/S-029)** | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | M | M (D12), **N-014 (S2) FIXED s22** (atomic staging + commit-by-rename) | **M-042 (S5b: lostpart.dat resume re-validation + layout snapshot missing)**; N-049 (S5b: record richness) | NA | M; N-014 (S2) | NA | NA | M | M | I-005.7 ack | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2) FIXED s7 (FIXED/ move s11)**, **N-015 (S2) FIXED s9 (`915bce3`; FIXED/ move s11)**, **M-055 (S22 NEW-FINDING) FIXED — the win layer those fixes landed into was uncompilable; four structural repairs + the win_syntax_check gate** , N-009d | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock; N-064 FIXED s31 — the Dict password-dictionary unlock aid, consent-gated) | NA | ~~N-009a~~ FIXED (u8 width, s2-wave; group closed s23); N-055 (S5) FIXED by the S-024 fix leg (0xD1 + 4-status grading) | NA | **M; S-024 (S3) FIXED — decoded-truth upgrade (s6)**: FUN_0040cdf0 exact (family-gated SSD-life, bad-count/caution/graded aggregation, temp never graded, NVMe family-0x13 path; analogs superseded — S-024 Resolution addendum + evidence s6-health-grading-decode.md) | NA | NA | NA | M | M | M (M-036 FIXED s6: NVMe full row set + units→bytes; ABI v2); **N-032 (S3) FIXED s30 (CSMI CC-0x19 transport wire-decoded + the pure three-transport chain seam; findings/FIXED/N-032; D-78)**; **N-042 (S4) FIXED s7 `3678cbf` (FIXED/ move s11)** |
| rawfixer | M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | M | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | ~~M-005~~ **FIXED (s14, FIXED/M-005)** | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4) FIXED** (FIXED/S-023: post-scan grading pass + RecoveryAssistant statistics + ABI regrade); ~~N-043~~ **FIXED (s18 DFX18: CR_EVT_HEARTBEAT — the honest 5 s liveness watchdog, standing truth not synthetic advance; TSan-clean)** | NA | M | M | M | M |
| visualization | S-009 (partial-FIXED-in-tree, finding file stale — N-020) | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M |

| ext4 | S-003 FIXED (HFS+ journal differential harvest, landed with S-021), S-004 FIXED, S-026 FIXED (meta_bg resolver, D-58); **M-010**, I-006 (S2), I-007 (S2); **M-037 FIXED (s10: resync-slide damage model — findings/FIXED/M-037)** | ok; **M-010**, I-006 (S2), I-007 (S2) | M (S-026 fixed: meta-group desc blocks) | S-025 OPEN (theirs: journal reparenting), S-004 FIXED | N-004 | NA | NA | M | M | N-004 | M (debt) |
| hfsplus | S-002 FIXED, S-003 FIXED, S-021 FIXED (s5 — journal harvest, the og's PRIMARY deleted surface), N-005 FIXED, N-038 FIXED; I-007 (S2) | M; I-007 (S2) | M | M | M | NA | NA | M | M | M | M (S-021 tests; THIN closed) |
| btrfs | I-003 ack, M (S-005 fixed), N-006; **M-010**, I-007 (S2) | M; **M-010**, I-007 (S2) | M | M | M | NA | NA | M | M | M | M **N-047 (S3) FIXED s22** (ROOT_REF subvol names + 0x80 file attribute base, corpus :3574-3577; both fail-before-proven) |
| zfs | OQ-T6, M (S-005 fixed); I-007 (S2); S-034 **FIXED (S33b: every ring state walked newest-first + identity dedup; findings/FIXED/S-034)** | S-034 **FIXED (S33b: deleted-in-window files surface as Deleted nodes — RingWindowDeletedFileRecalled et al.)** | OQ-T6 | M | M | M (S-034: deleted-in-window recall; beyond-window via carve) | NA | M | M | M | M || refs | N-007, I-004 ack, **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved — parity); **S-020 FIXED (s4: resident+all-rows+paged+granularity)** | M | M | **M-022 FIXED (s4: dual grammar + gen tag)** | M | NA | NA | M | **M-031 FIXED (s10: per-file progress ticks — findings/FIXED/M-031)** | M | M |

| zfs | OQ-T6, M (S-005 fixed); I-007 (S2) | ok | OQ-T6 | M | M | NA | NA | M | M | M | M |
| refs | **N-007 FIXED s34 DFX34 (first-DWORD discovery + signature_partial grading flag, fail-before executed; findings/FIXED/)**, I-004 ack (D-48b preserved), **M-022 FIXED (s4)**, I-012 (S3: GAP-ADS resolved — parity); **S-020 FIXED (s4)**; **N-036 FIXED s23 (the container-map half: og formula + populated map, fail-before executed; findings/FIXED/N-036)** | M | M | **M-022 FIXED (s4: dual grammar + gen tag)** | M | NA | NA | M | **M-031 FIXED (s10: per-file progress ticks — findings/FIXED/M-031)** | M | M |
| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), **M-002 FIXED (s10: torn chain keeps walked prefix — findings/FIXED/M-002)** | M | M (A-002 FIXED; A-001 fixed), **M-046 (S5: LDM GPT-path 512-hardcodes)** | M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007, ~~M-005~~ **FIXED** (FIXED/M-005, s14: the allocation-bitmap gate in both dispatch layers + telemetry; bench 1.88x on 50% used), **S-031 (S5: carved nodes never marked deleted)**, **S-032 (S5: archive inner-entry inventories)** | **S-031 (S5)**, **M-051 FIXED (s9)** (checksum/anchor/note/flag carriers; nflags-bit4 round-trip) | S-007, N-008, **N-051/N-052/N-053/N-054 S12 FIXED (convergent landings: the parallel session-11/7 lane's b2f48c6 canonical + this session's N-076 add-on)**; M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | S-007, N-008, M-005, S-032 (S5: multi-part stitching) | S-007, N-008, M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | **N-063 FIXED s17 (og per-analyser names; File%06u = registered default)**, **M-048 (S5: user-signature extensibility)** | M |

| table+ldm | M (A-002 FIXED — real Microsoft GUIDs restored), M-002 | M | M (A-002 FIXED; A-001 fixed) | M-002, M (OQ-T7 resolved — I-008) | ok | NA | NA | M | M | M | M (debt) |
| carve | S-007 **FIXED (primary set + full audio family)**, M-005 | M | S-007 **FIXED** (niche ~25-format tail precisely scoped in the finding), **N-008 FIXED s11** (sector-strided salvage + no clean-archive inflation + ZIP64 chain; gate half via N-068); M (S-015 units FIXED), M (S-017 ftyp magics FIXED) | M-005 | M | NA | NA | M (S-015 tail-walk FIXED: named 8 MiB bound, og range documented) | M | M | M |
| lost-partition | M (S-006 fixed; S-019 FIXED s4 — BitLocker family), OQ-T9 RESOLVED (M-017 formula) | NA | M (S-006 fixed; S-019) | M | M | NA | NA | M | M | M | M |
| tree | M | M | NA | M | M | NA | NA | ok | M | NA | M |
| session | **M-047 (S5: stats round-trip asymmetry)**, **M-050 (S5: node-model carriers)**, S-031-adj (S5) | M-047, M-050, **N-059 (S5: btrfs/zfs extras)**, ~~N-014~~ FIXED, ~~N-049~~ **FIXED-ALREADY s23 (M-042's v2 payload verified + SizeSourceFlagRoundTrip pin added; findings/FIXED/N-049)** | M | NA | M; N-014 (S2); **N-058 (S5: load validation deltas)** | NA | NA | M | ~~N-060~~ **FIXED (s18 DFX18)** | I-005.7 ack; ~~N-061~~ **FIXED (s18 DFX18)** | M (debt) |
| writer+report | M | M | NA | M | M | M | NA | ok | M | **N-013 (S2) FIXED s7 (FIXED/ move s11)**, **N-015 (S2) FIXED s9 (`915bce3`; FIXED/ move s11)**, ~~N-009d~~ **FIXED s23 (D-77 registered — inherit-destination ACL posture; findings/FIXED/N-009)**, **N-056 (S5) FIXED (s17)**, **M-055 (S22 NEW-FINDING) FIXED — win-layer buildability restored + gated** | M (debt) |
| health | M (S-014 FIXED s4 — live+offline auto-unlock; N-064 FIXED s31 — the Dict password-dictionary unlock aid, consent-gated) | NA | ~~N-009a~~ FIXED (u8 width, s2-wave; group closed s23); N-055 (S5) FIXED by the S-024 fix leg (0xD1 + 4-status grading) | NA | **M; S-024 (S3) FIXED — decoded-truth upgrade (s6)**: FUN_0040cdf0 exact (family-gated SSD-life, bad-count/caution/graded aggregation, temp never graded, NVMe family-0x13 path; analogs superseded — S-024 Resolution addendum + evidence s6-health-grading-decode.md) | NA | NA | NA | M | M | M + **N-032 (S3) FIXED s30 (CSMI CC-0x19 transport wire-decoded + the pure three-transport chain seam; findings/FIXED/N-032; D-78)**; **N-042 (S4) FIXED s7 `3678cbf` (FIXED/ move s11)** |
| rawfixer | **A-005 (S5 ACUTE: 4Kn stack-buffer overflow)**, M (S-008+S-010 fixed) | NA | M | M (S-008 fixed) | M | NA | NA | M (M-009 fixed) | **M-046 (S5: 4Kn exFAT plan truncation)** | M (S-008+S-010 fixed) | M |
| orchestrator | M | M | NA | ~~M-005~~ **FIXED (s14, FIXED/M-005)** | M; **N-016 (S2), N-017 (S2)** | **S-023 (S4: no post-scan grading engine / RecoveryAssistant — S5 addendum: OQ-S4-1 RESOLVED, table dumped)**; ~~N-043~~ **FIXED (s18 DFX18: CR_EVT_HEARTBEAT — the honest 5 s liveness watchdog, standing truth not synthetic advance; TSan-clean)** | NA | M | M | ~~N-061~~ **FIXED (s18 DFX18)**, ~~N-062~~ **FIXED (s18 DFX18: the memory monitor on the N-048 seam — check_memory_pressure + the platform sampler on the watchdog cadence + memory_warn_mb appended config; never scheduling-coupled; fail-before RED executed)** | M |
| visualization | **S-009 FIXED (S8 — the free overlay: engine bitmap truth produced + persisted + snapshot-carried; `kCellAllocFree` now real)**; ~~partial-FIXED-in-tree~~ | **M-023 (S3)** FIXED (findings/FIXED/M-023: generated 1,481-ext table + Email/Bookmarks cats; runtime-XML surface = deliberate non-adoption) | NA | M | M | NA | M | M | M | NA | M; **N-057 (S5: Ebr enum + budget-growth test debt remain; the bad-overlay coverage item paid by S-009)** |
| abi (cr_ui) | NA | M | ok | NA | M | M | **S-009 FIXED (S8: the free overlay rides the same `cr_ui_disk_map` ABI — no signature change; snapshots carry errors/free-map/view)** | M | M | M | ~~N-040~~ **FIXED (s18 DFX18: D4 closed — truthful era labels + §9 coherence; D1/D2/D3/D5 verified already-landed via §8/S-009/§6/M-029; three new pins incl. the fail-before-proven AbiVersionAnnotationsAreTruthful)**; N-041 (S4: dead code); I-016 (S4: factory register + FS-code spaces) |
| orchestrator (N-050 row) | **N-050 FIXED s23** (two-stage recognition: exFAT + FAT content proofs, contract documented in ground-truth-contracts §0) · ~~N-009c~~ **FIXED s23 (cr::text::wide_to_utf8 + the shared ASCII-ci contract; findings/FIXED/N-009)** | NA | M | NA | M | NA | M | NA | M | M | ok; N-020 (S2) FIXED; **N-044 (S4: --deep naming hazard)** |
| quality (cross-cutting) | NA | NA | NA | NA | NA | NA | NA | NA | NA | NA | **N-018 (S2) FIXED s7 `b1f9210` (FIXED/ move s11)**, N-019 (S2), **N-020 (S2) FIXED (doc-accuracy batch; FIXED/ move s22)**; M-006 FIXED; **M-053 (S8 NEW-FINDING) FIXED — the C-ABI consumer check itself was dead since M-015 (stale fixture → BAD_MAGIC + NULL-deref crash on every run, both lanes); fixture fixed + precondition bail-out, gate live again**; **M-055 (S22 NEW-FINDING) FIXED — win-layer buildability: four structural breaks repaired + the win_syntax_check parse gate (Q67)**; **M-057 (S23 NEW-FINDING) FIXED — app-layer buildability: cmd_apply's undeclared `&e` (MSVC-uncompilable since `0a68937`) + the parse gate extended to src/app (Q67 now covers every Windows-only directory)** |
| preview (S4 NEW) | NA | NA | NA | NA | NA | NA | NA | NA | **M-028 (S4: caps bypass — executed proof; provenance misattributions) FIXED (s21 — read_cost-gated budgets + checked_read; findings/FIXED/M-028)**; M-024 landed s15 (CarveFlags::disabled_formats) | I-013 (S4: architecture register — og socket protocol vs in-process reader) |
| query (S4 NEW) | ~~M-029~~ **FIXED (s32 DFX32: bucket-ID translation + general-raw naming — the decode-blocked remainder closed with the DRWUI first-hand decode; Resolution in FIXED/M-029)** + ~~M-023~~ FIXED (FIXED/M-023: category source now the generated table) | NA | NA | NA | NA | NA | NA | NA | NA | ~~M-029~~, ~~N-040~~ FIXED | M-029 (S4); S-023 grading rows now carry grades + sort key 6 |
| transport (S4 NEW) | **M-030 — FIXED (session 23)** (WinPE gate + stale-handle ladder [prior leg] + terminal OFFLINE + restore chain + 200-disk bound; OQ-2/OQ-3 decoded first-hand; findings/FIXED/M-030) | M-030 **FIXED (s23)** (the PE probe + direct-first on both open paths) | M-030 (IOCTL numbers = own driver's, grammar shape M) | M-030 (chunking provenance corrected; OQ-S4-3 perf open) | M-030 **FIXED (s23)**; **S-030 FIXED (S8 — image scan targets: flat/VMDK/VHD through `cr/storage/cr_image`, CLI --image, session v3 identity tail; D-64 registers edisk/dmg exclusions + VHDX/parent-chain remainders)** | NA | NA | M-030 **FIXED (s23)** (cr_ui_last_transport_error, ABI-since 2.2) | M-030 **FIXED (s23)** | M | **N-041 (S4: dead RdaClient + volume_letter_to_disk)**; I-016 (S4: wire register) |
| mom-capabilities (S5b NEW) | **M-043 (S5b: NTFS content-proof + backup-boot) FIXED s15**; **M-044 (S5b: UDF family) FIXED (s21 — the og-exact AVDP→VDS probe + FsFamily::Udf 11; findings/FIXED/M-044)**; **M-045 (S5b: FAT gates) FIXED s15**; N-050 (S5b: boot-only recognition) | **M-042 (S5b: lostpart persistence semantics)**; N-049 (S5b: record richness) | I-020 (S5b: corpus-doc corrections register) | **S-029 FIXED (s10)** (apply module + CLI; PARITY-DELTAS D-68/D-69) | **M-040 FIXED (stages a+b — findings/FIXED/M-040: stage a cr/bcdfix s34 DFX34, stage b cr/nvramfix + the win layer da46b14 s37 DFX37 — the FUN_1800046f0 matcher + FUN_180004b40 full-position rewrite decoded first-hand, the dynamic firmware seam (the og's :3281-3290), the BCD file side + the BCD00000000 force-unload; boot.ini + chkdsk REGISTERED (D-90); the host wiring landed as `rrecov apply --boot-fix`)** — was: (S5b: BCD/NVRAM/boot.ini/chkdsk family)**; ~~M-041 (S5b: check phase + error taxonomy 0x1001-0x100e)~~ **FIXED (s30 DFX30 — the cr/check module: full taxonomy + FAT/NTFS passes + never-abort wiring + REC_FS_CHECK + ABI/CLI; 15 tests; findings/FIXED/M-041; module row: modules/check-phase.md)** | **S-030 (S5b: image scan targets) FIXED (s8 — findings/FIXED/S-030; module row: modules/imaging.md)** | ~~N-048~~ **FIXED (s18 DFX18: the cr::log seam — host sink default-NULL + START/END/stage/checkpoint/candidate/read-error sites; CLI --log wired as the EPMLOG-role sink; D-75 registers the single-process divergence; fail-before RED executed)** | **I-019 (S5b: EPM tool-ops scope register)**; OQ-S5-1 (CheckVersion consumer) | M-040 pending (S-029 decided: implemented v1 MBR — D-68) | NA |
| check-phase (S30 NEW) | **M-041 FIXED (s30 DFX30)** — the damage-audit phase: FAT pass (DBR ceilings, dead-zone/dup scans, pairwise copy compare, dir audit, orphan sweep) + NTFS pass ($MFT/$MFTMirr/version/$Bitmap/cross-check), taxonomy 0x1001-0x100e | NA | NA | NA | NA | **M-041 FIXED (s30)** — never-abort wiring, REC_FS_CHECK, ABI, CLI --check | NA | NA | NA | NA | M-040 (the escalation endpoints) stays open on its row |

**Session-5 fix note (2026-10-26):** S-021 + S-003 FIXED (HFS+ journal
differential harvest — PARITY-DELTAS D-56; the fallback residual tracked in
OQ-S003-journal-fallback-salvage.md).

Cross-cutting rows: **Q66 = M-006 FIXED** (fuzz CI re-enabled by parallel leg `e30b84d`;
codeql deferral documented) **+ M-053 (S8) FIXED** (the C-ABI consumer gate —
registered in ctest — had crashed on every run since M-015's fixture update missed
`tests/ui_abi_check.c`; root-caused with a deterministic plain-C reproducer, fixture
fixed, gate green on release + ASan). **Q67 = M + N-018 (S2)** (ASan/TSan CI green; UBSan NOT enabled —
session 2 found this gap). **Q68 = N-020 (S2)** (session 2 found 6 stale doc claims;
previously marked M). **Q69 = M** (zero stub markers repo-wide, grep-verified). **Q70 = M**
(Win7 posture + import allowlist verified by CI PE-gate). **X47/X48/X49/X50 = tracked-open**
(og-execution not possible here; bench harness is the vehicle). **X51 = M post-GAP-S6-13 +
M-009 (S2)** (64 KiB envelope on the scan path; **session 2 found the repair-path `RwDevice`
bypasses it** — M-009). **X53 = M + N-017 (S2)** (cancel watcher 50 ms + per-record polls;
**session 2 found the orchestrator parallel candidate scan lacks the carve watcher** — N-017).

A cell reading `M (X-n fixed)` or `X-n FIXED` marks a finding the remediation session closed
— the finding file's Resolution section carries the fix commit and test evidence; the
underlying parameter is MATCH as of that fix. Cells with finding IDs marked **(S2)** are new
session-2 findings; the previous `M` verdict was refuted by first-hand decompile + core
source read. Cells marked `ack` are ACKNOWLEDGED informational findings (deliberate supersets
with pin tests, filed in FIXED/ since session 25).

**Session-3 changes (this leg):** S-015 (rawsigns KiB units, SEVERE), M-022 (ReFS
file-value grammar), M-023 (filter model — replaces the old GAP-cats note), N-030
(rawsigns confidence dead in og — **FIXED S12 as the registered D-66 superset,
option (b) of its own fix plan; pins re-cited in the Resolution**), N-031 (carve
dispatch/stride deltas; OQ-T8 RESOLVED), N-032 (health transports), I-012 (GAP-ADS
resolved — parity). Fixed
since the last matrix pass: S-010 (3e34ed2), M-009 (7fd00e0) — both first-hand
verified in FIXED/. CPartitionVirtualVolumeDevice (og virtual-volume device
layer) verified architecturally equivalent to the core IStorageDevice + slice
adapters (devirtualized field reads sector-size@+0x38/size via vt[3]/vt[5] —
HFSPlusSearchDLL :454/:550-572; evidence s3-refs-file-pipeline.md §7 context).

Group keys: B behavioral · D data model · C constants/formats · A algorithms ·
S state machine · O orchestration · P API · X performance · R architecture ·
F safety · Q quality (definitions in `PARAMETERS.md`).


---

# Main-auditor leg cells (session 2, third leg — decompile-exhaustive reads; core `e93decf` re-verified on `34b3441`)

| Module | Parameter(s) | Verdict change (evidence in the finding + `evidence/decompiled-quotes/*session2*`) |
|---|---|---|
| ntfs | B1/B4/D15/A26/B5 | **S-012 — FIXED (convergent multi-agent fix, session 4 + verification/correction)** — two-pass architecture landed: destroy default OFF (pass-1 membership, :18630), full pass-2 GUID-dump walk with claim map + og gates (:21152-21264), size-0 rescue (mirror resident + same-name merge); finding premises corrected (no cross-pass dedup og-side; $Boot = the claim gate's 6-byte compare); the verification leg's corrections on top: dump-walk stride = record size on every path (corpus uVar8/uVar16 decode), pseudo record# arithmetic, N-040 (pass-1 filter = og-15 table AND $SI SYSTEM+HIDDEN; the $FILE_NAME gate removed); Resolution + both addenda in findings/FIXED/S-012-*.md, N-040 in findings/FIXED/ |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024** (8.3 short-name field; two-sided boot ladder; pass-2 stride quirk superset; namespace-ranking superset; og error facility codes) |
| ntfs | B1/B4/D15/A26/B5 | **S-012 FIXED (diff-fix session)** — two-pass landed: pass-1 membership restored (destroy default off), GUID-dump pass 2 (claim-map viability, 0x3ff + blanket-$ + folder rules), size-0 rescue (mirror resident + same-name merge); Resolution in findings/FIXED/S-012-*.md |
| ntfs | C19/C20/D11/D12/B7/C23 | **N-024 — FIXED (minors session, 2026-10-26)** (short_name field + session persistence ✓; boot ladder = LoadNtfsVolume exactly ✓; stride quirk + namespace ranking registered D-52; error-code mapping registered in ABI-CONTRACT §4 — see findings/FIXED/N-024) |
| fat | B1/S35/X49 | **M-011 — FIXED (s4)** (per-flavor windows: FAT32 up to 300 MiB whole-loaded, FAT16 >= 12 sectors) |
| fat | A27/B2/S35 | **M-012 — FIXED (s4)** (mirror scrutiny: shifted-copy/cross-link reject + contiguity accept + forward BAD/EOC disbelieved; mirror-only scan on unreadable FAT1; the N-022 primary-FREE consult landed earlier) |
| fat | B1/B3/A27 | **M-015 — FIXED (s4)** (whole-table validation + copy retry: media pair, sweep, claims-map cross-links; watermark advisory) |
| fat | B1/C19/C20/B3 | **N-023 — FIXED (minors session, 2026-10-26)** (65524 flavor boundary ✓; BPB FS-string/numFATs/bps gates at engine open ✓; dentry date sanity behind `strict_date_sanity` ✓; exFAT tz = FIXED-ALREADY via S-011; stream reserved byte-OR ✓; mid-chain FREE/BAD registered D-51 — see findings/FIXED/N-023) |
| exfat | B2/A27/X47 | **S-013 — FIXED (session 3)** (windowed-FAT accessor, og 1 MiB windows; have_fat never false; extent coverage pinned; windowed failures degrade to the EXFAT-2 salvage) |
| exfat | B3/B4/B5/D15 | **M-013** (og DOES validate entry-set checksums — dual-form incl. deleted sets + reconstruction oracle; refutes the N-003 fix's verified-absence premise; supersedes N-003) |
| exfat | B3/B4/B10 | **M-014 — FIXED (s35)** (structural gates: secondary_count ∈ [2,18]; create+modify+access timestamp field sanity, all-zero FAILS; empty streams never emitted; name flags/first-char gates; reserved1/2 zero — findings/FIXED/M-014) |
| exfat | B3/A1/B6 | **N-174 — FIXED (S46)** (the `._` AppleDouble prefix sink: the og's props-builder refusal at exFAT:45151/:45222 landed in the core's entry walk post-name-assembly, the parked-set oracle inherits it; `apple_doubles_sunk` stats counter (the observability superset); AppleDoublePrefixSunk + RegularDotFilesNotSunk pins — the og's gate is prefix-specific, plain dotfiles unaffected; findings/FIXED/N-174) |
| hfsplus | B3/B5/B7/C20 | **N-028 — FIXED (minors session, 2026-10-26)** (sink filter at both record sources ✓; rsrc superset registered D-53; 'prdf' folderization ✓ — see findings/FIXED/N-028) + **N-038 FIXED** (ownerFlags 0x20 fork selection) + **N-005 FIXED** (block bound 16 MiB + product sanity) |
| btrfs | B1/B2/A26/B5 | ~~M-020~~ **FIXED (s33)** — full profile machinery: RAID10 geometry gate (og :2162-2168), class-0x20 rotating mirror og-exact (:2534-2540), RAID0/RAID10 stripe math as the registered superset (D-80; the og FAILS those classes — audit Summary decode error corrected in the Resolution), stripe-boundary extent splitting, default-subvolume DIR_ITEM pick (:3975-4060) with root-5 fallback rendering at the volume root; 4 fail-before + 2 updated expectations; findings/FIXED/M-020 |
| zfs | B11 (SA registry) | ~~M-038~~ **FIXED (s19)** — registry-driven SA decode; findings/FIXED/M-038 |
| zfs | B1/B2/A25/D15 | ~~M-019~~ **FIXED (s19)** — first-entry fallback for every DVA (foreign reads counted), fletcher4 {7,0} gate over raw bytes + 32-byte compare + next-DVA retry, L0 extents planned at the fallback position; fixtures now checksum-carrying; 2 fail-before + 1 pin; findings/FIXED/M-019 |
| refs | B1/C19/C20/A25 | ~~N-026~~ **FIXED (s28, mixed)** — boot gates aligned (bps ladder + cluster product {4K,64K}, og :13544-49) + 4-KiB open read; N-013a/N-013c INVALID on first-hand decode (0x50 body is by-construction — hardcoded 0x30 ref extension; the mid-volume formula is N-036's container key, og probes NO superblock backup); findings/FIXED/N-026 |
| refs | B1/C19 (N-007) | ~~N-007~~ **FIXED (s33)** — first-DWORD page-signature discovery (og :11692 `*param_2 == *(int*)pcVar4`), full-width memcmp as the grading signal (signature_partial); I-004 half stays ACKNOWLEDGED (D-48b); findings/FIXED/N-007-I-004 |
| table+ldm | B1/C20 | ~~M-016~~ **FIXED (s19)** — candidate list {2,0,1,2045,2046} = og probe order + tail superset behind; config+0 disk parses, +2 wins conflicts (committed_seq proof); 3 tests, findings/FIXED/M-016 |
| table+ldm | B1/A27/X49 | ~~N-025~~ **FIXED (s23)** — 1 MiB config gate (>= 0x801 -> ERR_BAD_CHECKSUM, og :59748-59759) + zero-interleave 64 KiB default on stripe/RAID-5 (og :47665/:68631-68641); 2 fail-before tests + boundary pin; findings/FIXED/N-025 |
| lost-partition | B3/A30/B16 (engine-side filter validation) | **M-035 — FIXED (s27 DFX27)** (the exFAT-only content-consistency scorer: considered/filterable/matched counters + the reject rule decompile-exact (strict < 0.3), substring classifier + MOV/MP4/JPG specials, out-of-bounds invalidation; scan_candidate wiring + detach_subtree; ScanStats::lost_content_rejected; 15 tests + fail-before; findings/FIXED/M-035) |
| lost-partition | A30 | ~~M-017~~ **FIXED (s19)** — the og's content model end to end: weighted count + bonus + 500 table-match (polarity inversion repaired), ln/ln permille, containment-drop, stable sort, the iVar16 size-match promotion re-decoded; findings/FIXED/M-017 |
| carve | B1/X47/X51 | **I-009** (byte-stepped probing = detection superset; block-shape constants) |
| carve | B2-adj/R54/R57 | **M-021** (video-repair pre-scan hook absent) |
| carve | B1/B2/C19/A28/Q65/P46 | **S-017 — FIXED (session 3)** (ftyp-family magics re-keyed on the box TYPE; F8 self-probe guards the unreachable-magic class; naked-moov salvage mirrors the og MovAnalyzer conservatively, tried last) |
| carve | B2/A28/B3 | **S-018 — INVALID (session 3)** (premise refuted first-hand: the og's modern JPEG analyser IS EOI-gated — 0xD9 appears as Ghidra's signed char `-0x27`, 5 sites; `uVar21 == 0 -> epilogue`; the audit's literal `0xd9` grep missed them; residual = legacy fallback sub-analysers + streamed continuation -> OQ-S3-JPEG) | carve | D13/B10 | ~~N-027~~ **FIXED (s28, mixed)** — naming split closed (one File%06u family, both layers through the namer chain); N-014a/c/d verified FIXED-ALREADY (N-016/M-024/N-008+N-068); findings/FIXED/N-027 |
| session | B10/D12 | **N-029 FIXED s17** (N-016a: maximal-subpart policy, DFX12 lane; N-016b: raw name bytes persisted [nflags bit 32] + og-parity restore naming from raw; findings/FIXED/) |
| health | F61/B1 | **S-014** (BitLocker auto-unlock: wrong live registry path, offline-hive path absent, unassociated enumeration — F61 was never scored before) |
| orchestrator | O38/P42/R56 | **A-004 — FIXED (session 3)** (ErrorRange fired through the config sink while the guard checked the live sink — delivered via the live sink now) + **S-016 — FIXED (session 3)** (snapshot error map was fed only AFTER the boundary emitters — the ErrorRange stream never fired; tel_counts_ now feeds it before every emit) |
| orchestrator/wide | B1/D16 | ~~M-018~~ **FIXED (s33)** — the FilterFolder pass: generated data (byte-exact corpus ini via scripts/gen_filterfolder.py, the file's `._.DS_Store`/`新建資料夾` over the finding's typos), NODE_FLAG_JUNK_FILTERED at post-walk pre-grading, query/CLI-tree/recover-all gates, junk_filtered stat, D-82 policy register — adjacent minors **N-016 + N-017 FIXED (minors session 2026-10-26)** (teardown double-emit removed, count pinned; CancelPoolWatcher factored + wired at the candidate fan-out — D-54) |
| visualization | (S-009 remainder) | disk-map: engine bitmaps unwired (`kCellAllocFree` unreachable), no CRSF record, snapshot path no error map, 2 acceptance tests missing, §7 doc drift |
| cross-cutting | Q68 (corpus docs) | **I-011** (8 corpus artifact claims corrected against the decompile; see CORPUS-ERRATA.md addendum) |
| cross-cutting | Q68/Q46 | **I-010** (scope exclusions + supersets registered: cloud/NAS/repair/WinPE/installer/licensing/i18n; ZFS label-field superset; ext4 primary-SB-only = og parity) |

**Retracted by this leg's own re-verification (recorded, not filed):** the exFAT
ChecksumDBR "no-skip" candidate — the og's fixer skips {106,107,112} via a
`std::find == end()` gate over the −1-terminated table at `DAT_180079a40`; the core's
spec-skip is a MATCH (the parallel leg's binary extraction was right; this leg's first
reading was wrong — corrected in `evidence/decompiled-quotes/fat-exfat-session2.md`).

# S3-parallel-leg cells (session 3, second continuation leg — wave 1 + residues; core `34b3441`)

| Module | Parameter(s) | Verdict change (evidence: finding file + `evidence/decompiled-quotes/s3-*` / `evidence/executions/s3-*`) |
|---|---|---|
| carve | C19/B1/B2 | **S-017 — FIXED (`1cc08c3`, post-wave-2)** (ftyp re-keyed on box TYPE + F8 structural self-probe; 6 fail-before tests) |
| carve | A28/B2 | **S-018 — INVALID (`2adfa29`, post-wave-2; refutation independently re-verified by the S3 lead: the `-0x27` signed-char FF D9 sites gate the modern analyser)** — residuals in tracked-open OQ-S3-jpeg-fallback-and-continuation |
| carve | A42/A45 | ****M-024 FIXED (s10: disabled_formats surface — findings/FIXED/M-024)**** (no runtime format-filter surface vs og RawConfig.ini [Filter], 34 toggles (CORRECTIONS #5 — was miscounted 37; M-024 addendum), STL/VPJ default-off) + **N-033** (registry exports consumed by FS engines — no core ABI) |
| carve | P47–P53/X47/X50 | ~~M-025~~ **FIXED (s32 DFX32: the WindowView — validator probes/checksums/hashes/tail-walks/feeds served from the already-read window; MEASURED 9.88× fewer read_at calls, 2.08× fewer device bytes; the pending-object model registered as D-80, future stateful analysers port behind the M-021 seam; geometry change not taken — byte-granularity dominates, I-009 superset kept)** |
| carve | P47–P53/X47/X50 | ~~M-025~~ **FIXED (s33)** — the window-serving read context (WindowDevice): validators/checksums/hashes/tail-scans read through the window buffer (the og's already-in-buffer model); MEASURED 52→4 device reads on the 4-plant fixture (13x, zero per-candidate re-reads); geometry unchanged per the finding's bench-evidence rule; stateless-vs-pending registered D-81; findings/FIXED/M-025 |
| carve | A25/A28/C19 | ~~N-031~~ **FIXED (s28)** — the zero-sector fast-skip landed (all-zero 47-byte sign window skips the raw-sign probe, counted in carve_zero_strides); stride superset + only_common coverage documented; addendum's chain semantics already equivalent (pins green); findings/FIXED/N-031 |
| lost-partition | A30/B1 | **S-019** (BitLocker "-FVE-FS-" probe family missing — S-006 under-listed the og; distinct from S-014's unlock-path finding) |
| lost-partition | A30 | **M-017 addendum** (RTTI proof: *CalcPartition = geometry calculators, slot 4 = SPC; full AdjustPartitionInfoOrder: containment drop, FS-grouped dedup, 0.1/0.3 tie rules, ±16 MiB promotion, cap 2/4/5/6, stable merge sort) |
| lost-partition | C20/B1 | **N-034 — FIXED (minors session, 2026-10-26)** (full og deep-scan ladder `fat::lost_boot_flavor` + engine-open gates `boot_passes_open_gates`; SPC sub-claim corrected as false positive — u8 domain; see findings/FIXED/N-034) |
| lost-partition | A29/C20 | **N-035 (S2) FIXED s22** (data-area-anchored containment/overlap extents on Candidate — the og's DataAreaStart/totalSec pair; fail-before executed) |
| refs | B2/B7/A25 | **S-020 — FIXED (s4)** (resident materialization e490/ea30 pointer-form + all-rows walk + paged chase cf30 + granularity e020; 6 fail-before tests; Resolution in findings/FIXED/S-020) |
| refs | B4/B5 | **M-026 — FIXED (s4)** (stale-MSB-page sweep landed: sig + vol-sig + live-exclusion + force-deleted dispatch; row-flag/ID2 superset preserved; both deleted models active) |
| refs | B7/A29-adj | **M-027 — FIXED (s4)** (placement-signature identity keep-one + live-over-deleted; $R*/S-*/SVI post-main recycle flush; $I evidence read + $R remap) |
| refs | C20/A27 | **N-036 PARTIALLY FIXED (s4)** (recognizer half: 0x40+refsize accepted, 0x20 physical, block-0 skip; container-map population open pending real-volume probe) |
| refs | B3/A27 | **N-037 — FIXED (s4, folded into S-020)** (granularity discard vs allocated size; premise corrected on the :10213 re-read) |
| refs | F-gate/B1 | **I-013** (host-OS runtime gate: og refuses < Win8 hosts; core host-agnostic — deliberate portability superset) |
| refs | (convergence) | I-012 named-streams parity confirmed convergent; C21/A26/X51/B10 newly-M (see modules/refs.md S3 rows) |
| hfsplus | B4/B5/B2/A25 | **S-021 — FIXED (s5)** (`journal_harvest_pass`: ring walk + differential + deleted-tier emission; D-56; the tx-magic derivation stays OQ) |
| hfsplus | B2/A26 (extents-overflow decode) | **A-007 — FIXED (closing session; twice-renumbered A-005→A-006→A-007 after the rawfixer-4Kn collision)** (descriptors at key+12, TN1150-verbatim — the 12-byte key includes the keyLength field; both parse sites (live walk + S-021 journal branch) were off by two and silently dropped fragmented tails; first populated overflow fixture) |
| hfsplus | B4/B5/B2/A25 | **S-021 — FIXED (fix session, 2026-10-26)** (journal deleted-record harvest: JIB/jhdr gates, ring walk with the u16 header test, og node-image validator, fork classification by volume ranges, catalog differential + extents side map; field-map corrections in evidence/decompiled-quotes/s3-hfsplus-inventory.md §3a; **discovered ACUTE A-005** — the B-tree record-span inversion — fixed in the same commit) |
| hfsplus | B2/B7 | **N-038 — FIXED; field-name residual RESOLVED (S42)**: the byte +0x29 IS `ownerFlags` (TN1150 byte-exact via the ushort-units decode — all ten branch reads mapped; evidence s42-hfs-forkfield-resolution.md §1); the predicate and fork switch unchanged |
| ext4 | B1/B4/A27/Q51 | **I-014** (deleted-inode sweep superset; fragmented-journal-walk correctness advantage — og physical+1 is wrong, core extent-map correct; X51 MATCH; "do not replicate" registrations) |
| hfsplus | B1/B7 (rows 4-6) | **I-014** (allocation-bitmap grading probable superset — UNVERIFIED residual, sharpened S42: no bitmap-test idiom found, the next probe is the allocation-object consumer walk, evidence s42 §4; hard links/hot files MATCH-by-omission; decmpfs modes 1/2 MATCH-by-omission; the completion-site residual itself CLOSED core-superset S42) |
| ntfs | B4/B5/A26 | **S-022 FIXED (diff-fix session — INDX/$INDEX_ROOT harvest landed; findings/FIXED/S-022-*.md)** (orphaned-record sweep: INDX/MFT records outside live $MFT + $INDEX_ROOT entry harvest — second-source names for files with dead MFT records; core has NO INDX decode) |
| ntfs | B4/C19 | **N-039** ($UsnJrnl harvest gate: og reason & 0x200 = COMPRESSION_CHANGE, harvest-all flag dead code; core true FILE_DELETE — core correct side; bounds deltas both ways; dead-agent probe disposition) |
| ntfs | B4/B5/A26 | **S-022** (orphaned-record sweep: INDX/MFT records outside live $MFT + $INDEX_ROOT entry harvest — second-source names for files with dead MFT records; core has NO INDX decode) |
| ntfs | B4/C19 | **N-039 — FIXED (minors session, 2026-10-26)** (0x20000 gate kept — og's 0x200 registered as an og defect in D-52; >64 MiB $J mines the TAIL now; fixture comment corrected; 3 pin tests + tail test — see findings/FIXED/N-039) |
| ntfs | B7/C19 (remainder) | MATCH-by-omission rows ($EA/$EA_INFORMATION/$VOLUME_*/$LOGGED_UTILITY_STREAM ignored by both — lead-verified by targeted greps); compression engines MATCH (both LZNT1-only; LZX/XPRESS = shared limitation via sparse+WOF path) — see modules/ntfs.md S3-NTFS2 rows |
| cross-cutting | Q68 | errata **E22-E28** (registry census corrections: CEPS-not-dBase :1985, OneNote-not-LZH :2156-2177, CRW/PST/DBX/HLP attributions, 35/33 key count) + **S-007 correction addendum** (fix plan's DBF validator dropped; OneNote magic + list-layer LZH replace it) |

*(Renumber map from the pre-merge wave-1 IDs: S-012→S-017, S-013→S-018, S-014→S-019,
S-015→S-020, M-011→M-024, M-012→M-025, M-015→M-026, M-016→M-027, N-025→N-033,
N-027→N-034, N-028→N-035, N-030→N-036, N-031→N-037, I-011→I-013; merged as
addenda: dispatch→N-031, video-feed→M-021, scoring/merge→M-017, resident-bodies→M-022,
byte-pin→I-009, probe-row→I-010; dropped as dups: boot-geometry→N-013b, byte-granularity→I-009.)*

## Session-5 detail rows (2026-10-26, audited `7cf6754`)

| Module | Parameters | Finding |
|---|---|---|
| rawfixer | F60/Q67/Q65/A25 | **A-005 — ACUTE (S5)** (4Kn stack-buffer overflow: `u8 x[512]` buffers receive `sector`-byte reads at cr_rawfixer.cpp:558/574/590/639; transport reports 4096 on 4Kn; tests pin 512 — FIX-3 fixed offsets, not buffers) |
| carve+session+query | B4/D11/D12/B1 | **S-031 (S5)** (carved files never marked deleted: og FLAGS=0x0020 constant on all 12,147 sampled records; core's deleted_only filter and deleted_count exclude the entire carve set) |
| carve | B1/B2/D11/A27/A28 | **S-032 (S5)** (ZIP/RAR inner-entry inventories: og returns the complete inner file inventory + multi-part RAR stitching; core emits one flat node) |
| ldm+rawfixer | C20/B2/A25/F60 | ~~M-046 (S5)~~ **FIXED (s10)** — GPT path geometry-correct ×3; exFAT plans bps-sized; LDM 512-LBA unit family fixed; 3 fail-before tests; findings/FIXED/M-046 |
| session+tree | D12/D11/B8 | ~~**M-047 (S5)**~~ **M-047 FIXED (s9)** (FIXED/M-047: recursive refresh_stats + the persisted-image trust on load; partial loads recompute) |
| carve | B1/A28/D16 | **M-048 (S5)** (EUUserData user-extensible signature DB — 111 extensions, `"offset,hex"` signs + MIME — has no core counterpart) |
| err+abi | C23/D14/F35/Q46 | ~~**M-049 (S5)**~~ **M-049 FIXED (s24 DFX24)** (FIXED/M-049: session-11 header truth + err_text; session-24 closes the registered step 2 — the engine-side error-context subset on the error-range event family: err_module/err_operation/err_attempts + CR_ERRMOD_*/CR_ERROP_*, per-range attribution recorded at failure time via the cr::errmod thread stamp + device-owner fallback; 8 fail-before-proven tests; ABI pins; D-68 updated) |
| session+tree | D11/D12/B4 | **M-050 (S5)** (node-model round-trip gaps: ~15 og wrapper fields — hidden-root bit, grading bits 5-7, roles, fixup, sentinels — have no core carrier) |
| carve+session | D11/D12/B9 | **M-051 (S5)** (RAW provenance unpersisted: timestamp string, analyser checksum w/ 0xFFFFFFFF invalid marker, note, tail anchor, boolean) |
| carve | A28/B3/C19 | ~~N-051 (S5) MKV DocType discrimination + element-bounds gate absent~~ **FIXED (S12)** — FIXED via the parallel lane's b2f48c6 (canonical): bounded-buffer DocType walk (webm/matroska via 0x6D626577), 0x200 element-bounds gate; DFX12 convergence addendum: 0x67805318 = Segment ID 0x18538067 LE + independent fail-before run (8/10 RED pre-fix) |
| carve | B10/A28 | ~~N-052 (S5) HEIC brand relabel to L"HEIF" absent~~ **FIXED (S12)** — FIXED via b2f48c6 (canonical): FMT_HEIF registry row (append-only) — heic-proper keeps HEIC, siblings heix/mif1/msf1 relabel HEIF; DFX12's alternative one-type-id carrier preserved in history |
| carve | C19/B1/B10 | ~~N-053 (S5) IIU magic: og binds TIFF/EP chain, core binds RW2~~ **FIXED (S12)** — FIXED via b2f48c6 (canonical): FMT_TIFF_EP claims the IIU key with validate_tiff; FMT_RW2 = post-reject IIU-salvage (SUPERSET D-63a); label-by-grammar; convergent with DFX12's same resolution |
| carve | C19/B1/B3 | ~~N-054 (S5) RAF 4-byte-vs-16-byte strictness (miss) + JPEG 4-dword-vs-prefix (superset)~~ **FIXED (S12)** — FIXED via b2f48c6 (canonical): RAF keyed on the FUJI 4CC, 16-byte string PRIMARY + variant path (conf 700); JPEG 3-byte prefix REGISTERED (D-64a, og's 4 dwords documented); + N-076 (DFX12): the ASCII-version gate fix their landing still lacked |
| health | B1/D11/Q68 | N-055 (S5) SMART thresholds (0xD1) never read — modules/health.md "presentation-layer" disposition corrected |
| carve | B1/A28 | N-076 (S12 NEW-FINDING, discovered during N-054 verification) — validate_raf's version gate rejected every real-world RAF (ASCII version "0100"/"0201" at +16 misread as binary, LE value ≫ 0x2000 cap); the FMT_RAF row was dead for real files since introduction (zero fixture coverage). **FIXED in the same change** — ASCII-digits OR small-binary acceptance; Carve.RafRealAsciiVersionAccepted fail-before executed |
| writer | F62/Q65/A35 | **N-056 FIXED s17** (WinDestIo .part-N temp leak on failed rename — commit deletes + close gates on the rename state; UNC contracts tested; findings/FIXED/) |
| table+visualization | Q65/A54/Q69 | N-057 (S5) TableLayout::Ebr dead + diskmap overlay/budget test debt |
| session | A35/D12/F35 | N-058 (S5) load validation deltas (forward parent re-parent, bad-marker light, short-count OK, name-length wrap) + CRC/resync superset register |
| session+btrfs+zfs | D11/D12 | N-059 (S5) btrfs secondary ids/generation + zfs secondary flags unpersisted |
| session+orchestrator | F32/F38/F41 | ~~N-060~~ **FIXED (s18 DFX18: per-node SerializeProgress hook + coalesced CR_EVT_SAVE_PROGRESS + the SAVE-SCOPED cr_ui_scan_save_cancel (the og's IsCancelled is the save object's state — save-on-scan-cancel intact) + atomic temp-then-rename saves; fail-before by construction; suite 716/716)** |
| orchestration+session | F35/Q46/D12 | ~~N-061~~ **FIXED (s18 DFX18: run-log = the N-048 seam + the CLI --log sink with the qa-logs semantic set at the core; auto-naming = session::auto_session_name_at, byte-exact vs the corpus sample; Session.AutoNamingConvention fail-before by construction; suite 733/733)** |
| orchestration | F35/P49 | ~~N-062~~ **FIXED (s18 DFX18: check_memory_pressure on the seam + the shim sampler on the 5 s watchdog cadence + memory_warn_mb (0 = OFF the conservative default); never scheduling-coupled)** |
| carve | B10/A28/Q68 | **N-063 FIXED s17** (default_namer "File%06u" was a core invention — now the registered deliberate default D-87; MP4/MOV per-analyser template decoded + replicated quirks; embedded_name carrier; findings/FIXED/) |
| health/bitlocker | F61/B1 | MATCH (N-064 FIXED s31: cr::dict + unlock_with_dictionary + --bitlocker-dict + cr_ui export; consent-gated, manage-bde plumbing, Office side out-of-scope per I-010) |
| health/bitlocker | F61/B1 | ~~N-064~~ **FIXED (s34 DFX34: pure cr/pwddict module — og-exact Dict format parse (comma-terminated CRLF, byte-verified first-hand), 20-entry authored seed = REGISTERED DIVERGENCE D-78 (the og's 228-entry asset never copied; loader is format-compatible), consent-gated sequencer (dedup, caps, attempt-log progress+length only) + win try_password_unlock (same unlock() plumbing as a typed password, no in-process crypto) + CLI --bitlocker-pwddict <path\|seed> explicit-consent action; scan path holds no reference (F61 preserved); 9 Pwddict tests + executed fail-before RED; suite 752/752; findings/FIXED/)** |
| cross-cutting | B1/F59/F61/F62/Q46 | I-021 (S5) scope/deliberate-delta registers (og always-elevated manifests, EuSendMessage installer-only, repair toggle, CRecoveryFilter layering, PENetwork SMB, DRWCfg.bin) |
| session/carve/btrfs/zfs | D12/A27/Q67 | I-022 (S5) core-superset registers (CRC32/resync/bomb-caps, structured btrfs extents, decoded zfs runlists, UTF-16 unification, extra props, core-only formats) |
## Session-6 verdict changes (deep continuation, 2026-10-26, audited `022c81d`-lineage / corpus `03ad46c`)

The never-cited-artifact lanes executed (session-5's A-H lanes, defined at `2001f7c` but never
run, were this session's scope): the 6 RAWSearchDLL carving-family artifacts + registry
census/dual-registry; the SaveScan record-4/IFileProperties-part3 grammar; the per-engine
function map; the MOM/WinPE/config/UIUX-wiring 16-artifact set; and the core's 8 never-cited
files. Every load-bearing citation verified first-hand before filing (three stale agent claims
filtered by verification: FAT alternate-EOC — N-002's fix is live; NTFS BAAD — MATCH
not-BAAD both sides; FAT quick-scan threading — already registered D-30x).

| Module | Parameter(s) | Verdict change (evidence in the finding + `evidence/decompiled-quotes/s6-*`) |
|---|---|---|
| fat | B4/B5/D11/A29 | **M-052 — FIXED (s24 DFX24 + s35 convergent)** (recycle-bin $I/$R pairing — the shared cr/recycle model wired into the dentry walk; four-root recognition, chain-read $I parse, original-name remap + post-walk fixup; findings/FIXED/M-052) |
| fat | A10/B-naming/FB-FalsePos | **N-176 — FIXED (S46)** (the LFN illegal-char fallback: the 9-char gate `?*|\/:"<>` in lfn_assemble — a checksum-valid corrupt LFN carrying any of them now takes the 8.3 fallback exactly like a checksum failure; LfnIllegalCharFallsBackTo83 9-iteration pin + LfnValidNameUnchanged; findings/FIXED/N-176) |
| fat | A10/B-naming | **N-175 — FIXED (S46)** (the NTRes 8.3 display-case fold: short_name_to_u16 takes the nt_reserved byte and applies the og's four-way exact-value switch — 0x00/0x08/0x10/0x18 fold (strupr/strlwr pairs), anything-else raw; ASCII-only, CP437 high half untouched; LFN path unaffected by construction; NtResCaseFoldFourWay 5-path pin + LFN regression pin + the BitLocker NTRes=0 fixture re-pin; findings/FIXED/N-175) |
| exfat | B1/A25/S35 | **N-067 FIXED s11** (upcase-table validation — og shape gates + checksum32 stream check, soft stats signal; findings/FIXED/); B4/B5/D11 **M-052 FIXED (s24 DFX24 + s35 convergent)** (the FAT-mirrored pairing — bin context through walk + oracle re-walks; findings/FIXED/M-052) |
| carve | A25/D11/P42 | **S-032 (converged)** (archive inner-file inventories missing) |
| carve | B1/C19 | **N-069 FIXED s11** (CAB mscf case key — FMT_CAB_LC dual row + two-spelling validator re-check; findings/FIXED/); B1/A28 **N-068 FIXED s11** (LFH sanity gates decompile-exact; EOCD half FIXED-ALREADY — the CD anchor + EOCD reconciliation already cover it; findings/FIXED/) |
| carve | A28/A29/D15/B3 | **N-070** (overlap scoring + two-generation fallback); **S-007 addendum** (bare-MPEG dispatch gap — 18+ u16 sync keys vs ID3-only; family roster with provenance) |

| carve | B1/C19 | **N-069 FIXED s11** (CAB mscf case key — FMT_CAB_LC dual row + two-spelling validator re-check; findings/FIXED/); B1/A28 **N-068** (ZIP EOCD anchor + LFH sanity gates) |
| carve | A28/A29/D15/B3 | **N-070 (S5) FIXED s22** (overlap scoring: accumulated base+verdict ranking implemented, fail-before CR2-vs-DNG; two-generation fallback REGISTERED as D-75 — legacy bodies undecoded); **M-056 (S22 NEW-FINDING) FIXED** (TIFF-family size basis mix — multi-strip TIFFs carved at the 256 MiB cap); **S-007 addendum** (bare-MPEG dispatch gap — 18+ u16 sync keys vs ID3-only; family roster with provenance) |
| session | D11/D12/B4/B9 | **S-031 (converged)** (carved files: deleted flag + timestamp + validator payload dropped at emission; sessions round-trip the misstatement) |
| session | D12/S36 | **N-066** (identity card: firmware/WWN/interface + 512-byte boot snapshot vs model/serial/CRC32); A35/D12/F35 **N-078 FIXED s17** (truncated carve_note silent-OK exit — the c.fail-less break; filed+fixed, renumbered past the parallel lane's N-077 leak finding; findings/FIXED/) |
| session/tree | D11/D12/P42 | **M-051+N-059 (converged)** (type-registry ID + per-FS secondary payload slots missing) |
| writer | A27/S35/P46 | ~~N-065~~ **FIXED-ALREADY (s34 DFX34 verification — parallel-lane landing: collision = the og's `%s_%d`/`%s_%d%s` two-branch shape, cap 10,000, exhaustion fails the item, `_dup` removed; mkdir = the 0/0xB7 contract in SEMANTIC form as a registered divergence — the literal gate broke drive-root chains, MSVC test #227; D-19's refuted claim superseded; proof tests verified green: CollisionNameUnderscoreFormat / CollisionExhaustionFailsTheItem / MkdirSemanticsMatchShCreateDirectoryEx; findings/FIXED/)** |
| health | P46/P68 | **S-024 addendum** (cr_health.h claims 0xD1/0xD8 never issued — doc-truth sub-item; capability gap stays S-024) |
| storage | B8/P42/A54 | MATCH-with-D-84 (N-071 FIXED s31: the two-stack merge — PhysicalDrive loop + cfgmgr32 GUID_DEVINTERFACE_DISK pass, pure merge_disk_enumeration dedup; residual stacks registered) |

| storage | B8/P42/A54 | **N-071 FIXED s30** (option (a) per the finding's recommendation: PARITY-DELTAS D-80 scope registration + the busy-disk 0-access metadata retry — a share-blocked disk stays listed; findings/FIXED/N-071) |
| storage | P47/P48 | ~~N-045~~ **MATCH (registered divergence)** (FIXED/N-045, session 18: 8 MiB/256 KiB reactive-LRU vs og 4 MiB prefetch engine REGISTERED as PARITY-DELTAS D-72 — budget + algorithm, corpus cites re-verified first-hand; doc-truth pin `Cached.ScanCacheBudgetIsTheRegisteredDivergence` keeps source constant + D-72 row + cr_cached.h divergence block in lockstep; no code change, the alignbench sweep owns the tuning decision) |
| storage | S35/Q43 | ~~N-046~~ **MATCH** (FIXED/N-046, session 18: `PartitionSlice::refresh_geometry()` — the og's RefreshSectorSize-from-parent surface (FUN_140643870) with revalidation hardening over the og's blind store; Slice.GeometryRefreshAfterMediaChange + Slice.GeometryRefreshRefusesInvalidatingChange; orchestrator auto-refresh excluded and documented — the corpus exposes no scan-path caller) |
| ext4 | A27/S35/B2 | ~~N-073~~ **FIXED-ALREADY** (FIXED/N-073, session 14: N-004's transport-level CatchBuffer landing covers the engine path — 3 engine-path pin tests; the S35 row in modules/ext4.md carries both N-004 and N-073 closed) |
| btrfs | P47/P48/S36 | **N-072 FIXED s34 DFX34** (the re-scan metadata cache: pure codec + engine wiring, OFF by default, caller-supplied dir, fsid+generation strict gate, warm scan skips the chunk derivation — nodes_read strictly lower; the og's undecoded format replaced by OUR registered one; findings/FIXED/); **OQ-S6-2** (RC4 PRGA target) |
| zfs | A25/S35 | **N-072 FIXED s34 DFX34** (the delay-execution label retry: bounded 250/500/1000 ms backoff, cancel-aware in 50 ms slices, live handles only via the new is_static_media discriminator — images never retry; label_retries counted; findings/FIXED/); INFO-register (lzjb/zle/gzip decode — no divergence claimable, evidence ledger only) |
| lost-partition | A30/A37/D12 | **I-024** (PartitionRecoveryD generator-sets + .dat persistence register) + **OQ-S6-1** (decode request); **M-017 addendum** (CalcPartition class evidence) |
| orchestrator | A38/A41/P42/A44 | ~~N-075~~ **MATCH** (FIXED/N-075, session 28: `cr_ui_scan_busy`/`cr_ui_scan_stopping` synchronous query exports — the og's sigIsBusy(bool&) x14 / sigIsStopping(bool&) x8 reference-out signals; pull-progress convention REGISTERED as D-78; typed IUIProgressInfo mapping documented in ABI-CONTRACT 5; pinned by `CrUi.ScanBusyStoppingQueries` + the plain-C edges); **I-023** (DRWCfg scope register) |
| visualization | P42/D-model | **S-009 addendum** (stale premise corrected — surface landed; residual: kCellAllocFree dead, VolumeBitmap discarded) |
| query | P42 (sort Type) — **MATCH** | M-029 addendum (path-group CLOSED session-6) + session-11: Type sort CLOSED (SortKey::Type, ABI code 7, iconIndex order) + the modified-unit doc lie fixed (FILETIME, pinned) + paging documented as the deliberate difference (no og page-size constant) + **session-32: the bucket-ID translation + general-raw naming CLOSED (query::apply_date_bucket/apply_size_bucket from the DRWUI FUN_140705c50/FUN_140706060/FUN_140254670/FUN_140255820/FUN_140256830 decode; cr_ui_query_apply_buckets added export; ScanConfig::raw_category_name) — the finding's every sub-item is now closed (FIXED/)** |
| abi (cr_ui) | P42 | ~~N-033~~ **MATCH** (FIXED/N-033: ABI sub-item landed s6 (cr_ui_carve_format_count/info) + the CLI sub-item closed s28 (`rrecov list-carve-formats`, FB-K01 exit discipline) + the per-scan refresh divergence REGISTERED as D-80; pinned by CarveFormatRegistryEnumeration + the parse gate) |
| quality (cross-cutting) | P46/P68 | ~~N-074~~ **FIXED (s18 DFX18 canonical `479b0c7` + convergent s22 DFX22 landing reconciled in the same-window merge — both lanes verified all five sub-items first-hand independently; pin Health.SmartCommandListMatchesImplementation fail-before-proven)**; **~~N-019~~ FIXED (s28: the 13-scenario Q65 gap — 4 new mutation-proven tests + 6 already-covered re-cited + 1 superseded by M-017's scoring + OQ-T6 pushed forward with new snapshot evidence; see FIXED/N-019)** |

New tracked-open: **OQ-S6-1** (PartitionRecoveryD generator-set semantics), **OQ-S6-2**
(Btrfs RC4 target), **OQ-S6-3** (CRAW +0xF4/anchor-B/checksum consumption) — 14 tracked-open
files total. Session-6 verified-fixed addenda: M-029 (partial), N-033 (partial), N-045 (cite),
M-017/M-030/S-007/S-009/S-024 (evidence sharpened).

## Session-9 detail rows (2026-10-26 — the diff-fix session-9 baseline-regression filing)

| Module | Parameter(s) | Verdict change |
|---|---|---|
| fat (test infrastructure / CI gate) | M-015 landing coverage | **M-053 — FIXED-ALREADY (s9; the repair itself is the parallel DFX7 lane's `b1f9210` inside N-018's defect set — this session's filing is the formal record: root-caused independently via worktree bisect at the regression point (parent green / landing SEGV), the formal finding filed per the discovered-divergence SOP after the parallel lane fixed the bytes without a finding. Engine code untouched (og-parity validator correct). `evidence/executions/s9-m053-abi-fixture-regression.md`) |

## Session-20 detail rows (2026-10-26 — DFX20, the unclaimed-MAJOR lane)

| Module | Parameter(s) | Verdict change |
|---|---|---|
| storage | S61 (BitLocker placement) | **M-033 FIXED (DFX20)** — the in-stack XTS-AES decrypting device now exists (`DecryptingDevice : IStorageDevice`, cr_decrypt_device.h; AES-128/256+XTS clean-room in cr_aes.cpp, pinned byte-exact against pycryptodome AND OpenSSL EVP 3.5 — the og's own cipher interface — plus FIPS-197 KATs; per-sector LE-index tweaks; the og's layer order physical -> slice -> [XTS] -> engine). Key surface = the og's GetFullVolumeEncryptionKey output shape: `--bitlocker-fvek` CLI + `cr_ui_open_volume_with_key` (additive ABI export, no version bump). Locked-without-key refusal PRESERVED (S61/F13; FB-B01). The recovery-password/BEK -> VMK -> FVEK unwrap chain stays out-of-corpus — tracked-open OQ-S20-1 with the decompile entry points (ParserBitLocker :27633, FUN_180042350/FUN_180044e90). 13 tests; suite 608/608. |
| abi (cr_ui) | P42 (export set) | **M-033 additive export** — `cr_ui_open_volume_with_key` (keyed locked-volume open, the in-stack decrypt opt-in); ABI policy: additive = compatible, no version bump; C-consumer check compiled clean against the new declaration. |
| session | D12/S36/P34/P35 | **M-042 FIXED (DFX20)** - resume re-validation landed: lost::revalidate_candidate at load (revalidate_disk) AND run() start (whole-disk-basis resumes); stale records DROPPED (PRD:20748-20842 semantics). REC_PARTITION_CONTEXT v2 append-only (bps/SPC/hidden/flags - the og's CSearchedPartitionInfo payload); REC_DISK_LAYOUT type 8 (partinfo equivalent - layout_changed surfaced at resume, never enforced); per-hit checkpoint forced when discovery produces candidates (sink-gated). Executed fail-before; 6 tests; suite 685/685. |
| lost-partition | A30/B1/S35 | **M-042 FIXED (DFX20)** - the discovery probes capture per-record geometry (probe_ntfs/probe_fat/probe_exfat fill bps/SPC/hidden); revalidate_candidate re-runs the SAME per-family probe for resume truth. |
| cli | C24/grammar | **M-033** — `scan --volume X: --bitlocker-fvek <hex>` (64/128 hex chars, pre-pass so option order is free); locked refusal message now points at the new path. |

## Session-25 detail rows (2026-10-26 — DFX25, the INFO+tracked-open lane)


| Module | Parameter(s) | Verdict change |
|---|---|---|
| zfs | B1/B8 (dataset hierarchy constants) | **OQ-Z1 RESOLVED → A-008 (ACUTE, NEW-FINDING) FIXED** — child_dir_zapobj was read from dsl_dir bonus+24 (the dd_origin_obj slot; kernel +32 per OpenZFS master dsl_dir.h fetched verbatim + the og's own decode: 256B dsl_dir copy / 320B dsl_dataset copy, head hop @+8, ds_bp@+128, child zap walk skipping $FREE/$ORIGIN). Normal pools: NO child dataset ever enumerated (every zfs create silently missing); clones: origin followed as a child zap (ghost datasets — proven in the executed fail-before). Fail-before RED 3/3 (EndToEndWalk, ChildDirZapReadsKernelOffset32, OriginObjNotTreatedAsChildZap); pass-after 16/16 Zfs; suite 739/739. Fixture de-self-referenced (kernel layout). findings/FIXED/A-008. |
| (info tier) | all | **22/22 acknowledged INFORMATIONAL findings moved INFORMATIONAL/ → FIXED/** (resolutions verified on `716d121`, pins green; two first-hand spot-checks re-executed). No verdict changes — `ack` cells now denote filed-in-FIXED. |

## Dev-sign session detail rows (2026-10-27/28, SA lane — driver-install/dev-sign module; corpus: 02-kernel-driver-architecture.md §5 + regdrv.reg)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| driver-install · installer executability | **MATCH (FIXED)** | **S-036** RESOLVED (upstream v0.2.3 + composite fail-loud flip + FloorVersion v0.2.3) |
| driver-install · service group | **MATCH (FIXED)** | **M-060** RESOLVED (fallback = install.ps1 twin, opt-in only; sc qc + Parameters assertions on both paths) |
| driver-install · Parameters registry (IoctlBase/MaxDisks/DebugLevel/SessionsEnabled) | **MATCH (FIXED)** | **M-060** RESOLVED (fallback = install.ps1 twin, opt-in only; sc qc + Parameters assertions on both paths) |
| driver-install · ImagePath form | **MATCH (FIXED)** | **M-060** RESOLVED (fallback = install.ps1 twin, opt-in only; sc qc + Parameters assertions on both paths) |
| driver-install · copy destination | MATCH | install.ps1:50-54/composite → `System32\drivers\<svc>.sys`; corpus §5.1 `%SystemRoot%\System32\Drivers\%s.sys` |
| driver-install · service shape (Type/Start/ErrorControl) | MATCH | `type= kernel start= demand error= normal` = regdrv.reg Type=1/Start=3/ErrorControl=1 (both paths) |
| driver-install · install-error semantics (start failure) | **MATCH (FIXED)** | **M-062** RESOLVED (start failure fatal exit 6 + RUNNING confirm exit 7) |
| driver-uninstall · teardown behavior | **MATCH (FIXED)** | **M-061** RESOLVED (uninstall.ps1 is the CI teardown, executed + post-state asserted) |
| dev-sign · environment matrix / HVCI constraint | **MATCH (FIXED)** | **M-063** RESOLVED (environments contract + windows-2025 matrix leg + release-body truth pass) |
| dev-sign · signature timestamp | **MATCH (FIXED)** | **N-084** RESOLVED (the /tr rationale documented + timestamp_server composite input) |
| dev-sign · installer interaction model | **MATCH (FIXED)** | **N-085** RESOLVED (operator-posture note; absorbed by S-038's packaging milestone) |
| dev-sign · signing posture (release artifact state) | DIVERGENCE-INFORMATIONAL | **I-026** — unsigned-by-design + consumption-time sign (execution-proven); og: vendor Authenticode (corpus :19) |
| dev-sign · load/round-trip behavior | MATCH | run 40549660074: RUNNING + control-device open + GET_INFO/GET_VERSION ready + grammar-path refusal class — the verified core of Phase 1 |

(No prior module file exists for this surface — a `modules/driver-install.md` lands with the Phase-2 wave that owns the packaging surfaces.)

## Dev-sign session detail rows — waves 1-2 (2026-10-28, audited `dd82db9`; corpus: the never-cited family/part1 artifacts)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| carve · ASF/WMV/WMA end-of-file determination (A28/B2) | MATCH (FIXED) | **A-010 FIXED** — the object walker (5 known top-level GUIDs + size gates + torn-tail device bound) walks to grammar break per the og's BruteForce contract (deep-dive:87-91); 4 fail-before tests in test_carve.cpp |
| carve · FH11 + CPT7/8/9 format coverage | **MATCH (FIXED)** | **S-037** RESOLVED — validate_fh11 (the og's FUN_180029f30 predicate) + validate_cpt (the FUN_180043630 gate + FUN_1800436e0 pointer-chase); 4 appended ids; fail-before-proven |
| carve · CPT format presence, 3 generations (A28/C19) | **MATCH (FIXED)** | **S-037** RESOLVED — three duplicate-key rows (the og's registry policy) sharing validate_cpt |
| carve · 131 family classes — presence/semantics vs the 6 family artifacts | 34 MATCH · 3 registered-delta · 64 S-007-remainder · 26 N/A · 1 UNDETERMINED | the wave's class-by-class table (agent sheet, scratch); residuals → **OQ-SA-2** |
| carve · signature-table packaging (R56/Q68) | **MATCH (REGISTERED DELIBERATE — D-103)** | **N-086** RESOLVED — the compiled-in posture registered as PARITY-DELTAS D-103 (option (a); content-identical, the usersigns surface covers runtime extension) | |
| session · FAT 8.3 name encoding (B10/D11) | **MATCH (FIXED)** | **N-087** RESOLVED — the full CP437 boundary decode (standard-verified table); 3 tests fail-before-proven | |
| session · IFileProperties 23-slot interface | 16 MATCH · 1 registered-residual · 1 UNDETERMINED · 5 N/A | the +0x20 getter = the N-050/N-059 type-registry residual (registered); +0x30 → OQ-SA-1 |
| session · NTFS/FAT/exFAT persisted fields | 28 of 32 MATCH (10/12, 9/10, 8/10) | carriers richer in places (4-state deleted, 4th timestamp, allocated_size); the 4 unmatched → **OQ-SA-1** (anchor datums + ids) |
| carve · RawConfig [Filter] count (doc truth) | CORRECTION PROPAGATED | CORRECTIONS #5: 34 not 37 — M-024 addendum; matrix/SUMMARY/evidence notes updated this session |

## Dev-sign session detail rows — waves 3-7 (2026-10-28, audited `dd82db9`; corpus: the Installer/ + config-files + UIUX/10 + pe-payload + MOM-stub materials)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| app-installer · K74 silent surface (13 params) | DIVERGENCE-SEVERE | **S-038** — Inno /SILENT//VERYSILENT//SUPPRESSMSGBOXES//NORESTART, ARP QuietUninstallString, unins%03d, admin-required — all absent (string tables verified) |
| app-installer · K77 registry + telemetry contract | DIVERGENCE-SEVERE | **S-038** — SetupUE 16-entry OS gate, `SOFTWARE\EASEUS\DRW\Version`, /SendInfo//SendInfoEx//UninstallStart//UninstallEnd |
| driver-install · K71/K72/K76 versioned/verified deployer contract | **MATCH (FIXED)** | **M-064** RESOLVED (v0.2.3 `6b50bb8` + v0.2.4 `323e75a`): write+verify + per-call SCM errors + the exit-code contract + the %s%d versioned coexistence (in-place upgrades; an in-use image never overwritten) + the -Uninstall verb + the versioned-aware teardown + the CI PS parse gate |
| carve · user-sign caps vs the og's own DB | **MATCH (FIXED)** | **S-039** RESOLVED — caps raised to the measured maxima (48/12/1024/80); the og's own DB loads 111/111; fail-before-proven (ParseOgShippedExtremes/HostileBlobStillAtomic/RegistrationRoundTripProceedsToScan); the ABI widths half converged as **M-067** (below) |
| carve · the user-sign ABI widths (the S-039 consumer surface) | **MATCH (FIXED)** | **M-067** RESOLVED — `cr_ui_user_sign` resized to the v3 caps (mime[81]/bytes[48]); `CR_UI_ABI_VERSION` 2 → 3 (the first struct-FIELD change); compile-RED fail-before proven (`-Werror=array-bounds`) |
| carve · default user-sign set | **SUPERSET (FIXED, D-104)** | **M-065** RESOLVED — data/userdata-defaults.json ships (the og's 111/190 verbatim) + the engine-init load + the release asset; OQ-SA-3 RE-RESOLVED (A2 scan-engine lane: the og's scan engine has NO user-sign channel — EUUserData feeds the CHK repair chain (CHKRepairFileSign); the landed behavior is a registered deliberate superset (D-104), not og parity; the FIX-6 "scan-setup trigger config" reading of the :227538 consumer is superseded — that consumer reads the per-scan carve-statistics map) |
| preview · archive-content enumeration (FS-origin) | **MATCH (FIXED)** | **M-066** RESOLVED — Metadata.entries (name/size/compressed/crc) collected on the existing CD walk, 512-entry cap + truncated flag; 4 tests fail-before-by-construction |
| preview · doc truth (M-028 propagation) | **MATCH (FIXED)** | **N-088** RESOLVED — all three sites corrected + the PreviewDocs grep guard |
| packaging · WinPE payload census (K78) | DISPOSITIONED-MATCH-at-plan | the PE capability boundary recorded as the boot-media builder's design input; roadmap.md:71 |
| MOM stubs · 17 never-cited per-module censuses | MATCH (zero new parameters) | wave-7 spot-check: 13 ROLLUP-COVERED + 4 INFRASTRUCTURE-ONLY; decode candidates noted (CWindowsFS ×3, DeviceManager offline-OS strings) |
| inventory truth · citation-count correction | CORRECTED | SURFACE-INVENTORY "22 of 32" → 17 of 30 (5 stubs cited in brace-group form at s5-session5-corpus-quotes.md:64) |
## Session-8 detail rows (2026-10-28, audited product `35adcb9`; corpus `be0cfe9` — the second-session deep audit)

| Surface × parameter | Verdict | Finding / evidence |
|---|---|---|
| imaging · disk→image create flow (P42/C24/F59) | DIVERGENCE-SEVERE | **S-040** — reader-only core; no `cr_ui_image_create`, no verb, no writer; the safe-scan posture's producer absent |
| carve · S-007 batch-2 remainder formats (B1/A28/C19) | DIVERGENCE-SEVERE | **S-041** — ~24 formats (CHM MNY EMF ONE MPEG-TS 3DS SWF JP2 MOBI + the text school) absent; previously unowned |
| carve · CRW key attribution + validator grammar (B1/C19/A28) | DIVERGENCE-MAJOR | **M-081** — II/MM\x1A\x00 keys are Canon CIFF per the census; landed as "Minolta MRW" rows whose PRD gate rejects genuine CRW |
| query · path_substring + state_mask axes (D16/P42) | DIVERGENCE-MAJOR | **M-072** — 5 of 7 og filter groups; no substring, no state mask (R-22) |
| orchestration · scan-time folder filter (S31/D16) | DIVERGENCE-MAJOR | **M-073** — PushFolderFilter unmirrored; post-hoc query only (R-14) |
| writer · destination error classification (C23/D14) | DIVERGENCE-MAJOR | **M-074** — too-long/invalid/network collapse to ERR_DEST_UNWRITABLE (R-41) |
| check · chkdsk launch endpoint (S35/P42) | DIVERGENCE-MAJOR | **M-075** — WinChkdsk launch absent; D-90 disposition superseded by the plan (R-65) |
| health/enum · TRIM facet in rows (P42/X48) | DIVERGENCE-MAJOR | **M-076** — datum health-facet-only; no row fields/flag (R-63) |
| preview · 100 MB/15 s product caps (X48/X49) | DIVERGENCE-MAJOR | **M-077** — no eligibility gate, no time bound in the path (R-34) |
| health · BitLocker turn-off op (F61/P42) | DIVERGENCE-MAJOR | **M-078** — CBitlockerTurnOffOperation unmirrored; unlock-only surface (R-88) |
| session/CLI · auto-save volume label (D12/B10) | **MATCH** | **M-079 FIXED** (`0b76757`, the parallel lane — fill_volume_label at scan setup) + this lane's convergence complement: the Windows-CI behavioral assertions (auto_junk-gate-fat16<stamp> / --label override), the exFAT fixture spec-repair, the resume label adoption |
| preview(thumb) · default read budget vs ordinary photos (X47/X48) | DIVERGENCE-MAJOR | **M-080** — 6 MiB 12-MP JPEG → BudgetExceeded (fast path gated on pixels only) |
| mom-capabilities · DeviceManager offline-OS identification (A54-58) | DIVERGENCE-MAJOR + disposition re-scope | **M-082** — live-module machinery (IsBootVolume chain); CheckVersion verdict covered only the dead copy |
| writer/query/CLI · M-018 junk gates (D16/B2) | **MATCH (gate restored + extended)** | **M-071 FIXED** — the rrecov pair restored byte-exact from `69507cb` + the cr_ui session/snapshot bulk walks gated (the W-lane surfaces that postdate M-018); explicit-selection stays open (the og's default-not-block semantic, pinned by test) |
| session · list scope + row shape (D12/B10/P42) | DIVERGENCE-MINOR | **N-090** — no `auto_` filter; 1 s save time; label[32]; no device-type |
| docs · PARITY-DELTAS registration gaps (Q68) | **MATCH** | **N-091 — FIXED (session 38)**: D-108/D-109/D-110 registered (the load-progress model, the thumbnail engine superset, the sanitizer superset); the auto-save posture rides D-105/D-12 |
| preview(thumb) · refusal matrix + cache (Q68/X47) | **MATCH** | **N-092 — FIXED (session 38)**: WMF/EMF/ICO named-and-refused (all five magic forms; `MetafileAndIconFamiliesAreNamedRefusals` RED→GREEN); the cache scope DECIDED and registered (no engine cache — host-side per the in-process doctrine; modules/preview.md + cr_thumbnail.h contract) |
| carve · batch-1 bounds MID/GZ (A28/C19) | **MATCH** | **N-093 — FIXED (session 38)**: all four bound pairs carried (MID 10 MB; GZ 20/400 MB; PSP 36/20 MB; SIT 20,480/200 MB) with og cites in the rows; the pin + the MID behavioral pair RED→GREEN |
| cli · `-h` usage text + stale cite (C24/Q68) | **MATCH** | **N-094 — FIXED (session 38)**: the sessions usage line landed + CORE-API-CURRENT re-pinned (3043-3063, count 11) + the verb lock grown to four sites with the `CrUi.CliHelpListsEveryDispatchedVerb` regression pin (both directions) |
| docs · five stale texts (Q68) | **MATCH** | **N-095 — FIXED (session 38, `995ba5d` — S11 repair: e1e5124 is a phantom pre-rebase SHA)**: GetProcName relabel (both sites); 34-toggles + the 1,481-unique nuance; the carve.md D-104 note; the 2-segment disk template; the ABI cites via N-117 |
| transport · identity IOCTL issuance (S35/P42) | DIVERGENCE-MINOR | **N-096** — og issues GET_DISK_CONTEXT_MODE per open; core never issues any |
| health/docs · auto.txt classification (F61/Q68) | **MATCH** | **N-097 — FIXED (session 38, `32ef33b` — S11 repair: 13245c0 is a phantom pre-rebase SHA)**: the inventory row carries the FVE-snapshot truth + the security datum; the core's no-persistence posture registered |
| abi/docs · enum refresh contract sentence (Q68) | **MATCH** | **N-098 — FIXED (session 38)**: the always-fresh contract in cr_ui.h + the CORE-API-CURRENT/API.md mirrors + R-86 flipped to PRESENT; `EnumerationRefreshContractPinned` RED→GREEN |
| config assets · data-asset ground truths (Q68/I-010) | INFORMATIONAL | **I-027** — quota contract, json_raw shield, Aliyun caps, schtasks autostart, filter_generalraw value |
| mom-capabilities · CWindowsFS live-FS adapter | OPEN QUESTION | **OQ-S8-1** — decoded; og-side consumer untraced |
| landing verification · W1a/W1b/W1c + thumbnail + carrier | MATCH at load-bearing points | worklog session-8 record (cancel-per-node; ERR_CANCELLED-never-flattened; cancelled-never-autosaves; 27/27 batch-1 magics; carrier all-parameters) |
| FIXED-truth spot-check · 10 sampled findings | 9 CONFIRMED · 1 PARTIAL | M-018 → **M-071**; the rest verified in-code (worklog record) |


## Session-8 wave-2 rows (audited `ff879f9` lineage, corpus `be0cfe9` — the continuation lane, IDs reconciled past the parallel deep-audit lane's set)

| Module · parameter (cell) | Verdict |
|---|---|
| carve · batch-2 format coverage (C19/A28/B1) | **S-041 + wave-2 addendum** — the parallel lane's owner finding gains the full decode: 7 text-school grammars, the 18-site config-gated zone, registry geometry (59+35), the BOM battery, FCPEvt end-to-end |
| carve · WMF placeable validator grammar (A28/C19) | ~~**S-042**~~ **MATCH** (fixed this session — both WMF branches re-derived from FUN_1800264d0: checksum@+20/XOR words 0..9/reserved-dword-zero/hmf/tail gates, exact Size*2+22 extent; std branch's invented gates removed; spec-derived fixtures; Resolution in findings/FIXED/S-042) |
| carve · MDB family second signature (C19/B1) | **M-084** — "Standard ACE DB" (ACCDB 0x4C) missing |
| carve · batch-1 evidence citations (C65-adj) | **N-099** — registry regions/ctor lines/byte-order corrected (truth pass; constants shipped correct except WMF; also corrects N-093's WPG-ctor cite per erratum E63) |
| carve · progressive result emission (A31/S38-adj) | **MATCH** — carve_slice streams nodes live during the pass (cr_scan.cpp:1877-88); og live-growth equivalent (UIUX/30:14) |
| preview/thumbnail · EXIF orientation (B2/A25) | ~~**S-043**~~ **MATCH** (fixed this session — the 8-case RotateFlip-then-thumbnail order for JPEG APP1/IFD0 0x112 + TIFF tag 274, before the 256-px pass; the embedded fast path's spec-derived rule registered D-105; Resolution in findings/FIXED/S-043) |
| preview/thumbnail · input eligibility (B1/P48) | **M-080 + wave-2 addendum** — the parallel lane's budget finding gains the og's 200 MB eligibility gate decode (:422428), correcting its "no input cap" claim; severity discussion recorded (SEVERE recommended) |
| preview/thumbnail · format coverage (B1) | **S-045** — video-keyframe/RAW/HEIC/PDF pipelines absent (og ships them); complements N-092's unnamed-trio + cache gaps |
| preview/thumbnail · grading side-effect (D15) | **MATCH** — M-083 FIXED (`e56baac7`, session-11B conformed; independently PARITY-VERIFIED by session-11-lead — the engine layer executed standalone, `evidence/s11-board-truth.md`; promoted at `cr_ui.cpp:3737-3742`); the residual undecoded og guard conditions tracked as OQ-S11-1 |
| preview/thumbnail · TIFF LZW (B1/A28) | **M-085** — compression-5 refused (mainstream variant) |
| preview/thumbnail · superset posture (A25) | **MATCH (I-028 RETIRED — session-42)** — the register stands as the deliberate-divergence record; its one ask (ABI-CONTRACT documentation of the both-axis + RGBA8 contract) landed with M-102 as §7i (:599-625); verified at `869da779` |
| preview/thumbnail · still-image families + bounded-read doctrine + never-upscale + honest refusals | **MATCH** |
| session · auto-save trigger gates (A31/D12) | ~~**S-044**~~ **MATCH** (fixed this session — results-found / not-import / not-already-saved gates landed on the ABI + CLI twins; the byte-flag config shape registered as D-105; findings/FIXED/S-044) |
| session · auto-save naming folder variant | **N-105** — "Folder" suffix absent (deferred; cross-ref M-079's volume-label gap) |
| session · crsf reader robustness (S35) | **MATCH** — per-chunk raw fallback == og's raw-memcpy-on-inflate-failure (md :98) |
| session · light summary read (D12) | **MATCH** — SESSION_STATE+VOLUME_INFO == the og's record-1-then-record-2 list read (phrasing corrected E64; complements N-090's scope/shape rows) |
| ntfs · session-wire field set (D11/D12) | **MATCH** | **N-100 — FIXED (session 38)**: `ntfs_total_clusters` carried as the nflags bit-9 block with the sparse-anywhere zeroing at both stamp sites + the clone; the resident-comment truth repair landed; `NtfsTotalClustersRoundTrip` + the walk-test half prove it |
| ntfs · origin-LCN stamp chain (C20) | **MATCH** (semantics) — chain cites corrected (+0x53/+0x4f first hop; FUN_180003250) |
| orchestration · device enumeration sixth source | **MATCH** | **N-106 — FIXED-ALREADY (classified session 38)**: the SI:86 disposition row with corpus cites + the N-071 sixth-source note (landed by the session-8 wave-2 lane) |
| audit-trail · SURFACE-INVENTORY arithmetic + classification rows | **MATCH** | **N-101 — FIXED (session 38, `32ef33b` — S11 repair: 13245c0 phantom)**: one live residue (per-binary 112/114) + the rest FIXED-ALREADY with reproductions |
| audit-trail · M-024 count propagation (finding file itself) | **MATCH** | **N-102 — FIXED (session 38, `32ef33b` — S11 repair: 13245c0 phantom)**: the propagation note flipped to done (every live site reads 34) |
| audit-trail · thumbnail API rows + heif attribution | **N-103 — FIXED (session 38, `e4f3184` — S11 repair: the cited 954c386 is a phantom pre-rebase SHA)**: R-28 rows engine-PRESENT + M-102 landed (`e56baac7`, retired S11); the heif.dll attribution corrected per the import tables re-read first-hand |
| audit-trail · ldm.md dangling ref + CAPABILITY-MATRIX HFS+ row | **MATCH** | **N-104 — FIXED (session 38, `32ef33b` — S11 repair: 13245c0 phantom)**: the table.md ref dropped; the HFS+ row DONE with the engine cite verified first-hand |

### Session-8 third lane (2026-10-28 — the driver lane + the merge/repair wave; audited core `35adcb9`, driver `323e75a`, corpus `be0cfe9`)

| Module · parameter (cell) | Verdict |
|---|---|
| driver-install · versioned coexistence across releases (K76) | **M-086** — FileVersion pinned 1.0.0.0 + no release stamping → the -Layout Versioned tag is always "10" → genuine upgrades hit exit 2 (the M-064 contract's version source starves it) |
| driver-install · WinPE UpperFilters encoding (K78) | **M-087** — regdrv-rda.reg hex(7) is 5 ASCII bytes, not UTF-16LE multi-string (the D4 handoff, now owned); og's regdrv.reg:35-36 shows the correct form |
| ci/packaging · FloorVersion pin count + CI-shape docs (K75/Q68) | **MATCH** (M-088 FIXED, DFX-38 — both pins at v0.2.4; the 4-lane doc truth; `tools/doc_truth_gate.py` wired into checks.yml enforces both invariants; ledger row in FIXED-LEDGER.md) |
| driver wire · extended-IOCTL fuzz coverage (Q66/C22) | **M-089** — rdabench.c:390 probes the phantom-zero 0x6000244x codes; the live 0x6002440/44/48 handlers receive zero fuzz iterations |
| driver wire · session-code count prose (C22/Q68) | **N-107** — 14 current-tense "15/18-command" sites vs the code's 19 (the D3 handoff, full inventory; core's DRIVER-CONTRACT.md already says 19) |
| driver wire · identity IOCTLs + Information×512 + LBA scaling + NAS/Cloud exclusion | **MATCH** — re-verified first-hand this session (rda_ioctl.h:63-65/146, rda_readwrite.c:269, rda_session.c:1122, rda_disk.c:191 vs eprdtdrv-sys--driver-internals.md:68,81-84; binary dword probe: all 19 codes in EUDCPEPM.sys) |
| docs · engine-constants row set (Q68) | **MATCH** | **N-108 — FIXED (session 38, `995ba5d` — S11 repair: e1e5124 phantom)**: both rows code-true (1 MiB+4 KiB; 12/48/1024/80) with code cites; the rawsigns comment + the SCAN-ENGINE-DELTA strike-through halves FIXED-ALREADY (parallel lanes); doc_truth_gate.py now enforces the class |
| audit-trail · A-007 evidence cites | **REPAIRED** — the dangling a006-extents-off-by-two.md cites re-pointed at the real a007-extents-off-by-two.md (dated note in the finding) |
| audit-trail · corpus errata | **E65/E66** — the census :99 0x11c gloss (284 = FreeHand 11, MTS/M2TS list-only) + the retdec qword-truncation tooling caveat (the CHM blocker's real cause) |
| carve · batch-2 remainder + WMF grammar | **MERGED** — this lane's duplicate S-041/S-042 filings merged into the continuation lane's owner files as convergence addenda (two lanes, two decompilers, one conclusion on WMF; the CHM/FH11 asm decode donated to S-041) |


## Session-8 lead-lane rows (FINAL numbering past the third lane, audited core `ff879f9`, corpus `be0cfe9` — 2026-10-28)

| Module | Lead-lane findings (final IDs; the parallel lanes' rows are in their own sections) |
|---|---|
| carve | **M-090** (DMG koly walk + DMG-specific cap — erratum E67), **M-091** (the MDB page-chain walk decoded), ~~M-098~~ (HIK — **FIXED, S42 fix lane**, findings/FIXED/M-098)/**M-099** (AEP — open), **N-111..N-113, N-115** (JP2 / LZH / text school / OfficeTagFromXML) — school-2/3 children of S-041; ~~S-047~~/~~M-092~~/~~M-093~~/~~M-096~~ RETIRED (MPEG-TS `a3abfbe`, HTML `eb1f116`, XML+SVG `ca701711`, MOBI `6e37dc21` — matrix row brought current by S11) |
| session + ABI | **N-116 — FIXED (session 38)**: rows newest-first by save_unix (the og's FUN_140273550 !(ftA<ftB) = newest-first, settled first-hand; TRUNCATED = the time-ordered prefix; CLI re-sorted by the same key; `SessionListOgTimeOrdering` RED→GREEN), **N-117/N-118 — FIXED (session 38, `e4f3184` doc-truth batch A — S11 repair: 954c386 phantom)**: the plan's W1b shape + the 7f/7g cites re-pinned; the R-52/54/56/R-31 gap rows flipped to PRESENT with landing cites, **I-029** (driver-code provenance) |
| query | **M-101** (Versions/Tags branches), **N-126** (Unsaved File) |
| preview | **M-108** (text-preview eligibility) + **N-137** (the thumbnail ABI header contradiction — filed S11-lead, renumbered from the N-136 draft after the remote lane's N-136 pushed first); ~~M-100~~ closed-as-covered `6d0f16c` and ~~M-102~~ FIXED (`e56baac7`: cr_ui_thumbnail_generate two-call + format probe; PARITY-VERIFIED session-11-lead) — matrix row current |
| boot repair | **N-121** (ESP automount), **N-122** (NVRAM create/delete) |
| CLI / transport | **N-123 — FIXED (session 38, `32ef33b` — S11 repair: 13245c0 phantom)**: the 11-verb recount + exit 4; N-124 FIXED session 10 — the transport.md SUPERSEDED annotation landed in 2025554; see evidence/s10-reverification-sweep.md |
| orchestration / docs / structure | **N-125** (disconnect states), **N-119 — FIXED (session 38, `995ba5d` — N-135: the recorded `e1e5124` was a pre-rebase phantom): D-88 re-adjudicated against the EuShare decode (leg 2 struck, trigger FIRED-and-adjudicated, the exclusion stands on scope)**, **N-120** (pointer drifts), **I-030** (module-map holes) |
**Session-9 changes (2026-10-28, SA-9 lane — audited core product code `35adcb9` / rda-driver `323e75a` / corpus `be0cfe9`; two-repo scope; RECONCILED at push time into the session-8 board — 7 convergent filings merged as addenda, 1 unique finding kept):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| transport · EPMNtDrv v2 identity IOCTLs + async completion + \Volume\{N} chain | **MATCH** | the never-audited surface verified by this lane: our driver carries 0x60002400 GetProcName (in==4 u32 PID, out>0x207), 0x60002404 GetFileObjName, 0x60002408 GetDiskContextMode (out==4, bound handles only), method-aware pass-through, ×512 Information — the C-sheet corpus decode vs the E-sheet code read, cross-checked (evidence: the SA-9 context sheets; SURFACE-INVENTORY rows flipped) |
| transport · EPMNtDrv trio naming + PID handshake | **MATCH** | the never-audited surface verified by this lane: the PID-token byte-sum transform re-derived at CREATE, mismatch → STATUS_INVALID_PARAMETER; \Device\<UPPERCASED service name> + EPMNTDRV fallback; DO_DIRECT_IO; benchmark-proven 64/64 |
| driver-install · WinPE filter Start posture | DIVERGENCE-MAJOR (parameter added to M-087) | **M-087 + SA-9 addendum** — the demand-start (Start=3) service + active UpperFilters cannot attach at volume-stack build; the og's filter service is Start=0 BOOT_START (EUDCPEPM in regdrv.reg) — the parameter their M-087 lacked |
| transport · driver doc-vs-code truthfulness (admin gate / S12 / 512-byte prose / unreachable default / 57th check) | DIVERGENCE-MINOR | **N-127** (the lane's unique survivor — renumbered from the collided N-091 at reconciliation) |
| V-lane · W1a/W1b/W1c session surfaces (list/progress+cancel/auto-save) | **MATCH** | the landings' corpus claims verified read-only (the D-sheet): v5 save_unix tail = the og's SYSTEMTIME mirror; per-node cancel semantics; completion-only auto-save (the og's own results-found/not-saved gates independently decoded by the parallel lane as S-044 — convergent) |
| V-lane · NTFS origin-LCN carrier | **MATCH** | both stamp sites + clone + nflags bit-8 nonzero-gate verified against the +0x64 decode (part1:85 erratum E68) |
| V-lane · thumbnail engine | **MATCH (documented superset)** | stb+native TIFF+EXIF fast path; the 20171/20172 IPC + UI caps n/a-by-layer; 256px constant not corpus-pinned (the parallel lane's thumbnail findings S-043/S-045/M-080/M-083/M-102 own the deeper gaps) |
| V-lane · installer S-038 steps 2-3 | **MATCH (the registered OS-gate divergence)** | the 13-parameter contract verified landed; step-4-accepted at run 40684304129 |
| G-lane · gap-hunt candidates | **MATCH (covered)** | Device.md CDiskDriveLayoutReadWriter = the lost-partition apply sequence (modules/lost-partition.md:205-218); RegLib (mom-capabilities:94-97 + health.md:63); Win32Bcd writer family (boot-repair.md); NTFSLib (ntfs.md:357); VideoViewer (I-015); CWindowsFS/offline-OS stay census stubs (no corpus behavior parameters — the parallel lane's M-082/N-106 own the adjacent dispositions) |
| carve · WMF placeable gate | MATCH (fixed this session) | **S-042 FIXED** (the std-WMF stricter-gates observation landed too — the og's key+params-only gates; evidence/decompiled-quotes/s9-wmf-placeable-gate.md; findings/FIXED/S-042) |
| carve · CRW/MRW key attribution | DIVERGENCE-SEVERE → **FIXED** | **M-081** (the parallel lane's filing, FIXED at `9e5dedc` — the CRW re-attribution landed; this lane's instruction-level registration quotes merged as the SA-9 convergence addendum, evidence/decompiled-quotes/s9-crw-mrw-registrations.md) |
| carve · batch-2 format set | DIVERGENCE-SEVERE (narrowing) | **S-041** (the parent) + the per-format children (~~S-047 MPEG-TS~~ **FIXED this session** — FMT_TS/FMT_M2TS sync scanners landed, findings/FIXED/S-047; ~~M-092 HTML~~ **FIXED this session** (the four-key chain + the </html> walk, findings/FIXED/M-092); M-093 XML/SVG **FIXED this session** (`ca701711` — S11 repair: d1dc51a is a phantom pre-rebase SHA — the 'csl'+0x1cd decodes, the probe-stamped id split), ~~M-096 MOBI~~ FIXED `6e37dc21` (S11 matrix repair — the row above said "remain", false at `7990a86d`), ~~M-098 HIK~~ **FIXED (S42 fix lane — findings/FIXED/M-098: the private_stream_1 discriminator, the BB->BD skip + direct-BD form, the 0x1A+gated-BE16 pack walk with the cap-exact 1 GiB, L"HIK" dumped from DAT_18006b428, the N-070 score contest at base 700; N-156 the generic-PS BE-length defect discovered+fixed in the same landing)**, ~~M-099 AEP~~ **FIXED (S42 fix lane — findings/FIXED/M-099: the bare-RIFX weak grammar as-is, the XMP-end scan, the 30 MiB cap exact)**, ~~N-111 JP2~~ **FIXED session-39** (the three-row family with the fully-decoded box-walk extent machinery, findings/FIXED/N-111), ~~N-112 LZH~~ **FIXED session-39** (the four method-key rows + the decoded entry-chain walk incl. the -lh5 quirk pinned, findings/FIXED/N-112), ~~N-113 text school~~ **FIXED session-39** (the six-row school incl. the POS needle settled from FUN_18001a470, findings/FIXED/N-113), N-115 OfficeTag remain) + school 1 LANDED (`b92ebef` CHM/ONE/MNY/EMF/SWF/3DS) — this lane's consolidated decode table merged as the SA-9 addendum (evidence/decompiled-quotes/s9-batch2-registrations.md, the instruction-level CHM disassembly s9-chm-gate-asm.md corroborating E66) |
| carve · the text school (VCF/GPX/OSP/POS/CHFP list probes) | **MATCH** (fixed session-39) | **N-113 FIXED** — six rows (VCF/GPX/GPX-BOM/OSP/POS/CHFP): every decoded grammar landed (VCF's multi-card continuation; GPX's declaration+<gpx window + the </gpx> walk, cap 100 MiB; OSP's needle+0xF with the unique reject-on-missing; POS's needle settled first-hand from FUN_18001a470 - "ProcessOn Schema File", extent +21+0x13; CHFP's +0x28 anchor gate + the </BiffProject> walk); the per-chunk find quirk and the VCF over-read ported exactly; 9 tests; findings/FIXED/N-113 |
| carve · LZH archive family (method gate + entry-chain walk) | **MATCH** (fixed session-39) | **N-112 FIXED** — FMT_LZH/LZH5/LZHD/LZH7 (one row per method key, the M-092 policy): the "-lh?-" probe gate at +2, the entry-chain walk decoded from FUN_18003dd70 + the .data constant dump (-lh0/-lhd: hdr+comp+2; -lh5: hdr only - the quirk PINNED; -lh7: hdr+comp; the ext-flag CRC suppression; the -2/+2 step-back dance; size = pos+1); cap 200 MiB (the ctor bound byte-exact); 6 tests; findings/FIXED/N-112 |
| carve · JP2/JPX/JPM family (brand map + size walk) | **MATCH** (fixed session-39) | **N-111 FIXED** — FMT_JP2/FMT_JPX/FMT_JPM landed: the "jP  " signature-box key (the length at +0 never gated — the salvage posture), the payload/ftyp/brand gates, the exact-match brand map (jp2 /jpx /jpm -> the og's 0x80/0x81/0x82) via CarveResult::format_override, and the size machinery decoded from FUN_18001c760/c820/ca60 (the finding's recorded follow-up executed): the BE u32 box walk + the len==1 extended form + the len==0 FF D9 EOC scan (16-zero back-off, guard 0x1F3FFFFF) + the jp2c-consecutive end rule; cap 500 MB; 7 tests; findings/FIXED/N-111 |
| driver-install · FileVersion pin | DIVERGENCE-MAJOR | **M-086** (the parallel lane's filing — this lane's convergent decode merged as the SA-9 addendum: the CI tag==resource gate proposal, evidence in the finding) |
| driver-install · regdrv-rda.reg hex(7) | DIVERGENCE-MAJOR | **M-087** (the parallel lane's filing + this lane's Start-posture parameter addendum) |
| transport · session-code count prose | DIVERGENCE-MINOR | **N-107** (the parallel lane's filing — 14 sites, supersedes this lane's 12-site list) |
| carve · MID/GZ/PSP/SIT batch-1 caps | DIVERGENCE-MINOR | **N-093** (the parallel lane's broader filing — PSP/SIT included; this lane's MID/GZ ctor quotes corroborate, evidence/decompiled-quotes/s9-mid-gz-bounds.md) |

**Session-10 continuation-lane wave 2 (2026-10-29 — audited `84a1832`; the og-codebase deep-dive; EXECUTED evidence class):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| orchestration · whole-disk scan-target construction (the partition list as the target-set basis) | **MATCH** (CONVERGENT DU — the S40 lane's `6ecd6bb4` pushed first; the merge keeps the union: their host-hint gate (covers the probed-FAT-alias-on-MBR case) + their stats names, this lane's display-name roots (M-104), probe-only skip accounting and scanned-only discovery reconciliation; S-051's merged Resolution in `findings/FIXED/`) | the plain disk target now builds per-volume targets from the partition table (every used non-container non-LDM entry: PartitionSlice over the read pipe, per-target probe, direct dispatch with the pass's deep_mode, F-1 subtree rebase, honest skip counters `table_volumes`/`table_volumes_scanned`/`table_volumes_skipped`); discovery's table-match candidates for scanned targets dropped (no double scan; a skipped target stays discoverable); RUN 2 reproduced then fixed (rc=0, files=4, deleted=2 on the two-volume fixture); RUN 3's forced-hint control unchanged. Corpus: architecture/04 §2/§3.4 + DRWUI :904076/:904198. Tests: 9 `Orchestrator.WholeDisk*` (both lanes' names converged from the finding's own test list) + `FatEngine.ReadVolumeLabel*` |
| orchestration/tree · the whole-disk session shape (per-volume display-name roots, FS-type markers, known-vs-lost) | **MATCH** (FIXED this session, lane D — M-104's Resolution in `findings/FIXED/`) | the per-volume roots carry display names (tier 1 the FAT boot-sector label via `fat::read_volume_label`; tier 2 the `NTFS @ <LBA>` FS marker — NTFS/ext4/exFAT label decodes ride N-133's landing; tier 3 the anonymous form, now reserved for genuinely-lost discovery candidates); one tree per volume (deleted as a node flag); the display names round-trip through the session. The og-exact FS-marker ROW and the standard-root narrowing stay the S-048/M-103 wave's scope (corpus-ambiguous, recorded in the Resolution). Tests: `WholeDiskVolumeRootsCarryDisplayNames` / `WholeDiskKnownVsLostDistinguishable` / `WholeDiskVolumeRootNamesRoundTripSession` / `WholeDiskMixedFamilies` |
| carve · the signature registry's structural invariant (kTable rows vs FMT_COUNT, the F8 gate) | **MATCH** (FIXED this session, lane D — A-012, DISCOVERED during S-051's verification) | the 81ee9775 rebase-window sweep had dropped M-093's FMT_SVG row (kTable 119 vs FMT_COUNT 120): the F8 gate hard-downed every `carve_slice` call with ERR_BAD_PARAMETER on all of main (`RegistryF8Gate` RED at bcf31316); the row restored verbatim from `ca701711`, the registry 120/120 clean, `RegistryF8Gate` GREEN, full suite 1105/1105. Full evidence: `findings/ACUTE/A-012-carve-registry-lost-fmt-svg-row-in-rebase-sweep.md` |
| orchestration/tree · the whole-disk session shape (per-volume display-name roots, FS-type markers, known-vs-lost) | **DIVERGENCE-MAJOR** | **M-104** — table-matched volumes surface as anonymous `Candidate @ <LBA>` roots (the +500 table-match rescue), Found/Deleted stay empty, no volume identity anywhere; the og's real-session depth-1 shape (`Local Disk(G:)` / `System Reserved (NTFS)` / `NTFS` marker / `Reconstructed` / `File Path Lost`) unreachable. EXECUTED: RUN 1 + the tree dump + the first-hand G-session census (same evidence file) |

**Session-10 SA-10 lane (2026-10-29 — audited `84a1832` base, reconciled at `ea1ac86`-lineage; the triage + deep + mark-remaining mandate):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| ldm/recovery · multi-disk extent basis (span/striped/mirror/RAID-5 recovery) | **MATCH** (S-052 FIXED, f4e500de — session-11 fix lane) | the per-root basis-id tree model + the controller's surviving device registry + the SafeWriter's LdmBasisTable routing + the physical-span overlap mapping + REC_LDM_LAYOUTS session persistence; recovery/preview/thumbnail read through the same virtual devices the scan read through (byte-exact by test); GAP-S6-15 CLOSED; proof set `LdmRecovery.*` + `Orchestrator.DynamicDiskScanStampsBasisAndSessionRoundTrips`; the two registered divergences: PARITY-DELTAS D-106/D-107 |
| packaging/health · the K71-K73 DRWService slice | **DIVERGENCE-MAJOR** | **M-105** — the og's transient SCM service lifecycle ("Read FVE Server": AUTO_START/LocalSystem/OWN_PROCESS, /install replace-on-install + StartServiceA, the stop-gated delete, per-call SCM error strings, DRWUI's per-use uninstall→install→harvest→2s→uninstall orchestration) decoded at the decompile level; the core has no counterpart (greps clean) and R-83's disposition note is false ("install lives in driver-repo CI scripts" — those install the RDA driver only) |
| carve · ONE/3DS size walks (school-1 follow-ups) | **MATCH** (FIXED — the S42 fix lane, `M-106` FIXED at `<this push>`; the Resolution + the 4-test proof set in `findings/FIXED/M-106-one-3ds-size-walks-decoded-but-standing-in.md`) — both walks landed from the first-hand re-read (ONE: the GUID-recheck chain, the 0xC4/0x64 size fields, the 2x-device gate, the break-point size, the 500,000,000 clamp; 3DS: the MM branch's section walk — the 0x58 shortcut, the stride u16@+2, the 5x-device cap; the NCSD/CCI branch keeps its bounded stand-in, the deviation documented in the Resolution); RED pre-fix by the stashed-engine rebuild (the 68 MiB chain truncated at the 64 MiB stand-in) |
| carve · the batch-1 stand-in residue | **DIVERGENCE-MAJOR** | **M-107** — ten sites still carry "recorded follow-up"/"stands in" comments (X3F :3084, CRW :3108, WPG :3197, WPD :3214, PSP :3236, SIT :3253, APE :3329, EPS :3702, DXF :3741, DWG :2644, SWF-RECT :3914) — S-007's promised follow-ups never owned until now; the X3F section-directory grammar corroborated by the never-cited FileRepair artifact (x3f-repair--section-checkers-walk.md:9-33); MDB resolved mid-flight (M-091), DMG too (M-090) |
| docs/CI · CAPABILITY-MATRIX prebuilt-CI row truth | DIVERGENCE-MINOR | **N-128** — the row cites four workflows + an actions/ dir that do not exist at HEAD (only checks/installer/release/test-benchmark) and buries the unowned ext4/HFS+/Btrfs/ZFS/ReFS staging follow-up inside a DONE verdict; M-088's fix did not touch this row |
| ldm/writer/tree · ID-collision repair (process) | — | **S-050→S-051 renumber** — the wave-2 lane's wholedisk scan-target S-050 collided with SA-10B's file-repair S-050 (both live on main); per the SA-9 rule the earlier push keeps the ID, the wave-2 file renumbers to S-051 (its orchestration/CROSS-REF/SUMMARY refs updated) |

| preview · text-preview eligibility (TxtViewFiles whitelist vs content sniff) | **MATCH** (session 39B, M-108 FIXED) — the generated pool (cr_preview_text_exts_data.cpp, byte-exact 97 tokens/92 unique — the finding's "119" was the artifact's ellipsis-note miscount, corrected) + the extension-gated fall-through + the BOM-aware snippet (UTF-8 BOM skipped, UTF-16LE/BE decoded incl. surrogate pairs, Latin-1 passed raw); extensionless names unsupported (the og's empty-needle early return); DRWUI:535858/:535937 + FUN_140310aa0's toupper fold re-verified first-hand |
| audit-trail · the fix-wave bookkeeping truth batch | **MATCH** (fixed session-39) | **N-135 FIXED** — the item-by-item verification at the current tree (items 1/4/5 FIXED-ALREADY by the b5da834/S11 repairs, reproduced; items 2/6/7 repaired this session) + SIX more live phantom cites found by the guard and re-pinned (M-074/M-077/M-071/M-082/batch-C/N-119) + the I-011 magic-constant mis-paste repaired; the phantom-SHA guard built (scripts/verify/verify_trail_shas.py — 470 tokens checked, exit 0, the acceptance evidence); findings/FIXED/N-135 |
| gate docs · API-DELTA §1.7 rows 44-45 (data-asset scoreboard) | **MATCH** (fixed session-39) | **N-132 FIXED** — rows 44-45 truthful: TxtViewFiles genuinely landed (M-108's data layer); SumaryFileType + PriorityDisplayType LANDED this session as query data + lookups (cr_config_display_data.cpp, verbatim from the corpus config files; the D-113 layering registered); usefullostfiles stays the OOS-UI row; R-31 and the API-DELTA now agree; tests ConfigDisplay.*; findings/FIXED/N-132 |
| gate docs · PARITY-OG-CORE-INVENTORY counts | DIVERGENCE-MINOR | **N-129** (the collision repair converged differently than the S11-lead draft: the remote trail lane's pushed repair renumbered the WAVE-3 bookkeeping file to N-135 and this file keeps N-129 — accepted at the S11-lead rebase per the earlier-push-wins rule; api-ms 44→40 · CORE_RL 19→24 · .mo 25→30 · the DRWReport cite :84→:97 · the EuOfficeTools casing · the stale self-nested row — all re-counted this session) |
| og-tree register · VideoViewer/VideoFileRepairer.exe | DIVERGENCE-MINOR (registration) | **N-130** (the Fixo video-repair console engine, 1.9 MB x86, DRWUI-spawned — no disposition row; SIDECAR-OOS registered per the I-010/D-26 repair-family precedent) |
| engines/orchestrator · on-disk volume labels (NTFS $Volume / ext4 s_volume_name / FAT boot / exFAT root dentry) | **MATCH** (fixed session-39) | **N-133 FIXED** — probe_slice_volume_label + the EngineRun/stats/cfg wiring: the og's engines decode the on-disk label (image scans included); the corrected precedence (--label > engine label > image stem) landed at all three consumer sites; M-079's refuted-premise comment replaced; the ext4 dead decode surfaced through Ext4ScanStats; two finding corrections recorded (the exFAT dentry form; the HFS+ half left as the known residual); tests VolumeLabel.*; findings/FIXED/N-133 |
| session · FS-native parent anchors (NTFS +0x40 / FAT +0x42 / exFAT +0x54) | DIVERGENCE-MINOR | **N-131** (the og persists the FS-native parent datum per node; the core keeps structural parent_id only — reload-time lineage cross-validation lost; the N-100 precedent class) |
| og-tree register · VideoViewer/sfc | UNVERIFIED (tracked-open) — **SUPERSEDED: see the resolution row below and its N-138 refutation (the payload is VideoFileRepairer.exe's live encrypted DB; the cell is history, not the current state)** | **OQ-S10-2** (lane-B — 211,614 B opaque " cfs-"/FAN. payload; no consumer string, no corpus characterization; the VideoFileRepairer.exe Ghidra magic-search is the queued probe) |
| corpus · ifileproperties-part2 ReFS FLAGS | CORPUS-ERRATA | **E69** (part2 §4's "invariant incl. FLAGS" contradicts part2's own ReFS table; the +0x39 [INFERRED] objectId reading weakened by the population-site read — serializer trace queued; core unaffected either way) |

**Session-10 continuation-lane wave 3 (2026-10-29 — the fix-wave landing-parity verification, audited `2beb555`):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| preview · thumbnail input budget (the og 200 MB eligibility anchor) | **MATCH** (session 39B, M-109 FIXED) — max_read default = 0xC800000 (the og's gate, DRWUI :422425-429 re-verified first-hand); the refuted no-cap doctrine replaced at all four sites; the band proof Thumbnail.LargeStillImageWithinOgBand (mutation-proven RED at the 32 MiB default) |
| preview · the stb adapter's large-IDAT decode (the 4 MiB clamp interaction) | **MATCH** (session 39B, M-112 FIXED — a NEW-FINDING filed per the §8 duty during M-109's band test) — the read_cb loop assembles requests across the kMaxRead-clamped reads; every photo-scale PNG (>4 MiB IDAT) was silently DecodeFailed pre-fix |
| board-truth · the fix-wave bookkeeping | **DIVERGENCE-MINOR** | **N-135** (renumbered from N-129 at session-11B — lane-B's earlier-pushed inventory-defects file keeps the ID) — N-103's cited :376 site missed while its Resolution claims both fixed; the gap-board's :379-vs-:225 self-contradiction; five phantom SHAs (edee513/771e2ec/dd0ffd5/95122bd/954c386 — all absent from the object store); the FIXED-LEDGER's missing rows + the 180053de0 mis-paste + the re-created FIXED/ dir; the §7g cite overshoot; the .cpp matrix comment |
| (verification records, no findings) | **MATCH ×8** | S-044, N-116, M-085, S-042, M-090, M-082, M-071, N-117 — every landing's parity claims verified first-hand against the findings' og-decodes + the corpus/decompile + the current tree (evidence/s10-landing-parity-verification.md; the full suite 1005/1005 at HEAD) |

**Session-10 S10-F lane (2026-10-29 — the function-level completeness pass; audited `44f043a5`, corpus `be0cfe9`; evidence: evidence/decompiled-quotes/s10f-*.md):**
the class/function-level mandate lane ("mark all remaining features, implementations,
functions as finding too") — the nine engine function inventories parsed (9,618 fns /
3,088 named), the per-engine class-coverage map built (191 classes cited / 49 never,
the never set semantically dispositioned one-by-one by the T3a/T3b read-agents with the
lead's first-hand re-verification), the nine ghidra trees' 103 binaries censused for
disposition (T4). NEW rows:

| Surface · parameter | Verdict | Cell |
|---|---|---|
| btrfs · result-set descent filter (@eaDir subtree suppression) | **MATCH** (session 39B, M-110 FIXED) — the exact-name descent suppression in populate() (the node emitted empty, children never walked); the prefix-match decode CORRECTED by first-hand binary disassembly (the 7-byte compare includes the literal's NUL: @eaDirBackup descends) — the og's walk suppresses descent into `@eaDir`-prefixed directories (BtrfsSearchDLL FUN_180001cd0 :704-711, 7-byte prefix compare, no length check; the node itself is emitted EMPTY) — Synology EA metadata subtrees never enumerated; the core descends unconditionally (zero @eaDir handling; the M-018 pass flags-not-hides and its pools lack the name — cannot reach the og's shape) |
| engine · volume-label decode/surface (all FS) | DIVERGENCE-MINOR | **N-133** — the og's engines decode the on-disk label (NTFS $Volume $VOLUME_NAME via FUN_18003f3a0 :50963-50980, instruction-decisive; ext4 s_volume_name per algorithms-v2 §2.5); the core never decodes one — the ext4 half-decode (cr_ext4_engine.cpp:200-201) is DEAD code; M-079's "no image-scan label semantics" premise corrected by addendum |
| og-tree register · six product-frame binaries + telemetry set | DIVERGENCE-MINOR (registration) | **N-134** — CloneModule.dll (clone/PreOS host, CEUCloneMgr ×398), legacy.dll (LibTomCrypt substrate, role undecoded), EuDownloader.exe (curl downloader ×287), Boot.dll/BootDriver.dll/peformat.dll (the media-builder family, distinct from M-040's repair side) — zero dispositions; the registers (I-019/I-010/I-027) never name them; the 103-binary census's only residue |
| engine classes · the 25 never-cited-by-name classes (NTFS/FAT/exFAT/Btrfs/EXT4/HFS+) | **MATCH (semantic)** | all COVERED under different citation names — the T3a/T3b reports (vtables re-derived from the og binaries via the trail's RTTI walker; core equivalents cited file:line): CNTFSQuickSearch→Q1/Q2/Q8/Q9+S-022/S-012; CNTFSBuildVolumeBitmap→M-039/S-012 (no $BadClus special-case either side); IFileSystemSearchController→O37/O38/S31-35 (10-slot vtable mapped); CFATFileProperties→Q10/Q11/OQ-SA-1 (no og FAT field unpersisted); CExFatPartitionEx→lost-partition B1/M-042 (the candidate record, not the CalcPartition kin); the HFS+ tree-builder family→Q6/I-016/S-021 — evidence s10f-product-frame-register.md §5 |
| ghidra trees · binary-level disposition census | **MATCH (engine trees) / register gaps (frame trees)** | 103 binaries censused: the engine-bearing trees (searchdll/mo/partition-cores/drivers/drwui) are CLEAN at binary level; the residue is exclusively product-frame (N-134's six + the telemetry set + the three infra-only MOM stubs whose rollup coverage re-confirms the inventory's claim) |
| board · the M-108 filename collision | — (process repair) | **M-108→M-109 renumber** — the landing-parity lane's M-080-residual M-108 collided with lane-B's preview-text M-108 (both live on main); per the SA-9 rule the earlier push keeps the ID — the file renumbered to M-109 with the internal-ID + CROSS-REF + evidence-mention updates (this lane) |

**Session-10 S10-F wave 2 (2026-10-29 — the tracked-open resolutions; audited `deea0241`):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| mom-capabilities · the in-app partition file-browse capability (ExplorerPartition → CWindowsFS) | **DIVERGENCE-MAJOR** | **M-111** — OQ-S8-1 RESOLVED by the consumer trace (the og's user-visible existing-file browsing of mounted partitions: DRWUI:438891/:1421116 → PartitionRecovery:8481-8492 → FileSystem.mo vtable+0x38 → CWindowsFS + the raw-parser fallback behind the same IFSInterface; two internal consumers: used-space, root-openability); the core has no counterpart seam (FindFirstFile* = the .crsf listings only) and no scope registration — every load-bearing cite lead-verified before filing |
| og-tree register · VideoViewer/sfc | **RESOLVED (dead payload)** | **OQ-S10-2 CLOSED** — the decisive probe ran (this session): zero consumers of the " cfs-"/FAN. container across every reversed binary + string table; hypotheses 1-2 refuted; registered dead-shipped via N-134's addendum |
| tracked-open · OQ-S8-1 | **RESOLVED** | CLOSED → converts to M-111 (the OQ's own resolution path executed) |

**Session-11 S11 lane (2026-10-29 — the continuation deep-audit lane: the RAWSearchDLL infrastructure walk [the trail's named wave-2 candidate, executed], the ZFS/ReFS function-level walk [the two trees the S10-F class pass's read-agents did not cover], the OQ traces, and the active-board triage; audited `46b81573` [code tree `432d15dc`'s], corpus `be0cfe9`; evidence: evidence/decompiled-quotes/s11-*.md + evidence/s11-triage-sweep.md):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| refs · B2/B3/B4 (the unnamed-$DATA-only row class) | **MATCH** | S-053 FIXED (session-40 main lane) — the emission gate split: the `name16.empty()` drop removed (the og's name builder no-ops on keySize≤4 and its emit path has no name gate, ReFSSearchDLL `:8674-8675`/`:8478-8481`); name-destroyed records now emit as unnamed recoverable rows (stored name EMPTY — the og persists empty; the display placeholder lives at the CLI print layer only, never the session/csv/json); content-based grading unchanged; session round-trip pins the empty name never mutates. Tests: Refs.UnnamedDataOnlyRowEmitted / UnnamedRowGradeIsContentBased / UnnamedOddLengthKeyEmittedNoCrash / UnnamedNameSessionRoundTrip (all RED pre-fix, GREEN post) |
| refs · B3 (the zero-size∧no-dot noise filter) | **MATCH** | N-141 FIXED (session-40 main lane, the S-053 pass over the same gate) — the compound filter landed: size==0 AND no '.' in name → filtered (the FileValues +0x30 field == parse_general_info's gi.size; `FUN_18000ba90 :8317-8337` size-first dot-second semantics exact); zero-size-with-extension and nonzero-no-dot survive; the suppression is counted (stats.zero_size_nodot_filtered — never silent); an unnamed zero-size row filters exactly as the og filters it (memchr skipped on length 0). Tests: Refs.ZeroSizeNoDotRowSuppressed / ZeroSizeWithExtensionEmitted / NonZeroNoDotEmitted / UnnamedZeroSizeRowSuppressed |
| carve · allocation-failure policy (A35-adjacent) | DIVERGENCE-MINOR | **N-139** — the og: 5-line PUBLOG memory diagnostic + `RaiseException(0xe0000001, 0xc0000025)` (RAWSearchDLL `FUN_1800020c0 :958-983`; DRWUI carries the identical policy at `:94179` — log-then-die product-wide, no handler); the core: silent `ERR_NO_MEMORY` at 8 nothrow sites AND uncaught `std::bad_alloc` from the two throwing buffers (cr_carve.cpp:757/:919 — no catch anywhere upstream) — the core's own two paths disagree |
| carve/report · D13 per-extension result aggregation | DIVERGENCE-MINOR | **N-140** — the og's `GetRawUserInfoMap` per-scan map<ext,count> (`:72213-72221`, insert-or-increment `:11376-11531`, ~60 feed sites, DRWUI renders the "found N .jpg / M .png" breakdown) has no core counterpart: CarveStats is 13 scalars, the report carries 8 category counts, no API tallies `carve_format` over results (the per-node field + the format-REGISTRY ABI exist — the aggregation is the gap) |
| zfs · B8/OQ-T6 multi-vdev | **MATCH (RESOLVED)** | **OQ-T6 CLOSED → I-032 (RETIRED session-42; children S-053/N-141 landed `e10ec381`)** — first-hand decode: the og resolves multi-vdev DVAs via the first-registry-entry fallback (`FUN_1800168b0 :19544-19556`, never fails; mirror OK, stripe local-half, raidz checksum-drops); convergent with the M-019 landing — zfs.md's "og extent unknown" note retires |
| zfs · B9 timestamps | **MATCH (RESOLVED)** | **I-032 (RETIRED session-42)** — the og persists ZPL times at 8-byte precision (the `FUN_180010ef0` family, hard-capped `< 9` at `:14937`); the core's one-second shape matches — the "og carries nanoseconds" note refuted by decode |
| refs · B4 grading boundaries | **MATCH (decoded, superset stands)** | **I-032** — the og's grade = 2-bit byte at node+0x70 (2=deleted) from record+0x59 + a host bulk re-grade hook (`:8476`/`:7137-7159`); no tiers — the core's finer grading stays the registered superset; refs.md B4's "not decoded" note retires |
| refs/zfs · engine-side volume labels (N-133's completion set) | **MATCH (nothing owed)** | **I-032** — ReFS's label branch is DEAD (no ctx+0x1b8 writer; the node is the constant "REFS") and ZFS's FS-name builder has no pool-name case (`:40011`, cases 0-10) — neither og engine decodes an on-disk label; N-133's NTFS/ext4/FAT legs unaffected |
| carve · infra boundary register (A31/32/38/51) | **INFO (register)** | **I-031** — the pending-16 cap with oldest-first force-finalize eviction (`:9573-9591`), Stop()'s partial-file flush (`:9658-9693`), the engine's zero progress emission (host-side only), the 512-multiple sector refusal (`:9265-9268`), the full 36-slot controller vtable (RVA 0x66e38), the user-info singleton layout — D-78/D-101/I-009 boundary addenda + new trail data |
| og-tree register · VideoViewer/sfc (the OQ-S10-2 resolution) | **REFUTATION (process)** | **N-138** — the S10-F "dead payload" closure is refuted by decode: the consumer is VideoFileRepairer.exe itself (loader `FUN_00514f80 :248131`, magic `0x73666320` at `:248309`, XOR key at `:249963-249966`, ctor load + rebuild-on-failure at `:245536`/`:246921-246931`); the zero-hit grep could not match `DAT_` symbols or hex constants — methodology note recorded; OQ-S10-2's file carries the refutation addendum; N-134's register row corrected |
| tracked-open · OQ-S4-3 | **OPEN (plan refined)** | the TTFR half's execution path pinned (the rrecov TtfrProbe → Run-Benchmark.ps1 → test-benchmark.yml chain verified on the live tree; Linux infeasibility re-confirmed; the og-side probe gap named) + two stale references in the file's earlier addenda corrected in-place |
| board · active-board liveness (the remove-the-fixed sweep) | **46 STILL / 2 FIXED / 3 PARTIAL / 5 driver-repo NDVs** | the S11 triage lane's 56-finding sweep — the 2 FIXED (M-102/M-083) and all bookkeeping repairs were landed CONCURRENTLY by the parallel session-11 wave-1/39B lanes (reconciled, no duplication); the 5 driver-repo NDVs dispositioned as OQ-S10-3 by the driver lane; the 3 PARTIALs (N-135 items, N-134 premise [re-scoped by its refutation-adjacent correction], I-030 items 2-3) ride their own findings — evidence/s11-triage-sweep.md |

| carve · the CSL 0x44c size branch | DIVERGENCE-MAJOR | **M-112** (filed M-111, renumbered at the push over the parallel lane's explorer-browse M-111): the og's dword@0x44c + 0x480 size read unported + unowned — 'csl' files sized by the </svg> walk / 1 MiB cap (FUN_180041950 :49749-49828 vs cr_analysers.cpp:2886-2889's own comment; the M-107 class's twelfth site) |
| abi/preview · dead CrPreview wiring | MATCH | N-142 FIXED (this session's S40-continuation lane): the dead src_session/src_node_id pair deleted with its two writes; the comment re-truthed to the landed promotion's own-argument addressing; grep census zero, cr_ui_tests 59/59 |
| register · the product-frame census | RESOLVED | **N-134 — FIXED-ALREADY** at `b202f9ed`: all six register rows pre-dated the finding's own filing by 6.5h (verified `git show 44f043a5:docs/PARITY-OG-CORE-INVENTORY.md`); retired session-11B with the sfc addenda preserved in the Resolution |
## Session-11-lead rows (2026-10-29 — the five go-deeper lanes; audited the `461825d8`-lineage at filing, corpus `be0cfe9`)

| Module × parameter | Verdict | Finding / evidence |
|---|---|---|
| carve · TS/M2TS size cap (C20/A27) | **MATCH** (session 40, S-054 FIXED at `d7ba41f6`) — both rows at the og's 200000000 (the clamp :35507-35509, the fallback :35513, the global :9049); registry pin + the 220 MB behavior clamp test; RELEASE-NOTES carries the S-047 repair |
| carve · suppress_overlap mid-stream walk cost (P47) | **MATCH** (session 40, M-117 FIXED at `d7ba41f6` — discovered during S-054's verification, §8; renumbered M-116→M-117 at the rebase per earlier-push-wins) — the covered check runs BEFORE the validator (the discarded cap-sized walk removed; the 220 MB fixture: >600 s non-completion → 11.3 s; the jump variant rejected — the og ranks overlapping candidates, it does not skip them) |
| preview/grading · the promotion trigger (D15) | DIVERGENCE-MAJOR | **M-115** — the og promotes on clean recovery-READ success (tier-gated, preview-pane-excluded, decode-independent); core promotes only on decode Ok → core UNDER-promotes damaged-but-readable images; OQ-S11-1 RESOLVED (the decode inverts the board-truth hypothesis); the ≤20-byte over-promotion edge included |
| boot-repair · the recovered-OS write-side surgery (P1/A54-58) | DIVERGENCE-MAJOR | **M-113** — the genuefisystemfile.cpp chain (TempSystem MountedDevices / TmpBCD device-element 12000004 / TmpSoftWare / SystemSetup+bootstat) absent + unregistered; R-69's "PRESENT" verdict covers only BCD-cell+NVRAM |
| apply · the host-environment side-effects (P60/A35) | DIVERGENCE-MAJOR | **M-114** — MainModule's silent System-Restore purge (srclient via WMI, both drives, 510-index fallback) absent + undecided + undocumented-in-corpus |
| carve · the filter's always-on set (D16) | **MATCH** | **N-149 FIXED** (session-43 minors lane) — carve_format_always_on() lands the mask 0x1000801's semantics (ids 0x30/0x3b/0x48 re-pinned at the builders): the three families cannot be disabled through any surface; the raw-sign list layer stays disableable (outside the mask range); the LOWDOC token-split pinned ("doc" disables the legacy rows, never the OLE row); CarveEngine.FilterExemptSetAlwaysOn RED pre-fix → green, suite 1223/1223 — the Resolution in findings/FIXED/ |
| carve · the ISpecial two-step dispatch (A25/A28) | DIVERGENCE-MINOR | **N-143** — the og demotes special-reject/plain-accept to generic id-8 and continues; core rejects outright (the D-75 sibling facet) |
| preview/thumbnail · the text-family render (B1) | DIVERGENCE-MINOR | **N-144** — the og's ≤100-byte Arial-13 200×200 text-snapshot thumbnails (TxtViewFiles-gated); core refuses text outright |
| apply · the cross-run interruption marker (A34/A35) | DIVERGENCE-MINOR | **N-145** — the og's pendingMarkedDisk marker (rwConfig.ini, event 0x102d) vs the CRUNDO01-only posture |
| carve · the FH11 extent stand-in (C20/P69) | **MATCH (registered stand-in)** | **N-146 FIXED** (session-43 minors lane) — the site re-homed into M-107's enumeration (the twelfth, the zero-end family class: FUN_180029f30 :32612-32633 re-read first-hand stamps CZeroEndRawFileInfo id 0x1d with no size walk; the D-101 decline covers the architecture; the 32 MiB salvage cap stays) and PINNED — Carve.Fh11BoundedExtentPinned holds the exact constant (mutation-proven fail-before: 32→16 MiB RED); the Resolution in findings/FIXED/ |
| carve · the CSL 0x44c size formula (C20) | DIVERGENCE-MAJOR | **M-112 (the S11B lane's filing — convergent with this lane's draft N-147, dropped per extend-don't-refile; their decode is the richer: the 0x44c-span gate + the +0x118 cap check at FUN_180041950 :49749-49828)** |
| ldm/zfs/preview-abi/carve · comment truth (D46) | DIVERGENCE-MINOR | **N-148** — cr_ldm.h's stale UNDECODED (OQ-T7 resolved MATCH), cr_zfs_engine.h vs D-09's wording, the :3754 cite, the EPS/AI 16-vs-64 drift, six dangling Resolution pointers, og_prop_flags unconsumed |
| config/data assets · the completeness pass (D68) | **MATCH + register extensions** | **I-033** — every engine-behavior asset byte-identity re-verified (FileFilter 1,481 / raw_signs 139+199+62 / EUUserData 111+190 / Dict 228/228); the register corrections (needcopy→AliyunWrap, BUILDPE-keeps-Firebase, remote_config runtime-refetched, recoveryFree's layout); the OSS-credentials + dev-logging data; legacy.dll closed (OpenSSL runtime); the tree bookkeeping (ghidra-local dup, 3 phantom REPORT entries) |
| carve · the repair wiring's second channel (M-021's seam) | **MATCH per-candidate + the facet registered** | the **S-050 addendum** — the og feeds every CHUNK to the repair interface (`vtable+0x40` DDC:9623-27) beside the per-candidate hook; the post-processor contract must decide the channel |
| og-tree register · the errata batch | REFERENCE-side | the CORPUS-ERRATA S11-lead rows (census :20-23/:82, the ZIP dual-gate conflation, the phantom REPORT entries, the dup tree, the two undocumented write-side families) |
| the negative results | MATCH (recorded) | Lane A's infrastructure table (17 rows — the load-bearing machinery fully dispositioned); Lane B's 40-asset inventory (all N/A dispositions register-cited); Lane C's 19-behavior interaction map (broadly covered/host-side); Lane D's binary-level CLEAN re-confirmation; Lane E's no-TODO/no-stub/every-refusal-owned census — `evidence/s11-lead-deep-lanes.md` |

**Session-11 S11-R lane (2026-10-29 — the reconciliation lane: the 63-commit push-window absorbed, the unique product landed; audited `8365956e` → re-verified `b6e0a9ef`):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| abi/preview · the archive inventory's ABI half (the M-100 re-open) | **DIVERGENCE-MAJOR** | **M-116** — the engine's `Metadata.entries` (cr_preview.h:134-191) crosses NO shipped surface: `cr_ui_metadata` (cr_ui.h:1214-1224) has no entries field and `preview_metadata_impl` (cr_ui.cpp:3613-3631) silently discards it; the session-10 retirement record's "reachable today through cr_ui_preview_metadata" claim refuted first-hand at TWO trees; the M-102 repair shape (two-call export, cr_ui.h:1283-1290 the landed precedent) is the fix |
| gate docs · the API currency batch | **DIVERGENCE-MINOR** | **N-150** — both directions, re-verified current at b6e0a9ef: the false-missing R-84 pair (the 6-facet driver-status surface landed; the rows still say probe-only) + the doubly-stale R-37 (→ M-116) + the overclaiming counts (API.md:81 "81" vs 83; CORE-API-CURRENT:16 "34+OK/10" vs 38/12; :345 3-way stale) + the stale-landed API-DELTA rows (10/14/18/22/§3) |
| session schema · the parent-anchor fix-plan basis | **MINOR-ADDENDUM** | **N-131 + S11-R addendum** — the fix-plan re-base: bit 512 is TAKEN by N-100's ntfs_total_clusters landing (cr_records.cpp:174/:278/:475); the +0x60-precedent argument is moot |
| api docs · the R-31 false-PRESENT spread | **MINOR-ADDENDUM** | **N-132 + S11-R addendum** — the e4f3184 propagation verified CURRENT at b6e0a9ef (R-31:225 still cites the false API-DELTA row 45 as its source) |
| carve · the stand-in census nits | **MINOR-ADDENDUM** | **M-107 + S11-R addendum** — AI has its own validator (the parenthetical corrected); EPS/AI stand-ins are 16 MiB not 64 MiB |
| infra register · the two unregister'd items | **INFO-EXTENSION** | **I-031 + S11-R extension** — the dormant only_common vendor mode (a deliberate-divergence D-row) + the A4 device-slot phrasing correction |
| board truth · the re-created findings/FIXED/ | **REPAIRED (this lane)** | The 7 re-created files (I-030, N-111..N-113, N-134, OQ-S10-2, OQ-S8-1 — all ledger-rowed) swept to history per the ledger's own no-re-create convention; the rows stay; the class is N-135's |

The lane's ABSORBED work (duplicated by the parallel lanes that pushed first,
preserved in the session-11-local-backup branch + this session's worklog):
the wave-1 board-truth repairs (M-102/M-083 retirement, the N-129→N-135
collision, the ledger conflict/phantom SHAs — f166440b/bc4513db/2cf3f4d0/
15279503); the OQ-S8-1 resolution + browse filing (4f44db49 → their M-111);
the RAWSearchDLL infra walk (d50a54dc → their N-139/N-140 + I-031); the
N-134 retirement (b37fabab, as FIXED-ALREADY where this lane had REFUTED —
same substance: the register rows predated the filing); the 54-finding
re-triage (their s11-triage-sweep.md; this lane's N-133 phantom-SHA repair
absorbed by session-39's ledger row note; the S-050 register-half and I-030
addenda absorbed by their S11B addenda; the M-107 0x44c fold absorbed by
their M-112).


**Session-41 S41 lane (2026-10-29 — the continuation audit lane per the user's second-session directive [remove-the-fixed + go-deeper + mark-all-remaining]; audited `c7fa527c` pre-rebase, RECONCILED at push over TWO parallel windows [the S11-R/S11-lead/S11B/S12/S40 lanes + the S-051/N-112/N-113/N-142/M-109/M-116 fix landings, then the S-053+N-141 emission-gate landing and the N-150/N-151 renumber]; corpus `be0cfe9`; evidence: evidence/decompiled-quotes/s41-registry-key-residue.md; all three filings renumbered at the push-time reconciliations per SA-9):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| carve · the never-filed DBX analyser (P1/P3/P19/P25/P28) | **MATCH** (**M-121 FIXED — CONVERGENT**: the parallel fix lane's `1d4c96b6` + this lane's correction push, renumbered M-117→M-118→M-121 per SA-9 [the ID triple-consumed: session-40 suppress-overlap row 442 keeps it]; the full Resolution + the 9-test proof set in `findings/FIXED/M-121-outlook-express-dbx-analyser-absent.md` [S42-B convergence: this lane independently detected the same M-117 collision at its push — the 0f5d57fb/9cc6a03e 2.5-minute race, recorded as N-160 item 7 + the FIXED/M-117 stub convergence note; the M-121 chain adopted wholesale here.] |
| carve · the DXF 999-comment entry key (P1/P19/P28) | DIVERGENCE-MINOR | **N-152** — the og's u16-layer key 0x3939 ("99") → CDXFAnalyser (idx 0x19, :2722-2733) + the probe's leading-comment acceptance ("999\n" / "999\r\n" then the shifted LF-tolerant SECTION pattern — FUN_1800376f0 :42240-42290); the core keys "  0\r" only and validate_dxf (:4296) requires "  0\r\nSECTION" at offset 0 — comment-led CAD exports are silently missed (the N-111 variant-key class) |
| carve · the never-filed DBX analyser (P1/P3/P19/P25/P28) | **DIVERGENCE-MAJOR** | **M-117** — the og's RawSpeed::DbxAnalyser (idx 0x15, 4-vftable-ref code weight) registered under THREE magic keys (JMF6 0x36464d4a / CFAD12FE 0xfe12adcf / F789342C 0x2c3489f7 — RAWSearchDLL :2192-2232), the GUID identity probe (4 OE store GUIDs + the JMF6 version word + the marked 16-byte pattern — FUN_180016740 :17509), the u32@+0x7C size pass + the marked-form block computation + the 272-byte embedded-name buffer (FUN_180016950 :17622), a dedicated 0x120-byte DBXRawFileInfo; the core: zero rows in 122 FMT_ entries, zero raw_signs entries, the og's own JSON has no dbx either. The session-3 key-diff marked all three keys "MISSING (N-026)" — the filing was planned (S3-CARVE.md:70) and never made; the ID was consumed by the refs-geometry finding. The 57-key byte-level re-diff: 53 landed, the DBX triple remains (the DXF-999 half landed with N-152, session-43). |
| carve · the DXF 999-comment entry key (P1/P19/P28) | **MATCH** | **N-152 FIXED** (session-43 minors lane) — the u16 "99" 0x3939 key lands as FMT_DXF_C999 (the :2722-2733 binding, the same CDXF chain idx 0x19) + validate_dxf's comment branch (the :42240-42290 probe re-read first-hand: "999\n" → the 14-byte LF section form, "999\r\n" → the 16-byte CRLF form, the NUL-guarded forward scan); both DXF rows suppress_overlap (the N-151/N-153 same-document rule); 4 pin tests, fail-before 3/4 RED, suite 1215/1215 — the Resolution in findings/FIXED/ carries the full record |
| gate docs · API-DELTA row 44 (TxtViewFiles) | **RESOLVED (by M-108's landing)** | **N-132 re-scoped (S41 addendum)** — the TxtViewFiles leg landed at aac75bba (cr_preview_text_exts*.cpp, byte-exact 97-token pool) and row 44 was corrected at the landing; the standing remainder = row 45's false "landed" claim + R-31's propagated clause (the S11-R addendum's aggravation record) + the SumaryFileType/PriorityDisplayType dispositions (0 code hits at c7fa527c) |
| corpus navigation · the 23 never-cited artifacts | **INFO (register)** | **I-034** — the 283-artifact citation census (260 cited / 23 never) + per-artifact dispositions (the 3 SearchDLL family artifacts drove M-118/N-152 (the DBX finding's S42 renumber applied); EUDCPEPM = the Todo-Backup sibling driver family; burn = N-134's media-builder family; UIUX/32 → S-050's S41 addendum; the rest = D-26/OOS-UI/methodology) + the 750-entry og-file census finding NO undispositioned product surface (the never-mentioned set = runtime deps + license texts + drw_light.rcc) |
| board · the count arithmetic (the 56-vs-55 drift, pre-rebase) | **process repair** | the gate docs' "56 active (7/18/24/7)" never decremented the MAJOR half at M-110's retirement — the S11-final truth commit `14cf8064`'s own arithmetic defect, caught by the S41 filesystem re-count at `c7fa527c`; repaired locally to 55 (7/17/24/7) — then absorbed by the reconciliation (the remote's S11B truth commit `1d3af98c` carried the same 55 filesystem truth, convergent) |
| board · the FIXED-archive convention conflict | **OPEN (user decision)** | the session-39 user amendment ("put fixed findings in fixed directory") vs the S11-R sweep's purge (fb3a9a98 retired the re-created FIXED/ files to history citing the pre-amendment sentence) — the S41 lane restored M-108/M-110 locally per the amendment, then CONFORMED to the remote's earlier-pushed purge at the reconciliation (the restorations dropped; the ledger rows carry both SHAs + the history paths + the note). The conflict is recorded, not adjudicated: the next lane should ask the user which convention governs. |
| repair line · the UI-side surface (S-050's evidence) | **evidence addendum** | S-050's S41 addendum: the never-cited UIUX/32 artifact decodes the row contracts (8 signals/10 slots, the Advanced Repair escalation), the serialized task queue, the priority-7 worker fleet, and the scan-completion video-repair probe — the scope registration's behavioral shape (no verdict change; complementary to the S11-lead per-chunk-feed addendum) |
| active board · liveness at c7fa527c (pre-rebase pin) | **54 STILL / 1 PARTIAL / 0 falsely-active** | the S41 battery re-verified every then-active finding's primary defect claim at the pinned commit: no phantom-fixed findings; the single partial = N-132's TxtViewFiles leg (above); the 5 driver-repo NDVs (M-086/M-087/M-089/N-107/N-127) stand as filed per the session-9 scope pin. Post-reconciliation note: the parallel windows' fixes (S-051/N-112/N-113/N-142-CrPreview/M-109/S-053+N-141) landed AFTER the pin — the retirements ride the remote's own lanes, convergent with this battery's STILL verdicts at the pin |


**Session-42 S42-B lane (2026-10-29 — the continuation audit lane per the user's second-session directive [remove-the-fixed + go-deeper + mark-all-remaining], the second such lane after S41; audited core `869da779` (HEAD at open), corpus `be0cfe9`; MERGED at push with the parallel S42-main window (487949a4/4f8fa661) which landed S-048/M-117-dbx/N-125/N-126/N-135/N-148/N-152 + the INFO-tier restoration AFTER this lane's pin — the battery's STILL verdicts are pinned-truth, the retirements ride the remote's own lanes (the S41 precedent); this lane's filings renumbered per SA-9: N-154→N-158, N-155→N-159, N-156→N-160, I-035→I-037; five parallel read lanes [config deep-diff, folder census, DRWUI never-walked hunt, core currency, perf evidence] + the main-lane first-hand verification battery; evidence: evidence/decompiled-quotes/s42-ogg-zip-typing-and-report-decodes.md):**

| Surface · parameter | Verdict | Cell |
|---|---|---|
| active board · liveness at 869da779 (the remove-the-fixed mandate) | **49 STILL / 0 falsely-active / 0 phantom-fixed (pinned truth)** | the S42 battery re-verified every active finding's primary defect claim at HEAD first-hand (grep/read per finding — S-040 image_create absent; S-048 zero `Reconstructed` in src; S-050 absent from both the code AND CAPABILITY-MATRIX §3's exclusion list; M-073/M-076/M-078/M-098/M-099/M-101/M-103/M-105/M-106 (the 64-MiB stand-in reads at cr_analysers.cpp:5530)/M-107 (12 stand-in sites)/M-111/M-112/M-113/M-114/M-115/M-116/M-118 all grep-verified; the 17 MINORs incl. N-146's `32ull<<20` at :4379, N-149/N-139/N-140/N-143/N-144/N-131/N-126/N-145 grep-zero; N-148's cr_ldm.h:30-32 comment read; N-150 re-verified + extended). The 5 driver-repo findings (M-086/M-087/M-089/N-107/N-127) stand as PUSHED-FORWARD (the third repo is out of the two-repo grant). The board moved 64→49 through the fix lanes' own proper retirements (S-049/S-051+M-104/S-052/S-053+N-141/S-054+M-117/N-111..N-153/M-108..M-110/M-102/M-083/M-072/M-074/M-077/N-093 + the doc-truth batch) — zero silent retirements found. [S42-B merge note: the parallel S42-main window retired S-048/M-117-dbx/N-125/N-126/N-135/N-148/N-152 + the INFO tier AFTER this pin — convergent with the battery's verdicts; the merged-tree board is re-truthed in SUMMARY.] |
| carve · OfficeTagFromXML's five-branch typing tail (P1/P11/P19/P25/P65) | DIVERGENCE-MINOR | **N-158** (renumbered from this lane's N-154 filing at the S42-B merge — the parallel window's bookkeeping-batch-2 keeps the ID) — the og's ZIP per-entry typing walk stamps XMIND (0x86, the FUN_180045110 content branch) / MMAP (0x87, `xsd/MindManagerApplication.xsd`) / EDDX (0x8c, both rels forms) / WFP (0x8d, both ProjectFolder forms — the label binary-resolved at .rdata RVA 0x6d940) / DRP (0x91, `MediaPool/Master/MpFolder.xml` — RVA 0x67c88); the core's N-115 landing covers none (grep-zero); RawConfig.ini's `[Filter]` keys XMIND/MMAP/EDDX/WFP/DRP (:12/:15/:17/:18/:22) couple to exactly these typed ids — the core's `disabled_formats` surface has no ids to gate |
| writer+report · the failed-files report's classification axis (P13/P11/P46/P68) | DIVERGENCE-MINOR | **N-159** (renumbered from this lane's N-155 filing at the S42-B merge — the parallel window's HFS+ zero-blocks finding keeps the ID) — the og's "recovery log" is an HTML report (`CSaveFailedFilesWithHtml`, 2000-byte buffer, `ShellExecuteW` open) whose per-file rows carry a tree-classification column (group 1 → Lost Files + Reconstructed@kind7/Tags-Extracted@kind6; group 0 → Existing@kind4 / Lost+Deleted@kind0; else the custom raw-category name — FUN_1403c6fc0 :642755-822, first-hand) + failed-count/failed-bytes counters; the core's `cr_ui_recovery_report_json` has no classification field; R-58's "per-run text log" and API-DELTA row 21's "og format NOT IN CORPUS" are both refuted by the decodable generator |
| audit-trail · the register-truth batch (P68/P65-equivalent) | DIVERGENCE-MINOR | **N-160** (renumbered from this lane's N-156 filing at the S42-B merge — the parallel window's RRWR-direction finding keeps the ID) — I-034's census scope label ("750 entries, top+1 deep" — actually 555 at top+1, 750 = files at depth ≤2, ~720 deeper files on family lines) + its phantom script citations (`s41_ogfile_census.py`/`s41_never_cited_census.sh` — zero hits); N-129's OQ-S10-1↔S-10-2 swap; inventory row 79's under-enumeration (EUAdobeRepair/EUArchiveRepair missing from the row's list) + row 104's missing S-048 cross-note; I-033 §12's overstated RawConfig invariant (STL/VPJ + nine keys inert both sides); the M-117 ID collision record (the renumber to M-118 executed this session) |
| corpus navigation · the S42 never-walked decode register | **INFO (register, acknowledged-at-filing per the S42-main INFO convention)** | **I-037** (renumbered from this lane's I-035 filing at the S42-B merge — the parallel window consumed I-035/I-036) — SearchResult.txt (the QA-gated `[register] SearchResult` full-result-list dump, path/folder/deleted per entry, :388969-389045) + the DRWUI user-mode bad-sector skip policy (12-read trigger, 2→64 MiB adaptive chunk, 0x163, sector-granular refinement, cached bad-range short-circuit, 163,840-sector toast, 1 GiB cap, file-backed 1-bit-per-sector bitmap — :179826-180075) enriching D-38/FB-S01's og-side record + the HTML fragment constants note; no capability gap (the core's RetryingDevice covers the class deliberately-differently per D-38) |
| api-doc truth · the fix-wave-stale extension (P46/P68) | DIVERGENCE-MINOR | **N-150's S42 addendum** — 8 newly-underclaiming API-DELTA rows (M-072 substring/state axes, M-074 ERR_DEST_* codes, M-077 Caps defaults, N-098 contract sentence, M-023 taxonomy, M-017, S-038 installer, OQ-SA-1 dwords — all landed, rows say absent) + 6 new defect sites (the phantom `CR_UI_ERR_FVE_LOCKED` macro at cr_ui.h:328/407/418/421; the dead duplicate `--label` handler at rrecov.cpp:1411 vs :1398 with DIVERGENT semantics; ABI-CONTRACT §7i's 32 MiB residue post-M-109; testing.md's 1,014-vs-1,199 census; API.md 81 / CORE-API-CURRENT 78-vs-83 exports and 34/39-vs-42 codes, 10-vs-12 events; bench/README's deleted-workflow citations) |
| board · the S-048/S-050 fresh evidence (the folder census's unique product) | **evidence refresh (S-048 now FIXED by the parallel window — convergent)** | the S42-B folder census re-verified both then-active SEVEREs first-hand: SaveScan/ carries exactly THREE real .rsf sessions (7,366,685 / 52,744,004 / 8,996,707 bytes, the GUID magic confirmed first-hand in all three — S-048's 189,115 carved rows at 100% depth-2 under per-volume parents stand); EUFileRepair/ is 87 files = 5 repair executables + 82 runtime DLLs with NO templates/signatures/data assets (S-050's capability gap + the §3-exclusion miss both stand at this lane's pin; the fix lane's implement-vs-exclude decision remains open) |
| perf · OQ-S4-3 closure state | **unchanged (tracked-open)** | the S42 perf-evidence pass: the transport-chunking half stays MEASURED (run 40315255368's transcription in the OQ file); the scan-level TTFR half has NO committed value anywhere (run 40747332987 was in flight at the pin; zero ttfr_ms exists in-tree; the emission rrecov.cpp:212-224 + extraction Run-Benchmark.ps1:289-303 are wired and verified) — NOT closeable from committed evidence; bench/baseline.json remains an empty placeholder while baseline-candidate.json (run 40073093997-smoke) awaits human-reviewed promotion |
## Session-42 detail rows (2026-10-29 — the continuation lane: remove-the-fixed liveness sweep + deeper comparison; audited core `869da779`, corpus `be0cfe9`)

| Module · parameter | Verdict | Evidence / finding |
|---|---|---|
| carve · A65/A67/A69 (the suite + gate at HEAD) | **DIVERGENCE-MAJOR** | **M-118** — `Carve.OoxmlRelsPathSplit` RED at HEAD (3 assertions, `test_carve.cpp:8453`: the N-115 test's lowercase rows `:8431-8434` vs the S-049 uppercase contract `cr_carve.cpp:1051-1090`); the landing `67de3276` rode Build+test=SKIPPED + Lint=FAILED; 4 clang-format violations; executed evidence `evidence/executions/s42-suite-red-at-head.md` |
| carve/usersigns · A68 (cap-annotation truth) | **DIVERGENCE-MINOR** | **N-154** — `cr_usersigns.h:46` claims the og's longest ext is 4 ("epub","hqx"); the stock EUUserData ships `manifest` (8) + `class` (5) (111-key parse verified; cap 8 itself correct — zero behavior impact) |
| preview/thumbnail · superset posture (A25) | **MATCH (I-028 RETIRED)** | the register's one ask (ABI-CONTRACT documentation) landed as §7i `:599-625` with M-102; verified at `869da779` |
| zfs/refs · the I-032 register children | **MATCH (I-032 RETIRED)** | S-053 + N-141 landed `e10ec381`; module-doc flips verified (`zfs.md:22/32/33`, `refs.md:26/27`); the .trashinfo open decode stays recorded in-file |
| board · A68/A69 (bookkeeping truth) | **REPAIRED this session** | the "64 active" README/worklog arithmetic corrected to the filesystem truth (49 at the S42 open; the S41 close's 6-SEVERE/22-MAJOR/28-MINOR count was pre-retirement); SUMMARY's two phantom SHAs (`dd0ffd5`/`95122bd` → `ce2405a`/`1f714884`); API-DELTA row 10's stale "MISSING" (M-023 landed: 8 categories/1,481 exts, `cr_query.h:30-33`) repaired; N-135's item set re-truthed (1/4/5 landed; 2/6/7 + the two prose phantoms + the guard script open) |
| config/data assets (all 12 groups) | **MATCH/REGISTERED (re-verified)** | raw_signs.json 139/199/62 byte-exact; EUUserData 111/111 keys + 190/190 signs 0 diffs; Dict 228/228; SaveScan auto-naming exact; the nine config/ files all dispositioned (the five not on the prior list read fresh: config.ini/nasconfig.ini/version.ini/themdark.ini/shellexe.ini); I-033 addendum carries the row-88/row-65 refinements + the remote_config framing decode (CORPUS-ERRATA #8) |
| archive.dll / EUUnZip.exe / PDFViewer.exe (the 0-1-citation trees) | **MATCH/REGISTERED (walked)** | archive.dll = libarchive, sole importer DRWUI enumeration-only (PARITY-OG-CORE-INVENTORY:54; engine half = M-066+S-032; open halves M-116 + the RAR/7z browse-scope OQ); EUUnZip = SIDECAR-OOS (:83; both DRWUI consumer flows verified: NasConfig connect.zip + Activity.zip); PDFViewer = OOS-UI (:30 scope statement; MFC+pdfium viewer, temp-file plumbing is UI-side) |
| the 49 active findings' liveness | **47 STILL-LIVE / 2 RETIRED / 5 CANNOT-VERIFY-HERE** | every SEVERE/MAJOR/MINOR/INFO re-verified against HEAD first-hand; the CANNOT-VERIFY set (M-086/M-087/M-089 → OQ-S10-3; N-107/N-127 → OQ-S10-2) lives in the driver repo, absent from this workspace; zero unretired fixes found |

## Session-42-D merge reconciliation rows (2026-10-29 — the third S42 lane's product reconciled over the S42-main/B/C + S12 + S40-ML windows at the merged tree)

| Item | Disposition |
|---|---|
| M-118 (this lane's suite-red filing) | **RENUMBERED M-123, RETIRED-CONVERGENT** — the remote N-157/M-120 chain owns the defect and repaired it (b00118b5 + 72a17b7c); this lane's unique evidence (the CI check-runs API proof, the four format sites, the parent-commit pin) preserved in `findings/FIXED/M-123-*.md` + `evidence/executions/s42-suite-red-at-head.md` |
| N-154 (this lane's usersigns filing) | **RENUMBERED N-161, STILL-LIVE** — `cr_usersigns.h:46` re-verified false at the merged tree; the remote's N-154 (bookkeeping batch 2) keeps the ID per earlier-push-wins |
| I-028 / I-032 | **CONVERGENT RETIREMENT** — both lanes retired them independently (the remote's INFO-tier acknowledgment restoration + this lane's liveness verification); ledger rows merged, both Resolutions preserved in-file |
| N-135 | closed by the remote's full fix (0eb5f82f + `scripts/verify/verify_trail_shas.py`); this lane's narrowing addendum preserved in the retired file as the at-pin record (the "guard not landed" note was true at `869da779`, superseded at the merge) |
| the M-118-cell row in this lane's S42 block above | read as M-123 post-merge (the cell's substance — the execution evidence — stands; the ID moved) |
