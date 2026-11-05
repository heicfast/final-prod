# UI-REPO-STATE — the fresh UI repository (recovery-wizard-ui-new, v3 @ dd2782c, v0.3.4)

Read-only inventory by Agent F (T2-f). Every claim cites the fresh repo at
`/home/z/my-project/recovery-wizard-ui-new` (branch `v3`, commit `dd2782c`,
confirmed via `git log`) unless another repo is named. This is the state that
will replace `final-prod/ui/` and onto which the real engine bridge (core ABI v4)
will be built. Companion sheets: Agent B (old final-prod bridge, ABI v2), Agent D
(core_recovery contract, ABI v4).

---

## 1. The connector contract (THE SEAM — exhaustive)

Single source of truth: `src/connector/contract.ts` (663 lines). Human companion:
`docs/CONNECTOR-CONTRACT.md` (112 lines, synced to post-ADR-008 signatures in
dd2782c). Factory: `src/connector/index.ts`.

### 1.1 Contract principles (contract.ts:1-13, CONNECTOR-CONTRACT.md:8-23)

1. The UI renders and requests; the connector provides and grades (confidence,
   health status, error reasons, destination validation are backend decisions).
2. Errors carry machine codes + i18n reason keys (`reasonKey` suffix under
   `errors.` or `preview.reason.`); the UI never string-matches error text.
3. Events are subscription-shaped: `onScanProgress`/`onRecoveryProgress` return
   unsubscribe functions; consumed via `useSyncExternalStore` external stores in
   `src/queries/scan.ts` / `src/queries/recovery.ts`.
4. Bounded payloads: preview payloads are bounded backend-side (hex first 64 KiB,
   text capped, PDF page bitmaps, AV first-frame thumbnail).

### 1.2 Error model

`ConnectorError extends Error` with `code: ConnectorErrorCode`, optional
`reasonKey?: string`, optional `detail?: Record<string, unknown>`
(contract.ts:49-54). **24 error codes** (contract.ts:56-80):

| Group | Codes |
| --- | --- |
| Device | `device-gone`, `device-permission` |
| Scan | `scan-not-found`, `scan-not-paused`, `scan-not-running`, `scan-scope-invalid` |
| Session | `session-corrupt`, `session-not-found` |
| Destination | `destination-overlap`, `destination-invalid`, `destination-unwritable`, `out-of-space` |
| Preview | `preview-unsupported`, `preview-too-large`, `preview-damaged`, `preview-encrypted`, `preview-timeout`, `preview-missing-data` |
| Recovery/Repair/Health | `recovery-item-failed`, `repair-consent-required`, `repair-failed`, `health-unavailable` |
| Meta | `not-implemented`, `unknown` |

Hard-refusal semantics documented in CONNECTOR-CONTRACT.md:66-70: a
`validateDestination` result of `allowed === false` with reason `overlap` cannot
be overridden by the UI; a corrupt session must fail whole with `session-corrupt`
(never a partial load).

### 1.3 Operation inventory — 36 operations across 9 domains

`RecoveryConnector` (contract.ts:631-641) composes 9 domain objects. Command
count: 34 commands + 2 event subscriptions.

| Domain | Operations (contract.ts lines) | Key request/response types |
| --- | --- | --- |
| **devices** | `listDevices()` :128-137, `getDeviceInfo(deviceId)` (id may be device OR volume id; connector resolves owner), `refreshDevices()` | `Device` (86-100: id, kind `internal\|usb\|sd\|optical\|virtual`, model, label, serial, capacity, status `ok\|permission-denied\|disconnected\|locked`, isSourceOfActiveScan, volumes, canScanWholeDisk); `Volume` (102-118: letter, fs `ntfs\|fat32\|exfat\|ext4\|btrfs\|refs\|hfsplus\|zfs\|raw\|unknown`, lostPartition?, quickScanAvailable, sectorSize?, bitlocker `boolean\|'unlocked'\|'locked'`); `LostPartitionHint` (120-126: startByte, sizeBytes, probableFs, note) |
| **scan** | `startScan(params)` :216, `pauseScan(scanId)`, `resumeScan(scanId)`, `cancelScan(scanId)`, `getScanState(scanId)`, `onScanProgress(scanId, cb) → unsubscribe` :222 | `ScanParams` (167-173: targetId, pathScope? — volume-root-relative or absolute, null = whole volume); `ScanProgressEvent` (175-196: status, stage, stageProgress 0..1 per-stage, **overallProgress monotonic 0..1**, bytesScanned/Total, filesFound, dataFoundBytes, errors, badSectorMap `BadSectorRange[]`, etaSeconds, throughputBytesPerSecond, recoverableDuringScan, deviceConnected, currentPath — the live "Searching: …" ticker); `ScanState` (203-213: params, target, status, startedAt, completedAt, progress, failure?{code,reasonKey}) |
| **results** | `getResultTree(scanId, parentId, mode)` :308-313, `queryResults(scanId, filter, sort, page{offset,limit})` :314-319, `getNodeDetail(scanId, nodeId)`, `countResults(scanId, filter)` | `ResultNode` (233-249: kind `file\|directory\|group`, origin `existing\|deleted\|raw\|lost-path\|unsaved`, category, confidence, thumbnailAvailable, parentId, attributes?); `ResultTreeMode` `'path'\|'type'\|'raw'`; `TreeNode` (253-263: icon `directory\|category\|group\|device`, childCount, fileCount, sizeBytes, special `deleted\|lost-path\|raw-found\|unsaved\|existing`); `TreePage` (265-268: nodes, parentHasMore); `ResultsFilter` (273-286: nodeId, treeMode, search, categories, dateFrom/To, sizeMin/MaxBytes, extensions, origins, hideEmptyFiles); `ResultsPage` (288-296: nodes, total, **totalBytes**, offset, pageSize, elapsedMs — perf grid #36); `NodeDetail` (298-306: fullPhysicalPath, createdAt, accessedAt, confidenceReason, badSectorsOverlap, clusterCount); `SortField` name/modified/type/size/confidence |
| **preview** | `requestPreview(scanId, nodeId)` :371-377 (connector infers kind from its own file knowledge — UI never classifies extensions), `getThumbnail(scanId, nodeId) → {dataUrl}\|null` | `PreviewPayload` union (328-369): `image`{dataUrl,width,height,bounded}, `hex`{bytes Uint8Array,totalSize,baseOffset}, `text`{text,encoding,totalSize,truncated}, `pdf`{pages[{dataUrl,width,height}],pageCount}, `av`{metadata,thumbnailDataUrl,durationSeconds}, `office`{metadata,extractedText,truncated} |
| **sessions** | `saveSession(scanId, path)` :393-398, `listSessions()`, `loadSession(path) → {scanId}`, `getSessionMeta(path)` | `SessionMeta` (383-391: id, fileName (.rsf-style), savedAt, targetLabel, filesFound, dataFoundBytes, completed) |
| **recovery** | `startRecovery(scanId, selection, destination, options) → {jobId}` :474-480, `cancelRecovery(jobId)`, `getRecoveryState(jobId)`, `getRecoveryItems(jobId, page)`, `onRecoveryProgress(jobId, cb) → unsubscribe`, `getRecoveryReport(jobId)` | `RecoverySelection` (470-472: `{kind:'nodes', nodeIds}` \| `{kind:'all', filter: ResultsFilter}` — the reference's Recover All); `RecoveryOptions` (406-412: collision `rename\|overwrite\|skip`, rebuildFolders, keepEmptyFolders, fileRepair, verifyOnWrite); `RecoveryProgressEvent` (435-448: overallProgress, currentFile, currentFileProgress, completed/failed/skipped/totalCount, throughput, verifying, etaSeconds); `RecoveryReport` (450-466: succeeded/failed/skipped item lists **capped at 500** with `processedTotal` exact + `truncatedItemLists` flag); `RecoveryItem` (426-433: state `pending\|running\|done\|failed\|skipped\|verifying`, error?, savedAs?) |
| **health** | `getHealth(deviceId)` :520-522 | `DiskHealth` (507-518: status `good\|caution\|bad`, temperatureC, powerOnHours, diskClass `ssd\|hdd\|unknown`, wearPercent, trimEnabled, reallocatedSectors, pendingSectors, attributes `SmartAttribute[]` — id/name/current/worst/threshold/raw/status) |
| **repair** | `dryRunRepair(deviceId)` :559-560, `applyRepair(deviceId, {consentToken}) → {applied}`, `getUndoLog(deviceId)`, `undoRepairs(deviceId) → {rolledBack}` | `RepairFinding` (536-543: kind `boot-sector-mismatch\|backup-dbr-stale\|fat-table-damaged\|mft-record-error\|sectors-per-cluster-corrupt\|checksum-mismatch`, where, descriptionKey, fixable, severity); `RepairDryRun`{findings, canApply}; `RepairUndoEntry` (551-557: index, action, where, at, snapshotBytes) |
| **system** | `getDriverStatus()` :604-605, `getAppInfo()`, `validateDestination(path, sourceDeviceId, neededBytes)`, `pickDirectory(hint?)`, `listMountPoints()`, `openPath(path)` | `DriverStatus`{installed,version,running,note?}; `AppInfo` (580-586: name, version, **connectorMode `'mock'\|'tauri'`**, **engineVersion: string\|null**, platform); `DestinationValidation` (596-602: allowed, reason `overlap\|unwritable\|invalid\|out-of-space`, reasonKey, freeBytes, neededBytes); `pickDirectory` hint (619-622: `{volumeId?}` roots the dialog at a volume for scan-scope picking; `{excludeDeviceId?}` steers destination picking away from the recovery source); `MountPoint`{id,label,path,freeBytes,totalBytes} |

Plus a mock-only dev surface: `ScenarioControl` (644-649: setScenario,
getScenario, setSeed, reset) over `MockScenarioId` (651-663, 11 scenarios) —
"real connector throws" per the doc comment.

### 1.4 State models the contract encodes

- `ScanStatus` (143-153): `idle | starting | running | paused | pausing |
  resuming | stopping | completed | cancelled | failed` (10 states).
- `ScanStage` (155-156): `volume-analysis | quick-index | deep-walk | carve |
  tree-build | done`, with `scanPhaseOf()` (161-165) mapping stages to the
  reference-visible phases `quick | advanced | organizing`.
- `RecoveryJobStatus` (423-424): `validating | running | completed |
  completed-partial | cancelled | failed`.
- `RecoveryItem.state`: `pending | running | done | failed | skipped |
  verifying`.
- Cancellation/progress semantics: **no AbortSignal, no cancel handles** —
  cancellation is an explicit domain command (`cancelScan`, `cancelRecovery`);
  progress subscriptions return plain unsubscribe closures. Pausing has its own
  commands with `scan-not-running`/`scan-not-paused` guards.

### 1.5 Capability flags the UI consumes

`Device.canScanWholeDisk` (deep search on raw device), `Volume.quickScanAvailable`
(false ⇒ UI offers deep only; mock collapses the quick pass), `Volume.lostPartition`
(candidate lost-partition entry point), `Volume.bitlocker`, `Volume.sectorSize`,
`ScanProgressEvent.recoverableDuringScan` (gates the mid-scan Recover All CTA) and
`.deviceConnected` (reconnect prompt), `Device.isSourceOfActiveScan`,
`AppInfo.connectorMode`/`engineVersion` (About dialog truthfulness).

### 1.6 ADR-008 and the post-ADR-008 contract shape

`docs/adr/008-lost-data-flow.md` (Accepted, session 3 T6; supersedes the
session-1/2 ScanConfigPage design). Decision summary:

1. **No scan-configuration step** — ScanConfigPage and its route deleted; every
   scan entry point starts directly and navigates to the live scan surface.
2. **One automatic chain**: `ScanParams` reduced to `{targetId, pathScope?}`;
   connector always runs volume-analysis → quick-index → deep-walk → carve →
   tree-build; quick pass collapses to a probe when `quickScanAvailable: false`.
3. Reference status formats ("Quick/Advanced scanning X%", "Files found: N
   (size)", "Remaining time", "Reading sector: X/Y", "Searching: <path>").
4. Affordances kept: pause, stop (confirm dialog with stat tiles),
   browse-while-scanning, Recover-All mid-scan, completion celebration.
5. Contract consequences (ADR-008 §Decision 5): scan mode / categories /
   sectorSize / bad-sector policy / retries **left the contract**; preview-kind
   inference moved behind the connector; recover-all became `RecoverySelection`;
   `SessionMeta` dropped `mode`; `deleteSession` removed (not
   reference-evidenced); `ResultsFilter` gained `hideEmptyFiles` (corpus toggle)
   and lost `confidence` (not a reference filter surface).

Other semantic notes pinned in CONNECTOR-CONTRACT.md:72-87: scan modes map to the
staged model (quick = FS index; deep = full structural walk; deep-carve adds the
signature scanner); `.rsf`-style sessions; path-mode tree partitions into three
disjoint top-level branches (`root`, `lost-paths`, `raw-all`); zero-byte files
always graded `confidence: 'poor'` (per `CRecoverability::GetEntryRecoverability`,
size < 1 → Poor); repair is transactional with `consentToken` = the user's typed
`REPAIR` acknowledgment.

### 1.7 Swapping procedure (CONNECTOR-CONTRACT.md:89-99)

1. Implement `RecoveryConnector` in `src/connector/tauri/tauri-connector.ts` by
   delegating to `invoke()` on thin Rust commands in `src-tauri/src/commands/`
   (the 3 reveal/path/env helpers already exist there).
2. Map engine progress callbacks to the two event subscriptions (Tauri
   `emit`/`listen`).
3. Set `VITE_CONNECTOR=tauri` at build time — factory `src/connector/index.ts`
   does the switch; **no component or view changes**.
4. The current stub throws `not-implemented` on every method with a pointer to
   the doc — a misconfigured build fails loudly, never silently.

---

## 2. The mock connector

Files: `src/connector/mock/mock-connector.ts` (1603 lines),
`fixtures.ts` (569), `node-space.ts` (946), `rng.ts` (27). Class `MockConnector`
implements all 9 domains + the scenario API. Factory wiring:
`src/connector/index.ts:27-31` — `VITE_CONNECTOR=mock` (the default; any value
other than exactly `'tauri'` falls back to mock, index.ts:17-20) instantiates it
and exposes `scenarioControl()`.

### 2.1 Scenario set (11, contract.ts:651-663; fixtures.ts:4-11)

`standard`, `large-ntfs`, `sd-card`, `btrfs-disk`, `lost-partition`,
`permission-denied`, `disconnect-mid-scan`, `bad-sectors`, `failing-disk`,
`out-of-space`, `corrupt-session`. Seed default 42 (mock-connector.ts:222);
`setSeed`/`setScenario` rebuild the world; `reset()` clears scans/jobs/sessions
and restores seed 42 + standard (256-272).

### 2.2 Datasets (fixtures.ts)

| Scenario | Devices (fixtures.ts lines) | Primary node space |
| --- | --- | --- |
| standard | `dev-sys` Samsung 970 EVO 500GB, 149.45 GB NTFS C: (`bitlocker:'unlocked'`, :285-318); `dev-data` WD Blue 2TB labeled "Temporary Storage (D:)" 150 GB NTFS (`bitlocker:'locked'`, :509-541); `dev-usb` SanDisk Ultra 64GB 60 GB exFAT F: (:542-565) | sys 180,000 files / 1,600 dirs; data 340,000 / 2,400; usb 21,000 / 260 |
| large-ntfs | `dev-large` Samsung 970 EVO 2TB, 2 TB NTFS D: (:477-503) + sys volume | **1,200,000 files / 5,400 dirs** (the 1.2 M-node volume) + sys 210,000 / 1,800 |
| sd-card | `dev-sdcard` SDXC 64 GB exFAT E: (:389-414) | 24,000 files / 120 dirs, photo-heavy (weighted extension pool) |
| btrfs-disk | `dev-btrfs` WD Red Pro 4TB, 4 TB Btrfs, no letter (:416-441) | 480,000 files / 2,600 dirs |
| lost-partition | `dev-lost` Seagate Barracuda 1TB with `vol-lost-main` 600 GB NTFS D: carrying `LostPartitionHint {startByte: 660 GB, sizeBytes: 330 GB, probableFs: ntfs, note: 'Unallocated area with a valid NTFS boot sector'}` (:443-476) | 96,000 files / 900 dirs |
| every scenario | + virtual "Specify a location" targets appended (fixturesFor :382-385): `dev-recycle-bin` (4 GB, 900 files) and `dev-desktop` (2 GB, 420 files), `canScanWholeDisk: false`, no letter (:329-380) | — |

Content realism: weighted extension pool `EXT_WEIGHTS` (fixtures.ts:100-136,
jpg 18, png 12, txt 8, pdf 8 …), `NAME_STEMS` (54 stems, :138-193), `DIR_NAMES`
(48 names incl. year/month folders, :195-243), `CATEGORY_BY_EXT` registry
(:26-97).

### 2.3 The procedural index (ADR-004, node-space.ts)

Directory skeleton materialized as real nodes + typed-array records for files
(names/attributes derived deterministically from record index); subtree
aggregates precomputed once so tree expansion is O(children); filter queries
scan typed arrays directly (~10-40 ms for 1.2 M records — grid #36 budget;
node-space.ts:1-12). Origin distribution per record (node-space.ts:41-48):
1 existing : 3 deleted : 1 raw : 1 lost-path. Confidence distribution (:49-57):
excellent 1, good 3, fair 1, poor 1, unknown 1. Scope resolution normalizes
drive-letter prefixes and separators (:97-110); `NodeScope` = directory-index
`Uint8Array` mask + precomputed subtree aggregates (:82-95).

### 2.4 Timings (asserted by tests/connector-mock.test.ts)

- `latency(meanMs, jitterMs)` helper with 20 ms floor (mock-connector.ts:1599-1602):
  listDevices 180±60, getDeviceInfo 90±40, refreshDevices 700±200, startScan
  420±120, queryResults 90±50, requestPreview 240±160, getHealth 280±120,
  dryRunRepair 600±200, applyRepair 900±300 …
- Scan tick `setInterval` 55-105 ms (55 + (scanId.length % 3) * 25,
  mock-connector.ts:412); **onScanProgress subscription polls at 150 ms**
  (:453-460); recovery job tick 120 ms (:1017), onRecoveryProgress polls 150 ms
  (:1052-1062).
- Test-mode speedups: `import.meta.env.MODE === 'test'` raises stage speed to
  0.22/tick (vs 0.014 + every-5th-tick 0.006) and caps recover-all targetTicks
  at 25 (vs 200) (:494-495, :984).
- Budgets pinned by tests (docs/TESTING.md:26-29): pause/resume < 600 ms, cancel
  ≤ 300 ms, filter query < 200 ms on standard.

### 2.5 Error injection (DEV panel)

- `permission-denied`: volume C: gets `status: 'permission-denied'`
  (rebuildDevices :283-286); startScan throws `device-permission`
  (:356-358).
- `bad-sectors`: all volumes `quickScanAvailable: false` (:287); scan runtime
  seeded with a 14-range bad-sector map (`makeBadSectorMap`, seed ^ 0xbad5e,
  :469-479); error counter +1 every 9 ticks (:513-515); health shows caution.
- `disconnect-mid-scan`: **one-shot** injection at overall > 0.38 — pauses the
  scan and clears the timer (:499-510); `deviceConnected: false` in progress
  (:604); resume models the reconnect (:429-435).
- `out-of-space`: `validateDestination` refuses with reason `out-of-space` when
  neededBytes > mount free (:1517-1525).
- `corrupt-session`: `loadSession` throws `session-corrupt` for `auto_*` session
  files (:916-918).
- `failing-disk`: health status `bad` with heavy reallocated/pending counts
  (:1239-1247).

### 2.6 FAKED behaviors — the rewiring must NOT carry these over

This is the fake-inventory (mock-connector.ts unless noted):

1. **Confidence grading** — procedural roll by record index (node-space.ts:49-57),
   not engine recoverability analysis.
2. **Image previews** — `syntheticImage()` generates a deterministic SVG
   (gradient + 7 circles + filename banner) as a data URL (:829-860); damaged
   injected at `idx % 97 === 0`, encrypted at `.crypt` extension (:721-724).
3. **PDF previews** — `syntheticPage()` fake A4 page SVGs, 1-3 pages
   (:862-884).
4. **Hex previews** — random bytes from `mulberry32(idx ^ 0xbeef)`, capped 64 KiB
   (:728-736); too-large threshold at 64 GiB.
5. **Text previews** — 220-line word-salad from a 10-word pool; UTF-16LE every
   7th file; truncated flag at > 16,000 bytes (:737-764).
6. **AV metadata** — hardcoded H.264/AAC-LC/30 fps, dims 1280-2560 (:772-789).
7. **Office extractedText** — canned sentence pool (:790-812).
8. **Thumbnails** — same synthetic SVG generator (:815-826).
9. **NodeDetail** — createdAt = modifiedAt; accessedAt = modifiedAt + 24 h;
   confidenceReason from a fixed 5-string map; `badSectorsOverlap = confidence ===
   'poor'`; clusterCount = ceil(size/4096) (:674-697).
10. **Health/SMART** — procedural from `seed ^ serial.length ^ 0xea11`; SSD
    detection by model-string sniffing (`'ssd'`/`'evo'`/usb); caution pinned to
    `dev-data` or bad-sectors scenario (:1228-1356).
11. **Repair findings** — hardcoded 3 findings (boot-sector-mismatch, backup-dbr-
    stale, fat-table-damaged) when scenario is bad-sectors or device is dev-data,
    else none (:1362-1398); applyRepair always appends the same 3 undo entries
    with fake snapshotBytes 512/131072/512 (:1401-1443); consent token must equal
    literal `'REPAIR'` (:1405-1411).
12. **getDriverStatus** — constant `{installed: true, version: 'mock-1.0.0',
    running: true}` (:1461-1468).
13. **getAppInfo** — `connectorMode: 'mock'`, `engineVersion: null`, platform
    from `navigator.platform` (:1470-1479).
14. **validateDestination** — overlap = destination drive letter ∈ source device
    letters; mount existence by path-prefix letter match; free space from fixture
    capacities (:1481-1527).
15. **pickDirectory** — cycles canned `Recovered Files` suggestions across
    mount points (`pickIndex`), or `suggestedScopePath()` from the node space for
    volumeId hints (:1529-1562). No native dialog.
16. **Sessions** — purely in-memory `Map`s (`sessionStore`,
    `sessionPathToScan` :226-227); completion auto-saves an `auto_*.rsf` meta
    (:570-583); **nothing touches disk**; save/list/load are lookups.
17. **Recovery outcomes** — per-item rolls: fail < 0.04, skip > 0.92 (only with
    collision skip), rename suffix " (1)" > 0.88 (:1182-1201); throughput
    `randBell(48e6, 30e6)` (:1213); batch scaling so any job finishes in ~200
    ticks (:983-985) — the real engine streams at disk speed.
18. **Scan counters** — filesFound/dataFound follow a saturating curve
    `1 - exp(-3.4 * overall)`, exact at completion (:549-559); bytes linear per
    tick with 96-156 totalTicks (:491, 517-521); ETA from remaining ticks.
19. **Live-results visibility** — during a scan, tree/page/count totals are
    trimmed by `visibleFraction` (:481-485, 635-671) rather than reflecting real
    incremental engine findings.
20. **Mount points** — derived from fixture volume letters (:1583-1594).
21. **openPath** — throws `not-implemented` honestly ("opening paths requires
    the desktop shell", reasonKey `errors.openPathMock`, :1569-1580).
22. **Stage progression** — fixed `STAGE_WEIGHTS` (volume-analysis 0.04,
    quick-index 0.14, deep-walk 0.42, carve 0.24, tree-build 0.16, :135-141) with
    weight renormalization when quick-index is omitted (:523-530).

### 2.7 Dev panel

`src/features/dev-panel/dev-panel.tsx`. Mounted only in dev builds — the shell
renders `<DevSurfaces/>` (DevPanel + PerfOverlay) behind
`import.meta.env.DEV` (src/app/shell.tsx:353). The panel returns null when
`useScenarioControl()` is null (real connector) or `ui.devPanelOpen` is false
(dev-panel.tsx:43-44). Controls: scenario `<select>` (all 11; changing it calls
`control.setScenario` + `qc.invalidateQueries()` — the whole mock world reshapes,
:74-91), seed input + Apply, language EN/中文, density toggle, perf overlay
toggle, Reset (control.reset()). `PerfOverlay` = rAF FPS meter +
PerformanceObserver longtask counter (:159-206). Scenario control hook:
`src/queries/dev.ts:19-25` (returns `scenarioControl()` from the factory).

---

## 3. The tauri connector stub

`src/connector/tauri/tauri-connector.ts` (35 lines) + `not-implemented.ts` (9
lines).

- `createTauriConnector()` builds each of the 9 domains via a `Proxy` whose
  `get` returns a function that **throws** `connNotImplemented('domain.prop',
  args.length)` for ANY property access (tauri-connector.ts:14-22). There is no
  partial implementation — it is all-or-nothing per the no-silent-fallback SOP
  ("a misconfigured build fails loudly and truthfully instead of pretending to
  work", header comment :1-8).
- `connNotImplemented()` produces an Error with `code: 'not-implemented'` and a
  message pointing at `docs/CONNECTOR-CONTRACT.md` and the mock default
  (not-implemented.ts:1-9).
- Verified by `tests/connector-mock.test.ts:563` — "throws NotImplementedError
  on every domain".
- The no-silent-fallback rule also surfaces in the mock itself: `openPath`
  throws rather than pretending (mock-connector.ts:1569-1580).

**What must be built there** (the complete invoke/event surface):

1. 34 `invoke()` command bindings covering all domains above (names and payload
   shapes per §1.3), plus error normalization: engine/bridge errors →
   `ConnectorError {code, reasonKey}` (the old final-prod bridge did this in
   `ui/src-tauri/src/bridge/error.rs` — BridgeError serialized as
   `{code, bridgeCode, message}`, engine numeric codes mapped through a table).
2. Event bridge: Tauri `listen()` (or the equivalent `emit` from Rust) wired to
   `onScanProgress(scanId, cb)` / `onRecoveryProgress(jobId, cb)` returning
   unsubscribe closures. The consumer side already exists: module-level external
   stores in `src/queries/scan.ts:21-83` (status-change + 10%-bucket
   invalidations) and `src/queries/recovery.ts:66-95`.
3. `getAppInfo()` must return `connectorMode: 'tauri'` and a real
   `engineVersion` (the About dialog renders these truthfully;
   `src/features/settings-about/about-dialog.tsx`).
4. Shell conveniences already available on the Rust side (see §5):
   `reveal_in_explorer`, `path_exists`, `app_environment`; plugins
   `tauri-plugin-dialog` (native folder picker for `pickDirectory`) and
   `tauri-plugin-opener`/explorer (for `openPath`).

---

## 4. UI structure & screens

### 4.1 src/ layout (117 files, verified by count)

```
src/app/          providers.tsx, router.tsx, shell.tsx, theme.tsx, error-boundary.tsx
src/assets/interim/chrome/   32 window-chrome SVGs (light+dark, nor/hov/pre/dis) + manifest.json
src/components/ui/  26 primitives: alert, badge, brand-icons, button, checkbox,
                  confidence-badge, dialog, empty-state (StatTile/CapacityBar),
                  file-icon, input, label, menus (dropdown/context/popover),
                  progress (linear + circular gauge), radio-group, results-tree,
                  skeleton, stepper (Segmented), switch, table, toaster, tooltip,
                  virtual-file-list, window-controls
src/connector/    contract.ts, index.ts, mock/ (4), tauri/ (2)
src/features/     12 folders: backup, device-selection, dev-panel, health,
                  lost-partition, preview, recovery-queue, repair, results-explorer,
                  scan-progress, settings-about  (+ sd-card page inside device-selection)
src/hooks/        use-physical-devices.ts
src/i18n/         index.ts, en.ts, zh-CN.ts
src/lib/          format.ts (Intl), hex-format.ts, logger.ts (structured), runtime.ts, utils.ts (cn)
src/queries/      11 files: client, devices, scan, results, preview, sessions,
                  recovery, health, repair, system, dev
src/stores/       scan-session-store, selection-store, filter-store (+sort), ui-store
src/styles/       tokens.css, index.css
src/theme/adopted/  themes.ts (THEME_IDS), tokens.css
src/workers/      hex.worker.ts + hex-worker-client.ts (off-main-thread hex preview,
                  jsdom fallback; ADR-003 preview rendering boundary)
```

### 4.2 Routing & navigation

React Router **v7 data mode with `createHashRouter`** (ADR-001; hash history
behaves identically in WebView2 custom protocol, browser dev, and static hosts —
src/app/router.tsx:17-21). Routes (router.tsx:22-45): `/` (Home), `/sd-card`,
`/tools` (legacy alias → HealthPage), `/backup`, `/scan/:deviceId/live`
(ScanProgressPage), `/results/:scanId` (ResultsRoute), `/recover/:scanId`,
`/health`, `/repair`, `/settings`, `/lost-partition`, `*` → ResultsPageFallback.
Guards: `/scan/:id/live` and `/results/:scanId` require a live/loaded session
via `getScanState`/`getSessionMeta`, else redirect `/` (docs/UI-MAP.md:45-48).

Shell (`src/app/shell.tsx`): custom frameless title bar (brand briefcase logo,
orange Buy-Now pill → ActivationDialog, key icon, standalone language dropdown,
**three-lines Menu** → Settings / Theme toggle / About, `WindowControls`,
`data-tauri-drag-region`, double-click maximize via `getCurrentWindow()` guarded
by `isTauriRuntime()`); 3-group sidebar (Data Recovery: Hardware Disk, SD Card,
Disk Health; Advanced Features: File Repair, Lost Partition Rescue; File Backup:
Scan Snapshot) with collapse (52 px rail) and an active-scan row during the
explorer flow; `#EBF2FF` ice-blue frame with white rounded-tl-2xl content card.
Dev surfaces (DevPanel + PerfOverlay) mount only under `import.meta.env.DEV`
(shell.tsx:353).

### 4.3 State management

- **Zustand 5** (`zustand@5.0.16`), 5 stores:
  - `ui-store.ts` — results viewMode details/icons, previewPaneWidth (240-560,
    default 320), sidebarCollapsed (persisted `rw.ui.sidebarCollapsed`),
    devPanelOpen, perfOverlayVisible.
  - `scan-session-store.ts` — active scan {scanId, targetId}, known scans map,
    celebrationSeenFor (celebration → results handoff).
  - `filter-store.ts` — full results filter UI state (treeMode, treeNodeId,
    search + debounced searchApplied, categories, date/size presets, extension
    input, existingOnly, hideEmptyFiles) + `useSortStore` (field/direction) +
    `resolveFilter()` mapping UI state → contract `ResultsFilter`
    (filter-store.ts:199-216).
  - `selection-store.ts` — two-mode selection ('ids' explicit set | 'all' =
    filter snapshot + count − exclusions; corpus tri-state CSelAllCheckBox).
- **TanStack Query 5** for all server state; single client with query keys in
  `src/queries/client.ts:18-42`; results paging via `useInfiniteQuery` at
  RESULTS_PAGE_SIZE = 200 (`src/queries/results.ts:11,34-55`).
- **Live streams** via `useSyncExternalStore` over connector subscriptions
  (`src/queries/scan.ts:115-128`, `recovery.ts:86-93`).

### 4.4 Screens (docs/screen-manifest.md — 10 surfaces + 11 dialogs)

S1 Home (`/`) — drive grid, banners (permission/disconnected), quick targets
(Recycle Bin / Desktop / Choose a folder), recent sessions, sticky CTA. S2 Lost
Partition Rescue (`/lost-partition`) — 2-step: select-disk → smart-partition
scan completed. S3 Live Scan (`/scan/:deviceId/live`) — the 3-pane results
surface live (browse-while-scanning), "Searching: <path>" ticker, status cluster
(gauge, "Quick/Advanced scanning X%", "Files found", "Remaining time", "Reading
sector"), pause/resume/stop + 3 modals (stop-confirm, disconnected
non-dismissible, celebration 520 px). S4 Results Explorer (`/results/:scanId`) —
toolbar, filter bar, 3-pane (tree path⇄type with `root`/`lost-paths`/`raw-all`
branches, virtualized 1M+-row list, preview pane 240-560 px), status bar with
contextual Recover N/All. S5 Recovery (`/recover/:scanId`) — destination
validation (overlap hard refusal), options (collision radio; fileRepair switch;
rebuildFolders/keepEmptyFolders/verifyOnWrite ride with defaults), queue, report
with JSON export. S6 Disk Health (`/health`) — v4 redesign: 3D `Drive3dIcon`
cards with Windows badge + state-dependent BitLocker open/closed lock, hero
health gauge, KPI tiles, SMART 7-column table. S7 File Repair (`/repair`) —
2-step dropzone → queue; `VolumeRepairWorkbench` behind Advanced Repair
(dry-run findings, two-factor consent: checkbox + typed `REPAIR`, undo log). S8
Settings + AboutDialog. S9 SD Card (`/sd-card`). S10 Scan Snapshot (`/backup`) —
session list with Open/resume. Dialog inventory (UI-MAP.md §3): stop-confirm,
celebration, destination, space gate, context menu, preview modal, session
picker, repair consent, about, activation, error. Noted absence: **no
close-during-scan guard** (UI-MAP.md:208-211).

### 4.5 i18n, theming, visuals

- i18n: i18next + react-i18next, `en` + `zh-CN`, **406 keys each (verified by
  script count), 1:1 parity** enforced by `tests/i18n-parity.test.ts`; unused/
  missing keys must be 0 (`scripts/i18n-sweep.mjs` + `tests/i18n-sweep.test.ts`,
  `i18n-sweep-extraction.test.ts`). Locale detection + persistence
  (`rw.ui.locale`), `document.documentElement.lang` sync
  (src/i18n/index.ts:9-43).
- Theming (ADR-007): adopted two-layer token system —
  `data-theme="<palette>"` + `data-scheme="light|dark"`; `THEME_IDS = [light,
  dark, ember, tide, blossom]` (src/theme/adopted/themes.ts:33) + app-level
  `'system'` and derived `'high-contrast'` (`.hc` class) + density
  comfortable/compact; persisted `rw.ui.theme` / `rw.ui.density`
  (src/app/theme.tsx:27-65).
- Visual components: `Drive3dIcon` (brand-icons.tsx:294) with state-dependent
  BitLocker lock, `RecycleBin3dIcon` (:397), `Desktop3dIcon` (:423),
  `SelectFolder3dIcon` (:450), `SaveDestinationIcon` (:473),
  `BrandBriefcaseLogo` (:9), sidebar icon set (`SidebarHardwareDiskIcon` …
  `SidebarChronoSnapIcon`).
- `public/` **does not exist** in the fresh repo — the 11 unused
  reference-product screenshots and AI-Studio scaffold (`.env.example`,
  `metadata.json`) were removed in the v0.3.4 clean-room pass
  (docs/ROADMAP.md:9-12; commit dd2782c).

---

## 5. src-tauri in the fresh repo (27 files, excluding target/)

- `tauri.conf.json`: productName "Recovery Wizard", **version 0.3.4**,
  identifier `com.heicfast.recoverywizard`, `beforeDevCommand: pnpm dev`,
  devUrl `http://localhost:1420`, frontendDist `../dist`; window 1240×840 (min
  800×600), centered, **resizable + maximized + decorations: false** (custom
  chrome); CSP null; bundle NSIS-only, icons, NSIS languages English +
  SimpChinese with displayLanguageSelector, webviewInstallMode
  downloadBootstrapper (:1-54).
- `Cargo.toml`: package `recovery-wizard-ui` **version 0.3.4**, lib
  `recovery_wizard_ui_lib` (staticlib/cdylib/rlib); deps: `tauri 2`,
  `tauri-plugin-dialog 2`, `tauri-plugin-opener 2`, `serde` — **no engine crate,
  no ABI bindings**; release profile opt-level "s", lto, codegen-units 1, panic
  abort, strip (:1-25).
- `src/main.rs` (6 lines) + `src/lib.rs` (30 lines): the shell is "deliberately
  thin" — plugins dialog + opener, **exactly 3 registered commands**:
  `commands::system::reveal_in_explorer`, `path_exists`, `app_environment`
  (lib.rs:17-21). `commands/system.rs` (58 lines) implements them: Windows
  explorer spawn with `/select,` for files, path existence check, and an
  `AppEnvironment {os, arch, family, webview_version}` struct. No `bridge/`
  directory, no engine linkage, no event emission.
- `capabilities/default.json`: minimal — `core:default`, `dialog:default`,
  `opener:default`, window minimize/toggle-maximize/is-maximized/close/
  start-dragging. Description says "Keep this minimal until the real bridge
  session expands it."
- `build.rs` = standard `tauri_build::build()`; icons set + `nsis-header.bmp` +
  `app-icon.png`.

**Gap vs final-prod's old wired bridge** (all under `final-prod/ui/src-tauri/src/`,
tracked at `git ls-files`; ~4,100 lines of Rust that do NOT exist here):

- `bridge/` (9 modules): `abi.rs` (565 ln — cr_ui C-ABI binding pinned 1:1 to
  `engine/include/cr/cr_ui.h`, **ABI version 2, 78-export surface**),
  `consent.rs` (188 ln — pre-scan repair consent flow S-008), `error.rs`
  (118 ln — BridgeError `{code, bridgeCode, message}` mapping engine numeric
  codes ↔ ConnectorErrorCode), `events.rs` (511 ln — engine-event marshaling,
  worker-thread sink → channel → single emitter → `app_handle.emit`), 
  `filefilter.rs` (1538 ln — GENERATED from the engine's
  `cr_filefilter_data.cpp`, 1481 extensions), `logging.rs` (152 ln —
  %ProgramData% operational log, rotation), `state.rs` (764 ln — scan/job
  registries + snapshot metadata cache, enforced close order), `util.rs`
  (307 ln — two-call helpers, 100ns-1601 → ISO-8601 conversion, base64).
- `commands/` (9 modules, 30 registered commands in old lib.rs):
  `devices.rs` (devices_list, devices_refresh), `scan.rs` (scan_start, scan_pause,
  scan_resume, scan_cancel, scan_state, scan_close), `results.rs` (results_tree,
  results_query, results_count, node_detail), `preview.rs` (preview_open,
  preview_thumbnail), `sessions.rs` (session_save, sessions_list, session_load),
  `recovery.rs` (recovery_start, recovery_cancel, recovery_state, recovery_items,
  recovery_report), `health.rs` (health_read), `repair.rs` (consent_respond,
  repair_dry_run, repair_apply, repair_undo_log, repair_undo), `system.rs`
  (system_driver_status, system_app_info, system_validate_destination,
  system_pick_directory, system_list_mounts, system_open_path).
- Also absent here: `scripts/gen-filefilter.mjs` (filefilter generator +
  freshness gate), `Cargo.lock`, `tests/connector-tauri.test.ts`,
  `tests/fakes/test-connector.ts`, `tests/health-truth.test.tsx`,
  `tests/results-tree-label.test.tsx`, `e2e/consent.spec.ts` (final-prod
  additions on top of the adopted base).
- **Critical**: the old bridge targeted engine **ABI v2**; the current core is
  **ABI v4** (`core_recovery/include/cr/cr_ui.h:81` `#define CR_UI_ABI_VERSION
  4u`). The old `bridge/abi.rs` cannot be copied as-is — it must be re-pinned
  from the v4 header (old final-prod lib.rs even had a startup ABI gate refusing
  to bind on mismatch).

---

## 6. Quality gates & build system

### 6.1 package.json (v0.3.4, packageManager pnpm@12.15.0, engines node >=24)

Scripts (17): `dev`, `build`, `preview`, `typecheck` (tsc --noEmit), `lint`,
`format`, `format:check`, `test` (vitest run), `test:watch`, `test:coverage`,
`e2e` (playwright test), `tauri`, `icons` (tauri icon), `arch:lint`
(scripts/arch-lint.mjs), `knip`, `jscpd` (min-tokens 70, ignoring
src/theme/adopted), `bundle:check` (scripts/bundle-check.mjs), `visual:compare`
(scripts/visual-compare.mjs). `pnpm-workspace.yaml` exists only to pin build
scripts (`allowBuilds`: @ast-grep/cli, unrs-resolver, esbuild, @tauri-apps/cli)
and `minimumReleaseAgeExclude: react-i18next@17.1.4`.

### 6.2 Lint / types / format

- `eslint.config.js`: @eslint/js recommended + **typescript-eslint
  recommendedTypeChecked** (TS files only, projectService) + react-hooks
  recommended + import-x with **13 `no-restricted-paths` zones** (components
  ← features banned; per-feature isolation with two documented compositions),
  `no-console: error` (only `src/lib/logger.ts` exempt), consistent-type-imports
  (inline), no-floating-promises, no-misused-promises (:1-125).
- `tsconfig.json`: `strict: true` + noUnusedLocals, noUnusedParameters,
  noFallthroughCasesInSwitch, noUncheckedSideEffectImports, verbatimModuleSyntax,
  isolatedModules; path alias `@/*`.
- `.prettierrc`: semi, singleQuote, trailingComma all, printWidth 96, LF.

### 6.3 Architecture governance (three overlapping enforcement layers)

`scripts/arch-lint.mjs` rules A1-A6: A1 only `src/queries/**` + `src/connector/**`
import the connector factory/implementations (type-only contract imports legal
everywhere); A2 no `invoke(` outside `src/connector/**`; A3 components never
import features; A4 app never imports features except shell/router wiring; A5
feature isolation except scan-progress → results-explorer and results-explorer →
preview; A6 no `any` in exported connector contract types. ast-grep rules in
`rules/` (`no-raw-invoke.yml`, `no-connector-import-outside-queries.yml`) with
`sgconfig.yml` `ruleDirs: ['rules']`. All three run in CI.

### 6.4 Build & budgets

`vite.config.ts`: React Compiler via `@vitejs/plugin-react` v6 with
`compiler: true` (oxc-transform-react port, ADR-002); port **1420** (3000 when
`DISABLE_HMR=true`), strictPort, `@` alias; build target chrome110, manualChunks
`vendor-i18n` / `vendor-query` / `vendor-react`. **Bundle budget gate**
(scripts/bundle-check.mjs): gzip size — main chunk ≤ **200 KB**, total initial
JS ≤ **500 KB** (env-overridable `BUDGET_MAIN_KB`/`BUDGET_TOTAL_KB`; CI pins the
same values). Coverage (vitest, v8): thresholds **70% lines/functions/statements,
60% branches**, scoped include list (connector, stores, lib, theme, shell, theme,
device-selection, about-dialog, and the 13 experience-critical components —
vite.config.ts:64-84).

### 6.5 Tests (145 tests / 22 files, docs/TESTING.md:22-24)

Unit: format (10), logger (5), node-space (11), stores (12), scan-session-store
(7), ui-store (6), theme-persist (4), theme-snapshot (6, against
`tests/fixtures/adopted-tokens.snapshot.json`). **Contract**:
connector-mock.test.ts (28 — devices determinism, ADR-008 automatic chain +
phase mapping + collapsed-quick, ticker gating, location-target scans, timing
budgets, disconnect injection, paging/filters incl. hide-empty, lost-path
origins, filter latency < 200 ms, preview kind inference, folder scope
(scan-scope-invalid, subtree-only listing, folder-as-root), overlap refusal,
explicit + recover-all jobs, SMART fields, failing-disk, REPAIR consent,
openPath honesty, **tauri-stub throws on every domain**). Component: components
(9), reference-surfaces (8), home-states (3), sidebar (5), window-controls (5),
dialog-choreography (4), press-states (3), route-fade (2), confidence-pill (2),
toaster (1). **Governance**: i18n-parity (4), i18n-sweep (2),
i18n-sweep-extraction (3).

### 6.6 E2E (12 Playwright tests)

`playwright.config.ts`: testDir e2e, workers 1, timeout 120 s, retries 1 on CI,
`vite preview` webServer on port 4173, Desktop Chrome 1280×832, trace
retain-on-failure. `e2e/smoke.spec.ts` (6): golden path scan→results→recover
(with console-error collection), quick-targets direct start, empty-route
fallback, window chrome hidden in browser, sidebar collapse persistence,
dialog choreography. `e2e/responsive.spec.ts` (6 after loop expansion): home
layout invariants at 800×600 / 1024×768 / 1920×1080, DPI 150% (dsf 1.5),
results 3-pane at minimum size, keyboard focus flow.

### 6.7 CI (4 workflows in .github/workflows/ — 304 lines total)

- `ci.yml` (92 ln; push to **v3** + PR; windows-latest; pnpm 12.15.0, Node 24):
  typecheck → lint → format:check → arch:lint → ast-grep (both rules, negation
  globs) → knip (`KNIP_DISABLE_RAW_TRANSFER=1`) → jscpd → test:coverage → build
  → bundle:check → cargo check → **cargo clippy -D warnings** → upload dist.
- `ui-smoke.yml` (78 ln): build (VITE_CONNECTOR=mock) + e2e + artifact uploads;
  second job `tauri-debug-build`: `pnpm tauri build --debug --no-bundle` with
  VITE_CONNECTOR=mock.
- `visual-compare.yml` (54 ln; workflow_dispatch, threshold input default 2%):
  e2e screenshots vs corpus PNG-geometry spec (`docs/visual-spec.md`) via
  scripts/visual-compare.mjs (checks `smoke-step-*.png` dimensions for 7 named
  steps).
- `release.yml` (80 ln; tags `v*`): **tag == tauri.conf.json version gate**,
  NSIS build, sha256 checksum, GitHub Release (explicitly notes artifacts are
  unsigned).

Note: in final-prod these subtree workflows were consolidated to the repo root
and removed from `ui/.github` (final-prod commit 2a99173c) — the re-merge must
not resurrect them under `ui/`.

---

## 7. Documentation inventory (docs/ = 171 files)

- 18 top-level .md: ACCEPTANCE, ARCHITECTURE, AUDIT-REPORT, CONNECTOR-CONTRACT,
  DESIGN-SYSTEM, EXPERIENCE-SPEC, IMPLEMENTATION-STATUS, INTERIM-ASSETS,
  MOTION-SPEC, ROADMAP, SCREENSHOT-MANIFEST, TESTING, UI-MAP, VISUAL-ACCEPTANCE,
  VISUAL-SPEC, component-inventory, screen-manifest, visual-spec.
- `docs/adr/` — 7 ADRs: 001 router choice (hash), 002 react-compiler, 003
  preview rendering boundary, 004 procedural mock index, 005 dependency
  resolution, 007 theme token adoption, **008 lost-data flow** (no 006 exists).
- `docs/evidence/` — 146 files: `a11y/` (axe runs, keyboard walks, theme
  verification JSONs/PNGs) and `vlm/shots/` (VLM screenshot analyses).
- Repo convention (commit dd2782c; docs/ROADMAP.md:16-18): **living docs** are
  synced to code (UI-MAP, screen-manifest, component-inventory, ARCHITECTURE,
  CONNECTOR-CONTRACT post-ADR-008, IMPLEMENTATION-STATUS, TESTING);
  **historical session docs get dated addenda only** (AUDIT-REPORT is marked
  "Historical record (session 4)", EXPERIENCE-SPEC, MOTION-SPEC, ACCEPTANCE,
  DESIGN-SYSTEM, INTERIM-ASSETS, SCREENSHOT-MANIFEST).
- Plus `worklog.md` (append-only multi-session log, newest at bottom) and
  `worklog/` (4 saved session prompts).

**Docs that claim the mock/simulator as the backend and must be updated when the
real core is wired** (grep -i "mock" hits): `README.md` (the entire framing:
"driven entirely by a Mock Connector… Swapping the simulator for a real IPC
bridge is a one-line environment change"), `docs/CONNECTOR-CONTRACT.md` (§"Mock
simulator guarantees" :101-112 + §"Swapping in the real bridge" :89-99),
`docs/ARCHITECTURE.md` ("hard connector boundary between UI and (mock) backend";
layer diagram "3 thin commands… No domain logic"), `docs/TESTING.md` (mock
contract timing budgets), `docs/UI-MAP.md`, `docs/ROADMAP.md` (roadmap item 3:
"Real validation: replace the JS-side validateDestination…"), 
`docs/INTERIM-ASSETS.md`, `docs/ACCEPTANCE.md`, `docs/VISUAL-ACCEPTANCE.md`,
`docs/AUDIT-REPORT.md`, `docs/IMPLEMENTATION-STATUS.md` (tracker rows cite mock
verifications). Code comments with the same claim: contract.ts:2-5 header,
index.ts, tauri-connector.ts, mock-connector.ts header, capabilities/default.json
description. The e2e/CI also pin `VITE_CONNECTOR: mock` for smoke builds.

---

## 8. Re-merge mechanics facts

- Fresh repo: default branch **v3 @ dd2782c** ("fix: fire the push CI on v3 …
  bump 0.3.4"), prior v3 tip f4a8f8a ("… bump 0.3.3"); remote branches v3 + v4.
  `git status` clean.
- final-prod adopted the UI at merge **86bc3168** ("merge: adopt
  recovery-wizard-ui-new history as ui/ (from f4a8f8a, v3)") — merge second
  parent is d7413e03 with the f4a8f8a message; final-prod then diverged
  (currently `ui/` @ e7cec9bd, "release(v0.6.0-rc2)").
- **Tracked files in final-prod/ui/ that do NOT exist in the fresh repo (37,
  exact list from `git ls-files` diff):**
  - `src-tauri/src/bridge/{abi,consent,error,events,filefilter,logging,state,util,mod}.rs` (9)
  - `src-tauri/src/commands/{devices,health,preview,recovery,repair,results,scan,sessions,system,mod}.rs` (10, incl. mod.rs)
  - `src-tauri/Cargo.lock`
  - `scripts/gen-filefilter.mjs`
  - `tests/connector-tauri.test.ts`, `tests/fakes/test-connector.ts`,
    `tests/health-truth.test.tsx`, `tests/results-tree-label.test.tsx`
  - `e2e/consent.spec.ts`
  - `.env.example`, `metadata.json` (AI-Studio scaffold — deliberately removed
    in fresh v0.3.4 clean-room pass, docs/ROADMAP.md:11-12)
  - `public/assets/aistudio/*.png` (11 reference screenshots — also deliberately
    removed; fresh repo has no `public/` at all)
- **Files in the fresh repo NOT in final-prod/ui/ (13):** `.github/workflows/
  {ci,release,ui-smoke,visual-compare}.yml` (superseded by final-prod root
  workflows — the ui-ci/ui-e2e root workflows exist there), `src/connector/mock/
  {fixtures,mock-connector,node-space,rng}.ts`, `src/connector/tauri/
  not-implemented.ts`, `src/queries/dev.ts`, `src/queries/repair.ts`,
  `tests/connector-mock.test.ts`, `tests/node-space.test.ts`. I.e. the fresh
  repo's mock connector, its tests, and the tauri stub are NEW relative to
  final-prod's adopted base (final-prod's copy predates the connector split into
  mock/ + tauri/ directories).
- **Version lockstep implication**: fresh repo is **0.3.4** (package.json,
  tauri.conf.json, Cargo.toml — all three); final-prod VERSION is
  **0.6.0-rc2**. final-prod's `build.ps1` `Assert-VersionConsistency`
  (build.ps1:36-44) FAILS the build unless `VERSION` == `ui/package.json`
  version == `ui/src-tauri/Cargo.toml` version ==
  `ui/src-tauri/tauri.conf.json` version. So the re-merge must bump the adopted
  UI's three version fields to the product VERSION (or the release version
  decided for the consolidation) in the same change; the fresh repo's own
  release.yml tag gate (tag v* == tauri.conf.json version) applies only inside
  the standalone UI repo, not after adoption.

---

## 9. Wiring work plan inputs

Ordered seams where the real core (core_recovery, ABI v4) must be wired in:

1. **Re-pin the ABI binding.** Rebuild `src-tauri/src/bridge/abi.rs` from
   `core_recovery/include/cr/cr_ui.h` (`CR_UI_ABI_VERSION 4u`, :81). The old
   final-prod bridge is ABI v2 (78 exports) — it is a structural reference
   (state registries, event marshaling, consent, error mapping, filefilter
   generation) but its bindings are stale. Keep the startup ABI-version gate
   pattern (old lib.rs setup).
2. **Rebuild the bridge modules** under `src-tauri/src/bridge/` (state, error,
   events, logging, util, consent, filefilter-regen via a new gen script) and
   the 30-command surface in `src-tauri/src/commands/` matching the contract's
   34 commands (note deltas: the contract now has `getSessionMeta`,
   `getRecoveryReport`, `getUndoLog`/`undoRepairs`, and no `scan_close` — verify
   each against the v4 header).
3. **Implement `src/connector/tauri/tauri-connector.ts` domain by domain.**
   Natural order (each keeps the rest of the app green because the queries layer
   is the only consumer): `system` first (getAppInfo must flip connectorMode to
   'tauri' + real engineVersion; pickDirectory via the dialog plugin;
   listMountPoints; openPath via opener/reveal_in_explorer; getDriverStatus),
   then `devices`, then `scan` + the event bridge, then `results`/`preview`,
   `sessions`, `recovery` + events, `health`, `repair`.
4. **Event bridge**: engine progress callbacks (worker threads) → channel →
   single emitter thread → `app_handle.emit` (the old events.rs marshaling
   rules: sink must not block; coalesce superseded samples; unknown event types
   ignored; HEARTBEAT never synthesizes progress) → TS `listen()` wrapped in the
   two subscription methods returning unsubscribe closures.
5. **Elevation UX.** The core refuses raw-disk access without an elevated token
   with `ERR_ELEVATION_REQUIRED = 0x500C0001`
   (`core_recovery/include/cr/cr_error.h:120`; cr_ui.h:322-323 documents the
   host-side UAC trigger flow). **The connector contract has no elevation error
   code** — decide the mapping (likely `device-permission` + a dedicated
   reasonKey, or a new contract code) and the re-launch/elevation flow. The UI
   already renders permission-denied banners on Home (UI-MAP §2.1) that can host
   this.
6. **Consent flows.** Contract: `applyRepair(deviceId, {consentToken})` with the
   typed `REPAIR` token (mock-connector.ts:1405; UI two-factor gate in
   repair-page). The old bridge additionally had a **pre-scan repair consent**
   (bridge/consent.rs, engine `cr_ui_scan_config.pre_scan_repair`, repair_consent
   callback on the run thread, default-deny, `consent_respond` command +
   e2e/consent.spec.ts). The current contract's `ScanParams` has no pre-scan
   repair surface — if the v4 engine still exposes it, the contract needs an
   extension (event or scan-param) or it must be configured engine-side.
7. **Session persistence.** `.rsf`-style save/load must hit the real filesystem
   (mock is in-memory). final-prod has portable-mode marker behavior (sessions
   beside the exe) from a4dbcfc1 (NG-18) — reconcile where sessions live.
8. **Error model mapping**: engine numeric codes (cr_error.h) →
   `ConnectorErrorCode` + reasonKey, following old bridge/error.rs's
   `{code, bridgeCode, message}` split.
9. **Build switch + capability expansion**: set `VITE_CONNECTOR=tauri` for
   production builds (keep `mock` default for tests/e2e — CI smoke builds pin
   mock); expand `capabilities/default.json` beyond the minimal set as the real
   bridge adds permissions.

Every place the **no-silent-fallback rule** will catch incomplete wiring (keep
these green/truthful while wiring operation by operation):

- The Proxy stub throws `not-implemented` on ANY unwired domain method
  (tauri-connector.ts:14-22) — a partially-wired connector must keep the Proxy
  for unwired domains rather than faking results.
- `tests/connector-mock.test.ts:563-565` asserts the stub throws on every
  domain — update per domain as implementations land (or convert to a
  wired-domains enumeration).
- The mock's own `openPath` throws rather than pretending
  (mock-connector.ts:1569-1580) — the real connector must implement it via the
  OS opener, not no-op.
- `AppInfo.connectorMode` must report the truth (mock-connector.ts:1475 / tauri
  side) — the About dialog renders it.
- ast-grep `no-raw-invoke` confines `invoke()` to `src/connector/**`; arch-lint
  A1 confines factory imports to `src/queries/**` + `src/connector/**` — the
  real adapter must live entirely inside those boundaries.
- CI gates that must stay green throughout: typecheck, lint, format:check,
  arch:lint, ast-grep ×2, knip (no dead code — half-wired bridges will trip it),
  jscpd, 145 tests with coverage thresholds (src/connector/** is in the coverage
  include list at 70/60), build + bundle budget (200/500 KB gzip — the real
  adapter's TS must stay lean), cargo check + clippy -D warnings, 12 e2e (they
  run against mock builds), and in final-prod: contracts validate + build.ps1
  version lockstep.

---

*Sheet complete. Fresh-repo facts verified against the working tree at v3 @
dd2782c; final-prod facts against its current HEAD; core facts against
core_recovery/include/cr headers.*
