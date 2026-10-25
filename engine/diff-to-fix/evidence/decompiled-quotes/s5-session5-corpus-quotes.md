# Session-5 corpus quotes — the never-cited artifacts read in full (evidence pack)

Provenance: every artifact below was NEVER cited by any prior session (verified by the
session-5 citation-gap sweep: 83 unique corpus paths cited across sessions 1-4 vs the full
artifact tree). Quotes are verbatim; line numbers are the artifact files' own.

## SaveScan/rsf-record4--tree-node-grammar.md (first read any session)

- Node grammar: wrapper type_flags (incl. the −0x10 hidden bit), flag_72/flag_73 bit
  fields (flag_73 bits 5-7 = grading per S-023), f_6c/f_68 = 0xFFFF sentinels, f_71 enum
  with 2↔5 load-time remap, root-role u32s, fixup field; per-node marker ∈ {0,1},
  name-length bounds, parent-reference checks on load.
  → M-050, N-058.

## SaveScan/ifileproperties-part3--btrfs-zfs-raw-generic.md (first read any session)

- `:82` — "FLAGS — **constant 0x0020 for all 12,147 records** (every carved file flagged
  deleted)". → S-031.
- `:77-90` — CRAWFileProperties: formatted timestamp string (FUN_140c41aa0,
  "%d-%d-%d %d-%d-%d"); `+0x56` u32 analyser checksum (0xFFFFFFFF = invalid marker;
  12,030/12,147 nonzero); second note string; `+0x8C` header anchor A; `+0x94` tail anchor
  B (261/12,147 nonzero); `+0xF4` boolean (2,431/12,147); `+0xF8` type-registry ID (35 for
  the shot*.jpg set). → M-051.
- `:23-25` Btrfs secondary ids A/B (+0x3B/+0x3F) + generation/transid qword (+0x81);
  `:52-65` ZFS secondary flags (+0x5A) + polymorphic ZfsDataRunlist (+0x60, own vtable
  +0x18/+0x20). → N-059.

## SearchDLL/rawsearchdll--family-{brance,bruteforce,filegen,rawspeed,special,xjq}.md (all first reads)

- family-bruteforce roster: CADTS/CADTSRawFileInfo, CAPE, CAsf, CDWG, CDXF, CEPS,
  CFCPEvt, CFH11, CGif, CM2TS, CMTS, JpegAnalyzer, MP3Analyzer, MovAnalyzer, PdfAnalyzer,
  TiffAnalyzer (+FileInfo pairs). → S-007 confirmation (formats already filed).
- family rosters cross-checked against the census: the 6 family files' per-family class
  counts CONFLICT with the census §4 counts in five places (12 vs 22, 23 vs 24, 47 vs 46,
  15 vs 20, 5 vs 8) → CORPUS-ERRATA rows this session.
- raw-dual-registry--u16-magic-keys.md `:37-40`: 0xFFF1 AAC ADTS; 0xFFF2..0xFFFF MPEG
  audio (12 syncs); 0xFFE2..0xFFE7 (6 codes); 0xF9FF — the naked-MPEG binding family.
  → S-007 (already filed; re-confirmed).

## SearchDLL/analyser-granular-batch1--pst-rar-mkv-heic.md (deeper read)

- `:36-41` — RAR4 walk FUN_180012d00, 272-byte filename buffer, "multi-part archive
  stitching + entry-table reconstruction for the results tree". → S-032.
- `:43-66` — MKV EBML vint decoder; DocType discrimination constants 0x6D626577
  ("webm")/0x67805318; bounds `n + value + 4 > 0x200: reject`. → N-051.
- `:67-80` — HEICAnalyser brand relabel: `heic` → L"HEIC"; HEIF-family → L"HEIF"
  ("rewrites the brand slot to L'HEIF' for the display name"); Type-ID 0x7C. → N-052.

## SearchDLL/rawsearchdll--zip-analyser-carving.md (deeper read)

- `:13-21` — all three ZIP anchors accepted (LFH / CD entry preferred / EOCD).
- `:29-38` — per-entry parser FUN_180045690; "Entry names accumulate into a list
  (local_68/local_a8 vectors, FUN_180007060 = merge) — **the analyser returns the complete
  inner file inventory, which the UI later presents as the archive's contents**". → S-032.

## config-files/config-data-reference.md (first full read)

- `:20` — "### Dict (2,742 B, 228 entries — byte-verified) — password brute-force
  dictionary". `:86` — "Dict ties to BitLockerLib (manage-bde -Password) + EuOfficeRepairFile
  (encrypted HWPX)". → N-064.
- EUUserData documented (`:95`) — 111-extension user signature store. → M-048 (the file
  itself parsed: {"123": {"signs": ["0,00001A00051004"], "mime": "application/vnd.lotus-1-2-3"}}).

## MOM-modules/{Error,Thread,Common,Config,CheckVersion,Log,bitlockerhelp,FileSystemCheck}.md (all first reads)

- Error.md: error-OBJECT census (CErrorManager/CErrorTrackManager + IError* interface
  family + per-module registrars). → M-049.
- Thread.md: MFC-style sync wrapper kit (CThread/CMutex/CEvent/CCriticalSection/CTimer) —
  NOT a pool; host-side CommonThreadPoolMgr + 45 named workers + CStopScanThread + 5 s
  progress watchdog are in DRWUI-thread-architecture (already audited). Core's ThreadPool
  is a documented superset. → no new finding (register).
- Log.md: EPMLOG.log file-mapping sink + CMemMonitorManager. → N-061, N-062.
- All 8 are structural censuses (RTTI/vftable/string histograms) with no behavioral
  tables → CORPUS-ERRATA meta-row.

## UIUX/{31,13,19,06,24,25,20,33,34}.md (all first reads)

- 31-filterbar: 7-dim AdvanceFilterCondition; 3-slot persistence; CSetFilterThread;
  re-assert IDs 0x1a/0x105/0x1006; CCheckNodeThread re-grade at TimeCritical. → M-029
  addendum.
- 19-diskhealth: Threshold column + Good/Caution verdicts + firstCautionIndex. → N-055.
- 06-dialog: live "%1 file chunks scanned" milestones; stop-dialog both-branches cancel
  semantics. → N-043 addendum.
- 13/25/20: the wiring graph — star bus (doDispatchMessage → 15 subscribers) + per-node
  sigInsertNode/SelectItem/RemoveItem into live Qt models; core's typed POD event stream is
  contract-level MATCH on threading/abort, architectural divergence on live per-node push
  (register with M-029's doc family; the engine contract — stage-boundary counters — is
  documented deliberate).
- 33/34: no new constraints (path/Type trees one class; breadcrumb needs paths — core
  MATCH).

## WinPE/{eusendmessage,boot-config}.md (both first reads)

- eusendmessage: WM_COPYDATA{dwData=999} → "DRW_Installer" window — installer-only.
  → I-021.
- boot-config: wpeinit→ChangeRes→auto.bat→DRW.exe chain; PE driver set ebrntdrv/epmdkdrv
  demand-load; EUDCPEPM boot-time disk-class filter — confirms M-030's environment-gate
  evidence. → I-021 + M-030 cross-ref.

## og PE manifests (direct binary parse this session — new corpus evidence)

- DRW.exe: requireAdministrator; DRWUI.exe: requireAdministrator; DRWService.exe:
  asInvoker. → I-021.

## Raw naming (OQ-S4-2 resolution — decompile-cited)

- RAWSearchDLL_decompiled.c:33312/33408/33447 — MP4/MOV naming template "Lost Name File
  (<mvhd metadata>)_EaseUS"; :33531 creation-time "%04d:%02d:%02d %02d:%02d:%02d";
  :35668 CZeroEndRawFileInfo content-embedded names; :9055-9148 the vtable+0x48 hook =
  one-shot "filter_generalraw" config fetch. → N-063 + M-029 addendum.
