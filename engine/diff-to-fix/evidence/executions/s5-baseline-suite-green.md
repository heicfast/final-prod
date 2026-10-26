# Session-5 baseline — full suite green at the audited commit

- Core commit audited this session: `7cf6754` (one commit past session-4 close `d870dd3`:
  the S-019 BitLocker-partition fix, which landed with its own audit trail and 3 new
  tests).
- Corpus: `03ad46c` (unmoved since session 1).
- Command sequence (executed 2026-10-26, this session):

```
cmake --preset linux-release -B /tmp/build-s5
cmake --build /tmp/build-s5 -j8
ctest --test-dir /tmp/build-s5
```

- Result: **366/366 tests passed, 100%** (includes `cr_ui_abi_check`, the S-019 additions,
  and the reghive/bitlocker lanes). Build clean, no warnings surfaced in the log tail.
- Purpose: baseline for the session's read-only findings (no code changed this session;
  this record pins that the audited tree is healthy, so every finding's repro is against a
  green baseline).
- Log: preserved at the session scratch area (`baseline-test.log`, outside the repo).
