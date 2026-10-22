# Testing

## Layer 1 - unit (every push, both platforms)
54 tests over the pure core: fixup, runlist, tables (GPT/MBR/EBR), slices,
cache, node graph, NTFS/FAT/exFAT end-to-end synthetic volumes, lost-partition
discovery, SHA-256 NIST vectors, carving (grammar + specificity + dedup),
CRSF sessions (round-trip + corruption), orchestrator staging, safe writer
(overlap refusal, collisions, hashing).

## Layer 2 - integration (Windows hosts, scripts/)
rda-driver integration: install the RDA service (scripts from the driver
repo), then `rrecov list` / `rrecov scan --disk 0 --deep` against test
media. Matrix: Win7 SP1 / Win10 / Win11 / WinPE; 512e, 4Kn, SMR; USB
attach/detach during scan (cancel path).

## Layer 3 - benchmark (docs/benchmarking.md, v0.2)
Scan throughput on the transport paths (RDA vs direct), carve rate per
format, session save/load rate.
