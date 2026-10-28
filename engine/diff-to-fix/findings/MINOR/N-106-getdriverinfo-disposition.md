# MINOR N-106 — GetDriverInfo.dll (drive-letter topology MOM module): the one corpus surface never dispositioned anywhere in the trail

**Severity:** MINOR
**Module:** orchestration / device enumeration (disposition)
**Parameters affected:** B8-adjacent (device/volume discovery), C68 (inventory completeness)
**Corpus references:** `artifacts/PartitionRecovery/getdriverinfo--tb-module-decode.md` (35 lines — exports TBCreateObject/TBCanUnloadNow, GetDriverInfo_Init/GetDriverInfo/GetDriverInfoImp; imports GetLogicalDrives + GetDriveTypeW: drive-letter topology with fixed/removable/CD-ROM/network classification; late-binds sibling TB modules; async completion via events; NO SCM imports — "drivers" = drive letters); twin card `artifacts/per-binary/GetDriverInfo.md`; listed at `architecture/00-INDEX.md:64`
**Core references:** audited `ff879f9` — zero trail mentions (verified by full-corpus basename grep this session); nearest rows: N-071 (enumeration breadth — WMI/setupapi/VDS/StorageSpaces/FunctionDiscovery + the PhysicalDrive loop) and the dead volume-letter code N-041 dispositioned
**Status:** READ-ONLY FINDING — no fix applied

## Summary

GetDriverInfo.dll is the only corpus surface that appears in the
corpus's own master index yet has no row in SURFACE-INVENTORY and no
disposition anywhere in the trail. It is a Todo-Backup-lineage MOM
module exposing logical drive-letter topology — a sixth og enumeration
source (drive letters + DRIVE_* classification) that N-071's census
does not list.

## Gap / Impact / Verification

Falsifiable: grep the trail for "getdriverinfo" — zero hits. Impact:
inventory completeness; the module is TB-lineage (its MOM pair
TBCreateObject/TBCanUnloadNow match the backup line's module shape),
so the likely verdict is OUT-OF-SCOPE-by-lineage — but that verdict
must be registered, not silent.

## Fix plan

1. Add the inventory row (PartitionRecovery family, TB-lineage) with
   the disposition: OUT-OF-SCOPE by lineage for the recovery engine
   (the DRW flow's own enumeration is the WMI/setupapi/VDS set N-071
   censuses) — unless the owner wants the drive-letter layer as a
   future volume-surface feature, in which case register it on the
   roadmap instead.
2. Note in N-071's row that the og has a sixth, TB-only enumeration
   source.

## Tests to add

None (disposition).

## Related findings

N-101 (the inventory arithmetic wave that surfaced it).

## Open questions

None.

---

## Resolution

**Status:** FIXED-ALREADY
**Fixed in commit:** (landed by the parallel session-8 wave-2 lane; classified this session — the session-38 minor-first lane)
**Verified by:** the live-trail reproduction: SURFACE-INVENTORY:86 (read this session) carries the GetDriverInfo.dll row — "REGISTERED session-8 wave-2 (N-106) — the one surface the corpus index carried with zero trail disposition: GetLogicalDrives/GetDriveTypeW letter-topology MOM module (TB lineage). OUT-OF-SCOPE by lineage for the recovery engine (the DRW flow's own enumeration = the N-071 census); the sixth-source note added to N-071's row" — exactly the finding's fix-plan step 1 and step 2 (the row cites the getdriverinfo--tb-module-decode artifact + the per-binary card + architecture/00-INDEX.md:64).
**Resolution notes:** The disposition the finding demanded (OUT-OF-SCOPE by lineage, registered not silent) is live in the inventory with the corpus cites; the sixth-source note on N-071's row landed with it. Nothing remained to edit this session beyond the classification record.
