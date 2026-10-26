# OQ-S6-1 — PartitionRecoveryD: CompleteSearch vs RegSearch generator-set semantics + lostpart.dat/partinfo.dat formats

**Filed by:** session-6 deep-continuation audit (I-020's decode request)
**Status:** RESOLVED (session 25, DFX25) — full decode; no code change (evidence: `../evidence/decompiled-quotes/s25-oq-s6-1-generator-sets-and-dat-files.md`)
**Blocking:** (was I-020's disposition — the upgrade condition is REFUTED; I-020 stays the acknowledged register)

## RESOLUTION (session 25, 2026-10-26)

1. **RegSearch vs CompleteSearch: behaviorally IDENTICAL.** Both derive
   from the same CGeneralGeneratorSetImpl base (ctor FUN_180006d90
   :4028); their vtables (dumped byte-level: 0x18002c1a8 vs 0x18002c2e8)
   share ALL methods except the dtor thunk. They are registration
   identities in the mode map (FUN_18000d3b0 :7919: **Complete=4,
   Reg=1, Fast=2, Fast1M=3** — factory thunks that just `new(0x38)` +
   the shared ctor). The "registry-driven second pass with unique
   recall" hypothesis is REFUTED; the name does not refer to the
   Windows registry. **I-020's upgrade condition is refuted — it stays
   the acknowledged INFORMATIONAL register.**
2. **The .dat files fully decoded**: they live in the MODULE'S OWN
   DIRECTORY (GetModuleFileNameW → dir → append; FUN_180025d30/:23126).
   partinfo.dat = 0x18-byte "Part"-magic records — the live-table
   snapshot; on first use per process BOTH files are DELETED, then
   partinfo.dat is rewritten from the current table; at the next run
   it is memcmp'd against the live table → layout_changed → progress
   reset. lostpart.dat = 0x448-byte "Part"-magic candidate records,
   appended per discovered candidate; loaded at scan start; when the
   layout is unchanged, candidates are deduped against the cache
   (match {+0x28,+0x30,+8}); new candidates reset progress.
   Cross-run caches in the DLL dir — NOT the registry, NOT %APPDATA%.
3. **The ×2 parser census = two INSTANCES of the whole search stack**
   (the generator sets are behaviorally identical, so they cannot own
   disjoint parser families).
4. **No unique recall** — the Reg path surfaces nothing the Complete
   pass cannot; the decode CONFIRMS M-042's landed design at the
   mechanism level (partinfo semantics = REC_DISK_LAYOUT type 8's
   layout_changed flow; lostpart.dat candidate cache + dedup +
   revalidation = M-042's per-candidate persistence +
   revalidate-on-resume). The core needs no new surface — no .dat
   files (session-embedded equivalents already landed; the og's
   delete-on-first-use makes them per-boot scratch, not durable
   knowledge).
5. Residual (small): which UI surface requests mode 1 vs 4 is
   undecoded (originates outside this module; cannot change the
   answer — the constructed objects are behaviorally identical).

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

I-024 (the register), M-017 (the core's single-pass scorer), S-006-FIXED / S-019-FIXED (the probe families the generator sets would drive).

---

## Session-14 addendum (DFX14 — pushed forward, unchanged)

Needs the PartitionRecoveryD decompile pass as filed; no new evidence this
session. I-024's acknowledgement (session-7 INFO wave) already records the
register's refusal to inflate severity on class/string evidence alone.
