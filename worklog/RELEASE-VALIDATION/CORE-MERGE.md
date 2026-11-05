# CORE-MERGE — gate §2 evidence: core_recovery merged, both histories preserved

Written: 2026-11-05 (consolidation release session). Every fact below was
re-measured from the repository at d166147b; commands are reproducible as
written.

## The two history-preserving merges

| Merge | SHA | Parents | Merge-base | What it did |
|---|---|---|---|---|
| P1 — UI re-base | 66fdfc47 | 1bae59fe (final-prod lane) + 5623aafb (rewritten recovery-wizard-ui-new v3 @dd2782c) | d7413e03 (rewritten f4a8f8a) | Adopted the fresh wizard base; the 40 driver-era UI commits wiped; the fresh tree won every ui/ divergence |
| P2 — engine advance | 283a5d90 | c9730310 (final-prod lane) + 7f5abd60 (rewritten core_recovery main @12f3edf7) | 7888565c (rewritten 22e9657) | 1092 commits of core history ride in under engine/; core won all 26 divergence points |

Both are TRUE merge commits (two parents each) — **no squash, no rebase,
no history rewritten on the pushed lane**. The upstream repos' own
histories remain fully reachable from main:

- `git rev-list --count 283a5d90^2 --not 283a5d90^1` → **1092** core-side
  commits reachable through the current tree.
- `git log --follow -- engine/include/cr/cr_ui.h` reaches core's own
  history — e9ff685f *"feat(direct-access)!: the kernel driver is replaced
  by direct raw i/o — \\.\PhysicalDriveN from user mode under administrator
  privilege"* is 4 commits deep in that file's chain, behind M-101, M-078,
  M-075, M-154 and the rest of core's line.
- `git log --follow -- ui/src/app/shell.tsx` reaches recovery-wizard-ui-new's
  own history (d7413e03 "restore all quality gates to green + bump 0.3.3",
  the v4 menu/health/backup commits, ...).

## Merge fidelity — the byte-identity proof

The rewritten merge parent's `engine/` subtree vs core_recovery@12f3edf7's
root tree:

```
diff <(git ls-tree -r 283a5d90^2 engine/ | sed 's|engine/||' | awk '{print $1,$3,$4}' | sort) \
     <(cd ../core_recovery && git ls-tree -r 12f3edf7 | awk '{print $1,$3,$4}' | sort)
# output: EMPTY (0 lines)
```

Every blob SHA and path in the merged subtree is identical to the source
repository's head at merge time. Nothing was dropped, nothing was altered
in transit; the only final-prod-side deltas are the documented follow-up
commits (01d39b33's Windows build-fix re-applies and CI adoption).

## Contracts: core_recovery is the single authority

- The ABI the bridge links is **v4** (`cr_ui_abi_version()` == 4; 95
  CR_UI_API exports per the contracts README) — the engine/ in this tree
  IS core@12f3edf7 (proof above).
- contracts/ data was re-pointed to the v4 vocabulary in the same pushes:
  40 error rows (incl. 0x500C0001 ERR_ELEVATION_REQUIRED), 12 events,
  92-export README count, 33-command manifest — `node contracts/validate.mjs`
  GREEN (phase 1 + phase 2) on every push since 01d39b33.
- Duplicate/divergent contract copies: the old v2-era bridge and its
  contracts died with the P1/P4 re-base; `ui/src/connector/contract.ts` is
  the UI's only vocabulary source and validate.mjs cross-checks it against
  the engine headers + error-codes.json on every contracts-ci run.

## Where the old final-prod-local engine patches went

Per INTEGRATION-PLAN §2.2, six local engine patches were dropped at the
merge (testPreset `-C`, applocal-deps, deploy retries were re-applied
bridge-side in 01d39b33; the rest filed as BF-01..BF-06 in
worklog/BEHAVIOR-FINDINGS.md — behavior decisions, not silent losses).
