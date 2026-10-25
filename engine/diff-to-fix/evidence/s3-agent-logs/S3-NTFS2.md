# S3-NTFS2 — NTFS remainder deep pass (agent scratch report)

Agent: S3-NTFS2 (read-only parity auditor). Core HEAD `34b3441`, corpus `03ad46c`.
ID range: S-022, M-028, M-029, N-039, N-040, N-041, N-042, I-015 (in order).

## (a) What I read

(kept dense; updated per item)

- PROTOCOL.md (full), SEVERITY-SCALE.md (full), modules/ntfs.md (full)
- Covered findings: S-012, N-024, N-011, M-007, M-008, I-001 (full reads)
- Dead agent USN probe: evidence/executions/s3-ntfs-usn-reason/ (run-output.txt + probe source, full)

## (b) Findings filed

- **N-039** (MINOR, grouped) — NTFS $UsnJrnl harvest: og gate `reason & 0x200` (compression-change bit; harvest-all flag dead code, write-once-zero verified exhaustively); core gates on true FILE_DELETE 0x20000 (winioctl.h). Two-sided + bounds deltas (og 512B rlen cap / no version check / no journal cap; core 64 MiB cap / V2+V3 / UTF-16). Disposition of dead agent's USN probe: premise inverted (its "realdelete" 0x80000200 = CLOSE|COMPRESSION_CHANGE; true delete = 0x80020000); probe output re-used as core-behavior evidence. Evidence: decompiled-quotes/s3-ntfs-usn-gate.md + executions/s3-ntfs-usn-reason/. Findings/MINOR/N-039-ntfs-usnjrnl-harvest-gate-mask-and-bounds.md

## (c) Parameters newly verified MATCH

(none yet)

## (d) UNVERIFIED residuals

(none yet)

## (e) Verdicts appended

(pending — will append USN row + attribute-dispatch rows to modules/ntfs.md at the end)

## Item status

1. $EA/$EA_INFORMATION — PENDING
2. EFS/$LOGGED_UTILITY_STREAM — PENDING
3. LZX/XPRESS engines — PENDING
4. $I30 index-derived deleted entries — PENDING
5. USN reason-flags disposition — **DONE → N-039 filed** (og gate decoded first-hand :20098; harvest-all dead; probe premise corrected)
6. $VOLUME_NAME/$VOLUME_INFO — PENDING (session-2 module file already records both-ignore MATCH; will re-confirm cheaply)
7. Full attribute dispatch table rows — PENDING

---

## Lead completion note (2026-10-26 — items 1-3, 6-7 closed by the S3 lead after the agent's deadline)

Items 1 ($EA/$EA_INFORMATION), 2 (EFS), 3 (LZX/XPRESS), 6 (volume attrs) and 7
(dispatch-table rows) were closed by the lead with targeted greps (no whole-file
reads): og has ZERO type comparisons against 0xE0/0xD0/0x60/0x70/0x100-as-type;
core dispatches 0x10/0x20/0x30/0x80/0xA0 only — MATCH-by-omission on $EA, volume
attrs, $LOGGED_UTILITY_STREAM; EFS MATCH-by-omission (type level; content grading
UNVERIFIED, no fixture); compression MATCH (both LZNT1-only: og ladder 0x3f/0x800/
0x8000 at :2916/:3814, core cr_decomp.cpp:131; LZX/XPRESS = shared limitation via
the sparse+WOF path outside both scopes). Verdict rows appended to modules/ntfs.md.
Item 4 ($I30/INDX) = S-022 (filed by the agent); item 5 (USN) = N-039 (filed, with
the dead-agent probe disposition). Items S-022/N-039 module rows appended by the
lead. M-028/M-029/N-040..N-042/I-015 unused (no further divergences found in the
remainder scope) — returned to the pool.
