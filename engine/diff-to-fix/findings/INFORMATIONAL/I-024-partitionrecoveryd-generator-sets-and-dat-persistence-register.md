# INFORMATIONAL I-024 — PartitionRecoveryD MOM module: two-generator-set lost-partition architecture + lostpart.dat/partinfo.dat persistence — capability register, semantics tracked open

**Severity:** INFORMATIONAL (tracked-open semantics; the capability divergence itself is MAJOR-shaped but undecoded — see OQ-S6-1)
**Module:** lost-partition, orchestration
**Parameters affected:** A30 (candidate scoring), A37 (stage model), D12 (session persistence — partition-table cache), A54 (module boundaries)
**Corpus references:** `re-workspace/artifacts/MOM-modules/PartitionRecoveryD.md:23-24` (`CGeneratorSetCompleteSearch` ×2, `CGeneratorSetRegSearch` ×2 — plus `ILocationGeneratorSet`, `IBlockparserSet`, `CGeneralGeneratorSetImpl`), `:37-38` (string evidence: `lostpart.dat`, `partinfo.dat`), `:15-21` (the parser family: CBlockParserLinux/Win32/CExtX/CFatx ×2 each — the MOM-side partition engine), `re-workspace/artifacts/PartitionRecovery/mom-module-system--architecture.md` (the MOM module system this DLL rides)
**Core references:** `src/core/lost/` (single `discover()` family — one generator path, no persistence files; `cr_scan.cpp:1032`-lineage the single lost-partition stage); `src/core/table/` (the table parse layer the probes share)
**Status:** READ-ONLY FINDING — no fix applied; deeper semantics tracked in OQ-S6-1

## Summary

The og's PartitionRecoveryD MOM module runs its lost-partition search through TWO generator sets (`CompleteSearch` ×2, `RegSearch` ×2 — the class census shows doubled instances) and persists partition knowledge in `lostpart.dat` / `partinfo.dat`. The core has a single-pass discover with no cross-run partition persistence. The class/string evidence is solid; the BEHAVIORAL difference (what RegSearch does that CompleteSearch does not, and what the .dat files feed) is not decoded — tracked precisely in OQ-S6-1.

## What the reference does

PartitionRecoveryD (first-cited this session) is a MOM module with the full parser family (Linux/Win32/ExtX/Fatx block parsers, doubled), generator-set classes for two search modes, location-generator interfaces, and the two .dat persistence files as embedded strings.

## What our core does

One lost-partition discovery pass (probe families + scoring per M-017), no generator-set split, no persisted partition tables between runs (sessions persist the RESULT tree, not partition knowledge).

## Gap / Disposition

Capability-register: the og's architecture suggests (a) a second, registry/cache-driven search pass and (b) cross-scan partition-table persistence. Neither is confirmed behaviorally — the class names and strings are the evidence. Filing as INFORMATIONAL register + OQ-S6-1 (the precise decode request) rather than a higher-severity claim the corpus does not yet support.

## Reproduction / verification

Corpus-derived (artifact first-cited this session; quotes above). Core side verified in-source at the audited commit.

## Fix plan

None until OQ-S6-1 resolves the semantics (CompleteSearch vs RegSearch division of labor; the .dat formats). If the decode shows the RegSearch pass recovers partitions CompleteSearch misses, this upgrades to MAJOR with a concrete fix plan; if it is a legacy-compatibility path, it closes as MATCH-architecture-different.

## Tests to add

None yet.

## Related findings

M-017 (lost-partition scoring — the core's single-pass scorer this would extend); S-006-FIXED (lost-partition probes); S-019-FIXED (BitLocker probe family); OQ-S6-1 (the tracked decode).

## Open questions

See OQ-S6-1 — the complete question set lives there.
