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
