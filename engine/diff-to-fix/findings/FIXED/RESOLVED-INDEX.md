# diff-to-fix/findings/FIXED/ — resolved findings (audit trail)

This directory preserves the original finding files for items the
remediation leg has resolved. Files here are kept verbatim (with the
added `## Resolution` section) so future sessions can audit the fix
against the original finding. They are NOT counted in the active
severity totals in `diff-to-fix/SUMMARY.md` and are NOT part of the
active roadmap.

## Resolved findings (12 — fixed between session 1 close and session 2 rebase)

| ID | Severity (orig) | Title | Fix commit | Resolution |
|---|---|---|---|---|
| A-001 | ACUTE | Six GPT type-GUID constants wrong; GPT dynamic disks fail LDM | `1cb6a43` | Canonical mixed-endian disk bytes; `FsKind::EfiSystem` added; 4 permanent regression tests incl. end-to-end GPT-dynamic LDM parse. Suite 263/263. Pre-fix probe reproduced `ERR_BAD_MAGIC` verbatim. |
| A-003 | ACUTE (discovered) | `fix --part --apply` wrote at disk LBA 0; volume-scan undo logs slice-relative | (this commit) | `repair::rebase_plan` — one offset-basis mechanism: plans rebased to the parent basis after consent, before execute (orchestrator by `slice->absolute_base()`, cmd_fix by `part_base`, display shows disk-absolute targets). S-008 writer adapters removed (mapping + rebase would double); undo logs parent-absolute per the documented `--undo-apply` contract. `RawFixer.RebasedPartitionPlanWritesAtDiskBasisNeverLba0` proves LBA 0 untouched + partition-absolute landing + disk-absolute undo offsets. |
| S-001 | SEVERE | Fully-overwritten deleted NTFS files emitted (graded) vs og destroy | `e93decf` | `WalkerFlags::destroy_fully_overwritten` (default ON) drops fully-overwritten deleted non-directories; `stats.destroyed_overwritten` telemetry. Sparse-run accounting sub-bug fixed in same commit. Honest-mode flag pins old keep-and-grade for parity-bench. |
| S-004 | SEVERE | ext4 journal salvage descriptor referenced only (not every-block) | `51ac671` / `e0175b3` | ext4 journal salvage now parses every block of the jsb-declared range (was: descriptor-only). jsb gate added. Fail-before/pass-after tests. |
| S-005 | SEVERE | ext4/Btrfs/ZFS names byte-lifted to mojibake | `2f3237d` | Shared strict RFC 3629 decoder `include/cr/cr_text.h`; sites: ext4 dir block, Btrfs dir items, ZFS dnode names + zap child names + nvlist pool display name (sub-site discovered during fix). 3 engine fail-before/pass-after tests + 11 decoder vectors. Suite 281/281. |
| S-008 | SEVERE | RawFixer boot-sector-only + not wired pre-scan vs og 33-method pre-scan repair | `e0175b3` | RawFixer pre-scan wiring + CheckMFT/CheckFatTable/FixByScanSector. The 33-method pre-scan repair surface is now wired into the scan path; damaged metadata volumes get structured recovery (not just carving). |
| M-001 | MAJOR | NTFS bootstrap rejects record sizes the og clamps | `5d6da5f` | Degradation ladder in `validate_boot_sector`: unusable encodings fall back to og's 0x200; usable values pass through; 0x400 min clamp both paths. Both callers (`NtfsVolume::open`, `lost::probe_ntfs`) benefit; classifier scores torn-DBR candidates. 5 fail-before/pass-after tests. Suite 287/287. |
| M-006 | MAJOR (combined w/ S-009) | Fuzz CI disabled | `e30b84d` | Fuzz CI job re-enabled (`if: false` removed). CodeQL lane deferred (documented deliberate decision). The combined S-009/M-006 finding file remains in SEVERE/ pending split (S-009 disk-map visualization was FIXED via GAP-S6-11 `56cce87`; M-006 fuzz CI FIXED here). Split tracked in N-020. |
| N-002 | MINOR | FAT mask-24 EOC family alternate values mark chains broken | `fb44073` | FAT mask-24 EOC family handled og-parity: 0x0FFFFFF8..0x0FFFFFFF all treated as EOC (not just 0x0FFFFFF8). Fail-before/pass-after test. |
| N-003 | MINOR | exFAT entry-set checksum enforced beyond og | `fb44073` | exFAT entry-set checksum default switched to og-parity (lenient on damaged sets — keep them, don't drop). Fail-before/pass-after test. |
| N-010 (parallel-leg-theirs) | MINOR | FatTable::next swallows the BAD sentinel into the reserved-range EOC branch | `cb8dafd` | `v == CHAIN_BAD` test moved before the reserved-range branch in `next()`. BAD-terminated chains now grade broken on both sides (was: clean end). Grading-only divergence (recovered bytes identical). Discovered during I-002's pin-test addition. Suite 313/313. |
| OQ-T1 | tracked-open | NTFS secondary claim map semantics | (none — decompile read) | RESOLVED by decompile read: secondary map is ctor-NULLed, never assigned anywhere in NTFSSearchDLL — dormant polymorphic hook; same-direction (re-allocated). No code change needed. |

## Acknowledged informational findings (NOT moved — deliberate supersets with pin tests)

These 7 findings (I-001 through I-007) are NOT bugs — they're deliberate
superset behaviors the parallel leg verified and pinned with regression
tests. They stay in `findings/INFORMATIONAL/` (not moved to FIXED/)
because the audit trail should preserve the "deliberate difference" record
and the pin tests guard against anyone "fixing parity" by removing the
superset behavior.

| ID | Title | Acknowledged via | Pin test |
|---|---|---|---|
| I-001 | NTFS known-20 metafile filter vs blanket `$`-skip | `f74a5dd` | `NtfsVolume.UserFileNamedDollarNotHidden` |
| I-002 | FAT12 BAD cluster canonicalized vs og raw-follow | `f74a5dd` | `FatEngine.Fat16BadClusterStillCanonical` (which itself surfaced N-010-theirs) |
| I-003 | Btrfs core exceeds og deep scan and mirror probing | `f74a5dd` | `Btrfs.MirrorSelectionNewestGeneration` (already existed) |
| I-004 | ReFS detection strictness + version superset | `f74a5dd` | gate semantics documented in code comment |
| I-005 | Grouped supersets (7 sub-rows) | `f74a5dd` | each sub-row spot-checked |
| I-006 (session 2) | ext4 nanoseconds superset (core preserves nsec; og drops to second-only) | (session 2 — pending ack) | (no pin test yet — session 2 informational, no behavioral change to test) |
| I-007 (session 2) | ext4/HFS+/Btrfs: core reads more timestamps than og (4 vs 2/3/3) | (session 2 — pending ack) | (no pin test yet — session 2 informational, no behavioral change to test) |

## Why these were moved out of the active dirs

Per session-2 mandate ("remove the fixed things"), the user directed
that resolved findings no longer be counted in the active severity
directories. Moving them to `FIXED/` (rather than deleting) preserves
the audit trail — each file still cites the original divergence, the
evidence, the fix plan, and the Resolution section that records the
fix commit + verification.

## Audit-trail discipline

- Files in this directory are immutable post-resolution. Any re-audit
  of a resolved finding's fix should be a NEW finding (e.g. "A-001
  re-audit: regression in commit X") rather than an edit to the file
  here.
- If a session discovers that a "fixed" finding was not actually fixed
  (e.g. the fix was reverted, or the test was insufficient), re-open
  the finding by moving it back to its original severity directory and
  appending a "Re-opened" section.
- The active counts in `SUMMARY.md` always exclude this directory.
- The 7 ACKNOWLEDGED informational findings stay in
  `findings/INFORMATIONAL/` — they are deliberate supersets, not bugs.
  Their pin tests live alongside the engine tests.

## S-002 — HFS+ Attributes B-tree / decmpfs not walked

**Severity (when open):** SEVERE · **Fix commit:** `c9e8860`
**Independently verified:** session-2 second close-out leg — FUN_180009c10 re-read
first-hand (tree-8 key search, keyLength `(nameLen+7)*2`, recordType 0x10 gate with
"unsupported attribute node format" skip, value u64@+8); core walk_attributes_tree +
kZlib codec extents confirmed in-tree; DecmpfsInlineAndRsrcContentRecovered /
DecmpfsDisabledLeavesEmptyShell green in the 313/313 ASan suite.
**Resolution:** attributes B-tree leaf-chain walk; decmpfs modes 3/4 emit codec-tagged
extents the writer inflates at recovery time; named attributes surface as
`..namedfork/<name>` children (D-46 superset registered).

## Session-2 main-auditor leg addenda (2026-10-25)

The fixed findings above carry this leg's independent verification (and, in two cases,
correction) sections appended in place:
- **A-001:** my leg's initial verification note inverted the LDM-GUID conclusion; the
  appended correction accepts the parallel leg's executed proof (A-002) and the
  `23adad1` restore; the four non-LDM GUIDs + ESP-category verification stand.
- **N-003:** appended note refutes the fix's "verified-absence" premise with the
  `FUN_18001eba0` dual-form checksum quote — the og DOES validate entry sets (incl.
  deleted); M-013 carries the correct parity fix.
- **N-002:** behavior verified; citation corrected (:8030-48 is the lost-FAT region
  signature validator; real termination evidence quoted).
- S-001 carries the mis-scoped postscript (destroy belongs to the missing pass-2
  architecture — S-012); S-005 the U+FFFD anchoring + N-029 residuals; S-002/S-004/S-008
  the og-parity boundaries for auditing their landed fixes; M-001 the re-derived ladder;
  N-001 the inclusive-bound nuance.
