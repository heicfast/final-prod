# Session S64 — diff-to-fix execution lane (the MINOR-first continuation)

Saved at lane open (token redacted per SOP). Core at open: `24e480c4`
(S60-B/S63 windows absorbed). Corpus: `be0cfe9`. Baseline suite at open:
**1485/1485 green** (Linux, `build-local`, full ctest).

## The mandate (user, verbatim excerpts)

- Senior systems engineer mandate: execute the roadmap in `core_recovery/diff-to-fix/`.
- For every finding — ACUTE, SEVERE, MAJOR, MINOR, INFORMATIONAL, TRACKED-OPEN —
  independently verify the finding against the corpus and the running code before
  any change. A false finding is corrected in the audit trail rather than "fixed"
  by breaking good code.
- Two repos only: core (`heicfast/core_recovery`) and corpus
  (`heicfast/easeus-drw-research`, read-only ground truth). Token env/git-config
  only, never committed, never logged.
- Re-implement from understanding; never paste decompiled code; never copy
  proprietary strings; cite the corpus artifact in the commit message.
- Every fix paired with a test that fails before and passes after.
- Safety hardenings are not slop — preserve them (destination overlap refusal,
  BitLocker legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- One finding, one verified fix, one push, one finding-file update.
- **User's ordering override for this lane: "Fix all the audit findings
  (priority to minor first) one by one... Aim for at least 20+ fixes in this
  session."** Fixed findings move to `diff-to-fix/findings/FIXED/`; all docs,
  counts, matrix, module verdicts updated properly.
- No shortcuts, no fallbacks that mask failure, no stub/mock/TODO/placeholder.
- The audit trail is the deliverable.

## SOP loop (per finding)

Verify → Reproduce → Classify (CONFIRMED / ALREADY-FIXED / INVALID / SUPERSEDED)
→ Fix → Re-review → Test (RED→GREEN proof) → Benchmark (perf-tagged only) →
Document (finding Resolution + matrix + module doc + SUMMARY + PARITY-DELTAS if
deliberate divergence) → Push → Loop.

## Board at lane open (recounted first-hand)

- findings/MAJOR/: 20 files (M-073…M-146; M-086/M-087/M-089 driver-repo-resident
  per OQ-S10-3 grant boundary)
- findings/MINOR/: 13 files (N-107…N-203; N-107/N-127 driver-repo-resident)
- findings/FIXED/: 151 files
- tracked-open/: 4 files (OQ-S10-1, OQ-S10-3, OQ-S4-3, OQ-S52-1)
- Next free IDs: S-067+, M-148+, N-210+, I-048+, A-015+
