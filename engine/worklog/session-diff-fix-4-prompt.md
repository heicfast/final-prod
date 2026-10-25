# Session prompt — diff-to-fix session 4 (diff-fix-4)

(Saved per SOP; token redacted. Full mandate text preserved in the IM transcript.)

Mandate summary:
- Execute the roadmap in core_recovery/diff-to-fix/ — every finding ACUTE→SEVERE→MAJOR→MINOR→INFORMATIONAL→TRACKED-OPEN.
- Two repos only: core_recovery (modified) + easeus-drw-research (ground truth, read-only). No other repos.
- For every finding: reproduce evidence → re-verify against corpus → classify
  (CONFIRMED-DIVERGENCE / ALREADY-FIXED / FALSE-POSITIVE / OUTDATED-EVIDENCE / NEW-FINDING) → fix → test → benchmark → document → push.
- One finding, one verified fix, one push, one finding-file update. Audit trail preserved (never delete findings).
- No shortcuts, no fallbacks masking failure, no stubs/mocks/TODOs. Production grade only.
- Safety hardenings preserved (destination-overlap refusal, BitLocker legitimate-unlock-only, RawFixer consent, 4Kn, cancellable IRP, SDDL).
- ReFS and LDM stay first-class. Win7-inclusive import gate intact. Signing stays disabled.
- Session goal: resolve 30+ findings minimum; move every touched finding to findings/FIXED/ with Resolution section.

Session-4 environment notes:
- Container had no cmake/clang/gtest; installed cmake 4.4.4 + ninja via pip --user (--break-system-packages); g++ 14.2 used.
- Googletest v1.15.2 via existing FetchContent (network fetch works).
- Baseline at session start: HEAD 4fe9bff, suite 351/351 green (ASan Debug build, ctest).
- Corpus at 03ad46c (unmoved, matches audit).
