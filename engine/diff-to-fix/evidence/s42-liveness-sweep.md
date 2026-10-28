# s42 evidence — the W1 remove-the-fixed liveness discharge + the focus batteries' residue dispositions

**Session-42 continuation lane, 2026-10-29. Core @ `b26017a7` (audited HEAD); corpus @ `be0cfe9`.**

## §1 The git-ground proof

`git diff 25e408b8..HEAD -- src/ include/` is **EMPTY** — the source tree at the audited HEAD is byte-identical to the post-rebase point where the S12 lane re-verified every defect site LIVE (its reconcile commit's own record). The only commits in the window are `d4776278` (worklog) and `b26017a7` (test-expectation hygiene + worklog). Therefore the S12 liveness sweep's verdicts (41 LIVE / 4 PARTIAL / 5 driver-repo / 5 register / no unretired fix) carry to HEAD verbatim for all source-code findings; the only delta is the test file, which this session re-executed (see the execution log — one test now fails; N-185 owns it, and it is a TEST defect, not a finding regression).

## §2 The SEVERE spot-probes at HEAD (this session, first-hand)

- **S-054** (the N-013 re-landing): `grep set_destination_spans|volume_extents src/ include/` → the machinery exists (`cr_safewriter.cpp:72-75`, `cr_transport.h:126/141`) but **zero production call sites** arm it (`rrecov.cpp` / `cr_ui.cpp`: no hits) — STILL LIVE.
- **S-040** (image create): no `cr_ui_image_create` / image-create surface in `src/ui/` or `cr_ui.h` — STILL LIVE.
- **S-048** (carve volume grouping): the carve root is still the flat `Other Deleted Files (Carved)` / configured name (`cr_scan.cpp:146-160`), no per-volume grouping — STILL LIVE.
- **S-050** (repair line): only the S-008 pre-scan repair exists (`cr_ui.cpp:510-536`); no Fixo/repair-during-recovery line — STILL LIVE.

## §3 The board reconciliation

Filesystem truth at HEAD: **53 active (4 SEVERE · 23 MAJOR · 17 MINOR · 9 INFO) + 19 FIXED-file + 3 tracked-open**. The S12-close "55 active (4/23/19/9)" minus the N-151/N-153 retirements (both in FIXED/, ledger rows verified) = the current 53 — the board was truthful entering this lane. This session's filings add: +4 SEVERE (S-058..S-061), +5 MAJOR (M-133..M-137), +6 MINOR (N-180..N-185), +1 INFO (I-044), +3 tracked-open (OQ-S42-1/2/3) → **69 active (8 · 28 · 23 · 10) / 6 tracked-open** at the lane's filings.

## §4 The focus batteries' already-covered ground (verified, not re-filed)

- **Timestamps**: S-011 (exFAT local-as-UTC), N-021 (FAT invalid-date→0), M-029/M-041/M-051/S-031 (carriers/provenance), I-006/I-007/I-032 (precision registers), N-116/N-090 (session time semantics) — all verified present in the tree/tests this session (spot-checked `cr_exfat_quick.cpp:176-200`, `cr_lfn.cpp:166-175`, `test_exfat_volume.cpp` pins). Residue: OQ-S42-1 (SI/FN precedence), OQ-S42-2 (Btrfs nsec), S-060 (restoration).
- **Partition recovery**: M-017/M-118/M-119/S-051/M-104/S-029/M-042/M-035 own the scoring/three-source/boundary/target/apply/resume/consistency slices — all still active and verified live this session (the grep set in §2 + the M-118 MountPoints2 absence re-proven in the S12 record). Residue: M-137 (the surfaced-shape batch).
- **Preview**: M-108 (text eligibility, landed + verified: `cr_preview_text_exts_data.cpp` pool), N-144 (text-snapshot thumbnails, active), M-115 (promotion gating, active), I-028 (superset register) — the engine-side split (content supply/eligibility vs render) verified current. Residue: N-184 (media metadata), the doc-truth cluster in N-180.
- **Scan semantics**: SCAN-ENGINE-DELTA rows Q1-Q11/A1-A6/C1-C9 re-walked via the focus agents; the residue is exactly N-181/N-182/I-044 (this session's filings) — nothing else new was found beyond those registrations.

## §5 The verification-protocol statement

Every finding filed this session cites: decompile lines re-read first-hand (the four worksheets) or core file:line re-read first-hand; the executed build+suite record; or the git-diff ground above. No claim rests on an agent's unverified reading (the two agent claims that failed first-hand verification were dropped: the "funnel" attribution of the destination dialog, and an agent-transcribed polarity cite that my own read refuted — the S-058 erratum chain).
