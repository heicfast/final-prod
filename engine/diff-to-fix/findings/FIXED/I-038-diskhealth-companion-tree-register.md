# INFORMATIONAL I-038 — the og-codebase/diskhealth/ companion-tree surface register (818 files never function-walked by any prior lane — the S12-claimed W3 surface, walked by this lane): 30 dispositions, 5 never-dispositioned surfaces registered, 2 bookkeeping repairs executed, 4 UNVERIFIED datums with next artifacts (RENUMBERED TWICE at the hot-window reconciliations: this lane's draft I-035 collided with the remote S12 lane's own register-batch renumber [draft I-033 → I-035, ACKNOWLEDGED-retired]; then the draft I-037 collided with the remote second-continuation lane's never-walked-decode register [the SearchResult.txt QA dump + the bad-sector skip policy — a COMPLEMENT, not an overlap: that register owns the corpus RE-artifact decodes, this one owns the companion product tree]; earlier-push-wins — theirs consumed both IDs; the remote S12 I-035 register batch's "DiskHealth resident shell" datum row is this register's §S15-adjacent complement)

**Severity:** INFORMATIONAL (a register: deliberate scope dispositions + trail bookkeeping; NO core code gap found — the health module is clean at the audited commit)
**Module:** health (the DiskHealth companion product tree) — modules/health.md
**Parameters affected:** D46 (documentation accuracy — the register rows this file lands), P69 (the scope-exclusion register's completeness), B1-adjacent (health decode coverage — verified, no gap)
**Corpus references:** `og-codebase/diskhealth/` (818 files / 39 dirs / 55 MB — full inventory, S42/W3a); `re-workspace/reversed/ghidra-wave2/DiskHealth_decompiled.c` (93,316 lines — every cite below first-hand); `re-workspace/reversed/ui-resources/diskhealth/` (the extracted .rcc manifest); `re-workspace/artifacts/DiskHealth/diskhealth--smart-companion.md`; `re-workspace/artifacts/per-binary/DiskHealth.md`; `re-workspace/artifacts/UIUX/19-diskhealth-quick-ui.md`; `architecture/01-build-tree-intelligence.md:66,75`
**Core references (at the audited commit `869da779`):** `src/core/health/{cr_grade,cr_family,cr_smart_ata,cr_csmi,cr_nvme}.cpp` (1,039 LOC) + `include/cr/health/*` (456 LOC) + `src/win/cr_health.cpp` (346 LOC) + 37 tests (`test_health_grade.cpp` 21, `test_health_csmi.cpp` 9, `test_nvme_health.cpp` 8) — all read in full by the S42/W3a pass
**Status:** ACKNOWLEDGED-RETIRED per the trail's current INFO convention (the e41cbb7e class: filed, rationale verified first-hand, retired to findings/FIXED/ with the ledger row) — the register's dispositions stand; trail-side repairs executed (this file + the ledger row + the N-032 dangling-path repair); no fix applied to any core file

## Summary

The corpus's `og-codebase/diskhealth/` tree (55 MB, 818 files) is the
installed deployment of a **separate companion product** — "EaseUS
DiskHealth" (internal name *DiskSmart*), a 32-bit Qt 5.15.2 Quick/QML app of
**CrystalDiskInfo lineage** (the CAtaSmart/DiskInfoMgr/CTrayIcon/AttrModel
engine classes), built from the DRW monorepo (PDB:
`F:\drw_backup\Output\Release_Win32\DiskHealth.pdb`). No prior audit lane
had function-walked it (only the DRW-side health grading/NVMe/CSMI surfaces
were audited). The S42/W3a walk censused every file into a family and
dispositioned every observable surface: 13 COVERED, 12 NOT-CORE-SCOPE under
existing trail conventions, 1 open DIVERGENCE (the ACTIVE M-076 TRIM row —
pre-existing), **5 NEVER-DISPOSITIONED surfaces** (registered by this
finding), 1 NO-SURFACE verdict (cross-run disk marking — N-145 stays
DRW-side), and 4 UNVERIFIED datums (registered with next artifacts). The
core's health module itself is **clean** — the walk found no engine-side
behavior gap; every decoded og health truth the trail cites was re-verified
first-hand (×512,000 @:5595, K-273 @:26989, the SMART-enable task file
@:4420-4426, the CSMI census strings @:4620-4828, the
Unknown/Good/Caution/Bad census).

## What the reference does (the tree's shape — inventory)

818 files = 1 `DiskHealth.exe` (1,781,480 B, PE x86, 4,636 fns / 93,316
decompiled lines, 0 exports) + 1 `DiskHealth.rcc` (125 embedded files: 10 app
QML + 50 publicQML + 65 res) + 13 stock-Qt5 DLLs + 5 VC/CRT + 40 api-ms-win
forwarders + 25 Qt plugin DLLs + 615 stock-Qt QML/bytecode files + 17
product `.qm` + 23 `qt_*.qm` + 31 stock PNGs + 3 `.bat` (addtask/deltask/
querytask — the schtasks ONLOGON autostart trio) + 1 `AliyunConfig.ini`
(`bAllowSendInfo=0`, opt-out default). Not present: `DiskHealthAuto.exe`
(referenced in the string table @0x8b73c — the §D5 datum), any `aliyun*.*`
DLL (boot-USB-side per the WinPE §2E artifact).

## What our core does (the engine half — verified MATCH)

The core's health module implements exactly the DRW-relevant engine surface:
the ATA SMART chain (`cr_smart_ata.cpp:98-129` — 0xEC/0xD0/0xD1, matching the
og's `SendAtaCommandPd` family), the 4-status grading rule-by-rule against
`FUN_0040cdf0` (:5755-6130, the s6 decode — 21 pin tests), the vendor family
classification (S-024's decoded set + the coverage-honesty block), the CSMI
CC-0x19 route (N-032/D-82), the NVMe STORAGE_QUERY route (I-018 byte-parity:
proto 3/type 2/page 2/512B @ win/cr_health.cpp:264-299) with the health
fields (×512,000 units, K-273, 100−percentage_used — M-036/N-009a/N-042
family) and the family-0x13 grade path. 37 tests green at HEAD. **The walk
re-verified every load-bearing og cite first-hand and proposes ZERO core
code changes.**

## Gap → Register (the 5 never-dispositioned surfaces)

All five are companion-scope transport/quirk surfaces under the core's
existing report-only policy (`cr_health.h:26-27`); each is a REGISTER row,
not a code gap:

- **R1. The 6 vendor NVMe CMD_TYPE routes** —
  `CMD_TYPE_NVME_{INTEL_RST,SAMSUNG,INTEL,JMICRON,ASMEDIA,REALTEK}` +
  `DoIdentifyDeviceNVMe{IntelRst,Samsung,Intel,JMicron,ASMedia,Realtek}`
  (UTF-16 string table, scratch dump :715-731; the 7th route — STORAGE_QUERY
  — is the covered one). Grep-negative across the whole trail; F-A4-19 named
  Realtek but the N-032 acceptance + D-82 registered only the CSMI/ATA
  remainders. **Disposition: not ported; the storage-query route is the
  universal modern route; vendor routes serve exotic bridge/vendor-stack
  disks → `ERR_HEALTH_UNAVAILABLE` is the report-only answer. Extends
  D-82.** (UNVERIFIED sub-datum U1: the wave2 pass did not recover the six
  routes' decompile call sites — string-table evidence only; next artifact:
  targeted xref around INTERFACE_TYPE_NVME sites :19403-19434/:21444-21451.)
- **R2. The USB-bridge quirk table** — `AddDisk - Si` (:10761), `AddDisk -
  ASM1352R`, the 28 `USB0A…USB20` log strings (:11043-11163+), the bridge
  VID/PID quirks (LOGITEC 0x00D9, Genesys 0x05E3/0x0702, IO-DATA 0x0122,
  JMicron). N-032's own TITLE names the usb-bridges half; D-82's
  registration text never did (grep-negative). **Disposition: not ported;
  usbstor's SAT layer serves the standard routes; bridge-only dialects read
  health-unavailable — registered, extends D-82.**
- **R3. The AAM/APM device-write setters** — enable/disable AAM (0xEF
  features 0x42/:9565 and 0xC2/:8058), enable/disable APM (0x05/:9575 and
  0x85/:8068): live code in the binary, unreachable from the shipped QML
  (no control in the 54-component census — dead capability in this skin).
  D-82's device-write census registered only the SMART auto-enable (0xD8
  task file @:4420-4426). **Disposition: the companion carries AAM/APM
  setters (dead in this UI skin); the core is report-only and will not grow
  them — completes D-82's device-write census.**
- **R4. The `sc config Winmgmt start= auto` system-service mutation**
  (:9508, before the WMI Win32_DiskDrive/Win32_PnPSignedDriver queries —
  u16 strings :516-545). **Disposition: host-concern datum (extends the
  I-027 autostart class); the core has no WMI dependency (SSD detection is
  the seek-penalty IOCTL, cr_health.cpp:325-342) — core-side no-op is
  correct.**
- **R5. The `diskHealthIndex=` external-launch contract** (DRW/EPM launch
  the companion pre-targeted at a disk; parsed at :233-254/:37772-37797) —
  **Disposition: companion app-shell, OOS-UI; the core's equivalent data is
  SmartReport per disk index; recorded for any future host.**

## Reproduction / verification

Every cite above was read first-hand (Read-with-offset on
DiskHealth_decompiled.c; `strings` both encodings — scratch dumps
`dh-strings-ascii.txt` 6,253 lines / `dh-strings-u16.txt` 1,111 lines; the
.rcc manifest cross-check; the full core health-module read). The
trail-negative greps (NVME vendor routes / ASM1352 / Winmgmt / AAM·APM) were
run across diff-to-fix/ + docs/. Corpus-worklog claims re-verified: rcc=125
files incl. 54 QML (:104) ✓; wave2 ingestion (:115) ✓; UIUX/19 (:202) ✓;
CrystalDiskInfo lineage (:435/:466/:494) ✓; boot-USB Phase-E copy
consistency ✓.

## Fix plan (for the engineering team)

1. **Trail-side (DONE in this wave by the S42 lane):** this register file;
   the N-032 dangling-path repair in FIXED-LEDGER.md:215; the
   CORPUS-ERRATA rows for the corpus-side count defects this walk
   surfaced.
2. **Doc-side (queued for the next doc lane, the I-033 convention):**
   PARITY-OG-CORE-INVENTORY.md:51's AliyunConfig consumer note needs one
   clause — "incl. the DiskHealth companion's own in-process client"
   (DiskHealth.exe reads/writes `diskhealth/AliyunConfig.ini` itself,
   :39732/:39762 — the "crash-report-chain-only" wording is imprecise for
   the diskhealth copy). The D-82 extension rows (R1-R3) land in
   docs/PARITY-DELTAS.md.
3. **Acceptance criteria:** the five dispositions registered in
   docs/PARITY-DELTAS.md (D-82 addendum) + the I-027 extension (R4); the
   UNVERIFIED datums U1-U4 either decoded by the named next artifacts or
   carried as passive data; zero core code changes (the health module
   verified clean).

## Tests to add

None — no behavior change is proposed. The health module's existing 37 tests
are the pin set for the verified MATCH surfaces; any future NVMe
vendor-route port would bring its own per-route fixture set (out of scope,
report-only policy).

## Related findings

**M-076** (the one ACTIVE health divergence — the per-row TRIM datum, rides
independent); **N-032/D-82** (the transport-remainder register this file
extends); **I-018** (the NVMe STORAGE_QUERY byte-parity); **S-024/N-055**
(the family-classification and 0xD1 thresholds history); **I-027** (the
host-concern datum class R4 extends); **I-034** (the asset-register class); **I-035** (the remote S12 lane's register batch — its 'DiskHealth resident shell' row is this register's §S15-adjacent complement); **I-037** (the remote second-continuation lane's never-walked-decode register — the complementary corpus-side half);
**N-154** (this lane's red-main filing — unrelated surface, same session).

## Open questions

The four UNVERIFIED datums (with next artifacts, per §6 of the context
sheet): **U1** the vendor-NVMe routes' decompile call sites (xref around
:19403-19434/:21444-21451); **U2** the SAT stamp-variant deltas
(-1A/-2/-2A — the SendAtaCommandPd family decode); **U3** DiskHealthAuto.exe's
exact role (the FUN_00458600 caller chain :35209/:40719 + the SetupUE
packaging side); **U4** the scheduled-task name arg %2's exact init site
(functions.txt static-initializer walk). All four are companion-scope
curiosity datums — none blocks a product decision; tracked here rather than
in tracked-open/ (no fix lane work is implied).
