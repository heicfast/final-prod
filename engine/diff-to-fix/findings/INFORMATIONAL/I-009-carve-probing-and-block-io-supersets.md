# INFORMATIONAL I-009 — Carve probing and block-I/O supersets (deliberate, registered)

**Severity:** INFORMATIONAL · **Module:** carve · **Parameters:** B1, X47, X51
**Audited commit:** `e93decf` · corpus `03ad46c` · **Status:** READ-ONLY — registered differences, no defect

1. **Probing stride.** The og probes signatures at **sector-aligned offsets only** (`RAWSearchDLL_decompiled.c:9496`: candidate = `buffer + half + sector_size × sector_index`; a header starting mid-sector is invisible to it). The core byte-steps (`cr_carve.cpp:78`). The core therefore finds files the og can never see — a genuine detection superset at ~512× the probe count. Registered: keep, but bench comparisons must account for it (core-only finds on non-aligned starts are expected, not regressions).
2. **Block shape.** og: 4 MiB blocks, 8 MiB double buffer + 0x1000 slack (`FUN_18000c430 :9269-96`), per-block progress and cancel. Core: 1 MiB window + 4 KiB overlap (`cr_carve.cpp:28-29`). Different but contract-equivalent; the og's bitmap gating (M-005 evidence) is the substantive difference, not the buffer sizes.
3. **exFAT FAT loading (og)** is 1 MiB-windowed with no cap (S-007's fix model) vs the core's whole-load — the *model* is registered as the fix direction in S-013.

**Action:** registration only (bench-filter note + `docs/parity/PARITY-DELTAS.md` rows).

---

**Session-3 addendum (2026-10-26):** the parallel leg's byte-granularity finding
(renumbered into this addendum to avoid duplication) adds one action item: pin the
property with a test — `Carve.ByteGranularityFindsMidSectorStart` (a planted valid
file at sector offset +137 must be carved) — so the M-025 read-model rework cannot
silently regress the superset by adopting sector-aligned dispatch.
