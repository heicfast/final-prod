# Session fix mandate — prompt record (token redacted)

Session type: fix session (the diff-to-fix roadmap). Mandate: for every finding —
ACUTE/SEVERE/MAJOR/MINOR/INFORMATIONAL/TRACKED-OPEN — independently verify,
fix, prove with tests, mark fixed in the audit trail with evidence. Corpus is
ground truth (easeus-drw-research, read-only). Core repo: heicfast/core_recovery.
Token redacted per hygiene rule (env/git config only).

Key SOPs: verify → reproduce → classify → fix → re-review → test → benchmark →
document → push → loop. One finding, one verified fix, one push. No shortcuts,
no stubs, no hallucinated parity. Windows direct I/O: multiples of 4KB/64KB.
Win7-inclusive import gate. Signing stays disabled.

First 20 TODOs: T1 pull+record, T2 read diff-to-fix, T3 read core, T4-T5 acute
classify+fix, T6-T7 severe, T8-T9 major, T10 perf baseline/fix/rebench, T11-T12
minor, T13 informational, T14 tracked-open, T15 matrix, T16 summary, T17 module
docs, T18 CI sweep, T19 discovered tests, T20 tag release + handoff.

Priority note from user: fix all findings, priority to major first, aim 20+ fixes.
