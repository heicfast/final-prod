# modules/session.md — session module audit verdicts (CRSF vs .rsf)

## S12 addendum (2026-10-29 — the session-2 deep-dive)

New active findings on this module: **A-013** (ACUTE — the CLI resume-truncate:
"wb" checkpoint sink opens on the resume target before the "rb" read);
**M-131** (the in-run checkpoint is truncate-in-place — the N-014 shape cr_ui fixed);
**S-057** (FIXED session-51 — the og auto-saves stopped scans into the shared
completion handler; the core never auto-saved a cancelled run. The auto-save now
runs from BOTH the completion and the ERR_CANCELLED branch — the factored
autosave_if_due_ helper, the S-044 gates identical, the CLI parity leg taken). Verified-MATCH: auto-save
gates/naming/sanitizer, load identity re-check, save/load progress, the LDM basis
re-arm (S-052), no-engine-state-persisted parity, the og's import path read-only.


Reference: the .rsf grammar per `re-workspace/artifacts/SaveScan/rsf-format--scan-session.md`
(+ `ifileproperties-part1/2/3`), auto-save naming/sanitizer, CORRECTIONS folded.

Core (audited `67193b6`): `src/core/session/` (721 LOC) + `include/cr/session/` +
`docs/crsf-format.md` + `tests/unit/test_session.cpp`.

## Verdicts (D12 field-equivalence — own byte format allowed)

- **Chunk framing** — `M`: core replicates the og algebra exactly — header
  `(len−1)&0xFFF | 0x8000` when deflated, payload ≤ 4 KiB, zlib level 5,
  deflate-only-if-strictly-smaller, per-chunk raw fallback on inflate failure
  (`cr_crsf.cpp:20–62` vs rsf-format §3).
- **Record inventory** — `M` at field-equivalence: og record-2 (volume name[100],
  deviceType, SYSTEMTIME, stat1/stat2, flags) ≈ core `volume_info` (fs_kind, slice
  offset/bytes, sectors, label[32], scanned/elapsed, flags + v2 identity tail with
  model/serial/boot_hash — the og's identity role is served by the boot-sector copy in
  record-3; core's CRC-of-boot u32 + serial/model is an equivalent-strength identity
  card, SESS-2 verified at resume); og record-3 (partition struct + 5400B volume blob)
  ≈ core `partition_context` (+ slice placement for F-1 rebasing — richer); og record-1
  (12B state) ≈ core `session_state` (stage_done_mask, counters, error-map ranges —
  richer and resume-critical); og record-4 tree ≈ core tree records.
- **Tree grammar** — `M`-note: og persists per-FS IFileProperties field orders (ReFS
  u32 anchor, ZFS name-len-first, CRAW unconditional runlist, Btrfs +0xB5 switch,
  HFS+ named fork class); core persists a normalized node + codec/psize-tagged extents
  + resident_data blobs (v2 flags). Field-equivalent (every og field has a core
  counterpart; core adds codec/psize which the og's runlist hex form lacks — superset
  for compressed streams). The og's GetType()==0xB never-saved rule ≈ core's
  role-tagged roots (G-07 inverse: core saves roots explicitly — documented
  improvement for resume role recovery).
- **Deleted flag / FLAGS semantics** — `M`: og FLAGS 0x20=deleted / 0x10=has-children;
  core carries DeletedState (richer: Active/Deleted/PartiallyOverwritten/Damaged) +
  node flags — equivalence at the deleted bit, superset beyond.
- **Progress semantics in-session** — `M`-note: og record-4 embeds the save/restore
  progress formulas (0x28 + processed×0x32/(total×2), restore 40+50·done/(total+done));
  core's session_state carries stage bits + counters and the orchestrator computes
  progress from live telemetry — the resume-progress UX differs mechanically (core
  recomputes; og replays stored fractions). No user-visible gap identified beyond the
  visualization surface (S-009).
- **Auto-save naming** — `NA` for the engine (CLI/UI-level; core's save-on-cancel uses
  its own naming; the og's `auto_<volume>_<ts>.rsf` + `/ * : ? " < > |` sanitizer set is
  replicated in the writer's sanitize_name for recovered filenames — verified `M` there).
- **S36 lifecycle** — `M`: save/load/resume with identity verification, mask-⊆-configured
  integrity check, torn-tail clamp, skip-forward resync, legacy fs_kind-12 remap;
  configured-mask preservation (BENCH-F2, G-07).
- **Hardening beyond og** — `I` (grouped I-005.7): 64 MiB record bomb cap (P1-7),
  torn-tail salvage (BENCH-F1), UNIT-step resync (F10).

## Findings index

| ID | Severity | One-line |
|---|---|---|
| — | — | No module-specific findings; hardening rows in I-005 |

## Test gaps (test debt, no finding)

64 MiB bomb-cap path, 65536-record guard, legacy remap, "Restored" synthetic root path.

---

## Session 2 re-audit (audited commit `5d6da5f`)

- **S34/D12/S36 — N-014 (MINOR):** stage checkpoint is NOT atomic.
  `cr_ui.cpp:1131` opens via `_wfopen_s(&f, w.c_str(), L"wb")` which
  TRUNCATES the existing file to zero on open. `emit_checkpoint_()`
  (`cr_scan.cpp:418-433`) does `seek(0)` then writes a fresh container
  header + records + finalize. `Writer::finalize()` (`cr_crsf.cpp:172-200`)
  patches `record_count` and per-record `chain`/`crc` fields IN PLACE via
  seek+write; no `.tmp` + atomic rename. If the process is killed between
  `fopen("wb")` (truncate) and `finalize()` completing, the checkpoint file
  is torn. The previous good checkpoint is GONE (truncated). The og's
  pattern writes each auto-save to a FRESH timestamped file
  (`auto_<volume>_<MM>_<DD>_<HH>_<MM>_<SS>.rsf` per
  `rsf-format--scan-session.md §5.5`), so a torn new file leaves the
  previous auto-save intact. The torn-tail salvage (`cr_crsf.cpp:240-276`)
  mitigates the worst case (recovers the indexed prefix); the atomic-rename
  fix preserves the previous good state on top.

### Session 2 findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| N-014 | MINOR | Stage checkpoint not atomic (truncate-then-write; previous destroyed on crash) |
## Session-2 update (2026-10-25, core `e93decf`)

- **N-016b filed:** core `.crsf` stores the U+FFFD-substituted name where the og `.rsf` keeps
  the original on-disk bytes (substitution at scan vs display/restore) — add a versioned raw
  name field for true round-trips of invalid-UTF-8 names.
- **S-009 remainder:** the disk-map session record (downsampled copy, versioned) is still
  absent — resumed sessions recompute at fixed 64 KiB cells.
- **M-018 interplay:** the junk-filter policy choice determines what sessions persist (hide
  at emission vs presentation-only) — register the choice before the session format gains
  any filter-state fields.
- Session lifecycle/crash-resume verdicts unchanged (S2-F sweep re-confirmed the og's
  checkpoint cadence claims).

---

## Session-5 addendum (2026-10-26, audited `7cf6754`)

- **Resume re-validation** — **FIXED (DFX20 / M-042, findings/FIXED/)**: lost::revalidate_candidate re-reads every restored candidate's start LBA through the CURRENT disk at both device-bearing surfaces (load_session_summary's revalidate_disk param + run() start on whole-disk-basis resumes) and DROPS stale records — the og's PRD:20748-20842 semantics. REC_PARTITION_CONTEXT v2 carries the per-record geometry payload (bps/SPC/hidden/flags, length-versioned); REC_DISK_LAYOUT (type 8) is the partinfo.dat equivalent (layout_changed surfaced at resume, never enforced); discovery-hit checkpoints now forced ahead of stage boundaries (sink-gated). Executed fail-before + 6 tests (test_resume_revalidation.cpp); suite 685/685.
- **Record richness** — `N-049`: persisted candidates lack bps/SPC/hidden/
  size-source/evidence fields the og's 0x448-byte record carries.

## Session-5 audit (audited commit `7cf6754`) — rsf-record4 + ifileproperties-part3 read for the first time

- **M-047 (MAJOR, NEW):** stats round-trip asymmetry — the writer persists 4×u64 root
  stats and the reader discards them (`cr_records.cpp:247-250`); `refresh_stats` is
  direct-children-only vs the og's persisted-and-trusted 40-byte recursive accumulator
  image (rsf-format:285, 318-320).
- **M-050 (MAJOR, NEW):** ~15 og wrapper/node fields (hidden-root bit, flag_72/73 grading
  bits, 0xFFFF sentinels, f_71 enum remap, root roles, fixup) have no core carrier.
- **N-058 (MINOR, NEW):** load validation deltas (forward parent re-parent, bad-marker
  light-node, short-count OK, u16 name-length wrap) vs the og's per-node checks; the
  core's CRC32/F10/bomb-caps registered as a superset (I-022).
- **N-059 FIXED (s31 — DFX31):** the per-FS secondary payload now round-trips — FileProperties carriers (btrfs_id_a = the owning subvol root id, btrfs_generation = the inode item's qword, zfs_secondary_flags = ZPL_FLAGS low u16, hfs_secondary_flags = the catalog flags, hfs_type_id = the Finder type; og semantics census-level, id_b never invented) + the nflags bit-6 fixed 24-byte append-only block; engines emit at every site. 3 round-trip tests, fail-before executed. (was: btrfs secondary ids A/B + generation/transid qword and zfs
  secondary flags unpersisted; ZfsDataRunlist flattening is a functional match.
- **N-060 (MINOR, NEW):** save_session has no progress/cancel contract (og 40%→65%
  per-node + per-child IsCancelled).
- **N-061 (MINOR, partial):** no session auto-naming (`auto_<label><M><D><H><m><s>`).
- **N-066 FIXED (s31 — DFX31):** the identity card's record-3 parity —
  the v4 tail (firmware/WWN/interface + the 512-byte boot snapshot),
  the scanner capturing the sector beside the hash, the PURE enumeration
  decoders. (was: no firmware/WWN/interface fields, CRC32 boot hash only)
- S-031 cross-module: carved nodes round-trip Active (the deleted flag's producer-side
  gap is carve's; the carrier exists).
## Session-6 rows (deep continuation, 2026-10-26 — the never-cited SaveScan artifacts consumed)

First citations this session: `rsf-record4--tree-node-grammar.md` + `ifileproperties-part3--btrfs-zfs-raw-generic.md` (evidence: `evidence/decompiled-quotes/s6-session-format-record4-and-ifileproperties.md`).

| ID | Severity | One-line |
|---|---|---|
| M-041 | MAJOR | Carved files: og flags every one deleted (FLAGS 0x20 constant, 12,147/12,147) + scan timestamp + validator payload; core emission sets none — sessions round-trip the misstatement (Active + zero dates) |
| N-049 | MINOR | Identity card: og record-3 has firmware/WWN/interface + full 512-byte boot snapshot (re-materialization); core has model/serial/CRC32 only |
| N-050 | MINOR | Node fields: no type-registry ID slot, no per-FS secondary payload slots (Btrfs gen/transid, ZFS flags, HFS+ mode/type — parsed then dropped) |
| OQ-S6-3 | tracked | CRAW reader +0xF4 boolean semantics; anchor-B consumption; checksum validation on load |

MATCH register (verified equivalent): chunked-zlib framing, marker-byte nodes, name/size/times/
attrs/flags, all four extent encodings, RAW name/format-id/anchor-A. EXTRA-IN-CORE: CRC32+
skip-forward container, Confidence grading, allocated_size, changed-time, stage-mask counters.

## Session-9 verdict (2026-10-26 — M-047)

| ID | Verdict |
|---|---|
| M-047 | **FIXED** — refresh_stats is recursive (the og's +0x58 accumulator semantics; iterative DFS, no depth risk; S-032 inner-entry rows count as tree rows); the reader stores the persisted 4×u64 root stats (the "reader stores the image directly" trust model, rsf-format :318-320); the load-time recompute runs only on partial loads (corruption salvage). 3 fail-before tests. |

---

## Session-11 fix row (2026-10-26, DFX11 — M-050 + N-058 closed, one loader change)

**M-050 FIXED** (findings/FIXED/M-050): carrier audit re-scoped post
S-023/M-051/M-047 — grade+overlap, provenance, root stats, short names
were ALREADY carried (the schema debt this finding predicted was paid by
those landings); now pinned by Session.GradeRoleAndProvenanceRoundTrip +
Session.OldFormatStillLoads (the forward-compat guard). Roles persist as
u16 strings (richer than the og's root-role u32 pair). Deliberate
NOT-PERSISTED, OQ-tracked: the hidden-root bit (no in-memory concept),
f_6c/f_68 sentinels + f_71 enum (undecoded in the corpus).

**N-058 FIXED** (same change, per M-050's plan step 2): (a) forward
parent references deferred + resolved post-loop, the never-resolving
count surfaced via the new optional deserialize_tree out-param
`reparented_out`; (b) marker outside {0,1} fails the record (never
normalized to a light node); (c) c.fail alone fails the load (a degraded
final node no longer loads as OK); (d) name AND full-node role lengths
bounds-checked BEFORE allocation (the hostile 128 KiB-per-node resize
gone). 6 tests; suite 602/602.

## Session-17 fix row (2026-10-26, DFX17 — N-078 closed + the leak baseline repair converged)

**N-078 FIXED** (findings/FIXED/, filed+fixed in-session — NEW-FINDING
from the baseline leak repair; renumbered N-077→N-078 after the parallel
session-22 lane filed the leak half under N-077 and landed first): the truncated-carve_note exit was the
ONLY corrupt-record break that left `c.fail` unset — a record whose
carve_note length overstates the remaining bytes loaded the container
as OK (silent partial load, violating N-058's own failed-stream rule).
One-line root cause: `c.fail = true` before the break (the pattern every
sibling site already used). The LeakSanitizer baseline red (FileProperties leaks on the five pre-attach
corrupt-record exits) was repaired CONVERGENTLY: this leg's PropsGuard and
the parallel session-22 lane's PropsRef (98b7d13, N-077) are functionally
identical; theirs landed first and stands — this leg's dropped in the union
per first-landing-wins. Session.CorruptRecordEarlyExitsReleaseProps drives all five
paths under the sanitizer gate (fail-before EXECUTED: LSan red on main,
case (4) returned OK=0). Suite 614/614 + 21/21 UI + ABI green.

## Session-17 fix row 2 (2026-10-26, DFX17 — N-029/N-016b closed)

**N-016b FIXED** (findings/FIXED/N-029; N-016a was the parallel DFX12
lane's): the .crsf container now persists the ORIGINAL on-disk name
bytes for raw-byte-engine names whose decode introduced lossless
mappings — nflags bit 32 (u16 byte-length + bytes, N-058/N-077-class
bounds-checked). FileProperties.name_raw rides from the ext4/Btrfs
engines (populated only when text::name_has_raw_band — the E000..E0FF
band where mapped-byte and genuine private-use codepoint are
inverse-ambiguous), through clone/hard-links and SnapNode, into the
writer: restore filenames derive from the RAW bytes via the
maximal-subpart policy (= the og's MultiByteToWideChar(CP_UTF8, 0)
restore semantics, DRWUI :264664-694/:357529) — FFFD-substituted names
on disk, lossless D-53 display in the tree/report. ZFS untouched (FFFD
baked at scan = the pinned engine asymmetry). 6 tests, fail-before
4/6 RED. B10/D12 verdicts → MATCH for the N-029 surface.

---

## Dev-sign session wave (2026-10-28, audited `dd82db9` — ifileproperties-part1's first full read)

The part1 artifact (the NTFS/FAT/exFAT field schemas + the 23-slot
IFileProperties interface) was cited once (N-024) vs parts 2/3 (×6/×8); the
field-level comparison this session:

- Interface: 23 slots — 16 MATCH/equivalent, 1 MISSING (+0x20 type-registry
  id getter — the registered N-050/N-059 residual), 1 UNDETERMINED, 5 N/A
  both sides.
- Fields: NTFS 12→10 matched, FAT 10→9, exFAT 10→8 — the core's carriers are
  in places richer than the og (4-state deleted model, 4th timestamp,
  allocated_size, codec/psize extents, resident_data, record_ref).
- **N-087 (MINOR)** — FAT 8.3 names: the og persists raw OEM-codepage bytes
  (part1:121); the core identity-maps each byte to U+00XX (`cr_lfn.cpp:113-129`)
  — non-ASCII 8.3 names display/restore with different glyphs. B10/D11 → MINOR.
- The anchor-datum slots (FAT +0x3e first-cluster — lost for chainless files
  where the og keeps the origin; exFAT +0x50, NTFS +0x64 — semantics
  undecoded in part1) → `tracked-open/OQ-SA-1-anchor-datum-fields.md` with
  the resolution probes (rsf_field_census.py correlation; og-on-exFAT run;
  FUN_14058c870 decompile).
- Test debt noted: no end-to-end FAT/exFAT persisted-field round-trip test
  exists to pin the MATCH columns (fix-plan item in OQ-SA-1).


## Session-8 section (2026-10-28, audited `35adcb9`)

W1a/W1b/W1c verified at the load-bearing points (cancel-per-node;
ERR_CANCELLED-never-flattened; cancelled-never-auto-saves; light-record
summary; atomic write; the v5 tail round-trip). New findings: **N-090** (list
scope/row shape — no `auto_` filter vs the og's home-list enumeration; 1 s
save time; label[32]; no device-type), **M-079 FIXED** (`0b76757` by the parallel fix-lane: fill_volume_label at scan setup — --label override > the --volume target's GetVolumeInformationW > the image filename stem; this lane's convergence complement: the Windows-CI behavioral assertions over the committed fixture + the exFAT fixture label-entry spec-repair + the resume label adoption in adopt_resume_state_) (was: the CLI never fills
`volume_label`: no fill site exists, `fill_identity_card` covers every identity
field but the label; CLI auto-saves are always `auto_<ts>`), **N-091** (the
load-progress model + auto-save posture + thumbnail superset unregistered in
PARITY-DELTAS).


## Session-8 wave-2 (audited `ff879f9` lineage, corpus `be0cfe9`; complements the deep-audit lane's N-090/M-079 rows)

- **S-044 (FIXED this session)** — the W1c auto-save diverged from the og's
  trigger contract: the og's three call sites gate on results-found
  (FUN_1403964b0), not-already-saved (+0x388, 2 of 3 sites), and
  not-import (+0x1c8); the entry gate is a byte flag on a config
  singleton (FUN_1400b5270 :165951-5), not a directory field. **FIXED:
  all three trigger-site gates landed (results-found / not-import via
  the resume flag / not-already-saved via the explicit-save flag), on
  both the ABI completion window and the CLI twin; the byte-flag config
  shape is the registered deliberate delta D-105; the Folder suffix is
  D-106/N-105's deferral. Tests: CrUi.AutoSaveSkipsEmptyResults /
  AutoSaveSkipsAfterExplicitSave / AutoSaveSkipsResumedRun; TSan clean
  on the pause-window save. findings/FIXED/S-044.** Evidence:
  `../evidence/decompiled-quotes/s8-autosave-and-stamps.md` §1-§3.
- **N-105 (MINOR)** — the og's folder-scan "Folder" name suffix
  (:442883-86) has no core equivalent (deferred until a folder-scan
  mode exists; cross-ref M-079's volume-label gap).
- **N-100 (MINOR)** — the og persists the NTFS +0x60 twin (total $DATA
  clusters, sparse-zeroed) beside +0x64 in record-4 (rsf-format :398);
  the core carries +0x64 only (nflags bit-8). Register or carry as
  bit-9 with the sparse-zeroing rule.
- Verified MATCH this wave: the crsf reader's per-chunk raw fallback
  (F10) mirrors the og's raw-memcpy-on-inflate-failure (md :98);
  read_summary's SESSION_STATE+VOLUME_INFO light read matches the og's
  actual list behavior (record 1 THEN record 2 — the W1a phrasing
  corrected, erratum E64; the og's WinPE variant and size gates are
  .rsf-format internals, not core divergences).


## N-090 landed (the S56 lane, 2026-10-29) — the session-list scope + row shape

- **The scope rule REGISTERED (D-129a — the row renumbered from its born-collided D-135 seat at the N-207 repair):** the every-`*.crsf` listing is the deliberate
  superset over the og's `auto_`+`.rsf` home-list filter (DRWUI:432326-432333 —
  substring pair, re-read first-hand). A manual save is a resumable session; hosts
  wanting the og's exact home view filter client-side. Pinned:
  `SessionListScopeSupersetPinsAutoAndManual`.
- **The label width CLOSED (not registered — fixed):** the og's record-2
  `volumeName[100]`/strncpy(…,99) width end-to-end — `ScanConfig.volume_label[100]`
  (the host channel), the v6 record tail `label_full[100]` (len-gated; zeros on
  v1..v5 = the legacy card is the whole truth), the ABI tail appends on
  `cr_ui_session_row` + `cr_ui_session_info` (no version bump; `label[32]` = the
  legacy prefix view). Pinned: `SessionListVolumeLabelFullWidthRoundTrips` (RED as
  the ABI-append compile failure, the house precedent).
- **The save-time seconds contract (D-129b) + deviceType not-carried (D-129c):**
  documented in ABI-CONTRACT §7f + PARITY-DELTAS D-129 (the og's ms + its 2→Desktop/
  3→Recycle-Bin table recorded for the host that eventually asks).

## Session-8 lead-lane wave (audited `ff879f9` — the W1a/W1b/W1c verification, final numbering)

The load-bearing halves verified MATCH (light records; per-node load progress + cancel +
the partial-tree discard; the completion trigger + the byte-exact naming). Filed:
**N-116** (the row-ORDERING divergence — the og's FILETIME time-sort :432112-432142 vs
the path-byte sort; a distinct axis from N-090's scope/row-shape finding — **FIXED in
session 38: rows now newest-first by save_unix with a path-ascending tie-break; the
comparator's direction settled first-hand (!(ftA < ftB) = newest-first); the TRUNCATED
answer is the time-ordered prefix; the CLI verb re-sorts by the same key; contract
re-worded in cr_ui.h + ABI-CONTRACT §7f; `SessionListOgTimeOrdering` RED→GREEN**),
**N-117**
(the plan's un-landed W1b shape + the ABI-CONTRACT line-cite drift), **N-118** (the stale
R-52/54/56 gap rows). The registration-class deltas ride the parallel N-091 + the lead
addendum (the sanitizer superset).

---

## Session-10 lane-B addendum (2026-10-29, audited `84a1832`)

**N-131 (MINOR, NEW):** the FS-native parent anchors are not persisted (NTFS
parent-reference qword +0x40 / FAT parent-directory u32 +0x42 / exFAT +0x54 twin per
ifileproperties-part1 :84/:129/:148) — the core persists the structural `parent_id`
index only (cr_records.cpp:175; `rg parent_anchor|parent_reference|parent_frn` → 0
hits). The hierarchy round-trips; the FS-native lineage datum (reload-time orphan
cross-validation, forensic parentage) does not. The N-100 precedent class; the full
per-field table — every load-bearing family MATCH-with-re-encoding, 13 SUPERSET
fields, the complete ABSENT enumeration — is `evidence/s10-field-parity-table.md`
(also the M-050 completeness record + the ReFS FLAGS caveat, erratum E69).
**Executed ground-truth:** the three real SaveScan .rsf files parsed first-hand this
session — the corpus container grammar byte-confirmed (magic, 0x8C header, 0x24
record header, 4 KiB chunked zlib), the strongest-evidence-class backing for the
session-format MATCH verdict (`evidence/executions/s10-rsf-ground-truth.md`).

---

## Session-12 continuation row (2026-10-29, audited `c7fa527c`, corpus `be0cfe9`)

**N-186 (MINOR — NEW; renumbered from N-142 at the N-160 doc-truth repair):** the og's variant-save device-descriptor record (the 5th
GUID `76a672b048058b46ba9e91093a63b517`, FUN_1403f1180 = [552-B struct] + [5400-B
volumeDevice blob]; rsf-format--scan-session.md:70-75 + :225-233) persists the full
device identity: bus type, model, serial, **firmware revision, WWN, interface
string, and the complete 512-byte boot-sector snapshot**. The core's identity card
(`VolumeSessionInfo` v2 tail, cr_records.h:190-210) carries model/serial/boot_hash
only — firmware/WWN/interface and the boot-sector content have no equivalent
(D12 field-parity gap on a variant path not seen in the shipped samples). Evidence:
`evidence/decompiled-quotes/s12-rsf-fifth-record.md`.

## S50 addendum (2026-10-29 — the continuation audit lane; audited core `34d6cf10`)

- **N-186 RETIRED (FIXED-ALREADY — the battery's one falsely-active finding):**
  the field set this finding asked for (firmware/WWN/interface_desc/
  boot_snapshot[512]) landed with N-066's `4d3f0409` three days BEFORE the
  filing — present at the finding's own audited tree c7fa527c (the "grep-
  verified zero hits" claim was false at its own pin). Retired to
  findings/FIXED/N-186 with the false-grep note + the VolumeSessionInfo
  exposure sliver (optional follow-up, not a gap). The N-066 row above
  remains the owner of record.
- **N-192 (MINOR — tree/session axis — FIXED in the S60 lane)** — `FileProperties::clone()`
  dropped `short_name` on the hard-link fan-out (cr_node.cpp vs the
  og's whole-block clone contract); the one-line carrier landed (the
  name block of clone()), the fan-out clone and the session roundtrip
  pinned RED->GREEN by NtfsVolume.HardLinkFanOutClonesKeepShortName —
  `findings/FIXED/N-192-clone-drops-short-name-on-fanout.md`. The
  part1-census sibling register I-047 (the +0x80 format/icon-ID constant
  family, acknowledged-at-filing) landed in findings/FIXED/.
- **The ifileproperties-part1 under-cited flag discharged:** the full
  census (51 MATCH / 13 DIVERGENT (15 of 17 non-MATCH trail-owned) /
  4 ABSENT / 7 N-A) at `evidence/s50-ifileproperties-part1-census.md` —
  the four module rows' "under-cited (x1)" note is closed by disposition.

---

## S54 addendum (2026-10-29 — the fifth continuation lane's focus deep dives)

Addenda filed on existing findings: **M-126 +S54** (the ETA budget seed decoded:
`total_bytes/60 MiB + 60 s` at DRWUI:315150 — the fix plan gains the seed step),
**N-178 +S54** (the og's engine-pause type gate: +0xD8 != 1 skips engine pause —
the core's all-stage pause is a superset to register or align), **N-186 +S54**
(the scope correction: the persistence half landed as the N-066 v4 tail BEFORE the
finding's pin — the live remainder is the VolumeSessionInfo mirror gap; the
parity-table row here owes the narrowed-scope correction). Verified-MATCH this
session: the .rsf field families re-walked against the persistence layer (the
state/persistence surface corroborated — no NEW divergence beyond the filed set:
the I-041/42/43 registers, S-057, N-090, the D-122 family all stand), the
checkpoint/save-trigger set, and the load path's v1..v5 tolerance ladder.

## S55 addendum (2026-10-29 — the sixth continuation lane: the persistence deep dive)

New on this module: **N-203** (the resume round-trip drops the candidate
data-area extent — the save/load pair never carries
data_start_lba/data_sector_count; the fix plan proposes the PartitionContext
v4 tail + the revalidate scratch-enrichment for pre-v4 sessions). The .dat
lifecycle register **I-046** records the og's persistence-owner truth for the
parity tables (per-process-generation cache, not a knowledge base). The
battery re-confirmed the standing session-family findings (N-090, N-131, the
I-041/42/43 registers) at the `1fccb0a4` pin — all convergent with the S54
lane's own battery.


## S66 lane (2026-10-29): N-203 landed

**N-203 FIXED** — the partition-context record's v4 tail: `data_start_lba` /
`data_sector_count` ride the session round-trip (77-byte record; v1..v3
sessions deserialize zeroed per the skip-unknown rule). The load path restores
both; `revalidate_candidate` drops its const — a successful probe of a
zero-extent record copies the scratch's re-derived data area back into the
candidate (the og's 0x448 CSearchedPartitionInfo image replays with the full
extent identity — the {start,end,disk} dedup key re-verified first-hand at
PRD:20825-20831; the copy-back is the core's first-resume repair, idempotent
once re-saved). Pins: RoundTripKeepsDataAreaExtent / V3SessionResumeReDerivesDataArea
/ RestoredWindowMatchesFreshDiscovery / EnrichedExtentIsIdempotentAcrossReSave
(RED: load-fill + enrichment surgically disabled, 3×FAILED → GREEN 11/11).

## S76 addendum (2026-10-29 — the MINOR-first fix lane; N-131 landed)

**N-131 FIXED** — both FS-native parent-anchor carriers landed as nflags-gated
append-only blocks (bits 12/13, fires-only-when-nonzero — pre-N-131 session
records stay shape-identical): `ntfs_parent_ref` u64 (the og's CNTFS
+0x40 — the $FILE_NAME parent reference masked to the 48-bit record number,
FUN_18002a340's local_41 mask decoded first-hand; stamped per-link at the
hard-link fan-out so each link keeps its own parent, plus the GUID-dump and
INDX second-source emissions; the USN journal pass stays unstamped — its
parent comes from the USN_RECORD_V2 header, not a $FILE_NAME, and the og's
USN emission builds name-only props) and `fat_parent_cluster` u32 (the og's
CFAT +0x42 — the containing directory node's own +0x3e origin cluster, the
walk-stamp decoded first-hand at FATSearchDLL :11654/:11500-04; the volume
root anchors 0 on every flavor by construction [engine init FUN_18000e130,
FAT32 included], a walked subdirectory anchors at its own first cluster).
**The exFAT +0x54 open question is RESOLVED first-hand — NO CARRIER:** the
exFAT engine never stamps +0x54 for children (exFATSearchDLL :45155's child
emission stamps vt+0x28 only; the sole +0x54 writes are root/lost-dir
0xFFFFFFFF initializations at :3895/:5026/:11461 and clone copies; the DRWUI
class exposes no +0x50/+0x54 accessor at all) — the anchor-twin hypothesis
and the part1 census's "type-registry ID" label are both refuted; carrying
nothing IS the parity. Tests: Session.NtfsParentRefRoundTrip /
Session.FatParentClusterRoundTrip (the round-trip + the zero-carrier gate) +
FatEngine.ParentAnchorCarriesContainingDirCluster /
NtfsVolume.ParentAnchorIsMaskedFileNameParentRecord (the walk-level stamps +
session survival); suite 1526/1526. The s10 field-parity table's rows 1-2
(now-carried) and the exFAT row (resolved dead-slot) are superseded by this
landing.

## Session-77 addendum (the S79 continuation audit lane; core `3799dee2`, re-verified at `0da3490b`, corpus `be0cfe9`)

- **M-150 (MAJOR, NEW):** the load-path count pre-allocation — `cr_records.cpp:372`
  (re-verified pin; the N-131 parent-anchor landing shifted the file) pre-allocates
  8×count from the uncapped hostile u32 (up to 34.4 GB of touched memory) before any node
  parse; the og's record-4 reader is per-node-factory (bounded by buffer exhaustion,
  rsf-record4 :16/:26). The container's 64 MiB record cap and the P1-7 amplification
  budget do not reach the in-record count field. The torn-session READ-side freeze/OOM
  complement to M-131's write-side repair. Evidence: `s79-lead-verification.md` §A.
