# s25 — OQ-S5-1 resolution: CheckVersion.mo is dead-shipped (MATCH-by-omission)

Session 25 (DFX25), 2026-10-26. Corpus `03ad46c`. The acceptance's
exhausted-sweep branch taken; every probe below executed first-hand.

## The load contract (the decode that frames the sweep)

The MOM runtime loads modules **on-demand by name**
(mom-module-system--architecture.md §1.1, decoded from
PartitionRecovery_decompiled.c):

```
FUN_1800017f0("DeviceManager")   // call-site request BY NAME
  └─ name + ".mo" → LoadLibraryA → GetModule   // required export
```

Therefore "is CheckVersion live?" reduces to "does any call site ever
request it by name?" — the module's shipping presence is not a load.

## The sweep (all negative)

1. **All decompiled hosts** (DRWUI, MainModule, RecoveryModule,
   ToolsModule, PartitionRecovery.dll, every SearchDLL, every other
   .mo module): `"CheckVersion"` appears ONLY inside
   CheckVersion_decompiled.c — the module's own GetName
   (`FUN_18000bb10` :7896, `return "CheckVersion"`), zero external
   callers. No `FUN_1800017f0("CheckVersion")`-style request exists
   anywhere.
2. **All PE binaries in the corpus** (BUILDPE/EaseUS-x64/* payload +
   the full og-codebase tree): zero references. The single
   strings-sweep hit — `EUFileRepair/CORE_RL_xml_.dll` — is
   **libxml2's `xmlCheckVersion` API** (ImageMagick's bundled libxml),
   a coincidental name collision, not a consumer.
3. **Config-driven loads**: every *.ini / *.xml in og-codebase and
   BUILDPE/EaseUS-x64/config swept — zero CheckVersion references.
4. **Generic directory-enumeration loaders**: none exist. The
   FindFirstFileW sites in the host DLLs
   (PartitionRecovery_decompiled.c :56255, :60558) are the
   recovered-filesystem walker (ACL/security-descriptor checks via
   GetFileSecurityW; FindNextFileW over `\*`) — file-tree scanning,
   not module loading.
5. The PE edition drops the MOM system entirely (pe-payload-census
   :53) — irrelevant either way, but recorded.

## Reconciliation of the session-14 "load list" claim

Session-14's addendum cited "the module IS in the 31-module MOM load
list (mom-module-system--architecture.md :69)". That table is the
module **inventory** (what ships in the PartitionRecovery directory);
the same document's §1.1 decodes the actual contract as on-demand
by-name loading. Shipping ≠ loading.

## Disposition

**MATCH-by-omission (dead-shipped)** — registered in
modules/mom-capabilities.md. No finding filed: the OS-aware recovery
capability never executes in the shipped product, so the core's
absence of OS-install classification (\winnt/\windows sweep, ARC-path
enumeration, offline SYSTEM-hive ProductType reads) is parity, not a
missing feature. No code change.
