# OQ-S6-1 — PartitionRecoveryD: CompleteSearch vs RegSearch generator-set semantics + lostpart.dat/partinfo.dat formats

**Filed by:** session-6 deep-continuation audit (I-020's decode request)
**Status:** RESOLVED (DFX30, 2026-10-26) — the generator-set semantics,
the .dat formats, and the recall question all decoded first-hand from
`ghidra-mo/PartitionRecoveryD_decompiled.c` (+ binary RTTI/disassembly).
Hypothesis (a) CONFIRMED: RegSearch is registry-seeded (the Windows
MountedDevices hive), not a second exhaustive pass.
**Blocking:** I-020's final disposition (INFORMATIONAL register today; upgrades to MAJOR if the decode shows a second search pass with unique recall)

## The precise questions

1. **What does `CGeneratorSetRegSearch` search that `CGeneratorSetCompleteSearch` does not (and vice versa)?** The class census shows both ×2 (PartitionRecoveryD.md:23-24). Hypotheses to falsify: (a) RegSearch = registry/cache-driven (reads persisted partition info from partinfo.dat / the Windows mounted-devices hive to target probe locations — a "remembered partitions" pass); (b) RegSearch = regex/signature-driven probe seeding vs CompleteSearch's exhaustive sweep; (c) they are per-disk vs per-volume instances of the same algorithm (the ×2 doubling).
2. **What do `lostpart.dat` and `partinfo.dat` contain, who writes them, and when are they consulted?** (PartitionRecoveryD.md:37-38 strings.) Are they cross-run caches (partition tables from previous scans, enabling "previously-seen partition" recovery) or per-run scratch? Where are they written (DLL dir / %APPDATA% / volume root)?
3. **Do the doubled block parsers (CBlockParserLinux/Win32/CExtX/CFatx ×2) serve the two generator sets independently** (one parser family per set), or is the doubling an artifact of two-instances-per-process?
4. **Does the RegSearch path (if cache-driven) surface results the complete pass cannot** — i.e., is there real recall value, or is it a latency optimization?

## RESOLUTION (DFX30, 2026-10-26 — first-hand decode)

**The family is FOUR generator sets, not two** (factory registrations
FUN_18000d3b0 at :6488/:6583/:8691/:8747): ID 1 =
`CGeneratorSetRegSearch`, ID 2 = `CGeneratorSetFastSearch`, ID 3 =
`CGeneratorSetFast1MSearch`, ID 4 = `CGeneratorSetCompleteSearch`. All
subclass `CSettingsStore` via `CGeneralGeneratorSetImpl` (base ctor
FUN_180006d90: settings-store init + maps at +8/+20/+30). The Fast pair
are PARAMETERIZED (ctors FUN_18000f030/FUN_18000f200 store two args at
+0x38/+0x40 — the "1M" in Fast1MSearch = the bounded-head-scan variant);
Complete and Reg have identical base construction and IDENTICAL vtables
(RTTI-walked: 5 slots each, only the deleting dtor differs —
FUN_18000b150 vs FUN_18000b330) — the difference is the GENERATOR each
factory attaches, not the class surface.

**Q1 (RegSearch vs CompleteSearch):** the concrete location generators
are `CLocationGenAbnormal` (:7746), `CLocationGenDependOnGeomotry`
[:sic] (:8537), and **`CLocationGenFromReg`** (:8774). The FromReg
generator's reader (FUN_18000f5b0 :8841-8925) decodes first-hand as:
`RegOpenKeyA(root, name)` -> `RegQueryValueExA(hkey, "Data")` ->
REG_BINARY, size > 0xE7, marker `buf[0xC4]=='#'` and `buf[0xE6]=='#'` ->
`memcpy(field, buf+0xC6, 0x20)` -> `swscanf(field, "%I64X", &val)` ->
if `val % sector_size == 0` emit `{byte_offset=val,
sector_count=val/sector_size}` — i.e. **the Windows MOUNTED DEVICES
hive** (each volume's "Data" value carries the parent-disk signature +
the partition's starting offset as #-delimited hex; the generator seeds
a probe at exactly that offset). RegSearch = the registry-seeded
targeted pass (mounted/remembered partition positions); CompleteSearch
= the exhaustive location-generator sweep (geometry + abnormal
positions). Hypothesis (a) confirmed; (b) regex/signature-seeding
refuted; (c) per-disk/per-volume instantiation refuted (the x2 census
doubling = the four-class factory family's COMDAT/dtor pairs).

**Q2 (lostpart.dat / partinfo.dat):** both are fixed-record caches with
a `"Part"` magic (`0x74726150`), read in a loop until short-read/bad
magic (FUN_180023120 :21241-21287 / FUN_180023280 :21289-21330):
- **partinfo.dat = 0x18 (24)-byte records** — the remembered LIVE table;
  at run start the module builds the current table and **memcmp's the
  24-byte records** to detect layout changes (FUN_180023120 param_3=1,
  the comparison site at :21016+ — the same semantics the core's M-042
  REC_DISK_LAYOUT landed).
- **lostpart.dat = 0x448 (1096)-byte records** — the remembered LOST
  candidates; consulted to list/recover previously-found partitions
  (FUN_180023280 param_3=0; the "get first remembered lost partition"
  API FUN_180020120 :19905-19929 reads entry 0's +8 field).
- Paths resolve via FUN_180025dc0(module-context, filename, out) — the
  module-directory concat; cross-run caches, not per-run scratch.

**Q3 (doubled block parsers):** superseded — the census's x2 doubling is
an artifact of the four-set factory family (each set instantiates the
shared parser pool through `IBlockparserSet` :7653); the parsers are not
per-set duplicated.

**Q4 (recall value):** RegSearch's recall is a SUBSET of the exhaustive
sweep's (the registry offsets are also probed by CompleteSearch's
geometry generators); its value is LATENCY (targeted first-pass at
remembered positions) plus the offline-disk case (MountedDevices records
outlive the on-disk table — a destroyed table with intact registry
records gets an immediate targeted probe). The CORE's exhaustive
lost-partition scan (S-006/S-019 probe families) covers the recall; the
registry-seeded fast path is a latency optimization the core does not
model — registered as a deliberate-difference note for
modules/mom-capabilities.md (no finding: recall parity holds).

## The corpus artifacts that must be examined next

- `re-workspace/reversed/` — the PartitionRecoveryD decompile tree (search for the generator-set vftables, the .dat filename xrefs, and the ctor sites that instantiate both sets ×2). Start from the strings `lostpart.dat` / `partinfo.dat` xrefs (file I/O sites) and walk callers.
- `MOM-modules/PartitionRecoveryD.md` (class census) and `PartitionRecovery/mom-module-system--architecture.md` (the module lifecycle — when PartitionRecoveryD loads, what messages it receives).
- Cross-check `mom-module-system--architecture.md`'s message table for partition-search request messages that name a mode ("complete" vs "registry"/"reg") — the UI's lost-partition flow may name the two passes explicitly.

## Why it was not resolved this session

The artifacts present class inventories and strings only; the generator-set division of labor and the .dat formats require decompile reading beyond this session's scope (the raw reversed trees for PartitionRecoveryD were not in this session's verified-citation set).

## Related findings

I-024 (the register), M-017 (the core's single-pass scorer), S-006-FIXED / S-019-FIXED (the probe families the generator sets would drive).

---

## Session-14 addendum (DFX14 — pushed forward, unchanged)

Needs the PartitionRecoveryD decompile pass as filed; no new evidence this
session. I-024's acknowledgement (session-7 INFO wave) already records the
register's refusal to inflate severity on class/string evidence alone.
