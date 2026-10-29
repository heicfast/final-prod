#!/usr/bin/env python3
"""verify_test_names.py - the duplicate-gtest-name lint (N-231).

The M-120/N-210/N-230 class's build-red shape: two racing lanes append
same-named TEST() macros (gtest derives one class per name - two
definitions = the redefinition error; cr_tests unbuildable, zero tests
runnable). The conflict-marker lint (N-210) cannot see it and the
formatter silently reformats BOTH copies - only compilation catches it,
and a lane that pushes without building ships the broken tree (the
class's ninth occurrence reached main red at 98bd30d6, third of the
class to do so).

This lint is deterministic and sub-second: it parses every gtest
TEST/TEST_F/TEST_P registration in the test tree and fails on any
name defined twice, printing the colliding file:line pairs. Gate order
in ci-local.sh: it runs BESIDE the marker lint, BEFORE the Werror
build, so the cheapest possible check catches the class the build
would only catch minutes later.

Exit codes: 0 = unique, 1 = duplicates found, 2 = usage/environment.
"""

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent.parent
TEST_ROOTS = [REPO / "tests"]

# TEST(Suite, Name) / TEST_F(Fixture, Name) / TEST_P(Param, Name) - the
# gtest registration macros. Suite and Name both participate in the
# derived class name (Suite_Name_Test), so the collision key is the
# ordered pair.
TEST_RE = re.compile(r"^\s*TEST(?:_F|_P)?\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s*,\s*([A-Za-z_][A-Za-z0-9_]*)\s*\)")


def main() -> int:
    if not all(p.is_dir() for p in TEST_ROOTS):
        print("verify_test_names: test roots missing", file=sys.stderr)
        return 2
    seen: dict[tuple[str, str], list[str]] = {}
    for root in TEST_ROOTS:
        for src in sorted(root.rglob("*")):
            if src.suffix not in (".cpp", ".cc", ".h", ".hpp"):
                continue
            try:
                text = src.read_text(encoding="utf-8", errors="replace")
            except OSError as exc:
                print(f"verify_test_names: cannot read {src}: {exc}", file=sys.stderr)
                return 2
            for lineno, line in enumerate(text.splitlines(), start=1):
                m = TEST_RE.match(line)
                if not m:
                    continue
                key = (m.group(1), m.group(2))
                seen.setdefault(key, []).append(f"{src.relative_to(REPO)}:{lineno}")
    dups = {k: locs for k, locs in seen.items() if len(locs) > 1}
    if dups:
        print("N-231 duplicate gtest names (the M-120/N-230 class - the build-red shape):")
        for (suite, name), locs in sorted(dups.items()):
            print(f"  {suite}.{name}:")
            for loc in locs:
                print(f"    {loc}")
        return 1
    total = len(seen)
    print(f"test-name uniqueness clean: {total} registrations, 0 collisions (N-231)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
