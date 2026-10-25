# Testing

## Layer 1 - unit (every push, both platforms)
132 tests over the pure core (session-5 audit wave: numbers move fast -
count via `grep -c "TEST(" tests/unit/*.cpp`): fixup, runlist, tables
(GPT/MBR/EBR + the corrected LDM GUIDs), slices, cache (incl. the
failed-blocks-not-pinned regression), retry/bisection, node graph, full
engine set end-to-end on synthetic volumes (NTFS incl. nested attrlists,
resident ADS content, UsnJrnl harvest, FILE0 salvage; FAT; exFAT; ext4
kernel-true layouts + jbd2 chain; HFS+; Btrfs kernel-true tree items;
ZFS OpenZFS-true labels/ZAP/dnodes/compression; ReFS), LDM full-implementation
(database graph, composite VBLKs, simple/spanned/striped/mirrored/RAID-5
with real-parity reconstruction + missing-member failure, orchestrator
dynamic-volume end-to-end), lost-partition discovery, SHA-256 NIST vectors,
carving (grammar + specificity + dedup), CRSF sessions (round-trip +
corruption), orchestrator staging + the R1.6 retry/cache pipe, safe writer
(overlap refusal, collisions, hashing, resume).

## Layer 2 - integration (Windows hosts, scripts/)
rda-driver integration: install the RDA service (scripts from the driver
repo), then `rrecov list` / `rrecov scan --disk 0 --deep` against test
media. Matrix: Win7 SP1 / Win10 / Win11 / WinPE; 512e, 4Kn, SMR; USB
attach/detach during scan (cancel path).

## Layer 3 - benchmark (docs/benchmarking.md, v0.2)
Scan throughput on the transport paths (RDA vs direct), carve rate per
format, session save/load rate.

## Fuzz + CodeQL CI status (M-006, diff-to-fix session)

- **Fuzz job: ENABLED.** The comparative-bench session's skip directive
  ("we are not doing core code changes") no longer holds — the remediation
  session changes parser code on every push. The job builds all 13
  structure-aware harnesses (`fuzz/Makefile`, clang + libFuzzer) and runs the
  45s-per-target smoke campaign with a zero-crash gate; crash corpora upload
  on failure (`fuzz/build/*crash*|leak*|timeout*`). A discovered crash is a
  red main by design — triage it, fix the root cause, re-run.
- **CodeQL job: deliberately deferred** (`if: false`, recorded not silent).
  Rationale: the comparative-bench slot rationale still applies to the
  static-analysis lane (it never blocked a release and its findings were
  informational); revisit when the remediation wave closes. This is the
  deferral the M-006 finding asked to be documented.
