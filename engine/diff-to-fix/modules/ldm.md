# modules/ldm.md — partition table + LDM module audit verdicts (table.md covers MBR/GPT/EBR)

Reference: Partition.mo / LdmManager.mo / PartitionRecoveryD per corpus artifacts
`PartitionRecovery/mo-module-map.md`, `PartitionRecovery/rawfixer-v2--method-inventory.md`,
`SearchDLL/zfs-vdev--mftmirr--calcpartition.md` §2–3; og binaries themselves (A-001
binary evidence; session-2 direct extraction — `evidence/executions/s2-ldm-guid-constants.md`).

Core (audited commit `67193b6`, session-2 re-verified at `2545b83`→`4f0c841`):
`src/core/table/` (cr_table_parse, cr_ldm, cr_crc32 — 1,900 LOC) +
`include/cr/table/` + `tests/unit/test_crc32_gpt.cpp` (334 LOC) +
`test_ldm.cpp` (506 LOC) + `test_refs_ldm.cpp` (81 LOC).

**Session-2 deep pass (T7):** GPT-LDM gating, PRIVHEAD candidate selection, RAID-5
rotation + reconstruct, and Partition.mo's entry scan read first-hand from the
decompiles (`evidence/decompiled-quotes/s2-ldm-gpt-gate.md`,
`s2-ldm-raid5-rotation.md`). OQ-T7 RESOLVED (left-symmetric, MATCH). **A-002
FIXED (A-002 fix session):** the two LDM GUID constants restored byte-for-byte
to the real Microsoft values (independently re-verified against the og binaries
first-hand); the regression tests de-self-referenced (real bytes hard-coded in
fixtures) + 3 new guards; real GPT dynamic disks parse end-to-end again.

## Behavioral

- **B1 detection** — **A-002 (session 2, ACUTE) — FIXED (session 3)**: the A-001 fix `1cb6a43` corrupted
  both LDM GUIDs with fabricated values — every **real** GPT dynamic disk failed
  `find_privhead` (ERR_BAD_MAGIC) until this session restored the real Microsoft
  GUIDs (byte-for-byte what the og's LdmManager.mo DAT_1800c4960 and
  Partition.mo DAT_180036630/0x36640 carry — extracted and re-verified
  independently by the fix session's own PE walker; fail-before/pass-after
  proof in the finding's Resolution). ESP/MSR/
  BIOS-boot/Linux half remains fixed correctly. `M` on MBR parsing (0x42 dynamic
  detection at fixed 6×512 metadata offset — matches the og's default candidate
  sector 6), GPT header/entry CRC validation, backup-GPT salvage at device end,
  GPT_ENTRIES_READ_CAP; og-side: GPT-LDM gate checks revision/reserved where the
  core does not (informational — both accept all real disks).
- **B2 recovery correctness** — `M` on MBR-dynamic path (PRIVHEAD/TOCBLOCK/VMDB parse,
  VBLK composites with kernel-quirk parity, fragment rules, dup-fragment → broken
  group); **OQ-T7 RESOLVED (session 2): RAID-5 rotation = left-symmetric on both sides
  (algebraically identical formulas; og selector ctor-hard-zeroed, R=1/2/3 dormant —
  I-008); reconstruct-on-failure XOR-folds the other columns, second missing member
  = surfaced failure, never zeroed, on both sides**; re-anchoring per member
  (per-member PRIVHEAD lds) implemented. PRIVHEAD candidate anchor: both sides read
  the GPT entry's LAST LBA (og `:59448` entry+0x28; core `cr_ldm.cpp:1020-1024`) —
  `M`. PRIVHEAD validation: og accepts version 2.11/2.12 only (`:59700-59706`), core
  accepts 2.x — informational (covers all real disks).
- **B3** — `M` unknown VBLK types counted (not silent).
- **B8 multi-volume/cross-partition** — `M` spanned/mirrored/stripped/RAID-5 layouts
  resolved; unresolved kept as Broken (no silent drops); cross-disk member registry in
  CLI. **A-002 FIXED (session 3): the GPT side is unblocked** — real-GUID dynamic
  disks parse end-to-end again (Ldm.GptDynamicDiskCanonicalGuidsParse with
  hard-coded Microsoft bytes; fabricated values rejected — see B1/C19).
- **B6 damaged-volume** — **M-002 FIXED (s10)** (torn chain keeps the walked prefix + ebr_status/ebr_break_lba honesty — findings/FIXED/M-002);
  `M` on two-step GPT salvage + backup-at-end.

## Constants and formats

- **C19** — **A-002 — FIXED (session 3)**: both LDM constants restored to the real
  Microsoft values; the regression tests de-self-referenced (fixtures carry
  hard-coded Microsoft bytes, never the constants under test) and hardened with
  `Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText` (bytes→text) +
  `Ldm.GptDynamicDiskFabricatedGuidsRejected` (negative guard).
  `M` on MBR/EBR/GPT signatures, PRIVHEAD/
  TOCBLOCK/VMDB magics, VBLK types 0x32/0x33/0x34/0x44/0x35/0x45/0x51.
- **C20** — `M` (header/entry layouts, end-of-disk PRIVHEAD fallbacks
  last−2048+1856/+2047/last−1).

## Algorithms

- **A30 lost-partition inputs** — `M` (partition table feeds the lost scanner; A-001's
  misclassifications pollute these inputs — fixed by A-001).
- **A27** — `M` on spanned/mirror/stripe reads; **OQ-T7 RESOLVED (session 2): RAID-5
  rotation MATCH — left-symmetric on both sides (algebraically identical); og's
  R=1/2/3 rotations are dormant dead code (I-008); reconstruct-on-failure semantics
  identical**.

## State machine

- **S32 cancellation** — `NA`-note: no cancel tokens in this module (contrast: FS
  engines poll). LDM config parse is bounded (≤4 MiB) and fast; volume reads during
  recovery are cancellable at the writer. Recorded as note (a multi-terabyte
  end-of-disk PRIVHEAD probe ladder is the only long pole — bounded to 3 reads).

## Performance

- **X51** — `M` sector reads, ≤4 MiB LDM config, 1 MiB read_exact chunks.

## Safety

- **F59/F62** — read-only module (no writes) — trivially safe.

## Quality

- **Q65 tests** — strong on parse paths (20 tests); gaps pinned by A-001 (canonical-GUID
  fixtures!), M-002 (torn EBR), EBR entry-0-link variant, degraded LDM paths,
  end-of-disk PRIVHEAD fallbacks.

## Findings index (this module)

| ID | Severity | One-line |
|---|---|---|
| A-001 | ACUTE | Six wrong GPT type GUIDs — GPT dynamic disks get zero LDM recovery (executed proof) — **4 of 6 constants FIXED (diff-to-fix session); LDM half was a FALSE POSITIVE, REOPENED, superseded by A-002** |
| A-002 | ACUTE (session 2) — **FIXED (session 3)** | The A-001 fix corrupted both LDM GUIDs with fabricated values — real GPT dynamic disks failed LDM discovery (executed proof + og binary extraction + Microsoft Learn). FIXED: constants restored to the real Microsoft GUIDs, tests de-self-referenced + negative guard added; full Resolution in the finding file |
| M-002 | MAJOR | Torn EBR chain discards walked volumes — **FIXED (s10)**: walked prefix kept unconditionally; status+break-LBA recorded |
| I-008 | INFO (session 2) | og Raid5VolumeStorage carries 4 selectable rotations, only left-symmetric enabled (ctor-hard-zeroed); core implements the enabled mode exactly |
| OQ-T7 | tracked-open | **RESOLVED (session 2)** — RAID-5 rotation = left-symmetric both sides, MATCH |

## Post-baseline note

Audited baseline `67193b6` already includes concurrent fixes through `7797bd6`; no
table/LDM files were touched by the concurrent session's later commits at audit time.

---

## Session 2 re-audit (audited commit `5d6da5f`)

No new ldm-specific findings in session 2. The session-1 verdicts
stand. Session 2's parallel agents re-verified the architecture parameters
(R54-R58) and safety parameters (F60-F64) cross-cuttingly; no new
ldm-specific divergences surfaced.
## Session-2 update (2026-10-25, core `e93decf`)

**Verdict changes (quotes in `../evidence/decompiled-quotes/ldm-raid5-and-scoring.md`):**
- **OQ-T7 RESOLVED — MATCH (safety-critical closure):** og RAID-5 rotation is hardwired
  layout 0 = LEFT-SYMMETRIC (ctor zeroes the selector; SPEC copy never reaches it; no other
  writer); core's formula is identical (parity col, data col, member offset, degraded-XOR set).
  **No transformation needed.** Interleave both config-driven from the component VBLK.
- **B1/C20 → M-016 FIXED (s19):** TOCBLOCK candidates — the core's list is now
  {config+2, config+0, config+1, +2045, +2046} (og probe order first, kernel
  tail-copy superset behind). config+0-only disks parse (fail-before proven);
  +1/+2 conflicts follow the +2 copy's VMDB anchor (committed_seq proof); the
  og's hard-fail-on-bad-bitmap-name stays un-adopted (forgiving per-copy walk =
  registered recovery superset). Resolution in findings/FIXED/M-016.
- **N-025 FIXED (s23):** the og's 1 MiB config-region gate is first-class
  (`config_size >= 0x801 -> ERR_BAD_CHECKSUM`, LdmManager :59748-59759 — the
  4 MiB read clamp removed as unreachable), and a zero interleave now defaults
  to the og's 64 KiB (128 sectors) on both the striped and RAID-5 layout paths
  (og :47665/:68631-68641) instead of losing the volume. 2 fail-before tests
  + the 2048-sector boundary pin; Resolution in findings/FIXED/N-025.
- **M-002 note (superseded s10):** FIXED — TornLinkKeepsPriorVolumes/TornLinkStatusReported prove the walked-prefix survival and the status vocabulary. Prior note (
  session-1 evidence; decompile locations now mapped for the next pass).

### Session-2 addendum (post-reconciliation)
Cross-references to the parallel legs' LDM findings: A-002 (the 1cb6a43 LDM-GUID regression,
executed proof, restore landed `23adad1`), A-003 (fix --part LBA-0 basis, FIXED), I-008
(the og's rotation modes 1-3 are dormant dead code — consistent with this leg's
hardwired-layout-0 finding; OQ-T7 RESOLVED-MATCH). This leg's active LDM items: none — N-025 FIXED in s23
(config-size + zero-interleave geometry); M-016 FIXED in s19.

---

## Session-5 audit (audited commit `7cf6754`)

- **M-046 (MAJOR — LDM half): FIXED (s10)** — GPT path geometry-correct ×3 sites + the 512-LBA unit family (end-of-disk candidates / backup bound / capacity gate); GptPathFindsMetadata4Kn fail-before proven. ~~the LDM-over-GPT discovery path hardcodes 512-byte
  LBAs (`cr_ldm.cpp:1007/:1014/:1021` — `read_at(1*512,…)`, `entries_start_lba*512`,
  `last_lba*512`) while the main GPT parser is geometry-correct
  (`cr_table_parse.cpp:19-22`). On 4Kn GPT disks the LDM metadata partition is looked up
  at wrong offsets → dynamic-disk recovery fails through this path. Fix: thread
  `geometry().logical_sector` through the three sites.
