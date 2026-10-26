#!/usr/bin/env bash
# build.sh — the Linux-subset product build (developer loop + Linux CI lanes).
# The FULL product (driver, installer, Tauri bundle) is Windows-only: run
# build.ps1 on Windows (docs/BUILD.md). This script runs everything that is
# meaningful on Linux:
#   - contracts validation (boundary truth)
#   - engine build + full test suite (linux presets; the pure core's CI home)
#   - ui static checks (typecheck, lint, unit tests) — everything except the
#     Tauri/cargo Windows shell and the installer lanes.
set -euo pipefail
export PATH="/home/z/.local/bin:$PATH"
command -v cmake >/dev/null || { echo "cmake not found" >&2; exit 2; }

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION="$(cat "$REPO_ROOT/VERSION")"
cd "$REPO_ROOT"

TARGET="${1:-all}"
echo "== final-prod Linux build == version=$VERSION target=$TARGET"

run_contracts() {
  echo "[contracts] boundary-truth validation"
  node contracts/validate.mjs
}

run_engine() {
  echo "[engine] CMake linux-release + tests"
  cd "$REPO_ROOT/engine"
  cmake --preset linux-release
  cmake --build --preset linux-release --parallel "$(nproc)"
  ctest --test-dir build/linux-release --output-on-failure
  cd "$REPO_ROOT"
}

run_ui_static() {
  echo "[ui] typecheck + lint + unit tests (frontend; no Windows shell here)"
  cd "$REPO_ROOT/ui"
  pnpm install --frozen-lockfile
  pnpm typecheck
  pnpm lint
  pnpm test
  cd "$REPO_ROOT"
}

case "$TARGET" in
  contracts) run_contracts ;;
  engine)    run_contracts; run_engine ;;
  ui)        run_contracts; run_ui_static ;;
  all)       run_contracts; run_engine; run_ui_static ;;
  *) echo "unknown target: $TARGET (use all|contracts|engine|ui)" >&2; exit 2 ;;
esac

echo "== Linux build complete ($TARGET) =="
