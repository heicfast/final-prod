# INFORMATIONAL I-033 — the config/data-asset register-extension batch: every engine-behavior asset re-verified byte-identical (FileFilter 1,481 / raw_signs 139+199+62 / EUUserData 111+190 / Dict 228/228); the register's consumer/posture corrections (needcopy→AliyunWrap, BUILDPE keeps Firebase, remote_config runtime-refetched, recoveryFree's concrete layout); legacy.dll's consumer closed (the OpenSSL runtime); corpus tree bookkeeping (ghidra-local is a duplicate, three phantom REPORT.md entries)

**Severity:** INFORMATIONAL (register/doc refinements and verified matches — no core defect; the lane's mandate was completeness, and the completeness verdict is MATCH across the engine-behavior tier)
**Module:** config/data assets / the corpus register / the reversed-tree bookkeeping
**Parameters affected:** D68 (documentation accuracy — the register rows), D69-adjacent (register hygiene)
**Corpus references:** the full inventory with per-asset citations in `evidence/s11-lead-deep-lanes.md` §Lane-B (Sections 1-3) and §Lane-D items D-4/D-5; the load-bearing cites: `exFATSearchDLL_decompiled.c:47165` (FileFilter's engine-side parse), `RAWSearchDLL_decompiled.c:23836-23850` (raw_signs load), `:13133-13200` (the [Filter] gate), `DRWUI_decompiled.c:274076` (Dict load), `AliyunWrap_decompiled.c:6579` (needcopy's true consumer), `:8723/8773/9726` (DataFile.ini's only consumers)
**Core references:** audited `461825d8`-lineage — the byte-identity proofs: `src/core/query/cr_filefilter_data.cpp:1509` (kCount=1481), `src/core/carve/cr_rawsigns_data.cpp:14-16` (139/199/62), `data/userdata-defaults.json` (111/111 keys, 190/190 signs — diffed this session), `src/core/recovery/cr_dict_data.cpp:17` (kDictPool 228/228 content-identical — diffed this session)
**Status:** READ-ONLY REGISTER — informational, no fix sought
**Prior audit check:** extends I-027 (the data-asset register), N-134 (the product-frame census), and the register's own rows (each extension cites its row); the byte-identity re-verifications are new (the register's VERIFIED verdicts were never before re-proven by content diff); the consumer corrections are new.

## Summary

The config/data-asset completeness pass (the user's mark-all-remaining mandate applied to the og's runtime-readable assets) re-verified every engine-behavior asset MATCH at byte/content level, and refined the register's truth in eight places: needcopy.txt's consumer is the Aliyun crash chain (not the installer), BUILDPE keeps the entire Firebase chain while dropping only Aliyun, remote_config_data is a runtime-refetched cache (root vs PE differ by 36,040 bytes), recoveryFree.ini's concrete quota layout is decoded, AliyunConfig.ini ships encrypted OSS credentials (a host-deployment security datum), the video-repair family ships dev-build debug logging in retail, FileFilter's compiled-in posture lacks the D-row its two siblings have, and legacy.dll's consumer is the OpenSSL runtime itself (closing N-134's open decode). Plus the reversed-tree bookkeeping: ghidra-local is a byte-identical duplicate of the S10-F-swept NTFSSearchDLL, and ghidra-repair's REPORT.md lists three phantom binaries that do not exist in the og tree.

## The register entries (each with its evidence)

1. **Byte-identity re-verification (four assets, first content-diff proof):**
   FileFilter.xml 1,489 entries/1,481 unique (the 8 cross-listed dupes reconciling
   the artifact-vs-CORRECTIONS count dispute — both right); raw_signs.json
   139/139 extensions, min/max identical after the KiB→bytes conversion, header
   and tail sign multisets AND per-extension attribution identical; EUUserData
   111/111 keys + 190/190 signs, zero value diffs vs `data/userdata-defaults.json`;
   Dict 228/228 entries content-identical to kDictPool. The register's VERIFIED
   rows confirmed by execution-grade diffing.
2. **needcopy.txt's consumer correction** (register row 98): `AliyunWrap_decompiled.c:6579`
   loads it — the crash-report chain's gather list (AliyunWrap itself + the CRT
   set), not an installer manifest. The row's classification outcome (skip)
   is unchanged; the row's text should be corrected.
3. **BUILDPE keeps the Firebase chain** (register 3-C's "drops the chain from
   WinPE" is Aliyun-only): firebasefetch.exe + firebaseonlineconfig.dll +
   google-services.json + remote_config_data + DRWReport.exe + DRW.exe all ship
   in the PE tree; `ls BUILDPE/EaseUS-x64 | grep -i aliyun` → empty (the
   falsifiable form).
4. **remote_config_data is runtime-refetched**: root 62,579 B vs PE 62,611 B,
   36,040 differing bytes — the "fetched cache + local fallback" decode's
   physical proof.
5. **recoveryFree.ini's concrete layout** (extends I-027 #1): the three nonzero
   QWORDs at +0x38 = 512 MiB, +0x60 = 2 GiB (the decoded EnableTotalSize
   default), +0x70 = 1.5 GiB.
6. **AliyunConfig.ini ships encrypted OSS credentials at the install root**
   (`:3-8`: LOGENDPOINT/ACCESSKEYID/ACCESSKEYSECRET/PROJECTNAME/LOGSTORENAME) —
   a host-deployment security datum for the register's row 51, beside N-097's
   plaintext auto.txt key datum.
7. **The video-repair family ships dev-build debug logging in retail**
   (reglog_win.txt's `movmp4repair.cpp[132] "new algorithm work"`;
   strutureReg.log's module "视频重组"/version "19.2回退分支"/environment
   "开发机器", level DEBUG) — product-hygiene datum for row 75.
8. **FileFilter's compiled-in posture lacks its own D-row** while raw_signs
   (D-103) and Dict (D-80) each have one — fold one sentence into D-103 or
   row 87 so the runtime-XML-vs-compiled question is not re-litigated per-asset.
9. **legacy.dll's consumer = the OpenSSL runtime** (closes N-134's open decode):
   zero importers/loaders across all 103 reversed trees' imports and decompiles;
   sole export `OSSL_provider_init` — the loader is libcrypto-3's provider
   machinery. N-134's fix-plan step 2 can close as "no og-binary consumer
   exists".
10. **Reversed-tree bookkeeping**: ghidra-local ≡ ghidra-searchdll/NTFSSearchDLL
    (md5-verified duplicate — the S10-F sweep's coverage claim extends to it for
    free); ghidra-misc/EUVideoRepair ≡ ghidra-repair/EUVideoRepair (same file,
    both trees); ghidra-repair REPORT.md's `EUZipRepairFile.exe` /
    `EURarRepairFile.exe` / `EUVideoRepairFile.exe` "NO OUTPUT" entries are
    phantoms (zero hits in og-codebase — the shipped bundle is 5 exes, all
    register-dispositioned).
11. **The `Global\20230920-VIRTUAL-DISK-NOTIFY-EVENT` wait helper**
    (RecoveryModule:73277-:73300): a tiny undocumented micro-region (likely
    VHD-attach coordination); recorded for completeness, no parity weight
    identified.
12. **RawConfig.ini's 34-key list as a completeness oracle** (register extension
    E1 — N-160's re-stated invariant, the original overstatement repaired):
    the keys minus the verified inert set equal the carve findings' format
    set. The inert set = keys with NO engine presence on either side
    (display-name keys only): STL/VPJ (no core carve row, no og analyser —
    they are not "grammar-covered"), plus INSP/INSV/CRM/FFF/DPX/MUSX/RVT
    (zero engine-side presence in the og); M2T rides the TS family's
    grammar; M4P is covered only as an MP4 brand alias. The uncovered-but-
    typed remainder is N-158's five OfficeTagFromXML branches. A falsifiable
    invariant any future key/format drift breaks (a cheap completeness
    tripwire for the carve board).

## Impact

Register truth for future sessions (the consumer corrections, the Firebase/PE
posture, the byte-identity proofs); two security/hygiene data points for the
host-deployment register (the OSS credentials, the dev-build logging); N-134's
open decode closed; the phantom REPORT entries and the duplicate tree recorded so
the next census doesn't double-count.

## Reproduction / verification

Every entry executed or code-cited (the content diffs ran this session — the
scripts' outputs are quoted in the evidence file; the consumer greps are
exhaustive over the reversed trees). Full record:
`evidence/s11-lead-deep-lanes.md` §Lane-B (all sections) + §Lane-D D-4/D-5.

## Fix plan (for the engineering team)

Register/doc only (the register is docs/ — read-only for the audit lane, so this
list is for the fix lane): apply entries 2/3/4/5/6/7 to the register's rows
(98/3-C/88/104/51/75); add the D-103 sentence (entry 8); close N-134's step 2
(entry 9); the corpus-side items (10/11) go to the corpus maintainers via the
CORPUS-ERRATA convention (already added this session — see the addendum commit).

## Tests to add

None (informational register; entry 12's invariant could become a doc-truth gate
check if the fix lane wants it).

## Related findings

I-027 (the data-asset register this extends); N-134 (the product-frame census —
entry 9 closes its decode); N-129 (the register-counts finding whose corrections
these refine); I-031 (the parallel infra register — complementary territory).

## Open questions

None.

<<<<<<< HEAD:diff-to-fix/findings/INFORMATIONAL/I-033-config-data-asset-register-extension-batch.md
---

## Session-42 extension addendum (2026-10-29, at core `869da779` / corpus `be0cfe9` — the config-asset deep-diff lane)

The full 12-group config/data re-diff (raw_signs, recoveryFree.ini, Dict,
dlconfig.xml, remote_config_data, PEtools.xml, DataFile.ini, the manifest/
log set, all nine config/ files, EUUserData, log/, SaveScan/) came back
**MATCH/REGISTERED across the board** — raw_signs.json re-verified
byte-exact (139/139 exts, 199/199 header signs, 62/62 tail signs; the
root↔BUILDPE copies md5-identical), EUUserData 111/111 keys / 190/190
signs with 0 value diffs, Dict 228/228, the SaveScan auto-naming contract
exact. Two register rows need refinement (fix-lane items; the register
itself is in docs/ — read-only for the audit):

- **Row 88 (remote_config_data) overstates**: "ZERO references across
  SearchDLL/MOM/PartitionRecovery artifacts" — `RAWSearchDLL_decompiled.c:9076/9084`
  DOES carry the `"filter_generalraw"` literal (verified first-hand this
  session: the engine asks DRWUI for the value via the naming hook;
  OQ-S4-2's decode — display-name mapping only, the ext-list map at
  global+0x18 written-never-read in-DLL — is the correct semantics; only
  the row's absolute phrasing is wrong). One-sentence repair citing
  OQ-S4-2.
- **Row 65 (config/version.ini) understates**: described as "Build-pair
  state file (PE census)" with no mention of the engine-enable keys —
  `scanext4`/`scanzfs` gate EXT4/ZFS QuickSearcher creation
  (`DRWUI_decompiled.c:913080-913112`, verified first-hand: license-vcall
  == 0 AND GetPrivateProfile("config/version.ini","scanext4")==0 → the
  EUCreateEXT4QuickSearcher case returns 0; same shape for
  scanzfs/EUCreateZFSQuickSearcher; ReFS/Btrfs cases 10/0xb carry NO
  gate). Shipped values: scanext4=1 present, scanzfs ABSENT → the free
  edition creates no ZFS searcher. The registered policy delta (3-F /
  I-016 note 1 / SCAN-ENGINE-DELTA Q1) should be cross-referenced from
  the row.

**New INFO datum for the register's fix lane (item 11's family):** the
`remote_config_data` file is the **verbatim WinHttp response body** cached
by `firebaseonlineconfig.dll` `FUN_180003010` (first-7-bytes `"fetched"`
check at `firebaseonlineconfig_decompiled.c:1703`, then `fwrite` of the
whole buffer) — NOT a client-serialized map. Length framing: `key\0
[len] value\0` with 1-byte lengths (`\x05false`), 2-byte-LE lengths
(`\x83\x02` = the 643-byte filter_generalraw array), and `\x00`-escaped
u16 (`\x00\x3d\x01` = the 317-byte json_pe). Full flag census this
session: 304 distinct keys / 598 key-tokens (33 enable_* with
__free/__abroad/__easeus segmentation; json_raw shield
"TXT;HTML;LOG;H;CPP;MM;" split on 0x3b at DRWUI :499195-499228; repair
flags → S-050; ~200 url_* + ~60 value_* monetization) — **no unregistered
engine-behavior flag exists** (the row 88 REPORTING-OOS disposition
holds).

**Related new finding:** N-154 (the one false cap-annotation the EUUserData
re-verification surfaced — the ext-length provenance comment).
=======
## Resolution

**Status:** ACKNOWLEDGED (the register-extension batch confirmed; the fix plan's register/doc items applied to the current tree)
**Fixed in commit:** this session's I-033 landing (session-42; cite-by-title: "fix(docs): I-033 - the config/data-asset register extensions applied — the consumer corrections, the PE posture, the byte-identity posture note, and N-134's step-2 closure")
**Verified by:** the load-bearing cites re-read first-hand this session: `AliyunWrap_decompiled.c:6579` (needcopy's consumer), the BUILDPE tree listing (firebase family present, aliyun absent), the root-vs-PE `remote_config_data` byte diff (62,579 vs 62,611, 36,040 differing), recoveryFree.ini's three nonzero QWORDs, AliyunConfig.ini `:3-8`, the reglog_win.txt/strutureReg.log debug lines; the corpus-side items (10/11 — the ghidra-local duplicate + the phantom REPORT entries) confirmed already in CORPUS-ERRATA's S11 batch.
**Resolution notes:** Entries 2/3/4/5/6/7/8/9 applied to their register rows: needcopy.txt's consumer corrected (the AliyunWrap crash-chain gather list, not an installer manifest — classification unchanged); 3-C's "og drops the chain from WinPE" corrected to the Aliyun-only truth (the firebase family ships in BUILDPE); remote_config_data's runtime-refetched cache datum added to the firebase row; recoveryFree.ini's concrete QWORD layout (+0x38=512 MiB / +0x60=2 GiB / +0x70=1.5 GiB) extends the quota note; AliyunConfig's encrypted-OSS-credentials security datum added; the video-repair family's dev-build debug logging added to the runtime-logs row; FileFilter's compiled-in posture note folded into its row (the D-103/D-80 sibling sentence — the runtime-XML-vs-compiled question is closed for all three assets); legacy.dll's row carries the N-134 step-2 closure (zero og-binary consumers; the loader is libcrypto-3's provider machinery — no .rsf-decryption hypothesis revival). The byte-identity re-verifications (FileFilter 1481 / raw_signs 139+199+62 / EUUserData 111+190 / Dict 228) stand as the register's MATCH proof set. No code change.
>>>>>>> origin/main:diff-to-fix/findings/FIXED/I-033-config-data-asset-register-extension-batch.md
