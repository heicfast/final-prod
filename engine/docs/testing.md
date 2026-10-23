# Testing

## Layer 1 — unit and ABI (every push, both platforms)

The GoogleTest suite covers fixups, runlists, GPT/MBR/EBR and LDM tables,
bounded devices, cache/retry behavior, the node graph and snapshots, every
filesystem engine, lost partitions, carving, CRSF, orchestration, query,
preview, writer/reporting, health parsing and RawFixer. Fixtures include live
and deleted trees, fragmented streams, hard links, ADS, sparse runs, damaged
metadata, compressed extents, dynamic-volume mappings and rollback paths.

`cr_ui_abi_check` is a plain-C11 consumer that scans a real FAT16 image through
`cr_ui`, exercises snapshots/query/preview/recovery/session round-trips and
byte-verifies recovered content. It runs on Linux and Windows.

## Layer 2 — Windows CLI and device integration

`tests/cli/cli_contract.ps1` consumes the CRSF produced by the C ABI scan and
executes every named `rrecov` command. It checks exit codes, JSON output,
strict corruption rejection, tolerant loading, byte-identical atomic save and
`repair`/`fix` compatibility. It is registered in CTest, not a manual script.

Seeded-VHD workflows exercise live/deleted/formatted/overwritten/fragmented
cases and driver-installed transport paths. The comparison workflow asserts
that the RDA path was actually used, preventing a silent direct-handle leg.
The device matrix includes Win7 SP1 through Win11/WinPE posture, 512e/4Kn and
attach/detach cancellation behavior where the runner supports it.

## Layer 3 — sanitizers, fuzzing and static analysis

Every push runs ASan, UBSan (`-fno-sanitize-recover=undefined`,
`halt_on_error=1`) and TSan as separate builds, structure-aware libFuzzer smoke
campaigns for every parser family, and CodeQL. Findings fail the job; there is
no continue-on-error or analyzer suppression lane. MSVC builds use `/W4 /WX`;
shipped PE files are stamped and checked against the Win7 import allowlist.

## Layer 4 — benchmark

`docs/benchmarking.md` defines transport throughput, seeded recall/precision,
false-positive, memory/latency and cancellation measurements. Final numbers
and MATCH/EXCEEDS/BELOW/NOT-COMPARABLE verdicts live in
`docs/BENCHMARK-REPORT.md`; any BELOW verdict blocks release until fixed.
