#!/usr/bin/env bash
# ci-local.sh - the Unix local validation entry point (docs/CI-LOCAL.md).
#
# Runs every check the CI's local-mirror job runs, in the same order, with
# the same strictness, BEFORE a push (the never-push-a-red-commit rule,
# docs/CONTRIBUTING.md). A fresh contributor runs this once and knows.
#
# Usage:
#   ./ci-local.sh            # the full default gate (fast; the pre-push set)
#   ./ci-local.sh --san      # + the TSan/UBSan sanitizer lanes
#   ./ci-local.sh --fuzz     # + the libFuzzer smoke campaign (~11 min)
#   ./ci-local.sh --all      # everything the Linux CI matrix runs
#
# Exit code: 0 = green (push), 1 = red (fix before pushing; each failure
# prints the exact files/issues and the fix command).

set -u
cd "$(dirname "$0")"

DO_SAN=0; DO_FUZZ=0
for arg in "$@"; do
  case "$arg" in
    --san) DO_SAN=1 ;;
    --fuzz) DO_FUZZ=1 ;;
    --all) DO_SAN=1; DO_FUZZ=1 ;;
    *) echo "unknown option: $arg (use --san / --fuzz / --all)"; exit 1 ;;
  esac
done

RED='\033[0;31m'; GRN='\033[0;32m'; YLW='\033[1;33m'; RST='\033[0m'
fail() { echo -e "${RED}[FAIL]${RST} $1"; FAILED=1; }
ok()   { echo -e "${GRN}[ ok ]${RST} $1"; }
info() { echo -e "${YLW}[info]${RST} $1"; }
FAILED=0

# ---------------------------------------------------------------- 1. format
echo "== [1/10] clang-format check =="
if command -v clang-format >/dev/null 2>&1; then
  BAD=$(find src include tests -name '*.h' -o -name '*.hpp' -o -name '*.cpp' -o -name '*.c' \
        | xargs clang-format --dry-run -Werror 2>&1 | grep "error:" | head -20)
  if [ -n "$BAD" ]; then
    fail "files need formatting (first 20 violations):"
    echo "$BAD"
    echo "  fix: find src include tests \( -name '*.h' -o -name '*.cpp' -o -name '*.c' \) -exec clang-format -i {} +"
  else
    ok "clang-format clean"
  fi
else
  fail "clang-format not installed. fix: pip install clang-format (or apt install clang-format)"
fi

# ------------------------------------------------------------------ 2. lint
echo "== [2/10] dead-surface + architecture lints =="
# N-041: deleted dead-surface symbols must never return.
if grep -rn --include='*.cpp' --include='*.h' --include='*.c' \
     -e 'RdaClient::' -e 'class RdaClient' -e 'volume_letter_to_disk' \
     src include tests tools bench fuzz 2>/dev/null; then
  fail "deleted dead-surface symbols found (N-041)"
else
  ok "N-041 dead-surface lint clean"
fi
# Driver-only architecture (docs/DRIVER-REQUIRED.md): the removed transport
# primitives must never return to production code.
if grep -rn --include='*.cpp' --include='*.h' --include='*.c' \
     -E 'rda_available\s*\(|win_pe_environment\s*\(|open_harddisk_volume\s*\(|prefer_rda' \
     src include 2>/dev/null | grep -vE '^\S+:[0-9]+:\s*(/\*|\*|//)'; then
  fail "removed fallback primitives found in production code (driver-only architecture)"
else
  ok "driver-only architecture lint clean (no rda_available/win_pe_environment/open_harddisk_volume/prefer_rda)"
fi
# N-210: committed conflict markers must never survive a merge in ANY text
# file (the M-120 class recurred in markdown, which -Werror cannot see).
# The one sanctioned carrier is M-120's own evidence quote.
if MARKERS=$(grep -rln '^<<<<<<< ' --include='*.md' --include='*.cpp' --include='*.h' \
     --include='*.c' --include='*.hpp' --include='*.yml' --include='*.txt' --include='*.sh' \
     src include tests tools bench fuzz docs scripts diff-to-fix worklog worklog.md 2>/dev/null \
     | grep -v 'M-120-committed-conflict-markers-red-main'); then
  fail "live conflict markers committed (the M-120/N-210 class):"
  echo "$MARKERS"
else
  ok "conflict-marker lint clean (M-120/N-210 class)"
fi
# N-231: duplicate gtest names - the class's build-red shape that the
# marker lint cannot see and the formatter silently reformats (two
# same-named TEST() macros = the gtest redefinition error; the ninth
# occurrence reached main red at 98bd30d6). Deterministic, sub-second,
# BEFORE the Werror build so the cheapest gate catches it first.
if python3 scripts/verify/verify_test_names.py; then
  :
else
  fail "duplicate gtest names (the M-120/N-230 class - N-231): see the pairs above"
fi
# No static analyzer (cppcheck) is configured in this repo's toolchain yet:
# reported honestly, never as a silent pass (docs/CI-LOCAL.md section 4).
info "static analysis: not configured in this repo yet (no cppcheck/clang-tidy config) - tracked, not silenced"

# ------------------------------------------------- 3. Werror build + tests
echo "== [3/10] warnings-as-errors build (gcc -Wall -Wextra -Werror -Wshadow; global in CMakeLists) =="
if cmake --preset linux-release >/dev/null 2>&1 && \
   cmake --build --preset linux-release -j"$(nproc)" >/tmp/ci-local-build.log 2>&1; then
  ok "linux-release build clean"
else
  fail "build failed (gcc Werror) - first errors:"
  grep -E "error:|warning:" /tmp/ci-local-build.log | head -20
fi

echo "== [4/10] unit + contract tests (the full ctest suite incl. cr_ui ABI + win_syntax_check) =="
if [ -d build/linux-release ]; then
  if (cd build/linux-release && ctest --output-on-failure -j"$(nproc)" >/tmp/ci-local-test.log 2>&1); then
    ok "ctest green ($(grep -oE '[0-9]+% tests passed' /tmp/ci-local-test.log | tail -1))"
  else
    fail "ctest red - first failures:"
    grep -E "Failed|failed" /tmp/ci-local-test.log | head -10
    echo "  rerun: (cd build/linux-release && ctest --output-on-failure -R <name>)"
  fi
else
  fail "no build directory - the build step must run first"
fi

# -------------------------------------------------- 5. repo truth self-tests
echo "== [5/10] report-schema validation + Win7 PE gate self-test =="
if python3 tools/validate_report.py "$(python3 -c 'import tempfile; print(tempfile.gettempdir())')/cr_report_sample.json" >/dev/null 2>&1; then
  ok "G-09 report schema validation"
else
  fail "report schema validation failed (run the Report.EmitsCiSchemaSample test first, or see tools/validate_report.py)"
fi
if python3 tools/test_verify_pe_um.py >/dev/null 2>&1; then
  ok "Win7 PE gate self-test"
else
  fail "Win7 PE gate self-test failed (tools/test_verify_pe_um.py)"
fi
if python3 tools/test_ceiling_metric.py >/dev/null 2>&1; then
  ok "GAP-S6-5 physical-ceiling scorer self-test"
else
  fail "physical-ceiling scorer self-test failed (tools/test_ceiling_metric.py)"
fi

# --------------------------------------------------------- 6. dependency pin
echo "== [6/10] dependency pinning (vcpkg manifest mode) =="
if [ -f vcpkg.json ] && python3 -c "import json;json.load(open('vcpkg.json'))" 2>/dev/null; then
  ok "vcpkg.json manifest present and valid (manifest mode = the repo's pinning policy)"
else
  fail "vcpkg.json missing or invalid"
fi

# ------------------------------------------------------------ 7. doc links
echo "== [7/10] documentation link check (relative links in docs/ + README) =="
LINK_BAD=0
while IFS= read -r -d '' md; do
  dir=$(dirname "$md")
  grep -oE '\]\(([^)h][^)]*\.md[^)]*)\)' "$md" 2>/dev/null | sed -E 's/^\]\(//; s/\)$//; s/#.*$//' | while IFS= read -r link; do
    [ -z "$link" ] && continue
    if [ ! -e "$dir/$link" ]; then
      echo "  broken: $md -> $link"
      LINK_BAD=1
    fi
  done
done < <(find docs -name '*.md' -print0; [ -f README.md ] && printf '%s\0' README.md)
if [ "$LINK_BAD" = "0" ]; then ok "doc links resolve"; else fail "broken relative doc links (above)"; fi

# ----------------------------------------------------------- 8. secrets scan
echo "== [8/10] secrets scan (working diff vs origin/main) =="
BASE=$(git merge-base HEAD origin/main 2>/dev/null || echo HEAD)
SECRET_HITS=$(git diff "$BASE" 2>/dev/null | grep -nE "ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16}|-----BEGIN [A-Z ]*PRIVATE KEY-----" | head -5)
if [ -n "$SECRET_HITS" ]; then
  fail "secrets found in the diff:"
  echo "$SECRET_HITS"
  echo "  fix: remove the secret, rotate it, and never commit it (docs/SECURITY.md)"
else
  ok "no secrets in the diff"
fi

# ------------------------------------------------------ 9. commit message
echo "== [9/10] commit message convention (type(scope): lowercase ...) =="
MSG_BAD=0
while IFS= read -r rev; do
  MSG=$(git log -1 --format='%s' "$rev")
  if ! echo "$MSG" | grep -qE '^(feat|fix|repair|docs|worklog|bench|ci|style|audit|merge|test|refactor|perf|chore|build|revert)(\([A-Za-z0-9._-]+\))?!?: .+'; then
    echo "  non-conforming: $rev $MSG"
    MSG_BAD=1
  fi
done < <(git log --format='%H' origin/main..HEAD 2>/dev/null)
if [ "$MSG_BAD" = "0" ]; then ok "commit messages conform"; else fail "fix the commit subjects above (docs/CONTRIBUTING.md convention)"; fi

# ------------------------------------------------------- 10. hook presence
echo "== [10/10] git hooks path =="
if [ "$(git config core.hooksPath 2>/dev/null)" = ".githooks" ]; then
  ok "core.hooksPath = .githooks (pre-commit/pre-push active)"
else
  info "hooks not installed (optional but recommended): git config core.hooksPath .githooks"
fi

# ------------------------------------------------------- optional lanes
if [ "$DO_SAN" = "1" ]; then
  echo "== [+] TSan lane =="
  cmake -S . -B build/tsan -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCR_ENABLE_ASAN=OFF \
    -DCMAKE_CXX_FLAGS="-fsanitize=thread -fPIE -DCR_SANITIZER_LANE" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread -pie" \
    -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread" >/dev/null 2>&1 \
    && cmake --build build/tsan -j"$(nproc)" >/dev/null 2>&1 \
    && (cd build/tsan && ctest -j2 >/tmp/ci-local-tsan.log 2>&1) \
    && ok "TSan green" || fail "TSan lane red (see /tmp/ci-local-tsan.log)"
  echo "== [+] UBSan lane =="
  cmake --preset linux-ubsan >/dev/null 2>&1 \
    && cmake --build --preset linux-ubsan -j"$(nproc)" >/dev/null 2>&1 \
    && ctest --preset linux-ubsan -j2 >/tmp/ci-local-ubsan.log 2>&1 \
    && ok "UBSan green" || fail "UBSan lane red (see /tmp/ci-local-ubsan.log)"
fi
if [ "$DO_FUZZ" = "1" ]; then
  echo "== [+] fuzz smoke campaign =="
  if command -v clang++ >/dev/null 2>&1; then
    make -C fuzz all CXX=clang++ >/dev/null 2>&1 && make -C fuzz run CXX=clang++ >/tmp/ci-local-fuzz.log 2>&1 \
      && ok "fuzz smoke zero-crash" || fail "fuzz campaign red (see /tmp/ci-local-fuzz.log)"
  else
    fail "clang++ not installed for the fuzz lane"
  fi
fi

echo
if [ "$FAILED" = "0" ]; then
  echo -e "${GRN}LOCAL GATE: GREEN - ready to push${RST}"
  exit 0
else
  echo -e "${RED}LOCAL GATE: RED - fix the above before pushing (never push a red commit)${RST}"
  exit 1
fi
