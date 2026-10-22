# Benchmarking (v0.2 harness)

Bodies:
- Transport: RDA path vs \\.\PhysicalDrive direct, sequential + strided reads
- Engines: MFT walk records/sec; FAT chain dirs/sec; carve MB/s per format
- Sessions: save/load MB/s + compression ratio per record type

Baselines are recorded per commit on the benchmark workflow and compared
against the previous release (regression gate: >5% slowdown blocks merge).

The v0.1 engine layer already exposes the counters needed (CachedReader
hits/misses, carve stats, scan bytes) - see cr::carve::CarveStats and
cr::CachedReader.
