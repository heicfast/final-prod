# S11-lead deep-lanes evidence — the five go-deeper waves (Lanes A-E)

**Session:** session-11-lead (the continuation audit lane; territory claimed at
`83d154b5`) · **Baselines: core `461825d8`-lineage at filing (the board moved
mid-wave: M-072/M-074/M-077/M-083/M-096/M-100/M-102/M-110/M-108/M-093-class
retirements landed by the parallel fix lanes; every lane's core-cite was re-checked
against the filing tree), corpus `be0cfe9`.** Five parallel read-only research
agents (Lanes A-E) + the lead's first-hand re-verification of every load-bearing
decode before filing (the TS clamp, the filter-exempt mask arithmetic, the
promotion guard, the ZIP probe gate, the boot-surgery chains, the marker strings).
The filings from these waves (post-reconciliation IDs): **S-054, M-115, M-113, M-114,
N-149, N-143, N-144, N-145, N-146, N-148, I-033** (+ the addenda and corpus-errata
rows below). Push-time reconciliation with the parallel S11B lane: their M-112 (the
CSL 0x44c size-field finding — CONVERGENT with this lane's draft N-147, which is
DROPPED per extend-don't-refile; their decode is the richer) and their N-142 (the
dead CrPreview wiring) pushed first and keep their IDs; this lane's promotion-gating
filing renumbers M-112→M-115 and the filter-exempt filing N-142→N-149. Nothing outside `diff-to-fix/` +
`worklog*` was touched.

---

## LANE A — the RAWSearchDLL infrastructure + family walk (S11-lead/T6)

Citation key: `DDC:line` = `re-workspace/reversed/ghidra-searchdll/RAWSearchDLL_decompiled.c:line`.

### The infrastructure verdict table

| # | Element | Verdict | Key evidence |
|---|---|---|---|
| 1 | Factory/export contract | MATCH (registered ABI shape) | DDC:72122-72221 vs the cr_ui ABI |
| 2 | Dispatch order u32→u16→list | MATCH-by-superset (I-009) | DDC:9842-9921; cr_carve.cpp:349-427 |
| 3 | Static-vs-working registry duality | MATCH | exports=static (DDC:72170-72208); working rebuilt+filtered (DDC:4386-4434); core: static table + per-scan disable maps (cr_carve.cpp:979-993). **Corpus-errata candidate**: census :20-23 "same data" is wrong — the exports project the static triplet only |
| 4 | Per-key chains + two-generation fallback | REGISTERED (D-75) | DDC:1762-1805 (DMG/X3F byte-exact) |
| 5 | ISpecial two-step + id-8 continue | **FILED N-143** | DDC:9877-9883, 10020-10039 |
| 6 | Per-search filter + exempt set | **FILED N-142** | the mask 0x1000801 (bits 0/11/24 → ids 0x30/0x3b/0x48 = OLE/RIFF/ZIP) at DDC:4412-4418 — lead re-derived the arithmetic |
| 7 | Scan geometry (ping-pong vs windows) | MATCH (M-025/D-81) | buffer = spc×sector×2+0x1000 (DDC:9205-9209); 4 MiB chunks (DDC:9271); half-memmove (DDC:9620) |
| 8 | Bitmap/occupancy/zero-skip gates | MATCH (M-005/M-039/N-031, all FIXED) | DDC:9446-9459, 9498-9510 |
| 9 | Pending-object continuation | REGISTERED (D-80/D-81) + the parallel lane's I-031 entries (pending-16 cap, Stop-flush) | DDC:9573-9591, 9658-9693 |
| 10 | Per-scan dedup | MATCH | DDC:10171-10176 vs emitted_at/find_by_extent (cr_carve.cpp:377-415) |
| 11 | Progress | MATCH-registered (D-78) | the og DLL emits none (DDC:9439-9441 — the per-chunk continue-check is the only observer interaction) |
| 12 | Cancellation | MATCH coarse; salvage facet registered D-101 | DDC:9658-9693 |
| 13 | Video-repair hook | MATCH per-candidate (M-021) + **NEW facet → the M-021 addendum**: the og feeds every CHUNK's buffer to the repair interface (`vtable+0x40` at DDC:9623-9627; init `+0x30` at DDC:9226-9236); core's CarvePostProcessor is per-candidate only (cr_carve.h:96-160) | |
| 14 | CNeedRawMgr config | TRACKED (OQ-SA-2) | DDC:13040-13133, consulted at DDC:4243-4247 |
| 15 | The 200 MB global | **FILED S-054** (the TS family's 10× cap) | the global DDC:9049; the TS clamp DDC:35507-35514; core cr_registry.cpp:1027/:1038 at 2e9; `grep 2000000000\|0x77359400 DDC` → 0 (run twice) |
| 16 | Load-time table validation | CORE SUPERSET (informational) | cr_registry.cpp:1119-1202 |
| 17 | ZIP analyser structure | MATCH post-N-068/S-032/M-042; **the probe/walk dual gate** (probe `≤0x13 or ==0x62` DDC:18297 vs walk `≤10` DDC:18385) — core implements the walk gate (the net-tighter semantics); the artifact's §1 table conflated the two → **corpus-errata row**, not a divergence (lead-verified: the og's net admission = probe AND walk = ≤10) | |

### The family map (og family → core school)

BruteForce (23 classes; frame-sync/walk-to-EOF) → core's walker validators
(validate_ts/m2ts/mp3_raw/asf/jpeg/mpegps/es — present); RawSpeed (15; index-first
archives) → zip_common/rar4/rar5/tar/7z/mdb (present; DbxAnalyser = S-007's open
remainder); Brance (12; modern media/Apple) → dmg/x3f/ogg/rm/mxf/flv/riff-family/
svg/lzh/office-tag/crw (present per-format; the open children own the residue);
xjq (5; newest + the shared RAW base) → heic/jp2/m1v/mobi + tiff_common (present);
File\*Analyser (47; first generation + the fallback chains) → the D-75 registered
absence; Special (29) → the text school (M-099/N-113 filed) + aac/aif/7z present.

### Incidental format-level flags

- **F7 89 34 2C** = FileZIPAnalyser's SECOND registered key (DDC:2231-2247, id
  0x48 under both it and PK\x03\x04) — the census's undecoded ":82" row resolved;
  if real files carry this magic the og carves them as ZIP and core has no such
  row → **corpus-errata row** (the census decode) + a flagged follow-up (no
  evidence of real-world files this session).
- WMF's double-key to one analyser (0x90001 AND 0x9ac6cdd7, DDC:2301-2321)
  corroborates core's FMT_WMF/FMT_WMF_PL mirror (cr_registry.cpp:167-168) — MATCH.

---

## LANE B — the config/data-asset completeness pass (S11-lead/T7)

**Headline: every engine-behavior asset re-verified MATCH at byte/content level —
the first content-diff proof of the register's VERIFIED verdicts.** The full
inventory (6 tiers, 40+ assets, each with role + consumer citation) and the three
deep-dives (FileFilter.xml's 1,489/1,481 structure; raw_signs.json vs the two
core signature layers — near-disjoint by design, 138 json exts breadth-only, 80
registry exts precision-only, sqlite dual-registered; DataFile.ini+Dict) are in
the agent's sheet; the register extensions and security/hygiene data are filed as
**I-033**. The byte-identity diffs ran this session (FileFilter via
scripts/gen_filefilter.py's generated table; raw_signs per-extension attribution;
EUUserData and Dict direct content diffs — zero value diffs in all four).

---

## LANE C — the DRWUI decode + results-phase wiring (S11-lead/T8)

### The promotion-guard decode (OQ-S11-1 → RESOLVED; filed as M-112)

The object: `ThumbnailFactory::ThumbnailWorkItem` (ctor FUN_140263530
:422303-422341; 0x68 bytes). Layout: +0x40 result node (grade +0x73, type +0x70),
+0x48 source file node, +0x58 int request category (3 = "PreViewFile" preview-pane
raw-data; {0,1,2,4} = the list/icon tiers), +0x5c byte family×tier flag =
(Graphics-type ∨ fourcc∈{jpg,jpeg,tif,tiff} ∨ TxtView) ∧ category∈{1,2,4}.

The worker (FUN_1402636e0 :422377-422524): cancel/session gates → context build →
size gate 200 MiB (:422428, M-109's anchor) → non-media: lock, local_168 =
isTxtView (:422433), **local_160 = the recovery-read verdict** (:422442;
FUN_1803eba50 returns 0 = clean success with data, −1 cancelled, −2 no data,
−3 data-but-failed, :664341-664359); the TxtView branch renders the ≤100-byte
Arial-13 200×200 text snapshot (:422447-455 → FUN_1403d88e0 :652885-959) setting
local_167; the media branch extracts a frame (:422469-471); payload → MemThumbnail
(:422479-488) → AttachData decodes LATER (:652297+; cat-3 returns before decode
:652280-281); then THE GUARD (:422507-514, binary-verified 0x140263b2c-b9a):

```
if (grade != 2 && (textSnapshotRendered ||
    (!isTxtView && readVerdict==0 && (tierFlag || category != 3))))
    promote(node, 2);
```

The og promotes on READ evidence; the decode's failure never retracts. Core
(cr_ui.cpp:3737-3742) promotes only on decode Ok → **core under-promotes
damaged-but-readable images** (the inverse of the board-truth hypothesis), never
promotes text files (no snapshot surface — **N-144**) or media (deferred —
OQ-S10-1), and over-promotes ≤20-byte images (the og's dispatcher skips size
≤0x14, :421400-421402). Filed: **M-115** (the trigger semantics), **N-144** (the
text-snapshot surface), the tier-system decode registered (the tier VALUE names
{0,1,2,4}→view-modes unresolved — virtual callers), the AVPreviewStub media-preview
family (:677395-677451) handed to the preview-artifact maintainer, and M-109's
fix-plan enrichment (the og reads the full ≤200 MiB file BEFORE decoding — the
read-then-decode order matters for the memory-guard step).

### The results-phase interaction inventory

19 behaviors mapped (wiring-graph + interaction-log cited): filter/sort/search
axes COVERED (cr_ui.h:997-1074, the M-029/M-072 landings); preview/recover/
session/regrade families COVERED; the repair split FILED (S-050); the
auto-recover/view-recovered/ESC/quota/cache-lifecycle surfaces HOST-SIDE by
doctrine. No new engine-behavioral divergence beyond Part A's.

---

## LANE D — the nine-tree never-compared sweep (S11-lead/T9)

Binary-level coverage: CLEAN (the S10-F census re-confirmed; the register covers
1468/1470 files). The never-compared function space concentrates in the
EPM-heritage shared layer static-linked into the six PartitionRecovery DLLs:

- **D-1 → FILED M-113**: the recovered-OS boot-environment write-side surgery
  (genuefisystemfile.cpp: TempSystem MountedDevices rewrite :73731-73786; TmpBCD
  bootmgr {9dea862c…}→23000003→device-element 12000004 rewrite :74837-74887;
  TmpSoftWare CurrentVersion :73610-621/:76496-76504; SystemSetup/bootstat :42851)
  — zero core, zero corpus-artifact, the R-69 "PRESENT" verdict over-coarse.
- **D-2 → FILED M-114**: MainModule's System-Restore purge (srclient
  SRRemoveRestorePoint via WMI enumerate-all, 510-index fallback, BOTH drives of
  the copy/repartition pair, :138378-138420/:71496/:71511) — zero core, zero
  corpus documentation.
- **D-3 → FILED N-145**: the pendingMarkedDisk cross-run marker
  (../res/rwConfig.ini [QuickPartition], event 0x102d, :48164-48340/:123130-144).
- **D-4 → I-033 entry 9**: legacy.dll's consumer = the OpenSSL runtime (zero
  product-binary importers; closes N-134's decode).
- **D-5 → I-033 entry 11**: the VIRTUAL-DISK-NOTIFY-EVENT wait helper.
- **Corpus bookkeeping → I-033 entry 10**: ghidra-local ≡ ghidra-searchdll/
  NTFSSearchDLL (md5 `4d6ac40b…` both); EUVideoRepair duplicated across trees;
  ghidra-repair REPORT.md's three phantom "NO OUTPUT" binaries (zero og-tree
  presence).
- Verified-covered regions (no re-file): PRD error model (EUE_UNKNOWNEXCEPT is
  the only EUE string tree-wide — M-049's module-attribution), progress (S32),
  session (I-024/M-042), Storage Spaces (D-85/N-071), the driver wire (M-086/87/89
  pushed forward), the repair chain (M-021 MATCH re-verified with the exact
  LoadLibrary/GetProcAddress/binding cites; S-050 owns the line).

---

## LANE E — the core stand-in/stub register (S11-lead/T10)

Grep census: 14 patterns, ~700 raw hits, fully classified. **No TODO/FIXME/XXX/
HACK debt in shipped paths; no stubs; every named refusal owned.** The
stand-in register: 15 size-walk sites ALL owned by M-106/M-107 (the current-line
map recorded — the findings' cites drifted with the landings); 11 registered
deliberates (D-rows cited); the residue: **N-146** (FH11 zero-end, unowned), **N-147→dropped** (SVG/CSL 0x44c — convergent with the S11B lane's M-112, the richer decode), **N-148** (the comment-truth
batch: cr_ldm.h's stale UNDECODED vs OQ-T7's resolved MATCH; cr_zfs_engine.h vs
D-09's demanded wording; the EPS/AI 16-vs-64 MiB drift; six dangling
Resolution pointers into purged findings; cr_ui.cpp:3754's 9-lines-loose cite;
og_prop_flags decoded-but-unconsumed). Negative results recorded: the
AUDIT-FALLBACKS REMOVE list verified processed; data/ is the single registered
superset (D-104).

---

## The addenda landed with this evidence

1. **M-021 addendum** (Lane A row 13): the og's per-chunk repair feed — the og
   passes every scanned chunk's buffer to the EUVideoRepair interface
   (`vtable+0x40`, DDC:9623-9627; the init `vtable+0x30` with device+size,
   DDC:9226-9236), so the repair module sees ALL raw bytes, not just validated
   candidates; core's CarvePostProcessor (cr_carve.h:96-160) is per-candidate.
   The og's exact LoadLibrary/GetProcAddress/binding cites re-verified verbatim
   (DDC:9215/9228/9251/9261-9268) — the per-candidate seam itself is MATCH.
2. **CORPUS-ERRATA rows** (this session's additions): the census :20-23
   static/working refinement; the census :82 "Windows thumbnail/repair variant"
   = FileZIPAnalyser's F7 89 34 2C second key; the ZIP dual-gate conflation (the
   probe tier's 0x13/0x62 rule vs the walk tier's ≤10 — the artifact's §1 table
   merged them; core's net semantics MATCH); the ghidra-repair REPORT.md phantom
   entries; the ghidra-local duplicate.

## The lead's first-hand verification log (the no-hallucination gate)

- S-054's three cites re-read at filing (the clamp, the fallback, the global; the
  2e9 grep re-run: 0 hits). The ×10 comment's git origin traced (`a3abfbeb`).
- N-142's mask arithmetic re-derived (0x1000801 bits 0/11/24 over id−0x30) and
  the three ids cross-checked against the builder cites.
- M-112's guard re-read verbatim at :422507-514 (matches the agent's quote
  exactly); the :422433/:422442 trigger locals confirmed.
- The ZIP probe gate re-read (:18294-18302) and core's walk gate located — the
  net-semantics analysis that kept a false positive OFF the board.
- M-113's TempSystem chain (:73731-73786) and TmpBCD chain (:74837-74887)
  spot-verified; the repo-wide zero-greps re-run by the lead.
- M-114's srclient/GetProcAddress pair and the WQL string spot-verified.
- N-145's QSettings path/group/key strings spot-verified.
- Lane B's four byte-identity diffs reviewed (the scripts' outputs quoted in the
  agent's sheet).
