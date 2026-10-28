# Session-8 verification records (2026-10-28, audited `35adcb9`; corpus `be0cfe9`)

The session's five read lanes produced context sheets in the working scratch
area; this file records the load-bearing verification RESULTS the filed
findings rest on (each re-verified first-hand by the lead before filing).

## 1. Landing verification — W1a/W1b/W1c/thumbnail (the B lane)

**Verified TRUE (the load-bearing claims):**
- W1b cancel: the hook fires per attached node (cr_records.cpp:544), cancel
  checked first even with fn==NULL (cr_ui.cpp:2707-2708); `*out` untouched on
  abort (:2745-2748); `ERR_CANCELLED` never flattened into `SESSION_FORMAT`
  (cr_scan.cpp:2113-2118: `if (rc2 == ERR_CANCELLED) return ERR_CANCELLED;`)
  — pinned by test_session.cpp:1554-1565.
- W1c: cancelled runs never auto-save (shim early-return cr_ui.cpp:1989-1990;
  CLI rrecov.cpp:1640); naming byte-exact vs the corpus samples
  (cr_crsf.cpp:363-404; the sanitizer is the og's 9 chars + 2 hardening
  additions); atomic write tmp+rename (:2450-2478); one event + one file.
- W1a: two-call pattern (cr_ui.cpp:2761-2764), byte-sorted caller-owned rows,
  light-record summary (peek_record; REC_SESSION_STATE/REC_VOLUME_INFO only),
  ERR_OPEN_FAILED on directory failure, TRUNCATED-with-summary-intact.
- Thumbnail (05e6786): both-axis 256 px (cr_thumbnail.h:60), dims-first bomb
  guard (cr_thumbnail.cpp:766-910), truncation disclosure on Ok (:994-998),
  the I-015-lineage placement does not contradict the ABI N/A-by-layer row.

**Divergences found** (filed): N-090 (list scope/row shape), M-079 (label
never filled), N-091 (registration gaps), M-080 (the budget), N-092 (the
refusal matrix/cache). Full parameter tables in the lane's context sheet;
every divergence re-verified first-hand by the lead (the greps/reads cited
in the finding files).

## 2. S-007 batch-1 key verification (the C-part1 lane; 12 representative formats re-read at the og registration sites)

27/27 batch-1 magics byte-exact (MID/OGG/RM/MXF/DMG/X3F/MRW×3/WPG/WPD/PSP/
SIT/GZ/APE/AU/AIFF/HLP×3/ICO/CUR/WMF-PL/WMF-STD/TTF/MDB/TAR/EPS/AI/DXF/3GP).
The EPS/AI key-ownership split is deliberate and documented (both og keys
present: `C5 D0 D3 C6` = FMT_AI; u16 `%!` = FMT_EPS — the census's "DB
chain" label superseded). Bounds: WPG/MDB carry the og ctors exactly;
MID/PSP/SIT/GZ deviate (superset direction) → **N-093**.

## 3. NTFS origin-LCN carrier verification (the C-part1 lane)

All contract parameters present at `35adcb9`: main-walk stamp
(cr_mftwalk.cpp:893-901, `.front()` + `u32()` truncation + sparse→0xFFFFFFFF),
GUID-dump stamp (:1488-1494), resident→0 (zero-init field, guard
`!data_runs.empty()`), nflags bit-8 nonzero-gated persistence
(cr_records.cpp:172-173/:268-269/:460-463), clone carry (cr_node.cpp:74), the
three tests (test_ntfs_volume.cpp:1497-1572/:2967-2986,
test_session.cpp:1177-1231). `eac8e1d` on main; the trail's `1285be3` was the
parallel lane's rebased twin (content covered).

## 4. FIXED-truth spot-check (the C-part2 lane; 10 findings sampled)

CONFIRMED-FIXED (9): A-002 (the real Microsoft LDM GUIDs byte-exact at
cr_table_wire.h:91-94 + the de-self-referenced tests), A-005 (sector-sized
heap vectors + 4Kn tests), S-023 (the grading engine end-to-end), S-031
(both carve emit sites stamp Deleted + scan time), S-037 (FH11 constant
predicate + CPT pointer-chase), M-013 (the dual checksum + unconditional
deleted-set gate), N-087 (the CP437 table), A-011 (the BE reads + the
parenthesized page-count gate), N-059 (the 24-byte append block + engine
emission). **PARTIAL (1): M-018** → **M-071** (the rrecov.cpp gates were
dropped by the N-064 reconstruction; git archaeology in the finding).

## 5. og-codebase asset sweep (the A lane)

Every asset maps to a register/exclusion (the disposition table is I-027's
sweep summary). Five ground-truth contracts decoded for the first time (the
2 GiB free-quota registry family + 500 MB clamp; json_raw's
"TXT;HTML;LOG;H;CPP;MM;" shield split at DRWUI:499195-499228; the Aliyun
batching caps; the DiskHealth schtasks ONLOGON/HIGHEST autostart;
filter_generalraw's 65-extension value) → **I-027**. auto.txt's true
semantics (the FVE key snapshot: DRWUI creates `\\.\Pipe\DRWBitlocker`
at :430715, deletes+rewrites auto.txt at :430781-430798) → **N-097**.

## 6. The two NEVER-CITED driver docs + MOM decode candidates (the E lane)

Identity-IOCTL/wire MATCH table (11 parameters), grammar/PID-token/handshake
MATCH table (12 parameters) — the residuals are N-096 + the two
DRIVER-REPO-ITEM handoffs (worklog-recorded for the rda-driver lane). The
DeviceManager offline-OS decode (IsBootVolume at :51303; the hive profile at
:62034-62410; the MountedDevices mapping; Select/Current resolution; the
installer sweep :62854-62920; module loaded by name per the s25 evidence)
→ **M-082**. CWindowsFS decoded (factory :10650-10797; ctor :14025; the
mounted-vs-raw dispatch) → **OQ-S8-1**. CORRECTIONS.md 1-9 absorption
verified (9/9 in the trail; propagation tails → N-095/N-096).

## 7. Remaining-feature evidence packages (the D lane)

All 9 R-rows verified against the current tree (the greps + reads are in the
finding files): R-22→M-072, R-14→M-073, R-41→M-074, R-65→M-075, R-86→N-098,
R-63→M-076, R-34→M-077, R-93→S-040, R-88→M-078; the batch-2 state → S-041
(the per-format absence greps re-run by the lead: zero FMT_ rows, zero
raw-sign entries on both sides) + M-081 (the CRW keys exist but mislabeled
and PRD-gated — cr_registry.cpp:709-738 + cr_analysers.cpp:3057-3088 read
first-hand).
