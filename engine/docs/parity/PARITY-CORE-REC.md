# PARITY-CORE-REC — Parity-A3 Audit: Raw Carving / Lost Partitions / LDM / Partition Tables

**Task ID:** 2-a3 · **Agent:** parity auditor (read-only) · **Date:** 2026-10-23

## Header

| Item | Value |
|---|---|
| Ours (auditee) | `core_recovery` @ HEAD `2473c03e` + `d42bb00` (clean tree, no local mods) |
| Corpus (ground truth) | `easeus-drw-research` (architecture/, re-workspace/artifacts/, re-workspace/reversed/, og-codebase/) |
| Scope | carve/*, lost/*, table/* (incl. LDM), gen_rawsigns.py, ldm_reference.*, 5 test files |
| Coverage (our code, line-by-line) | **17/17 in-scope source files read fully** (7,520 LOC); `test_refs_ldm.cpp` NOT read (ReFS-side, out of carve scope); `scripts/ldm_reference.c/h` (kernel verbatim copy) spot-verified at ~15 constants, not re-read line-by-line |
| Coverage (corpus) | All carving/lost/LDM artifacts located & read: `SearchDLL/rawsearch-carving--deep-dive.md`, `SearchDLL/rawsearchdll--analyser-registry-census.md`, `SearchDLL/rawsearchdll--family-{bruteforce,special,filegen}.md`, `SearchDLL/rawsearchdll--zip-analyser-carving.md`, `SearchDLL/raw-dual-registry--u16-magic-keys.md`, `SearchDLL/searchdll-family--architecture.md`, `MOM-modules/{LdmManager,PartitionRecoveryD,Partition}.md`, `architecture/05-scan-engine-architecture.md`, `config-files/config-data-reference.md` (targeted), `DRWUI-exe--main-ui-architecture.md` §9.2 (targeted); targeted decompiled verification: `ghidra-searchdll/RAWSearchDLL_decompiled.c` (JPEG/PDF/PST/ZIP sites), `ghidra-mo/{LdmManager,Partition,PartitionRecoveryD}_decompiled.c` (PRIVHEAD/VMDB/TOCBLOCK/GPT/lost-part sites), `DRWUI.exe/strings_ascii.txt` (naming) |
| Mandatory rawsigns byte-comparison | **DONE — full semantic diff of all 139/199/62 entries (supersedes the 10-spot-check): byte-exact MATCH** (see F-A3-1) |
| Method note | Prior agents' AUDIT.md rows are claims, not evidence; every verdict below cites either our file:line + corpus file:line/§/address, or is filed UNVERIFIED |

## Audit grid

| Subsystem / Axis | Verdict | Notes |
|---|---|---|
| **Carving — raw-sign data (constants)** | **MATCH** | 139 formats / 199 header / 62 tail signs byte-exact vs `og-codebase/raw_signs.json`; schema `{header:{signs:["off,hex,conf"]}, tail, min, max}` matches corpus `config-data-reference.md:8-12`; conf×1000 of 0.50–1.00 → 500–1000 ✓ |
| **Carving — registry/dispatch (data model)** | **DELIBERATE-DELTA** | Ours: 30-entry array + first-byte dispatch + kTrialOrder specificity (cr_carve.cpp:35-39). og: `std::map<u32 LE fourcc, chain>` + second u16-key registry + modern→legacy fallback chains (census §1-3). Behavior (shared-magic resolution, first-valid-wins) equivalent; mechanism differs |
| **Carving — JPEG grammar** | **MATCH w/ P3 deltas** | Marker walk, BE segment lengths, 0xFF desync break — all confirmed at og `RAWSearchDLL_decompiled.c:39640-39680`; deltas: SOF precision-8 check absent (F-A3-8), our 3-byte `FFD8FF` prefix is broader than og's 4 keys E0/E1/DB/EE |
| **Carving — PDF grammar** | **MATCH (core) / UNVERIFIED (EOF rule)** | `%PDF-` magic + version grammar confirmed (og 41230-41233: `0x46445025` + `param_2[1]=='-'`); og's %%EOF selection rule not decoded — our last-%%EOF-before-next-`%PDF-` bound is self-justified (cr_analysers.cpp:460-495) |
| **Carving — ZIP** | **MISMATCH** | og accepts 3 anchors (LFH/CD/EOCD) + salvages indexless zips + padding extension; ours: LFH anchor only, fails closed without geometry-consistent EOCD (F-A3-3) |
| **Carving — PST** | **MATCH (constants) / MISMATCH (unknown version)** | 0x0E→size@0xA8, 0x17→size@0xB8, 1 MiB floor all verified byte-exact at og `FUN_180041eb0`; but og *accepts* unknown versions with the 1 MiB floor, ours rejects (F-A3-5) |
| **Carving — TIFF family** | **MISSING (part)** | BE-TIFF (`MM\0*`, og key 0x2a004d4d) and ORF variants IIRS/MMOR not reachable — registry only carries II-magics (F-A3-4) |
| **Carving — raw-sign layer semantics** | **DELIBERATE-DELTA (documented)** | 512-stride probes, header-only extent, (h+t)/2+100 pairing implemented per cr_rawsigns.h:11-21 which itself documents "exact hit handler was not RE'd" — honest. New: raw-sign can re-claim grammar-rejected starts (F-A3-12, undocumented) |
| **Carving — pass order / dedup / min_size / hashing** | **MATCH (design) / DELTA (mechanism)** | grammar→sign order matches og "C++ chain first, then JSON signs" (census §5); F9 extent dedup is ours (og correlates in the merge phase, deep-dive §4); min/max gates ✓ corpus ("Size bounds prune absurd candidates"); ≤8 MiB truncated-SHA256-256→128-bit id is our design (og silent) |
| **Carving — capability claims** | **MATCH (honest) w/ gaps** | AUDIT.md row 32 honestly lists missing og analysers (gzip/BMP/GIF/OGG/…); code headers make no og-parity claim for the 30-format set |
| **Carving — naming** | **MISMATCH (string)** | Architecture MATCH (host callback, og DRWUI vtable+0x48, deep-dive §4); og host string is `FILE%d.%s` (DRWUI strings_ascii.txt:22735) vs ours `Carved%06u.%s` (cr_scan.cpp:559) / engine default `File%06u` (F-A3-7) |
| **Carving — threading** | **DELTA (ours)** | og carve threading UNDECODED in corpus; ours: window fan-out + ResultTree mutex + per-worker overlap sets (verified `cr_node.h:238-240` mutex) — no og evidence either way |
| **Lost partitions — probe/stride** | **MATCH (probe) / UNVERIFIED (stride, scoring)** | 55AA@+510/+511 + OEM@+3 vs `"NTFS    "` confirmed in og `PartitionRecoveryD_decompiled.c:10334-10365`; stride/scoring/evidence-tags not decoded in corpus (generator/parser class census only) |
| **Lost partitions — classification/dedup/materialize** | **MATCH (our contract)** | Inside/Overlaps/Contains ✓ (cr_candidates.cpp:158-175); reported-but-marked Inside ✓ (-20); unknown-BPB sizing from device end ✓ (row 31b fix verified in code, cr_scan.cpp:390-398 passes live table — row 31a fixed) |
| **LDM — database constants** | **MATCH** | PRIVHEAD ver 2.11/2.12, geometry @0x11B/0x123/0x12B/0x133, GUID@0x30 — verified against og `LdmManager_decompiled.c:59695-59775`; VMDB 4.10 gate + vblk_size==0→0x80 default verified at og :59515-59540; TOC copies 1/2/2045/2046 + PRIV copies 6/1856/2047 = kernel `ldm_reference.h:66-79` ✓; VMDB-vs-TOC size agreement check ✓ (og :59517-59521, ours degrades instead of aborting — documented) |
| **LDM — VBLK/graph/layouts** | **MATCH (vs kernel/libldm) / UNVERIFIED (vs og)** | VBLK types 0x32/0x33/0x34/0x44/0x35/0x45/0x51, flags 0x10/0x08/0x08/0x08/0x20/0x80/0x02, per-type size gates, composite ≤4 fragments + duplicate-fragment poison — all consistent with `ldm_reference.{c,h}` (e.g. vol5 base 0x18, VBLK_SIZE_VOL5=58 floor); RAID-5 left-symmetric `P=(N-1)-(row%N)` is libldm `raid5_ls` — og's own rotation math not decoded (F-A3-11) |
| **LDM — volume devices** | **MATCH (classes) / UNVERIFIED (og math)** | og ships Simple/Span/Stripe/Mirror/Raid5 VolumeStorage (vftables ×4, `LdmManager_decompiled.c:63342-70097`); XOR reconstruction evidenced only by crash-string `Raid5VolumeStorage::ComputeDataWithOtherDisk` (:88616); `WriteParityDisk` claim not findable (F-A3-11) |
| **GPT/MBR — constants** | **MATCH** | `EFI PART` u64 0x5452415020494645 @LBA1 (og Partition_decompiled.c:1293/4208/4813); MBR 0xAA55@+0x1FE (og :1694/:6312); protective 0xEE@+0x1C2 (og :4185); backup header at last LBA (og reads `param_2-1`, :4211-4215); CRC32 IEEE (og `GPT_ChksumCrc32`, header over 92B w/ field zeroed, og :4834-4850; entries CRC og :4867-4873) — ours cr_crc32.cpp + cr_table_parse.cpp:46-64 |
| **GPT/MBR — validation depth** | **MISMATCH (doc-vs-code) + P3 gaps** | Ours: signature/entry_size/count/header_size-range/CRCs only; og adds revision==0x10000, reserved==0, header_size==0x5C exact, current/backup LBA cross-checks, usable-range bounds (og :4810-4857). Backup-salvage path dead code (F-A3-2, F-A3-9) |
| **GPT — LDM GUIDs** | **MATCH (post A-002 fix)** | History: the A-001 remediation `1cb6a43` replaced both LDM constants with fabricated values ({5808c8aa-8e9f-...} / {af9b60a0-384e-...}) present in no og binary, no Microsoft source, and no real disk — regressing every real GPT dynamic disk to ERR_BAD_MAGIC (A-002; the A-001 LDM rows were a false positive from a mixed-endian mis-decode, its harness having encoded the fabricated bytes). A-002 restored the REAL constants: PARTITION_LDM_METADATA_GUID {5808c8aa-7e8f-42e0-85d2-e1e90434cfb3} and PARTITION_LDM_DATA_GUID {af9b60a0-1431-4f62-bc68-3311714a69ad} (winioctl.h / Microsoft Learn; same bytes extracted first-hand from LdmManager.mo DAT_1800c4960 and Partition.mo DAT_180036630/DAT_180036640 — the og GPT-LDM gate memcmp target in FUN_1800724f0; Linux kernel efi.h agrees). Regression-guarded, non-self-referentially, by Gpt.CanonicalTypeGuidConstantsMatchRealDiskBytes, Gpt.LdmGuidConstantsMatchMicrosoftWinioctlText, Ldm.GptDynamicDiskCanonicalGuidsParse (real Microsoft bytes hard-coded in the fixture), Ldm.GptDynamicDiskFabricatedGuidsRejected (negative guard) and Ldm.GptDynamicDiskTableClassifiesLdmEntries |
| **EBR chains / extended 0x05/0x0F** | **MATCH (standard) / UNVERIFIED (og)** | Our chain walk + cycle guard (cr_table_parse.cpp:107-173) is spec-standard; og EBR specifics not decoded |

## Findings

### F-A3-1 — rawsigns data table vs corpus raw_signs.json: byte-exact (MANDATORY CHECK — PASS)
- **Priority:** P1 (verification result, no fix) · **Axis:** Constants & offsets · **Verdict:** MATCH
- **Ours:** `src/core/carve/cr_rawsigns_data.cpp:14-16` (kFormatCount=139, kHeaderSignCount=199, kTailSignCount=62), full arrays.
- **Corpus:** `og-codebase/raw_signs.json` (139 extensions; 93 header-only + 46 header+tail, programmatic recount).
- **What I did:** regenerated the table with `scripts/gen_rawsigns.py` into a temp dir from the corpus JSON, then parsed BOTH the generated and the checked-in C++ (comment-stripped, array-semantic parse asserting per-sign length) and compared every tuple `(offset, bytes, conf, fmt)` and `(ext, min, max)`:
  - kFormats: **139/139 identical**; kHeaderSigns: **199/199 identical**; kTailSigns: **62/62 identical** (offsets, every byte, confidences ×1000, format indices, sort order).
  - Raw `diff` shows only clang-format re-wrapping (checked-in file is 3-per-line formatted; generated is 1-per-line) — zero semantic deltas.
  - Spot-checks cited for the record: sqlite hdr `0,53514c69746520666f726d,0.92`→`{0,…,920,…}`; sqlite tail `31,111d0515190f0f776f7264,1.00`→`{31,…,1000,…}` (sorted idx 38); tgz `0,1f8b0800,0.63`→630; flac `664c614300000022,0.94`→940; evtx `456c6646696c6500,0.55`→550; lnk `4c00000001140200…,0.91`→910; msi/OLE `d0cf11e0a1b11ae1,0.68`→680; nes `4e45531a,0.99`→990 + tail `36,4420…,1.00`; vob `10,0189c3f8,0.86`→860 @off10.
  - Bounds: corpus max header offset 40, max tail offset 46, max sign len 32 — our `kMaxSignBytes=40`, `buckets_[47*256]`, probe loop `off<47` (cr_rawsigns.h:36,95; cr_rawsigns.cpp:36) all cover with margin; gen_rawsigns.py:20 comment "corpus max observed: 32" verified true.
- **Description:** the compiled layer-1 table faithfully reproduces the og's carving database.

### F-A3-2 — GPT salvage claim is doc-fiction; backup-header path is dead code
- **Priority:** P2 · **Axis:** Behavior / Error surface · **Verdict:** MISMATCH (doc-vs-code) + dead code
- **Ours:** `include/cr/table/cr_table_parse.h:69-71` claims "GptDamaged when CRC checks fail on BOTH copies — partitions still extracted with per-entry best effort (salvage-over-purity, fix F7/F10)". Code: `src/core/table/cr_table_parse.cpp:203-268` — `last_lba` is set **only inside `if (rc_p == OK)`** (:207-213), so `load_gpt_side(device, last_lba, …)` (:241) runs only when the primary fully validated; the `if (!out.gpt_primary_ok)` backup-salvage branch (:244-264) is **unreachable**. A primary header with valid `EFI PART` signature but bad header/entries CRC → `GptDamaged` with **zero** partitions.
- **og evidence:** `ghidra-mo/Partition_decompiled.c:4205-4218` (CGPTDiskAnalyzer): when LBA1 lacks `EFI PART`, og re-reads at **last LBA** (`param_2 - 1`) and accepts the backup — i.e. og detects GPT via the backup when the primary header is destroyed.
- **Suggested fix:** when the LBA1 signature is present but `load_gpt_side(1,…)` fails (or LBA1 lacks the signature but a protective 0xEE entry exists), probe the last LBA for a backup header and salvage entries from it; on entries-CRC-only failure, extract entries best-effort (flag `gpt_*_ok=false`) to match the documented contract.

### F-A3-3 — ZIP carving: single anchor, fails closed where og salvages
- **Priority:** P2 · **Axis:** Behavior / Capability · **Verdict:** MISMATCH
- **Ours:** registry binds only `PK\x03\x04` (cr_registry.cpp:80,87); `validate_zip`/`validate_docx` require a geometry-consistent EOCD (entries>0, cd_offset≥30, cd fits before EOCD — cr_analysers.cpp:517-538) → an indexless ZIP (LFH entries, CD destroyed) is **rejected**.
- **og evidence:** `SearchDLL/rawsearchdll--zip-analyser-carving.md` §1-4 (decompiled `FUN_1800176e0/176b0/180045690`): analyser accepts all three anchors — `PK\x03\x04` (with u16@+8 sanity gate ≤0x13/==0x62, verified `RAWSearchDLL_decompiled.c:18296-18301`), `PK\x01\x02` CD entry ("preferred anchor"), `PK\x05\x06` EOCD; computes size as the **sum of parsed entries**; indexless zips return status 0xd (still carved, flagged); extends the end through sector-aligned zero padding (last-16-bytes-zero test, 5.24 MB cap).
- **Suggested fix:** register CD and EOCD as additional ZIP anchors (a `header_offset`-style entry or a dedicated validator invoked at those magics); add an indexless-LFH salvage mode (walk LFH chain, mark low-confidence/fragmented); consider the zero-padding extension for slack-aligned ends.

### F-A3-4 — Big-endian TIFF + ORF variants unreachable (dead validator branch)
- **Priority:** P2 · **Axis:** Constants / Capability · **Verdict:** MISSING (known in AUDIT.md row 32, invisible in code)
- **Ours:** registry magics are II-only: TIFF `{'I','I',0x2A,0x00}` (cr_registry.cpp:57), ORF `IIRO` (:182-191), RW2 `IIU\0` (:202-211). `tiff_common`'s BE branch (`bo[0]!='I'` → `MM` path, cr_analysers.cpp:286-291,300,306-312) is **dead code** — no registry magic can reach it.
- **og evidence:** `SearchDLL/rawsearchdll--analyser-registry-census.md` §3: og keys `0x2a004d4d` (`MM\0*`), `0x2a4949` (`II*\0`), `0x554949` (`IIU`), `0x4f524949` (`IIRO`), **plus** `0x53524949` (`IIRS`) and `0x524f4d4d` (`MMOR`) for Olympus ORF; also `raw-dual-registry--u16-magic-keys.md` §2 key 0x4D4D `MM`.
- **Suggested fix:** add `FMT_TIFF_BE` (magic `MM\0*`, reuse `tiff_common`), and ORF magics IIRS/MMOR (either extra registry rows or a small magic-alias table). At minimum add a code comment that BE-TIFF is a known gap so the dead branch isn't mistaken for support.

### F-A3-5 — PST unknown-version handling inverted vs og
- **Priority:** P2 · **Axis:** Behavior · **Verdict:** MISMATCH
- **Ours:** `cr_analysers.cpp:729-735`: version must be 0x0E or 0x17, else `return r` (reject). Comment: "unknown generation: reject (corpus floors others)".
- **og evidence:** `ghidra-searchdll/RAWSearchDLL_decompiled.c` `FUN_180041eb0` (the function our own comment cites): default size `uVar1 = 0x100000` (1 MiB); if version@+0x0A == 0x0E → size=u32@+0xA8; if == 0x17 → size=u32@+0xB8; **any other version → size stays 1 MiB and the function returns 1 (ACCEPT)**. The og floors the *size* for unknown generations; it does not reject them.
- **Suggested fix:** accept unknown versions with the 1 MiB floor (keep our wMagicClient/AMap hardening gates — they are documented additions); fix the comment.

### F-A3-6 — CarveStats double-counts raw-sign hits
- **Priority:** P3 · **Axis:** Data model (stats) · **Verdict:** MISMATCH (bug)
- **Ours:** `src/core/carve/cr_carve.cpp:412-415` — `stats.sign_hits += ctx.sign_hits.load(); stats.sign_validated += ctx.sign_validated.load();` appear **twice** (duplicated lines).
- **Effect:** reported sign_hits/sign_validated are 2× actual. Behavior unaffected; UI/session stats wrong.
- **Suggested fix:** delete the duplicated pair.

### F-A3-7 — Carved-file naming strings diverge from og host policy
- **Priority:** P3 · **Axis:** Behavior / Capability · **Verdict:** MISMATCH (string) / MATCH (architecture)
- **Ours:** engine default `File%06u.<ext>` (cr_carve.cpp:303-314, comment "corpus default policy"); orchestrator host namer `Carved%06u.%s` (cr_scan.cpp:556-564).
- **og evidence:** architecture matches ours — naming lives in the host via `std::function<string(string,string)>` at vtable+0x48 (`SearchDLL/rawsearch-carving--deep-dive.md` §4; `DRWUI-exe--main-ui-architecture.md` §9.2). The actual host format string is **`FILE%d.%s`** (`re-workspace/reversed/DRWUI.exe/strings_ascii.txt:22735`), plus `Lost Name File (%d).%s` for unnamed hits. No `Carved%`/`File%06` string exists anywhere in DRWUI or RAWSearchDLL strings.
- **Suggested fix:** align the orchestrator namer to `FILE%d.%s` (or document the deliberate delta); fix the cr_carve.h:44 "corpus default policy" comment (corpus string is uppercase, unpadded).

### F-A3-8 — JPEG SOF precision gate absent; prefilter broader than og
- **Priority:** P3 · **Axis:** Constants & offsets · **Verdict:** DELTA
- **Ours:** `cr_analysers.cpp:196-197` sets `saw_sof` for markers C0/C1/C2/C3 with **no precision check**; registry magic is 3-byte `FF D8 FF` (cr_registry.cpp:46) which also matches FFD8FFE2/E3/… that og does not key.
- **og evidence:** `RAWSearchDLL_decompiled.c:39653`: `(bVar1 == 0xc0) && (pbVar25[4] == 8)` — SOF0 **with precision 8** only; registry keys exactly FFD8FFE0/E1/DB/EE (census §3; `searchdll-family--architecture.md` §4.2).
- **Suggested fix:** optional hardening — require `precision==8` on the SOF segment (offset +4 within the segment) for C0; keep the broader 3-byte probe (validated by grammar) but note it as a deliberate superset.

### F-A3-9 — GPT header validation shallower than og (revision/reserved/LBA cross-checks)
- **Priority:** P3 · **Axis:** Behavior / Error surface · **Verdict:** DELTA (permissive) + MISSING checks
- **Ours:** `cr_table_parse.cpp:36-52` checks signature, entry_size==128, entry_count bounds (≤1 MiB array), header_size∈[92,512], header CRC over header_size, entries CRC.
- **og evidence:** `ghidra-mo/Partition_decompiled.c:4810-4857` additionally requires revision==0x00010000, reserved(+0x14)==0, header_size==0x5C **exactly**, current_lba==1 (primary) / ==last (backup) with backup_lba mirrored, first/last_usable within disk and bounding entries_start; CRC over fixed 0x5C bytes.
- **Suggested fix:** add the cheap structural cross-checks (revision, reserved, current/backup LBA consistency, usable-range sanity) as *degradation* signals (layout stays Gpt/GptDamaged per our salvage stance) rather than hard rejects; document the permissiveness as deliberate.

### F-A3-10 — LDM config-size gate: og rejects >1 MiB, ours clamps at 4 MiB; og checks real disk size
- **Priority:** P3 · **Axis:** Behavior / Constants · **Verdict:** DELTA (partially documented)
- **Ours:** `cr_ldm.cpp:1101-1107` clamps `cfg_secs` to 8192 (4 MiB) with comment "spec: 1 MiB"; geometry gate compares `lds+ldsize > config_start` (kernel rule, cr_ldm.cpp:1091-1092).
- **og evidence:** `ghidra-mo/LdmManager_decompiled.c:59753-59760`: `config_size > 2048` sectors → "ConfigSize larger than 1m ?." → **error 0xfffffff8 (abort)**; og also validates the public region against the **real disk size** (:59718-59727, "PRIVHEAD size doesn't match real disk size" / "disk and database overlap").
- **Suggested fix:** tighten the clamp to 2048 sectors (or keep 4 MiB but mark `stats.degraded`), and add the real-disk-size validation as a degradation signal; cite the og sites in the comment.

### F-A3-11 — LDM doc claims not backed by corpus: `WriteParityDisk`, og RAID-5 layout math
- **Priority:** P3 · **Axis:** Capability claims / Constants · **Verdict:** UNVERIFIED (doc claim)
- **Ours:** `include/cr/table/cr_ldm.h:26-28` — "corpus LdmManager.mo: full og parity confirmed (…Raid5VolumeStorage incl. WriteParityDisk / ComputeDataWithOtherDisk)".
- **og evidence:** the five VolumeStorage classes exist (vftables, `LdmManager_decompiled.c:63342/68901/69239/69557/70070` — matches the ×4 census in `MOM-modules/LdmManager.md`); `ComputeDataWithOtherDisk` appears only as a crash-handler string (:88616). **`WriteParityDisk` is not findable anywhere in the 91,475-line decompilation** (case-insensitive search). The og's parity-placement/rotation formula is not decoded in any corpus artifact; our left-symmetric `P(row)=(N-1)-(row%N)` is cited to libldm `raid5_ls` (legitimate independent ground truth, and test-verified in `test_ldm.cpp:263-335`), but "full og parity confirmed" overstates what the corpus shows.
- **Suggested fix:** reword the header ledger: "class census confirmed; og RAID-5 read/reconstruction math not decoded — layout per libldm raid5_ls"; drop or evidence `WriteParityDisk`.

### F-A3-12 — Raw-sign layer can re-claim starts the grammar layer rejected
- **Priority:** P3 · **Axis:** State machine · **Verdict:** DELTA (undocumented)
- **Ours:** `cr_carve.cpp:181-183` skips raw-sign probing only at `emitted_at` positions (grammar-**validated** or dedup-resolved). A start where e.g. the SQLite *grammar* validator failed (damaged page count) is still probed by the raw-sign layer's `sqlite` extension (same ext exists in kFormats) and can be emitted header-only.
- **og evidence:** `SearchDLL/rawsearchdll--analyser-registry-census.md` §5: "the engine consults BOTH: magic-key registry first (C++ chain), then the JSON signs for **unclaimed** extensions" — for extensions with a C++ analyser, the chain's verdict is final; the JSON layer does not second-guess it.
- **Suggested fix:** either suppress raw-sign probing for extensions that overlap a grammar FormatId (sqlite is the only current collision — check `kFormats` vs registry exts), or document this as a deliberate salvage superset in cr_rawsigns.h's divergence list.

### F-A3-13 — No per-extension runtime gate (RawConfig.ini) and no user-format layer (EUUserData)
- **Priority:** P3 · **Axis:** Capability · **Verdict:** MISSING
- **Ours:** `CarveFlags` (cr_carve.h:35-39) has only hash/threads knobs; no per-format enable, no user-defined signature hook.
- **og evidence:** engine gated per-extension at runtime via `config/RawConfig.ini` (37 extension toggles, loaded at decompiled line 13133 — deep-dive §4); `GetRawUserInfoMap` export exposes the user extension→analyser bindings (deep-dive §1; census §1); `EUUserData` is a second magic/MIME DB (searchdll-family §4.3).
- **Suggested fix:** add a per-format enable mask to CarveFlags (config-file driven), and note EUUserData as an unimplemented og layer in capability docs.

### F-A3-14 — Tail-pair search bound (≤2 MiB) contradicts header doc (dead on current corpus) — RESOLVED by S-015
- **Priority:** P3 · **Axis:** Constants / Doc · **Verdict:** RESOLVED (diff-to-fix S-015, session 3)
- **Ours (post-S-015):** the tail walk is now a chunked forward scan with a NAMED bound (`kRawSignTailScanLimit` = 8 MiB, `cr_rawsigns.h`) documented against the og's true full-window behavior (FUN_1800205b0 streams the whole [min<<10, max<<10) window, up to 200 MiB); the bound is a documented divergence (tails beyond it emit header-only). The min/max units are decoded KiB (S-015) and converted once in the generator, so max−min can reach ~190 MiB — the old `hi - lo <= 2 MiB` gate would have skipped ALL pairing on the true-unit table; the chunked walk restores it within the bound.

### F-A3-15 — Carve scope: full-slice scan vs og's unallocated+free-space targeting
- **Priority:** P3 · **Axis:** Behavior / Threading (cost) · **Verdict:** DELTA (documented in spirit)
- **Ours:** `cr_scan.cpp:560` carves the whole slice; F9 extent dedup against the structured tree suppresses re-finds (cr_carve.h:48-51).
- **og evidence:** `SearchDLL/searchdll-family--architecture.md` §6.4: "RAWSearchDLL carves by signatures across **unallocated clusters (volume bitmap from FS engine) + free space**".
- **Suggested fix:** none required for parity of results; note the IO-cost delta and consider bitmap-restricted carving as an optimization.

### F-A3-16 — Un-evidenced code comment: "the og validators reject RAR5 entirely"
- **Priority:** P3 · **Axis:** Capability claims · **Verdict:** UNVERIFIED (comment)
- **Ours:** `cr_analysers.cpp:760-761` — RAR5 validator introduced as "clean-room improvement" on the claim that og rejects RAR5.
- **og evidence:** og binds `Rar!` (0x21726152) to `RawSpeed::RARAnalyser → FileRARAnalyser` (census §3) — a key that matches RAR5 files' first 4 bytes too; whether the og analyser accepts or rejects the RAR5 block grammar is **not decoded** in any corpus artifact.
- **Suggested fix:** soften the comment to "og RAR5 handling not decoded; validator is a clean-room addition", or verify against `FileRARAnalyser` in the decompiled tree.

### F-A3-17 — Lost-partition scoring/evidence-tag model has no corpus ground truth (honesty check passed)
- **Priority:** P3 (informational) · **Axis:** Data model / Constants · **Verdict:** UNVERIFIED (by corpus), internally consistent
- **Ours:** points 15/25/30 (boot-sig/bpb-valid/mft-probe), FAT 20/10, exFAT 15/20, GPT 10/20, overlap −20/−10 (cr_candidates.cpp:62-154,275-290); tags match the header contract (cr_lostpart.h:27-30).
- **og evidence:** `PartitionRecoveryD.mo` exposes `CRecoverySearchEngine` + `ILocationGeneratorSet`/`CGeneratorSetCompleteSearch`/`CGeneratorSetRegSearch` + per-FS block parsers (`MOM-modules/PartitionRecoveryD.md`), persists `lostpart.dat`/`partinfo.dat`, and DRWUI logs "Insert lost partition %s, %d, StartSector=%I64d, TotalSector=%I64d" (DRWUI §9.2) — but no scoring constants, strides, or tag vocabulary are decoded anywhere in the corpus. The NTFS `"NTFS    "`@+3 OEM probe we use IS og-confirmed (`PartitionRecoveryD_decompiled.c:10334-10365`).
- **Suggested fix:** none possible until the generator/parser functions are decoded; keep the claims framed as "our contract" (they currently are).

## UNVERIFIED (explicitly not covered / no corpus evidence either way)

1. **og raw-sign hit handler internals** — 512-byte stride probing, tail-search anchoring (at `start+min` vs `start`), header-only extent rule `(max, device-end)`, and the exact header+tail confidence combination. Our cr_rawsigns.h:11-21 marks these as divergences; corpus is silent beyond "header + tail confidences rank overlapping candidates" (searchdll-family §4.4, config-reference:10). The stride claim in the task brief is *our* documented guess, not corpus fact.
2. **og lost-partition discovery strides, scoring constants, dedup order, CalcPartition semantics** — corpus has `CalcPartition.log` samples and class censuses only.
3. **og RAID-5 parity rotation + WriteParityDisk + mirror failover order** (see F-A3-11).
4. **og JPEG EOI-termination and SOF+SOS acceptance rule** — the marker walk is confirmed; the termination/acceptance specifics are ours (stricter: we require SOF+SOS before EOI).
5. **og PDF %%EOF selection rule** (ours: last %%EOF before next `%PDF-`).
6. **og ZIP end-padding extension exact stop conditions** beyond the artifact's pseudocode; og LFH sanity-field semantics at +8 (`version_needed` vs `method` reading differs between artifact text and decompiled shape).
7. **og EBR chain-walk specifics** and the og MBR-type→FS mapping table beyond 0xEE/0x42 (ours is the public-convention mapping, cr_table_parse.cpp:74-105).
8. **og GPT LDM metadata/data GUID bytes** — verified against Microsoft/kernel/libldm only (F-A3 grid note).
9. **og carve threading model** — ours (window fan-out) has no og counterpart decoded.
10. **og hash-at-carve behavior** — our ≤8 MiB truncated-SHA256 128-bit id is undocused in corpus.
11. **`analyser-granular-batch1--pst-rar-mkv-heic.md`, `family-brance/rawspeed/xjq` details, `engine-matrix--factory-contract-verification.md`, `per-binary/*` 57-line stubs** — skimmed/not read (lower-value duplicates or out of scope); `test_refs_ldm.cpp` not read (ReFS detection, adjacent but out of the carve/lost/LDM scope).
12. **FileRepair PDF/Office repair artifacts** — out of scope (repair chain, not carving).

## Deliberate-delta candidates (documented, defensible)

- Parallel carve (threads>1) with ResultTree-serialized emission + per-worker overlap sets — ours; no og evidence (grid).
- F8 fail-closed grammar registry (validator mandatory) vs og's modern→legacy dual-validation chains — different philosophy, same false-positive goal; our raw-sign layer preserves og's header-only breadth.
- F9 first-extent+size-class dedup vs og merge-phase correlation.
- 128-bit SHA-256 content id at emit (≤8 MiB).
- LDM recovery stance: damaged VBLKs counted+skipped (kernel/og abort), uncommitted VMDB parsed with `stats.degraded`, config-region clamped (F-A3-10 wants a tighter clamp), backup-PRIVHEAD fallback before giving up (cr_ldm.cpp:1054-1071) — the og aborts on corrupt PRIVHEAD ("Cannot find disk%d PRIVHEAD structure…Aborting", og :59769-59776).
- PST hardening (wMagicClient "MS" gate + AMap readability probe) — documented additions on top of og-verified constants.
- Host-side NameCallback architecture — matches og; the *string* is a delta (F-A3-7).
- RAR5/DNG/HEIC/WEBP/7Z validators — clean-room additions beyond confirmed og analysers (og has C7ZIPAnalyser, RawFormat_xjq::HEIC — so 7z/HEIC have og counterparts; DNG rides og's TIFF-family analysers per multimap note; WEBP unconfirmed). S-017 (session 3): the four ftyp-family signatures (MP4/MOV/M4A/HEIC) are REACHABLE now — previously the magic baked literal zero box-size bytes and no real BMFF file could ever match; keyed on "ftyp"+header_offset 4 with an F8 self-probe guarding the class (see diff-to-fix S-017 Resolution).

## Bottom line

The two data-faithfulness pillars of this scope are solid: **the raw_signs.json compilation is byte-exact (F-A3-1)** and **the LDM database constants are og-verified where the corpus permits** (PRIVHEAD/VMDB offsets, version gates, vblk 0x80 default, VMDB-TOC agreement check). The defects found are concentrated in (a) salvage paths our own docs promise but the code doesn't deliver (GPT backup/CRC salvage — F-A3-2), (b) og salvage behaviors we fail-closed on (ZIP anchors/indexless — F-A3-3; PST unknown versions — F-A3-5), (c) unreachable og format variants (BE-TIFF/ORF — F-A3-4), and (d) small honesty gaps in comments (F-A3-7/11/16) plus one real stats bug (F-A3-6). No P0/P1 wrong-behavior-vs-ground-truth findings; nothing in the carve core contradicts the corpus on magic bytes, size gates, or the layered architecture.
