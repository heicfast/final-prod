# Session-42 continuation-audit prompt — ADDENDUM: the focus battery (saved per the mandate; token redacted; IM gateway metadata stripped)
# Baseline at save: core main = b26017a7 (the S42 lane-open tip), corpus = be0cfe9.
# Lane: session-42 continuation audit lane, W2 re-scoped by this addendum. The standing SOP text
# is preserved verbatim in worklog/session-diff-audit-prompt.md and session-42-continuation-audit-prompt.md.

Continuation directive (this session, per the user, verbatim except token):
"This is second session of this work, first is already completed, now you have go look for other
things which session 1 missed and continue working and remove the fixed things and go more deep, do
more comparison on og-codebase and our core and mark all remaining features, implementations,
functions, as finding too., Special focus on file reconstruction, timestamp, recovery chances
status, partition recovery , partition recovered api support format,etc, preview, how scanning
works on og-codebase, whats quick scan, whats advanced/deep scan, which triggers after what
parameter, how both combine, collaboration, etc independently verify and validate all, check in
our core, if diff found mark as severe finding"

## The S42 lane's operative reading of this addendum (W2 re-scope):

The six focus areas become dedicated deep-dive lanes, each independently verified and validated
against both the og-codebase/re-workspace corpus and the core at `b26017a7`:

1. **File reconstruction** — the og's content-reconstruction chain on recovery (the export loop's
   engine-side reads, IInWindowStream/IFileProperties-mediated content supply, damaged-extent and
   partial-read semantics, the "Reconstructed" result-tree branch) vs the core's recovery/writer
   path.
2. **Timestamp** — per-FS timestamp decode (granularity, epoch, field layout), UTC/local
   semantics, carriers, persistence, display-layer semantics.
3. **Recovery chances status** — the og's CRecoverability grading engine (grade set, decision
   rules, the device.kind adjustment, the NTFS/ReFS/Btrfs deep validation path, per-selection
   re-grade, the RecoveryAssistant statistics surface) vs the core's cr_grading model.
4. **Partition recovery + the recovered-partition API support format** — the og's lost-partition
   discovery/recovery flow AND the surface through which recovered partitions are exposed
   (scan-target insertion, browse capability, session/API shape, supported formats).
5. **Preview** — the og's preview subsystem (text/image/video/archive/PDF/office surfaces,
   eligibility, size caps, the thumbnail protocol) vs the core's preview module.
6. **Scan-mode semantics** — how scanning works on the og: what quick scan is, what
   advanced/deep scan is, which parameters trigger each (and which trigger the auto-advance),
   how the two combine (merge, tree shape, StartSector reuse), and how the engines collaborate
   (multi-engine scan sets, cross-engine dedup, RAW engagement gates).

Severity discipline per the user's directive: a verified behavioral difference in these focus
areas that users/recovery results observe is filed at SEVERE; supporting-shape differences file
at their own scale-consistent severity. Every finding independently verified (corpus citation +
core cite at the audited hash; execution where feasible).

Read-only guarantee: diff-to-fix/ + worklog only, per-commit.
