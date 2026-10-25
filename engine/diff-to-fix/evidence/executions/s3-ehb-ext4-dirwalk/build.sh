#!/bin/sh
# s3-ehb-ext4-dirwalk — build + run (S3-EHB, read-only audit; artifacts live
# only under diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk/).
# Toolchain: g++ (any C++20); no cmake needed for this 4-TU probe.
set -e
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"   # core_recovery repo root
cd "$ROOT"
g++ -std=c++20 -Iinclude -Isrc \
    diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk/probe.cpp \
    src/core/ext4/cr_ext4_engine.cpp \
    src/core/tree/cr_node.cpp \
    src/core/err/cr_error.cpp \
    -o diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk/probe
./diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk/probe \
    | tee diff-to-fix/evidence/executions/s3-ehb-ext4-dirwalk/output.txt
