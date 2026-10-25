# OQ-S5-1 — Is CheckVersion.mo's OS-aware recovery live in the og product, or dead-shipped legacy?

**Status:** OPEN (session 5) · **Module:** mom-runtime/CheckVersion · **Parameters:** P1 (detection — OS-install classification), P42 (surface)
**Audited commit:** `7cf6754` · corpus `03ad46c`

## The precise question

CheckVersion.mo fully implements offline OS-install detection and
server-vs-client classification (session-5 decode:
`FUN_180003050` module main builds "\winnt"/"\windows" candidate dirs;
`DiskContainer::scan` `FUN_180003580` enumerates ARC paths
rdisk 0..31 × partition 1..32; `IsWindowsServerInstalled::operator()`
`FUN_1800029e0` reads `%s\System32\config\system`, `\$$$PROTO.HIV\SELECT\
CURRENT`, then `CONTROLSET00%d\CONTROL\PRODUCTOPTIONS\PRODUCTTYPE` and
compares ServerNT/LanmanNT/WinNT — all cited in
`evidence/decompiled-quotes/s5-mom-capabilities.md` §4). The core has no
equivalent (zero ProductType/OS-detect hits).

**No consumer was found**: a strings dump across every PartitionRecovery
DLL, DRWUI.exe, and the .mo set finds the name "CheckVersion" only in the
module itself. If no host loads it, the og product's live behavior does
not include OS-aware recovery and the core's absence is MATCH-by-omission
(dead code both sides, conceptually); if a host loads it by an untraced
dynamic path, the capability is live and a finding (missing feature) must
be filed.

## Corpus artifacts to examine next

1. BUILDPE payload string sweep: `og-codebase/BUILDPE/EaseUS-x64/**` —
   any binary referencing "CheckVersion.mo" or "CheckVersion".
2. `DRWService.exe` / `EuRepairHost.exe` / `InfoForSetup.exe` — not in the
   reversed set; run strings/imports for the module name.
3. The MOM runtime's load path (`mom-module-system--architecture.md` §1):
   whether GetModule-by-name can be driven by a config file (config/*.ini
   in og-codebase) rather than code references.

## Acceptance

RESOLVED when a consumer is found (→ file the missing-capability finding
with the consumer's call chain) or the payload sweep is exhausted (→
register MATCH-by-omission with dead-shipping evidence in
modules/mom-capabilities.md).

---

## Session-14 addendum (DFX14 — pushed forward with NEW negative-result evidence)

Corpus probes run this session: (a) zero CheckVersion references in
DRWUI_decompiled.c (grep clean — no UI-side callers); (b) zero call sites
in the ghidra-partition decompiles (RecoveryModule/MainModule) outside
CheckVersion's own module; (c) the module IS in the 31-module MOM load
list (mom-module-system--architecture.md :69; mo-module-map.md :47 lists
it as the Windows-aware-recovery component alongside RegLib); (d) the PE
edition drops the entire MOM system (pe-payload-census :53), so the
capability is full-edition-only. The live-vs-legacy question remains
open: the LOAD is documented, the INVOCATION is not decoded anywhere.
Next concrete step (refined): a MainModule_decompiled.c pass over the
module-init/dispatch vtable to find CheckVersion's registered callbacks —
the corpus lacks that decompile today; queue it as the next RE artifact.
