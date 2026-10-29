# modules/apply.md — the consent-gated lost-partition apply module audit verdicts (the table-write path)

Reference: the og's lost-partition flow is a TWO-STEP product feature — discover
(the decoded probe family, S-006/S-019 parity), then APPLY: RecoveryModule's
CreateRecoveryPartitionObject hosts the apply scenario; PartitionRecovery.dll's
FUN_1800566d0 "rebuild disk layout" writes the recovered entry back into the
user's table (the legacy chain: IOCTL_DISK_DELETE_DRIVE_LAYOUT 0x7C100 :46953,
raw sector-0 rewrite — zeroed entries + 0x55AA :46957-46962,
IOCTL_DISK_SET_DRIVE_LAYOUT 0x7C010 with a 4-entry DRIVE_LAYOUT_INFORMATION
:46995-47002; the modern single-entry path IOCTL 0x7C058 with an 0x18-byte record
:47015; every write followed by IOCTL_DISK_UPDATE_PROPERTIES 0x70140 with a 3x
Sleep(1000) retry :46873-46880). The GPT variant FUN_1800392f0 :32797-32860
regenerates the 32-BIT DISK SIGNATURE via GPT_GenerateSignature and rewrites the
protective MBR — the S-029 Resolution's corpus correction: the regenerated
identity is the MBR+440 signature value, NOT the 128-bit GPT disk GUID (every
call site passes a 4-byte stack slot as one register argument; the GPT headers
stay untouched).

Core (audited at the session-11B tree `b37fabab`-lineage): `src/core/apply/`
(cr_apply.cpp) + `include/cr/apply/` + `tests/unit/test_apply.cpp` (815 LOC);
surfaced as `rrecov apply --disk N --session F --candidate N [--apply]`.

**Scope note (I-030's packaging rule):** this row SUMMARIZES already-scored
coverage — S-029 (the module's own finding, FIXED), D-98 (the apply safety
registers), D-68 (the IMPLEMENTED row), and M-040's apply endpoints (the
boot-chain repair family, FIXED `94c9dda`). No new audit is opened here.

## Behavioral

- **B-x the apply semantics** — **S-029 — FIXED**: the recovered partition entry
  lands in the user's MBR, transactionally, undoably, through the same seams
  RawFixer established (S-008/S-010): the raw sector-0 write path via
  repair::IWriteDevice (the og's raw-write leg), the volume quiesce in the
  platform transport's lock_volume() (the S-010 sequence), and the shared
  CRUNDO01 undo container. `plan_apply` (read-only, containment-refusing) +
  `execute_apply` (consent callback → revalidation → transactional sector-0
  write → read-back verify → rollback). The kernel-mediated IOCTL chain (the
  og's DELETE/SET_DRIVE_LAYOUT + UPDATE_PROPERTIES retry tail) is the
  IApplyDevice seam — the Windows RW transport implements it; file/RAM devices
  keep the portable no-op defaults (the sector write IS the commit).
- **B-x the GPT variant** — **REGISTERED v1 boundary (D-68/D-98)**: MBR only;
  GPT layouts are refused cleanly (the og's GPT variant FUN_1800392f0 incl. the
  signature regen is v2 scope). Registered, not silent.

## Safety

- **D-98 — the apply safety registers (S-029's deliberate supersets)**: four
  registered deltas over the og's writer, each a safety superset, none a
  capability loss (the consent gate before the volume lock; the containment
  refusal; the transactional write + read-back + rollback the og's raw chain
  lacks; the shared undo). PARITY-DELTAS `:2313+`.
- **A63-adjacent consent** — MATCH-by-superset: the og's apply is a product flow
  behind UI confirmation; the core's consent callback fires BEFORE the volume
  lock is taken (the caller deliberates without holding the volume).

## API

- **D-68 (IMPLEMENTED row, PARITY-DELTAS `:2010+`)** — the CLI surface
  (`rrecov apply`) + the C++ plan/execute pair; the ABI export rides the same
  staging rule as the reghive profile (exposed when a host-facing flow consumes
  it).

## Tests

- `tests/unit/test_apply.cpp` (815 LOC) — the plan/refusal set, the transactional
  write + rollback, the containment refusals, the undo round-trip (the S-029
  landing's RED→GREEN set).

## Related findings and registrations

S-029 (FIXED) · D-98 (PARITY-DELTAS `:2313`) · D-68 (PARITY-DELTAS `:2010`) ·
M-040 (the boot-chain repair family, FIXED `94c9dda` — the apply endpoints'
siblings) · S-051/M-104 (the whole-disk scan-side pair — the candidates this
module applies) · N-121 (the boot-fix ESP consumer).

## S80/S81 fix-lane addendum (2026-10-29 — N-145 lands)

**N-145 FIXED** (the S11-lead Lane D filing): the cross-run in-flight
journal. `apply::inflight_begin/end/probe` — a single journal file in a
host-supplied state directory (`<identity>\t<begin-unix>` per line),
idempotent in all three operations, with the og's exact semantic: mark
at op-begin, remove at op-completion, so only process death leaves the
mark (the detectable class). The og's CWD-relative rwConfig.ini is the
REGISTERED DECLINE (the finding's own fix plan names it an og defect).
Wired at both CLI write paths (cmd_apply around execute_apply, with
the next-start probe warning for a surviving mark; fix --undo-apply
around repair::undo), the journal beside the session / undo artifacts.
The cr_ui ABI hosts adopt the same helpers when they carry a state dir
(the og's own next-start consumer is UI-side and undecoded). Tests:
the lifecycle (incl. the crash shape), the multi-disk idempotence, and
the probe's missing/truncation contracts; win_syntax_check green over
the CLI wiring; suite 1540 ran / 0 FAILED.
## Session-81 addendum — N-145: the cross-run in-flight journal (2026-10-29)

**The interruption record landed.** The og marks the target disk in
rwConfig.ini [QuickPartition] pendingMarkedDisk for the repartition op's
lifetime (event 0x102d); the core's CRUNDO01 transaction was stronger
per-op but left no trace across a process death. The landing: the
CRIFJ01 journal record ({disk size, pre-apply sector-0 CRC, candidate
LBA/kind, disk ordinal, begin timestamp}, CRC32-checked container,
the CRUNDO01 house style), emitted through an optional
`ApplyJournalFn` sink — Begin after the lock + both revalidations,
before the first write; End on every clean settle (success or
fully-verified rollback; ERR_REPAIR_UNDO_FAILED keeps it ARMED — the
disk is still half-modified). The pure `journal_probe()` re-identifies
the disk at the next start (size identity; the CRC the secondary hint).
Host wiring: the CLI writes `rrecov-apply.ifj` beside its undo log and
probes it at every startup — the interruption warning names the disk
ordinal and the undo-apply recovery path. The og's CWD-relative ini
location declined (PARITY-DELTAS D-152 — a marker whose findability
depends on the working directory is a defect, not a parity surface).
Tests: Apply.InFlightJournalLifecycleSuccess / RefusalEmitsNothing /
CleanRollbackEnds / SurvivesAsRecordForNextStart (the crash shape: the
round-trip, the probe's three answers, the torn-container refusal).
Suite 1634/1634. The A34/A35 rows: MATCH.
