# SMART-MATRIX — platform capability × transport × product behavior, per Windows version

> Session 2026-11-17 (T6). Sources: `worklog/REFERENCE-SMART.md` (the corpus
> ground truth), `worklog/VERIFICATION.md` (V1/V8/V9/V10), the engine sources
> (cr_health.{h,cpp}, cr_sat.{h,cpp}, cr_health_wmi.cpp, cr_csmi.{h,cpp},
> cr_grade.cpp, cr_ui.cpp health_read_impl), and the platform documentation
> (Microsoft Learn — marked [MSFT] where the claim is from the documented
> platform surface rather than the corpus). Verification discipline: every
> OURS cell names the route that exists in code TODAY (commit 2a4b02ed); the
> RUNTIME EVIDENCE section records what is actually PROVEN on real Windows so
> far, and the physical-class validation that remains deferred, honestly.

## 1 The transport table (ours)

Chain order per disk (the first channel answering IDENTIFY serves the set —
cr_smart_ata.cpp; the og pre-selects one accessor family per disk at AddDisk
time, N-032 model — equivalent per-disk outcomes, documented divergence):

| # | Route | Win7 SP1 | Win8/8.1 | Win10 | Win11 | Notes |
|---|---|---|---|---|---|---|
| 1 | `ata-passthrough` — IOCTL_ATA_PASS_THROUGH (ATA_PASS_THROUGH_EX) | ✓ | ✓ | ✓ | ✓ | [MSFT] the direct ATA route, available since XP/Server 2003 era; works for drives behind standard ATA drivers (storahci/ataport) |
| 2 | `smart-rcv-drive-data` — SMART_RCV_DRIVE_DATA 0x7C088 (the pre-Vista SMART ioctl family, IDEREGS incl. the SMART magic 01/4F/C2 task file) | ✓ | ✓ | ✓ | ✓ | [MSFT] the legacy fallback for filter stacks that reject route 1; the reference's own Pd fallback ladder (REFERENCE-SMART §2) |
| 3 | `sat-passthrough` — the reference's byte-exact 12-byte SAT stamps (A1 08/1C 0E …) over IOCTL_SCSI_PASS_THROUGH_DIRECT | bridge-dep. | bridge-dep. | bridge-dep. | bridge-dep. | Works wherever the bridge firmware implements SCSI-ATA Translation [MSFT]; the reference's GetSmartAttributeSat route with the -1A retry (commit 0bfc634a; stamps decoded from DiskHealth_decompiled.c FUN_00419280/:9124) |
| 4 | `csmi-miniport` — CSMI CC-0x19 SRB_IO_CONTROL via IOCTL_SCSI_MINIPORT | vendor-dep. | vendor-dep. | vendor-dep. | vendor-dep. | Intel RST / AMD rcraid arrays; the wire decoded from FUN_00425450 (cr_csmi.h). VROC/VMD members: passthrough depends on the controller — a miss is a clean transport miss, never a fabricated table |
| 5 | `nvme` — IOCTL_STORAGE_QUERY_PROPERTY + StorageDeviceProtocolSpecificProperty, health log page 2 (the D-NVMe-01-fixed buffer shape, offset-8 protocol data) | ✗ Unknown | partial | ✓ | ✓ | [MSFT] the protocol-specific query structure ships with Win8.1; reliable NVMe log-page service is the Win10+ storage stack ("Working with NVMe drives"). On Win7/8.x an NVMe disk without a vendor driver grades Unknown — TRUTHFUL, never fabricated (D-P4d) |
| 6 | `wmi-failurepredict` — MSStorageDriver_FailurePredict{Data,Thresholds} from root\WMI (post-chain fallback) | ✓ (ATA-served disks) | ✓ | ✓ | ✓ | [MSFT] the classes are published by the ATA port stack — they cover internal ATA disks whose direct ioctls are blocked; USBSTOR/SD disks publish nothing (miss = truthful). The reference's own last route (§2); we do NOT force-start Winmgmt (divergence, D-P4b) |

Per-disk verdict: family-first like the og (REFERENCE-SMART §4) — the NVMe
health log grades via `grade_nvme` (critical_warning → Bad; life =
100 − percentage_used with >10/==10/<10 cutoffs; "Parallels Virtual NVMe
Disk" → Good via the product string); otherwise the ATA table's S-024
worst-of-attributes; neither source → **Unknown** (gray). The grade crosses
the ABI (cr_ui_smart_report.grade, commit 2a4b02ed) and the bridge maps it
1:1 — no re-derivation anywhere. **No percentage exists anywhere in the
chain** (the fake healthScore class was removed in 6e12f167, and the
grade-chain commit deleted the bridge's wear>=80 approximation).

## 2 Per disk class — behavior and truthful states

| Disk class | Our behavior today | The reference (corpus) | State |
|---|---|---|---|
| Internal SATA HDD/SSD | routes 1→2→6 (→3/4 where applicable); full 30×12 table + thresholds (ex2), per-row grades, 4-state verdict | ATA passthrough + legacy + WMI fallback (§2) | MATCHED (routes 1/2/6 byte-family-equivalent; the og's task file is the same IDEREGS evidence) |
| Internal NVMe | route 5 on Win10+ (health log: temperature, spare, % used, power cycles, data units ×512,000 — the M-036 row set); Unknown on Win7/8.x without a vendor driver | the generic STORAGE_QUERY route on Win10+; 7 VENDOR routes for Win7/8-era + USB-NVMe (§2) | MATCHED on Win10+; vendor routes DEFERRED (D-P4c — payloads only partially decoded, VERIFICATION V9; no fabricated dialects) |
| USB-bridged SATA | route 3 (SAT stamps, byte-exact) after 1/2 miss; bridge without SAT → Unknown | SAT + the 23-branch vendor quirk ladder (§2) | GENERIC SAT MATCHED; vendor ladder DEFERRED (D-P4c) |
| USB NVMe enclosures | route 5 where the box's bridge exposes NVMe passthrough to the storage stack; otherwise Unknown | vendor NVMe routes (JMicron/ASMedia/Realtek…) | DEFERRED with the D-P4c family |
| SCSI/SAS | route 3/4 attempt (SAT over SPT / CSMI); a pure SCSI disk without SAT → Unknown | DoIdentifyDeviceScsi + generic table (§3.4; log-page depth UNKNOWN per Agent E) | PARTIAL — the reference's SCSI identify route is not separately implemented; SAS drives behind standard drivers typically answer SAT; miss = truthful Unknown |
| SD cards | **no transport exists** — enumerated, listed, selected, graded Unknown (gray) with the empty-table note; never a fake green | same model: SD = removable media only, no transport strings exist, grade gates land Unknown (§5) | MATCHED (the mock models it; the UI renders it; BF-31's fake card is gone since ba2b9347) |
| RAID (VROC/VMD/RST) members | route 4 (CSMI) where the miniport speaks it; else routes 1-3 as the controller allows; else Unknown | CSMI route + RAID census (§2); the census routing stays companion-side (documented in cr_health.h) | MATCHED for the CSMI wire; the RAID census routing is recorded divergence (N-032) |

## 3 Runtime evidence so far (honest)

PROVEN on real Windows (GitHub windows-2022 runners, the bridge-integration
lane, run 44071105301 + successors):
- The truthful no-source path: `health_read_per_disk_is_coherent` PASSED on
  VHD-backed disks — transport "", rc 0x50070001 (ERR_HEALTH_UNAVAILABLE),
  the exact "SMART not available on this transport" presentation the
  mandate requires. No fabricated rows anywhere in the report.
- The full engine suite on MSVC (1761 tests) green, including the SAT stamp
  byte-exactness (10), the 4-state grading decode (V8-verified family), the
  CSMI wire tests, and the NVMe parse tests.

PROVEN statically: every stamp and structure this session shipped is cited
to the corpus decompile line-by-line (cr_sat.h, cr_csmi.h,
cr_health_wmi.cpp headers).

NOT YET PROVEN (deferred with precise scope): per-disk-class health output
on PHYSICAL hardware — real NVMe, SATA SSD, SATA HDD, a USB-bridged drive,
an SD card, side by side on one machine. GitHub-hosted runners have no NVMe
passthrough, no USB bridges, no SD slots; a VHD cannot answer SMART. Scope:
run the forensic-repro lane (or the product) on a physical machine with
those classes present, capture the per-disk grade + transport + first
attribute rows under `worklog/VALIDATION/SMART/`, and compare against the
reference DiskHealth.exe on the same machine where feasible. This is the
§7.1 gate's open half.

## 4 Windows-version differences, matched or documented

| OS | What differs | Our handling | Reference handling (corpus) |
|---|---|---|---|
| Win7 SP1 | no native NVMe stack; ATA/legacy/CSMI/SAT/WMI all available | ATA chain + CSMI + SAT + WMI; NVMe = truthful Unknown unless the vendor driver exposes a route we speak | vendor NVMe routes exist precisely for this (INFERRED, §8); same truthful-Unknown class otherwise |
| Win8/8.1 | protocol-specific query structure exists; NVMe log service unreliable | routes as Win7; the D-NVMe-01-fixed query is attempted and its failure is a clean miss | corpus silent; same class |
| Win10/11 | full surface incl. the NVMe protocol query | everything on; the NVMe row set renders | the generic route (§2) |
| Any OS, behind bridges/RAID | the controller may or may not pass SMART through | each miss is a clean transport miss down the chain; the disk stays listed and grades Unknown | the same miss-tolerant ladder (§2), plus vendor dialects (DEFERRED, D-P4c) |

The Win7 import posture is machine-gated: the WMI fallback's COM pair
(ole32/oleaut32) was added to the Win7 import allowlist with the full
reason trail (commit 72fad8c2) — every export available since Windows 2000.
