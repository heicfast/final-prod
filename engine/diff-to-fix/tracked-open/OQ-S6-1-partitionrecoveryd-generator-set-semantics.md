# OQ-S6-1 — PartitionRecoveryD: CompleteSearch vs RegSearch generator-set semantics + lostpart.dat/partinfo.dat formats

**Filed by:** session-6 deep-continuation audit (I-020's decode request)
**Status:** OPEN — corpus artifacts do not decode the behavior; needs a decompile pass
**Blocking:** I-020's final disposition (INFORMATIONAL register today; upgrades to MAJOR if the decode shows a second search pass with unique recall)

## The precise questions

1. **What does `CGeneratorSetRegSearch` search that `CGeneratorSetCompleteSearch` does not (and vice versa)?** The class census shows both ×2 (PartitionRecoveryD.md:23-24). Hypotheses to falsify: (a) RegSearch = registry/cache-driven (reads persisted partition info from partinfo.dat / the Windows mounted-devices hive to target probe locations — a "remembered partitions" pass); (b) RegSearch = regex/signature-driven probe seeding vs CompleteSearch's exhaustive sweep; (c) they are per-disk vs per-volume instances of the same algorithm (the ×2 doubling).
2. **What do `lostpart.dat` and `partinfo.dat` contain, who writes them, and when are they consulted?** (PartitionRecoveryD.md:37-38 strings.) Are they cross-run caches (partition tables from previous scans, enabling "previously-seen partition" recovery) or per-run scratch? Where are they written (DLL dir / %APPDATA% / volume root)?
3. **Do the doubled block parsers (CBlockParserLinux/Win32/CExtX/CFatx ×2) serve the two generator sets independently** (one parser family per set), or is the doubling an artifact of two-instances-per-process?
4. **Does the RegSearch path (if cache-driven) surface results the complete pass cannot** — i.e., is there real recall value, or is it a latency optimization?

## The corpus artifacts that must be examined next

- `re-workspace/reversed/` — the PartitionRecoveryD decompile tree (search for the generator-set vftables, the .dat filename xrefs, and the ctor sites that instantiate both sets ×2). Start from the strings `lostpart.dat` / `partinfo.dat` xrefs (file I/O sites) and walk callers.
- `MOM-modules/PartitionRecoveryD.md` (class census) and `PartitionRecovery/mom-module-system--architecture.md` (the module lifecycle — when PartitionRecoveryD loads, what messages it receives).
- Cross-check `mom-module-system--architecture.md`'s message table for partition-search request messages that name a mode ("complete" vs "registry"/"reg") — the UI's lost-partition flow may name the two passes explicitly.

## Why it was not resolved this session

The artifacts present class inventories and strings only; the generator-set division of labor and the .dat formats require decompile reading beyond this session's scope (the raw reversed trees for PartitionRecoveryD were not in this session's verified-citation set).

## Related findings

I-020 (the register), M-017 (the core's single-pass scorer), S-006-FIXED / S-019-FIXED (the probe families the generator sets would drive).
