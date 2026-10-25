# Session 2 — diff-to-fix continuation audit prompt (redacted)

## Mandate
Session 1 of this audit is complete (31 findings + 9 tracked-open). A parallel
remediation leg has fixed A-001, M-001, S-001, S-005 and resolved OQ-T1.

Session 2's three jobs:
1. Look for things session 1 MISSED — new divergences in parameters/modules not
   deeply audited.
2. Continue the audit into areas session 1 didn't cover (B9 timestamps, B7 ADS/
   hardlinks/sparse/reparse per engine, B16-B18 sort/filter/paging, P43-P46 API,
   F60-F64 safety, X47-X53 perf static analysis, R54-R58 architecture, Q65-Q70
   quality per-module).
3. Remove the fixed things — move A-001/M-001/S-001/S-005/OQ-T1 finding files to
   findings/FIXED/ with a RESOLVED banner so they're preserved for the audit
   trail but no longer counted as active roadmap items.

## Read-only mandate unchanged
No source code, headers, tests, build files, workflows, or existing docs are
modified. Only `diff-to-fix/` and `worklog.md` (and `worklog/session-*.md`)
are written.

## Repositories
- Core (audited): https://github.com/heicfast/core_recovery  (HEAD: 5d6da5f)
- Research corpus (ground truth): https://github.com/heicfast/easeus-drw-research
  (HEAD: 03ad46c, unmoved since session 1)
- Token: env/git-config only, never committed (token-redacted in this file).

## What session 1 likely missed (initial gap list)
- Timestamp precision/timezone per engine (B9)
- ADS / reparse / $AttrList on NTFS (B7 partial)
- Hard-link behavior beyond "PER-LINK names" (B7 partial)
- Sparse file extent accounting beyond the S-001 sub-fix (B7 partial)
- Sort/filter/paging model per engine (B16-B18 unverified)
- API ownership/lifetimes/async/ABI versioning (P43-P45)
- Bad-sector / BitLocker / dest-overlap / RawFixer consent / SDDL (F60-F64)
- Performance: I/O read sizes/alignment audit beyond "X51 = M" claim (X47-X53)
- Architecture: dependency direction / wiring / layering shortcuts (R54-R58)
- Per-parameter test mapping (Q65) — claimed MATCH but no mapping produced
- Per-parser fuzz harness audit (Q66) — only CI gate M-006
- Sanitizer cleanliness per module (Q67) — claimed MATCH but no per-module check
- ghidra-partition decompiled .c files (session 1 used .mo files only)
- Carve fragmentation reassembly + dedup precedence
- Session .crsf vs .rsf field equivalence (D12 only superficial)
- Writer atomic write / disk-full / partial-overwrite refusal
- CLI long-option edge cases / batch mode / exit code completeness
- Engine-specific: ext4 extent tree depth, HFS+ resource fork, Btrfs subvol
  rooting, ZFS snapshot, ReFS integrity streams, LDM RAID-0/1 stripe alignment

## Workflow
Long-horizon waves. Commit and push per coherent wave. Maintain worklog.md
append-only. Every new finding cites the audited commit hash (5d6da5f for new
findings; existing findings keep 67193b6 unless re-verified against 5d6da5f).

## Severity discipline unchanged
ACUTE = wrong recovery / data loss / crash / security regression.
SEVERE = user-observable behavioral gap.
MAJOR = perf, edge coverage, architecture.
MINOR = cosmetic, doc drift.
INFO = deliberate, reviewed difference.
No inflation, no deflation.
