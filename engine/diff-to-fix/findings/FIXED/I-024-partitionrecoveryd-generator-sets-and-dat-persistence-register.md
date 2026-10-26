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

---

## Resolution

**Status:** ACKNOWLEDGED (verified this session, closing session / df5 lineage — semantics stay tracked in OQ-S6-1)
**Fixed in commit:** none — capability register; the behavioral decode remains OQ-S6-1's scope
**Verified by:** corpus artifact re-read first-hand (MOM-modules/PartitionRecoveryD.md :14-24 + :35-40 — CGeneratorSetCompleteSearch ×2 / CGeneratorSetRegSearch ×2 / CGeneralGeneratorSetImpl ×2 / CBlockParser{Linux,Win32,ExtX,Fatx} ×2 and the lostpart.dat / partinfo.dat strings are verbatim); core side re-verified (src/core/lost/ single discover family; no .dat persistence). OQ-S6-1 exists in tracked-open/ with the precise question set.
**Resolution notes:** The register's honesty is CONFIRMED: the class/string evidence supports the capability register but NOT a higher-severity behavioral claim — the finding correctly refuses to inflate (SEVERITY-SCALE discipline) and routes the decode to OQ-S6-1 with a concrete upgrade path (RegSearch pass recovering what CompleteSearch misses → MAJOR with a fix plan; legacy-compat → MATCH-architecture-different). The rationale is preserved; the upgrade decision belongs to whoever resolves OQ-S6-1.

### Addendum (session 25, DFX25 — OQ-S6-1 decoded; the register's open question closed)

OQ-S6-1's decode (evidence:
`../../evidence/decompiled-quotes/s25-oq-s6-1-generator-sets-and-dat-files.md`)
closed this register's tracked behavioral question: CGeneratorSetRegSearch
and CGeneratorSetCompleteSearch are behaviorally IDENTICAL (vtables dumped
byte-level from the binary share every method except the dtor; they are
registration identities in the mode map — Complete=4, Reg=1, Fast=2,
Fast1M=3 via FUN_18000d3b0). The .dat files are cross-run caches in the
module's own directory: partinfo.dat = 0x18-byte "Part"-magic live-table
snapshot (delete-both-on-first-use; memcmp'd at the next run →
layout_changed → progress reset), lostpart.dat = 0x448-byte "Part"-magic
candidate cache (appended per discovery; deduped by {+0x28,+0x30,+8} when
the layout is unchanged). No second search pass with unique recall exists
— the register's INFORMATIONAL classification stands as correct, and
M-042's landed session-embedded equivalents (REC_DISK_LAYOUT type 8 +
per-candidate revalidation) are confirmed at the mechanism level.
