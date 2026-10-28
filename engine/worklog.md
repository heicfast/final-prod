**Session-40 lane B territory claim (coordination contract, 2026-10-29):** this lane
(MINOR-first per the user directive) continues on the UNCLAIMED MINOR set: N-115
(OfficeTagFromXML typing), N-126 (the Unsaved File category), N-125 (the disconnect-state
classification), N-132 (API-DELTA truth + the SumaryFileType/PriorityDisplayType
dispositions), N-135 (the fix-wave bookkeeping batch), N-148 (the comment-truth batch),
N-150 (the api-doc-truth batch). Pulled-before-pushed discipline; one finding, one push.
Landings this lane so far: N-151 (the nested-declaration double-carve rider), N-137, N-129,
N-130+N-138, N-128, N-096, N-122, N-121 + the N-111/N-113 convergence verifications.
- **N-121 FIXED** — the ESP auto-mount step (the og's CUpdateGptSystemBootPartition)
  registered as the documented HOST contract (PARITY-DELTAS D-90/N-121 + the rrecov CLI help +
  the boot-repair module row): the host discovers the ESP by type GUID, mounts, passes the BCD
  path, unmounts exception-safe; the core implementation is the registered follow-up gated on
  the DeviceManager :49956 behavioral decode (the corpus's trace-string-level evidence is the
  finding's own open question) with the two-test plan attached.
- **N-122 FIXED** — the NVRAM Boot#### create/delete exclusion registered (D-90's addendum in
  PARITY-DELTAS + the boot-repair.md row): the patch-only v1's scope is now a navigable register
  row with the EFIBoot cites, the honest-stop pin re-verified, the rationale, and the next
  concrete step (the :3405-3480 create-walk decode, then create-on-missing with the test flip).
- **N-096 FIXED** — the identity-IOCTL non-issuance registered per the finding's option 2:
  D-DRV-6 in PARITY-DELTAS (the og's sole client identity IOCTL — the per-open
  GET_DISK_CONTEXT_MODE mode probe — has no decoded behavioral consumer; the core's
  non-issuance is the deliberate lighter posture), and the DRIVER-UTILIZATION control-IOCTL row
  downgraded from MATCH to "MATCH (wire) / OMITTED (client issuance — deliberate)". Doc-only.
- **N-128 FIXED** — the CAPABILITY-MATRIX prebuilt-CI row doubly-untrue claim repaired: the
  evidence column re-pointed at the actual post-consolidation lane set (checks / installer /
  release / test-benchmark — no .github/actions/ exists), and the DONE verdict split from the
  ext4/HFS+/Btrfs/ZFS/ReFS staging follow-up, which now has an owner (this row) and a next
  concrete step (per-filesystem staging scripts feeding the same expected-vs-observed harness).
  Doc-only; the directory listing is the verification.
- **N-130 + N-138 FIXED** — the VideoFileRepairer.exe register row (SIDECAR-OOS, the full
  evidence chain: fix_main_code lineage, -r/-a CLI, MP4/QT atom grammar, avcodec-62 + faad2, the
  M-021 touchpoint) enriched with N-138's decode (the <dir>\sfc encrypted engine-init database);
  the family row cross-noted. N-138's trail-truth half: the FIXED-LEDGER's OQ-S10-2 row now
  carries the SUPERSEDED verdict (the "dead payload" closure refuted by the byte-level decode —
  hypothesis 1, live repair-line data asset, was correct); the matrix's refutation row and the
  README note (the audit lane's landings) re-verified standing.
- **N-129 FIXED** — the og-tree disposition register's six numeric/cite defects, every count
  re-run against the corpus this session (40 api-ms / 24 CORE_RL / 30 .mo / the :97 DRWReport
  cite / the on-disk EuOfficeTools casing / the stale self-nested row deleted). Doc-only; the
  re-runs ARE the acceptance. One self-correction mid-fix: this lane's first pass dropped the
  EuOfficeTools entry (misreading item 5 as non-existence); the tree re-verification restored
  it with the correct casing — the finding's text was right.
- **N-137 FIXED** — the thumbnail ABI header's two contract-truth defects: (1) the section
  comment's "0x50030004 ERR_TRUNCATED never returned" denial replaced with the truth the
  implementation, ABI-CONTRACT §7i, and the existing test already carried (a decode REFUSAL
  returns ERR_TRUNCATED; in-band truncation stays an Ok-path disclosure); (2) the caps field's
  "validated 0/1" claim enforced — allow_embedded > 1 now refuses with ERR_BAD_PARAMETER
  (0/1 = default-ON/explicit-ON, the og-parity always-prefer-embedded preserved and documented
  as the no-off-position design). Tests: ThumbnailCapsAllowEmbeddedValidated (RED pre-fix on
  the silent 7-acceptance) + ThumbnailHeaderMatchesContract (the doc-truth pin, RED pre-fix).
  cr_ui_tests 61/61; cr_tests 1082/1082. Audit trail: retired to findings/FIXED/ with
  Resolution; ledger row.
- **N-113 CONVERGENCE + N-150 FIXED (the rider over the parallel landing)** — this lane's own
  N-113 implementation (six rows, every grammar re-verified first-hand, the POS needle question
  settled: the literal "ProcessOn Schema File", formula needle_end+0x13 calibrated against
  OSP's +0xF twin) hit the push window against the parallel minors lane's equivalent landing
  (b6e0a9ef — theirs stands). The verification of THEIR landing exposed three nested-candidate
  double-carves their tests missed (the SVG declaration+interior pair — pre-existing from
  M-093; the VCF inner card; the GPX BOM interior declaration): **N-151 filed and fixed (renumbered from N-150 at the S11-R collision — their api-doc batch pushed first)** —
  suppress_overlap on the four M-093 rows + FMT_VCF/FMT_GPX/FMT_GPX_BOM. Tests:
  SvgDeclaredDocumentCarvesOnce, TextSchoolVcfInnerCardIsSameFile, GpxBomFormCarvesOnce (all
  RED on c0f69250, GREEN after). Suite 1082/1082 (1040/42/0). Audit trail: N-150 filed +
  retired to findings/FIXED/ with the reproductions; ledger row; matrix current.

## Fix session 40 — the diff-to-fix board execution (2026-10-29, T1 baseline: core `4a6a5164`, corpus `be0cfe9`; prompt saved to worklog/session-40-diff-fix-prompt.md; SEVERE-first order; board at open: 6 SEVERE · 18 MAJOR · 21 MINOR · 5 INFO active + 3 tracked-open; suites 1003 + 59 green at open)

- **S-051 FIXED** — the whole-disk scan target set. Evidence reproduced
  verbatim on current main FIRST (the audit's own probe: RUN 2 = rc=0,
  files_found=0 on the healthy two-volume fixture — the silent empty
  verdict), corpus re-read first-hand (architecture/04 §2 routing table +
  §3.4 "reads the partition list, inserts lost partitions as ADDITIONAL
  scan targets"). The fix: `run_engine_pass` builds one scan target per
  used, non-container partition when the slice basis is whole-disk and
  a table parses — a PartitionSlice per target over the same retry/cache
  pipe, per-target probe_family, per-volume tree root, F-1 subtree
  rebase, honest skip accounting (ScanStats.table_volumes*). Gates: LDM
  precedence kept; host-forced fs_hint keeps the single dispatch
  (host_fs_hint_ captured pre-probe); probed families (incl. the FAT
  alias on MBR boot code) route to the table flow. Discovery reconciles
  against the primary target set (table-match candidates dropped before
  persistence — no double scan, lost list never carries in-table
  volumes). Fixtures: the evidence's exact two-volume MBR disk + a GPT
  twin (CRCs exact) + mixed FAT16/exFAT + damaged-skip + superfloppy
  regression pins. The exFAT builder extracted to test_fixtures.hpp
  (shared, no drift). Tests: WholeDiskScansTableVolumes (RUN 2's exact
  flip to files=4/deleted=2 proven by the re-run probe),
  WholeDiskGptTwin, WholeDiskMixedFamilies,
  WholeDiskDamagedVolumeSkippedHonestly, WholeDiskSuperfloppyUnchanged,
  WholeDiskNoDoubleScan. Suite 1009/1009 + 59 green. Docs:
  CORE-API-CURRENT/API.md (cr_ui_open_disk semantics), rrecov
  --whole-disk note. Audit trail: Resolution in the finding, retire +
  ledger row in the follow-up trail commit.

## Fix session 40 (2026-10-29 — the fix-all-minors-first mandate; T1 baseline: core `09941ef4` at open, corpus `be0cfe9`; prompt: worklog/session-diff-fix-40-prompt.md)

The user's session-40 mandate: fix every active finding, **MINOR first**, one verified fix per
push, every fix proven fail-before/pass-after, findings retired to findings/FIXED/ with
Resolutions. Suite baseline at open re-verified: 1037 tests / 995 pass / 42 env skips / 0 fail.

- **N-111 CONVERGENCE (du-resolved, their landing stands)** — this lane independently decoded
  and implemented the JP2 family (FUN_18001c380's gates + FUN_18001c820's two-phase box walk +
  FUN_18001ca60's jp2c marker map), then hit the push window: the parallel session-39 lane's
  equivalent landing (461825d8) was on main first. Their landing verified post-hoc by this
  lane: same gates, same walk semantics, same override architecture — the decode is now
  two-lane-verified. 13/13 Jp2+Lzh tests green; suite 1064/1064 at `aab031ac`. Record:
  worklog/session-diff-fix-40-convergence.md.

## Fix session — the parity close (2026-10-29, T1 baseline: core `84a1832`, corpus `be0cfe9`; prompt saved to worklog/session-diff-fix-parity-close-prompt.md)

- **S-042 FIXED** — the WMF placeable grammar. Re-read FUN_1800264d0
  (:29807-29845) first-hand before touching anything (the finding's decode
  confirmed gate-for-gate). `validate_wmf_placeable`: hmf@+4==0, reserved
  dword@+16..19==0 (the old inch gate read the reserved field and rejected
  every real file), checksum@+20==XOR(words 0..9), std header@+22, params
  word@+38==0, extent = Size@+28*2+22 exact (was: 64 MiB probe heuristic).
  Registry min_size 26→40. `validate_wmf_std`: the invented gates
  (version/objects/maxrec/words) removed — the og gates key+params only
  (SA-9 addendum's observation). Fixtures regenerated from the canonical
  22-byte Aldus layout per the fixture-regeneration requirement (the old
  plants mirrored the bug). Tests: WmfPlaceableOgGrammarCarves,
  WmfPlaceableOldCoreGrammarRejected, WmfPlaceableTwentyByteVariantRejected,
  WmfStdOgGatesAcceptNonCanonicalHeaders + re-planted S007 batch-1 tests.
  Suite 1027/1027 green (Linux release). Audit trail: finding moved to
  findings/FIXED/ with Resolution; CROSS-REF-MATRIX cells → MATCH;
  modules/carve.md verdict updated; SUMMARY counts updated.
# worklog.md — the live multi-agent session log

**Archive note:** the 2026-10-28 history bulk (17,690 lines, sessions 1
through the diff-fix waves) was removed from main via PR #13 (980d09c /
92bdff1); every entry remains retrievable from git history
(`git log --diff-filter=A -- worklog.md`). This file is the LIVE log —
each agent session appends its Task-ID entry here per the coordination
contract (handoffs, boundary items, convergence records).


---

## Fix session 39B — the MAJOR preview/query parity pass (2026-10-29, T1 baseline: core `7990a86d`, corpus `be0cfe9`; prompt: worklog/session-diff-fix-39-minors-prompt.md; a parallel instance of the same user mandate — this lane rebrands 39B after the lead 39 instance's T2-claim landed first)

**T1 trail-repair (pre-work, the N-135 class discharged first) — DU-RECONCILED
at push with the parallel session-11B lane** (their convergent repairs landed on
main first and STAND: the conflict-marker sweep, the N-129→N-135 renumber, and
the M-083/M-102 retirement; this lane's landing KEEPS the unique content — the
phantom-SHA row sweep, the M-082/S-045 missing rows, the extended counts):
- **The phantom-SHA row sweep (this lane's unique product)**: 19 landing-cite
  repairs in FIXED-LEDGER.md, every one verified by `git cat-file -t` + `git
  log --grep` first-hand — A-005 604bd2cd (was the FUN_ 180010720 mis-paste),
  A-011 c8999987 (18002e2b0), M-017 48a6c099 (140510a0e), M-020 f830db90
  (8dbfc2d2), M-064 dfb2761 (6b50bb8; fix itself = rda-driver v0.2.4 323e75a,
  cross-repo), M-079 0b76757c (f43b747), M-090 9daa787 (180053de0), N-054
  b2f48c64 (494a5546), S-031 the 26d842b6 arc + 7514368a (140c41aa0), S-034
  79ca47ba (180006b40), M-084/M-091 8a7055ea (16475b8), N-116 9445627f
  (632d21e), M-093 ca701711 (d1dc51a), M-096 6e37dc21 (7ca83f5), M-085
  fb7cf30, S-041's cites, the purge header's 9496f3c/771e2ec.
- **M-082 + S-045 rows added** — both findings were retired from the tree
  without ledger rows (M-082: fix 1f714884, the f8bb4d21 retirement's "ledger
  row" claim never landed; S-045: real fix 021ae079, the 06cd56e4 retirement
  cites the phantom 3aba383) — the N-135 item-4 completion. Row count after:
  **287 (267 main + 20 OQ)**.
- **Ledger row-count truth**: 264 rows at the 7990a86d baseline; +2 missing rows
  added (M-082 — retired at f8bb4d21 whose "ledger row" claim never landed, fix
  1f714884; S-045 — retired at 06cd56e4 citing the phantom 3aba383, real fix
  021ae079) = **266 rows**. The 270-282 headline claims were push-window narrative
  arithmetic (claimed landings, never rows) — the snapshots keep as dated history
  with every phantom SHA replaced by the post-rebase real commit.
- **Phantom-SHA sweep (N-135 item 3 extended)**: 16 mis-cited landing records
  repaired — A-005 604bd2cd (was the FUN_ 180010720 mis-paste), A-011 c8999987
  (18002e2b0), M-017 48a6c099 (140510a0e), M-020 f830db90 (8dbfc2d2), M-064
  dfb2761 (6b50bb8), M-090 9daa787 (180053de0), N-054 b2f48c64 (494a5546), S-031
  the 26d842b6-arc + 7514368a (140c41aa0), S-034 79ca47ba (180006b40), M-084/M-091
  8a7055ea (16475b8), N-116 9445627f (632d21e), M-093 ca701711 (d1dc51a), M-096
  6e37dc21 (7ca83f5), M-079 0b76757c (f43b747), M-085 fb7cf30, S-041 row's cites,
  the purge header's S-044 7b7e095e (9496f3c) + M-090 cites. All verified via
  `git cat-file -t` + `git log --grep` first-hand.
- **N-129 ID collision + M-083/M-102 retirement**: convergent with session-11B
  (bc4513db) — their landing stands (this lane's local drafts dropped at the
  DU resolution; the collision analysis agrees: lane-B's inventory finding keeps
  N-129, the wave-3 bookkeeping batch renumbers N-135; M-083/M-102 both FIXED at
  e56baac7 with the retirement completed by their lane).
- Board after the repair: **50 active (6 SEVERE · 20 MAJOR · 19 MINOR · 5 INFO)
  + 5 tracked-open** (OQ-S10-2 now carrying M-086/M-087/M-089 + N-107/N-127's
  driver halves, per the driver lane's 0a08cb3c disposition — accepted verbatim:
  those fix surfaces live in heicfast/rda-driver, outside this session's
  two-repo grant).

**T2 (M-108 FIXED — the board's own recommended first fix):** the
TxtViewFiles text-preview eligibility model. Corpus re-verified first-hand:
both DRWUI consumers (:535858/:535937 + the second at :535913-:535960),
FUN_140310950 (the extension transform) and FUN_140310aa0 (the
toupper-folded case-INSENSITIVE search — decoded in the loop this session),
the ".txt;" ini-absent default, both corpus FilterFolder.ini copies
byte-identical. COUNT CORRECTION: the whitelist is 97 tokens / 92 unique
case-folded (the "119" in the finding + the artifact was the ellipsis-note
miscount). Landed: scripts/gen_preview_text_exts.py +
src/core/preview/cr_preview_text_exts_data.cpp (byte-exact pool) +
cr_preview_text_exts.cpp (case-insensitive membership) + the
extract_metadata fall-through gate + the BOM-aware snippet (UTF-8 BOM
skipped; UTF-16LE/BE decoded incl. surrogate pairs with the N-016b U+FFFD
rule; Latin-1 raw) + PreviewReader::props() (the name source). The
extensionless-name case decoded and ported (the consumers' empty-needle
early return -> unsupported). 8 new tests; fail-before PROVEN by mutation
(pre-fix heuristic restored: 7 RED). 2 existing tests updated with the
behavior change (the false-positive expectation + two .bin->.txt renames
preserving the compressed-extent tests' text-sniff intent). D-111
registered (the precedence edge: corpus-silent). API-DELTA row 44 now
true. RED REPAIR: the M-074 parallel landing's win_syntax_check failure
(the three WinError constants missing from tests/win-stub/windows.h —
ERROR_FILENAME_EXCED_RANGE/ERROR_INVALID_NAME/ERROR_BAD_PATHNAME with the
real WinError values). ctest 1102/1102.

**T3 (M-110 FIXED — the semantics corrected before landing):** the
Synology @eaDir descent suppression. The instruction-level verification
DISPROVED the finding's decode: disassembled
og-codebase/BtrfsSearchDLL.dll first-hand (capstone) — the compare loop
at 180001e30-180001e44 is movzx/cmp/cmp-rax-7 over r12 = 0x180018580
("@eaDir\0"), SEVEN bytes including the NUL terminator: the match is
EXACT, not prefix — "@eaDirBackup" mismatches at index 6 and descends
normally. The fix lands the exact-match suppression in
cr_btrfs_engine.cpp's populate() (the node emitted empty, children never
walked; NOT routed through cr_filterfolder — different mechanism, the
not-in-pools guard pins the boundary). Fixture: build_synology_volume
(the standard /photos/@eaDir/SYNO@EA@photos layout + the @eaDirBackup
discriminator + the unaffected sibling). Tests:
Btrfs.EaDirSubtreeSuppressed / EaDirNodeItselfEmitted (fail-before
PROVEN by mutation: both RED with the suppression removed) /
EaDirExactMatchOnly (the discriminator — RED against a prefix-match
implementation) / EaDirNotInFilterfolderPools. ctest 1110/1110. The
finding + the evidence file's prefix paragraph corrected in the
Resolution; the module row + matrix cell -> MATCH.

**T4 (M-109 + M-112 FIXED — a NEW finding discovered and closed in the
same test):** the thumbnail 200 MB eligibility anchor + the latent
large-IDAT decode defect. M-109: the og's gate re-verified first-hand
(DRWUI :422425-429 'if (0xc800000 < size) skip'); max_read default
32 MiB -> 0xC800000; the refuted "no input cap" doctrine replaced at
all four sites (cr_thumbnail.h x2, cr_thumbnail.cpp, cr_ui.h, the test
pins); the band proof build_large_png (a genuine in-test PNG encoder —
zlib over incompressible PRNG scanlines, 3500x3200 = 34 MB); the
past-gate refusal at 200 MB+1; mutation-proven fail-before (the 32 MiB
default restored: the band test AND the anchor pin RED). M-112
(NEW-FINDING, §8 duty): the band test's outofdata failure isolated a
latent engine defect — StbioSource::read_cb issued a single
read_at (clamped by PreviewReader::kMaxRead to 4 MiB) while stb's PNG
loader demands the full IDAT in one getn — every photo-scale PNG (>4
MiB IDAT) was silently DecodeFailed since the engine's landing; no
prior fixture exceeded the clamp. The fix: the read loop (each
iteration budget-spent, true-EOF on a 0-byte return). The band test is
the shared regression proof (RED outofdata pre-loop, green post).
ctest 1119/1119.

**Session-39B territory claim (coordination contract):** the lead 39 instance
claimed MINOR + INFO; 11D claimed SEVERE; 11C/11D the MAJOR orchestration/carve
cluster. **39B takes the unclaimed MAJOR preview/query/ABI cluster:
M-108 (preview text eligibility — the board's own recommended first fix),
M-109 (thumbnail 200 MB eligibility anchor), M-110 (Btrfs @eaDir subtree
suppression), M-101 (results-tree Versions/Tags branches), M-077 (preview caps
defaults), M-074 (destination validation codes), M-076 (trim facet enum rows)**
— none overlap the claimed territories. Pull-before-push discipline active;
one finding, one verified fix, one push.


Task ID: SCAN-2e-recon (Agent 2 — the S-090/A-011 + N-089 convergent-landing reconciliation)
Agent: Agent 2 (scan-engine lane)

Work Log:
- Collision resolved: the probe lane's A-011+N-089 landing (c899998)
  and this lane's unpushed S-090+N-089 fix (042f363, discarded) fixed
  the SAME two defects discovered independently in the same window
  (this lane's instrument: its first spec-correct BE SQLite fixtures
  went RED; the debug trail isolated the LE reads, then the
  `2ull << 30 / page_size` precedence parse = the 2-page cap).
- The landed A-011 decode supersedes this lane's draft on every
  dimension (the +56 text-encoding gate, the RetDec-export Analyse
  entry FUN_18002e2b0, the encoding-aware marker forms, the 4-page
  scan, the note-propagation mechanism) — no code kept from this
  lane's draft; the draft's S-090 finding file dropped (A-011 stands).
- Landed as THIS lane's reconciliation: the A-011 convergence record
  (the complementary Ghidra-side FUN_18002dfc1 :35404-35432 name-gate
  decode + the second-stage isolation fail-before: fixes held, subtype
  neutralized -> only the FCP-label test RED — the label-only proof
  N-089's severity rests on) + the SCAN-ENGINE-DELTA F2b row (A-011
  form) + this entry. The probe lane's OQ-SA-2 file already carries
  this lane's convergent resolutions (their :1463221 merge note).
- Verified on the landed state: full suite 932 passed / 0 failed
  (linux-release).

Stage Summary:
- The convergent landing reconciled with zero duplication: one code
  path (A-011's), one finding id per defect, both lanes' evidence
  merged into the record. The A2 session's net queue after this:
  OQ-SA-1's NTFS +0x64 half (narrowed probe), OQ-S4-3's TTFR half
  (execution-bound), the S-007 remainder. SCAN-ENGINE-DELTA.md is
  current with the board.

---
Task ID: API-FIX (the api-fix session — context recovery + the driver-era ABI drift batch + three CI-red repairs)
Agent: main engineer (API-focused remediation)

Work Log:
- Context recovery per protocol: three parallel read-legs (corpus /
  core / driver) produced the API ground-truth pack
  (/home/z/my-project/api-ctx/*.md, path:line-cited); the consolidated
  45-row present|inconsistent|missing|superset delta board is
  committed as diff-to-fix/API-DELTA.md.
- API-FIX-0 (1117ac9): repaired the two CI-red breakages the parallel
  lanes left (MSVC C4244 in the FH11 extent check -> integer drift
  check; clang-format violations in 9 files). Verified green on the CI
  lane's exact config (gcc-13+ASan; a matching local toolchain was
  bootstrapped under /home/z/gcc13 — gcc-14 mis-compares warnings).
- API-FIX-1..3 (c169332): the driver-era ABI-contract drift batch —
  the phantom-zero extended-IOCTL literal tail swept from 12 sites
  (0x6000244x -> 0x600244x, 3bda8e2's residue); cr_ui.h's phantom
  CR_UI_ERR_INVALID_PARAM and the unreachable "direct" transport
  value removed; ABI-CONTRACT §4 gains the ERR_DRIVER_* 0x500B
  family, §6 gains CR_EVT_FS_CHECK(10) + the M-041 struct tail, §7e
  rewritten driver-only; API.md 78->79 exports / v3 / 3 missing rows;
  CORE-API-CURRENT SUPERSEDED banner; API-SURFACE-AUDIT headline
  61->79@v3. Four doc-truth pins hold it (test_cr_ui.cpp).
- Format tail (630df04): the OQ-SA-1 lane's test_fat_volume.cpp
  comment alignment (the second format miss caught by local-mirror).
- Double-start repair (605d682): the v0.2.4 rda-driver installer
  (M-064's %s%d batch) starts the service itself; the
  install-driver composite action re-issued sc.exe start and failed
  the staged-scenarios lane on 1056 — the start step is now
  idempotent (RUNNING-query first; any other start failure stays
  fatal per the v0.2.3 exit-code lesson).
- Driver repo state (read-only): rda-driver's own doc drifts (D2 stale
  0x6000244x in docs/architecture.md, D3 stale 15/18-command counts
  vs the 19-code set, D4 invalid hex(7) UpperFilters data) recorded in
  the delta pack — that repo's lane owns them (it has active lanes;
  single-repo commit discipline held here).

Stage Summary:
- The API surface's normative docs now match the driver-only code they
  describe, with 4 regression pins; three CI-red states repaired
  across the session; the remaining API-scope board: S-038 step-2+
  (installer implementation, packaging lane), A-ABI-6 (codeql policy —
  owner call), rda-driver D2/D3/D4 doc drifts. Suites verified green:
  969/969 (my batch), CI success on 1117ac9 + 630df04.

---
Task ID: API-FIX-2 (api-fix session 2 — the post-A-011 CLI/CRSF doc truth pass)
Agent: api-fix lane (this session)

Work Log:
- Context-recovery protocol re-run, API scope only: three parallel
  read legs (corpus / core / driver) -> the session-2 delta board in
  diff-to-fix/API-DELTA.md §4 (Session 2).
- Session-1 board re-verified first-hand: driver wire byte-exact vs
  rda_ioctl.h (identity 0x60002400/04/08 + extended 0x6002440/44/48 +
  all 19 session codes + F11 x512); 79 cr_ui exports recounted;
  39+OK error codes recounted; CLI dispatch = 10 verbs (code truth).
- Rebase reconciliation: the remote lanes landed M-064 FULLY FIXED
  upstream (rda-driver v0.2.4 323e75a: -Layout Versioned %s%d
  coexistence + -Uninstall verb + versioned-aware teardown + PS AST
  CI gate) and the A-011/N-089 convergence (fc74677) while this
  session verified — the board row reconciled; core's driver-status
  discovery confirmed unaffected (the service carries the binPath).
- Four NEW verified drift items (all doc-vs-code, zero code defects):
  A: docs/crsf-format.md omits REC_FS_CHECK type 9 (M-041) and its
  payload grammar; B: the rrecov.cpp header comment lists 8 of 10
  verbs + misses scan --image/--deep/--repair-undo/--user-signs and
  recover --image; C: docs/API.md §1 the same 8-of-10 verb gap + a
  phantom [plan] value on --repair (bare flag at rrecov.cpp:1089) +
  a duplicate exit-code row 3; D: docs/CORE-API-CURRENT.md §c says
  9 verbs (omits driver, stale wmain cite) and c.7/c.8/c.9 still
  document the REMOVED --no-rda flag + the "direct" transport label
  (only "rda" exists, rrecov.cpp:1992) + stale function-range cites.
- Fixes A-D landing as separate commits (the three-site verb-list
  pin: docs/API.md + CORE-API-CURRENT §c + the rrecov.cpp header
  comment must move together from now on).

Stage Summary:
- The API lane's remaining board after this session: S-038 steps 2-4
  (packaging lane, owner decisions pending), A-ABI-6 (owner policy),
  OQ-SA-1/OQ-S4-3 (engine-scope probes). Everything API-surface-shaped
  is now doc-consistent with the code at the tree this session pins.

---
Task ID: API-FIX-2 (landing record — the four fix commits)
Agent: api-fix lane (this session)

Fix landings:
- A `5c3fb32`: docs/crsf-format.md — REC_FS_CHECK type 9 row + the full
  FS_CHECK payload grammar (fields verified against serialize/deserialize
  _fs_check at src/core/check/cr_fscheck.cpp:1009-1089 + the CHK_* enum
  0x1001..0x100E at cr_fscheck.h:80-97).
- B+C `3c9a747`: the three-site CLI truth pass, first two sites —
  rrecov.cpp header comment (10 verbs in dispatch order + the complete
  scan flag surface + the full exit-code map; comment-only, parse-safe)
  and docs/API.md §1 (both missing verb lines, scan --image/--deep/
  --repair-undo, the phantom [plan] value removed, recover --image,
  --boot-fix added to the consent list per its M-040 shared-apply gate,
  duplicate exit row 3 collapsed).
- D `8ff17fc`: docs/CORE-API-CURRENT.md — §c 10-verb truth + the stderr
  exception (require_driver is the only fwprintf(stderr) surface) + all
  function-range cites re-pinned + REMOVED --no-rda dropped + the
  rda-only transport label; the fix grew into the §a/§b/§d refresh
  (phantom prefer_rda signature, deleted rda_available/win_pe_environment
  rows, 51 stale cr_ui.h cites mechanically re-pinned; the header now
  pins the live-section cite refresh, §e/§f keep their 22e9657-era
  cites under the banner).
- Board: diff-to-fix/API-DELTA.md §4 items A-D flipped FIXED with the
  commit refs; the three-site verb-list pin stands
  (CliVerbListsAreThreeSiteLocked).

Verification: ci-local doc-links / architecture lint (the driver-only
blocklist — which independently confirms the rda_available /
win_pe_environment / prefer_rda removals this session's §a edits made) /
secrets / commit-convention lanes all green; cmake + clang-format are
absent in this container so the build/test lanes could not run locally —
the only code-file change is comment-only and parse-verified (the
windows.h include is the sole expected parse stop on Linux).

---
Task ID: API-FIX-4 (the M-070 convergence note — this lane's diagnostic half)
Agent: main engineer (api-fix session)

Work Log:
- Reproduced the 40673882330 failure shape end-to-end: staged-truth.json's
  "manifest_version": null + "expected_recoverable": null (the if/else
  @() unwrap to $null, proven in pwsh 7.4.6) + 0.02 s seed-window =
  $manifest was null-or-string on the runner; every static reproduction
  of the invocation (hashtable splat, relative path, LF+CRLF, dot-source
  topology) parsed 11 files, isolating the cause to the script body.
- The api-fix-2 lane landed the mechanism (89dd7ad): `$manifest` is
  `$Manifest` case-insensitively - the SAME variable as the [string]
  param, whose type constraint coerced the parsed PSCustomObject back to
  a STRING on assignment (.files/.version -> null). My loud-gate variant
  (47c7ee5, discarded in the rebase) was the complementary guard; the
  rename is the root fix and the lane's finding file is canonical.
- This lane's residual contribution: the mechanism-isolation evidence
  (the null-vs-empty output-shape distinction + the empty-string
  ConvertFrom-Json silently-yields-null proof) now lives in the finding
  trail via this entry.

Stage Summary:
- M-070 closed by the api-fix-2 lane's rename (89dd7ad); staged-scenarios
  should now stage 11 files - the lane's next dispatch is the proof.

---
Task ID: PARITY-FIX-1 (senior-dev session — the recall-gap register truth pass + GAP-S6-5's scorer action)
Agent: main session (25+yr senior-dev persona, full 3-repo context recovery done)

Work Log:
- Context recovery protocol executed: 6 parallel agents over the research
  corpus (architecture docs / artifacts / og-codebase / reversed+raw-evidence),
  core_recovery (full tree + board), rda-driver (all 66 files). Board at
  session start: 1 active SEVERE (S-038 steps 2-4) + 2 tracked-open
  (OQ-S4-3, OQ-SA-1) + I-026 informational.
- Verified bench/root-causes.json (filed 2026-10-24) against current code:
  ALL SEVEN engine-side recall gaps are already fixed in code with tests
  (GAP-S6-1/2 = 9fc82b0; S6-3/4 = 7797bd6; S6-6/7/8 = 03ddcd2 + F-1 cd9ea16)
  - the register, CORE-GAP-LIST.md rows and PERFORMANCE-PROFILE.md rows
  were all STALE (predate the landings).
- GAP-S6-5's action landed (the only open item): the physical-ceiling
  metric. volume.ps1 gains the HashRanges composite C# op (one locked
  pass, per-file extent-range SHA-256, short-read honesty) +
  Invoke-PhysicalCeilingProbe; apply-scenario's decay branch emits
  physical-ceiling.json (seed-time extents are valid post-decay addresses
  - deletion never moves bytes); the cluster-size resolution is extracted
  to Resolve-ClusterBytes shared by partial_overwrite + decay (logic
  byte-identical, BENCH-F4c/F5c lessons kept); compute_metrics --ceiling
  adds the physical_ceiling block (compute_ceiling(), recall judged
  against the physically-intact set); compare.py surfaces the ceiling on
  BELOW rows; tools/test_ceiling_metric.py (5 cases, 16 assertions) wired
  into ci-local step 5.
- Truth pass: root-causes.json every entry now carries a resolution field
  with the landing commit (original evidence kept verbatim; the decay
  entry carries GAP-S6-5's root-cause CORRECTION - physical destruction,
  not an engine defect); CORE-GAP-LIST.md S6-1..S6-8 all flipped CLOSED
  with commit refs; PERFORMANCE-PROFILE.md rows annotated fixed-in-code
  (numbers stand until the next bench dispatch re-measures).

Stage Summary:
- The engine-side recall-gap register is fully closed in code AND the
  registers now tell the truth. Remaining session targets: S-038 steps
  2-4 (installer, per PACKAGING.md defaults), M-068 board hygiene move,
  testing.md count truth, OQ-SA-1 NTFS +0x64 decompile probe, OQ-S4-3
  TTFR probe target.
---
Task ID: SCAN-2f (Agent 2 — scan-engine lane: the context-recovery re-run + the OQ-SA-1 full closure + the S-007 niche batch)
Agent: Agent 2 (scan-engine lane)

Work Log:
- Context recovery per protocol: FOUR parallel read legs (corpus
  carve-family census/dual-registry; corpus NTFS decompile trace; core
  carve-state audit; rda-driver scan-surface audit). SCAN-ENGINE-DELTA.md
  re-verified + refreshed at this tree (A4 count reconciliation: 131
  roster rows = 106 analysers + 7 FileInfo classes + 18 infra; the
  "67 of 131" residue figure replaced by the precise ~30-format work
  list; N-068/N-069/N-070/S-032/S-037 sub-items re-verified FIXED; F2's
  D-104 registration recorded; C9's probe-target correction folded in).
- OQ-SA-1 NTFS +0x64 — DECODED, instruction-decisive, first-hand: the
  low dword of the FIRST $DATA extent's starting LCN (NOT parent-ref,
  NOT own record number). Producer chain: FUN_1800069e0 :5126-5127
  (src+0x53 <- first pair start from FUN_1800336f0 :41028; src+0x4f <-
  Sum pair seconds = total clusters) -> FUN_180003020 :1986-1988
  (dst+0x64/dst+0x60); the $FILE_NAME path FUN_18002a340 :33912 ZEROES
  the slot (local_39 = src+0x4f/+0x53 as one u64). Engine-side +0x60 =
  total data-run clusters (the "+0x60 type-registry id" census reading
  is the DRWUI wire-side overwrite, part1:86). The FAT +0x3e / exFAT
  +0x50 / NTFS +0x64 anchor triple closes; .rsf persists +0x64
  immediately before the type id (DRWUI_decompiled.c:853462-853464).
  Core carrier + nflags bit-8 wire + fail-before tests = the F-lane fix
  this session; corpus erratum row queued (part1:85 "secondary ID").
- Driver leg (read-only): rda-driver v0.2.4 wire byte-stable (identity
  0x60002400/04/08 + extended 0x6002440/44/48 + all 19 session codes
  re-verified; F11 x512; NAS/Cloud exclusion holds). Driver-repo D3/D4
  doc drifts + the FileVersion-1.0.0.0-vs-tag note recorded as handoffs
  in SCAN-ENGINE-DELTA.md (single-repo commit discipline held).
- handoff: PARITY-FIX-1 lane — your "remaining session targets" list
  carries "OQ-SA-1 NTFS +0x64 decompile probe": that probe is RESOLVED
  by this lane (the decode above; the Delta Report Q11 row + the
  tracked-open file update land with this lane's F-lane fix). No
  further +0x64 work needed; OQ-S4-3's TTFR half stays
  execution-bound (the probe-target correction 97cc3b2 governs).

Stage Summary:
- Scan-engine board after context recovery: quick lane clean (zero open
  defects); A-lane work = the S-007 niche batch (keyed set, corpus-verified
  magics, batch-1 scope pinned in the queue); C-lane clean except the
  execution-bound TTFR half; F-lane work = the NTFS origin-LCN carrier.
  Fixes land next in the mandated order: A-lane batch -> F-lane carrier.

Fix landings (same session):
- A-lane S-007 NICHE BATCH 1 LANDED (e018bf4): 27 og-only formats / 31
  registry rows / 26 validators - MID OGG RM MXF DMG(BZh1) X3F MRW(x3)
  WPG WPD PSP SIT GZ APE AU AIFF HLP(x3) ICO CUR WMF-PL WMF-STD TTF MDB
  TAR EPS AI DXF 3GP. Every key re-read first-hand at its RAWSearchDLL
  registration site before the row landed; WPG bounds {20, 0x5F5E100} and
  MDB bounds {0x1000, 0xBEBC200} verified at their FileInfo ctors; the
  WMF-placeable checksum convention decoded (the stored word = XOR of
  words 0..8). Three census corrections: errata E49 (3F5F03 = Windows
  Help, not NEF), E50 (DXF is a keyed u32 format), E51/E52 (MDB rides the
  u16 0x0100 chain shared with TTF). Shipped mechanical gates:
  static_assert(kTrialCount == FMT_COUNT) (trial-order membership was
  silently unverified), kMaxTrialSlots 96->160, the DMG footer-scan seam
  made forward-progress-safe (an infinite-loop class caught by the first
  test run). Fail-before proven (validators neutralized: 3/3 positive
  RED; restored 4/4); full suite 941/0 gcc-ASan; hostile ASan smoke
  extended with the walk-heavy niche families, 400 mutated iters, zero
  findings. Batch-2 remainder pinned with causes in the S-007 addendum
  (CHM/MNY/EMF/ONE/MPEG-TS/3DS/SWF/JP2/MOBI + the list-layer text
  school). handoff: Agent 1 - the CLI's list-carve-formats surface
  enumerates the registry, so the 27 new formats surface automatically;
  no API-contract change required on your side (the carve_format values
  are append-only stable).


---
Task ID: PARITY-FIX-2 (S-038 steps 2-3 — the app installer + the three gates)
Agent: main session (senior-dev parity-fix continuation)

Work Log:
- The two PACKAGING.md owner decisions closed at the contract's own
  documented defaults (the owner may overturn either by reopening the row):
  section 2 = Inno Setup (evidence-pinned, the 13-parameter contract maps
  1:1), section 8 = omit (no telemetry endpoint; no stub verbs).
- installer/core-recovery.iss: the 13-parameter implementation. Parameters
  1-5/7/10 native Inno; parameter 6 the admin manifest; parameter 9 the
  frozen single-instance AppId; parameters 11-12 intentionally OMIT (the
  recorded decision); parameter 8 (ARP QuietUninstallString, the reference's
  exact ' /SILENT' form) + parameter 13 (the version key at
  HKLM\SOFTWARE\heicfast\core_recovery\Version) written AND read-back-verified
  by [Code] (the M-064 write-and-hope lesson); the version key's removal
  owned by the uninstaller's [Code] hook (Inno only uninstalls [Registry]
  entries). OS gate (section 6 modernized envelope, Win7 SP1 -> Server 2025,
  x64-only) in InitializeSetup; driver deployer (section 10) as the default
  'driver' component - install.ps1 stays the SCM source of truth, exit codes
  propagate (failed driver install fails the app install), teardown before
  file deletion.
- Caught + fixed pre-push: the [Code] ARP-key string must use the raw GUID
  (AppIdRaw define) - the {{-escaped AppId define would build a wrong
  registry key.
- installer/osgate.ps1: the shared decision module (injectable version
  tuples; the OSGATE-V1 marker is the sync pin); installer/run-gates.ps1:
  gates 1+2 executable (silent round trip + version key + no-residue:
  files/ARP/version-key/service/driver-image sweep); installer/
  build-installer.ps1: payload staging + ISCC; installer/README.md.
- tools/test_installer_osgate.py: gate 3 - the sync-pin check (comment-style
  agnostic) + the injectable matrix (9 in-envelope ADMITS incl. Win7 SP1
  floor + Server 2025, 5 below-floor REFUSES incl. Win7 RTM/SP0, 3
  wrong-arch REFUSES incl. ARM64/Itanium).
- .github/workflows/installer.yml: the gate lane (gate 3 fast job + PS AST
  parse; the full build -> fetch driver -> job-local test-sign -> ISCC ->
  round-trip job on windows-2022, Inno pinned 6.4.3 via choco). The
  installer PS files joined scripts/ps_parsecheck.ps1.
- PACKAGING.md: status -> IMPLEMENTED; sections 2/7/8/12 record the closed
  decisions + the registered version-key path.
- S-038 finding: steps 2-3 Resolution appended; step 4 (acceptance) flips
  when the lane's first dispatch is green.

Stage Summary:
- S-038 steps 2-3 landed (11 files). Next: dispatch installer.yml and watch
  the three gates; then the M-068 hygiene move + testing.md count truth +
  OQ-SA-1 NTFS +0x64 decompile probe.


---
Task ID: PARITY-FIX-3 (CI red repair + the installer-lane YAML fix)
Agent: main session

Work Log:
- installer.yml: the step name carried an unquoted colon ('the v0.2.3
  lesson: a broken script...') - YAML mapping-value error, zero jobs, the
  lane's first dispatch died instantly (run 40682041579). Quoted.
- CI red repair (run 40680931476, three lanes red on the parallel
  thumbnail landing 05e6786 - not this session's code, repaired per the
  repo's ci-restore-green precedent):
  (1) windows/local-mirror: cr_thumbnail.cpp:479 MSVC C4244-as-error -
  the TIFF bps loop narrowed vector<u64> elements into a u32 range-for
  var; now compares in u64 and narrows once at the assignment with the
  8/16 refusal gate as the safety rationale;
  (2) fuzz: cr_thumbnail.cpp:36 - '-Wmaybe-uninitialized' is a GCC-only
  warning group; under clang -Werror it fails as unknown-warning-option;
  now guarded by __GNUC__ && !__clang__ (clang's equivalent lives under
  -Wconditional-uninitialized; the vendored-stb pragma region keeps the
  rest).
- g++ -fsyntax-only -Wall -Wextra -Wpedantic -Wshadow -Werror: the fixed
  file is clean.

Stage Summary:
- Pushing the repair + re-dispatching the installer lane.

---
Task ID: PARITY-FIX-4 (board hygiene: the M-068 move + testing.md count truth)
Agent: main session

Work Log:
- M-068 moved findings/MAJOR/ -> findings/FIXED/ (git mv, history kept):
  the fix landed in the SA-6 wave (43f4962 + 38ccecb) but the file never
  moved; Resolution section appended; RESOLVED-INDEX row added.
- SUMMARY.md: the PARITY-FIX-3/4 delta entry on top (board: 245 FIXED, 1
  active S-038 awaiting the lane's green dispatch, 2 tracked-open).
- docs/testing.md: the stale '926 tests' -> the current truth (1,014
  TEST/TEST_F macros across tests/unit, 61 files; latest executed green
  1,003/1,003 release+ASan at the thumbnail landing 05e6786).

Stage Summary:
- Board hygiene closed. Watching the installer lane (run 40682340640) and
  the CI validation lane (run 40682340755); next: OQ-SA-1's engine-side
  NTFS origin-LCN carrier if the SCAN-2f lane has not landed it.

---
Task ID: API-EXP-1 (api-fix session 3 — the corpus-parity expansion: context recovery + the plan-of-record board)
Agent: api-exp lane (this session)

Work Log:
- Context-recovery protocol run 3, API scope only: three parallel read
  legs (corpus be0cfe9 / core 89dd7ad / driver 323e75a) -> the session-3
  board committed as diff-to-fix/API-DELTA.md §5.
- Standing state re-verified first-hand: 79 exports / v3 / 39+OK / 10
  verbs all stand; driver wire byte-exact (the SAME 19 session-code
  values both sides — the driver worklog's "18-code" phrasing counts the
  dispatch set minus the AC0 discovery probe; reconciled in the board).
- Reconciliation: the S-036 fail-loud + FloorVersion close-out the rda
  driver worklog names as "core_recovery-side" is ALREADY landed here
  (fetch-release.ps1 semver floor gate :59-65, install-driver composite
  default fail-loud + FloorVersion v0.2.3, release/staged floors) — the
  driver lane's line is stale, recorded for its next sync. A-ABI-6's
  RELEASE-NOTES half is gone (no CodeQL claim at this tree); the stale
  half is the audit row itself (S3-1). M-068: the hygiene move was
  landed by the board-hygiene lane at this rebase (2a556e5 — the FIXED/
  move + the Resolution + the RESOLVED-INDEX row); folded, no action.
- Rebase note: while this session filed, the S-038 lane landed steps 2-3
  (3e6a8fa, the Inno installer + the three test gates) and SCAN-2f
  decoded the OQ-SA-1 NTFS +0x64 half (7bda022) — both folded into this
  lane's board (API-DELTA §5.1) at the rebase.
- A fresh local verification lane was bootstrapped in this container
  (cmake 4.4.4 via pip, gtest 1.15.2 FetchContent, gcc 14.2.0 Release,
  clang-format 18.1.8): full suite 977/977 green at the base tree.
- The remaining corpus-parity API work re-based onto the plan of record:
  PLAN-API-EXPANSION Waves 1-5 = 12 ops (S3-W1a..S3-W5a), every one
  verified ABSENT first-hand at 89dd7ad. This session executes them in
  the plan's landing order (W1a session-list first), each op = C++ + ABI
  + CLI + contract text + tests, read-on-demand corpus verification per
  op, committing every 2-3 files.

Stage Summary:
- Board filed (rebase-adjusted); execution starts at W1a (R-56 session
  list + summary). This lane's queue: S3-1 (the A-ABI-6 audit-row truth
  pass) then Waves 1-5; NOT touching S-038's installer lane, SCAN-2f's
  engine probes, or API-FIX-4's CI lane.

Task ID: PARITY-FIX-5 (installer-lane retry fix + OQ-S4-3's TTFR harness)
Agent: main session

Work Log:
- installer.yml choco step: the windows-2022 image ships Inno Setup 6.7.1
  PREINSTALLED - the pinned 6.4.3 install downgrade-failed (run
  40682340640). Now detects ISCC first, choco unpinned only when absent;
  the 6.4.3 references swept from ISS/build-installer/README/PACKAGING.
  Gate 3 (InstallerOSGateMatrix) was GREEN on that run - the sync pin +
  the full envelope matrix passed on the runner.
- Next: the OQ-S4-3 TTFR harness on the staged-scenarios lane (option 1
  of the 97cc3b2 probe-target correction - no engineering prerequisite,
  the event stream carries the timestamps).

Stage Summary:
- Retrying the round-trip job; TTFR extraction next.


---
Task ID: CI-REBUILD-WIN (the owner-ordered windows-only 3-lane consolidation)
Agent: the CI rebuild session (context recovery protocol, API scope)

Work Log:
- Full context recovery first: 5 parallel explore agents (build system,
  CI autopsy, CLI+API contracts, driver+bench infrastructure, research
  corpus) - every contract re-verified first-hand against the tree before
  a line was written (zero-invention bar).
- WIPED: ci.yml (7 jobs incl. 4 ubuntu lanes), core-install.yml,
  driver-install.yml, staged-scenarios.yml, the old release.yml,
  .github/scripts/{stage-disk.ps1,verify-scenario.ps1},
  .github/scenarios/seed-manifest.json, .github/actions/install-driver
  (the owner directive: windows runners only; the ubuntu sanitizer/fuzz
  lanes are documented drops, coverage stays local via ci-local.sh).
  KEPT: installer.yml (the S-038 lane landed mid-session on the moved
  main - fresh active work, independent of anything wiped).
- LANDED the 3-lane set: checks.yml (lint fast-fail -> build+test with
  vcpkg binary caching + Win7 posture + G-09 + driverless refusal probes
  + the new PS-AST and workflow-YAML parse gates), release.yml (validate
  via workflow_call -> resolve -> build-core + driver-tier -> assemble
  + publish; PERSISTENT dev-cert signing replacing the ephemeral
  job-local cert, public cert shipped as rda-test-sign.cer), and
  test-benchmark.yml (single elevated windows-2022 job: release fetch ->
  driver trust/install/verify -> api_harness.c compiled against the
  release SDK -> New-TestDisk (diskpart-only fixed VHDX, MBR+GPT,
  FAT32/NTFS/exFAT, dual-partition) -> Run-Benchmark orchestrator over
  benchmark-matrix.json: deterministic 23-format corpus, 10 scenarios,
  CLI + API lanes, floor/discovery/observe gates, apply probe,
  cancel+resume probe, bench verb + clean-volume timings) with
  Teardown-Benchmark enforcing the M-061/M-069 contracts and the
  structured 00..99 evidence artifact.
- Secrets installed: DEV_SIGN_CERT_PFX_B64 + DEV_SIGN_CERT_PASSWORD
  (CN=RRECOV-DEV-SIGN, RSA-2048 SHA-256, codeSigning EKU, 3-year).
- Docs reconciled: CI-CURRENT.md rewritten for the new lane set;
  CI-SCENARIOS.md pointed at benchmark-matrix.json; RELEASE-NOTES entry.

Stage Summary:
- Validation pending: the checks dispatch, then a release publish
  (auto next-patch), then the auto-dispatched test-benchmark against it -
  the empirical loop the lane set itself enforces (a rewritten composite
  with zero dispatches is an unverified composite - the M-068 lesson).

Task ID: PARITY-FIX-6 (OQ-S4-3's TTFR harness — the event-stream probe)
Agent: main session

Work Log:
- rrecov scan: TtfrProbe + ttfr_event_fn — ONE log line at the first
  positive FilesFound event (run-elapsed ms from the event's
  pause-compensated timestamp_ms; the honest granularity: counters
  coalesce at stage boundaries). Set before the ScanController
  construction (the controller copies cfg — caught by review); atomic
  once-flag (worker threads); no new CLI flag (the existing --log).
- verify-scenario.ps1: --log engine-log.txt + the ttfr block in the
  evidence JSON + the printed line; the evidence upload covers it.
- OQ-S4-3 tracked-open file: the harness section appended; the
  measurement is the lane's next dispatch (execution-bound).
- Gate trail: win_syntax_check green (it caught two relocation defects
  before the push — the parse gate working as designed); ps_sanity OK.

Stage Summary:
- The last code-side item this session could land without colliding with
  the parallel lanes' pinned queues. Watching: the installer round-trip
  job + the CI lane at 624db00.

---
Task ID: PARITY-FIX-6b (the TTFR extraction port to the rebuilt CI lane)
Agent: main session

Work Log:
- The owner's CI consolidation (566c483) landed mid-flight: checks/release/
  test-benchmark replace the 5-workflow set; staged-scenarios.yml +
  verify-scenario.ps1 are GONE. The rebase: verify-scenario.ps1 deleted
  (their side wins - the new lane owns the harness); the rrecov probe kept
  (harness-agnostic).
- The extraction PORTED to Run-Benchmark.ps1: the per-mode scan block
  (which already ran every scan with --log stage-<mode>.log) now extracts
  ttfr_ms/ttfr_first_files into $cleanPerf.scans[] -> the evidence tree's
  clean-volume-benchmark.json (+ the stage logs already land in evidence).
- The OQ-S4-3 file's harness section corrected to name the new lane.

Stage Summary:
- The TTFR probe + extraction now ride the owner's lane of record. The
  measurement = its next dispatch.

---
Task ID: PARITY-FIX-7 (gate-1d quote-canonicalization fix — 14/14 install-side assertions GREEN)
Agent: main session

Work Log:
- Run 40682713693 (the installer round-trip at the fixed Inno step): gates
  1a-1c + 2 ALL GREEN — install exit 0, version key write+verify
  (1.0.0-gate.3), ARP UninstallString + QuietUninstallString (ends
  " /SILENT") + DisplayName/DisplayVersion/Publisher/InstallLocation,
  driver service RDA RUNNING with the Boot File System group (the
  PACKAGING section-10 internal step proven end-to-end). The build, ISCC
  compile, and /VERYSILENT zero-prompt install all work.
- The single failure: gate 1d's exe parse. Inno writes UninstallString
  QUOTED ("C:\Program Files\..."); my ISS built '"' + quoted + '"' =
  ""path"" /SILENT; the gate's Substring parse then produced an empty
  FilePath. Fixed BOTH sides: the ISS strips UninstallString's surrounding
  quotes before composing the canonical single-quoted form; run-gates
  normalizes doubled quotes defensively, splits exe/args robustly (the
  $args automatic-variable trap avoided), and validates the exe exists
  before Start-Process.

Stage Summary:
- Gate 1d/1e (uninstall + residue) re-proven on the next dispatch — the
  only unproven half of the round trip.

---
Task ID: PARITY-FIX-8 (the fuzz-lane red repair: the stb EOF protocol + the UBSan nonnull)
Agent: main session

Work Log:
- Run 40682713696's fuzz job (the thumbnail landing's first fuzz
  exercise; the earlier CI red at 40680931476 never reached the campaign):
  (1) fuzz_thumbnail TIMEOUT at stbi__skip_jpeg_junk_at_end
  (stb_image.h:3393) - the mechanism: StbioSource::read_cb returned 0 on
  budget exhaustion WITHOUT setting eof_; stb's junk-skip loop keys on
  at_eof, so a budget-stopped stream that keeps reporting not-at-EOF
  while reads return 0 spins forever. Fixed: budget exhaustion now sets
  eof_ (budget exhaustion IS an EOF for stb's stream protocol).
  (2) fuzz_preview.cpp:41 UBSan nonnull - memcpy(buf, data_.data()+off,
  0) with an empty data_ (null data()); guarded by take > 0.
- Both files g++ -fsyntax-only clean under the repo's full warning set.
  The next fuzz dispatch (the new Checks lane) is the proof.

Stage Summary:
- All three lanes of the earlier red repaired (windows/local-mirror build,
  fuzz build, fuzz campaign). Watching the installer round-trip at
  1c4db82 (gate 1d/1e proof) + the new Checks lane.

---
Task ID: PARITY-FIX-9 (S-038 step 4 ACCEPTED — the board's last active SEVERE closed)
Agent: main session

Work Log:
- Run 40684304129 (1c4db82): the installer lane's first fully-green
  dispatch - gate 3 (sync pin + 17 probes) AND gates 1+2 (all 18
  assertions: install/version-key/ARP-both-strings/driver-internal-step/
  quiet-uninstall/no-residue). The full list is in S-038's Resolution.
- S-038 moved SEVERE/ -> FIXED/ with the step-4 Resolution; RESOLVED-INDEX
  row added; SUMMARY's board: 0 active, 246 FIXED, 2 tracked-open;
  PACKAGING.md status -> ACCEPTED.

Stage Summary:
- Session board outcome: the last active SEVERE closed; every engine-side
  recall gap verified fixed + registers truthful; the TTFR harness landed;
  three CI-red classes repaired. Remaining tracked-open items are owned
  by the parallel lanes (SCAN-2f's origin-LCN carrier) or execution-bound
  (the TTFR measurement on the rebuilt lane's next dispatch).

---
Task ID: PARITY-FIX-10 (session close-out)
Agent: main session (senior-dev parity-fix)

Work Log:
- Test & Benchmark lane (run 40684560877): the failure is the lane's own
  documented expected state - the latest release (v0.5.1) predates the
  combined era and lacks rda-test-sign.cer; the error message itself
  prescribes the remediation (publish the first combined release per
  docs/RELEASE.md and re-run). An owner tag-push action, not a defect.
- Session ledger (all pushed): d0a102c (GAP-S6-5 scorer + register truth),
  3e6a8fa (S-038 steps 2-3), d9b6862 (CI-red repair #1 + YAML colon),
  2a556e5 (M-068 move + testing.md), 47598bf/3307980 (TTFR harness +
  port), 1c4db82 (gate-1d quote canonicalization), 7a6374a (fuzz red
  repair), 51633dd (S-038 FIXED close-out). Plus the rebase-resolved
  merges with the SCAN-2f / api-exp-1 / CI-rebuild lanes.
- Session outcome: the board's last active SEVERE closed (S-038, all
  three gates green, run 40684304129); all seven engine-side recall gaps
  verified fixed with truthful registers; the physical-ceiling scorer +
  TTFR harness landed; three CI-red classes repaired; board = 0 active /
  246 FIXED / 2 tracked-open (both owned: SCAN-2f's carrier queue; the
  rebuilt lane's TTFR measurement dispatch).

Stage Summary:
- Handoffs left on the board: (a) the origin-LCN carrier (SCAN-2f F-lane),
  (b) the TTFR transcription after the next test-benchmark dispatch,
  (c) the first combined release (owner action; unblocks the
  test-benchmark lane + the installer payload's signed-driver posture).
Task ID: SCAN-3 (Agent 2 — the scan-engine context-recovery re-run + the OQ-SA-1 NTFS decode + the fix queue execution)
Agent: Agent 2 (scan-engine lane)

Work Log:
- Context recovery per protocol: four parallel read-legs (research quick/
  advanced spec + NTFS +0x64 probe; research carve census; core state audit;
  rda-driver scan surfaces). The refreshed Scan Engine Inventory / Delta
  Report is diff-to-fix/SCAN-ENGINE-DELTA.md (session-3 resolution log +
  updated fix queue; every load-bearing claim re-verified first-hand in the
  decompiles before acting).
- OQ-SA-1 NTFS +0x64 — DECODED, instruction-decisive: +0x64 = low u32 of the
  FIRST $DATA RUN'S STARTING LCN (the vtable+0x30 location-anchor family —
  NTFS sibling of the landed FAT +0x3e / exFAT +0x50 carriers); +0x60 = low
  u32 of TOTAL $DATA RUN LENGTH IN CLUSTERS (sparse-zeroed). The N-050/N-059
  "+0x60 type-registry id" census reading is REFUTED three independent ways
  (encoder outputs at every writer site; the +0x20 vtable getter is a
  return-0 stub for 8 of 10 per-FS classes; the census's per-extension value
  multiplicity is allocated-clusters behavior). Decode chain: FUN_1800336f0
  :41028/:41043 -> FUN_1800069e0 :5126-5127 -> FUN_180003020 :1987-1988 ->
  attach twin FUN_180033250 :2264-2265 -> serializer FUN_180003440 :2362.
  The prior session's "local_41 -> +0x53" trace premise was the wrong stack
  slot (local_41 @ raw+0x47 feeds +0x58; the parent ref exits via *param_4,
  never stored). OQ-SA-1 CLOSED (all three anchor halves).
- handoff: corpus-lane — the corpus artifacts carrying the refuted reading
  need the correction rows (ifileproperties-part1--ntfs-fat-exfat.md
  +0x60/+0x64/+0x40 rows; rsf-format--scan-session.md :385;
  ntfs-engine-algorithms-v2--deleted-viability.md's vtable+0x28
  "record_size" label — it is the SECTOR size). Core-side errata rows are in
  diff-to-fix/CORPUS-ERRATA.md (this session's batch).
- handoff: api/packaging lane — board hygiene: M-068's finding file still
  sits in findings/MAJOR/ while its own text records the fix (43f4962 +
  convergence 38ccecb); SUMMARY's board count (1 active · 244 FIXED) does
  not include it. Move to FIXED/ + amend the count. No scan-engine content
  involved.
- handoff: driver lane — rda-driver verified clean at v0.2.4 (323e75a):
  19 live session codes vs the README's stale "15 commands" prose (header +
  switch authoritative); the versioned image suffix derives from PE version
  1.0 (rda10.sys), not the v0.2.4 tag. Zero NAS/Cloud paths confirmed. No
  core scan-engine change required.
- S-007 remainder scope locked: the s3 carve census re-verified the niche
  work list against the registry decode and absorbed 8 census-artifact
  corrections (DXF="  0\r" u32 + u16 "99"; EPS/AI=C5D0D3C6 + u16 "%!";
  OneNote=E4 52 5C 7B id 0x31; II/MM\x1A=CRW not MRW (MRW=\0MRM id 0x66);
  placeable WMF=D7CD C69A same analyser as std WMF; ICO/CUR keys 00 00 01 00
  / 00 00 02 00 (both id 0x22); 00 01 00 00=TTF; u16 MM=3DS id 0x01; WPG
  id 0x33 with 0x45 = the SIZE stamp, erratum E48). Full key/grammar table
  in SCAN-ENGINE-DELTA session-3 log. Fix order: quick scan first (the NTFS
  carrier, landing next commit), then the per-school carve batches.

- Rebase reconciliation: the parallel SCAN-2f A2 leg (7bda022) decoded the
  SAME +0x64 value in the same window — the two legs' evidence merged in
  SCAN-ENGINE-DELTA.md (their DRWUI-twin cite + resident-wrapper caveat
  kept; their "+0x60 = DRWUI wire-side overwrite" phrasing CORRECTED by
  this leg's re-read of FUN_140520d20 DRWUI:853463-464 — the twin
  serializer persists the ENGINE-POPULATED +0x64/+0x60 verbatim, no
  overwrite exists at that site; the census values are
  allocated-clusters behavior). Their A4 count reconciliation (131 = 106
  analysers + 7 FileInfo + 18 infra) folded in. Their queue pinning
  (A-lane niche batch -> F-lane NTFS carrier) merged into this session's
  execution order.
- Also swept in this rebase: the stray '>>>>>>> 32a14d1' marker the TTFR
  lane left inside worklog.md on origin/main (same class as the orphaned
  marker 47598bf swept before; no content lost — the surrounding API-FIX-4
  entry is intact).

- OQ-SA-1 LANDED (1285be3, rebased to eac8e1d): tree::FileProperties::
  ntfs_origin_lcn + both walk stamps (main + GUID-dump) + clone() carry +
  nflags bit-8 persistence; fail-before proven first-hand (stamps
  neutralized -> engine tests RED, session round-trip green); full suite
  1010/1010 green pre-merge, 1012/1012 at the merged tree (gcc-13 ASan,
  this container). OQ-SA-1 is now FULLY closed: decoded, carried,
  persisted, tested.

Stage Summary:
- Delta report refreshed + committed; OQ-SA-1 closed at the decode level;
  the remaining scan-engine work is exactly the S-007 niche-format batches
  (advanced/deep), the two stale docs/PARAMETERS.md §3 rows (formats/
  parameters), and the execution-bound OQ-S4-3 (stays tracked). Combination
  layer audit-verified present — no action.

---
Task ID: API-EXP-1 (W1a landing record — R-56 the session list + summary)
Agent: api-exp lane (this session)

Fix landings:
- Core half `78a6552` (feat(session)): the VolumeInfo v5 tail (u64
  save_unix - our mirror of the og record-2's SYSTEMTIME scanTime,
  rsf-format §5.2 +0x6C; v1..v4 files read 0 = "not recorded"), the
  save-path stamp + the load pass-through, Reader::peek_record (the
  header-only type peek - same offsets table + error codes as
  read_record), ScanController::read_summary (light records only;
  heavy payloads skipped BY TYPE before any payload I/O; the legacy
  no-VolumeInfo rule = all-stages configured), the SessionSummary
  carrier, and the crsf-format.md VOLUME_INFO grammar section (the
  doc had NO volume-info section at all - now v1..v5 with every
  tail's length-carries-version rule). Tests: SaveUnixVTailRoundTrip
  (the 1024B v5 pin + the v4/v1 cut points), PeekRecordHeaderOnly,
  ReadSummaryLightRecordsOnly (2MB tree record present; summary
  fields exact), LegacySessionWithoutVolumeInfo; the
  VolumeInfoIdentityCardRoundTrip v4-size pin moved to the v5 total
  (1036B) with the wire.
- Surface half `eb16335` (feat(api)): cr_ui_session_list (cr_ui.h:808-
  850, two-call size pattern == snapshot_dfs, byte-sorted caller-owned
  POD rows, error rows for unreadable files, path-truncation rule,
  ERR_OPEN_FAILED fails loud on the directory), the rrecov `sessions
  [--dir D] [--json]` verb (FROZEN schema; unreadable rows are data
  never failure; enumeration failure exits 1), the C-consumer gate
  block, the parse-gate stub growth (WIN32_FIND_DATAW + the
  FindFirstFileExW family + MSVC-CRT localtime_s - the gate's own
  new-SDK-symbol rule), the three-site verb lock honored in one
  change (rrecov.cpp header + API.md §1 + CORE-API-CURRENT §c, c.3 =
  sessions, c.4..c.10 renumbered), ABI-CONTRACT §7f, every count site
  (79->80 exports, 10->11 verbs), and 39 cite re-pins (18 cr_ui.h +
  21 rrecov.cpp, every anchor re-verified first-hand, script-asserted).

Verification: 1009/1009 green locally (gcc 14.2 Release, incl. the
win_syntax_check parse gate), clang-format-18 clean, secrets/lints
clean. Board: API-DELTA §5.2 row S3-W1a flipped DONE. Next: W1b
(R-54 the session load progress + cancel - the corpus contract is
SaveScan:344-376, restore 40+50*done/(total+done) + per-child
IUIProgressInfo cancel; the N-060 save mirror).

---
Task ID: API-EXP-1 (W1b landing record — R-54 the session load progress + cancel)
Agent: api-exp lane (this session)

- Landed `ce6bcb0` (feat(session)): the load-side twin of the N-060 save
  machinery, corpus contract SaveScan:344-376 verified first-hand (the
  restore progress window + the per-child IUIProgressInfo IsCancelled
  teardown). Core: session::DeserializeProgress per attached node,
  ERR_CANCELLED propagating through the container layer (the
  REC_RESULT_TREE case explicitly refuses to flatten a cancel into
  SESSION_FORMAT). ABI: cr_ui_session_load_with_events (the caller-owned
  zero-init cancel flag IS the og's polled progress object - set from
  any thread; *out untouched on abort) + CR_EVT_SESSION_LOAD_PROGRESS
  (type 11, coalesced). Docs: ABI-CONTRACT §7g + the §6 event row +
  81-export counts + the cr_ui.h cite set re-mechanized for the +15/+21
  header inserts (all anchors grep-verified). Tests: 2 unit + 1 ABI +
  the C-gate NULL/NULL contract smoke; suite 1012/1012.
- Board: API-DELTA §5.2 row S3-W1b flipped DONE. Next: W1c (R-52 the
  auto-save-on-completion) - the corpus contract is SaveScan:443-451 +
  section 5.5 (auto_<vol>_<M>_<D>_<H>_<m>_<s> naming, the N-061
  auto_session_name already landed; what is missing is the ScanConfig
  trigger + the completion write + the event/stats surfacing).

---
Task ID: API-EXP-1 (W1c landing record + the Wave-1 completion note)
Agent: api-exp lane (this session)

- Landed `c95335f` (feat(scan)): R-52 the completion auto-save. Wave 1
  of the plan of record is now COMPLETE - W1a (R-56 the session list,
  78a6552/eb16335), W1b (R-54 the load progress + cancel, ce6bcb0),
  W1c (R-52, c95335f) - all three SaveScan-parity rows flipped DONE on
  the S3 board, every landing corpus-cited and locally green.
- The remaining plan-of-record board for the next session/lane: Wave 2
  (S3-W2a the query path_substring + state_mask axes, R-22; S3-W2b the
  scan-time folder filter, R-14), Wave 3 (R-41 dest codes, R-65 chkdsk,
  R-86 doc sentence, R-63 TRIM facet), Wave 4 (R-34 preview caps
  defaults, R-93 image create), Wave 5 (R-88 BitLocker turn-off).
  Session state at handoff: suite 1016/1016 on the local gcc-14 lane,
  all counts truthful (81 exports / 11 verbs / 39+OK), the three-site
  CLI lock held on every verb/flag change.

---
Task ID: SA-8/T1 (session-8 open — baseline record)
Agent: session-8 full parity audit (this session)

Baseline (cloned fresh this session):
- core_recovery @ 35adcb93cf433af4339ee7bdb4c7f5e3e13e8fcf (main) — merged
  product repo (confirmed: core engine + win layer + audit trail in one tree;
  driver ships as prebuilt rda.sys release asset).
- rda-driver @ 323e75acf4cd745960813af4e7fd586a4a34d1a0 (main, v0.2.4 — the
  M-064 remainder landed).
- corpus easeus-drw-research @ be0cfe95ecf22ac67e0f3a823dd003163b196d38 (main)
  — ONE commit past the trail's standing 03ad46c: CORRECTIONS.md #9
  (EUUserData -> CHKRepairFileSign; GetRawUserInfoMap = per-scan
  carve-statistics map). Absorption verified first-hand: CORPUS-ERRATA E-A/E-B
  + PARITY-DELTAS D-104 + SCAN-ENGINE-DELTA F2 row already carry it.

Session scope determination (from the trail + corpus state, not the prompt):
the board reads 0 active / 249 FIXED / 1 INFORMATIONAL (I-026) / 1 tracked-open
(OQ-S4-3, Windows-execution-bound). New audit surface since the last audited
baselines: (a) product code landed since the dev-sign close and the SCAN-2f/3
context legs — API-EXP W1a/W1b/W1c (78a6552/eb16335, ce6bcb0, c95335f),
SCAN-2f/3 OQ-SA-1 carrier (eac8e1d/1285be3), S-007 niche batch-1 (e018bf4),
thumbnail engine (05e6786), installer S-038 steps 2-3 (3e6a8fa), CI rebuild
(566c483) — each claims corpus contracts; this session verifies those claims
read-only. (b) The S-007 batch-2 remainder (~14 corpus-documented analyser
formats absent, causes pinned in the FIXED addendum — a live recall gap
recorded outside an active finding). (c) The driver-lane handoffs D3/D4
(recorded not fixed). (d) Gate-doc currency: SURFACE-INVENTORY/PARAMETERS cite
corpus 03ad46c; the corpus moved.

Prompt saved redacted: worklog/session-8-parity-audit-prompt.md. Read twice.

---
Task ID: SA-8/T2 (session-8 wave 2 — the continuation mandate: find what prior
sessions missed, verify the new landings, mark every remaining capability as
a finding)
Agent: session-8 full parity audit (this session, second leg)

Session mandate (user): "look for other things which session 1 missed and
continue working and remove the fixed things and go more deep, do more
comparison on og-codebase and our core and mark all remaining features,
implementations, functions, as finding too." Two-repo scope THIS session:
core_recovery (ff879f9 lineage, read-only) + easeus-drw-research (be0cfe9).
The rda-driver lane handoffs D3/D4 stay OUT of this session's scope (the
two-repo mandate; they remain that lane's items on its own repo).

Wave-2 lanes (from the T1 scope determination + the continuation mandate):
- V-lane: read-only verification of the post-dev-sign landings' corpus
  claims (API-EXP W1a/W1b/W1c; OQ-SA-1 carrier eac8e1d; S-007 batch-1
  e018bf4; thumbnail 05e6786; installer S-038 steps 2-3; CI rebuild 566c483).
- R-lane: the S-007 batch-2 remainder becomes PROPER FINDINGS (the live
  recall gap currently recorded outside an active finding) — decode what is
  decodable at the og registration sites first (CHM/MNY/EMF/ONE/MPEG-TS/
  3DS/SWF/JP2/MOBI/LZH + the text school + FCPEvt/OfficeTagFromXML/CRW).
- G-lane: the gap hunt — corpus artifacts + og-codebase surfaces the prior
  sessions' waves never dispositioned; core capabilities without corpus
  basis (superset register); thumbnail engine vs corpus preview evidence.
- C-lane: gate-doc currency (SURFACE-INVENTORY/PARAMETERS/README corpus
  pin 03ad46c -> be0cfe9; board counts; CROSS-REF-MATRIX new rows).

Baselines: core ff879f9 (suite green per the landing lanes' records),
corpus be0cfe9 (CORRECTIONS #9 absorbed per T1). Board at open: 0 active /
249 FIXED / 1 INFO (I-026) / 1 tracked-open (OQ-S4-3). ID space continues:
next A-012+, S-040+, M-071+, N-090+, I-027+.

---
Task ID: CI-REL-1 (the CI-red repair + the v0.5.2 release cut)
Agent: senior-dev session (this session)

Work Log:
- Context: every Checks run since c95335f red on the MSVC lane; the
  release lane red at validate since 40690974024 (three separate
  build breaks fixed by 66404ae/6ab676f/35adcb9 - C4305/C4804,
  clang-format pin drift, getpid C4996); the test-benchmark lane red
  at fetch (40684560877: v0.5.1 predates the combined era, no
  rda-test-sign.cer asset).
- The last remaining red: run 40694592245 (35adcb9), test 980
  CrUi.AutoSaveOnCompletion at test_cr_ui.cpp:396 - the FIRST block's
  cr_ui_session_list on the just-created EMPTY autosave dir returns
  0x50020001 (ERR_OPEN_FAILED) on MSVC. Root cause: the Windows
  list_crsf_files maps FindFirstFileExW's INVALID_HANDLE_VALUE to
  false unconditionally; on an empty directory the API fails with
  ERROR_FILE_NOT_FOUND ('searched, nothing matched') which is the
  empty-list SUCCESS - POSIX opendir on the same dir succeeds, hence
  gcc-green/msvc-red (the dominant msvc-only-red class).
- The fix (both surfaces of the seam): cr_ui.cpp's list_crsf_files
  returns GetLastError()!=ERROR_FILE_NOT_FOUND on the invalid handle
  (ERROR_PATH_NOT_FOUND/access-denied stay failures); rrecov.cpp's
  print_sessions takes the same branch - an empty dir prints zero
  rows and exits 0 (mirroring the ABI contract), a genuinely missing
  directory stays FB-K01 exit 1.
- Verification in this container: gcc 14.2 linux-release build clean
  (-Wall -Wextra -Werror), full suite 1020/1020 green including the
  win_syntax_check parse gate (7.05s - the _WIN32 branches of both
  edited files parse against the win-stub SDK) and the AutoSave test
  itself; clang-format 18.1.8 (the pinned CI version) dry-run clean
  on both files.
- RELEASE-NOTES.md vNext gains the fix bullet (the honest surface for
  the v0.5.2 consumer).

Stage Summary:
- Next: push, watch Checks go green (the last known red), then cut
  v0.5.2 (the first release of the combined era with the persistent
  dev-sign: rda-test-sign.cer ships, which is exactly the asset the
  test-benchmark lane's fetch demands), and the release's own dispatch
  continues the test+benchmark pass.

---
Task ID: CI-REL-1b (the 40700058960 correction - the error-code theory died on real windows)
Agent: senior-dev session (this session, second leg)

Work Log:
- The first fix (ERROR_FILE_NOT_FOUND -> empty success) was WRONG on
  real windows-2022: run 40700058960 came back with the observed
  matrix INVERTED from the documented one - the nonexistent
  no-such-dir-w1a/no-such-dir-cgate dirs report ERROR_FILE_NOT_FOUND
  (my key wrongly mapped them to success, breaking test 981 :514 and
  the abi C-gate :570, both pinning ERR_OPEN_FAILED) while the
  existing EMPTY autosave dir reports a DIFFERENT code (test 980
  stayed red at :396). 3 red, worse than the 1 we started with.
- The lesson recorded in the code comment: the FindFirstFileExW error
  code is NOT a reliable empty-vs-missing axis across real systems;
  the version-proof discriminator is the DIRECTORY-EXISTS probe. The
  landed fix (dd225da): on a failed enumeration, GetFileAttributesW
  decides - existing directory = the empty-list success (POSIX opendir
  semantics, the W1c first-block contract OK + n==0), missing path or
  a plain file handed as a dir (POSIX ENOTDIR) = the genuine
  ERR_OPEN_FAILED. Known accepted divergence noted in the comment: a
  dir with denied-list-but-readable-attributes (a rare ACL shape)
  reports empty where POSIX opendir would fail.
- Both surfaces again: the ABI seam and the CLI sessions verb (missing
  dir stays FB-K01 exit 1). Local: gcc 1020/1020 incl. the parse gate,
  format 18.1.8 clean. RELEASE-NOTES vNext bullet corrected to the
  final truth.

Stage Summary:
- Watching run for dd225da; on green, cut v0.5.2 (the combined-era
  release with the persistent dev-sign) - the release dispatch then
  continues the test+benchmark lane against the fresh tag.
---
Task ID: SA-8/T2-EXEC (the T2 plan's execution record — the five lanes ran in this
session; the entries below are the results. The product tree moved under the
audit mid-flight (the CI-REL lane's dd225da et al.); every load-bearing absence
claim was RE-VERIFIED on the final rebased tree before push — see the entry's
tail and the s8 evidence file.)
Task ID: SA-8 (the session-8 deep audit — the second-session continuation; findings filed, boards updated, close-out)
Agent: session-8 full parity audit (this session)

Scope executed (the T1-determined plan + the user's second-session mandate:
find what session 1+ missed, verify/remove the fixed things, go deeper on the
og-codebase comparison, mark all remaining features as findings):

1. **Five parallel read lanes** (og-codebase never-audited asset sweep; the
   W1a/W1b/W1c/thumbnail landing verification; S-007 batch-1 + NTFS carrier +
   FIXED-truth spot-check; the 9 R-row evidence packages; the two NEVER-CITED
   driver docs + the MOM decode candidates). Every load-bearing claim
   re-verified FIRST-HAND by the lead before filing (the M-018 regression
   grep census, the volume_label fill-site census, the CRW registry+validator
   read, the thumbnail budget guards, the session-list filter read, the CLI
   -h block, the batch-2 absence loop, the DeviceManager decompile greps).
   Evidence: diff-to-fix/evidence/s8-verification-records.md.

2. **25 items filed** (the session's diff-to-fix/ output):
   SEVERE: S-040 (image create absent, R-93), S-041 (the S-007 batch-2
   remainder, ~24 formats — previously recorded outside an active finding).
   MAJOR: M-071 (the M-018 regression — the N-064 rrecov.cpp reconstruction
   dropped the junk gates; git-proven), M-072..M-078 (the R-row remainder:
   query axes, scan-time folder filter, dest codes, chkdsk, TRIM facet,
   preview caps, BitLocker turn-off), M-079 (auto-save label never filled),
   M-080 (thumbnail budget starves ordinary photos), M-081 (CRW
   misattribution + the PRD gate rejects genuine CRW), M-082 (DeviceManager
   offline-OS identification — the CheckVersion verdict re-scoped).
   MINOR: N-090..N-098. INFORMATIONAL: I-027 (the data-asset ground-truth
   register). TRACKED-OPEN: OQ-S8-1 (CWindowsFS consumer).

3. **Landing verification results** (the T1 scope items): W1a/W1b/W1c TRUE
   at the load-bearing points; the NTFS origin-LCN carrier TRUE on all
   parameters; batch-1 27/27 magics byte-exact (bounds → N-093); the
   installer steps 2-3 verified by the lane's own green dispatch record (run
   40684304129); the CI rebuild docs-only. FIXED spot-check: 9/10 CONFIRMED,
   M-018 PARTIAL → M-071.

4. **Boards updated:** SUMMARY (the session-8 delta + the new counts + the
   recommended fix order), CROSS-REF-MATRIX (the session-8 detail rows),
   API-DELTA §5.2 (row→finding map), SCAN-ENGINE-DELTA A4 (the batch-1
   landed/batch-2-owned correction), SURFACE-INVENTORY (the corpus-currency
   + the driver-doc discharge + the decode-candidate dispositions), README
   (the session-8 audited-commit row), 13 module files (session-8 sections).

Board at close: **25 active** (2 SEVERE · 12 MAJOR · 9 MINOR · I-027; I-026
informational) · 249 FIXED · tracked-open 2 (OQ-S4-3 unchanged; OQ-S8-1
new). Recommended first fixes: M-071 (2 lines, a regression), M-081 (small,
self-contained), then S-041's batches, M-079 (one function), M-080 (decide
before the ABI handoff), S-040 (the largest feature).

Driver-lane handoffs (recorded not fixed — the rda-driver repo is out of
this audit's two-repo scope; for that lane): (a) the READ-length semantics
nuance (og Length = sectors vs rda Length = bytes; the Information×512
invariant holds — an og-style client through rda.sys would under-read; no
in-product effect, core is rda's only client); (b) the disk.sys-comparison
field reading (trio artifact's DriverName vs DRIVER-CONTRACT's ServiceKeyName
— the decompile's `*(DO->DriverObject+0x30)+0x18` chase matches neither
cleanly; flagged to S7); plus the standing D3 (the 15/18-vs-19 session-code
prose, seven driver-repo sites) and D4 (regdrv-rda.reg hex(7) UpperFilters
still invalid).

No source code modified. Every changed path is under diff-to-fix/ + worklog*.
Task ID: SCAN-4/A2 (session open — the scan-engine context-recovery re-run; the Delta
Report refreshed; the two batch-1 misattributions fixed; the batch-2 decode filed)
Agent: scan-engine lane (A2, this session)

- Baselines (cloned fresh this session): core 0058271 (main), corpus be0cfe9 (main),
  rda-driver 323e75a (v0.2.4). Four read legs per protocol (corpus spec; corpus
  decompile — a narrowed retry after the first attempt hit a container deadline, both
  read-only; core state; driver). Driver leg: v0.2.4 wire byte-stable, NAS/Cloud zero
  code paths re-proven by full-repo grep.
- Q1-Q11, A1-A3/A5-A6, M1-M5, F1-F7 re-verified: zero open defects outside the A4
  batch-2 remainder. C9/OQ-S4-3 stays Windows-execution-bound (unchanged).
- TWO live batch-1 misattribution defects found by the legs and verified first-hand
  at the registration sites, then FIXED (registry rows relabeled + validators
  rewritten to the og grammar; FormatId values untouched — append-only discipline):
  (a) II/MM 1A 00 = CRW (Brance::CRWAnalyser id 0x12, HEAP + {CCDR,TIFP,ARCH} +
  ver-2 grammar, RAWSearchDLL :1746-1761 + FUN_180052000 :62333-62353) — the landed
  "Minolta MRW" rows' invented PRD gate rejected every real CRW file; MRW is \0MRM
  only (id 0x66) with the og size formula (FUN_180012100 :14233-14258); (b)
  DB A5 2D 00 / 94 A6 2E 00 = FileLowDocAnalyser id 0x17 (old-Word .doc;
  formula-only validator FUN_18001b650 :21167-21188) — the landed "Windows Help"
  rows relabeled to ext "doc"; HLP = 3F 5F 03 00 only with the og size gate
  (FUN_180041c50 :49883-49895).
- Batch-2 remainder: DECODED at validator level (all 17 items — the session-5 grammar
  table in SCAN-ENGINE-DELTA is the record); the S-007 causes re-pinned (CHM's
  :22635 pin invalid → real ITSF grammar :17705-17740; MPEG-TS's 0x11C cause stale
  = FH11; FCPEvt has NO XML side — covered by FMT_SQLITE + N-089; SWF ZWS rejected /
  ver∈[3,20]; JP2 ids are brand mappings). Per-school fix batches in flight.
- Errata E53/E54 filed (census's "!BDN = Bentley" → FilePSTAnalyser id 0x38;
  dual-registry's "0x4449 ID = AIFF" → the MP3/ID3 chain, AIFF = FORM id 0x50) —
  both verified first-hand at the registration sites.
- handoff (API lane): the carve registry's mislabeled rows corrected in place —
  FormatId VALUES unchanged (append-only held); the ext/name surface changed on 4
  rows (mrw→crw x2, hlp→doc x2, both corrections of WRONG labels to og truth);
  list-carve-formats reads the registry automatically, no API change needed.


---
Task ID: SA-8/W2 (session-8 wave 2 — the continuation lane, executed;
RECONCILED at push time over the parallel deep-audit lane's same-session
commits)
Agent: session-8 full parity audit (continuation lane, this session)

Method: five parallel read agents (keyed batch-2 decode, list-layer/
text-school decode, landings verification, core capability inventory,
corpus never-cited sweep, thumbnail parity) + first-hand re-reads by the
lead of every acute candidate (WMF FUN_1800264d0 :29807-45; thumbnail
FUN_1402fcc30 :521694-725 / FUN_1403d7840 :652383-412 / the grade
promotion :422498-518; the core validators/autosave/thumbnail code; the
FMT_ absence sweep; both WPG-ctor regions to settle the :15700-vs-:20542
cite conflict — :15700-01 wins, erratum E63).

Reconciliation record (the push-time rebase over 370d659..ffa7a7d): the
parallel deep-audit lane filed the plan-of-record R-rows + the batch-2
owner (S-041) + the CRW misattribution (M-081) + the thumbnail budget
(M-080) + the data-asset register (I-027) with IDs S-040/S-041, M-071..
M-082, N-090..N-098, I-027. This lane's IDs were renumbered past that
set (S-042..S-045, M-083..M-085, N-099..N-106, I-028, errata E55-E64)
and the two content-duplicates were MERGED, not duplicated: the batch-2
full decode -> the S-041 addendum (7 text-school grammars, the
config-gated zone, registry geometry, the BOM battery, FCPEvt
end-to-end); the thumbnail budget og-side evidence (the 200 MB
eligibility gate :422428, which CORRECTS M-080's "no input cap" claim,
plus the wrong-anchor test pin) -> the M-080 addendum with the severity
discussion (SEVERE recommended, first-filed MAJOR stands per the
extend-never-overwrite rule).

Findings filed (16): S-042 WMF placeable grammar (disjoint accept-sets;
batch-1 claim refuted) · S-043 thumbnail EXIF orientation · S-044
auto-save gates (results-found, not-already-saved; byte-flag config) ·
S-045 thumbnail format coverage (video/RAW/HEIC/PDF) · M-083 grade-2
promotion · M-084 ACCDB second signature · M-085 TIFF LZW · N-099
batch-1 citation truth pass (incl. the N-093 WPG-cite correction) ·
N-100 +0x60 persistence + OQ-SA-1 chain corrections · N-101 inventory
arithmetic + classification rows · N-102 M-024 count propagation in
the finding file itself · N-103 thumbnail API rows + heif attribution ·
N-104 ldm.md/CAPABILITY-MATRIX truth defects · N-105 Folder-suffix
naming · N-106 GetDriverInfo disposition · I-028 thumbnail superset
register. Errata E55-E64. Evidence: decompiled-quotes/
s8-{wmf-placeable-grammar,thumbnail-pipeline,autosave-and-stamps,
batch2-formats}.md.

Verification lane results: W1a/W1b CONFIRMED (scanTime, restore
progress formula + per-child cancel, record-1+2 list read); W1c PARTIAL
(naming CONFIRMED, trigger placement CONFIRMED, gates MISSING = S-044);
OQ-SA-1 carrier semantics CONFIRMED, chain + resident caveats corrected
(N-100); batch-1 constants verified correct except WMF (S-042) + ACCDB
(M-084); registry regions corrected (N-099/E55). Thumbnail engine:
first-ever parameter verdicts (this lane: 4 defects + 1 register + 5
MATCH + 6 UNVERIFIED-tracked; the parallel lane's M-080/N-092 complete
the set). Progressive carve emission: MATCH. No unregistered core
superset; no undispositioned src/core module; driver D3/D4 out of this
session's two-repo scope (remain the rda-driver lane's items).

Board after both session-8 legs: 41 active (S-040..S-045, M-071..M-085,
N-090..N-106, I-026/I-027/I-028) + 249 FIXED + OQ-S4-3/OQ-S8-1
tracked-open. The S-007 batch-2 remainder is no longer "outside an
active finding" — S-041 owns it with the decode complete.

---
Task ID: CI-REL-2 (the v0.5.2 cut - the first combined-era release + the dev-sign cert repair)
Agent: senior-dev session (this session, third leg)

Work Log:
- dd225da went green (run 40702251898 - lint + MSVC build + the full
  1021-test ctest, the first fully green Checks since c95335f); tag
  v0.5.2 cut at 99eddb0 and pushed; release run 40704304307: validate
  GREEN, build-core GREEN, driver-tier RED at the sign step.
- The dev-sign failure was the CERT, not the workflow: signtool signed
  fine but Get-AuthenticodeSignature returned UnknownError - "a
  certificate's basic constraint extension has not been observed" -
  the stored cert lacked Basic Constraints, and a self-signed cert is
  its own issuer, so the chain engine demands the extension.
  Regenerated the cert to the documented shape (CN=RRECOV-DEV-SIGN,
  RSA-2048 SHA-256, 3-year, BC critical CA:FALSE, KU critical
  digitalSignature, EKU codeSigning; AES-256 PFX) and updated BOTH
  repo secrets via the sealed-box API. Full rerun (attempt 2): every
  job green, v0.5.2 PUBLISHED with the 8-asset set incl. rda-test-
  sign.cer - exactly the asset the benchmark fetch demanded.
- The auto-dispatched benchmark (40710818026) died at the harness
  compile: Tee-Object into evidence/05-feature-probes which only
  Run-Benchmark.ps1 creates LATER (step 6 vs step 5), fatal under
  ErrorActionPreference=Stop. Fixed the step to self-create its
  subdir (9cb098c, rebased over the parallel session-8 audit lane's
  3 commits - no conflict), re-dispatched against v0.5.2
  (40712214950, reason release-v0.5.2-retry-after-evidence-dir-fix).
- Security note: the new PFX password is NOT in the repo anywhere -
  it lives in the repo secrets and one local file outside git
  (scripts/devsign.pw pattern); the public cert ships as the release
  asset for consumers.

Stage Summary:
- Watching 40712214950 (smoke matrix, up to 120 min). On green: the
  owner's full pipeline is restored end-to-end - commit -> checks ->
  release (dev-signed) -> test+benchmark. Remaining known-open: the
  parallel lanes' session-8 findings (their own worklog entries).
Task ID: SA-8/T2 (session-8 third lane — the read waves)
Agent: session-8 full parity audit, lead (this session)

Five parallel read agents (A: corpus contracts for the new landings —
SaveScan/rsf-format read fully end-to-end, UIUX/10+30, the batch-2 evidence;
B: corpus coverage/citation sweep — all 274 artifacts + 9 architecture docs
vs the trail; C: new-landing parity verification — W1a/W1b/W1c,
ntfs_origin_lcn, S-007 batch-1, thumbnail, installer; D: board-truth sweep;
E: driver + RE tooling audit), then first-hand re-verification by the lead of
every load-bearing claim before filing: both WMF decompile sites re-read
personally (ghidra :29820-29845 + retdec :38495-38515 — the term-by-term XOR
word-list mapping resolved), the core WMF validator re-read (cr_analysers.cpp),
CI-CURRENT/FloorVersion/README/PREBUILT sites re-read, regdrv-rda.reg decoded
byte-level, rda_version.h read.

Key verified results feeding the filings:
- WMF placeable grammar: og checksum word@+20 over words 0..9 + zero-dword@+16
  vs core checksum@+18 over words 0..8 + inch 1..14400 — DISJOINT accept sets.
  (Independently and convergently decoded by the continuation lane's S-042 —
  merged; two lanes, two decompilers, one conclusion.)
- CHM: the ":22635 u64-widened GUID conflict" is a retdec constant-print
  artifact (erratum E66); the real gates are the canonical ITSF v3 GUID pair
  as genuine u64 compares at +0x18/+0x28 + hdr-len 0x60 + chunk magic 0x1FE +
  zeros, size u64@+0x68, NO bounds. Key 284/0x11C = FreeHand 11 (erratum E65);
  MTS/M2TS are list-only. Convergent with SCAN-4's decode; donated to S-041.
- W1a/W1b/W1c parity: verified MATCH-with-registered-divergences (the
  load-permille window mapping, default-off auto-save, .crsf extension —
  the parallel lanes filed the registration/threshold items as N-091/S-044/
  N-105; this lane's residual doc nits folded into the merges).
- ntfs_origin_lcn + S-007 batch-1 keys: verified landed and corpus-conformant
  (both walk stamps, clone carry, bit-8; the batch-1 key set byte-exact vs
  the registration sites — EXCEPT the WMF validator grammar and the two
  misattributions the parallel lanes already fixed at 9e5dedc, verified gone
  in-source).
- Driver lane (this lane's unique territory): M-086 FileVersion pin, M-087
  regdrv hex(7), M-088 FloorVersion/CI-docs cluster, M-089 rdabench phantom-
  zero fuzz gap, N-107 the 14 stale count sites; wire contract re-verified
  MATCH (identity IOCTLs raw-exact, Information x512, LBA scaling, 19 session
  codes binary-probed, NAS/Cloud exclusion holds).
- Board truth: docs/PARAMETERS.md §3 rows STILL STALE (the SCAN-2f "fixed"
  claim false — N-108, claim corrected in SCAN-ENGINE-DELTA); A-007 dangling
  cites repaired; SURFACE-INVENTORY gained §9 (the DRWService §5-claim
  correction, EuShare/glob/MOM-count fixes, coverage verdict CLEAN).

Parallel-lane reconciliation: the deep-audit lane (S-040/S-041 + M-071..M-082
+ N-090..N-098 + I-027, 14:31) and the continuation lane (S-042..S-045 +
M-083..M-085 + N-099..N-106 + I-028, 14:52) filed first; this lane's
duplicate S-040/S-041 filings merged into their owner files as convergence
addenda (the no-duplication rule), and this lane's unique findings took
M-086..M-089 / N-107 / N-108. Board after all legs: 46 active + 249 FIXED +
OQ-S4-3/OQ-S8-1 tracked.

---
Task ID: SA-8/T3 (session-8 third lane — the CLOSE-OUT handoff)
Agent: session-8 full parity audit, lead (this session)

Audited commits (this lane): core product code `35adcb9` (trail commits
rebased through the parallel lanes to `7748254`; the parallel fix lanes moved
the product tree to `9e5dedc`+ mid-session — every load-bearing absence claim
re-verified on the final rebased tree); driver `323e75a` (v0.2.4); corpus
`be0cfe9`. Read-only discipline verified: this lane's two commits
(`8ac5211`, `5553216`) touch ONLY diff-to-fix/ + worklog* — zero product
files (the product-file changes between 35adcb9 and HEAD belong to the
parallel CI-REL/scan-engine fix lanes).

Coverage statistics (this lane's waves):
- Corpus: 274 artifacts + 9 architecture docs enumerated; citation state
  re-swept against the entire trail; coverage verdict CLEAN (zero in-scope
  un-dispositioned engine parameters; SURFACE-INVENTORY §9 records the
  corrections: DRWService §5-claim, EuShare parenthetical, §3 glob names,
  MOM 17->16, per-binary 3/114, SearchDLL 30/30, SaveScan 5/5).
- New landings verified: W1a/W1b/W1c (MATCH with registered divergences),
  ntfs_origin_lcn (both stamps + clone + bit-8 verified), S-007 batch-1 keys
  (byte-exact vs registration sites; WMF grammar = S-042; misattributions
  fixed by the parallel lane at 9e5dedc, verified gone), thumbnail (the
  registration/threshold items filed by the parallel lanes: I-028/N-091/
  M-080/S-043/S-045), installer S-038 (13/13 parameters contract-faithful).
- Driver (this lane's unique territory, fully dispositioned): wire contract
  MATCH (identity IOCTLs raw-exact vs eprdtdrv-internals:81-83, Information
  x512, LBA scaling, 19 session codes binary-probed in EUDCPEPM.sys, NAS/
  Cloud exclusion holds); M-086..M-089 + N-107 filed; D2/D3/D4 handoffs all
  owned; v0.2.4's M-064 claims verified implemented (with the FileVersion
  caveat = M-086).
- Board truth: N-108 (docs/PARAMETERS.md stale constants + the SCAN-2f false
  "fixed" claim, corrected in place); A-007 cite repair; errata E65/E66;
  CROSS-REF-MATRIX/README/SUMMARY current.

Board at this lane's close (all session-8 legs combined): **47 active**
(6 SEVERE: S-040 S-041 S-042 S-043 S-044 S-045 · 19 MAJOR: M-071..M-089 ·
19 MINOR: N-090..N-108 · I-026/I-027/I-028) + 249 FIXED + tracked-open
OQ-S4-3 (Windows-execution-bound) + OQ-S8-1. No UNVERIFIED items remain
untracked: every open decode question lives inside a finding's fix plan
(the zero-invention bar held), and the two tracked-open questions carry
precise probes.

Recommended first 20 fixes (cross-lane priority order, this lane's vote):
1. S-042 WMF placeable validator (wrong-results class; the grammar is fully
   decoded; one function)
2. S-041 batch-2a CHM + MTS/M2TS (grammars decoded; CHM's blocker refuted)
3. M-071 the M-018 regression (junk-filter gates lost — recover-all writes
   the og's hidden junk; user-data pollution class)
4. S-044 auto-save gates (results-found + not-already-saved; the og never
   auto-saves an empty/already-saved scan)
5. M-080 thumbnail budget + the 200 MB eligibility gate (starves ordinary
   photos)
6. M-086 driver FileVersion stamping (defeats upgrades; one release-lane
   decision + the drill)
7. M-087 regdrv-rda.reg one-line fix + the hex(7) CI gate
8. M-084 ACCDB second signature + M-085 TIFF LZW (small, decoded, high-value)
9. S-043 thumbnail EXIF orientation (wrong-aspect output class)
10. M-088 FloorVersion single-sourcing + the CI-shape doc sweep (the
    "ONE pin source" lesson)
11. M-089 rdabench probe-list derivation + the phantom-zero sweep
12. N-108 docs/PARAMETERS.md two rows (5-minute fix; integrator-facing)
13. M-082 DeviceManager offline-OS identification (decode work scoped)
14. M-072..M-078 the R-row batch (query axes, folder filter, dest codes,
    chkdsk, TRIM, preview caps, BitLocker turn-off — the API-EXP plan of
    record)
15. N-107 driver 15/18-command prose sweep (one commit)
16. S-045 thumbnail format coverage phase 2 (video/RAW/HEIC/PDF)
17. M-079 CLI autosave volume-label fill site
18. M-083 thumbnail grade promotion
19. N-090..N-106 the MINOR batch (doc-truth + propagation rows)
20. OQ-S4-3 the TTFR measurement (rides the next green Windows dispatch —
    execution-bound, stays tracked)

Handoff notes for the next session:
- The parallel-lane environment is ACTIVE (three session-8 audit lanes + two
  fix lanes landed mid-audit). Pull before every wave; re-verify absence
  claims on the final tree (the N-090 cite-drift lesson).
- The corpus remains at be0cfe9 (one errata line past 03ad46c — absorbed).
  Watch the corpus repo for new CORRECTIONS rows; each is a scope delta.
- Driver findings file HERE (core trail, the M-064 precedent) with fix plans
  pointing at rda-driver; single-repo commit discipline holds.
- The remaining "executed-comparison" verification class (Windows-only paths:
  the driver install drills, reg import, WMF fixture carve) is out of this
  container's reach — the findings carry the static evidence and the drills
  are the fix lanes' acceptance criteria.
Task ID: SCAN-4/A2 (landing record — the Delta Report, M-081 + its twin, S-041 school 1)
Agent: scan-engine lane (A2, this session)

- Landed (rebased onto the session-8 audit's tree): `ffa7a7d` — the Delta
  Report refresh (SCAN-ENGINE-DELTA session-5 resolution log: the full
  batch-2 validator-grammar decode, all 17 items first-hand; the two
  misattribution defects filed; A4 phrasing corrections for CHM/JP2/EMF/
  FCPEvt/OfficeTagFromXML), the S-007 batch-2 causes re-pinned (CHM's
  :22635 = std::_Tree rebalance code, the real ITSF gate at :17705-17740;
  MPEG-TS's 0x11C = the landed FH11; FCPEvt has NO XML side - covered by
  FMT_SQLITE + N-089), errata E53 (census's !BDN "Bentley" = wrong; it is
  FilePSTAnalyser id 0x38, verified :2324-2330) + E54 (dual-registry's
  0x4449 "ID" = AIFF wrong; it chains MP3/ID3 id 0x2b, AIFF = FORM id 0x50),
  and this session's worklog baseline entry.
- Landed `9e5dedc`: M-081 RESOLVED (MAJOR -> FIXED, the finding file's
  resolution record lists every fix-plan step) + the LowDoc/HLP twin —
  FMT_CRW/FMT_CRW_MM (the og HEAP grammar), FMT_MRW (the og size formula),
  FMT_LOWDOC/FMT_LOWDOC2 (FileLowDoc id 0x17, ext "doc"), FMT_HLP (the og
  size gate); the retired II1A+PRD synthetic asserted NOT carved;
  S007M081CiwMrmLowdocHlpOgGrammar pins every gate + exact formula sizes +
  the corrected row labels. Suite 1017->1021 green.
- Landed `b92ebef`: S-041 batch-2 school 1 — CHM ONE MNY EMF SWF(FWS+CWS)
  3DS(MM + FMT_3DS_NCSD, the list-layer twin; the NCSD constant 0x4453434e
  extracted from the og DLL .data VA 0x180093978) — 8 rows, enum values
  appended (append-only held), S007NicheBatch2School1Carve/RejectsNoise
  (1023/1023 green). ONE/3DS carry the bounded-extent stand-in; their
  size-walks are the recorded follow-ups.
- Convergence with the session-8 audit: M-081 = this lane's defect (a);
  S-041's decode precondition = the session-5 table; N-093 (batch-1 bounds
  deviations) noted - the relabeled rows keep their landed bounds.
- F-lane doc truth: cr_rawsigns.h's "(30 formats)" stale count -> the
  per-FormatId phrasing; cr_signatures.h M-024 comment's "[Filter] (37 named
  toggles" -> the shipped file's 34 (CORRECTIONS #5).
- Open for the next A2 leg: S-041 school 2/3 (HTML SVG LZH MTS/M2TS MOBI JP2
  + the text school; the session-5 grammar table is the work list) +
  OfficeTagFromXML's archive-lane classification surface + the ONE/3DS
  size-walks + C9/OQ-S4-3 (Windows-execution-bound, unchanged).

---
Task ID: SA-8/T10b (session-8 lead-lane close-out — the FINAL three-window reconciliation)
Agent: session-8 full parity audit (the lead lane; three concurrent lanes reconciled)

The push was rejected twice by concurrent lanes landing first — the reconciliation
ran three times, each past the new max IDs:
- Window 1 (the deep-audit lane): 13 duplicates merged (WMF -> their S-042 by
  addendum; CRW/MRW -> their M-081, already FIXED by 9e5dedc; the API-queue items
  -> their S-040/M-072..M-078; the registration gaps -> their N-091; +0x60 ->
  their N-100); the lead's 35 kept findings renumbered S-046../M-086../N-107...
- Window 2 (the fix lane + the third driver lane): school 1 of S-041 LANDED
  (b92ebef: CHM ONE MNY EMF SWF 3DS — the lead's 7 duplicate per-format filings
  dropped, its decodes convergent with the remote errata E56-E66); FCPEvt
  closed-as-covered; the third lane's M-086..M-089/N-107/N-108 taken; the final
  renumber: 28 kept (S-047, M-090..M-102, N-111..N-126, I-029/I-030).
- Errata: the lead's census corrections were ALL duplicates (remote E53-E66 +
  the older rows 19/23) except ONE — E67 (the DMG bound/walk, filed as M-090).

The kept set's headline additions the parallel waves did NOT have:
- M-090: the DMG koly-trailer walk + DMG-specific 0xC800000 cap (:49436-49440/
  :63806) vs core's bzip2-EOS+10/global cap — carved DMGs truncated before the
  mandatory UDIF footer (erratum E67).
- M-091: the MDB page-chain size-walk decode (:52001-52039) — the landed row's
  "undecoded" comment falsifiable; complements M-084.
- N-116: the session-list row-ORDERING divergence (the og's FILETIME time-sort
  :432112-432142) — distinct from N-090's scope axis.
- The school-2/3 per-format fix units (S-047 MTS/M2TS, M-092 HTML, M-093 XML/SVG,
  M-096 MOBI, M-098 HIK, M-099 AEP, N-111 JP2, N-112 LZH, N-113 text school,
  N-115 OfficeTagFromXML) — children of S-041 per its addendum #2.
- M-100/M-101/M-102 (browse-inside, Versions/Tags branches, the thumbnail ABI),
  N-119..N-126 (D-88's stale premise, the pointer drifts, ESP automount, NVRAM
  create/delete, cli/transport staleness, the disconnect states, Unsaved File),
  I-029/I-030 (driver-code provenance, module-map holes).
- The fixture-regeneration requirement on S-042: landed tests must derive
  fixtures from spec, never from the implementation — the class behind S-042
  and the M-081 fix.

Board after the final merge: 75 active (7 SEVERE · 29 MAJOR · 34 MINOR · 5 INFO
incl. I-026) / 249 FIXED / 1 tracked-open (OQ-S4-3). No source code modified —
diff-to-fix/ + worklog only. Every kept finding cites audited commit ff879f9;
the three-window merge map is recorded in the addenda, the boards, and
diff-to-fix/evidence/sa8-context-sheets.md.
Task ID: SA-9/T1 (session-9 open — baseline record)
Agent: session-9 full parity audit (this session)

Baseline (cloned fresh this session):
- core_recovery @ 005827112ac7f531c38def082ef67f8ff194e92e (main) — merged
  product repo (confirmed by inspection, the standing determination: core
  engine src/core + include/cr, win layer src/win, driver-consuming transport,
  CI, audit trail in one tree; the driver ships as prebuilt rda.sys release
  asset from heicfast/rda-driver per docs/PREBUILT-ARTIFACTS.md). Product-code
  baseline for this session's audit: 35adcb93cf433af4339ee7bdb4c7f5e3e13e8fcf
  (the last code commit; 0058271 and ff879f9 above it are audit-trail-only
  commits — verified: git log 35adcb9..HEAD over non-audit paths is empty).
- rda-driver @ 323e75acf4cd745960813af4e7fd586a4a34d1a0 (main, v0.2.4).
- corpus easeus-drw-research @ be0cfe95ecf22ac67e0f3a823dd003163b196d38 (main)
  — unchanged from session-8's T1 (CORRECTIONS.md #9 one past the trail's
  long-standing 03ad46c; absorption already verified by SA-8/T1).

Session scope (from the trail state + the prompt's two-repo grant, not from
any invented list): THIS session has BOTH repos (core + rda-driver), so the
driver-repo handoffs recorded by SCAN-2f come INTO scope alongside the
session-8 wave-2 lanes that opened but did not execute:
- V-lane: read-only verification of the post-dev-sign landings' corpus claims
  (API-EXP W1a/W1b/W1c; OQ-SA-1 carrier eac8e1d; S-007 batch-1 e018bf4;
  thumbnail 05e6786; installer S-038 steps 2-3 3e6a8fa; CI rebuild 566c483).
- R-lane: the S-007 batch-2 remainder becomes PROPER FINDINGS (CHM/MNY/EMF/
  ONE/MPEG-TS/3DS/SWF/JP2/MOBI/LZH + the text school + FCPEvt/
  OfficeTagFromXML/CRW) — decode what is decodable at the og registration
  sites first.
- G-lane: the gap hunt — corpus artifacts + og-codebase surfaces the prior
  waves never dispositioned; core capabilities without corpus basis (superset
  register); thumbnail engine vs corpus preview evidence; the NEVER-AUDITED
  EPMNtDrv v2 internals (identity IOCTLs + async completion engine) + trio
  naming/PID handshake flagged to the S7 lane that never landed.
- D-lane (new, this session's two-repo grant): rda-driver repo — D3 (the
  stale "15/18-command" prose vs the 19-code set, seven sites), D4 (regdrv-
  rda.reg hex(7) UpperFilters invalid), D2-partial doc drifts, the
  FileVersion-1.0.0.0-vs-tag-v0.2.4 pin.
- C-lane: gate-doc currency (SURFACE-INVENTORY/PARAMETERS/README corpus pin
  03ad46c -> be0cfe9; board counts; CROSS-REF-MATRIX rows for new findings).

Board at open: 0 active / 249 FIXED / 1 INFORMATIONAL (I-026) / 1
tracked-open (OQ-S4-3 — TTFR half execution-bound). ID space continues:
next A-012+, S-040+, M-071+, N-090+, I-027+. URLs recorded here and in
diff-to-fix/README.md (the standing record — re-verified this session).

Prompt saved redacted: worklog/session-9-parity-audit-prompt.md. Read twice.

---
Task ID: SA-9/T2 (the parallel reading wave — five agents, both repos + corpus)
Agent: session-9 full parity audit (this session)

Context sheets produced in the scratch area (/home/z/my-project/audit-ctx/,
NOT committed per the SOP): A-corpus-inventory.md (376 lines — the artifact
walk: 274 artifacts / 244 unique basenames, 92 cited by literal basename /
110 in any form, 134 never-cited; og-codebase 17-dir tree map; top-15
gap-hunt candidates), B-batch2-decode.md (the batch-2 format decode: every
format's key/id/class/bounds/gate with decompile cites), C-driver-corpus.md
(976 lines — the EPMNtDrv v2 internals + trio naming/PID handshake + hook
drivers + 7-driver map, every claim cross-verified against the ghidra
decompiles), D-core-vlane.md (the V-lane verification of the post-dev-sign
landings), E-driver-read.md (the rda-driver repo read — all 66 files, the
wire contract, D3/D4/FileVersion verification).

Headline results (each verified FIRST-HAND by the lead before any finding
was written — the verification log is the findings' evidence files):
- V-lane: W1a/W1b/W1c/origin-LCN/thumbnail/installer landings verify MATCH
  (documented divergences stay documented); TWO new defects found in the
  S-007 batch-1 carve landing: the WMF placeable validator reads a WRONG
  header layout (inch@+16 = the reserved field; checksum@+18 vs the og's
  +20; XOR-9-words vs the og's 10) and REJECTS every real placeable WMF
  (the test plant mirrors our own wrong convention — the suite cannot catch
  it); MID/GZ registry caps drift from the og ctors (64MiB vs 10MB; 512MiB/
  min-18 vs 400MB/min-20).
- R-lane: the batch-2 formats fully decoded at the registration sites —
  the CHM GUID-compare-width puzzle RESOLVED at the instruction level
  (capstone on the og binary: u64 compares, Ghidra right, RetDec's stack-
  built constants mis-fused; the canonical ITSF layout puts GUID1@+0x18/
  GUID2@+0x28 per the Russotto spec — the og's gate is canonical, the prior
  session's chmlib-struct-based expectation was the error); CRW/MRW
  resolved instruction-decisive: II/MM 1A 00 -> Brance::CRWAnalyser id 0x12
  (CIFF gate), FileMRWAnalyser id 0x66 rides \0MRM ONLY — our batch-1 MRW
  rows on II/MM 1A 00 are dead rows and Canon CRW is not carved at all.
- G-lane: the gap-hunt candidates dispositioned against the existing module
  coverage (CDiskDriveLayoutReadWriter = the lost-partition apply sequence,
  covered modules/lost-partition.md:205-218; RegLib covered mom-capabilities
  :94-97 + health.md:63; BCD writer covered boot-repair.md; NTFSLib covered
  ntfs.md:357; VideoViewer covered I-015; CWindowsFS/offline-OS stay census
  stubs — the corpus documents the classes but no behavior parameters).
- D-lane (this session's two-repo grant): D3 CONFIRMED and expanded (16
  prose + 2 structural stale-count sites; nuance: sites describing the og's
  EUDCPEPM reference set as 15 are CORRECT — the true served set is 19);
  D4 CONFIRMED (hex(7) tokens are not hex bytes; plus TWO more divergences:
  the class GUID targets DISKDRIVE where the og's regdrv.reg writes VOLUME,
  and Start=3 demand-start vs the og's boot-start filter); FileVersion
  CONFIRMED pinned 1.0.0.0 vs tag v0.2.4 (the M-064 versioned-coexistence
  scheme is inert: every v0.x build computes <name>10.sys so the upgrade
  branch never triggers); the admin-gate doc claim at rda_ioctl.h:367 has
  NO code check behind it.
- The never-audited EPMNtDrv v2 + trio surfaces resolve MATCH (our driver
  implements the PID-token handshake byte-exactly, service-name-derived
  naming, DO_DIRECT_IO, METHOD_BUFFERED identity IOCTLs, pass-through,
  x512 Information — cross-verified C-sheet corpus decode vs E-sheet code
  read); the SURFACE-INVENTORY rows flip to AUDITED this session.

Agent-B corrections absorbed by the lead (the sheet's own errors, caught by
first-hand re-reads, NOT carried into findings): the "MMOR -> CRW chain"
association is wrong (MMOR/IIRS/FUJI -> BruteForce::TiffAnalyzer id 0x40,
ghidra :1727-1744 + FUN_180037b50); the B-sheet's CHM GUID tail constant
reading was garbled (the true tail u64 = 0xECE622C9A0000C9E, verified at
the instruction level).

Findings written this session: S-040 (WMF placeable layout), S-041 (CRW
absent + MRW dead rows), S-042 (batch-2 formats absent), M-071 (FileVersion
pin), M-072 (regdrv-rda.reg), N-090 (D3 counts), N-091 (driver doc-vs-code
truthfulness), N-092 (MID/GZ caps). Evidence under evidence/decompiled-
quotes/s9-* and evidence/executions/s9-*. Gate docs updated: README (the
session-9 block), SURFACE-INVENTORY (corpus pin be0cfe9 + row flips + the
EuShare/glob corrections), PARAMETERS (pin), CROSS-REF-MATRIX (new rows),
SUMMARY (delta block), CORPUS-ERRATA (E53-E55), modules/carve.md,
modules/driver-install.md, modules/transport.md.

---
Task ID: SA-9/T19+T20 (session-9 close — the push-time reconciliation + the final verification sweep)
Agent: session-9 full parity audit (this session, closing)

THE PUSH-TIME RECONCILIATION (the session's defining event): the first push
was rejected — the parallel session-8 lanes (the continuation lane 24e93ad,
the deep-audit lane 370d659, the third lane 5553216/7922b5e, the scan-4 lane
ffa7a7d/a972d29, the lead-lane final reconciliation 2025554) had landed a
75-active-findings board, the M-081 CRW FIX AS CODE (9e5dedc), the school-1
batch-2 formats AS CODE (b92ebef: CHM/ONE/MNY/EMF/SWF/3DS), and errata
E53-E67 — concurrently with this lane's independent decodes. Per the SOP
(never duplicate; extend; the matrix must not carry two rows for one
divergence), the merge was reconciled:
- 7 of this lane's 8 filings were CONVERGENT → merged as addenda into the
  parallel board's owner files (S-040→S-042 WMF incl. the std-gates
  observation; S-041→M-081-FIXED; S-042→S-041 parent + children; M-071→
  M-086; M-072→M-87 + THE NEW START-POSTURE PARAMETER; N-090→N-107;
  N-092→N-093).
- 1 filing was UNIQUE → kept, renumbered N-127 (the rda-driver doc-vs-code
  truthfulness batch — the admin-gate claim with no code behind it).
- This lane's errata E53/E54 were convergent with the remote E57/E66
  (dropped, convergence noted); E55 (part1:85, the SCAN-2f queue's row)
  was unique → filed as E68.
- The ID collisions (both sides had used S-040..S-042/M-071/M-072/
  N-090..N-092 for different findings) resolved in favor of the remote
  board's numbering — the dropped local files' content survives fully in
  the addenda + the six s9-* evidence files.

This lane's UNIQUE contributions that survive on the merged board:
1. N-127 (the driver doc-truth batch).
2. The M-087 Start-posture parameter (demand-start + active UpperFilters
   cannot attach at stack-build; the og's filter is BOOT_START).
3. The EPMNtDrv v2 + trio naming/PID-handshake MATCH verdicts (the
   never-audited surfaces — the SURFACE-INVENTORY rows flipped; no prior
   lane had dispositioned them).
4. The V-lane landing verifications (W1a/W1b/W1c, origin-LCN, thumbnail,
   installer steps 2-3, CI) + the G-lane gap-hunt dispositions (every
   candidate traced to existing coverage — zero false findings filed).
5. Erratum E68 + the instruction-level CHM capstone evidence (s9-chm-gate-
   asm.md) corroborating E66.
6. The SURFACE-INVENTORY bookkeeping corrections (the EuShare miscount,
   the dead citation globs, the honest citation counts) + the corpus pin
   (be0cfe9).
7. The pre-landing registry inventory (s9-carve-registry-inventory.md —
   the 35adcb9 baseline the school-2/3 children can diff against).

Coverage statistics (this lane, at its 35adcb9 baseline):
- Carve surface: the batch-2 set fully decoded (per-format keys/ids/
  classes/bounds/gates); batch-1 verification (WMF functional defect,
  MID/GZ/PSP/SIT cap drift); the registry inventoried (101 rows).
- Transport surface: the driver repo read end-to-end (66 files); the
  EPMNtDrv v2 + trio surfaces dispositioned MATCH; the wire contract
  verified (19 session codes, identity + extended IOCTLs, the buffer
  gates); D3/D4/FileVersion resolved into findings.
- The V-lane: 8 landing groups verified, all corpus claims checked.
- The G-lane: 15 gap candidates dispositioned against module coverage.
- Evidence: 5 decompiled-quote files + 1 execution file (all first-hand
  verified by the lead; the CHM gate disassembled from the og binary).

THE READ-ONLY GUARANTEE (verified twice): this session's own commits
(6e9b6dd, 05b9892, ea59128, 167dbe3's resolution) touched NOTHING outside
diff-to-fix/ + worklog* — git diff 0058271..ea59128 over non-audit paths
is empty. The product-file changes visible in the final tree (cr_carve/
cr_analysers/cr_registry/test_carve/cr_ui/rrecov/test-benchmark.yml/
RELEASE-NOTES) entered exclusively through the MERGE with origin/main
(the parallel lanes' landed code: b92ebef, 9e5dedc, dd225da, 095a329,
9cb098c) — pulled per the SOP's moving-target rule, and this lane's
affected findings were re-verified against those landings during the
reconciliation (M-081's fix matches this lane's decode; the school-1
gates match this lane's instruction-level CHM read).

Recommended first fixes (this lane's contributions to the standing order,
for the next fixing session — the full 76-active board is SUMMARY's):
1. S-042 (WMF) — small, format-correctness, user-visible recall; the
   fixture-regeneration requirement is part of the fix.
2. M-086 + M-088 + N-127-item-1 — ONE driver-contract session: un-pin
   the version resource, the CI tag==resource gate, the FloorVersion
   split, and the admin-gate posture decision (og-parity vs hardening).
3. M-087 (+ the SA-9 Start-posture parameter) — the reg file rewrite +
   the parser gate + the (a)/(b) posture decision with the boot-media lane.
4. N-107 — the 19-count doc pass (mechanical).
5. The S-041 school-2/3 children in their fix-plan order (MPEG-TS first).
6. N-127's remaining mechanical items + N-093's cap one-liners.

Handoff: the board stands at the session-8 close + N-127 (76 active /
249 FIXED / 5 INFO / 1 tracked-open OQ-S4-3 — execution-bound, unchanged).
Next IDs: S-048+, M-103+, N-128+, I-031+, A-012+. The SA-9 context
sheets remain in the session's scratch area (/home/z/my-project/audit-ctx/
— A/B/C/D/E, not committed per the SOP); their load-bearing content is
preserved in the s9-* evidence files and the finding addenda.

---
Task ID: SA-10/T1 (session-10 open — the baseline record)
Agent: session-10 continuation parity audit (the "remove the fixed / go
deeper / mark all remaining" mandate)

Work Log:
- Context recovery per protocol: pulled origin/main (6 commits, all the
  parallel session-9 lanes' trail: 6e9b6dd/05b9892/ea59128/167dbe3/
  df0d5c0/84a1832 — the reconciliation with the parallel session-8 board
  already absorbed there; product-code tip b92ebef, corpus be0cfe9
  unchanged).
- The board read in full: SUMMARY (5036 lines), worklog (1357 lines),
  SURFACE-INVENTORY, API-DELTA (fully dispositioned: every S3-W row is a
  finding or DONE), the 75 active findings enumerated, the 2 tracked-open.
- Baseline recorded: core HEAD 84a1832 / product-code b92ebef / corpus
  be0cfe9. Prompt saved redacted (worklog/session-10-parity-audit-prompt.md),
  read twice.
- Session scope (from the trail's own residual list, not invented):
  (a) TRIAGE — re-verify all 75 active findings against HEAD; flip FIXED
  with Resolution records (the "remove the fixed things" ask);
  (b) DEEP — the DRWService install lifecycle (the one named NEVER-AUDITED
  dedicated finding), the b92ebef "recorded follow-ups" (ONE/3DS bounded
  extents), every "recorded outside an active finding" note still standing,
  the never-cited residue re-sweep;
  (c) MARK REMAINING — anything corpus-documented without a finding gets
  one; module-matrix/gate-doc currency after the landings.

Stage Summary:
- Baseline set; next: the parallel triage/deep/currency lanes, then
  lead-first-hand verification before any filing.

---
Task ID: DFX-38 (the closing fix session — board 84a1832)
Agent: lead (single-lane; two-repo grant: core_recovery + corpus only)

Work Log:
- T1: pulled both repos. Core HEAD `84a1832` (docs-only since product
  base `35adcb9` — the audit's cited tree), corpus `be0cfe9` (the pinned
  corpus commit, unchanged). Prompt saved token-redacted to
  `worklog/session-diff-fix-closing-prompt.md`. Baseline built clean
  (linux-release): suite 978 ran / 953 passed / 25 skipped
  (Windows-only) / 0 failed.
- T2/T3: diff-to-fix/ read in full (README/SUMMARY/matrix/PARAMETERS/
  SEVERITY-SCALE/SURFACE-INVENTORY + all 75 active findings + evidence
  index); core read at HEAD; load-bearing absence claims re-verified
  first-hand by grep at `84a1832` (M-071 gates: rrecov.cpp 0 hits;
  S-043 orientation: 0 hits; S-044 no gates; M-079 no fill site;
  M-084 no ACCDB; M-090 no koly; M-091 the 64 MiB stand-in — all
  CONFIRMED at HEAD).
- Scope determination: M-086/M-087/M-089/N-107/N-127 target the
  rda-driver repo — OUT OF SCOPE under this session's two-repo grant;
  dispositions recorded in the finding files, not fixed here.
- Fix order (board-recommended, data-integrity first): M-071 → SEVERE
  tier (S-042, S-043, S-044, S-040, S-047 + the S-041 children) →
  MAJOR → MINOR → INFO/ACK → tracked-open.

Task ID: S10-F/T1 (session-10, the FUNCTION-LEVEL COMPLETENESS lane — opens in parallel with SA-10)
Agent: session-10 function-level completeness audit (lead lane S10-F)

LANE SPLIT (the push-time-reconciliation SOP applied at open, not at close —
the parallel SA-10 lane pushed its T1 first; this lane declares complementary
territory to avoid duplicate filings and ID collisions):
- SA-10 (theirs, kept above): TRIAGE (the 75-active flip-fixed re-verify),
  the DRWService install lifecycle deep-dive, the recorded-but-unfiled
  follow-ups, the never-cited residue re-sweep, gate-doc currency.
- S10-F (this lane): the FUNCTION-LEVEL COMPLETENESS pass — the mandate's
  "mark all remaining features, implementations, functions as finding too"
  taken literally against the corpus's own function inventories:
  ghidra-searchdll/*_functions.txt (9 engines, ~9,900 functions; the named
  behavioural set dispositioned first), ghidra-mo (30 .mo decompiles),
  ghidra-partition (19), ghidra-core (5), ghidra-drivers (7), ghidra-misc/
  ghidra-wave2/ghidra-repair (40) — every named function/class/feature
  mapped to its trail disposition (MATCH verdict, finding, FIXED record, or
  module coverage); the never-dispositioned residue becomes findings or
  explicit MATCH records this session. Deep first-hand comparison of each
  candidate before filing; evidence files under evidence/decompiled-quotes/
  s10f-*.

Baseline (identical to SA-10's, recorded independently): core HEAD 84a1832
(trail-only) / product-code tip b92ebef / corpus be0cfe9. Board at open: 75
active (7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO) / 250 FIXED / 2 tracked-open.
Next free IDs coordinated at push time: S-048+, M-103+, N-128+, I-031+,
A-012+.

### session-10/T1-parallel-lane — this lane's baseline (the all-findings fix session)

Repos: core `84a1832` (250 FIXED / 75 active: 7 SEVERE · 28 MAJOR · 35 MINOR · 5 INFO / 2 tracked-open), corpus `be0cfe9`. Scope per this session's grant: TWO repos only (core + corpus) — the rda-driver-repo-resident findings (M-086, M-087, M-089, N-107, N-127's driver halves) get verified + core-side halves fixed + tracked-open push-forwards, not driver-repo edits. Baseline suite: linux-release 978 tests / 953 pass / 0 fail / 51 codec-gated skips, exit 0. Every batch-2 grammar re-verified first-hand at the decompile + the og binary's .data this session before any code: WMF placeable (:29807-45), MDB/ACCDB dual signature (:16497-16517 + ctor :16532-16546), HTML probe/walk (:14307-14395, :50817-84), SVG four-key chain + the .data decode (DAT_180093920=`"<svg "`, :928=`"<svg\r "`, :930=`"<?xml version="`, :948=`"<svg\n "`, :950=`"</svg>"`, :958=`"<svg>\r"`, :960=`"<svg>\n"`, :968=`"csl"` (the generic-XML marker), :970=`"<svg> "`, :940=0x1cd, :918=0x1cd), MOBI (:29369-29485: BOOKMOBI@+0x3C weak gate, EndTag 0x0E98E0D0A step 0x200 cap 0x1F400000), HIK (:30412-30560: pack len (buf[0xd]&7)+0xE, BB→BD gates, 1 GiB walk), AEP (:11550-11790: bare RIFX, BE-u32@+4, `<?xpacket end` 0x200-window scan, cap 0x1E00001, 8-NUL tail trim), JP2 (:21812-21935: jP/0x0D0A870A/ftyp + brand map jp2/jpx/jpm → 0x80/0x81/0x82, box length unchecked), LZH (:46937-47013: u32@+2 ∈ {-lh0,-lh5,-lhd,-lh7} + buf[6]=='-', entry-chain walk), VCF (:45316-45455: BEGIN:VCARD, END:VCARD\r\n walk, multi-card continuation, blank-line tail, cap 0x1400001), GPX (:20860-20936: decl + <gpx u32 window from +0x16, 0x200 iterations; config-gated registration), OSP (:50104-25: `{\r\n "di": ` u64+u16), POS (:20203-45: `{"diagram":` memicmp), MPEG-TS (:35578-35677, :38457-38536: 188/192 auto-detect by sync position + {0x40,0x41,'P',0x10} set, 0x400 packets/chunk, 0x20-flush, min 0x5000, cap global×10, M2TS stamp id 0x74), DMG koly (:63782-63868: 512-aligned first pass + 8-zero pre-gate, full-chunk second pass past 0x2800, size=hit+0x200), MDB page walk (:52001-39: 0x1000 strides, u16 word ∈ (0,0x901], cap 0x6400000). Prompt saved (already redacted at worklog/session-diff-fix-prompt.md).
---
Task ID: S38/T1+T4 (session 38 open — the minor-first fixing session)
Agent: session-38 diff-fix lane (this session)

Session open (T1): core `84a1832` (main, up to date with origin), corpus `be0cfe9`
(read-only), suite 1023/1023 green at open (linux-release, gcc Werror, clang-format
18.1.8 clean). Prompt saved token-redacted at worklog/session-diff-fix-38-prompt.md.
Scope decision recorded there: the user's session directive is MINOR-first (the 35-item
tier), then INFORMATIONAL acknowledgements, then SEVERE/MAJOR as runway allows; the
two-repo grant covers core + corpus ONLY — the rda-driver repo findings (N-107, N-127,
the driver-file halves of M-086..M-089) get precise push-forwards this session, never
cross-repo writes.

N-094 FIXED (the -h sessions verb): re-verified at current main first (usage block
rrecov.cpp:2996-3041, dispatch :3043-3063 — 10 lines listed vs 11 dispatched; the
finding's cites had drifted but the defect stood; ALSO the finding under-reported:
CORE-API-CURRENT:318-319 listed 11 verbs but counted them "(10)"). RED proven by the
new `CrUi.CliHelpListsEveryDispatchedVerb` pin (tests/unit/test_cr_ui.cpp): on pre-fix
main it failed on exactly one verb — 'sessions' dispatched but missing from -h. Fix:
one usage line (W1a form) + the CORE-API-CURRENT cite re-pin AND count repair + the
verb lock grown THREE->FOUR sites (API-DELTA rule box names the -h block; the pin
enforces both directions — dispatched=>listed and listed=>dispatched — plus the header
verb table) + API-DELTA §0's own "10 verbs" scope line corrected to 11. Suite
1024/1024 green, format clean. Audit trail: finding moved to findings/FIXED/ with
Resolution; RESOLVED-INDEX session-38 table opened; CROSS-REF-MATRIX row -> MATCH;
SUMMARY session-38 delta opened.

---
Task ID: FIX-T1/T2 (the closing-fix lane — M-071)
Agent: lead fix session (two-repo grant: core_recovery + corpus only)

Work Log:
- Baseline re-set after the parallel lanes' pushes (b478a4b read; product
  tip b92ebef): suite 1023/1023 linux-release green before any change.
- M-071 verified first-hand at HEAD: `rg -c NODE_FLAG_JUNK_FILTERED src/`
  → 3 hits, all core (filterfolder set/query gate) — 0 in rrecov.cpp;
  the cr_scan.cpp:1200 comment claims three gated surfaces; git
  archaeology re-confirmed (`69507cb` 2 gates → `cfa4c75` 0).
  CONFIRMED-DIVERGENCE.
- Fix: both rrecov.cpp gates restored byte-exact from the M-018 form
  (print_tree walk + recover-all plan lambda, continue-before-descent);
  the cr_ui bulk walks gated identically (plan_enqueue_session + the
  snapshot subtree plan — the W-lane surfaces postdate M-018 and never
  carried the gate; SnapNode.flags persists the flag for the value
  path). SafeWriter::enqueue deliberately NOT gated — the og's filter is
  a results-set default, not a hard block (explicit selection of a junk
  node stays possible; pinned by test). The cr_scan comment now names
  every gated surface truthfully.
- Tests: build_fat16_junk fixture (DESKTOP.INI + LFN "System Volume
  Information" with KEEP.ME inside) + scan_junk_to_session helper +
  RecoverAllSkipsJunkFlaggedNodes / RecoverAllSnapshotWalkSkipsJunk-
  FlaggedNodes / RecoverExplicitSelectionStillAllowsJunk. Fail-before
  EXECUTED: with src/ui/cr_ui.cpp stashed out, the two bulk-walk tests
  run RED (junk written), the explicit-selection test stays green;
  with the fix, 3/3 green. Full ctest 1026/1026, clang-format 18.1.8
  clean.
- Audit trail: M-071 Resolution recorded, file moved to findings/FIXED/;
  CROSS-REF-MATRIX D16/B2 row → MATCH; modules/writer.md M-071 → FIXED;
  SUMMARY opening section added (board 74 active / 251 FIXED).

Stage Summary:
- M-071 closed with the regression-proof pair the original M-018
  landing lacked. Next: the SEVERE tier (S-042 first — the WMF grammar
  contradiction, then S-043/S-044/S-045/S-040/S-047 and the S-041
  children).

---
Task ID: FIX-T3 (the closing-fix lane — territory claim + the thumbnail SEVERE cluster)
Agent: lead fix session (two-repo grant: core_recovery + corpus only)

Lane split observed at push time (the S-042/M-090 lane works the carve
grammar family; session-38 works the MINOR tier). THIS lane's claimed
territory, to avoid duplicate work: the thumbnail family (S-043, S-045,
M-080, M-083, M-085, M-102, N-092), the auto-save gates (S-044, M-079),
image create (S-040), and the remaining session/CLI majors. Work starts
at S-043 (EXIF orientation).

---
Task ID: SA-10B/T1 (the B lane — session-10 open; the fourth parallel lane's baseline)
Agent: session-10 B-lane parity audit (the user's "second session" — the sessions 1-9 corpus is "session 1")

THIS LANE'S DECLARED TERRITORY (complementary to SA-10's triage, S10-F's
function-level completeness, and the DFX-38 closing-fix lane; no overlap,
per the push-time SOP):

- L-A: the FIXED-purge EXECUTION — `git rm` the 250 findings/FIXED/
  files per the user's "remove the fixed things" (git history is the
  archive); a FIXED-LEDGER keeps the IDs navigable; gate-doc updates.
- L-B: mine the corpus's three REAL og .rsf sessions
  (og-codebase/SaveScan/auto_*.rsf — ~2M rows, parsed with the core's
  own scripts/compare_og_rsf.py) for tree-shape/naming ground truth the
  prior sessions never extracted: the Reconstructed/<volume> carve
  grouping, the %s_%06d per-file naming contract, the File Path Lost /
  LostDir<record> orphan grouping. An executed-comparison class NEW
  this session — the first mining of the real session payloads.
- L-C: og shipped-data-table parity — raw_signs.json regen sync
  (EXECUTED at lane open: MATCH — 139 formats / 199 header / 62 tail
  signs byte-identical after scripts/gen_rawsigns.py regeneration; the
  only diff is the clang-format 2-per-line packing of the committed
  file); EUUserData; config/*.ini at value level where gaps remain.
- L-D: the product-frame capability census (PDFViewer / EUOfficeViewer /
  EUHwpViewer / the repair line / iCloud / Dict / Downloader) — verify
  every og product-frame binary has a trail disposition; file the
  remainder per "mark all remaining features, implementations,
  functions, as finding too".
- L-E: spot-verification of the active board at the current tree; close.

Baseline (cloned fresh this lane): core_recovery @ 84a1832 at clone; the
tree now sits at origin/main 62074bf (product-code tip b92ebef + the
parallel fix lanes' S-042 fix f6e303e + N-094 fix 26da616 — both flip
FIXED at this lane's board pass); corpus @ be0cfe9 (unchanged). Board at
open: 75 active / 250 FIXED / 2 tracked-open (73 active after the
S-042 + N-094 flips). IDs this lane intends: S-048+, M-103+ (the
declared free set; push-time reconciliation if the parallel lanes
collide).

NOTE: this lane's first three push attempts were rejected by the
parallel lanes landing concurrently (the SOP's moving-target
environment); the botched local rebase states were discarded each time —
origin's worklog was never touched by them.

Prompt saved redacted: worklog/session-10-parity-audit-prompt.md (the
remote lanes' copy — same mandate — kept). Read twice.

---
Task ID: DFX-38/recon-1 (the M-071 convergent-landing reconciliation + the CLI execution proof)
Agent: lead (DFX-38)

Work Log:
- Convergence: this lane's M-071 fix (10fde58, prepared in the same
  window) and the parallel lane's landing (ce2405a) were semantically
  identical — the four bulk surfaces gated, explicit selection ungated,
  the comment truthful; both lanes independently discovered the ABI
  twins. The parallel lane landed first; this lane's overlapping code
  was discarded at rebase (kept on branch dfx38-local for the record).
- Donated on top (the landed Resolution's own proof gap - "the
  rrecov.cpp pair cannot execute on Linux"): tools/gen_fat16_junk_
  fixture.py (deterministic FAT16 fixture with the og's FileName-pool
  junk set; verified against the real orchestrator first-hand:
  5 files, junk_filtered==2, controls clean) + the checks.yml "CLI
  junk-gate probes" step (Windows-lane execution proof: scan/tree
  never print DESKTOP.INI and always print DOC.TXT; recover --all
  never writes the junk set, always writes DOC.TXT - driverless via
  the classification-(b) image path).
- Resolution addendum added to the FIXED finding (the convergence
  record + the new artifacts).
- Board re-read after the parallel lane's landings (S-042 f6e303e,
  N-094 26da616, M-071 ce2405a all FIXED): this session's SEVERE queue
  loses S-042; the MINOR queue loses N-094. Suite on the merged tree:
  1031/1031 green (linux-release).

Stage Summary:
- M-071 fully closed by convergence with the execution-proof gap
  filled. The DFX-38 queue continues at the SEVERE tier: S-043
  (thumbnail EXIF orientation) next - the parallel lane has not
  touched it.


---
Task ID: DFX-38/T-territory (the lane's declared territory — no overlap with the active lanes)
Agent: lead (DFX-38)

Territory (observed at origin/main 1e7c963; complementary to FIX-T3's
thumbnail/auto-save/image-create claim, the S-042/M-090 carve-grammar
lane, SA-10B's purge/mining/census, and S10-F's completeness lane):

- M-088 (FloorVersion pin split + the stale CI-shape doc cluster)
- M-082 (DeviceManager offline-OS/boot-volume identification)
- M-101 (Versions + Tags Extracted tree branches)
- M-100 (archive browse-inside preview)
- The doc-truth/config MINORs NOT in the other lanes' families:
  N-090, N-091, N-093, N-095, N-096, N-097, N-098, N-099, N-100,
  N-101, N-102, N-104, N-106, N-108, N-116..N-126 (skipping N-092/N-103
  thumbnails, N-105 auto-save, the carve children N-111..N-115, and the
  out-of-grant driver findings N-107/N-127)
- The INFORMATIONAL dispositions (I-026..I-030) and the tracked-open
  items (OQ-S4-3, OQ-S8-1)

Order: M-088 (docs, quick) -> M-082 -> M-101 -> M-100 -> the MINOR
batch -> INFO/tracked-open. Re-pull before every push; convergences
reconciled per the M-071 pattern (the landed fix stands; unique
evidence donated on top).

## Fix session — territory declaration (post-S-044, for the parallel lanes)

Reading the pushed worklogs (fix-lane claims the thumbnail family + image
create; DFX-38 claims M-088/M-082/M-101/M-100 + the non-thumbnail/carve
MINORs + INFO + tracked-open; SA-10B claims real-rsf mining/data-table
parity/product-frame census): **this lane takes the CARVE lane** — S-047
(the MPEG-TS/M2TS sync-scanner seam), the S-041 school-2/3 children
(M-092 HTML, M-093 XML+SVG, M-096 MOBI, M-098 HIK, M-099 AEP, N-111 JP2,
N-112 LZH, N-113 the text school, N-115 OfficeTagFromXML), the batch-1
verification failures M-090 (DMG koly walk) / M-091 (MDB page chain) /
M-084 (ACCDB second signature) / M-085 (TIFF LZW, if it lands on the
carve side) / N-093 (batch-1 bounds), and the carve-side MINORs not
claimed by DFX-38. S-044 was claimed by the fix-lane's worklog but landed
here first (`7b7e095`) — the gates + tests + D-105/D-106 registrations
are on main; the fix-lane should re-verify against the landed state per
the SOP before touching it.


## Session 10 — the continuation deep audit (the second conversation session: re-verify the board, remove the fixed, then go deeper than the artifacts ever went)

Date: 2026-10-28 (evening). Audited: core `84a1832` (= origin/main, verified
0/0 after fetch — the session-9 close commit), corpus `be0cfe9` (unchanged
since session-8's T1). Prompt saved redacted to
`worklog/session-10-deep-audit-prompt.md`; read twice before work.

User directive (this session's delta over the standing SOP): (1) look for
what session 1 missed; (2) REMOVE THE FIXED THINGS — re-verify the active
board against HEAD and move anything the fixing lanes landed to FIXED/;
(3) go deeper — more og-codebase ↔ core comparison; (4) mark ALL remaining
features / implementations / functions as findings.

Board at session open (per the session-9 close): 76 active
(7 SEVERE / 28 MAJOR / 35 MINOR / 5 INFO) + 1 tracked-open (OQ-S4-3) /
250 FIXED. Scope note: this session's prompt pins TWO repositories (core +
corpus); the rda-driver repo is OUT of this session's scope (its standing
findings M-086..M-089/N-107/N-127 stay on the board untouched; no new
driver-repo work this session).

Plan (the session TODO spine): T1 bootstrap (this entry) → T2 read the full
active board into a verification checklist → T3/T3b re-verification sweep of
all 76 actives vs HEAD `84a1832` (first-hand source reads; anything the
lanes landed moves to FIXED/ with a resolution record) → T5/T6 deep-dive
target discovery at the og-codebase FILE level and the corpus FUNCTION
level (the trail's coverage is artifact-doc-shaped; the user wants the
tree itself walked) → T7-T9 comparison waves (config/data assets; the
PartitionRecovery 57-file MOM tree; the diskhealth 818-file tree) → T10
new findings + matrix rows → T11 gate-doc truth pass + final push.
---
---
---
N-092 FIXED (the thumbnail refusal matrix's unnamed trio + the cache scope): re-verified
at current main first (sniff cr_thumbnail.cpp:201-228 no metafile/icon detection; header
deferred list unnamed) + corpus first-hand (DRWUI_imports.txt GDI+ block ~:3356 incl.
GdipGetImageThumbnail — GDI+ rasterizes WMF/EMF/ICO natively, the og's own path). Fix per
the finding's plan: all five magic forms (placeable WMF 0xD7CDC69A, standard WMF
type/headersize, EMF " EMF"@+40 — sniff buffer 16->48, ICO, CUR) named-and-refused
(Unsupported with source_format set); header contract + v1-matrix comment extended; the
cache scope DECIDED and registered (no engine-side cache — og caches UI-side in
ThumbnailFactory; core's in-process doctrine keeps make_thumbnail stateless; LRU-if-ABI
candidate noted for M-102) in modules/preview.md + cr_thumbnail.h. RED proven
(MetafileAndIconFamiliesAreNamedRefusals failed all five forms on the pre-fix sniff via
the stash-revert build), GREEN after; FormatSupportIsTheDocumentedV1Matrix extended.
Board now 74 active (6 SEVERE · 28 MAJOR · 33 MINOR · 5 INFO) / 253 FIXED / 2 tracked-open.

N-094 residual closed (the session-10 sweep addendum's post-fix check): CORE-API-CURRENT
headline :14 carried the third stale verb count ("9 verbs", cite rrecov.cpp:2534-2551) -
the sweep found three contradictory counts coexisting in one doc (:14 "9", :319 "(10)"
listing 11 names, :37 "11"). The :14 headline now reads 11 verbs with the re-pinned
3043-3063 cite and the explicit verb list (the mechanical sweep the addendum prescribed:
all count/cite sites, re-asserted against the dispatch table itself). No code change;
the CrUi.CliHelpListsEveryDispatchedVerb pin already guards the source-level truth.

### session-10 (this lane) — territory declaration (after the M-090/M-084/M-091 landings)

Landed so far this lane: M-090 (771e2ec), M-084+M-091 (8a7055e). TERRITORY DECLARED to prevent collision with the parallel fix lanes (603e955's carve lane took S-047 + the S-041 school-2/3 children + the batch-1 verification failures; DFX-38 took M-088, M-082, M-101, M-100 + the non-thumbnail/carve MINORs + INFO + tracked-open): THIS LANE takes the thumbnail/preview family (S-043 EXIF orientation, S-045 format coverage, M-080 budget, M-083 grade promotion, M-085 TIFF LZW, M-102 ABI export, N-103 API-row truths) + the session/query/CLI/writer/check majors (M-072 query axes, M-073 scan-time folder filter, M-074 dest codes, M-075 chkdsk, M-076 TRIM rows, M-077 preview caps, M-078 BitLocker turn-off, M-079 autosave label) + S-040 image create + the driver-repo-resident dispositions (M-086, M-087, M-089, N-107, N-127 — two-repo scope: verify + core-side halves + tracked-open push-forwards). Newly filed findings (S-048, S-049, M-103 from the SA-10B audit lane) coordinate at push time.

---
Task ID: SA-10B/T2-T7 (the B lane's main wave — the real-rsf mining findings, the census, the purge, the gate docs)
Agent: session-10 B-lane parity audit (this lane)

Work Log (the wave, consolidated):
- T2 (L-B executed): parsed all three real og sessions with the core's own
  compare_og_rsf.py (292 MiB record-4 payload, consumed-to-the-byte clean);
  extracted the root sets (NTFS / File Path Lost / Reconstructed in every
  session), the carve-branch shape (100% of 189,115 carved rows under
  Reconstructed/<volume-display-name>), the naming contract census (34
  formats' EXT_%06d + the 5 content-derived prefix families), the LostDir
  grouping (398 folders / 938,982 rows in H), and the carved-row field
  values (size=0, confidence empty, ALL deleted-flagged). The DRWUI naming
  function decoded at FUN_140c44770 (%s_%06d + the >99999 fallback); the
  LostDir%d sites re-read at NTFSSearchDLL:42041/:48915 and
  exFATSearchDLL:5182. Evidence file written:
  evidence/executions/s10b-real-rsf-session-mining.md.
- T3: three findings filed — S-048 (SEVERE, the Reconstructed/<volume>
  grouping absent), S-049 (SEVERE, the naming contract; N-063's recorded
  remainder resolved + the MP4/MOV template-vs-EXT nuance recorded for the
  fix lane), M-103 (MAJOR, LostDir per-parent grouping + the File Path Lost
  branch name). First-hand core cites re-verified at the moving tree
  (cr_scan.cpp:151/:202, cr_carve.cpp:907, cr_mftwalk.cpp:1091).
- T4 (L-C): the CHK/ln chain, EUUserData, Dict, config/*.ini census — every
  row dispositioned (F2/D-104, M-023/M-073, I-027, N-064); CLEAN. The
  raw_signs regen-sync MATCH recorded (executed at lane open).
- T5 (L-D): the product-frame census — every og binary mapped (viewers ->
  UI-layer OOS rows; iCloud -> EXCLUDED-NAS; repair binaries -> the one
  HOLE); S-050 filed (SEVERE): the file-repair line unimplemented AND the
  SURFACE-INVENTORY's CAPABILITY-MATRIX §3 citation false (§3 lists only
  Cloud/NAS/BitLocker-derivation/signing/.rsf-import). The row corrected
  in place to cite S-050.
- T6 (L-A, the user-mandated purge): FIXED-LEDGER.md generated (the 255-row
  index + the 4 mid-flight landings appended at the rebase = 259);
  findings/FIXED/ git-rm'd; the push-time reconcile absorbed the parallel
  fix lanes' concurrent moves INTO the purge (M-084/M-091/N-092/N-094/
  RESOLVED-INDEX modify-delete conflicts resolved by retiring them too +
  ledger rows). Purge landed at origin as 530fa32; findings/FIXED/ does
  not exist on main.
- T7: gate docs — SUMMARY's SA-10B delta block (with the prior-header
  note), CROSS-REF-MATRIX's SA-10B session block (S-048/S-049/M-103/S-050
  cells), README's session-10 block, SURFACE-INVENTORY §5's File-repair
  row corrected, modules/carve.md + modules/ntfs.md addenda.

Stage Summary:
- 4 new findings (2 SEVERE + 1 SEVERE product-frame + 1 MAJOR), 1 executed
  MATCH, 1 census CLEAN, 259 findings retired per the user instruction,
  all gate docs current. Board at this stage: 70 active (8 SEVERE · 25
  MAJOR · 32 MINOR · 5 INFO) / 259 FIXED-in-history / tracked-open
  unchanged. No source code modified — diff-to-fix/ + worklog* only
  (verified per wave).

---
Task ID: SA-10B/T8 (the lane close — the handoff)
Agent: session-10 B-lane parity audit (closing)

THE LANE'S PRODUCT (all pushed; this lane's commits: 1e7c963 T1 baseline,
e27e7c1 the L-B findings, 530fa32 the purge, ea2f3ea the gate docs,
6dbb171 the convention note, + this close):
- **The user's "remove the fixed things": EXECUTED.** 259 resolved findings
  retired to history; findings/FIXED/ does not exist on main; the
  FIXED-LEDGER is the index; the go-forward convention is recorded (fix
  lanes retire to history + ledger, never re-create the directory).
- **4 new findings, every one evidence-complete** (executed + decompile +
  first-hand core cites): S-048, S-049, M-103 (the real-.rsf mining — the
  corpus's three REAL og sessions mined for the first time: tree shape,
  naming contract, LostDir grouping; the evidence file carries the verbatim
  parser outputs) and S-050 (the product-frame census's one hole: the
  file-repair line, unimplemented AND untruthfully registered).
- **1 executed MATCH recorded** (raw_signs.json regen sync — the compiled
  layer-1 table byte-identical to the og's shipped JSON) and **2 census
  CLEANs** (og data assets; product-frame binaries).

COVERAGE (this lane): the three real sessions parsed to their last byte;
the naming/tree ground truth extracted for 34 carve formats + the orphan
branch; the product-frame binary set enumerated and disposition-mapped;
the shipped data tables compared at byte level. Read-only guarantee:
verified per wave — this lane's commits touch ONLY diff-to-fix/ +
worklog* (every `git show --stat` checked before push).

HANDOFF — the recommended first fixes from THIS lane's filings (priority):
1. **S-048 + S-049 + M-103 as ONE tree-shape wave** (they share the
   evidence file, the fixture source, and the session round-trip test
   seam): the Reconstructed/<volume> children first (S-048 — smallest,
   highest user impact), then the EXT_%06d namer + the per-analyser
   prefixes (S-049 — the prefix extraction is per-format, landable
   incrementally), then the LostDir grouping (M-103 — the parent-record
   is already at the reparent site). Fixtures MUST be spec-derived (the
   S-042 class lesson, recorded in the evidence file).
2. **S-050's registration repair immediately** (a 2-line doc fix — the
   SURFACE-INVENTORY row already corrected by this lane; the
   CAPABILITY-MATRIX §3 decision row is the fix lane's first commit),
   then the build-vs-exclude decision with the corpus's 8 FileRepair
   artifacts as the build contract if (a).
3. The parallel fix lanes' queues continue: S-043 (thumbnail family,
   claimed), S-047 + the S-041 school-2/3 children (carve lane, claimed —
   NOTE: the real-session extension census now ranks the school-2/3
   formats by real-world volume: GZ > SVG > PNG > ICO > wav > HTML > JPG —
   GZ/HTML/SVG first maximizes real-user impact), M-072..M-078 (the
   R-row batch, claimed).
4. The remaining SEVEREs after those queues: S-040 (image-create), S-045
   (thumbnail format coverage phase 2), S-041's parent row.

OPEN ITEMS THIS LANE LEAVES (none new to tracked-open; recorded inside the
findings): S-049's two open questions (the JPG Model-vs-Software
precedence fixture; the MP4/MOV template-vs-EXT interplay decode) and
S-050's product decision. The 2 existing tracked-open (OQ-S4-3
execution-bound; OQ-S8-1's consumer question) are unchanged.

Board at lane close: **70 active (8 SEVERE · 25 MAJOR · 32 MINOR · 5 INFO)
/ 259 FIXED-in-history / 2 tracked-open** — at origin/main 6dbb171-lineage
(+ this close). The parallel-lane environment is ACTIVE; pull before
every wave; the counts move every few commits.
Task ID: DFX-38/M-088 (the FloorVersion pin split + the CI-shape doc truth + the doc-truth gate)
Agent: lead (DFX-38)

Work Log:
- Verified M-088 at HEAD programmatically (display artifacts ruled out by
  design): release.yml:291 v0.2.4 vs installer.yml:118 v0.2.3 (the split);
  CI-CURRENT.md:60's "ONE pin source" claim false; installer.yml absent
  from the lane table; README:125-132 describing the deleted Ubuntu/CodeQL/
  API-benchmark lanes; PREBUILT-ARTIFACTS citing the deleted composite
  action + three deleted workflows as the current mechanism.
- Byte-level verification note: the workflows' `branches: [main]` lines
  DISPLAY as `branches: ain]` through this container's tool-output
  sanitizer ([m is eaten as an escape) — proven by hex dump
  (5b 6d 61 69 6e 5d) + GitHub's live run list firing on push to main. No
  false finding filed; recorded so future sessions hex-verify before
  trusting the display on bracket-bearing lines.
- Fix: installer.yml → v0.2.4 (both pins equal, matching the M-061
  teardown contract's versioned-uninstall floor); CI-CURRENT.md rewritten
  (4-lane table + Files + the honest two-live-sites pin statement + the
  junk-gate probes and the new gate in the component list);
  PREBUILT-ARTIFACTS.md §2/§5 rewritten (fetch-release.ps1 as the one
  convention enforcer; the real 3-consumer table; the deleted trio
  preserved as marked history); README Status paragraph + the
  Documentation table's CI row; RELEASE-NOTES correction line;
  CI-SCENARIOS lane-truth banner + the S-DRV-REFUSE row; CI-LOCAL
  supersession banner + cost-table fix; DRIVER-RELEASE/DRIVER-CONTRACT
  citations marked historical.
- NEW GATE: tools/doc_truth_gate.py (the finding's own acceptance
  criterion): (1) every live -FloorVersion pin is one set at one floor and
  matches CI-CURRENT's declared set; (2) deleted CI surfaces are cited
  only with historical markers. First run FAILED with 8 violations
  (fail-before executed) — including 4 sites the finding's census missed
  (DRIVER-UTILIZATION:145/:252, PERFORMANCE-BASELINE:142,
  benchmarking.md:36, PLAN-CI-REBUILD §3.2-3.4/acceptance) — all fixed;
  green after (38 historically-marked mentions). Wired into checks.yml's
  lint job (runs before any build compute).
- Both edited workflows YAML-parse clean; doc_truth_gate green; local
  suite unaffected (docs+scripts only).

Stage Summary:
- M-088 closed with the prevention mechanism the finding asked for. The
  DFX-38 queue continues at M-082 (DeviceManager offline-OS
  identification) — the biggest unclaimed major.


---
Task ID: DFX-38/M-088-recon (the purge-world adaptation)
Agent: lead (DFX-38)

Work Log:
- The SA-10B purge landed mid-flight (255 FIXED files retired to git
  history + FIXED-LEDGER.md). This lane's M-088 landing was re-cherry-
  picked onto the purged tree and ADAPTED: the findings/FIXED/M-088 file
  retired per the new convention (full Resolution preserved in git
  history), the ledger row added (261 resolved), the matrix row relinked
  to the ledger, the SUMMARY counts restated as the post-purge filesystem
  truth (69 active: 8 SEVERE incl. the new S-048/S-049/S-050 filings ·
  24 MAJOR · 32 MINOR · 5 INFO).
- NOTE for all lanes: this lane's session mandate says "No finding file
  deleted. The audit trail is preserved." — the purge came from a
  DIFFERENT session's mandate ("remove the fixed things"). Both are now
  reconciled: history preserves every Resolution (git log --follow), the
  ledger keeps every ID navigable, and NEW resolutions by this lane are
  recorded as ledger rows + SUMMARY entries going forward.

Stage Summary:
- M-088 fully closed in the purge world. DFX-38 continues at M-082.
---
N-116 FIXED (the session-list row ordering): re-verified at current main first (path-byte
sort in list_crsf_files both branches; session_list_impl filled in that order; the CLI
re-sorted by path with its own comment pinning the wrong contract). The finding's open
question settled first-hand: FUN_140273550 (DRWUI_decompiled.c:432112-142, re-read this
session) returns !(ftA < ftB) over the record-2 SYSTEMTIMEs = NEWEST-FIRST (the ordering
the og's Continue-last banner depends on; the identical side-calls in both branches are
a decompiler artifact). Fix: session_list_impl decodes every candidate then sorts
save_unix DESC, path-ascending tie-break; the TRUNCATED under-capacity answer is the
time-ordered prefix (the NEWEST rows, not an arbitrary path prefix); cr_ui.h two-call
contract + ABI-CONTRACT 7f re-worded; the CLI sessions verb re-sorts by the same key.
RED proven (SessionListOgTimeOrdering: row 0 was the path-first OLD session), GREEN
after (row 0 = the path-last NEW one); tie-break + CLI-source-agreement pins included.
Suite 1041/1041. Style repair rode along (the parallel S-044 landing 7b7e095 carried
clang-format violations in cr_ui.h/cr_ui.cpp/test_cr_ui.cpp - reformatted, gate green).
Board (filesystem-counted): 70 active (7 SEVERE · 27 MAJOR · 31 MINOR · 5 INFO) / 258
FIXED / 2 tracked-open.

N-116 trail conform (the purge convention): the replayed N-116 landing (632d21e) moved
the finding file into findings/FIXED/ under the pre-purge convention this session
started with; the SA-10B purge (user-mandated) established the go-forward rule while
the session ran - ledger row + git rm, never a re-created FIXED/ dir. This commit
retires the file into git history (the full Resolution + the RED->GREEN proof are
preserved at 632d21e) and appends the ledger row. S-043's file (the parallel lane's
b3938c8 landing) also sits in the re-created dir - noted in the ledger's counts line
for the next audit lane to retire; not touched here (their lane may still be working).


---
Task ID: DFX-38/M-082 (the DeviceManager offline-OS profile — the capability lands)
Agent: lead (DFX-38)

Work Log:
- Re-verified the finding at HEAD: reghive is FVE-only (cr_reghive.h's own
  header notes the og "does NOT consult the Select key" for FVE while
  DeviceManager DOES); cr_bootfix takes a caller-supplied BCD path; no
  offline-OS identification anywhere (grep: IsBootVolume/
  PartitionBootInfo/EASEUS_HMG — 0 hits trail-wide, re-confirmed).
- Corpus re-read before implementation (the full walk :62034-62330), plus
  ONE first-hand binary decode the finding's citation set did not carry:
  DAT_18007d37c (the ControlSet rendering format) = "%03d" — read from
  the shipped og-codebase/PartitionRecovery/DeviceManager.mo .rdata
  (file offset 0x7bf7c) this session. The first implementation draft used
  plain to_string; the SystemPartition test went RED (Current=1 -> my
  "ControlSet1" vs the fixture's real-Windows "ControlSet001"), and the
  binary decode settled the %03d truth. The corpus-first discipline
  working as designed.
- Landed: include/cr/reghive/cr_offlineos.h + src/core/reghive/
  cr_offlineos.cpp — the og's 0x418-byte CPartitionBootInfo walk over the
  S-014 regf parser: the 8 CurrentVersion identity fields (0x80-capped,
  present-if-found), the SystemRoot lowercase+drive-letter gate, the
  \MountedDevices "\DosDevices\<x>:" value read (leading-backslash value
  names; exactly-12-byte gate: u32 signature + u64 offset), the
  \Select\Current resolution (001 fallback, %03d rendering), and the
  ProductOptions classification (WinNT->1, LanmanNT->2, ServerNT->3,
  case-insensitive). Core-only C++ API — no ABI export (the M-082 fix
  plan's own staging; PARITY-DELTAS D-107 records the decision + the
  boot-fix auto-locate follow-up consumer).
- Tests: the 5-test OfflineOs.* family (identity fields + the
  \Device\-root rejection; Select=2 -> \ControlSet002 + the 001
  fallback; the 12-byte MountedDevices decode with a decoy letter;
  the 5-case ProductType classification incl. case-insensitivity and
  unknown; the missing-keys tolerance trio). All RED-by-absence
  pre-landing. Suite 1045/1045 linux-release; clang-format clean.
- Docs: mom-capabilities.md re-scoped (the dead-shipped verdict covers
  ONLY the CheckVersion copy; the live DeviceManager copy implemented);
  PARITY-DELTAS D-107; the FIXED-LEDGER row.

Stage Summary:
- M-082 closed: the offline-OS identification capability exists, is
  corpus-exact (including the %03d decode), and is test-proven. The
  bootfix auto-locate wiring is the recorded follow-up consumer.
---
Doc-truth batch A (N-117 + N-118 + N-103, one docs commit per the T12 module-batch rule):
all three CONFIRMED at current main before any edit. N-117: PLAN-API-EXPANSION:40 named
the un-landed cr_ui_session_load_cancel(h) shape (the shipped surface is the cancel_flag
arg on load_with_events, cr_ui.h:933-944) + ABI-CONTRACT 479/525 cited stale cr_ui.h
ranges (823-865/867-886 vs the measured 865-914/916-944) - both repaired. N-118: the
R-52/R-54/R-56 gap rows read PARTIAL with pre-landing claims while W1a/W1b/W1c + the
S-044 gates are landed and test-covered; R-31's "Missing" components all dispositioned
(M-024+D-97 toggles; usefullostfiles OOS-UI at OGI:64; PriorityDisplayType via the
query/filefilter data layers) - all five rows + the second table's three flipped to
PRESENT with landing cites and the real residuals (N-090/N-091) named. N-103: corpus
import tables re-read first-hand (heif_image_handle_get_thumbnail = EUImgRepairFile
ONLY at ghidra-repair/EUImgRepairFile_imports.txt:6; EURawImg carries the decode set,
no get_thumbnail) - the two R-28 rows, the PLAN-API-EXPANSION:81 row, and the OGI
heif.dll attribution line corrected; M-102 remains the tracked ABI handoff.

Doc-truth batch A conform (the ledger convention): N-117/N-118/N-103 retired into
history (their Resolution sections preserved at 954c386) with ledger rows appended;
matrix + SUMMARY board updated. Filesystem truth: 63 active (7 SEVERE · 23 MAJOR ·
28 MINOR · 5 INFO) / 265 resolved / 2 tracked-open.
## Fix session — S-047 landed (the MPEG-TS/M2TS sync scanners)

- **S-047 FIXED** — the corpus re-read first-hand (registrations :3143-3149/
  :3173-3179; the chunk handler FUN_18002e430 :35578-35677; the packet walk
  FUN_180032460 :38457-38536). FMT_TS + FMT_M2TS rows (1-byte 0x47 magic;
  sync anchors +0/+4 = 188/192-byte strides — the og's own detect order);
  validate_ts_common: the follow-byte set {0x40,0x41,'P',0x10}, the
  block-buffered strided walk, all-0xFF null-packet tolerance ("+null"
  evidence marker), sync-loss run commit, 0x5000 min extent, 200 MB x10
  cap, the m2ts extension stamp. suppress_overlap on both rows (the
  repeating-sync family). The 2 KiB progressive first block keeps noise
  candidates cheap (1-byte magic amplification bounded: measured 54 ms for
  1,769 hostile candidates). kTrialOrder: M2TS before TS. The
  WindowServing test invariant updated (TS trials on 0x47+set payload
  bytes are expected now; the device-call count stays the point). Tests:
  6 new Carve.* TS tests; suite 1043/1043; ASan+UBSan lanes clean over
  carve (117/117 each). Audit trail: finding -> FIXED/ with Resolution;
  matrix + carve.md + S-041 pointer + SUMMARY counts updated
  (72 active / 253 FIXED).
---

Doc-truth batch B (N-095 + N-108 + N-119, one docs commit): all three CONFIRMED at
current main first. N-095: the 0x60002400 GetProcName relabel at both REFERENCE-DRIVER-
BEHAVIOR sites (I-011 #8); the 37->34 toggle recount + the 1,481-unique nuance across
the five PARITY-CORE-REC/ARCH/UI sites; carve.md's OQ-SA-3 D-104 supersession note;
transport.md's disk template corrected to the decompile's 2-segment %d\\%d form
(Device_decompiled.c:50628 re-read via the s4 evidence). Item 5 (the ABI cites) landed
via this session's N-117. N-108: PARAMETERS.md's two constant rows rewritten to the code
truth (1 MiB + 4 KiB overlap; 12/48/1024/80 caps with code cites); the rawsigns.h comment
and the SCAN-ENGINE-DELTA strike-through were ALREADY fixed by parallel lanes - recorded
as such. N-119: D-88 re-adjudicated - the EuShare artifact (eumrx-nas-architecture.md:10-21,
re-read first-hand) answers the semantics (on-box bitmap scan via the SSH-root agent); leg
(2) struck, the trigger marked FIRED-and-adjudicated, the exclusion stands on scope alone.

Doc-truth batch B conform: N-095/N-108/N-119 retired into history (Resolutions preserved
at e1e5124), ledger rows appended, matrix rows flipped, SUMMARY board updated.
Filesystem truth: 60 active (8 SEVERE · 22 MAJOR · 25 MINOR · 5 INFO) / 268 resolved /
2 tracked-open. Session-38 tally: 10 findings closed.

Task ID: FIX-T4 (S-043 closed)
Agent: lead fix session (closing-fix lane)

Work Log:
- S-043 verified first-hand: cr_thumbnail.cpp had zero orientation code
  (grep + full read); the corpus evidence (FUN_1402fcc30 :521694-725,
  call order :652307-309, s8-thumbnail-pipeline.md §1) re-read directly.
  CONFIRMED-DIVERGENCE.
- Fix: apply_orientation (the 8 EXIF cases as the og's RotateFlipType
  mapping, cases 5-8 swapping axes) applied to decoded RGBA BEFORE the
  box downscale (the og's RotateFlip-then-GetImageThumbnail order).
  JPEG: ExifWalker gained ifd0_orientation() (APP1/IFD0 tag 0x112,
  type 3 count 1, 1..8; anything else = no transform = the og's
  GetPropertyItemSize==0 branch). TIFF: tag 274 captured in the native
  decoder. Non-JPEG stb formats stay untransformed (the og reads no
  0x112 property for them either).
- The embedded fast path (the finding's conditional made precise):
  serves as-stored per CIPA DC-008 (embedded thumbs are stored upright -
  applying IFD0's tag would double-rotate spec-conformant files);
  honors an IFD1-DECLARED 0x112 as the not-already-oriented marker.
  The og has no embedded fast path (it full-decodes) so the corpus has
  no statement on that leg - the rule is spec-derived, registered
  PARITY-DELTAS D-105, both behaviors pinned by test.
- Tests: 8 new (ExifOrientationAppliedBeforeDownscale over 2..8,
  OrientationOneIsIdentity, OrientationInvalidIgnored, TiffOrientation
  Applied/InvalidIgnored, EmbeddedFastPathServesStoredOrientationBy
  Default, EmbeddedFastPathHonorsIfd1DeclaredOrientation). Fixtures:
  exif_orient_{1..9}.jpg + tiff_orient6.tif + tiff_orient_9invalid.tif
  + huge_dims_exif_thumb_ifd1o6.jpg (gen_thumbnail_fixtures.py's
  exif_app1 parameterized; pre-existing fixtures byte-identical).
  Fail-before EXECUTED: with cr_thumbnail.cpp stashed, 3 transform tests
  RED (Exif/Tiff/EmbeddedIfd1), identity/invalid/serves-stored green.
  Suite 1041/1041 linux-release; thumbnail suite ASan-clean
  (build/linux-gcc); clang-format 18.1.8 clean.
- Audit trail: Resolution + FIXED/ move; matrix row -> MATCH;
  modules/preview.md S-043 -> FIXED; PARITY-DELTAS D-105; SUMMARY
  board 71 active / 254 FIXED.

Stage Summary:
- S-043 closed with the og's exact pipeline order. Next in the claimed
  territory: M-083 (grade promotion - the same work item's other
  side-effect), then M-080 (the budget middle-class), M-085 (TIFF LZW),
  S-044 (auto-save gates), S-045 (format coverage), S-040 (image create).

---
Task ID: FIX-T4b (S-043 push-time reconciliation)
Agent: lead fix session (closing-fix lane)

Work Log:
- The push-time reconciliation: a parallel lane had landed S-043's base
  (d0787e9: walker orientation read, apply_exif_orientation, the
  exif_orient{1..9} battery, embedded path excluded wholesale) while this
  lane's independent implementation was in flight. NOT a replacement -
  rebased as RIDERS on their base: (1) TIFF tag 274 (the GDI+ 0x112
  datum for TIFF inputs - their landing missed the TIFF leg);
  (2) the embedded path's D-105 rule made precise (serves as-stored per
  CIPA DC-008; honors an IFD1-DECLARED 0x112 as the not-already-oriented
  marker - the finding's own fix-plan conditional).
- Test batteries deduplicated: their ExifOrientation parameterized
  battery kept for the JPEG full path; this lane's unique riders kept
  (TiffOrientationApplied/InvalidIgnored, EmbeddedFastPathServesStored-
  OrientationByDefault, EmbeddedFastPathHonorsIfd1DeclaredOrientation).
  This lane's duplicate JPEG battery dropped (equivalent assertions).
- Fixtures: byteheavy12mp.jpg restored to SHA256SUMS (the M-080 lane's
  hand-spliced fixture, outside the generator); tiff_orient6.tif,
  tiff_orient_9invalid.tif, huge_dims_exif_thumb_ifd1o6.jpg added via
  the generator's new exif_app1_ifd1 parameterization.
- Board convention adopted (FIXED-LEDGER): the S-043 merged Resolution
  + M-080's file retire to history with ledger rows (both lanes had
  re-created findings/FIXED/ before the convention was readable - the
  ledger itself flagged S-043's row as pending).

Stage Summary:
- S-043 fully closed across both lanes. Territory re-read after the
  parallel landings: M-083 (grade promotion) is next - M-080/S-044/N-092/
  M-088/N-116 already landed by others. Then M-085, M-079, S-045, S-040.
---
Doc-truth batch C (N-104 + N-097 + N-102 + N-123 + N-101 + N-120 + N-099 + N-106):
every site re-verified at current main first. FIXED live this batch: N-104 (ldm.md's
dangling table.md ref dropped; CAPABILITY-MATRIX's HFS+ row flipped to DONE with the
engine cite re-verified first-hand at cr_hfsplus_engine.cpp:514); N-097 (the auto.txt
inventory row corrected to the FVE-key-snapshot truth - pipe create :430715 + rewrite
:430781-798 re-read first-hand; the plaintext-at-install-root security datum registered);
N-102 (the SURFACE-INVENTORY propagation note flipped to done - the retired M-024 file's
mentions stay historical); N-123 (cli.md's 7-verb row -> the 11-verb dispatch with the
wave cites + exit 4 added); N-101's one live residue (per-binary 113/114 -> 112/114 -
deepening-batch1 is cited by M-064); N-120's live residues (same arithmetic item; the
rest verified already landed). FIXED-ALREADY with reproductions: N-099 (E55 carries the
registry-region/94-binding corrections verbatim + the WPG byte-order fixes live at
SCAN-ENGINE-DELTA:78/260 + carve.md:630); N-106 (the SI:86 GetDriverInfo row with the
OUT-OF-SCOPE-by-lineage disposition + the N-071 sixth-source note); N-101's and N-120's
other items (the session-9 currency pass's arithmetic; the S-050 FileRepair pointer
repair; the EuShare count; the be0cfe9 pins).

Doc-truth batch C conform: all eight files retired into history (Resolutions preserved
at 13245c0), ledger rows appended (incl. the two FIXED-ALREADY classifications), matrix
rows flipped. Filesystem truth: 50 active (6 SEVERE · 22 MAJOR · 17 MINOR · 5 INFO) /
276 resolved / 2 tracked-open. Session-38 tally: 18 findings closed.


---
Task ID: SA-10/close (the lead lane's full record — triage + deep + mark-remaining + the five-lane reconciliation)
Agent: session-10 continuation parity audit (the lead lane, closing)

Work Log:
- T1 baseline: core 84a1832 / corpus be0cfe9; the 75-active board read in
  full; prompt saved redacted (worklog/session-10-parity-audit-prompt.md).
- The parallel lanes A-D ran (triage / DRWService deep-dive / residual sweep
  / gate-doc currency), then the LEAD first-hand re-verified every
  load-bearing claim before filing: the S-041 flip rationale, M-100's
  cr_preview.h state, M-105's core greps + R-83 + PACKAGING.md, the
  ONE/3DS/batch-1 stand-in sites, the LDM chain (cr_node.h:405-406 →
  cr_scan.cpp:1500 → cr_ui.cpp:3698-3707 → cr_safewriter.cpp:379), the
  gate-doc truths (counts, S-038 rows, row 67, A-005 duplicate).
- THE PUSH-TIME RECONCILIATION (the session's defining event, the SA-9
  scenario again): the first wave-1 push was rejected — 42+ commits had
  landed concurrently (the closing-fix lanes: S-042 f6e303e, S-044 9496f3c,
  M-071 dd0ffd5, M-090 771e2ec, M-084+M-091 16475b8, N-092 5ee3858, N-094
  26da616, S-043 b3938c8, M-080 5ccc0da, M-085 edee513, M-082 1f71488,
  M-088 41d8829, N-116/N-117/N-118/N-103; SA-10B: the real-rsf mining
  S-048/S-049/S-050/M-103 + the user-mandated FIXED purge 530fa32 + the
  FIXED-LEDGER go-forward convention; the wave-2 lane: the whole-disk
  findings; session-38: the doc-truth batches). The lane rebuilt everything
  against the new state:
  - ID collisions resolved: the wave-2 lane's S-050 (whole-disk) vs SA-10B's
    S-050 (file-repair) — the earlier push keeps the ID; the wave-2 file
    renumbered S-051 with all refs. This lane's own filings renumbered past
    the taken IDs: S-052 (LDM), M-105 (DRWService), M-106 (ONE/3DS), M-107
    (batch-1 residue), N-128 (CAPABILITY-Matrix).
  - Convention conformance: S-041 + N-101 retired to HISTORY with Resolution
    records (two commits: the records, then the retirement) + FIXED-LEDGER
    rows; the three pending files (S-043/M-080/M-085) conformed the same
    way; FIXED/ is gone from the tree again.
  - Convergence: this lane's M-100 re-scope + N-099 addendum duplicated the
    session-38 sweep's (dropped; one complementary N-099 note kept); the
    cite-refresh addenda dropped for every finding the fix lanes retired
    mid-flight (S-042/S-044/M-084/M-090/M-091/N-094/N-116); kept where live
    (N-090, I-029, M-079's siblings).
  - All five new findings' cites re-verified at the reconciliation HEAD
    (the carve file shifted ~50 lines with the M-084/M-091/M-090 landings;
    cr_ui.cpp shifted with M-071/N-092/N-116; MDB dropped out of M-107's
    residue list — resolved mid-flight; DRWService greps re-run clean).
- Gate-doc truth wave: SURFACE-INVENTORY rows 43/67/95; driver-install.md
  (S-038's K74/K77 flipped to FIXED, the "stays ACTIVE" note, the session-9
  addendum's M-071/M-072 renumber annotations, the M-105 DRWService
  addendum); carve.md (the landed-format annotations, M-081/S-041 currency,
  the M-106/M-107 addendum); ldm.md (the S-052 recovery-side addendum);
  CROSS-REF-MATRIX (the SA-10 block); SUMMARY (the delta block on top);
  README (the board line); FIXED-LEDGER (backfill + counts 265→270).

Stage Summary:
- Board at close (final push, absorbing the mid-flight S-047 `a3abfbe`,
  S-045-half `021ae07` + OQ-S10-1, N-095/N-108/N-119 `995ba5d`, and batch C
  `13245c0`+`0872ac2` (N-104/N-097/N-102/N-123/N-101/N-120 fixed +
  N-099/N-106 FIXED-ALREADY — all convergent with this lane's triage; the
  duplicate ledger rows deduped, this lane's N-099/N-120 addenda dropped as
  superseded): 54 active (6 SEVERE · 25 MAJOR · 18 MINOR · 5 INFO) / 277
  resolved-in-history / 3 tracked-open (OQ-S4-3, OQ-S8-1, OQ-S10-1).
  Next free IDs: S-053+, M-108+, N-129+, I-031+, A-012+.
- This lane's unique contributions: S-052 (the LDM span recovery misbase —
  the recovery-side half of B8/A27, GAP-S6-15's owner), M-105 (the
  DRWService K71-K73 decode), M-106/M-107 (the walk follow-ups owned), N-128
  (the CAPABILITY-MATRIX truth row), the S-050→S-051 collision repair, the
  S-041/N-101 closures, the five-lane reconciliation itself, and the
  gate-doc truth repairs (row 67, driver-install's S-038 rows, the ledger
  backfill).
- The mandate's three asks delivered: triage (0 falsely-active findings at
  the base; 2 flipped + 5 conformed at the reconciliation), deeper (the
  DRWService + LDM-recovery + whole-disk-scan surfaces), mark-remaining
  (every recorded-but-unfiled follow-up now owned — the sweep found and
  closed all six ownerless obligations: U-1..U-6 → M-106/M-107/M-105/N-128/
  S-052/direct-repairs).
- READ-ONLY GUARANTEE: this lane's commits touched NOTHING outside
  diff-to-fix/ + worklog* (git diff ea1ac86..HEAD over non-audit paths is
  empty — verified at close). The product-code changes in the tree entered
  exclusively through the pulled parallel lanes' landings.
- Handoff: the fix-order contributions are in SUMMARY's block. The carve
  school-2/3 children should land in SA-10B's real-world order (GZ/SVG/PNG
  first — the og's top-3 carve formats by volume). S-052's fix is
  architectural (the basis descriptor + the retained virtual devices) —
  start there.

---
Task ID: SA-10/truth-note (the close commit's message vs the final board)
Agent: session-10 lead lane

The pushed close commit's message (bb2cd8a) names the board as it stood when
the message was composed (66 active / 270 / 2 tracked-open); the three
subsequent rebase rounds (the S-047/S-045-half/N-095+N-108+N-119 landings,
then batch C's eight, then the 64a3b18 marker sweep) moved the board under
it before the final push. **The filesystem is the truth: 54 active (6 SEVERE
· 25 MAJOR · 18 MINOR · 5 INFO) / 277 in-history (FIXED-LEDGER.md, the 277
line on top of the dated snapshots) / 3 tracked-open (OQ-S4-3, OQ-S8-1,
OQ-S10-1)** — SUMMARY's SA-10 block, README's board line, and the worklog
close entry all carry these numbers. Recorded per the truth mandate (the
84a1832 precedent).
## Fix session — M-092 landed (the HTML carve chain)

- **M-092 FIXED** — corpus re-verified first-hand (the :14263-14281 chain ctor,
  the FUN_180012270 probe, the FUN_180042e60 size walk). Four rows
  (FMT_HTML/<h, FMT_HTML_H/<H, FMT_HTML_BANG/<!, FMT_HTML_UTF16/FF FE — the
  E59 BOM chain), one shared validate_html: the case-insensitive <HTML (5B) /
  <!DOCTYPE HTML (14B) / BOM+wide probes; the chunked ASCII </html> scan
  (1 MiB cap — NO wide needle exists in the DLL; the plan's parenthetical
  was an inference, corrected in the Resolution), sector round-up clamped
  to the slab, the bounded-cap fallback with the heuristic note. The
  Consistency vocabulary test updated (HTML checkable — the OQ-FLT-1
  registry-derived design). Tests: 4 Carve.Html*; suite 1071/1071.
---
Registration batch D (N-091 + N-098 + N-105): N-091's three unregistered deltas landed as
PARITY-DELTAS D-108 (the load-progress model: stage permille + the 0xFFFFFFFF sentinel vs
the og's fixed 40-65 pct window, the byte-field recomputation route documented), D-109
(the in-engine thumbnail decode superset, the I-015 lineage - previously only in OGI C.2 +
preview.md), D-110 (the auto-session-name sanitizer superset: backslash + C0 beyond the
og's 8 chars); the auto-save posture item was already covered by the parallel lane's
D-105 + D-12 (recorded as FIXED-ALREADY halves); a D-107 numbering collision between two
parallel lanes' rows was found and flagged with the go-forward rule. N-098: the
always-fresh enumeration contract landed (cr_ui.h's section + the CORE-API-CURRENT/API.md
mirrors + R-86 flipped to PRESENT) with `CrUi.EnumerationRefreshContractPinned`
(RED-then-GREEN proven by the stash-revert build: the sentence assertions failed pre-fix);
the pin also guards the local-list implementation shape and the absence of static state in
enumerate_disks_impl. N-105: FIXED-ALREADY - D-106 is the demanded deferral record (the
S-044 lane's rider), reproduction in the Resolution.

Batch D conform: N-091/N-098/N-105 retired into history, ledger rows appended, matrix
rows flipped, SUMMARY board updated. Filesystem truth: 47 active (6 SEVERE · 22 MAJOR ·
14 MINOR · 5 INFO) / 279 resolved / 2 tracked-open. Session-38 tally: 21 findings closed.

---
Task ID: FIX-T5 (M-102 + M-083 closed)
Agent: lead fix session (closing-fix lane)

Work Log:
- M-102 verified: zero cr_ui thumbnail symbols at HEAD (the engine's
  20+ tests validated a surface no C host could call). M-083 verified:
  zero grading references in cr_thumbnail.cpp; the og's promotion quote
  (:422498-518) re-read from the evidence file. Both CONFIRMED.
- Fix (one coherent surface): cr_ui_thumbnail_generate (two-call: rgba
  NULL = the size probe - the decode runs because dims depend on it;
  too-small buffer = 0x50030009 with bytes_needed, nothing partial) +
  cr_ui_thumbnail_format_supported. Caps mirror ThumbnailCaps with
  per-field zero-defaults. Reader follows the preview's F-1 basis rule.
  M-083's promotion wired at the ABI success path (the finding's own
  recommended seam): grade != High -> set_grade(High); the node setter
  IS the og's FUN_140089630 semantics (monotonic-max, sticky-2).
- DISCOVERED-DURING-FIX (engine truth defect, fixed root-cause): the
  engine reported truncated=1 for COMPLETE PNGs (stb's post-parse EOF
  probe poisons the disclosure). JPEG is the one lenient family; the
  strict decoders' Ok proves completeness -> out.truncated now gates on
  is_jpeg && hit_eof; pinned by the engine test (the first test version
  was VACUOUS for the grade promotion - existing files grade 2 at insert
  - restructured to the DELETED-PNG shape so the below-2 start is real).
- Tests: 4 ABI tests (TwoCallPattern, TruncationDisclosed,
  UnsupportedFormat, SuccessPromotesGradeTo2 with the deleted-PNG
  fixture + the failed-generation-never-promotes control +
  idempotence) + the ui_abi_check.c C-consumer block. Fail-before:
  the grade promotion patched out -> RED; restored -> GREEN. The ABI
  tests do not compile without the export (M-102's own impact proof).
- Docs: ABI-CONTRACT §7i (the thumbnail contract), CORE-API-CURRENT
  count sites (83 exports), API-DELTA 17b row. Trail: both Resolutions
  recorded, files retired to history with ledger rows (270 resolved).

Stage Summary:
- The thumbnail family is now fully closed at every layer (engine, ABI,
  grading side-effect). Territory remaining: M-079 (auto-save label),
  S-040 (image create). Next parallel pull before starting S-040.

---
Task ID: S10-B (session-10 lane B — the continuation deep audit, RECONCILED AT THE THIRD PUSH with the SA-10 lead close + the truth batches + the fix lanes + the M-102/M-083 thumbnail fix)
Agent: session-10 lane-B parity audit (this session)

THE BASELINE: core 84a1832 (the verification tree) · corpus be0cfe9 · the rda-driver
repo OUT of this lane's two-repo grant (M-086/M-087/M-089/N-127 stand as filed).

THE PUSH-TIME RECONCILIATIONS (three times — the moving-target SOP executed at every
rejection): (1) the four concurrent fix lanes + the SA-10B purge convention + the
parallel session-10 audit lane; (2) the SA-10 lead close (S-052, M-105 DRWService,
M-106, M-107, N-128, the same S-050->S-051 repair) + the truth batches + the M-092
HTML fix + S-041's umbrella closure; (3) the M-102+M-083 thumbnail ABI/grade-promotion
fix + the M-092 ledger conform. RENUMBERED at every push (the collision convention,
earlier remote numbering wins): M-103 -> M-105 -> **M-108**; N-128 -> **N-132**;
OQ-S10-1 -> **OQ-S10-2**. CONVERGENT: the verification sweep (69 verdicts agree —
the unique content preserved in s10-verification-pass-b.md) and the S-050->S-051
collision repair (the remote SA-10 close made the same repair independently).

THIS LANE'S UNIQUE CONTRIBUTIONS (what survives on the merged board):
1. M-100 resolved closed-as-covered — the one board move every parallel lane missed
   (6d0f16c is an ancestor of BOTH of M-100's own audited baselines; FIXED-LEDGER
   row 281; the narrative in s10-verification-pass-b.md).
2. M-108 (EXECUTED) — the text-preview eligibility model, a surface no prior finding
   owned: the og's TxtViewFiles 119-ext extension whitelist (decompile-verified
   first-hand at DRWUI:535858/:535937) vs the core's 16-byte content sniff; the
   compiled probe proves UTF-8-BOM/UTF-16/Latin-1 text -> kind=unsupported and
   printable-head binary -> text+garbage (disjoint accept-sets, both directions).
3. N-132/N-129 — the gate-doc truth pair: API-DELTA rows 44-45's false "landed"
   claims (TxtViewFiles/SumaryFileType/PriorityDisplayType) + six register count
   defects from the 1470-file re-count.
4. N-130 + OQ-S10-2 — the og-tree completeness sweep's two genuinely undispositioned
   files (the Fixo VideoFileRepairer.exe engine, registered SIDECAR-OOS; the opaque
   sfc payload, tracked-open with the Ghidra magic-search queued).
5. N-131 + E69 + the field-parity table — the first per-field enumeration of the
   og's persisted node schema vs cr_records.cpp (every load-bearing family
   MATCH-with-re-encoding; 13 SUPERSET; the parent-anchor ABSENT class filed; the
   ReFS FLAGS corpus contradiction errata'd with the population-site read).
6. The executed .rsf ground-truth (the three real SaveScan files parsed — the corpus
   container grammar byte-confirmed, no erratum needed) + the historical records
   preserved in the lane-B evidence (M-071 archaeology, the M-050 wrapper-scalar
   enumeration, the retired-addenda content: S-041's 8-children remainder precision
   after the M-092 landing, N-101/N-106/N-120's dispositions).

COVERAGE STATISTICS (this lane): 75 findings re-verified at 84a1832 (5 agents; the
load-bearing verdicts lead-re-verified) · 5 new findings + 1 erratum + 1 OQ filed ·
1 board move (M-100) + 1 convergent collision repair (S-050->S-051) · 6 evidence
files (2 executed proofs, 2 decompile/grep records, 2 verification/parity tables) ·
the 1470-file register sweep · the 3-artifact field-parity table.

THE READ-ONLY GUARANTEE (verified at every push): this lane's commits touch NOTHING
outside diff-to-fix/ + worklog* — the probes (s10-preview-probe.cpp, rsf-probe.py)
and their binaries live OUTSIDE the repo (/home/z/my-project/scripts/); the preview
probe compiled the repo's own sources against an in-memory harness without writing
them. The parallel lanes' prompt file restored untouched (this lane's saved as
session-10-lane-B-parity-audit-prompt.md).

RECOMMENDED FIRST FIXES (lane-B contributions to the standing order):
1. M-108 — the BOM/UTF-16 head-gate fix is one branch; the whitelist landing is the
   gen_filterfolder-precedent data file. User-visible on the most common Windows
   text shapes.
2. N-129 + N-132 — ONE mechanical gate-doc pass (the six register counts, the
   API-DELTA row corrections, the N-130/OQ-S10-2 registration rows).
3. N-131 + E69's serializer trace — one session closes the session-schema story.
4. The standing order's head unchanged: S-051/S-052 (the scan-target/LDM SEVEREs),
   M-104..M-107, the 8 remaining carve children; the M-083/M-102 files await the
   trail-lane retirement (the code landed at e56baac).

HANDOFF: next IDs S-053+, M-109+, N-133+, I-031+, A-012+. Board after this push
(the filesystem is the truth): 54 active finding files (6 SEVERE · 24 MAJOR ·
19 MINOR · 5 INFO — the MAJOR set still carries the code-fixed M-083/M-102 pending
their trail retirement) / 281 resolved (FIXED-LEDGER) / 4 tracked-open (OQ-S4-3 —
one Windows dispatch from closing; OQ-S8-1 — the MOM interface-chain trace;
OQ-S10-1 — the codec-bound thumbnail deps; OQ-S10-2 — the sfc probe). The lane-B
scratch (the probe sources + the agents' context sheets) lives outside the repo at
/home/z/my-project/scripts/ and /home/z/my-project/audit-work/; the load-bearing
content is preserved in the six s10-* evidence files.

---

Task ID: S10-C (session-10 continuation lane — the close record; the SECOND-CONVERSATION-SESSION mandate executed in three waves)
Agent: session-10 continuation audit (this lane; complementary to SA-10/SA-10B/S10-F/DFX-38 and the fix lanes per the push-time SOP)

THE MANDATE (the user's continuation directive): look for what session 1
missed; REMOVE THE FIXED THINGS; go deeper on og-codebase ↔ core; mark
all remaining features/implementations/functions.

WAVE 1 — the board-truth sweep (T2-T4): four parallel agents re-verified
all 76 actives at HEAD 84a1832 first-hand (every core-reference re-read;
the lead spot-verified the load-bearing verdicts). 74 of 75 in-scope
STILL-ACTIVE; N-124 FIXED (moved with the resolution record); M-100
re-scoped (its "grep: absent" core-ref was FALSE AT FILING — 6d0f16c/M-066
landed the engine half pre-filing; the residual is the ABI/CLI exposure).
Five addenda filed with new defects the sweep caught: M-079 (a NEW false
cr_scan.h:236-239 comment), N-094 (the triple verb-count drift), N-102
(the third 37-toggles instance at cr_carve.h:88 + the finding's own
citation defect), N-099 (E51's wrong MDB ctor cite), M-100 (R-37 doubly
stale). Evidence: evidence/s10-reverification-sweep.md (the full verdict
table + the line re-pins + the push-time reconciliation note).

WAVE 2 — the whole-disk scan-flow deep-dive (the og-codebase comparison
the prior sessions never ran): executed end-to-end on a controlled
two-volume MBR fixture (the builder + probe committed in
evidence/executions/s10-wholedisk-scan-targets.md) vs the og's own three
real .rsf sessions (re-parsed first-hand). Filed S-051 (SEVERE — the
plain disk target cr_ui_open_disk/scan --disk N silently returns rc=0
with ZERO files on healthy partitioned disks; the og's CDoScanPartition
reads the partition list and scans every volume) and M-104 (MAJOR — the
in-table volumes surface only as anonymous "Candidate @ LBA" roots; no
display names, no FS-type markers, known-vs-lost indistinguishable; the
og's per-volume real-session shape unreachable). The record census
resolved the multi-volume persistence contract first-hand: ONE record-2
per session naming the PRIMARY volume (the G session's 147B record-2 =
"Local Disk (G:)", the artifact layout confirmed byte-exact); the
multi-volume registry is record-1's special-partition list (System
Reserved @ LBA 2048 in the G session; entryCount=0 in the single-volume
sessions). Both findings' open questions RESOLVED by the census.

WAVE 3 — the fix-wave landing-parity verification (the second half of
remove-the-fixed: VERIFY the fixed): three parallel agents independently
verified the nine freshest landings (S-044, N-116, M-080, M-085, S-042,
M-090, M-082, M-071, N-117/N-118/N-103) against their findings'
og-decodes + the corpus/decompile + the current tree. EIGHT
PARITY-VERIFIED (the LZW early-change point independently reproduced
against a from-scratch reference decode; the koly walk re-derived
term-for-term from FUN_180053de0; the %03d re-checked against the .mo
binary at offset 0x7bf7c; the junk gates byte-exact to 69507cb; the
full suite 1005/1005 at HEAD in an independent run). TWO residuals
filed: M-108 (the M-080 landing stopped at 32 MiB — the og's 200 MB
anchor not adopted, the 32-200 MB band refused, the pinning test still
citing the refuted no-cap doctrine) and N-129 (the bookkeeping truth
batch: N-103's cited :376 site missed while its Resolution claims both
fixed; the gap-board's :379/:225 self-contradiction; five phantom
pre-rebase SHAs; the FIXED-LEDGER's own gaps). Evidence:
evidence/s10-landing-parity-verification.md.

ALSO THIS LANE: the Dict byte-compare (228 entries, ORDERED MATCH vs
the og's file — the N-064 landing verified); the diskhealth/USB-bridge
territory checked and found already dispositioned (N-032 + PARITY-CORE-
DOM D-78 — no duplicate filed); the ID-collision renumbers executed at
push time (this lane's S-050→S-051 by the SA-10 rule; M-105→M-108 and
N-128→N-129 by the same rule — the SA-10/SA-10B lanes' same-numbered
findings pushed first and keep the IDs).

BOARD AT THIS LANE'S CLOSE (the filesystem is the truth): 56 active
(6 SEVERE · 25 MAJOR · 20 MINOR · 5 INFO) / 281 FIXED-LEDGER rows /
4 tracked-open. This lane's contributions to the active board: S-051,
M-104, M-108, N-129 (4 filings) + the M-100 re-scope + 5 addenda. The
concurrent lanes (SA-10: S-052/M-105/M-106/M-107/N-128; SA-10B:
S-048/S-049/M-103 + the FIXED purge; the fix lanes: S-042/S-043/S-044/
M-071/M-080/M-082/M-085/M-090/M-092/N-094/N-116/M-102+M-083/N-091/
N-098/N-105/N-103/N-117/N-118 all landed and verified) own the rest of
the movement.

READ-ONLY GUARANTEE: this lane's commits touched NOTHING outside
diff-to-fix/ + worklog* (verified per-commit; every product-file change
visible in the final tree entered through the MERGE with origin/main —
the parallel fix lanes' landed code).

Handoff: the highest-value next fixes from THIS lane's filings:
(1) S-051 + M-104 as one whole-disk shape pass (the target builder + the
per-volume display-name roots; composes with S-048/M-103/M-079 — the
fixtures and the real-session census are in the evidence file);
(2) M-108 (one constant + one test-doctrine re-pin + a D-row — small);
(3) N-129's mechanical sweep (the :376/:379 rows, the phantom SHAs, the
ledger rows) + the optional git-cat-file guard script;
(4) S-052 (the SA-10 lane's LDM finding) pairs naturally with the
whole-disk pass — the LDM volumes ride the same candidate machinery.
## Fix session — M-093 landed (the XML/SVG chain) + the two-case scan decode

- **M-093 FIXED** (`d1dc51a`) — both open questions discharged first-hand
  from the og binary's .data (PE section walk + hexdump): DAT_180093968 =
  "csl" (the Corel marker -> generic XML id 0x13); the numeric key 0x1cd =
  a raw dword magic CD 01 00 00 (direct SVG). The full six-needle svg set
  hexdump-decoded. The probe is case-SENSITIVE (vs HTML's _memicmp); the
  id split is content-decided — landed via CarveResult::format_override
  (the emit site resolves the effective row). Four rows (XML/XML_BOM/
  SVG_CD01/SVG); bounds {0, 1 MiB}; the </svg> walk without sector
  round-up; the 0x44c CSL-size branch recorded as the residual stand-in.
- **Decode discovery** — the og's shared end-marker scan (FUN_180054470)
  is two-variant case-aware (_mbslwr_s then _mbsupr_s: all-lower +
  all-upper, never mixed). scan_tag_two_case landed; the M-092 HTML
  scanner refined to the og-exact semantics in the same change.
- Tests: 5 new Carve.* (Svg*/GenericXml*/XmlNo*); suite 1081/1081.
---
---
N-100 FIXED (the NTFS +0x60 total-clusters persistence): re-verified at current main first
(bit-8-only persistence at cr_records.cpp:172/:268/:460; both stamp sites origin-only;
the "derivable" rationale refuted by the sparse rule). The fix (option 1):
FileProperties::ntfs_total_clusters with the sparse-anywhere zeroing (FUN_1800336f0
:41096-98) stamped at BOTH walk sites + the GUID-dump twin; nflags bit-9 4-byte block
(fires only when nonzero - pre-fix sessions shape-identical; the og's rsf record-4
persists the pair in the same order); the hard-link clone carries it. The addendum's
comment repair landed too (the resident prose now states the decoded truth - the og's
resident wrapper stamps the synthetic record-anchor pair; core's 0 is the honest
deliberate no-runs value). Tests: Session.NtfsTotalClustersRoundTrip (the
not-derivable pair - sparse total 0 beside nonzero allocated_size - round-trips; the
zero-total gate keeps old records shape-identical) + the walk-test's total half
(readme 2 / deleted 4 / holes 0 / dirdel 0 / ads 1). Compile-RED on the reverted tree
(field-addition precedent). Suite 1077/1077; format clean.

N-100 conform: ledger row + retirement + SUMMARY board (46 active / 280 resolved).
Session-38 tally: 22 findings closed.

N-129-flagged residuals from this session's own landings, verified and repaired (the
honest response to the session-10 close-out's bookkeeping flags): (1) the R-28 row at
REFERENCE-API-GAP:376 - my N-103 edit fixed only the :222 site while the Resolution
claimed both; the :376 row now carries the engine-PRESENT state + the M-102 handoff
(the N-129 flag was CORRECT); (2) the phantom pre-rebase SHAs in my FIXED-LEDGER rows
(954c386/e1e5124/13245c0 - all rewritten by the push-window rebases) - every cell now
cites the real post-rebase SHAs (e4f3184 batch A / 995ba5d batch B / 32ef33b batch C /
859e286 batch D / faa547f N-100).

---
Task ID: S10-F/T3-T8 (session-10, the FUNCTION-LEVEL COMPLETENESS lane — the reading/verification waves and the filings)
Agent: session-10 function-level completeness audit (lead lane S10-F)

Work Log:
- T2 (the inventories): the nine ghidra-searchdll function lists parsed
  (scripts persisted under audit-ctx scratch; the parse/disposition/domain
  filter scripts saved to /home/z/my-project/scripts/s10_*.py) — 9,618
  functions / 3,088 named / 3,088→122 domain after CRT/STL/EH filtering;
  the class-coverage map built (vftable labels): 191 classes cited by name,
  49 never (type_info/ATL noise + the engine candidates).
- T3 (the class pass): two read-agents (T3a NTFS/FAT/exFAT, T3b
  Btrfs/EXT4/HFS+) verified all 25 never-cited classes — vtables re-derived
  from the og binaries via the trail's own RTTI walker, core equivalents
  cited, trail dispositions cited. ALL COVERED except two behavioral gaps
  (the @eaDir suppression; the volume-label dead decode) — both verified
  first-hand by the lead before filing (the FUN_180001cd0 control flow
  re-read; the FUN_18003f3a0 $VOLUME_NAME chain re-read).
- T4 (the tree census): the read-agent enumerated all 103 decompiled
  binaries across the nine ghidra trees; the lead spot-verified every
  strong candidate's RTTI claims and the zero-disposition greps. Six
  substantive never-dispositioned binaries + the telemetry name-absent set.
- T5-T8 (the filings): M-110 (MAJOR, @eaDir), N-133 (MINOR, volume labels)
  + the M-079 addendum (the premise correction), N-134 (MINOR, the register
  completion); evidence files s10f-btrfs-eadir-suppression.md /
  s10f-engine-volume-labels.md / s10f-product-frame-register.md; module
  rows appended (btrfs/ext4/mom-capabilities); CROSS-REF-MATRIX's S10-F
  block; SURFACE-INVENTORY §11; SUMMARY's S10-F delta; README's lane block.
- Push-time reconciliations (two): the SA-10/DFX-38 baseline collision at
  open (lane split declared in the T1 worklog entry); the post-wave remote
  pull (the parallel lanes' 30-commit sprint: SA-10's triage close at 54
  active, the fix lanes' M-102/M-083/M-092/S-043 landings, lane-B's
  M-108/N-129..N-132/OQ-S10-1/2, the landing-parity lane's M-105/M-108) —
  absorbed by rebase; my finding IDs re-coordinated (M-106→M-110 after the
  lanes took M-106/M-107/M-108; N-128→N-133 after N-128..N-132 were taken);
  the M-108 filename collision found on the pulled board and repaired per
  the SA-9 rule (the later-pushed M-080-residual renumbered M-109).
- Core-side cites re-verified at the CURRENT tree (44f043a5) after the
  pull: cr_ext4_engine.cpp:200-201 unchanged; fill_identity_card now at
  cr_ui.cpp:1850-1866 (label still absent from the fill set); the @eaDir
  grep still zero across src/include/tests.

Stage Summary:
- Three findings filed (M-110, N-133, N-134) + one addendum + one process
  repair; the class/tree census verdicts recorded as MATCH rows (the
  engines' class surface and the engine-bearing trees are CLEAN — the
  trail's coverage claims survive the function-level check).
- Next: the wave-2 candidates (RAWSearchDLL infrastructure function-level
  walk; the OQ-S8-1 consumer trace if time permits) or close-out.

## Fix session — M-096 landed (MOBI)

- **M-096 FIXED** (`7ca83f5`) — the full-salvage BOOKMOBI gate at +0x3C
  (nothing before it validated — ported exactly), the 0A 0D 8E E9 end-tag
  walk at 0x200 steps (the open question's byte order discharged), cap
  500 MB; the shipped RawConfig posture verified (MOBI=1 — default ON).
  Tests: 3 Carve.Mobi* (incl. the non-step-aligned-tag fallback pin);
  suite 1088/1088.

---
## Session-11 open (2026-10-28 17:49 UTC — the user's session-2 continuation: remove-the-fixed + go-deeper + mark-all-remaining)

Baseline: **core `7990a86d`** (pulled at open: the mid-session M-096 landing absorbed —
MOBI carve fixed `6e37dc21` + conformed `7990a86d`) · **corpus `be0cfe9`**. Prompt saved
redacted at `worklog/session-11-diff-audit-prompt.md`, read twice. Board at open:
**55 active** (6 SEVERE · 23 MAJOR · 21 MINOR · 5 INFO) / ~282 FIXED-LEDGER rows /
4 tracked-open (OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2).

Session decomposition (the user's three asks):
1. **Remove-the-fixed** — first-hand verification of the freshest landings + board-truth
   repairs. OPEN FLAGS already identified at T1: (a) M-102+M-083 fixed in `e56baac7`
   with Resolution sections + ledger rows appended, but the finding files were NEVER
   `git rm`'d (the purge convention's step 3 missed — both still sit active in the
   tree while the ledger claims them resolved); their ledger cells read "(this
   landing)" instead of the real SHA. (b) An ID collision on the board: TWO files
   carry N-129 (`N-129-fix-wave-bookkeeping-truth-batch.md` from the wave-3 lane,
   pushed first; `N-129-parity-og-core-inventory-numeric-defects.md` from lane-B,
   which its own SUMMARY delta says should have been renumbered — the repair was never
   executed).
2. **Go more deep** — the S10-F named next candidates (RAWSearchDLL infrastructure
   function-level walk; the OQ-S8-1 consumer trace) + the lanes S10-F did not reach:
   the og's config/data assets (FileFilter.xml / DataFile.ini / raw_signs.json / Dict /
   EUUserData / remote_config_data), the never-cited function space of the seven
   non-searchdll ghidra trees (drwui / core / misc / local / wave2 / mo / partition /
   drivers / repair), and the core-side stand-in/follow-up comment register.
3. **Mark all remaining** — every divergence found gets a finding file per the
   template; extend-don't-refile against the 55 actives; the SA-9 ID-collision rule on
   every push.

---

## Session 11B — the continuation audit, lane B (2026-10-29 — the second session-11 lane, differentiated per the no-duplication convention)

Task ID: S11B/T1 (session-11 lane B — setup + state assessment; the lane at ac39c321 opened first and owns the declared scope)
Agent: continuation audit lane (the user's "second session" instruction re-applied at the current board)

Baselines recorded at open:
- **Core audited commit: `7990a86d`** (main; the S10-F close + the M-096 fix-conform
  lineage). 1212 commits total on main.
- **Corpus: `be0cfe9`** (unmoved since the SA-10B session — verified `git rev-parse HEAD`).
- Board at open (filesystem is the truth): **55 active finding files** — 6 SEVERE ·
  23 MAJOR · 21 MINOR (22 IDs: the N-129 collision) · 5 INFO; FIXED-LEDGER ~285
  data rows (the 281/282 count claims conflict — see T5); 4 tracked-open
  (OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2).

State assessment findings at open (board hygiene — to repair this session):
1. **Committed merge conflict in `diff-to-fix/FIXED-LEDGER.md`** (lines 22–32,
   the 282-vs-281 counts stack) — introduced by `5fecdae1` ("repair(trail)",
   ironic: the repair commit left the conflict markers in the pushed tree);
   the `64a3b18c` sweep convention (both sides kept as dated history, one
   truth line on top) not applied to this second occurrence.
2. **N-129 ID collision**: two files on main —
   `N-129-parity-og-core-inventory-numeric-defects.md` (lane-B, first pushed
   `0e279e43` 17:32:28Z) and `N-129-fix-wave-bookkeeping-truth-batch.md`
   (wave-3, first pushed `f6c00177` 17:33:36Z). Per the SA-9 rule (earlier
   remote push wins): lane-B keeps N-129; the wave-3 bookkeeping file
   renumbers to **N-135** (next free after N-134).
3. **Ledger count drift**: the table carries ~285 data rows (264 strict
   `[A-Z]-[0-9]` pattern rows + 21 OQ-prefixed rows) while the top line
   claims 281/282 — the N-129(lane-B) numeric-defects class extends to the
   ledger's own top line; to be reconciled at T5 with the row-count truth.

Session plan (S11): T2/T3 re-triage of all 55 active findings at HEAD;
T4 recent-landings side-effect analysis (M-093/M-096/M-079/N-100 fixes vs
the open children); T5 the three process repairs; T6–T9 the deep-dive waves
(RAWSearchDLL infrastructure walk; the six never-dispositioned product-frame
binaries; the OQ consumer traces; the corpus-surface gap-hunt); T10 filings;
T11 gate docs; T12 close.

READ-ONLY guarantee: this session's commits touch NOTHING outside
`diff-to-fix/` + `worklog*` (verified per-commit at close).
### N-136's Resolution record (renumbered from the N-128 ID collision; session-10, this lane — the file retired per the purge convention)

**Status:** FIXED · **Fixed in commit:** this rebased landing (745c042-lineage) · **Verified by:** the C gate's default-path invocation (FAIL sl_found pre-fix at the 84a1832 baseline worktree; OK post-fix) + the new dot-listing pin + the absolute-path class still OK + cr_ui_tests 58/58 + cr_tests 1026/1026. **Notes:** both list_crsf_files branches (POSIX + Windows) compose BARE names when the caller's dir is exactly "." — a caller who saved "name.crsf" (CWD-relative) now string-matches its own row; every other dir keeps the existing separator-convention join (run 40689196858). Discovered during the M-102 verification (the gate's default argv); predates this session (reproduced at 84a1832); CI never exercised the class (ctest passes an absolute path).

Task ID: S11B/T5 (the board-truth repair wave — executed, reconciled with the parallel fix lane's mid-flight landing)
Agent: session-11 lane B

Work Log:
- The three repairs landed (bc4513db + the rebase over the parallel fix lane's
  740d9b56/fce38c3d/715300ba):
  1. **M-102/M-083 retired** — the e56baac7 landing's missed purge step. The
     parallel fix lane deleted the files directly from findings/MAJOR/ in their
     fce38c3d reconciliation; my two-phase FIXED/ move was dropped at the rebase
     resolution (the DU conflicts resolved in their favor — the content with
     Resolution sections is recoverable at the MAJOR path, verified
     `git show fce38c3d^:...`). The ledger rows carry the real SHA e56baac7 +
     the truthful history path (the MAJOR-path deletion).
  2. **The N-129 collision resolved the ancestry-decisive way** — wave-3's
     bookkeeping file renumbered **N-129 → N-135** (0e279e43 pushed lane-B's
     inventory-defects N-129 at 17:32:28Z; f6c00177 — the bookkeeping file's
     first appearance — is its git DESCENDANT at 17:33:36Z; the SA-9 rule).
     Renumber note in-file; matrix/SUMMARY/evidence refs updated. NOTE: the
     parallel session-11 lane's worklog entry proposed the OPPOSITE resolution
     ("wave-3's file pushed first") — refuted by the object graph; the
     ancestry proof is recorded here and in the renumber note.
  3. **The FIXED-LEDGER conflict swept** (5fecdae1's unswept second occurrence,
     lines 22-32) per the 64a3b18 convention + **the count truth established by
     whole-table re-count: 285 rows** (265 main-pattern + 20 OQ-prefixed) — the
     prior 276-282 top-line figures were push-window estimates that never
     re-counted the table.
- Convergence with the parallel fix lane: their N-128 (the session-list CWD
  bare-path composition bug, DISCOVERED during their thumbnail verification)
  renumbered to N-136 in their own 715300ba repair (the third N-128 claimant
  conformed — the capability-matrix N-128 keeps the ID). My lane absorbed the
  renumber; no further action needed.
- Board after the wave: **53 active (6 SEVERE · 21 MAJOR · 21 MINOR · 5 INFO)**
  / 285+2 resolved rows (M-102/M-083 already counted; N-136 appended by the
  parallel lane) / 4 tracked-open. Next free IDs: S-053+, M-111+, N-137+,
  I-031+, A-012+.
---
---
N-093 FIXED (the batch-1 bound pairs): re-verified at current main first (MID {14, 64 MiB},
GZ {18, 512 MiB}, PSP {32, 128 MiB}, SIT {16, 512 MiB} - all supersets vs the og's
registered caps). The og's caps carried for all four (MID 10,000,000; GZ 20/400,000,000 -
the min also corrected; PSP 36/20,000,000; SIT 20,480/200,000,000) with the ctor cites in
the row comments; plant_sit grew to the og's own 20 KiB floor. RED proven on the pre-fix
registry: the pin test failed on all four pairs AND the 10,000,001-byte MIDI CARVED under
the 64 MiB row (the finding's exact falsifiable, reproduced behaviorally); GREEN after.
Suite 1090/1090; format clean. (PSP/SIT behavioral at/over-max tests are pin-level only -
20/200 MB fixtures are not unit-test scale; the 10 MB MID pair proves the gate mechanics.)

### M-072's Resolution record (session-10, this lane — retired per the purge convention)

**Status:** FIXED · **Commit:** bf17e55 · **Verified by:** Query.M072PathSubstringAndStateMask (all buckets + the sugar-equivalence pin + the zero-mask default), suite 1030/1030 + cr_ui_tests 59/59. **Notes:** path_substring (contains over the materialized full path, active-axis-only materialization incl. the root segment) + state_mask (DeletedState bits, deleted_only kept as sugar) + the ABI append (cr_ui_query path_substring/state_mask, zero-init defaults). The og's own state-bucket values remain undecoded — the four-bit mapping is the registered interpretation.

---
## Session 11C — the diff-fix lane (2026-10-29 — the user's fix mandate: verify every finding, fix, prove, mark)

Task ID: S11C/T1 (setup + baselines)
Agent: session-11 fix lane (the diff-to-fix execution session)

Baselines recorded at open:
- **Core at open: `35bee726`** (main; session-11/T1 setup commit — the continuation
  lane's open flags recorded there are this lane's first work items) · **corpus
  `be0cfe9`** (unmoved, verified).
- **Build: Linux gcc 14.2 Release + Ninja; full ctest 1090/1090 GREEN at the
  baseline tree** (cr_tests 1029: 987 pass / 42 skip / 0 fail; cr_ui_tests 59/59;
  cr_ui_abi_check green via its ctest-registered absolute-path invocation;
  win_syntax_check green). Prompt saved redacted at
  `worklog/session-11-diff-fix-prompt.md`.
- Board at open (filesystem truth): **55 active files** (6 SEVERE · 24 MAJOR ·
  21 MINOR with the N-129 ID collision · 5 INFO) · FIXED-LEDGER ~285 data rows
  with a COMMITTED MERGE CONFLICT (lines 22-32) and a 281/282-vs-~285 count drift ·
  4 tracked-open.

Session plan (per the fix mandate's SOP order — no ACUTE active, so SEVERE first):
0. Board hygiene repairs (the S11B/T1 flags): the FIXED-LEDGER conflict + count
   truth; the N-129 collision (wave-3's file renumbers to N-135 per the
   earlier-push-wins rule); the M-102/M-083 un-retired files (resolutions
   landed in e56baac7 — the retirement conform + the "(this landing)" ledger
   cells get the real SHA).
1. SEVERE: S-040, S-048, S-049, S-050, S-051, S-052 — verify → classify → fix →
   test → push, one finding per push.
2. MAJOR: M-072..M-078, M-086, M-087, M-089, M-098..M-101, M-103..M-110.
3. MINOR: N-090..N-134 (the N-135 renumber included).
4. INFO: I-026..I-030 — acknowledge with rationale verification.
5. TRACKED-OPEN: OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2 — resolve or push forward.
6. Gate docs (CROSS-REF-MATRIX, SUMMARY, modules/*, PARITY-DELTAS) kept current
   with every landing.

---
## Session-11D fix lane (2026-10-29 open — the diff-TO-FIX execution lane D; scope differentiated from session-11C's pushed open)

Baseline at open: pulled through **`b1473173`** — this lane opened against `35bee726`, then
absorbed at rebase: the parallel fix lane's **thumbnail ABI landing** (`740d9b56`, M-102+M-083
FIXED-ALREADY — independently verified in-source by this lane pre-pull: both exports at
cr_ui.h:1278/:1285 + implementations, the grade promotion at cr_ui.cpp:3741-3742, all four named
tests present; the N-128→N-136 renumber), lane-B S11B's **three board-truth repairs**
(`bc4513db` — the N-135 renumber, the FIXED-LEDGER conflict sweep, the 285-row count truth),
and session-11C's pre-open landings: **M-072 FIXED** (`f6fd86fb` — the path_substring +
state_mask query axes; to be re-verified at this lane's next suite run), **N-093 FIXED**
(`03ad5523` — the batch-1 bound pairs; same), and the **driver-grant disposition** (`0a08cb3c`
— M-086/M-087/M-089's fix surfaces live in heicfast/rda-driver, out of this session's
two-repo grant; tracked forward under OQ-S10-2 with N-107/N-127's driver halves riding).
**Corpus `be0cfe9`** (unmoved). Prompt saved redacted at `worklog/session-diff-fix-prompt.md`,
read twice. **Baseline suite executed at `743d7b8d` (pre-pull tree): 1090/1090 green.**

**THIS LANE'S CLAIMED SCOPE (collision avoidance vs session-11C): the SEVERE tier, in this
order — S-051 (wholedisk scan targets) → S-049 (carved-naming contract) → S-048 (Reconstructed
per-volume grouping) → S-052 (LDM recovery basis) → S-050 (repair-line registration decision)
→ S-040 (image-create producer).** Each finding independently verified against the corpus and
the running code before any change; every fix paired with fail-before/pass-after tests; one
finding, one verified fix, one push. Post-SEVERE, this lane takes the MAJOR tier's
orchestration/carve-cluster findings (M-103, M-104, M-108, M-109, M-110) unless 11C lands them
first. Pull before every push; the SA-9 collision rule governs any ID or landing conflicts.

---
## Session-11D wave 1 (2026-10-29 — A-012: the carve-registry row restore, DISCOVERED during S-051's verification)

Task ID: S11D/W1 (the diff-TO-FIX lane; part 1 of the S-051 landing)
Agent: session-11D fix lane

- **A-012 (ACUTE, DISCOVERED)**: while running the S-051 evidence probe
  post-fix, the carve stage returned ERR_BAD_PARAMETER (0x50030003 — the
  evidence file's own "ERR_OPEN_FAILED" annotation is wrong, corrected in
  S-051's Resolution). Traced to `carve_slice`'s F8 gate:
  `validate_registry(nullptr) == 2` — **the 81ee9775 rebase-window conflict
  sweep dropped M-093's FMT_SVG row** (kTable 119 vs FMT_COUNT 120; the mobi
  row displaced to index 118/id 119). The two conflict sides were TWO
  DIFFERENT rows (HEAD: the MOBI entry; other: the one-line FMT_SVG entry);
  the sweep kept one, dropped one as if duplicates.
- **Fail-before EXECUTED**: `CarveEngine.RegistryF8Gate` (test_carve.cpp:516,
  the pre-existing pin) RED at `bcf31316` — the tree stashed, rebuilt, the
  single test run: "1 FAILED TEST"; `/tmp/regcheck`: `rejects=2 of 119`,
  the mobi entry reported BAD. **Main was RED at open** — the parallel
  lanes' push window shipped a hard-down carve stage.
- **Fix**: the FMT_SVG row restored verbatim from M-093's landing
  (`git show ca701711`), with the in-table provenance comment. The parallel
  lanes' 9 residual clang-format violations (cr_signatures.h,
  cr_analysers.cpp, test_carve.cpp, test_thumbnail.cpp — under the
  CI-pinned 18.1.8) swept in the same commit.
- **Pass-after**: `RegistryF8Gate` GREEN; registry 120/120; the full suite
  1105/1105 with the S-051 landing below; format gate 0 violations.
## Session 39 — the diff-to-fix parity-fix session (2026-10-29; opens at core `35bee726`, corpus `be0cfe9`; prompt saved to worklog/session-diff-fix-39-prompt.md)

**Mandate (the user's operative line):** "Fix all the audit findings (priority
to minor first) one by one, with full context and knowledge and update and put
fixed findings in fixed directory. Update all docs, counts and all properly."
Fix order this session: **MINOR → INFORMATIONAL → MAJOR → SEVERE →
TRACKED-OPEN** (the user's explicit "minor first" overrides the template §4
order; no ACUTE findings are on the board).

**T1 (baselines + prompt):** core `35bee726` (pulled one commit over the
session-11 audit lane's `ac39c321`), corpus `be0cfe9`. Build green
(linux-release, cmake 4.4.4 / g++ 14.2 via pip-installed toolchain); suite
**1029 tests / 987 passed / 85 skipped (Win-only) / 0 failed**. Prompt saved
redacted. Convention recorded: fixed findings move to
`diff-to-fix/findings/FIXED/` (the "fixed directory") with Resolution sections
in-file AND take a FIXED-LEDGER row — the ledger stays the master index.

**T1c/T1e (hygiene repairs, commit `0c3fd6d2`):** the committed FIXED-LEDGER
merge conflict (5fecdae1 residue, lines 22-32) resolved per the 64a3b18
convention — both sides kept as dated history, one truth line on top. The
count truth: **284 table rows = 264 resolved findings + 20 resolved OQ
entries** (the 281/282 claims counted with looser inclusion rules; grep
commands recorded in the ledger's truth line).

**T1d (hygiene repair, the renumber commit):** the N-129 ID collision
resolved — wave-3's fix-wave-bookkeeping-truth-batch renumbers to **N-135**
(lane-B keeps N-129 per the SA-9 rule; the S-050→S-051 precedent shape:
file renamed with history, renumber note in-file, SUMMARY + the wave-3
evidence file annotated). Next free IDs: S-053+, M-111+, N-136+, I-031+,
A-012+.

**Board at open (55 active + 4 tracked-open):** SEVERE S-040/S-048/S-049/
S-050/S-051/S-052 · MAJOR M-072..M-078/M-083/M-086/M-087/M-089/M-098/M-099/
M-101..M-110 · MINOR N-090/N-093/N-096/N-107/N-111/N-112/N-113/N-115/N-121/
N-122/N-125..N-134/N-135 · INFO I-026..I-030 · OQ-S4-3/OQ-S8-1/OQ-S10-1/
OQ-S10-2.

**Session-39 T1 addendum (the push-window rebase, recorded per the
coordination contract):** while the T1 hygiene repairs were being prepared,
the remote moved +3 (the parallel fix lane's N-093 landing + its
cr_registry.cpp marker sweep; the M-072 landing + retirement; the
driver-lane's M-086/M-087/M-089 → tracked-open OQ-S10-2 disposition — all
three findings' fix surfaces live in heicfast/rda-driver, outside this
session's core+corpus grant, so that disposition is accepted verbatim).
The rebase over them surfaced two convergences: session-11B had ALSO
repaired the FIXED-LEDGER counts conflict and ALSO renumbered the wave-3
N-129 → N-135 (their push wins; session-39's versions dropped to history
snapshots + the b5da834-provenance NOTE addendum kept in the N-135 file);
and origin/main itself carried a SECOND committed-marker occurrence (the
N-136/N-093 ledger-row collision at the table tail) — repaired here, both
rows kept. The ledger truth line re-counted post-rebase: **287 rows = 267
main-pattern + 20 OQ, no duplicate IDs**. M-072/M-102/M-083/N-093/N-136 are
off the active board via the parallel lanes — the session-39 board is
**50 active (6 SEVERE · 20 MAJOR · 19 MINOR · 5 INFO) + 5 tracked-open**
(OQ-S4-3, OQ-S8-1, OQ-S10-1, OQ-S10-2 now carrying M-086/M-087/M-089 +
N-107/N-127's driver halves). The MINOR lane starts next per the mandate.

**Session-39 territory claim (coordination contract, per this session's
user mandate "priority to minor first"):** session-39 takes the **MINOR tier
(N-090 .. N-135) and the INFORMATIONAL tier (I-026..I-030)**, then
tracked-open items not claimed elsewhere, and the board-wide gate-doc truth
sweep (CROSS-REF-MATRIX / SUMMARY / modules/* / counts) at close. The
SEVERE tier belongs to session-11D (their claim pushed first); the MAJOR
orchestration/carve cluster likewise. Session-39 will NOT touch
S-040/S-048..S-052 or the 11D-claimed MAJORs; if a MINOR finding's fix
collides with a parallel lane's territory (e.g. a driver-side MINOR), it is
left to that lane and noted here. Pull-before-push discipline active.

### M-074's Resolution record (session-10, this lane — retired per the purge convention)

**Status:** FIXED · **Commit:** 09b311d · **Verified by:** WinDestIo.M074DestinationCodesDifferentiated (Windows lane; RED pre-fix — the codes did not exist) + the Linux family/text compile (1033/1033). **Notes:** ERR_DEST_PATH_TOO_LONG/PATH_INVALID/NETWORK (0x50050006..8, append-only) + err_text rows in the og's wording + the shared classify_dest_error seam in create_temp + mkdir; the mapped-drive dynamic half is the documented limitation.

### The F8 gate restore (session-10, this lane)

The parallel M-093 lane's mid-flight push (enum + trial order without the kTable row) made every carve_slice call fail the runtime registry-size gate while the build stayed green (the kTrialOrder static_assert counts the trial array, not the table) — ParallelCarve.MatchesSingleThreaded red at 1342373891. The missing FMT_SVG row landed in FormatId order; suite 1033/1033 green; the green-restore commit cites the root cause (the table/count asymmetry between the two build-time gates — a static_assert on kTable size vs FMT_COUNT would have caught this at build time; noted for the carve lane as a hardening candidate).

---

## REBASE RECONCILIATION NOTE (session-11-lead, replaying a0a48172 onto 2cf3f4d0)

The wave-1 commit a0a48172 was authored against the pre-push window and replayed over
the concurrent lanes' landings (session-11B/11C/11D + session-39/39B). Convergent
repairs resolved per the earlier-push-wins rule: (1) the N-129 collision — the S11-lead
draft renumbered lane-B's register-counts file, but session-11B's timestamp check
(lane-B's file added 0e279e43 17:32:28Z, 68s before wave-3's f6c00177 17:33:36Z) proves
lane-B pushed FIRST — lane-B's file keeps N-129, wave-3's bookkeeping file is N-135
(the S11-lead rebase accepts the remote resolution and corrects its own evidence file);
(2) the M-102/M-083 retirement — session-11B's fix lane deleted the files at fce38c3d
mid-window; the S11-lead's independent PARITY-VERIFICATION (the engine layer executed
standalone, 40/40 checks, zero ASan) rides through as the verification record
(evidence/s11-board-truth.md) with the ledger cells annotated; the pre-retirement
addenda the S11-lead appended to the two files did not survive the rebase window (the
files were already deleted on the remote side) — the verification content lives in the
evidence file and the ledger notes; (3) the phantom-SHA sweep — session-39B's 19-row
sweep stands; the S11-lead adds the four repairs their sweep missed (M-041's cell held
a CORPUS-repo commit; M-088's placeholder + fictional FIXED/ path; the M-083/M-102
verification annotations; the 54-row as-recorded-in-file census erratum); (4) the S11-lead
draft's N-136 filing renumbers to **N-137** — the remote's N-136 (the session-list
cwd-dir finding renumbered from N-128) pushed first and keeps the ID.


---
Task ID: S11/T1-T5 (session-11 wave 1 — remove-the-fixed + board truth)
Agent: session-11 continuation lane (lead)

Work Log:
- T1: baselines recorded (core `7990a86d` pulled at open absorbing the M-096 landing; corpus `be0cfe9`); prompt saved redacted; the parallel-lane environment quiet at open (the last push was `7990a86d` itself).
- T2: M-102/M-083 — the landing `e56baac7` had appended Resolutions + ledger rows but never executed purge step 3 (both files sat ACTIVE while the ledger claimed them resolved; the cells read "(this landing)"). Independent verification (parallel agent + lead re-read of every load-bearing cite): **both PARITY-VERIFIED** — the engine layer executed standalone (the shipped TUs compiled -O2/-O0+ASan, the tests' exact fixture bytes, 40/40 checks, zero ASan: the two-call probe matrix, the 0x50030009+bytes_needed refusal with rgba untouched, caps validation refusals, complete-PNG truncated=0 / truncated-PNG DecodeFailed / truncated-JPEG Ok+truncated=1, the full grade-setter semantics); the corpus decodes re-read first-hand (DRWUI :652395-652410 the 256-px GDI+ scale; :422507-513 the !=2→set-2 promotion; :134188-206 the setter's monotonic-max+sticky-2). Both files retired with pre-retirement verification addenda (preserved in history). Residuals: **N-136** filed (cr_ui.h:1272-1274 "ERR_TRUNCATED never returned" vs the DecodeFailed→ERR_TRUNCATED map at cr_ui.cpp:3677-3678 + §7i:607 + the test's own 0x50030004u expectation at test_cr_ui.cpp:3508; and the inert allow_embedded knob — 0=default-ON, nonzero=ON, no off position, the "validated 0/1" header claim unenforced) + **OQ-S11-1** queued (the og's undecoded promotion-guard conditions at :422508-511: local_167/168/160, +0x5c flag, +0x58!=3 — decode attempted in wave 2) + the stale core-side doc rows recorded for the fix lane (REFERENCE-API-GAP:222/:376, PLAN-API-EXPANSION:81, PARITY-DELTAS:3229 — all now false in the "ABI open" direction).
- T3: the N-129 ID collision repaired — lane-B's `N-129-parity-og-core-inventory-numeric-defects.md` renumbered **N-135** per the SA-9 rule (the wave-3 bookkeeping N-129 pushed first); the renumber note added to the file; CROSS-REF-MATRIX:668, README:32, SUMMARY:29 updated.
- T4: the full ledger-vs-tree sweep (parallel agent + lead verification of every repair): the **committed merge-conflict markers** in FIXED-LEDGER (:22-32, from the 5fecdae1 repair itself — the b5da834 side it cites does not exist) RESOLVED (the real-SHA side kept); the **missing M-082 row** added (fix `1f714884`, retire `f8bb4d21`, docs `20ea77d2` — the retire commit claimed a row was appended); **M-088's fictional retired path** corrected (the row cited findings/FIXED/M-088-… which never existed in history — the file was retired from findings/MAJOR/ by the fix commit `41d88297` itself); every recent phantom fix-SHA repaired (M-041 03ad46c→`612abc48` (the cell held a CORPUS commit); M-064 6b50bb8→`dfb2761d` (upstream SHA); M-079 f43b747→`0b76757c`+`b82ea173`; M-084/M-091 16475b8→`8a7055ea`; M-085 (parallel-lane)→`fb7cf30`; M-090 180053de0→`9daa787b`; M-093 d1dc51a→`ca701711`+`608665fb`; M-096 7ca83f5→`6e37dc21`+`7990a86d`; N-116 632d21e→`9445627f`+`48876278`; S-041's 771e2ec/16475b8→`9daa787b`/`8a7055ea`); the **54-row ancient as-recorded-in-file census** (40 decompile addresses + 14 magic constants) recorded as the ledger's S11 erratum (extends N-129 item 4; the mechanical backfill is the fix lane's); the gate docs brought current (README's S11 block; SUMMARY's S11 head block; CROSS-REF-MATRIX's phantom batch-SHA cites — A=`e4f3184`, B=`995ba5d`, C=`32ef33b` — and the M-096/M-100/M-102/M-083/M-092/M-093 stale rows; modules/preview.md's "IF the ABI export ever lands" + "the registered Agent 1 handoff" rows flipped; modules/carve.md's M-090 phantom).
- T5: the full active-board re-verification at the current tree deferred to the wave-2 close (the 53 remaining actives re-walked there; the three mid-flight fixes at open (M-093/N-100/M-079) were already conformed by their own lanes and their landings verified in S10-F/wave-3 — no re-work needed).

Stage Summary:
- Board: **53 active (6 SEVERE · 21 MAJOR · 21 MINOR · 5 INFO) / 285 resolved / 4 tracked-open**; next free S-053+, M-111+, N-137+, I-031+, A-012+.
- Findings: +N-135 (the renumber), +N-136 (new); retirements: M-102, M-083 (both PARITY-VERIFIED).
- The wave-2 deep lanes (RAWSearchDLL infrastructure; og config/data assets; the seven non-searchdll ghidra trees; the promotion-guard decode; the core stand-in register) spawn next.

---
Task ID: S10-F/T11 (session-10, wave 2 — the tracked-open resolutions)
Agent: session-10 function-level completeness audit (lead lane S10-F)

Work Log:
- OQ-S8-1 (CWindowsFS's consumer): the T11a read-agent traced the IFSInterface
  factory through the MOM dispatch (the vtable+0x38 slot — zero direct callers)
  to THREE consumers: (A) the og's in-app partition file browser (DRWUI's
  browse button :1421116 → CreateExplorerPartitionObject :438891 →
  ExplorerPartition::Open :8481-8492 → the factory → CWindowsFS for
  OS-accessible volumes / raw parsers otherwise — USER-VISIBLE existing-file
  browsing), (B) DeviceManager's used-space, (C) PartitionRecoveryD's
  root-openability probe. The lead verified every load-bearing cite first-hand
  (the GetProcAddress sequence, the raw-flag factory chain, the browse
  trigger) before filing M-111 (MAJOR — the capability finding with the scope
  decision; evidence s10f-cwindowsfs-consumer-trace.md).
- OQ-S10-2 (VideoViewer/sfc): the decisive probe ran — zero consumers across
  every reversed binary + string table (the wide grep's only hits: Windows'
  own sfc.dll import in unins000, and the "plcfsed" substring in an unrelated
  error string); first-hand structure read (the " cfs-" header + 30 FAN.
  markers + high-entropy payload, no clean framing). Verdict: dead-shipped
  payload; registered via N-134's addendum; the OQ closed.
- Both OQ files moved to findings/FIXED/ with CLOSED status + ledger rows;
  CROSS-REF-MATRIX/mom-capabilities/SUMMARY updated.

Stage Summary:
- Two of the four tracked-open questions RESOLVED this wave; M-111 filed;
  the remaining two (OQ-S4-3 execution-bound; OQ-S10-1 the fix lanes') stay
  tracked. Next: close-out (README currency, final counts, the read-only
  guarantee verification, final push).
### M-077's Resolution record (session-10, this lane — retired per the purge convention)

**Status:** FIXED · **Commit:** af5ca4d · **Verified by:** Preview.M077SizeGate100MiB (RED pre-fix) + Preview.M077TimeGuardDefaultsOnAndOverridable; suite 1035/1035. **Notes:** Caps::max_file_bytes = 100 MiB (the eligibility gate, refuses before any read, truncated-disclosed) + Caps::max_decode_ms = 15000 (the fetch-heartbeat steady-clock deadline; UINT32_MAX disables); zero-init = ON (product policy); the ABI inherits through the existing Caps{} call site; M-028's 4 MiB read/work budgets unchanged beneath.
---
## Session-11-lead territory claim + message-truth correction (post-rebase, at f166440b)

**Message-truth correction:** the f166440b commit MESSAGE is the pre-reconciliation
draft (it describes the wrong N-129 direction, the N-136 draft ID, and the 53-active
board); the commit CONTENT is the reconciled truth (N-129 lane-B-keeps / N-135
wave-3; the N-137 filing; the OQ-S10-3 renumber; the 51-active board). The rebase
--continue accepted the original message unchanged (dumb-terminal EDITOR). This note
is the truth record — the content, not the message, is authoritative (the same
standard the findings hold the product docs to).

**Territory claimed by THIS lane (session-11-lead, the AUDIT lane — complementary
to the fix lanes' claims):** the go-deeper audit waves — (1) the RAWSearchDLL
infrastructure + six-family function-level walk vs core's carve/registry; (2) the
og-codebase config/data assets never compared (FileFilter.xml / DataFile.ini /
raw_signs.json / Dict/ / EUUserData / remote_config_data / EuShare config surfaces);
(3) the seven non-searchdll ghidra trees' never-cited function space (drwui / core /
misc / local / wave2 / mo / partition / drivers / repair); (4) the core-side
stand-in / recorded-follow-up comment register (beyond M-107's ten size-walk sites);
(5) the OQ-S11-1 promotion-guard decode (DRWUI:422508-511). Filings from these waves
cite this claim; the fix lanes' claimed findings (11D's SEVERE tier, 39's MINOR/INFO
tiers, 39B's MAJOR preview/query/ABI cluster) are NOT re-audited by this lane beyond
the remove-the-fixed verification already landed.

---
Task ID: S10-F/T12 (session-10, the FUNCTION-LEVEL COMPLETENESS lane — CLOSE)
Agent: session-10 function-level completeness audit (lead lane S10-F)

THE READ-ONLY GUARANTEE (verified per-commit): all six of this lane's commits
(b478a4b, dd0b53d, deea024, 8365956, ac39c32, 4f44db4) touch NOTHING outside
diff-to-fix/ + worklog* — the per-commit file-list check is clean. The
product-code changes visible in the final tree (test_thumbnail, cr_records,
rrecov, cr_carve/cr_analysers, test_carve, test_cr_ui, ...) entered
exclusively through the REBASES onto the parallel fix lanes' landings
(M-074 d6c02914, M-079 f43b747, M-093 ca701711, N-100 faa547f7, M-092 eb1f116,
M-102/M-083 e56baac7, S-043 df08c07e, M-088...) — pulled per the SOP's
moving-target rule; this lane's affected findings were re-verified against
those landings during the reconciliations (N-133 re-verified + sharpened at
the post-M-079-fix tree: the landed image-stem fallback bakes in the refuted
premise, now the divergence N-133 owns).

COVERAGE STATISTICS (this lane):
- Function inventories: 9 engine function lists parsed — 9,618 functions,
  3,088 named, 122 domain-classified after CRT/STL/EH filtering; the
  class-coverage map: 191 classes cited by name / 49 never (the 49
  dispositioned one-by-one: 25 engine classes by the T3a/T3b vtable-level
  reports — ALL COVERED semantically; the rest noise/infra).
- The ghidra-tree census: 103 decompiled binaries across 9 trees — 71
  dispositioned, 23 OOS-registered (14 name-absent at family level), 9
  never-dispositioned (6 substantive → N-134; 3 weak infra-only,
  re-confirming the inventory's rollup-covered claim).
- Tracked-open: 2 of 4 resolved (OQ-S8-1 → the consumer trace + M-111;
  OQ-S10-2 → the decisive probe + the dead-payload registration).
- Findings filed: M-110 (MAJOR @eaDir), M-111 (MAJOR the browse capability),
  N-133 (MINOR engine-side volume labels + the M-079 premise correction),
  N-134 (MINOR the product-frame register batch); 1 addendum (M-079 via the
  ledger row after its mid-session retirement); 1 process repair (the
  M-108→M-109 collision).
- Evidence: 4 evidence files (s10f-btrfs-eadir-suppression,
  s10f-engine-volume-labels, s10f-product-frame-register,
  s10f-cwindowsfs-consumer-trace) — every load-bearing claim lead-verified
  before filing (the FUN_180001cd0 control flow; the FUN_18003f3a0
  $VOLUME_NAME chain; the six binaries' RTTI greps; the ExplorerPartition
  chain's three key sites).

BOARD AT LANE CLOSE (filesystem truth, after the parallel lanes' concurrent
retirements): 52 active (6 SEVERE · 20 MAJOR · 21 MINOR · 5 INFO) / 294
ledger rows / 3 tracked-open (OQ-S4-3 execution-bound, OQ-S10-1 the fix
lanes' thumbnail-codec question, OQ-S10-3 driver-repo-resident).

THIS LANE'S CONTRIBUTIONS TO THE STANDING FIX ORDER (for the next fixing
session, on top of the existing order):
1. M-110 (@eaDir) — small, engine-scoped, user-visible on Synology volumes;
   the fixture + three tests are specified; NOT a filterfolder change (the
   semantics are descent-suppression, different mechanism).
2. N-133's fix plan — coordinate with the landed M-079: add the engine-side
   label decode precedence leg (engine on-disk label > image-stem fallback)
   and replace the false-premise comment at rrecov.cpp:329-332.
3. M-111 step 1 — the scope DECISION (one docs commit: register the
   deliberate absence or open the browse seam); the finding closes either way.
4. N-134 — the register rows (docs-only; the legacy.dll decode question can
   stay open indefinitely).

HANDOFF: the function/class/binary-level completeness pass is DONE — the
corpus's engine-bearing surfaces are verified CLEAN at every level the
mandate named (artifact → binary → class → the two new behavioral decodes
filed). The residue for future sessions: the FIX work (the 52 active
findings), the legacy.dll decode (N-134's open question), and the three
tracked-open questions. No source code was modified by this lane.

## session-10 (this lane) — the close-out handoff

**Landed this lane (13 findings resolved, each with a fail-before/pass-after proof):** M-090 (DMG koly walk + ctor cap, 771e2ec-lineage), M-084 (ACCDB second signature) + M-091 (the MDB page-chain walk) together, S-043 (thumbnail EXIF orientation, the full 8-case RotateFlip battery), M-080 (the raised 32 MiB default + the byte-oversize embedded fast path), M-085 (the TIFF LZW decoder, reference-verified before landing), S-045 (the TIFF-RAW half — the JPEGInterchangeFormat preview path, source_format "raw"; the codec-bound halves HEIC/WebP/PDF/video honestly tracked-open as OQ-S10-1), M-102+M-083 (reconciled with the parallel lane's node-addressed landing that reached main first — their surface stands, verified green), N-136 (the session-list CWD-dir bare-path fix, DISCOVERED during the M-102 verification, renumbered from the N-128 ID collision per the SA-9 rule), M-072 (path_substring + state_mask query axes + the ABI append), M-074 (the destination-validation taxonomy), M-077 (the 100 MiB/15 s preview product caps). Plus: the F8-gate green-restore (the parallel M-093 lane's mid-flight push missed the FMT_SVG kTable row — every carve_slice call was failing the runtime registry-size gate while the build stayed green; root cause: the two build-time gates count different things — a static_assert on the kTable size itself is the hardening candidate), and the driver-repo dispositions (M-086/M-087/M-089 pushed forward as tracked-open OQ-S10-2 with the two-repo grant boundary recorded).

**Remaining of this lane's declared territory (with the next concrete steps):**
- **M-073 (scan-time folder filter)** — ScanConfig gains `std::vector<std::string> filter_folders` (UTF-8 prefixes, empty = off); the post-parse prune reuses cr_query.cpp's path_compatible prefix logic BEFORE node insertion; ABI: cr_ui_scan_config tail append (`const char* const* filter_folders_utf8; uint32_t filter_folder_count;`); CLI `scan --filter-folder P` (repeatable) + the four-site verb lock; the session round-trip persistence (the config-record field); tests: ScanTimeFolderFilter.{PrunesOutOfPrefixNodes,EmptyIsOff,SurvivesSessionRoundTrip}.
- **M-075 (chkdsk launch)** — new src/win/cr_fsck_volume_win.cpp (CreateProcessW `chkdsk.exe <vol> /f`, the build_command seam mirroring detail::build_unlock_command), ABI cr_ui_fsck_volume(char letter, uint32_t flags, int32_t* exit_code), CLI `fix --chkdsk C:` + the lock, and the D-90/PARITY-DELTAS:2977 rewrite (the "chkdsk stays out of core" registration is superseded).
- **M-076 (TRIM enum rows)** — cr_ui_disk_info tail-append trim_known/trim_on (zero-init = unknown) + the opt-in cr_ui_enumerate_disks_ex(flags with CR_ENUM_FLAG_TRIM) calling win::health::query_trim_state per disk when opted in + `list --trim` + the latency measurement at landing.
- **M-078 (BitLocker turn-off)** — cr_bitlocker.cpp turn_off(letter) extending detail::build_unlock_command with the -off mode (the SFC-safe fixed form), ABI cr_ui_turn_off_bitlocker (explicit opt-in, progress, never silent), CLI `unlock --turn-off C:`, the D-16 legitimate-unlock-only register row updated.
- **N-096 (identity IOCTL non-issuance)** — the disposition note in the finding + DRIVER-UTILIZATION.md:64 row correction (the wire matches; the client-side per-open 0x60002408 query is the informational divergence — decide whether the transport issues it at open).

**The board at this lane's close:** the audit trail's live counts live in SUMMARY.md + FIXED-LEDGER.md (the parallel lanes maintain both continuously; this lane's rows are in the ledger). Both suites green at the close (cr_tests 1035/1035, cr_ui_tests 59/59, the C gate OK both invocation classes, clang-format 18.1.8 clean). New findings keep arriving from the active audit lanes (S-048..S-052, M-103..M-110, N-129..N-134 at the last pull) — the fix lanes should re-pull before claiming.

---
Task ID: S38/CLOSE (session 38 close — the handoff)
Agent: session-38 diff-fix lane (the minor-first session)

FINAL STATE (this lane's last push: 83b9358; main green locally at every push:
linux-release gcc Werror, full-repo clang-format 18.1.8 zero violations, suite
1094/1094, win_syntax_check zero FAILs after the M-074 stub growth).

THIS SESSION'S TALLY — 22 findings closed (the user's minor-first directive):
FIXED with RED→GREEN tests: N-094 (the -h sessions verb + the four-site verb lock),
N-092 (the WMF/EMF/ICO named refusals + the cache-scope decision), N-116 (the
session-list newest-first ordering; the comparator direction settled first-hand),
N-098 (the always-fresh enumeration contract + its pin), N-100 (the NTFS +0x60
total-clusters carrier, nflags bit-9, sparse-zeroing), N-093 (the og's four batch-1
bound pairs; the 10,000,001-byte MIDI carved under the old 64 MiB row - the finding's
exact falsifiable reproduced). FIXED doc-truth with site-verified edits: N-117, N-118,
N-103, N-095, N-108, N-119, N-104, N-097, N-102, N-123, N-101, N-120, N-091 (the
D-108/D-109/D-110 registrations). FIXED-ALREADY with reproductions: N-099 (E55),
N-106 (the SI:86 row), N-105 (D-106). Plus two honesty repairs this session owned
after parallel-lane flags/landings: the N-129-flagged residuals (the missed R-28:376
site + the phantom pre-rebase ledger SHAs) and the FMT_SVG registry row my own rebase
window dropped (the 187-test F8-gate regression, root-caused and restored), the
parallel lanes' format dirt (twice), and the M-074 win-stub growth.

HONEST REMAINDERS (not closed by this session, with the next step):
- N-107, N-127: the fixes live in the rda-driver repo - OUT OF this session's
  two-repo grant (core + corpus only). The findings' own site lists are the complete
  work orders; the next step is a driver-repo session with the same SOPs.
- N-111/N-112/N-113/N-115 (the carve school-2/3 children): the parallel carve lane
  is actively landing exactly these (M-092/M-093/M-096 landed mid-session; MOBI/SVG
  in the tree now). Left to the active lane per the no-duplication rule.
- N-090 (session-list row shape: label[32] vs og 100, 1 s granularity, no device-type,
  the auto_ filter decision): the registration move (D-111: the every-file listing
  superset + the not-carried set) is the next session's quick win; the ABI-widening
  halves need the version-gate policy decision first.
- N-096 (identity-IOCTL non-issuance): option 2 (the D-112 registration + the
  DRIVER-UTILIZATION wording downgrade) is one doc commit; option 1 (issue the mode
  query at open) needs the Windows lane's win-stub test.
- N-121/N-122 (ESP automount, NVRAM create/delete): registration-first per their own
  plans; the implementations need the corpus decode passes their Open questions name.
- N-125 (disconnect classification): the error-code append + the documented
  reattach-continue flow; the DRWUI dialog-trigger probe settles the engine-side
  question first.
- N-126 (Unsaved File category): the og's membership list decode (the R-27 type-ID
  space) is the prerequisite; then the append-only category enum + the carve plumbing.
- The parallel lanes' newer filings (N-128+...): owned by their lanes; the board's
  live truth is the filesystem + FIXED-LEDGER.

METHOD NOTES for the next session (the push-window lessons):
1. Pull OFTEN - this window saw 15+ parallel landings; every rebase window is a
   conflict-resolution event: resolve worklog by keeping BOTH sides, and RE-RUN THE
   FULL SUITE + THE FULL-REPO FORMAT CHECK before pushing (rebase windows inherit
   other lanes' dirt - twice this session).
2. Conflicted rebases over shared TABLE files (cr_registry.cpp) can silently drop
   rows - the F8 size gate caught mine; trust the gates, re-run everything.
3. The purge convention is live: Resolution in the finding file -> FIXED-LEDGER row
   -> git rm; the file's history copy is the archive. Post-rebase SHAs are the only
   citable ones (pre-rebase SHAs become phantoms - N-129's class).
4. The board's counts: the filesystem is the truth (the "board-count truth pass"
   pattern); recount at every handoff.
Task ID: S38-M071-CONVERGE (this lane's reconciliation landing)
Agent: session-38 execution lane (parallel to the fix-lane that landed ce2405a)

Work Log:
- Collision + resolution: this lane's independently-prepared M-071 fix (identical two-gate restoration, same 69507cb form, same census evidence) hit the push window with the parallel fix-lane's ce2405a already on main. Per the convergence precedent (the SCAN-2e-recon lineage): the landed fix is the superset (rrecov pair + the cr_ui session/snapshot walks + the truthful comment + the explicit-selection pin) — re-verified first-hand at reconciled main (rrecov gates :825/:1970, cr_ui :3711/:3852, the cr_scan comment naming every surface) — and no duplicate code was kept from this lane's draft (branch s38-backup holds it; the draft's unique artifacts were re-applied).
- The surviving landing (what the landed Resolution itself named as its gap - "the rrecov.cpp pair cannot execute on Linux"): (1) the Windows-CI step `CLI junk-filter contract (M-071 - tree + recover-all)` in checks.yml - rrecov scan --image + tree --in + recover --in over the committed fixture, asserting the junk set never prints and never lands in the recovery output while the controls do; (2) tools/gen_junk_gate_fixture.py + tests/data/images/junk-gate-fat16.raw (SHA256SUMS-pinned, deterministic, the suite's build_fat16 geometry; the generator's own 2-byte slice-shrink and 8.3-padding slips were caught by the new test's assertions during bring-up); (3) FilterFolder.JunkGateFixtureFileScansAndFlagsHiddenSet (Linux proof of the shipped file: 4 junk nodes flagged, query-gated, TRACKER.TXT stays a result - the two-surface semantic) + SafeWriter.ExplicitSelectionStillRecoversJunkFlaggedNode (the core-side explicit-selection pin, twin of the landed CrUi pin); (4) the M-071 finding's session-38 convergence addendum.
- Trail repairs: driver-install.md's mislabeled M-071 row -> M-086 (the SA-9 lane's pre-reconciliation local number); ci-local.sh + ci-local.ps1 commit-scope regex widened to [A-Za-z0-9._-] (259 origin/main commits carry uppercase scopes - the regex was the outlier); docs/PACKAGING.md S-038 link repointed to FIXED/ (the PARITY-FIX-9 move; ci-local's doc-link check found it).
- Environment truth: local clang-format pinned to the CI's 18.1.8 (the unpinned 23.x turned [1/10] red on untouched files - the exact drift the CI pin comment documents); developer note in the ci(gate) commit.

Stage Summary:
- Board at this landing: 71 active (6 SEVERE · 26 MAJOR · 34 MINOR · 5 INFO) · 254 FIXED · 2 tracked-open (M-071/S-042/M-090/N-094 closed by the parallel lanes; this lane added the M-071 execution proof + repairs). Next queue for THIS lane (user directive: severe/major first): S-041's school-2/3 children (M-092 HTML, M-093 XML/SVG, M-096 MOBI, M-098 HIK, M-099 AEP - grammars fully decoded in the finding files), M-084 (ACCDB second signature), M-079 (auto-save label), M-080 (thumbnail budget), M-083 (grade promotion), S-044 (auto-save gates), S-043 (EXIF orientation), S-047 (TS sync scanner - the list-layer architecture enabler). The driver-repo findings (M-086..M-089, N-107) stay out of this session's two-repo grant.
- Push-window reconciliation #2 (the rebase onto the SA-10B/session-10 wave): the DFX-38 lane's addendum reveals a THIRD parallel M-071 lane had landed its own Windows-CI step ("CLI junk-gate probes") + runtime generator (gen_fat16_junk_fixture.py) in the same window. Two steps for one contract = the exact duplication the trail merges away; merged into ONE — this lane's step kept (strict assertion superset: $USNJRNL + SVI + TRACKER.TXT subtree-skip + KEEP.ME + deleted _ELFILE.TXT controls on top of the DFX probe's set), consumed by the committed, SHA-pinned, Linux-suite-proven fixture; the DFX generator retired WITH ITS STEP (their in-memory CrUi tests + build_fat16_junk() contract remain the ABI-side twin). Both addenda preserved in the finding; the merge recorded in the post-merge note.

---
Task ID: S38-M079-CONVERGE (the auto-save volume-label convergence record)
Agent: session-38 execution lane

Work Log:
- This lane verified M-079 independently (same census: no fill site; the false cr_scan.h:236-239 comment) and implemented an ORCHESTRATOR-side derivation (the FS structures themselves: FAT root label dentry/BPB with the FAT32 chain walked via FatTable, exFAT 0x83/boot, NTFS $VOLUME_NAME via the bootstrap reader) with cross-platform behavioral tests - while a parallel fix-lane landed the CLI-side fill (0b76757: fill_volume_label at scan setup, --label override > the --volume target's GetVolumeInformationW > the image filename stem) and closed the finding with its own source-truth pin test (CrUi.CliScanFillsAutoSaveVolumeLabel).
- Convergence resolution per the repo's precedent: the LANDED design stands (their fill follows the finding's fix-plan seam - the identity-card sibling - and covers the CLI surface the finding named; the og's own GetVolumeName is the mounted-volume API their --volume path calls directly). This lane's orchestrator-derivation draft was REVERTED (branch history holds it; the design note is recorded here for any future session that wants FS-truth labels on raw image scans - e.g. auto_<volume-label> instead of the image stem - as a deliberate-divergence decision, not a defect).
- This lane's surviving M-079 complements (the unduplicated value): (1) the Windows-CI junk-gate step's label assertions, rewritten to the LANDED semantics - rrecov scan --image over the fixture now proves end-to-end that fill_volume_label's stem rides the name (auto_junk-gate-fat16<stamp>) and that --label CUSTOM overrides (auto_CUSTOM<stamp>); the landed fix's own test is a source-truth pin (MSVC-only surface), so the CI step is the behavioral execution proof; (2) the exFAT test fixture's label entry SPEC-REPAIR (+2 -> +4, the exFAT spec 7.2 VolumeLabel offset; the old position overlapped the reserved field - a spec-invalid entry the skip-only tests could never see); (3) the resume-path label adoption in adopt_resume_state_ (the SESS-2 card completion: model/serial were adopted from the loaded session, the label was not - without it a load->re-save cycle drops the label the first save carried).
- The M-071-converge material (the CI step, the committed fixture, the generator, the tests, the trail repairs) is untouched by this reconciliation - no other lane duplicated it.

Stage Summary:
- Board: 70 active (6 SEVERE · 25 MAJOR · 34 MINOR · 5 INFO) + the ledger. The remaining SEVEREs: S-040 (image-create, session-10's territory), S-045 (thumbnail format coverage phase 2 — video/RAW/HEIC/PDF pipelines, unclaimed), S-047 + the S-041 school-2/3 children (carve lane's territory), S-048/S-049/S-050 (SA-10B's new filings, their territory). This lane's next: pull, re-verify the board, take the next unclaimed major or assist the carve school (M-085 TIFF LZW unclaimed as of this writing).

---
Task ID: S38-CLOSE (this lane's session close and handoff)
Agent: session-38 execution lane

Work Log:
- This lane's landings (verified, pushed, green): `53eb9fff` — the M-071 convergence proof set (the Windows-CI junk-gate contract step over the committed Linux-proven fixture — the behavioral execution the landed fix's own Resolution named as its gap; the generator + SHA-pinned fixture; the two Linux gtests; the ledger/matrix/module trail) + the M-079 convergence complements (the CI label assertions in the LANDED fill_volume_label semantics — auto_junk-gate-fat16<stamp> / the --label override; the exFAT fixture label-entry spec-repair +2->+4 per spec 7.2; the resume-path label adoption completing the SESS-2 card) + the win-stub growth for M-074's SDK constants (the parse gate's own prescribed remedy — a real catch: the M-074 landing red the Linux gate while Windows CI stayed green, exactly the class the gate exists for) + the gate repairs (the commit-scope regex widened to the de-facto uppercase-scope convention in both local mirrors; the PACKAGING.md S-038 link repointed through the purge) + the drive-by clang-format repairs on four waves of parallel landings (keeping the [1/10] gate green at merged HEADs).
- The convergence discipline this session executed (the repo's own precedent, three times over): M-071 (three parallel lanes; the landed superset kept, this lane's proof-set supplement landed), M-079 (the landed CLI-side fill kept; this lane's orchestrator-side FS-derivation draft REVERTED with the design note preserved here for any future deliberate-divergence decision — FS-truth labels on raw image scans: FAT root dentry/BPB, exFAT 0x83, NTFS $VOLUME_NAME), and the CI-step merge (the DFX-38 lane's probe absorbed into the assertion superset).
- Verification posture at every landing: full suite green (1025 -> 1098 tests as the parallel lanes landed), the local gate GREEN end-to-end, clang-format 18.1.8 (the CI pin; the unpinned 23.x drift documented), the token never committed (askpass-only, env-only; the secrets scan green every time).
- Push-window lessons recorded for future lanes (this session hit them all): (1) the remote moves every few minutes - fetch immediately before EVERY push and expect 2-3 rebase cycles per landing; (2) NEVER trust a quiet `git pull --rebase -q` in a retry loop - a conflicted rebase masquerades as "in sync" through `git status -sb` grep heuristics (this session recovered one such state via the reflog + cherry-pick; the recovery is recorded above); (3) trail files (worklog/SUMMARY/matrix/ledger) conflict on EVERY cycle - resolve keeping both sides, never dropping the other lane's entries; (4) the commit-scope regex accepts [A-Za-z0-9._-] - no spaces, no '+'.

Stage Summary:
- Board at this lane's close: 51 active (6 SEVERE · 19 MAJOR · 21 MINOR · 5 INFO) + 3 tracked-open, every remaining SEVERE under a declared territory (S-040 session-10's image-create; S-048/S-049/S-050 SA-10B's carve-grouping/naming/file-repair; S-051 the whole-disk lane; S-052 the LDM span - GAP-S6-15's owner). This lane's two verified findings (M-071, M-079) are closed with their convergence records; the audit trail's per-finding truth is intact (ledger rows, matrix MATCH cells, module verdicts, the retired files carry their Resolutions in git history).
- The remaining work is the fleet's active queue, not an unclaimed gap; the two-repo grant held throughout (the driver-repo findings M-086..M-089/N-107 were dispositioned as out-of-scope per the session prompt, never touched).
### Session-39 fix log — N-111 landed (the JP2/JPX/JPM family)

**Finding:** MINOR N-111 (the JP2 family absent; the jP -signature probe +
brand map decoded; the size walk recorded as the follow-up).
**Classification:** CONFIRMED-DIVERGENCE (re-verified first-hand: the probe
gates + the brand map + the registration re-read in the corpus at
RAWSearchDLL_decompiled.c:3544-3559/:21812-21936 before any code was
written; the size walk FUN_18001c760/c820/ca60 then decoded line-by-line —
the finding's own "not decoded — recorded follow-up" was executed this
session, no stand-in needed).
**Fix:** FMT_JP2/FMT_JPX/FMT_JPM (ids appended; serialized carve_format
values stable). FMT_JP2 carries the og's own scan key — the "jP  "
signature-box type at +4 (the box length at +0 never gated, the salvage
posture). FMT_JPX/FMT_JPM carry the brand-position keys (+0x14) so every
family member resolves its own extension; the exact-match brand map
("jp2 "→0x80 / "jpx "→0x81 / "jpm "→0x82, unknown brands reject) rides
CarveResult::format_override (the M-093 SVG precedent). The extent: the
decoded box walk — BE u32 lengths, the len==1 u64 extended form, the len==0
FF D9 EOC byte scan (16-zero back-off latching once, guard 0x1F3FFFFF),
the high-bit size-0 reject, and the jp2c-consecutive rule (the first
non-jp2c box after a codestream box ends the extent). Cap 500 MB
(0x1F400000). kTrialOrder extended (the completeness static_assert).
**Tests (RED pre-fix by construction — the formats did not exist):**
Carve.Jp2BrandMap / Jp2SignaturePayloadGate / Jp2UncheckedBoxLength /
Jp2UnknownBrandRejected / Jp2BoxWalkExtent / Jp2EocScan /
Jp2Jp2cFinalAtSliceEdgeFallsBackBounded. Suite 1040/1040 (998 passed / 42
Win-only skipped / 0 failed, linux-release); clang-format clean; F8 +
S-017 self-probe green with the three new rows.
**Finding correction recorded (the audit trail, not the code):** the
finding's test-plan line "a corrupt-length signature box still carves
(with the correct extent)" is gate-level TRUE (the probe never reads the
length) but extent-level FALSE — the og's own walk advances BY the bogus
length, exits every window bound, and never terminates (the c820 decode);
the core's single-pass adaptation takes the family's bounded fallback
there. Documented in the finding's Resolution + the test comment.
**Trail:** the finding moved to findings/FIXED/ with the Resolution
(the session-39 convention: the user's "put fixed findings in fixed
directory" line; the FIXED-LEDGER go-forward section amended); ledger row
N-111 added (268 main-pattern + 20 OQ = 288); CROSS-REF-MATRIX: the
carve·JP2-family cell → MATCH + the batch-2 row updated; modules/carve.md
verdict updated; SUMMARY session-39 delta block added (board 49 active).
---
Task ID: S11/T1 (session-11 continuation lane — the open, second-of-its-kind session per the user: "look for what session 1 missed, remove the fixed things, go more deep, mark all remaining features, implementations, functions, as finding too")
Agent: session-11 continuation audit lane (S11)

Session open at core `8365956e` (main; the S10-F close), corpus `be0cfe9` (unmoved).
Prompt saved redacted at worklog/session-11-continuation-audit-prompt.md. Repos cloned
fresh; the audit-ctx scratch is /home/z/my-project/audit-ctx/ (outside both repos).

OPENING FINDS (first-hand, before any lane spawns):
- **N-129 ID collision on the board**: TWO N-129 files live in findings/MINOR/ —
  `N-129-parity-og-core-inventory-numeric-defects.md` (S10-B lane, landed 0e279e43
  17:26) and `N-129-fix-wave-bookkeeping-truth-batch.md` (session-10 wave-3 lane,
  landed f6c00177 17:32, itself renumbered N-128→N-129 without seeing S10-B's push).
  Per the SA-9 rule (earlier push wins) the S10-B inventory finding keeps N-129; the
  wave-3 bookkeeping batch renumbers to **N-135** (next free MINOR after S10-F's
  N-133/N-134). Repair queued this session.
- **The bookkeeping batch triaged at HEAD 8365956e** (remove-the-fixed applied to its
  own items): item 1 (REFERENCE-API-GAP:376 R-28 row) FIXED by 5fecdae1; item 6 (the
  re-created findings/FIXED/ dir) RESOLVED (dir gone). Still standing at HEAD: item 2
  (R-31 :379 PARTIAL vs :225 PRESENT self-contradiction), item 3 (five phantom SHAs —
  all five re-verified ABSENT via git cat-file at this session; the ledger header
  block + the S-041 row still cite phantom `771e2ec` as M-090's landing), item 4 (the
  M-090 ledger row's `180053de0` decompile-name-as-SHA mis-paste at FIXED-LEDGER:154),
  item 5 (M-082's ledger row still missing — M-080/S-043/M-085 have rows, M-082 does
  not), item 7 MUTATED (ABI-CONTRACT:525 §7g cites cr_ui.h:916-944; the W1b block has
  since grown — the decl now ENDS at :946, the cite now undershoots), item 8
  (cr_thumbnail.cpp:11-12 file-top matrix still lacks LZW while the header's
  include/cr/preview/cr_thumbnail.h:40 matrix carries it).
- **Last-10-commits liveness**: the three freshest fix landings (0b76757c M-079,
  ca701711 M-093, faa547f7 N-100) all targeted ALREADY-RETIRED findings — no active
  finding's defect site is known-fixed by them; N-133 was already re-verified
  post-landing by S10-F. Full active-board liveness sweep assigned to a lane.

SESSION LANES (spawned in parallel; scratch sheets under /home/z/my-project/audit-ctx/):
- L1 RAWINFRA — the RAWSearchDLL infrastructure function-level walk (the trail's own
  named wave-2 candidate, never executed: the S10-F 122-domain-function disposition
  never landed as evidence — its scripts lived in the wiped scratch area).
- L2 ZFSREFS-FN — ZfsSearchDLL + ReFSSearchDLL function-level walks (the two engine
  trees the S10-F class pass's read-agents did not cover; module docs are
  behavior-level from the corpus deep-dives, not per-function).
- L3 OQTRACE — OQ-S8-1 (CWindowsFS consumer trace in the MOM decompiles) + OQ-S10-2
  (the sfc payload probe) + OQ-S4-3 scoping (chunking-relative-perf).
- L4 TRIAGE — the 56 active findings' cited core-side defect sites re-verified at
  8365956e (remove-the-fixed sweep) + the S10-B N-129's six register items re-check.

---
Task ID: S11/T2-T10 (session-11 continuation lane — the deep walks, the filings, the reconciliation, the close)
Agent: session-11 continuation audit lane (S11 — the second continuation lane; distinct from the parallel session-11 wave-1 lane at 7990a86d and the session-11-lead claim at 83d154b5)

SESSION CLOSE RECORD. Opened at core 8365956e / corpus be0cfe9; closed at core
46b81573 (the worklog commit atop 432d15dc's code tree; the parallel lanes'
landings absorbed by rebase + first-hand re-verification per the SOP).

WAVE 1 — the four parallel lanes (L1 RAWINFRA / L2 ZFSREFS-FN / L3 OQTRACE /
L4 TRIAGE; scratch sheets under /home/z/my-project/audit-ctx/, promoted to
evidence/): the RAWSearchDLL infrastructure walk (64 infra functions, all 1,921
functions censused — the trail's named wave-2 candidate, NEVER executed before;
the load-bearing machinery verified COVERED by M-005/M-021/M-024/M-025/N-017/
N-031/D-78/D-100/D-101/S-049), the ZFS/ReFS function-level walk (981 + 629
functions; 61 + ~35 engine bodies in full — the two trees the S10-F class
pass's read-agents did not cover), the OQ traces (CWindowsFS consumer, the sfc
payload, OQ-S4-3 scoping), and the 56-finding liveness triage.

WAVE 2 — the push-window reconciliation (the hottest of the trail): the pull
brought 30+ parallel commits — the M-072/M-074/M-077/M-096/N-093 fixes, the
M-102/M-083 PARITY-VERIFIED retirement, the N-129→N-135 renumber, the ledger
conflict/phantom-SHA sweeps, M-111's filing, and BOTH OQ resolutions (S10-F
wave-2, 4f44db49). My lanes' prescribed repairs were ALL landed concurrently —
reconciled with zero duplication (the triage residue rides its evidence file).
THE RECONCILIATION'S OWN FINDING: the S10-F OQ-S10-2 "dead payload" resolution
is a FALSE NEGATIVE — my L3 lane's byte-level decode (executed before their
push) proves the consumer is VideoFileRepairer.exe itself (magic 0x73666320 as
a hex constant + the filename as a DAT_ symbol — their ASCII grep could not
match either). Filed N-138 + the refutation addenda (the OQ file + N-134's
register row) + the methodology note.

WAVE 3 — the filings, every load-bearing claim re-verified FIRST-HAND by the
lead before writing (the decompile bodies re-read at their cites; the core
cites re-pinned at 46b81573 after the fix lanes' line shifts; the sfc decode
re-executed in python; the DRWUI 0xe0000001 policy-twin grepped):
- S-053 (SEVERE): the ReFS unnamed-row drop — silent loss of recoverable
  content (refs.md B5's "minor, parity unknown" resolved the severe direction).
- N-138/N-139/N-140/N-141 (MINOR): the sfc false-resolution refutation; the
  carve OOM policy (with the L1 candidate's overclaim CORRECTED before filing
  — carve_format exists in src/ui; the filed gap is the aggregation surface
  only); the per-extension stats surface; the ReFS zero-size∧no-dot filter.
- I-031/I-032 (INFO): the two walks' registers — OQ-T6 RESOLVED (multi-vdev
  MATCH with M-019), zfs B9 closed MATCH, refs B4 decoded (2-bit grade byte),
  the ReFS/ZFS volume-label closure, the pending-16/Stop-flush/no-progress/
  512-gate boundary set, the 36-slot controller vtable.
- Addenda: OQ-S10-2's refutation, N-134's register correction, M-111's
  double-verification corroboration, OQ-S4-3's refined plan + two stale refs.
- Module rows: refs.md B4/B5, zfs.md B2/B8/B9 + the OQ-T6 table row, carve.md
  findings index. CROSS-REF-MATRIX's S11 block (12 rows). CORPUS-ERRATA's
  E-S11 batch (7 precision corrections). SUMMARY + README lane blocks.

BOARD AT THIS LANE'S CLOSE: 58 active (7 SEVERE · 19 MAJOR · 25 MINOR ·
7 INFO) / 285 resolved (FIXED-LEDGER, unchanged this lane) / 3 tracked-open
(OQ-S4-3 refined; OQ-S10-1; OQ-S10-3). Next free IDs: S-054+, M-112+, N-142+,
I-033+, A-012+.

TERRITORY RECONCILIATION (for the session-11-lead lane's next pull): your
claimed wave-1 (RAWSearchDLL infrastructure walk) was executed and filed by
this lane BEFORE your claim (agents 17:50 → results 18:0x; claim 18:15:38) —
per earlier-push-wins, N-139/N-140/I-031 + s11-rawsearchdll-infra-walk.md own
the infrastructure half; your six-family carve-grammar half remains open.
Wave-3 (non-searchdll trees) does not collide with this lane's ZFS/ReFS walk.

READ-ONLY GUARANTEE: this lane's commits touch NOTHING outside diff-to-fix/ +
worklog* (the two repos' product trees untouched; verified per-commit).

Handoff — the highest-value next fixes from THIS lane's filings:
(1) S-053 + N-141 as ONE ReFS emission-gate pass (the S-053 half is silent
    content loss — the single highest-value fix on the board per severity;
    the fix plan's ordering note: S-053's gate split first, N-141's compound
    filter second, the three-class boundary test pins both directions);
(2) N-139's bad_alloc unification (two try/catch blocks + one diagnostic line
    — small, contract-cleaning, test-scoped in the finding);
(3) N-140's per-format tally (the CarveStats extension + the report row);
(4) N-138's doc-truth sweep (the OQ/N-134/register corrections — one docs
    commit; the sfc row's class is unchanged, only the reason);
(5) I-031/I-032's register addenda (D-78/D-101/I-009 + zfs/refs rows —
    documentation-only);
then the standing board order (S-051/S-052 whole-disk + LDM, M-108's BOM
head-gate, the carve school-2/3 children).

---
Task ID: S40/T1 (session-40 open — baselines + prompt + territory claim)
Agent: session-40 diff-fix lane (the user's minor-first directive)

Work Log:
- Baselines recorded at open: **core origin/main `461825d8`** (the N-111 JP2 landing — pulled at T1, one
  commit past the M-108 retirement `4a6a5164`), **corpus `be0cfe9`** (unmoved). Board at open (filesystem
  truth, re-counted post-pull): **49 active (6 SEVERE · 18 MAJOR · 20 MINOR · 5 INFO) + 3 tracked-open**
  (OQ-S4-3, OQ-S10-1, OQ-S10-3) + FIXED/ holding the retired-in-place set (OQ-S8-1, OQ-S10-2, N-111).
  **Rebase reconciliation #1 (recorded per the coordination contract):** the S11 audit wave landed
  mid-push (`85c8491d`+`d50a54dc`+`53495c0c` — S-053/N-138..N-141/I-031/I-032 filed, the OQ-S10-2
  dead-payload resolution REFUTED by N-138's byte-level decode, the counts re-truthed) — board at the
  rebase point: **57 active (7 SEVERE · 18 MAJOR · 25 MINOR · 7 INFO) / 294 ledger rows / 3 tracked-open**.
  This lane's queue is unaffected by the new filings except where noted per-finding (N-130's OQ-S10-2
  addendum connection); the S11 lane's own fix claims are honored as theirs.
- Prompt saved token-redacted: `worklog/session-diff-fix-40-prompt.md` (the generic
  `session-diff-fix-prompt.md` name is taken by the session-1-era copy; the numbered convention is the
  repo's own). Read twice; the minor-first directive governs this lane's queue.
- **Session-40 territory claim (coordination contract, pull-before-push discipline active):** the
  **MINOR tier except the carve lane's in-flight school children** — N-112/N-113/N-115 belong to the
  active session-39 carve lane (N-111 just landed; per the S38 no-duplication rule their next formats
  are theirs) — **and except the driver-repo residuals N-107/N-127** (their fix surfaces live in
  heicfast/rda-driver, outside this session's core+corpus grant; carried by OQ-S10-3). This lane's
  initial queue, minor-first: **N-090, N-096, N-121, N-122, N-125, N-126, N-128, N-129, N-130, N-131,
  N-132, N-133, N-134, N-135, N-137** (15 findings), each with independent re-verification vs the
  corpus before any code. After the minor queue: the INFORMATIONAL tier (I-026..I-030, re-claiming at
  that point to honor session-39's earlier board), then tracked-open, then whatever MAJOR/SEVERE
  territory remains unclaimed. Pull-fetch before every push; convergence events resolved per the
  earlier-push-wins precedent.
- Convention adopted this lane (per the user's directive and the session-39 carve lane's live
  precedent): resolved findings get the **Resolution section in the finding file, then `git mv` into
  `diff-to-fix/findings/FIXED/`**, the FIXED-LEDGER row, the matrix/module/SUMMARY updates — nothing
  deleted, the file's pre-move history preserved by git.

Stage Summary:
- Next: build/test green baseline (cmake preset linux-release; full suite + the local gate mirrors),
  then the audit-doc read (SEVERITY-SCALE/PARAMETERS/CROSS-REF-MATRIX/the 15 queued finding files +
  their cited corpus artifacts), then the per-finding loop: verify → reproduce → classify → fix →
  test → trail → push.

Task ID: S11B/T2-T4 + T10 (the re-triage wave, the landing-parity verification, the filings — RECONCILED AT PUSH over the hottest parallel window of the trail)
Agent: session-11 lane B (the continuation audit)

Work Log:
- **T2/T3 (the full re-triage)**: all 55 active-at-open findings verified at HEAD by
  three parallel agents + lead spot-verification. Verdicts: 6 SEVERE all still-active
  (S-040/S-049/S-052 stale-cites with corrected line maps; S-050 PARTIAL — its
  false-citation leg fixed at ea2f3ea7); 21 MAJOR verified (M-102/M-083 already
  conformed; M-107's all-eleven sites standing with three in-file corrections);
  21 MINOR + 5 INFO verified (N-134 FIXED-ALREADY — see below; I-026 stale-content;
  I-027/I-028/I-029 accurate). The T4 landing-parity wave found the five recent
  landings' CODE corpus-faithful (M-096/N-100/thumbnail-ABI PARITY-VERIFIED; M-093
  one residual; M-079's image-stem divergence already owned by N-133).
- **T10 (this lane's filings, renumbered at push per the SA-9 rule)**:
  - **M-112** (MAJOR, filed M-111): the CSL 0x44c size branch — decoded-but-unported
    with NO active owner (the M-107 class's twelfth site); corpus FUN_180041950
    :49749+ re-read first-hand (the +0x480 arithmetic quoted); core grep: only the
    comment cites 0x44c. M-107's addendum extended with the site + three in-file
    corrections (AI own validator since e018bf4f; EPS/AI at 16 MiB; SWF FileLength).
  - **N-142** (MINOR, filed N-137): the dropped thumbnail variant's dead wiring —
    CrPreview::src_session/src_node_id written at cr_ui.cpp:3581-3582, read nowhere;
    the promotion runs off cr_ui_thumbnail_generate's own args.
  - **N-134 RETIRED (FIXED-ALREADY)**: all six "never-dispositioned" register rows
    pre-existed at b202f9ed (11:12:59Z) — 6.5h before the finding's filing; present
    verbatim at the finding's own audited 44f043a5 (verified `git show`). The sfc
    addenda preserved in the retired file's Resolution.
- **The phantom-SHA truth repairs** (convergent with session-39's census — both lanes
  repaired the recent rows with git-cat-file-verified commits; their census
  quantified deeper: 73-of-285 rows): this lane's verified repair map — M-084/M-091→
  8a7055ea, M-090→9daa787 (+the 180053de0 mis-paste), S-044→7b7e095e, N-116→9445627f,
  M-093→ca701711, M-096→6e37dc21, M-079→0b76757c+b82ea173, M-082→1f714884 (the missing
  row), M-071→ce2405a, N-100→faa547f7, 954c386→e4f3184.
- **The addenda batch** (16 files): N-135 (the complementary per-item status +
  recurrence table, merged over the session-11 census addendum), N-133 (+the
  registration leg: D-03 stale, no image-stem D-row), N-131 (the N-100-precedent
  refresh), N-132 (the :225 propagating aggravation), S-050 (leg-2-fixed), N-096
  (the extended-IOCTL nuance), I-026 (the persistent dev-cert cite-refresh), + 10
  cite-refreshes (S-040/S-049/S-052/M-073/M-076/N-090/N-121/I-028). The M-072/N-093
  addenda DROPPED at the push reconciliation — both findings retired by the parallel
  fix lanes mid-flight.
- **Board after this push: 56 active (7 SEVERE · 18 MAJOR · 24 MINOR · 7 INFO) /
  298 resolved rows (276 main + 22 OQ, whole-table re-count) / 3 tracked-open.**
  Next free IDs: S-054+, M-113+, N-143+, I-033+, A-012+.
- The N-129→N-135 renumber and the M-102/M-083 retirement from this lane's earlier
  push stand (743d7b8d lineage; the remote lanes' truth commits cite them).


---
## Session-11D wave 1, part 2 (2026-10-29 — S-051 + M-104: the whole-disk partition-list targets + the display-name roots)

Task ID: S11D/W1 (cont. — one coherent landing; M-104's presentation rides S-051's
target builder per both finding files' own cross-references)
Agent: session-11D fix lane

**S-051 (SEVERE → FIXED)** — the silent empty whole-disk scan:
- **Verified first**: the corpus citations re-read (architecture/04 §2 routing +
  §3.4 "reads the partition list"; the insertion trace at
  DRWUI_decompiled.c:904076/:904198 first-hand) and the executed RUN 2
  reproduced at session open on the current main (rc=0, files=0, the empty
  four-root tree — the probe output quoted in this session's earlier entry).
  CONFIRMED-DIVERGENCE.
- **Fix**: `run_engine_pass`'s partition-list target block (cr_scan.cpp) —
  whole-disk basis + unforced hint ⇒ the table parsed, every used
  non-container non-LDM entry a `PartitionSlice` target over the read pipe,
  per-target `probe_family`, `scan_candidate` under the volume's own root,
  F-1 subtree rebase (the candidate fan-out's pattern), skip-honesty counters
  (`ScanStats.table_volume_targets` / `table_volume_skipped`), a per-target
  Info/Warn log line (the og's insertion-trace analog). The deep stage's
  discovery drops candidates matching a scanned target's LBA (the
  no-double-scan reconciliation; honestly-skipped targets stay
  discoverable — the og's additional-not-replacement semantics). The LDM
  block keeps precedence; superfloppy and forced-hint paths unchanged (RUN 3
  control re-executed, with the evidence file's ERR_OPEN_FAILED annotation
  corrected to ERR_BAD_PARAMETER — 0x50030003's true name).
- **The naming primitive** (M-104's tier 1): `fat::read_volume_label`
  (cr_chain.cpp + cr_fat_engine.h) — validate_boot-gated, +0x2B/+0x47 by
  flavor, trailing-pad trim, label-less refusal (never a blank name).
- **Tests (all fail-before by construction — the stats fields, roots and
  helpers did not exist)**: `Orchestrator.WholeDiskScansTableVolumes` (the
  RUN 2 shape flipped: files 4, deleted 2, targets 2, DOC.TXT ×2),
  `WholeDiskGptTwin` (the GPT entry array drives the same targets; both
  CRCs computed in the fixture), `WholeDiskMixedFamilies` (FAT + NTFS
  per-target dispatch; the tier-2 `NTFS @ <LBA>` name),
  `WholeDiskDamagedVolumeSkippedHonestly` (skip counted, rc OK),
  `WholeDiskSuperfloppyUnchanged` (no table ⇒ Found-root behavior pinned),
  `WholeDiskNoDoubleScan` (lost_partitions 0, zero anonymous roots),
  `WholeDiskVolumeRootsCarryDisplayNames` (VOLONE/VOLTWO; the deleted file
  under the same root, flagged), `WholeDiskKnownVsLostDistinguishable`
  (in-table label root vs exactly one discovery candidate),
  `WholeDiskVolumeRootNamesRoundTripSession` (the names persist + reload),
  `FatEngine.ReadVolumeLabelDecodesBootSector` / `ReadVolumeLabelRejectsLabelLessAndNonFat`.
  Shared fixtures: test_fixtures.hpp's `build_labeled_fat16` /
  `build_two_volume_mbr_disk` / `build_two_volume_gpt_disk` (the evidence
  builder's shape, self-contained for the suite; S-048/M-103 reuse them).
- **Pass-after**: the evidence probe's RUN 1/2/3 re-executed: RUN 1 rc=0,
  files=4, deleted=2, VOLONE/VOLTWO roots, lost_partitions=0; RUN 2 the
  same files under the display-name roots (THE finding's acceptance:
  files_found=4 / deleted_found=2 with rc=0); RUN 3 the honest
  forced-hint failure unchanged. Full suite **1105/1105**; the
  whole-disk lost-partition suite (Lost.*) green unchanged — `discover()`'s
  own contract untouched (the filter lives one site above it).
- **M-104 (MAJOR → FIXED)** — rides the same landing; the naming tiers, the
  known-vs-lost distinction, the one-tree-per-volume shape, the session
  round-trip. Honest residuals recorded in its Resolution: the FS-marker
  ROW (corpus-ambiguous per the finding's open question) and the standard-
  root narrowing stay the S-048/M-103 wave's scope; the NTFS/ext4/exFAT
  label decodes compose via N-133 (MINOR tier, next).
- **Trail**: S-051 + M-104 moved to `findings/FIXED/` with Resolutions (the
  user's fixed-directory instruction; the ledger's go-forward convention
  updated); the ledger's SECOND committed merge conflict (the N-093/M-096
  rebase's unswept markers — the same class lane-B swept at bc4513db)
  resolved both-rows-kept (N-136 + N-093 with its real SHA 03ad5523); the
  matrix rows flipped to MATCH; A-012's rows added; SUMMARY counts: **51
  active (5 SEVERE · 20 MAJOR · 21 MINOR · 5 INFO) / 288 ledger rows**.

---
### Session-11D W1 close (the rebase reconciliation + the push)

- The rebase over the parallel S11/39/40 lanes' 34-commit window: **A-012's
  row restore was CONVERGENT** — the parallel S11 lane had independently
  found the same regression and pushed `e4273f2b` ("restore the FMT_SVG row
  ... 187 tests red") before my fix reached the remote; per the SA-9 rule
  their repair stands, my landing was dropped at the DU resolution, and the
  A-012 finding file (kept: the independent fail-before execution + the
  root-cause trace + the format sweep) was re-classified as the convergent
  verification record, moved to findings/FIXED/.
- The ledger's hunk conflicts resolved: the remote's N-134/N-111 rows kept,
  N-093's row now carries the real SHA 03ad5523 (my placeholder repair), the
  remote's phantom-SHA S-041 repair kept, my A-012/S-051/M-104 rows appended;
  the go-forward convention header now names BOTH lanes' identical user
  instruction (session-39's and session-11D's "put fixed findings in fixed
  directory" — the same mandate reached every fix lane).
- SUMMARY counts reconciled to the merged filesystem truth: **53 active
  (6 SEVERE · 17 MAJOR · 24 MINOR · 6 INFO) / 301 ledger data rows
  (279 finding + 22 OQ) / 3 tracked-open**.
- The merged tree fully re-verified: **1128/1128 green** (my 15 new tests +
  the parallel lanes' 23), clang-format 18.1.8 clean (the parallel lanes'
  newest landings' 22 violations swept: test_preview.cpp,
  test_btrfs_volume.cpp, cr_preview.cpp — the same residue class as the
  first sweep).
- NEXT (this lane's declared order): **S-049** (the carved-file naming
  contract — the EXT_%06d default + the per-analyser content prefixes),
  composing with the display-name mechanism landed here.
---
Task ID: S40-ML/T0 (the session-40 MAIN lane open — CI restore + territory claim)
Agent: session-40 main diff-fix lane (the user's tracked-open-first directive; the
parallel minor-first lane's claim at 865f8b43 honored — no overlap)

Work Log:
- Baselines at open: core origin/main 461825d8 pulled, then fast-forwarded to
  c7fa527c (the S11 audit wave d50a54dc/53495c0c + the M-110 landing b95c924b +
  the ledger cite repair). Corpus be0cfe9 (unmoved). Local suite at open: 1052
  tests / 1010 passed / 42 Win-only skipped / 0 failed (linux-release, g++ 14.2);
  clang-format 18.1.8 (the CI pin).
- **CI RED RESTORED (the first finding of this lane, class: push-window format
  dirt):** the last two Checks runs (461825d8, b95c924b) failed [1/2] clang-format
  — 18 violations the parallel landings left (cr_preview.cpp x3 from the M-108
  text-eligibility wave; test_preview.cpp x15 from the same; the M-110
  test_btrfs_volume.cpp dirt was already repaired in c7fa527c's lineage).
  Repaired: clang-format -i on the two files; local [1/10] re-run clean (0
  violations repo-wide); preview/thumbnail suites re-run green (40 passed / 29
  Win-only skipped). The known lesson (worklog S38-close method note 1) holds:
  re-run the FULL-REPO format check after every rebase window.
- **Territory claim (coordination contract):** the parallel S40 lane owns the
  MINOR queue (N-090..N-137 per its claim) and plans INFO next. THIS lane owns,
  per the user's tracked-open-first directive: (1) the three tracked-open OQs
  (OQ-S4-3's Windows dispatch ALREADY IN FLIGHT — run 40747332987, dispatched
  via the API at open on c7fa527c; OQ-S10-1's video host-callback seam; OQ-S10-3
  the out-of-grant push-forward), then (2) the SEVERE tier (S-040, S-048, S-049,
  S-050, S-051, S-052, S-053), then (3) the MAJOR tier (M-073..M-111, the
  driver-repo halves excepted per the two-repo grant), then (4) the unclaimed
  new minors N-138..N-141 (filed after the parallel lane's queue was set).
  Pull-fetch before every push; convergence per the earlier-push-wins precedent.
- Prompt saved token-redacted: worklog/session-diff-fix-40-prompt.parallel-main-lane.md
  (the unnumbered name is the session-1-era copy; the parallel lane holds the
  plain -40- name; this lane takes the .parallel-main-lane suffix per the
  session-37 precedent).

Stage Summary:
- Next: OQ dispositions (the benchmark transcription when 40747332987 lands; the
  OQ-S10-1 video seam; the OQ-S10-3 grant-boundary record), then the SEVERE
  queue in impact order: S-053 (ReFS unnamed rows), S-052 (LDM span basis),
  S-051 (whole-disk targets), S-049 (carved naming), S-048 (carve grouping),
  S-040 (image-create), S-050 (the repair-line registration).


## Session-11B CLOSE (2026-10-29) — the continuation audit handoff

**Audited commit:** core `7990a86d` at open → the hot-parallel-window lineage through
`602dd170` at close (my lane's baselines re-verified at each rebase; the product-code
tip my verification agents used: `743d7b8d`-lineage incl. `740d9b56`). **Corpus:
`be0cfe9`** (unmoved all session). 1212+ commits on main at close.

**READ-ONLY GUARANTEE: HELD.** This lane's six surviving commits (35bee726 ·
bc4513db · 743d7b8d · b37fabab · 602dd170 + the rebase-resolution riders) each
verified `git show --name-only`: zero files outside `diff-to-fix/` + `worklog*`.
(The window's other commits are the parallel lanes' — S11/S11-lead/39/40/fix-lane
— their source landings are theirs.)

**Coverage statistics (the user's three asks, decomposed):**
1. **Remove-the-fixed** — all 55 active-at-open findings re-verified at HEAD by
   three parallel agents + lead spot-verification of every load-bearing claim.
   Board movement this lane: **M-102/M-083 conformed** (the e56baac7 landing's
   missed purge step), **N-134 retired FIXED-ALREADY** (b202f9ed pre-dated its own
   filing by 6.5h), **I-030 resolved** (the module map completed). Dropped as
   moot at push: the M-072/N-093 addenda (both retired mid-flight by the fix
   lanes — convergent with this lane's triage).
2. **Go-deeper** — the T4 landing-parity wave over the five freshest landings:
   M-096/N-100/thumbnail-ABI PARITY-VERIFIED; M-093's one residual filed (M-112);
   M-079's image-stem divergence confirmed owned by N-133 (+the registration leg
   found: D-03 stale, no D-row). The phantom-SHA truth class repaired (11 cells,
   every replacement git-cat-file-verified) — convergent with session-39's deeper
   census; the guard script is now load-bearing.
3. **Mark-all-remaining** — **M-112** (the CSL 0x44c unported-unowned stand-in,
   the M-107 class's twelfth site), **N-142** (the dropped thumbnail variant's
   dead wiring), + 16 addenda (the per-item status table on N-135; N-133's
   registration leg; the cite-refresh batch over 10 stale-cite findings; M-107's
   three in-file corrections), + the reghive.md/apply.md module rows (the map
   completed, 28 rows).

**Board at close: 55 active (7 SEVERE · 18 MAJOR · 24 MINOR · 6 INFO) / 299
resolved rows (277 main + 22 OQ — the whole-table re-count truth; the close-entry's
first draft said 56/7-INFO — it carried the pre-I-030-retirement count; the
filesystem is the truth, corrected) / 3 tracked-open
(OQ-S4-3, OQ-S10-1, OQ-S10-3). Next free IDs: S-054+, M-113+, N-143+, I-033+, A-012+.**

**Recommended first 20 fixes (priority order):**
1. **S-052** — the LDM span misbase: silently wrong recovery bytes on spans/
   stripes/mirrors/RAID-5; the five-link chain map is in the finding's cite-refresh
   addendum; multi-disk fixtures already exist (test_ldm.cpp:206+).
2. **S-051 + M-104** as one whole-disk shape pass (the partition-list target
   builder + the per-volume display-name roots; composes with S-048/M-103/M-079 —
   the executed fixtures and the real-session census are in the evidence files).
3. **S-053** — the ReFS unnamed-$DATA drop (the parallel lane's new SEVERE; the
   decode is complete in the finding).
4. **M-109** — the thumbnail 32 MiB → 200 MB budget: one constant + three doctrine
   comments + a D-row; the cheapest SEVERE-adjacent fix on the board.
5. **M-103** — the LostDir per-parent grouping at the existing cr_mftwalk.cpp:1104-1107
   site (62.6% of a real og session's shape).
6. **M-112** — the CSL 0x44c size branch: one branch inside validate_xmlsvg, the
   decode quoted in the finding; four tests named.
7. **S-048 + S-049** — the carve volume-grouping + naming contract pair (the
   real-session evidence is complete; the naming decode table is in S-049).
8. **N-142** — the dead CrPreview wiring: one deletion + comment rewrite (hygiene;
   10 minutes).
9. **M-106** — the ONE/3DS size walks: mechanical against the already-landed
   decodes (the 500MB chunk-chain / 200MB section-walk).
10. **M-107's remaining ten sites** — the batch-1 stand-ins, one session of
    decode-and-port (X3F first per the fix plan; EPS/AI at 16 MiB per the
    addendum's corrections).
11. **M-101** — the Versions/Tags-Extracted branches (the og's tree shape).
12. **M-105** — the DRWService lifecycle posture decision (implement vs formal §3
    exclusion; rides the driver-contract session with M-086/M-087 residuals).
13. **M-073** — the scan-time folder filter (PushFolderFilter port).
14. **M-075** — the chkdsk launch (or its formal out-of-core registration rewrite).
15. **M-076** — the TRIM facet (one enumeration row + the wiring).
16. **M-078** — the BitLocker turn-off tail (CBitlockerTurnOffOperation port).
17. **M-098 + M-099 + N-112 + N-113 + N-115** — the carve school-2/3 children in
    the real-world order (HIK/AEP are decode-complete in their findings).
18. **M-111** — the Explorer-partition-browse capability decision (the parallel
    lane's finding; pairs with OQ-S8-1's resolution).
19. **N-132 + N-135's doc-truth residues** — the API-DELTA false-landed flip +
    the R-31/:225/:379 reconciliation + the 54 ancient ledger rows' SHA backfill
    (the guard script).
20. **S-040** — the image-create producer half (the ABI + win-layer writer +
    CLI verb; the biggest single feature on the board).

**Handoff to the next session:** the board is truth-current at 602dd170; every
active finding's cites were re-verified this session (the stale ones carry
cite-refresh addenda); the driver-repo trio (M-086/M-087/M-089 + N-107/N-127) and
OQ-S10-3 need a driver-repo lane; OQ-S4-3 is execution-bound (hardware fixtures);
OQ-S10-1 is vendoring-bound (codec dependencies named in the OQ).

### Session-39 fix log — N-112 landed (the LZH entry-chain walk)

**Finding:** MINOR N-112 (the LZH archive family absent; the method gate +
entry-chain walk decoded; list-layer only; erratum E53's key correction).
**Classification:** CONFIRMED-DIVERGENCE (re-verified first-hand: the probe
FUN_18003db20 + the registration :3097-3133 re-read; the walk FUN_18003dd70
decoded line-by-line; **the .data method constants dumped directly from
og-codebase/RAWSearchDLL.dll at RVA 0x9388c** — the decompile labels them
DAT_18009388c..a8 and the binary read settles the mapping: Case A's full
stride = -lh0/-lhd, Case B's hdr-only stride = -lh5, Case C's hdr+comp
stride = -lh7).
**Fix:** FMT_LZH/FMT_LZH5/FMT_LZHD/FMT_LZH7 (one row per method key, the
M-092 duplicate-key policy; all ext "lzh" — the og stamps one id 0x24
whichever key fired). The 5-byte magic carries the probe's own gate
("-lh?-" at +2). The walk ported EXACTLY: the per-method strides; the
ext-flag (+0x120) suppression of the +2 CRC stride; the -2/+2 CRC
step-back dance at the chain break; size = pos+1 (the end-marker byte);
the 200 MiB ctor bound byte-exact (0xC800000). The -lh5 hdr-only stride
is a decoded QUIRK (the og's walk lands on the CRC field after a -lh5
entry and breaks the chain there) — ported, NOT fixed, PINNED by
Carve.LzhLh5HeaderOnlyStridePinned.
**Tests:** Carve.LzhMethodFamilyAccepted / LzhEntryChainExtent /
LzhBrokenChainSalvagesLeadingEntries / LzhLh5HeaderOnlyStridePinned /
LzhExtFlagSuppressesTheCrcStride / LzhWrongDashByteRejects. Suite
1058/1058 (1016 passed / 42 Win-only skipped / 0 failed); clang-format
clean; F8 + self-probe green.
**Trail:** findings/FIXED/N-112 with Resolution; ledger row (269 main +
20 OQ = 289); CROSS-REF-MATRIX: the LZH cell → MATCH + the batch-2 row
updated; modules/carve.md; SUMMARY (board 48 active).
---
Task ID: S11-lead/T6-T11 (session-11-lead waves 2-3 — the five go-deeper lanes + the filings)
Agent: session-11 continuation lane (lead)

Work Log:
- The territory claim (83d154b5) held except where the parallel lanes crossed it mid-flight: a parallel "S11" lane executed the RAWSearchDLL infrastructure walk on their own track and filed I-031/N-139/N-140/N-141 (their walk's residue: the OOM policy, the stats surface, the refs filters) + S-053/N-138 + I-032 + the S40 lane opened — per the no-duplication convention every candidate finding was cross-checked against their filings before mine went to disk; the unique content survived (the TS cap, the filter-exempt mask, the ISpecial two-step, the promotion-guard decode, the boot surgery, the SR purge, the marker, the stand-in/comment batches, the asset-register extensions).
- Lane A (T6): the 17-row infrastructure verdict table — the load-bearing machinery fully dispositioned (MATCH/registered); the unique filings S-054 (the ×10 cap — the load-bearing cites re-read first-hand: the 200000000 clamp at DDC:35507-14, the global at :9049, the 2e9 grep 0 hits, the ×10 comment's origin traced to a3abfbeb, no test pins it), N-142 (the exempt mask arithmetic re-derived: 0x1000801 bits 0/11/24 → ids 0x30/0x3b/0x48 = OLE/RIFF/ZIP), N-143 (the ISpecial two-step); the M-021-seam re-verification (the exact LoadLibrary/GetProcAddress cites) + the per-chunk repair-feed facet (S-050 addendum); the ZIP dual-gate analysis KEPT A FALSE POSITIVE OFF THE BOARD (the og's net admission = probe AND walk = ≤10 = core — recorded as a corpus-erratum instead).
- Lane B (T7): the 40-asset completeness pass — every engine-behavior asset re-verified MATCH by content diff (FileFilter 1,481 unique / raw_signs 139+199+62 with per-extension attribution / EUUserData 111+190 / Dict 228/228); the register extensions filed as I-033 (needcopy→AliyunWrap, BUILDPE-keeps-Firebase, remote_config runtime-refetched, recoveryFree's layout, the OSS-credentials + dev-logging data, the FileFilter D-row consistency, the RawConfig 34-key completeness oracle).
- Lane C (T8): THE DECODE — OQ-S11-1 resolved: the og's promotion is read-gated (FUN_1803eba50 == 0), tier-gated ({0,1,2,4} vs the cat-3 preview-pane exclusion), payload-gated, TxtView-snapshot-gated — binary-verified at 0x140263b2c-b9a; the guard re-read verbatim by the lead. Core under-promotes damaged-but-readable images (the INVERSE of the board-truth hypothesis — recorded honestly in M-112 and the evidence file). Filed: M-112 (the trigger), N-144 (the text-snapshot render), the tier-system + AVPreviewStub decodes registered, M-109's fix-plan enriched (the read-before-decode order). The 19-behavior interaction map: broadly covered/host-side.
- Lane D (T9): the nine trees swept; the binary-level CLEAN census re-confirmed; the never-compared space concentrated in the EPM-heritage shared layer: M-113 (the genuefisystemfile boot surgery — TempSystem/TmpBCD/TmpSoftWare chains spot-verified first-hand; the repo-wide zero-greps re-run), M-114 (the srclient purge — the WQL string and the both-drives caller spot-verified), N-145 (the pendingMarkedDisk marker — the QSettings strings spot-verified), legacy.dll closed (I-033), the tree bookkeeping (the ghidra-local duplicate, the 3 phantom REPORT entries).
- Lane E (T10): the stand-in register — 15 size-walk sites ALL M-106/M-107-owned (the current-line map recorded); the unique filings N-146 (FH11), N-147 (CSL 0x44c), N-148 (the seven-item comment-truth batch — cr_ldm.h's stale UNDECODED vs OQ-T7's resolved MATCH the sharpest); the negative results (no TODO/FIXME debt, no stubs, every named refusal owned, the fallbacks-REMOVE list processed).
- T11 (the filings): S-054, M-112→M-115, M-113, M-114, N-142→N-149, N-143, N-144, N-145, N-146, N-147→DROPPED (convergent with the S11B lane's M-112 CSL filing — extend-don't-refile), N-148, I-033 (11 files survive the push-time reconciliation); the S-050 + N-134 addenda; 7 CORPUS-ERRATA rows; the CROSS-REF-MATRIX session block (16 rows incl. the negative results); 6 module files appended (carve/preview/boot-repair/orchestration/ldm/zfs); the SUMMARY/README deltas; evidence/s11-lead-deep-lanes.md (the consolidated five-lane sheet + the lead's first-hand verification log).

Stage Summary:
- 12 findings (1 SEVERE · 3 MAJOR · 7 MINOR · 1 INFO) + 2 addenda + 7 errata rows + the evidence sheet; the go-deeper mandate's five lanes all closed with recorded negative results.
- The parallel-lane environment stayed hot the whole session (11B/11C/11D/39/39B/S40 + the second S11 instance); every filing re-coordinated against the board at disk time; the read-only guarantee holds (this session's commits touch only diff-to-fix/ + worklog*).
- Remaining: T12 (the close — the board count truth pass at the final push, the worklog handoff, the OQ-S11-1 tracked-open disposition update).

---
## Session-11-lead CLOSE (2026-10-29 — the continuation lane's handoff)

**Audited: core `7990a86d` at open → the `461825d8`-lineage at filing (the board
moved three times mid-session; every filing's core-cites re-checked at disk time);
corpus `be0cfe9` (unmoved).** Final commit: `aab031ac`-lineage (the waves-2-3
landing, itself rebased twice over the concurrent lanes). Message-truth note: the
aab031ac commit MESSAGE lists the pre-reconciliation IDs (M-112/N-142/N-147); the
CONTENT is the reconciled truth (M-115/N-149/the N-147 drop) — the content, not
the message, is authoritative (the f166440b precedent).

**The three asks, delivered:**
1. **Remove-the-fixed (wave 1):** M-102/M-083 independently PARITY-VERIFIED (the
   engine layer executed standalone — 40/40 checks, zero ASan — the two-call
   probe matrix, the 0x50030009+bytes_needed refusal with rgba untouched, caps
   validation refusals, the complete-PNG/truncated-PNG/truncated-JPEG
   truncation-disclosure trio, the full grade-setter semantics) and retired
   (racing the parallel 11B fix lane's own deletion — the verification record
   rides the ledger cells + evidence/s11-board-truth.md); the N-129 ID collision
   repaired per the file-addition timestamps; the OQ-S10-2 collision repaired
   (the driver-repo file → OQ-S10-3); the FIXED-LEDGER's committed merge-conflict
   resolved; M-041's corpus-SHA cell, M-088's placeholder + fictional path, and
   the 54-row as-recorded-in-file census repaired/recorded; N-137 filed (the ABI
   header's never-returned contradiction + the inert allow_embedded knob).
2. **Go more deep (waves 2-3):** five lanes, all closed with recorded negative
   results — the RAWSearchDLL infrastructure (the 17-row MATCH table; the unique
   S-054/N-149/N-143), the og config/data assets (the 40-asset pass; every
   engine-behavior asset byte-identity re-verified; I-033), the DRWUI decode
   (OQ-S11-1 RESOLVED — the promotion is read-gated, binary-verified; M-115 +
   N-144), the nine ghidra trees (M-113/M-114/N-145; the binary-level CLEAN
   census re-confirmed; legacy.dll closed), the core stand-in register
   (N-146/N-148; no TODO debt, no stubs, every refusal owned).
3. **Mark all remaining:** 11 findings survive the push-time reconciliation
   (S-054, M-115, M-113, M-114, N-149, N-143, N-144, N-145, N-146, N-148,
   I-033) + 2 addenda (S-050's per-chunk repair feed; N-134's legacy.dll
   closure) + 7 CORPUS-ERRATA rows + the consolidated evidence sheet.

**Board at close: 64 active (8 SEVERE · 20 MAJOR · 29 MINOR · 7 INFO) — this
lane's net contribution +11 filings; the concurrent lanes (11B/11C/11D/39/39B/S40
+ the second S11 instance) own the rest of the movement (their retirements:
M-072/M-074/M-077/M-083/M-096/M-100/M-102/M-108/M-109/M-110/M-112/N-111/N-134/
I-030 + the OQ resolutions; their filings: M-111/M-112/N-138..N-142/I-031/I-032/
S-053 + more landing as this closes). Next free IDs: S-055+, M-116+, N-150+,
I-034+, A-012+.** 3 tracked-open remain (OQ-S4-3, OQ-S10-1, OQ-S10-3 — the
driver-repo grant boundary).

**Recommended first fixes from THIS lane's filings (priority order):**
1. **S-054** — the TS/M2TS cap: one constant × 2 rows + the provenance comment +
   two pin tests; the wrong-output class, mechanical, no design work.
2. **M-115** — the promotion trigger: the read-vs-decode verdict split in the
   thumbnail outcome (the engine already reads the extents; disclose the read
   separately) + the ≤20-byte skip + the cite repair; pairs with N-144's render
   surface (one landing).
3. **M-113** — the boot surgery: the reghive-writer module (Win32-only, the
   platform doctrine) + the R-69 verdict flip; the VM drill is the acceptance.
4. **M-114** — the SR-purge posture decision (host-consent plan item vs
   registered decline) — a decision first, code second.
5. **N-149** — the filter-exempt set: three format ids + one test.
6. **N-146/N-148** — the registration sweeps (the FH11 re-home + the
   comment-truth batch): one mechanical commit each.
7. Then the standing order (S-051/S-052, the carve children, the S11B lane's
   M-111/M-112 residue).

**The read-only guarantee:** every commit this session touches only `diff-to-fix/`
+ `worklog*` (verified per-commit; the two finding retirements in wave 1 were git
rm of files this session itself appended verification addenda to). No source
code was modified. No fix was applied.

## Fix session 40 (continued) — S-054 + the M-117 discovery (renumbered from the draft M-116 at the push-rebase: the S11-R lane's archive-inventory M-116 pushed first, per the earlier-push-wins rule)

- **S-054 FIXED** — the TS/M2TS carve cap re-pinned to the og's 200 MB
  controller global (2000000000 → 200000000 on both rows; the "x 10"
  provenance refuted — the clamp at RAWSearchDLL:35507-35509, the
  fallback literal :35513, the global :9049, and 2e9 appears nowhere in
  the binary — every cite re-verified first-hand at the fix). Tests:
  Carve.TsM2TsCapIsTheOg200MB (registry pin) +
  Carve.TsStreamClampedAt200MB (behavior: the 220 MB stream clamps;
  pre-fix one ~219,999,852-byte node). RELEASE-NOTES carries the S-047
  repair note.
- **M-117 (drafted M-116; renumbered at the push) FILED + FIXED (discovered during S-054's verification, per §8
  — announced here and in the commit)** — the suppress_overlap trial
  loop ran the full cap-sized validator walk for every mid-stream sync
  BEFORE the covered check discarded it: the 220 MB fixture did not
  complete inside 600 s pre-fix; post-fix 11.3 s. The fix is the
  pre-validator covered-check reorder (terminal states identical). The
  post-emission jump variant was tried and REJECTED — it broke
  S007CommonFormatsCarve (an AAC ADTS start inside another format's
  walked extent; the og RANKS overlapping candidates, it does not skip
  them) — the code comment at the emission site records the lesson.
  Suite 1030/1030 + 59 green.

---
### Session-11D W1 final (the S-051 CONVERGENT DU merge — the union of both lanes' landings)

- **The DU**: the S40 fix lane landed a convergent S-051 (`6ecd6bb4`,
  pushed 18:49:47Z) in the same window as this lane's ad0535d6-lineage —
  both lanes implemented the finding's own fix plan independently, with
  six IDENTICAL test names derived verbatim from the finding's "Tests to
  add" list. The pull-time merge collided on cr_scan.cpp/h, the fixtures
  and the trail files.
- **The union (this merge commit)**: their gate skeleton stands (their push
  won per the SA-9 rule) — `host_fs_hint_` (covers the probed-FAT-alias-on-
  MBR case this lane's Unknown-only gate missed: probe_family's FAT
  heuristic can fire on MBR boot-code bytes, which would have re-opened
  the silent-empty door via an honest-looking engine failure), their direct
  `run_engine_dispatch` per target with the pass's own deep_mode (candidate
  mode's always-deep semantics were wrong for primary table targets), and
  their `table_volumes*` counter names. This lane's contributions kept:
  the M-104 display-name tiers (the FAT boot-sector label via
  `fat::read_volume_label` / their GPT-entry-name tier folded in as tier 2 /
  the `NTFS @ <LBA>` FS marker / the anonymous form last-resort), the
  probe-Unknown-only skip accounting (an identified-family volume with an
  engine open failure counts as SCANNED with 0 files, not skipped — the
  probe's verdict is the volume's identity), and the scanned-only
  `table_target_lbas_` recording (a skipped damaged volume stays
  discoverable — the og's additional-targets semantics, architecture/04
  §3.4).
- **Test consolidation**: their six local tests dropped (this lane's nine
  are supersets: the same assertions + the display names + the stats
  names + the round-trip); their `build_exfat_image` extraction KEPT
  (test_exfat_volume.cpp consumes it via using-declarations).
- **The trail**: the ledger's two S-051 rows merged into one CONVERGENT-DU
  row (their SHA + the union description + the one live file in
  findings/FIXED/); the matrix's two MATCH rows folded; the finding
  files carry DU-merge addenda; the modules' lane-blocks both kept with
  the DU note.
- **Post-merge verification (executed)**: full suite **1136/1136** green
  (the S40 lane's tests + this lane's, consolidated); the evidence probe
  re-run: RUN 2 rc=0 files=4 deleted=2 with the VOLONE/VOLTWO display-name
  roots; clang-format 18.1.8 clean (the S40/thumbnail lanes' newest
  residue swept: cr_ui.h, test_exfat_volume.cpp, test_thumbnail.cpp).
- The board at the merged truth: **53 active (6 SEVERE · 17 MAJOR ·
  24 MINOR · 6 INFO) / 301 ledger data rows / 3 tracked-open** (the count
  recorded for the next conform pass).

## Session-12 S12 lane (2026-10-29 — the continuation audit lane opens; grant: the two repos only, core + corpus)

Task ID: S12/T0 (open — baselines + prompt + territory claim)
Agent: session-12 continuation audit lane (the user's continuation instruction: find what prior sessions missed, remove the fixed things, go more deep, do more comparison on og-codebase and core, mark all remaining features/implementations/functions)

Work Log:
- Baselines at open: **core origin/main `f47dc026`** (pulled; the trail moved SIX
  times during this lane's setup — `620f449` → `21b6c072` (the M-109/M-112-thumbnail
  fix + conform) → `d61474d9` (the N-112 LZH fix) → `aab031ac` (S11-lead waves-2-3,
  12 filings) → `f47dc026` (the S11-lead close)), **corpus `be0cfe9`** (unmoved).
  Board at open (filesystem truth, recounted): **64 active (8 SEVERE · 20 MAJOR ·
  29 MINOR · 7 INFO) / 300 ledger rows / 3 tracked-open** (OQ-S4-3, OQ-S10-1,
  OQ-S10-3). Next free IDs at open: S-055+, M-116+, N-150+, I-034+, A-012+.
- Prompt saved token-redacted: `worklog/session-12-continuation-audit-prompt.md`
  (hygiene-verified: no live-token pattern). Read twice; the §1–§10 SOP plus the
  continuation ask govern this lane.
- **T0 findings at open — board-truth defects discovered during setup, claimed
  as this lane's Wave 1** (all in `diff-to-fix/`, the lane's writable surface):
  1. **Phantom SHA in FIXED-LEDGER rows 179–180** (added by the conform commit
     `21b6c072`): both rows cite fix-commit `5103ecd8`, which does not exist in
     ANY ref (`git rev-parse` + `git log --all`: unknown revision). The real fix
     commit is **`76e2d351`** (verified in-repo: "fix(preview): M-109 + M-112 -
     the og's 200 MB thumbnail eligibility anchor adopted ... the read loop").
     The phantom-SHA truth class again — session-39B repaired 73 rows of it;
     these two are NEW since that census.
  2. **M-112 ID collision**: ledger row 180 assigns **M-112** to the large-IDAT
     StbioSource read-loop defect (the fix lane's §8-duty NEW-FINDING), but
     **M-112 is already taken by the ACTIVE S11B finding** (the CSL 0x44c size
     branch, filed at `b37fabab`, pushed earlier — the SA-9 earlier-push-wins
     rule). The S11-lead lane already honored the collision for their own
     filings (their M-112→M-115, their N-147→dropped/extend-don't-refile); the
     fix lane's row never got the same repair.
  3. **The never-filed finding**: the large-IDAT defect has NO finding file in
     any commit — ledger row 180 cites
     `findings/MAJOR/M-112-stbio-read-cb-single-shot-breaks-large-png-idats.md`
     "(retired to history)" — a path that never existed (the M-088
     fictional-path class). The defect is REAL and code-RESOLVED (the read loop
     + the shared band test at `76e2d351`), but the trail's own §8 duty
     requires the finding artifact to exist.
  **Wave-1 repair plan (claimed):** renumber the large-IDAT defect to **M-116**
  (next free MAJOR per the S11-lead close) with a retroactive finding file —
  full template + Resolution — retired into `findings/FIXED/` per the
  N-134/FIXED-ALREADY precedent; repair both rows' SHA cells to `76e2d351`;
  add the collision note in-file on the active M-112-CSL; then the
  whole-ledger SHA verification sweep (every hex token in FIXED-LEDGER.md
  `git cat-file`-verified; phantoms repaired to real commits) + the counts
  truth re-count. The `21b6c072` message-truth note recorded (unrewritable
  history; the f166440b precedent).
- **Territory claim (coordination contract; pull-before-push every wave,
  earlier-push-wins on convergence):**
  - **Wave 1 (above)** — the bookkeeping-truth repair.
  - **Wave 2 (landing-parity, read-only)** — verification of the three freshest
    landings against the corpus decodes: N-112's LZH entry-chain walk
    (`d61474d9`) vs the corpus decode (FUN_18003db20 probe + FUN_18003dd70 walk
    + the RVA 0x9388c method constants read from og-codebase/RAWSearchDLL.dll);
    M-109's 200 MB eligibility anchor + M-116's read loop (`76e2d351`) vs the
    DRWUI :422425-429 gate (`0xc800000`). The S-054 precedent (a landed fix with
    a 10× cap divergence, filed by S11-lead) makes this wave load-bearing.
  - **Wave 3 (go-deeper, never-walked surfaces)** — the
    `og-codebase/diskhealth` product tree (**818 files, never function-walked
    by any prior lane** — only the DRW-side health grading/NVMe/CSMI surfaces
    were audited) compared against the core's health module; the
    never-dispositioned og-codebase asset families (`language/` 37 QM files,
    `License/` 19, `EuShare/`, `NasConfig/`, `platforms/`, `imageformats/`,
    `log/`, `Resource*/`) against the I-027/I-033 registers — posture
    registration per the mark-all-remaining ask; the corpus's own worklog
    cross-check (never read by any audit lane per the trail's citations).
  - **Wave 4 (mark-all-remaining)** — the core-side citation-gap sweep at the
    CURRENT tree (every src/include/tests file cited-or-dispositioned; the
    prior citation-gap sweep was session 5's, ~200 commits ago).
  - **NOT claimed** (the active fix lanes' territory): all SEVERE/MAJOR/MINOR
    FIX work (the S40-ML lane's SEVERE queue; the S40 minor lane's N-090..N-137
    queue), the OQ dispositions, the S11-lead's closed items, the driver-repo
    residuals (out of this session's two-repo grant; OQ-S10-3 carries them).

Stage Summary:
- Next: Wave 1 (the ledger repair + verification sweep), then the parallel
  deep-read agents for Waves 2–4, then filings + the close.

### Session-39 fix log — N-113 landed (the text school)

**Finding:** MINOR N-113 (five text-format list probes absent: VCF/GPX/OSP/
POS/CHFP).
**Classification:** CONFIRMED-DIVERGENCE (all five probes + walks re-verified
first-hand at the corpus cites before any code; the POS open question
settled by the demanded read of FUN_18001a470: the walk needle is
"ProcessOn Schema File", extent = find + 21 + 0x13, cap 100 MiB).
**Fix:** FMT_VCF/FMT_GPX/FMT_GPX_BOM/FMT_OSP/FMT_POS/FMT_CHFP. Decoded
grammars ported exactly: VCF (the END:VCARD\r\n walk in 0x400 chunks with
the 10-byte over-read, the multi-card BEGIN:VCARD continuation, cap 20 MiB);
GPX (the declaration at 0 or +3, the <gpx u32 window [+0x16,+0x216) — a
root before +0x16 rejects, the og's own window semantics, pinned by test;
the </gpx> walk in 0x200 chunks, cap 100 MiB — the id!=0xa0 branch decode);
OSP (the 10-byte prefix gate, the libopenshot needle, extent = find+14+0xF,
cap 2 MiB, the family's unique REJECT-ON-MISSING — the og's size-0 exit,
ported as ok=false); POS (the needle settled, extent = find+21+0x13, cap
100 MiB); CHFP (the case-sensitive declaration + the case-insensitive
<BiffProject at +0x28 — position-sensitive, pinned by test; the
</BiffProject> walk, cap 100 MiB). The per-chunk find quirk (needles
straddling a 512-byte boundary are missed by the og's chunked find) ported
exactly via chunk_find; the trailing-NUL trim adapted as the bounded
fallback (the M-092/M-093 convention). The case-insensitive og gates carry
canonical-case magics — the exotic-case subset registered (the M-092 class).
**Tests:** Carve.TextSchoolVcfAcceptedAndExtent / VcfMultiCardContinuation /
VcfNoEndTagFallsBackBounded / GpxAcceptedAndExtent / GpxRootBeforeWindowRejects
/ OspAcceptedAndRejectOnMissing / PosAcceptedAndExtent / ChfpAcceptedAndExtent
/ ChfpRootOffAnchorRejects. Suite 1073/1073 (1031 / 42 Win-only skipped / 0
failed); clang-format clean; F8 + self-probe green.
**Trail:** findings/FIXED/N-113 with Resolution; ledger row (270 main + 20
OQ = 290); CROSS-REF-MATRIX cell → MATCH; modules/carve.md; SUMMARY (board
47 active).


---
Task ID: S40-OPEN (session-40 minors continuation lane open)
Agent: session-40 execution lane (the user's "priority to minor first" mandate, continuation over the session-39 N-111 landing)

Work Log:
- T1 done: repos pulled (core `461825d8`, corpus `be0cfe9` — the audited-commit lineage current); prompt saved token-redacted to worklog/session-diff-fix-40-minors-prompt.md; baseline verified GREEN before any change: full build (gcc 14.2, -Wall -Wextra -Wpedantic -Werror -Wshadow) + cr_tests 1052 ran / 1010 passed / 42 Win-only skipped / 0 failed (~102s); clang-format 18.1.8 (the CI pin) installed; hooksPath wired.
- Board at open (filesystem truth, recounted): 49 active = 6 SEVERE · 18 MAJOR · 20 MINOR · 5 INFO + 3 tracked-open (OQ-S4-3, OQ-S10-1, OQ-S10-3). FIXED/ holds N-111 + the two resolved OQ files. Ledger 288 rows.
- Lane plan: the 20 open MINORs one-by-one (classify first, fix with RED->GREEN tests), the 5 INFO ACKNOWLEDGEDs, the 3 tracked-open dispositions, then the MAJOR/SEVERE queue as capacity allows. One finding, one verified fix, one push.

Stage Summary:
- Lane open; baseline green; no findings touched yet.

---
Task ID: S40C-1 (N-142 fixed — the dead CrPreview origin pair)
Agent: session-40-continuation minors lane

Work Log:
- Classification: CONFIRMED (re-verified at c0f69250 before the edit: exactly 4 hits — declarations :887-888 + writes :3584-3585, zero reads repo-wide; the promotion at :3753-3756 re-read running off thumbnail_generate's own arguments; the fce38c3d drop record cross-checked).
- Fix: src/ui/cr_ui.cpp — the CrPreview::src_session/src_node_id pair + the two preview-open writes deleted; the struct comment rewritten to the true addressing (cr_ui_thumbnail_generate's own session/node_id args, ABI-CONTRACT §7i) with the rebase-drop history + N-142 recorded. No ABI change (CrPreview internal).
- Proof: grep census zero hits (the finding's own acceptance); rebuild green; cr_ui_tests 59/59 (incl. ThumbnailSuccessPromotesGradeTo2 + PreviewSessionAndSnapshot); the 34-test CrUi/Preview slice green.
- Trail: Resolution in the file; git mv to findings/FIXED/; ledger row N-142; matrix cell → MATCH; SUMMARY session-40-continuation block added (board 54 active: 7 SEVERE · 20 MAJOR · 27 MINOR · 7 INFO / 306 rows / 3 tracked-open).

Stage Summary:
- First minor landed this lane; queue next: S-053+N-141 (the recommended one-pass), N-139, N-140, N-138, N-149, N-148, N-146, then the INFO acknowledgments.
---
Task ID: S11-R (session-11, the reconciliation lane — the 63-commit push window absorbed; the unique product landed)
Agent: session-11 continuation lead (the fifth parallel session-11 lane)

Work Log:
- T1 at 8365956e: the prompt saved token-redacted + read twice; the
  board-truth defects found at open (M-083/M-102 fixed-but-unretired, the
  duplicate N-129 pair, the 283-vs-281 ledger drift).
- WAVE-1 (board truth, EXECUTED locally): M-083/M-102 retired with the
  ledger placeholders filled (e56baac7); the N-129 collision repaired
  (wave-3's bookkeeping batch → N-135, the SA-9 rule); the b5da834 ledger
  conflict residue resolved; the M-090 180053de0 mis-paste → 9daa787; the
  exact row census recorded. — ABSORBED at push: the remote lanes landed
  the same repairs (f166440b/bc4513db/2cf3f4d0/15279503/23849cac).
- WAVE-2 (the re-triage, EXECUTED locally): five parallel agents, all 54
  true-active findings re-verified (44 STILL-DEFECTIVE · S-050 PARTIAL ·
  N-134 REFUTED-at-filing · I-030 PARTIAL · 4 INFO accurate); the addenda
  drafted. — ABSORBED: their s11-triage-sweep.md (the same 56-row table at
  the same baseline) + their b37fabab addenda batch; N-134 retired by
  theirs as FIXED-ALREADY (same substance — the register rows predated the
  filing; this lane's REFUTED label recorded in the backup branch).
- WAVE-3 (the S10-F queue, EXECUTED locally): the RAWSearchDLL
  infrastructure walk (the 18-row census + 14 MATCH holds + the
  zero-threading/zero-error-propagation negatives); the API-DELTA currency
  pass; the OQ-S8-1 consumer trace (four hops verified first-hand). —
  ABSORBED: their s11-rawsearchdll-infra-walk.md + N-139/N-140 + I-031;
  their s10f-cwindowsfs-consumer-trace.md + M-111 (their browse finding is
  DEEPER than this lane's draft — the ShellExecuteA branch, the enumeration
  impl, the IFSInterface vtable; this lane's S-053 draft DROPPED); the
  S-050 per-chunk-feed addendum (this lane's D-INF-1).
- THE PUSH REJECTION + RECONCILIATION: 63 concurrent-lane commits (the
  parallel S11/S11B/lead/39/40/S12 lanes + the fix wave — M-108/M-109/
  M-110/M-072/M-074/M-077/M-102+M-083/N-093/N-111..N-113/S-051 landed;
  the S12 lane opened; the session-40 minor-first mandate opened). Rebase
  attempted, aborted at the conflict scale; the lane state preserved in
  session-11-local-backup (4 commits); main reset to b6e0a9ef; the UNIQUE
  product re-derived against the new HEAD and landed:
  * M-116 (THE headline — no remote lane caught it): the M-100 re-open.
    The retirement's residue claim ("reachable today through
    cr_ui_preview_metadata") refuted FIRST-HAND at 8365956e AND b6e0a9ef
    (cr_ui.h:1214-1224 no entries field; cr_ui.cpp:3613-3631 m.entries
    never referenced). The archive inventory's ABI half re-opened; the
    M-100 ledger row annotated.
  * N-150 (the API doc-truth batch): every item re-verified CURRENT at
    b6e0a9ef — the false-missing R-84 pair, the doubly-stale R-37, the
    81-vs-83 export count, the 34-vs-38 error-code count, the :345 line,
    the stale-landed API-DELTA rows.
  * The unique addenda: N-131 (bit-512 re-base — N-100's landing took the
    bit), N-132 (the R-31 spread verified current), M-107 (the two nits).
  * I-031's extension: only_common + the A4 phrasing.
  * The re-created findings/FIXED/ sweep: 7 ledger-rowed files retired to
    history per the ledger's own convention.
- BOOKKEEPING INCIDENT (this lane's own, recorded per the truth standard):
  the lane's SUMMARY/README insertion script carried a p/r variable-target
  bug that wrote README content into the SUMMARY path on the pre-rebase
  local branch (the backup branch preserves the corrupted intermediate);
  fixed before the reconciliation landing by restoring from the last good
  state. Lesson: one writer variable per file.
- READ-ONLY GUARANTEE: verified per-commit on the local branch (all four
  commits touch only diff-to-fix/ + worklog*); the reconciliation landing
  touches only diff-to-fix/ + worklog*; zero product files.

Stage Summary:
- Board after S11-R (filesystem truth): 64 active (7 S · 21 M · 29 N ·
  7 I) / 300 resolved ledger rows / 3 tracked-open.
- The lane's recommended fix order: M-116 first (the M-102-pattern
  two-call archive export + N-150's row-state sweep in the same wave).
- The backup branch (session-11-local-backup) preserves the full
  pre-reconciliation lane state for any future forensics.

---
Task ID: S11-R/close (session-11, the reconciliation lane's final sweep)
Agent: session-11 continuation lead

Work Log:
- The final rebase window (c26f77d9's N-142 fix) re-created
  findings/FIXED/ again (the c56481ca sweep's class recurring within the
  hour) — swept once more: N-142's ledger row exists (verified); the file
  retired to history. The convention holds on the landed tree.
- The landed session: 0f9e4345 (the S11-R reconciliation) + this sweep.
  READ-ONLY GUARANTEE re-verified on both: only diff-to-fix/ + worklog*
  touched; zero product files; no conflict markers anywhere.

Stage Summary:
- Board at the landed HEAD: 63 active (7 S · 21 M · 28 N · 7 I) / 300
  resolved ledger rows / 3 tracked-open. The lane is closed; the backup
  branch (session-11-local-backup) preserves the full pre-reconciliation
  state for forensics.

---
Task ID: S40-ML/T1 (S-053 + N-141 fixed — the ReFS emission gate, one pass both directions)
Agent: session-40 main diff-fix lane (tracked-open-first)

Work Log:
- **S-053 (SEVERE, CONFIRMED-DIVERGENCE) — FIXED.** Verified first-hand
  against the corpus before any code: the name builder FUN_18000c2b0
  no-ops on keySize<=4 (:8674-75, `if (0 < iVar4)` wraps the whole
  builder — empty is a LEGAL return); FUN_18000ba90's survive path
  (LAB_18000bbfb -> RefsNodeHeader walk -> the per-attribute emitters ->
  d970 identity/recycle routing) consults the name ONLY in the N-141
  dot-scan — no name-liveness gate anywhere (the lead's own line-by-line
  read, completing the S11 walk's own "confirm no later name gate"
  re-verify note). Fix: the `name16.empty()` half of the emission gate
  removed; `ev.val_len < 0x28` (the og's own minimum-record shape guard)
  stays. Unnamed rows: stored name EMPTY (the og persists empty), full
  attribute walk, content-based grading, identity map participation.
  Display fallback = the CLI human-readable print ONLY (File%06u
  per-print placeholder in print_tree; csv/json/session keep the true
  empty name — the finding's "display layer ONLY, never baked into the
  session" requirement, pinned by the round-trip test).
- **N-141 (MINOR, CONFIRMED-DIVERGENCE) — FIXED, the same pass** (both
  findings name the single-pass fix). The size field decoded to the SAME
  +0x30 both engines read (FUN_18000c060's param_3 out = the found
  sub-object's puVar1[6] = byte +0x30; parse_general_info's gi.size =
  le64(var + 0x30)). Fix: the compound gate after parse_general_info —
  size==0 AND no U'.' -> filtered, counted in stats.zero_size_nodot_filtered
  (new field, the granularity_discards precedent); zero-size-with-dot and
  nonzero survive; unnamed-zero-size filters (the og's memchr skips on
  length 0 — decoded, not assumed).
- **Tests (fail-before/pass-after PROVEN):** 4 S-053 tests + 4 N-141
  boundary tests in test_refs_volume.cpp. RED proof: the engine stash
  rebuild ran the 8 — 6 FAILED pre-fix (the 2 passers are the
  over-filtering boundary guards, green by design both sides), 8/8 GREEN
  post-fix. Suites: full linux-release 1064/1064 (1022 passed / 42
  Win-only skipped / 0 failed); the FULL ASan lane 1064 tests green with
  ZERO reports; clang-format 18.1.8 clean repo-wide.
- **The ASan catch (a real memory bug, fixed root-cause):** the full
  sanitizer lane detonated on Carve_Batch1OgCapsAreTheRegistryRows — the
  N-093 test iterated `for (s = reg; s->ext; ++s)` past kTable's end
  (global-buffer-overflow READ of 8 bytes; the table is count-based by
  contract — cr_signatures.h "one entry per FormatId, order = FormatId";
  every other consumer uses registry_size()). Repaired to the count-based
  loop with the contract comment. This is the test-side defect class the
  ci-local --san lane exists to catch; it had never been run over the
  N-093 landing.
- Trail: both finding files carry full Resolution sections (retired to
  history in this commit per the S11-R-enforced convention — the ledger
  is the index, the Resolutions ride this commit); FIXED-LEDGER +2 rows
  (306 = 284 main + 22 OQ, filesystem-recounted); CROSS-REF-MATRIX: the
  refs B2/B3/B4 cell + the B3 cell -> MATCH; modules/refs.md B5 note
  resolved-FIXED; SUMMARY session-40-MAIN delta block. Corpus citations
  in the code comments: ReFSSearchDLL_decompiled.c :8674-75, :8478-81,
  :8317-37, :8554-8640 (the c060 size-read decode).
- OQ-S4-3: the Windows benchmark (run 40747332987) dispatched at lane
  open is executing; the TTFR transcription lands on its completion.
- Coordination: S-053 was in the parallel SEVERE-first lane's declared
  queue (not in flight — their in-flight is S-052); this lane landed it
  first, their entry drops per the convergence rule. This lane's next:
  S-049 + S-048 (the carve pair), then S-050/S-040/S-054, then M-116+.

Stage Summary:
- Board after this landing: 61 active (6 SEVERE · 21 MAJOR · 27 MINOR · 7
  INFO) / 306 resolved ledger rows / 3 tracked-open. Suites green
  (release + ASan); format clean; the push carries the two- finding fix,
  the ASan test repair, and the full trail.

---
### Session-11D W1 closed (pushed at 1c4a106c after five hot-window merge cycles)

The wave's landings, all verified on the merged tree (1146/1146 green,
format clean, the evidence probe re-run):
- **S-051 FIXED** — the CONVERGENT DU union with the S40 lane (their
  host-hint gate + stats names; this lane's display names, skip accounting,
  scanned-only reconciliation). RUN 2: rc=0 files=4 deleted=2.
- **M-104 FIXED** — the display-name tiers riding the union.
- **A-012** — the CONVERGENT record (the S11 lane's e4273f2b repair stands;
  this lane's fail-before execution + root-cause trace in the finding file).
- Trail reconciled through the hot window: the DU rows in the ledger, the
  folded matrix rows, the addenda in both finding files.

**NEXT (the lane's declared SEVERE order): S-049** — the og's universal
carved-naming contract (`<PREFIX>_%06d.EXT` + the content-derived
prefixes). The label primitive (fat::read_volume_label) and the volume-root
mechanism from W1 compose.

## Session 11C — T3: S-052 FIXED (the LDM span recovery basis)

Task ID: S11C/T3
Agent: session-11 fix lane

**The landing (`f4e500de`):** the finding's full architectural plan —
(1) the tree gains per-root basis ids (`ResultTree::set_root_basis`;
roots only, descendants read through the parent walk; persisted as
node-flags bit10 in the tree record), (2) the scan controller keeps the
virtual volume devices past run()'s return (`ldm_devices_`/`ldm_layouts_`
— the loop's old stack device replaced; roots stamped with index+1),
(3) the SafeWriter resolves each item's read source through the borrowed
`LdmBasisTable` (basis 0/unresolvable = the absolute source, whose reads
then fail honestly per-extent), (4) the R7.2/F12 overlap refusal maps
layout-relative extents to PHYSICAL member spans first
(`LdmVolumeDevice::physical_spans`: exact spanned/striped, conservative
every-copy/every-column for mirrored/RAID-5), (5) the session persists
both halves (REC_LDM_LAYOUTS type 10 + the root stamps;
`load_session_summary` surfaces the layouts; the resumed controller
adopts them for re-save truthfulness), and (6) the UI arms the session's
devices lazily at the first basis-aware enqueue/preview/thumbnail
(Windows: PRIVHEAD-GUID disk enumeration; else local-self — missing
members fail loudly, never wrong-disk bytes).

**Verified:** `LdmRecovery.*` (7: span byte-exact WITH the executed
fail-before — the same extent read against the absolute source serves
0x11 where the scan saw 0x22; stripe round-trip; RAID-5 reconstruction;
overlap true+false positives; simple identity; preview==recovery; the
session round-trip) + `Orchestrator.DynamicDiskScanStampsBasisAnd-
SessionRoundTrips`. Full ctest **1121/1121**; clang-format clean;
GAP-S6-15 CLOSED in CORE-GAP-LIST.md; ABI-CONTRACT + crsf-format.md +
PARITY-DELTAS (D-106/D-107) + modules/ldm.md updated in the same landing.

**Honest boundaries (registered):** the cross-disk
member-on-destination overlap case inherits the absolute-basis check's
same-disk convention (spans on other member disks = disjoint hardware);
a resumed incomplete deep stage re-runs the LDM loop (fresh roots — the
pre-existing resume root-duplication class, unchanged here); the
Windows re-arm enumeration compiles on the win lane (win_syntax_check
green) with the multi-disk byte-exactness proven at the core level.

**Process notes:** the parallel fix lane landed M-108/N-111/N-112
mid-flight (this lane's independent M-108 implementation dropped at the
pull — the no-duplication convention); the FIXED/ convention amendment
(user instruction: fixed findings live in the fixed directory) is
recorded in the ledger's preamble.

Task ID: S41 (session-41 continuation audit lane — the consolidated record; opened at core c7fa527c / corpus be0cfe9, RECONCILED at push over TWO parallel windows, closed at the reconciled push)
Agent: session-41 continuation audit lane (the user's "second session" directive: find what prior sessions missed, remove the fixed things, go deeper, mark all remaining)

SESSION RECORD (the pre-rebase states preserved at `s41-local-backup` and in the reflog;
the two reconciliation rebuilds were forced by the trail's hottest windows).

WAVE 0 — LANE OPEN: baselines (core c7fa527c, corpus be0cfe9); prompt saved token-redacted;
the 56-vs-55 board-truth defect found at T1 (the S11-final truth commit 14cf8064's own
arithmetic — the MAJOR half never decremented at M-110's retirement).

WAVE 1 — BOARD RE-TRUTH (pinned at c7fa527c): filesystem truth 55 (7/17/24/7); the ledger's
296 rows verified TRUE (274 main + 22 OQ — the single-regex count had missed the OQ rows);
the FIXED-archive convention conformed to the session-39 user amendment locally — later
CONFORMED BACK to the remote's earlier-pushed S11-R purge at the reconciliation (the
conflict recorded as an OPEN user decision in the matrix's S41 block); N-111's ledger SHA
cell filled (461825d8); OQ-S10-2's row carries the N-138 refutation note.

WAVE 2 — THE REMOVE-THE-FIXED BATTERY (pinned at c7fa527c): 54/55 then-active findings
STILL, one PARTIAL (N-132's TxtViewFiles leg landed with M-108/aac75bba; re-scoped by
addendum to row 45 + R-31's propagated clause — convergent with the S11-R aggravation
addendum). M-076's apparent TRIM flip checked and REFUTED (the hits are the health
facet's smart-report struct; cr_ui_disk_info still lacks the fields).

WAVE 3 — THE DEEP-COMPARISON FILINGS: the never-cited-artifact census (283 docs, 260
cited / 23 never) opened the genuinely unaudited territory; the three never-cited
RAWSearchDLL family censuses cross-checked against the 122-member enum → the 57-key
byte-level registry re-diff (53 landed, 4 absent) →
- **M-117 (MAJOR)** — the Outlook Express Dbx analyser entirely absent (the og's 3-key
  registry :2192-2232, the 4-GUID identity probe FUN_180016740:17509, the u32@+0x7C size
  pass FUN_180016950:17622 — the session-3 "MISSING (N-026)" residue planned at
  S3-CARVE.md:70 and never filed);
- **N-152 (MINOR)** — the DXF 999-comment entry key (the u16 0x3939 binding :2722-2733 +
  the probe's leading-comment acceptance :42240+; the core keys "  0\r" only);
- **I-034 (INFO)** — the never-cited artifact register + the two completeness censuses
  (the 283-artifact citation census + the 750-entry og-file census: NO undispositioned
  product surface).
The never-cited EUDCPEPM artifact dispositioned (the Todo-Backup sibling driver family);
burn → N-134's family; UIUX/32 → S-050's addendum; UIUX/16's AI layer → D-26.

WAVES 4-5 — THE TWO PUSH-TIME RECONCILIATIONS (the hottest windows of the trail absorbed):
window 1 (20+ commits: the S11-R/S11-lead/S11B/S12/S40 lanes' filings + the S-051/N-112/
N-113/N-142/M-109/M-116 fix landings) forced the first renumbering set (M-112→M-117,
N-142→N-151, I-033→I-034 — each ID concurrently consumed on the remote); window 2 (the
S-053+N-141 emission-gate landing e10ec381 + the remote's own N-150→N-151 renumber
6f7d33e8) forced the second (N-151→N-152). Both resolved per SA-9 earlier-push-wins; the
rebase conflicts resolved by the rebuild-on-remote method; the N-132/S-050 addenda
re-appended after the remote's own convergent addenda each time; the ledger row
improvements kept; the FIXED/-dir restorations dropped (the remote's purge wins; the
convention conflict recorded as the open user decision).

BOARD AT THIS LANE'S CLOSE (the reconciled filesystem truth, re-counted post-rebase):
**64 active (6 SEVERE · 22 MAJOR · 28 MINOR · 8 INFO) / 307 resolved rows (the S41
filesystem re-count at the close: 285 main-pattern + 22 OQ-prefixed, all unique — the
remote's own "300 → +2 = 302" arithmetic drifts by 5 against the table, the N-129
class on the remote's ledger claims; recorded for the next truth lane) / 3 tracked-open
(OQ-S4-3; OQ-S10-1; OQ-S10-3 — all three unchanged this lane).** Next free IDs: S-055+,
M-118+, N-153+, I-035+, A-012+.

READ-ONLY GUARANTEE: verified per-commit — the lane's own commits touch NOTHING outside
diff-to-fix/ + worklog* (the product-code commits visible in the window are the parallel
FIX lanes' landings, not this lane's). The product trees of BOTH repos untouched by S41.

Handoff — the highest-value next moves from THIS lane's product:
(1) **M-117's fix** (the Dbx analyser — fix-plan step 1 names the two FUN bodies to decode
    first: FUN_180042b30's block computation + the Analyse virtual; the grammar is
    otherwise decoded; three registry rows + validate_dbx + the five tests are scoped);
(2) **N-152's landing** (one u16 key row + validate_dxf's leading-comment branch);
(3) **N-132's remainder** (row 45 + R-31's propagated clause + the config-model
    dispositions — docs-only);
(4) **the FIXED-archive convention decision** (the open user question: the session-39
    amendment vs the S11-R purge — one sentence settles it; the ledger rows carry both
    SHAs and the history paths either way);
(5) the standing order (S-052 first — silently wrong LDM bytes, now the board's top
    SEVERE after S-053's landing; the carve school-2/3 children).
### Session-39 fix log — N-133 landed (the engine-side on-disk volume labels)

**Finding:** MINOR N-133 (the engine-side labels never decoded/surfaced; the
ext4 half-decode dead; M-079's image-stem leg baking the refuted premise).
**Classification:** CONFIRMED-DIVERGENCE with TWO corrections recorded: the
corpus re-verification (a) confirmed the NTFS/ext4 decodes exactly as cited
(NTFSSearchDLL :647-676 + FUN_18003f3a0 :50963-50980; algorithms-v2 SS2
step 5), (b) corrected the exFAT half — the label is the ROOT-DIR 0x83
dentry (spec 7.2: EntryType@0, CharacterCount@1, UTF-16@4 — the layout the
M-079 spec-repair in test_fixtures.hpp pins), NOT the "+0x80" boot field
the finding inferred; the FAT +0x2B field landed as the format's standard
structure.
**Fix:** orchestrator::probe_slice_volume_label (cr_vollabel.cpp — the
family detection + the four decoders; the NTFS one through the REAL
NtfsVolume::open + read_record(3) + fixup + enum_attributes, the 0x60
resident value wide->narrow with the N-016b U+FFFD rule). Wiring: the
decode rides run_engine_dispatch's tail into EngineRun.volume_label (the
og's architecture — the engine layer decodes at volume init, image scans
included); the main pass replaces a DERIVED label
(ScanConfig.volume_label_derived — set only by fill_volume_label's
image-stem leg) with the engine label and surfaces ScanStats.volume_label;
the rrecov auto-save + the cr_ui auto-save apply the corrected precedence
(--label > engine label > image stem); the ext4 engine's dead
Volume::volume_name now surfaces through Ext4ScanStats.volume_label;
M-079's refuted-premise comment replaced with the corrected precedence
note. PARITY-DELTAS D-112 registers the remaining divergence (a label-less
image keeps the stem — the deliberate last-resort).
**Tests:** VolumeLabel.Ext4FromSuperblock / Ext4LabellessYieldsEmpty /
FatFromBootSector / FatLabellessYieldsEmpty / NtfsFromVolumeRecord (the
full bootstrap path) / CallerValueWins / UnknownFamilyLeavesFallback.
Suite 1086/1086 (1044 / 42 Win-only skipped / 0 failed); clang-format
clean.
**Known residual (recorded in the Resolution):** the HFS+ half (the
volume-name key (-2) catalog-record walk) not landed — no corpus-decoded
gate, no test in the finding's own plan; the probe returns
ERR_UNSUPPORTED for HFS+ (the stem fallback, unchanged behavior).
**Trail:** findings/FIXED/N-133; ledger row (271 main + 20 OQ = 291);
CROSS-REF-MATRIX cell → MATCH; SUMMARY (board 46 active);
PARITY-DELTAS D-112.

### Session-39 fix log — N-132 landed (the display-config data layers + the API-DELTA truth repair) + the merge-window red repaired

**Finding:** MINOR N-132 (API-DELTA rows 44-45's false "landed" claims for
TxtViewFiles/SumaryFileType/PriorityDisplayType).
**Classification:** CONFIRMED-DIVERGENCE at the current tree, NARROWED: the
parallel M-108 fix landed TxtViewFiles for real (row 44's claim became
true in the interim); SumaryFileType + PriorityDisplayType remained absent
(grep re-verified) — row 45 was still false.
**Fix (fix-the-cause, the M-108 data-layer precedent):** both tables
landed as query-layer data + public lookups —
query::summary_family_for_extension (the 11 unification groups, the ini's
order verbatim) + query::type_display_order / type_display_category_name
(the 5-category/35-ext alias ordering, read DIRECTLY from
og-codebase/config/PriorityDisplayType.xml this session). The layering
registered as PARITY-DELTAS D-113 (the og consumes both in DRWUI's
presentation; the core lands them as query data — the D-107 lineage).
API-DELTA row 45 repaired to the truthful split (usefullostfiles stays
the OOS-UI row; the landed halves carry real cites) — R-31 and the
API-DELTA now agree.
**Tests:** ConfigDisplay.SummaryFamilyUnificationGroups /
SummaryFamilyUngroupedStandsAlone / SummaryFamilyNullAndEmptySafe /
TypeDisplayOrderVerbatim / TypeDisplayOrderUnrankedReturnsFalse /
TypeDisplayCategoryNamesVerbatim. Suite 1077/1077 (0 failed);
clang-format clean.
**Hygiene repair (the merge window left main RED):** test_fixtures.hpp's
table::crc32 calls lost their transitive include in the S41 merge-window
landings — the cr/table/cr_table_wire.h include restored (verified red
at origin/main HEAD with my changes stashed; the repair is in this
commit).
**Trail:** findings/FIXED/N-132; ledger row; CROSS-REF-MATRIX cell →
MATCH; API-DELTA rows repaired; PARITY-DELTAS D-113.

**Session-39 addendum (the N-132 convergence note for the S40B lane):** the
S40B lane's territory claim (7320a2f6) lists N-132 among its unclaimed set;
this lane's N-132 fix landed first (complete: the data layers + tests + the
truth repair). Per the convergence convention S40B's N-132 pass becomes a
verification of this landing (the N-111 precedent shape). The
test_fixtures.hpp crc32 include: convergent repair by two lanes (theirs
7320a2f6-lineage, mine this commit) - one include kept.
