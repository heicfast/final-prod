# Type maps — every struct conversion the bridge performs

Field-level truth for the Rust bridge. Left column is always the engine type
(`engine/include/cr/cr_ui.h` unless noted); right column is always the UI type
(`ui/src/connector/contract.ts`). Anything not listed here does not cross the
boundary.

## 1. Devices — `cr_ui_disk_info` / `cr_ui_volume_info` → `Device` / `Volume`

| Engine | UI | Notes |
|---|---|---|
| `cr_ui_disk_info.number` | `Device.id` = `"disk-<number>"` | id scheme minted by the bridge |
| `cr_ui_disk_info.model[128]` | `Device.model` | UTF-8; empty → `"Hard Disk #<number>"` |
| `cr_ui_disk_info.serial[128]` | `Device.serial` | |
| `cr_ui_disk_info.size_bytes` | `Device.capacity.totalBytes` | per-disk totals |
| (Win32 `GetDiskFreeSpaceExW` per volume, summed) | `Device.capacity.usedBytes` / `freeBytes` | engine gives totals only; the bridge fills usage from the OS for lettered volumes; volumes without letters contribute 0 usage |
| `cr_ui_disk_info.removable` | `Device.kind`: `removable → 'usb'`, else `'internal'` | **G-9**: no bus type in the engine struct — 'sd'/'optical' are indistinguishable; recorded in INTEGRATION-GAPS |
| (bridge state: scan registry) | `Device.isSourceOfActiveScan` | |
| `cr_ui_disk_info.layout` (0 none/1 MBR/2 GPT/3 EBR), `table_ok` | `Device.canScanWholeDisk` = `true` | whole-disk entry point exists whenever a disk view is openable |
| (fresh open probe result) | `Device.status`: open OK → `'ok'`; `0x50020001` → `'device-permission'` (refined `'device-gone'` when enumeration dropped it); `0x50060001` on a volume → `'locked'` | |

| Engine | UI | Notes |
|---|---|---|
| `cr_ui_volume_info.letter` | `Volume.id` = `"vol-<letter>"`, `Volume.letter` = `"C:"`-style or null | unlettered volumes: `vol-<disk>-<offset>` id, letter null |
| `cr_ui_volume_info.disk` | `Volume.deviceId` = `"disk-<disk>"` | |
| `cr_ui_volume_info.label[64]` | `Volume.label` | |
| `cr_ui_volume_info.bytes` | `Volume.capacity.totalBytes` | usage from GetDiskFreeSpaceExW on the letter |
| `cr_ui_volume_info.fs_name[16]` | `Volume.fs`: `ntfs/fat32/exfat/ext4/btrfs/refs/hfsplus/zfs` passthrough; `raw` when carve-only; else `'unknown'` | engine strings are lowercase family names; the map is a table, not string matching on display text |
| `cr_ui_volume_info.bitlocker` | `Volume.bitlocker`: engine locked → `'locked'`; probe-open succeeded → `'unlocked'`; absent → `false` | tri-state preserved |
| (engine has no pre-scan source) | `Volume.lostPartition` = absent | **G-10**: lost candidates are scan-time events |
| (engine has no gate) | `Volume.quickScanAvailable` = `true` | the mock's bad-sector flip was simulation-only |
| `cr_ui_volume_info` (via owning disk row) | `Volume.sectorSize` = owning disk `logical_sector` | |

## 2. Scan progress — `cr_ui_telemetry` + `cr_ui_event` → `ScanProgressEvent`

| Engine | UI | Notes |
|---|---|---|
| telemetry `overall_permille` | `overallProgress` = permille/1000, clamped monotonic | engine guarantees monotonic; clamp is defense |
| event `stage_permille` (type 2) | `stageProgress` | resets per stage — matches contract |
| event `stage` + run phase | `stage`: `ScanStage` map in event-mapping.md | |
| telemetry `bytes_scanned` | `bytesScanned` | carve-true-bytes (honest fill rate) |
| device geometry `total_bytes` (or volume slice) | `bytesTotal` | |
| `files_found + deleted_found + carved_found` | `filesFound` | documented sum |
| (no live counter — **G-3**) | `dataFoundBytes` = 0 live; exact value computed at completion (snapshot sum) | |
| event type 4 (cumulative `error_ranges`) | `errors` | |
| event type 4 spans | `badSectorMap: BadSectorRange[]` (`startByte`=err_start_bytes, `sizeBytes`=err_len_bytes) | |
| bridge-derived (bytes/elapsed) | `etaSeconds`, `throughputBytesPerSecond` | |
| snapshot exists (`cr_ui_scan_snapshot`, partial ok) | `recoverableDuringScan` = true | the mock's 0.4 threshold was simulation |
| (no disconnect events — **G-8**) | `deviceConnected` = heartbeat-fresh within 2× heartbeat window | synthesized from CR_EVT_HEARTBEAT liveness + I/O error patterns |
| (no path event — **G-2**) | `currentPath` = null | UI hides the ticker; never fabricated |

## 3. Scan state — `cr_ui_scan_busy/stopping/telemetry` + run result → `ScanState`

| Engine | UI |
|---|---|
| registered `scanId` ("scan-N") | `ScanState.scanId` |
| registry entry | `params`, `target` (the resolved Device/Volume) |
| bridge-tracked start instant | `startedAt` |
| run() return code latched | `completedAt` + `status`: 0 → `'completed'`; `0x50020005` → `'cancelled'`; other → `'failed'` with `failure.code` via error-codes.json |
| busy && !paused | `'running'` |
| telemetry `paused` | `'paused'` |
| `stopping` | `'stopping'` |
| bridge transient (pre-run) | `'starting'`; token-call acks surface as `'pausing'`/`'resuming'` within the UI poll window |

## 4. Tree / results — `cr_ui_node_info` + snapshot navigation → `TreeNode` / `ResultNode` / `NodeDetail`

Node id scheme (bridge-minted, opaque to the UI): `"f<snapshot index>"` for
files, `"d<snapshot index>"` for directories — the registry stores
(index ↔ `node_id` u64 ↔ snapshot ref) so preview/recovery resolve to the right
engine handles. The scheme intentionally matches the mock's visible format so
zero UI components change.

| Engine | UI |
|---|---|
| `cr_ui_node_info.name` (two-call name fetch) | `name` / `label` |
| parent-index walk | `path` ('/'-joined) / `parentId` |
| `logical_size` | `sizeBytes` |
| `modified` (100ns-1601) | `modifiedAt` (ISO-8601; single conversion point in the bridge) |
| `attributes & 0x10` | `kind`: directory vs file |
| `origin` (0 structured / 1 carved / 2 lost / 3 virtual) | `origin`: structured+deleted==0 → `'existing'`; structured+deleted≠0 → `'deleted'`; 1 → `'raw'`; 2 → `'lost-path'`; 3 → `'unsaved'` |
| `confidence` (ordinal 0..4) | `confidence`: 4→`'excellent'`, 3→`'good'`, 2→`'fair'`, 1→`'poor'`, 0/unknown → `'unknown'` |
| category derivation (extension table, bridge-side; same classification the engine's category_mask uses) | `category`: `FileCategoryId` or null |
| `extent_count` | `NodeDetail.clusterCount` |
| `created` / `accessed` | `NodeDetail.createdAt` / `accessedAt` |
| `overlap` + carve provenance (`carve_format`, `hash_valid`, `sha128`) | `NodeDetail.confidenceReason` (i18n key composed by the bridge from grade/carve/verdict evidence) |
| node extents ∩ session error map | `NodeDetail.badSectorsOverlap` |
| snapshot subtree aggregate pass | `TreeNode.childCount` / `fileCount` / `sizeBytes` |

Virtual grouping nodes ('path' mode): bridge-synthesized `'lost-paths'`
(origin==2 aggregation), `'raw-found'`/`'raw-all'` (origin==1), with
`special` flags `lost-path`/`raw-found` — reproducing the mock's 3-branch
tree from real node origins. 'type' mode: 8 category nodes with
`ext:<cat>:<ext>` leaves (count-only query per category + extension
aggregation).

## 5. Query — `ResultsFilter` → `cr_ui_query`

| UI | Engine | Notes |
|---|---|---|
| `search` | `name_substring` | |
| `extensions` | `extensions[]` (lowercased WITH dot) + `extension_count` | |
| `categories` | `category_mask`: graphics→1, video→2, audio→3, document→4, archive→5, other→6, email→7, bookmarks→8 (OR of bits) | **OQ-2 resolved from cr_ui.h:794** |
| `sizeMinBytes`/`sizeMaxBytes` | `size_min`/`size_max` (0 = unbounded) | `hideEmptyFiles` → `size_min = max(size_min, 1)` |
| `dateFrom`/`dateTo` (ISO) | `modified_from`/`modified_to` (100ns-1601; 0 = unbounded) | |
| `origins` | `origin_mask` + `deleted_only`: existing → structured,deleted==0; deleted → structured,deleted≠0; raw → carved; lost-path → lost; unsaved → virtual | |
| `nodeId` + `treeMode` | pre-scan: path-prefix restriction via `path_prefix` where the node resolves to a path; on sessions the bridge resolves the node id to its subtree first | |
| sort `name/modified/type/size/confidence` | `sort` = 1/3/7/2/4; `direction asc/desc` → `dir` 0/1 | |
| `page.offset/limit` | `offset`/`limit` (UI page size 200 stays host-side) | |
| (count-only calls) | `count_only` = 1 | |
| `elapsedMs` | bridge wall-clock measurement of the query | |

`ResultsPage.total` ← `total_matched`; `totalBytes` ← page sum (bounded page —
the contract's display semantics); rows → `ResultNode` via §4 maps.

**Mid-scan (G-7)**: `cr_ui_query_run` requires a session (stable trees). For
live scans the bridge runs the same filter/sort/page semantics in Rust over
the cached `cr_ui_snapshot` (bounded refresh cadence; partial=1 is truth) —
the REAL partial tree replaces the mock's `visibleFraction` heuristic.

## 6. Recovery — `RecoverySelection`/`RecoveryOptions` → engine enqueue + config; stats/report → `RecoveryJob`/`RecoveryItem`/`RecoveryReport`

| UI | Engine |
|---|---|
| `options.collision` rename/skip/overwrite | `cr_ui_recovery_config.conflict` = 0/1/2 |
| `options.verifyOnWrite` | `skip_hash_verify` = !verifyOnWrite |
| `options.rebuildFolders` | `skip_dir_reconstruct` = !rebuildFolders |
| `options.keepEmptyFolders` | **no engine equivalent — G-5**: option disabled with truthful note |
| `options.fileRepair` | **no engine equivalent — G-5**: core repair is pre-scan RawFixer, not during-recovery; disabled with truthful note |
| (defaults) | `read_chunk` 0 = 1 MiB, `rename_attempts` 0 = 64 |
| `selection.kind === 'nodes'` files | `cr_ui_recovery_enqueue_node` per node id (session: `node_id`; mid-scan snapshot: `_snapshot` index variant, G-02b) |
| `selection.kind === 'nodes'` directories | `cr_ui_recovery_enqueue_subtree` / `_subtree_snapshot` |
| `selection.kind === 'all'` + filter | bridge pages the filtered query (§5) and enqueues every row (files + subtree roots) |
| `destination` | `cr_ui_recovery_set_destination` (UTF-8) — overlap armed on lettered targets (F12) |

Status map (stats → `RecoveryJob.status`):

| Engine signal | UI status |
|---|---|
| bridge enqueue phase | `'validating'` |
| `cr_ui_recovery_process` in flight | `'running'` |
| returned 0x50020005 | `'cancelled'` |
| fatal pre-stats error | `'failed'` |
| done>0, errors==0, refused==0, hash_mismatch==0 | `'completed'` |
| done>0, any of errors/refused/hash_mismatch>0 | `'completed-partial'` |
| `nothing_to_recover` only | `'completed'` with empty lists + truthful `processedTotal: 0` |

Items (`cr_ui_recovery_report_json` items[], `docs/recovery-report-schema.md`):
`outcome done → 'done'`, `refused → 'skipped'`, `errors/hash_mismatch →
'failed'` (reason carried), queued/pending → `'pending'`, in-flight →
`'running'`; `savedAs` ← item path; `processedTotal` ← exact
succeeded+failed+skipped count; per-list cap 500 with `truncatedItemLists:
true` (the mock's truncation contract, now backed by the engine's own report);
`totalBytesWritten` ← totals.bytes; `verified` ← count(hash_status=='verified')>0
&& errors==0; started/finished ← bridge-tracked instants.

## 7. Health — `cr_ui_smart_report` / `cr_ui_smart_attr` → `DiskHealth` / `SmartAttribute`

| Engine | UI |
|---|---|
| grading (failing_now / critical_warning / attr failing / wear) → 4-status Unknown/Good/Caution/Bad | `status`: Good→`'good'`, Caution→`'caution'`, Bad→`'bad'`; Unknown/`available==0` → health-unavailable error |
| `temperature_c` (ATA) / `temperature_c_x10` (NVMe) | `temperatureC` |
| `power_on_hours` / `power_on_hours_nvme` | `powerOnHours` |
| `ssd_like` | `diskClass`: 1→`'ssd'`, 0→`'hdd'`, unknown→`'unknown'` |
| `ssd_wear_level` (ATA) / `percentage_used` (NVMe) | `wearPercent` |
| `trim_known ? trim_enabled : null` | `trimEnabled` |
| `reallocated_sectors` / `pending_sectors` | `reallocatedSectors` / `pendingSectors` |
| `cr_ui_smart_attr{id,name,value,worst,raw,failing}` | `SmartAttribute{id,name,current:value,worst,threshold:null,raw,status: failing?'failing':'ok'}` — **the engine carries no threshold field; null is truth** |

## 8. Preview — `cr_ui_metadata` + `cr_ui_preview_*` → `PreviewPayload`

Connector decides kind (ADR-008) from engine metadata:

| Engine kind (cr_ui_metadata.kind) | UI PreviewPayload |
|---|---|
| `image` | `{kind:'image', dataUrl: base64(mime by format), width, height, bounded: size>cap}` — dataUrl assembly capped (preview-too-large beyond); EXIF orientation/datetime carried in `detail` |
| `text` | `{kind:'text', text: text_snippet (+bounded read), encoding, totalSize, truncated}` |
| `unsupported` (or encrypted-class ext) | throw `preview-unsupported` / `preview-encrypted` |
| `video`/`audio` (av) | **G-4**: no renderer — metadata + hex fallback with truthful "preview not rendered" (payload kind 'av' with metadata; thumbnailDataUrl null) |
| `document`/`archive` (office/pdf) | **G-4**: metadata + hex fallback likewise |

Hex fallback (default for unhandled kinds): `cr_ui_preview_read` 64 KiB →
`{kind:'hex', bytes, totalSize, baseOffset}` (ADR-003 bounded payload).
`cr_ui_preview_unreadable` spans > 0 → `preview-damaged` annotation rides the
payload (content still returned; the truth is displayed). Node with no bytes
(metadata-only) → `preview-missing-data`. Preview cap: `cr_ui_preview_read`
len ≤ 4 MiB/call (engine contract).

## 9. Sessions — bridge directory services → `SessionMeta`

| Field | Source |
|---|---|
| `id` | `"sess-<n>"` bridge-minted on load; for list rows the file-name hash |
| `fileName` | file name in the sessions dir |
| `savedAt` | file mtime |
| `targetLabel` | `cr_ui_session_get_info.label` (+"model") on loaded sessions; file-stem fallback on list |
| `filesFound` | `cr_ui_session_stats.files_found + deleted_found + carved_found` |
| `dataFoundBytes` | snapshot sum (one pass; **G-3** note applies) |
| `completed` | `stage_done_mask` vs `configured_mask` |

Session store: `%ProgramData%\HeicFast\Recovery Wizard\sessions\` (portable:
`.\sessions\`); auto-name `auto_<label>_<M>_<D>_<H>_<m>_<s>.crsf` (N-061
parity). Load: `cr_ui_session_load` → browsable registry entry; corrupt →
error-codes.json session rules.

## 10. System — bridge surfaces

- `DriverStatus`: installed = SCM service `RDA` exists; running = service
  running AND device `\\.\RDA` opens; version from IOCTL GET_VERSION
  (`driver/src/shared/inc/rda_ioctl.h`) when open; `note` carries the truthful
  unsigned/test-signing posture.
- `AppInfo`: `{name: Tauri productName, version: root VERSION, engineVersion:
  null (G-6 — no engine version export; ABI version 2 is a contract number,
  not a product version), platform: OS version string}`.
  `connectorMode` is REMOVED from the contract (mock gone — one real backend).
- `DestinationValidation`: same-letter source → `{allowed:false,
  reason:'overlap'}` (engine backstops 0x50050001); non-mount path →
  `'invalid'`; free < needed → `'out-of-space'` (engine backstops 0x50050004);
  write probe fails → `'unwritable'`.
- `MountPoint`: per lettered volume: `{id:"vol-<L>", label, path:"L:\\",
  freeBytes, totalBytes}`.
