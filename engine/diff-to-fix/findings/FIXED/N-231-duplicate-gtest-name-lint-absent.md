# MINOR N-231 — the duplicate-gtest-name class has NO gate: the build-red shape (the gtest redefinition error) is invisible to the marker lint, the formatter reformats both copies, and 3 of the class's 9 occurrences shipped main red — the root-cause gate is now landed

**Severity:** MINOR (the class convention — M-120 `db228b75`, N-210 `f3da99f1`, N-217 `ed501caa`, N-219 `27bda40e`, N-222, N-227 `d15d32a4`, N-229, N-230 `98bd30d6`; the class's ninth occurrence, third to reach main build-red)
**Module:** tests / CI gating (the never-push-a-red-commit family — the N-210 half is gated, this half was not)
**Parameters affected:** Q68 (audit-trail/CI truth — the duplicate shape breaks the build silently to the marker lint), Q66-adjacent (the gate that would have caught it in sub-second)
**Corpus references:** none (a core-repo build-integrity defect — the N-230 convention)
**Core references (all first-hand this session):** the class's last shape — `Carve.WpdZeroSizeFieldDeclines` at BOTH `tests/unit/test_carve.cpp:3434` and `:3612` in the 98bd30d6 tree (`git show 98bd30d6:tests/unit/test_carve.cpp`, the gtest `TEST()` macro derives one class per name → `error: redefinition of 'class {anonymous}::Carve_WpdZeroSizeFieldDeclines_Test'` — cr_tests unbuildable); the existing gates read: `ci-local.sh:71-81` (the N-210 marker lint — catches `<<<<<<<` only), `.githooks/pre-commit` (secrets + format only), the Werror build (catches it, but the racing lanes' five windows pushed without running it — the S92 handoff's own lesson, "five windows, five un-gated red trees"); the name-uniqueness check: absent everywhere (`grep -rn "unique|duplicate" ci-local.sh .githooks scripts/verify/` → 0 gates)
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule — the gate is the fix)

## Summary

The recurring build-red class has two shapes: committed conflict markers (gated since N-210) and duplicate gtest names (NOT gated until now). The duplicate shape is invisible to every cheap gate: the marker lint cannot see it, clang-format silently reformats BOTH copies, and only the full Werror build catches it — the exact step the racing lanes skip before pushing. The class's ninth occurrence (N-230's tree) shipped main build-red with both lanes' "suite green" claims true only of their own trees.

## The defect (the gate's absence)

1. Two racing lanes append same-named `TEST(Suite, Name)` macros (the convergence shape: both lanes independently name the same behavior).
2. gtest derives one class per name (`Suite_Name_Test`) — two definitions = the redefinition error; cr_tests unbuildable, zero tests runnable.
3. The N-210 marker lint passes; the style sweep reformats both copies without compiling between them; the push ships red.

## The fix (the root-cause gate, landed this session)

1. **`scripts/verify/verify_test_names.py`** — the full-tree duplicate-gtest-name lint: parses every `TEST`/`TEST_F`/`TEST_P` registration under `tests/`, fails on any `(Suite, Name)` pair defined twice, printing the colliding file:line pairs. Deterministic, dependency-free, sub-second (1745 registrations at the landing tree).
2. **Wired into `ci-local.sh` stage 2** — beside the N-210 marker lint, BEFORE the Werror build (the cheapest gate catches the class first).
3. **Wired into `.githooks/pre-commit`** — the full-tree form, NOT a staged-only variant: the class's real shape is lane B's staged file colliding with lane A's already-committed file, which a staged-only check cannot see (the first draft's staged-only form was proven insufficient and replaced — the two-file RED proof below).
4. **A ctest entry** `test_name_uniqueness` — CI enforces it on both lanes (`find_package(Python3 REQUIRED)` — both CI images ship python for the existing doc_truth_gate/stamp_osversion tooling; a python-less environment fails loudly, never silently skips).

## Reproduction / verification (all first-hand this session)

- **RED at the historical tree:** the lint run against the 98bd30d6 tree's test file exits 1 with exactly N-230's pair (`Carve.WpdZeroSizeFieldDeclines: :3434 AND :3612`).
- **RED at the two-file shape:** a staged `test_dupcheck.cpp` colliding with `test_carve.cpp`'s committed name → the pre-commit hook FAILS (exit 1, the pair printed). The staged-only first draft PASSED this shape wrongly — the insufficiency was found by this very test and the full-tree form replaced it.
- **GREEN at the landing tree:** 1745 registrations, 0 collisions (exit 0); the hook's clean path passes; the ctest entry passes in 0.11 s.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this session's N-231 landing — `scripts/verify/verify_test_names.py` + the ci-local.sh stage-2 wiring + the pre-commit full-tree form + the ctest entry)
**Verified by:** the three proofs above (the historical-tree RED, the two-file staged-collision RED via the hook, the clean-tree GREEN + the ctest pass)
**Resolution notes:** The gate catches the class at its ROOT (the collision, before the build that only catches it minutes later). The recurrence lesson recorded with N-230 stands: lanes must run the local gate before pushing — but now the gate cannot be passed by a tree the build would reject on this class. The M-120/N-210 marker half and the N-231 name half now cover both shapes of the build-red class.
