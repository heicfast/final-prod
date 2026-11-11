/**
 * CONNECTOR CONTRACT — the single source of truth for every command and event
 * the recovery backend will ever expose. The UI talks ONLY to this interface
 * (via src/queries). Swapping the Mock Connector for a real IPC bridge must
 * require zero changes above this layer.
 *
 * Principles:
 *  - The UI renders and requests; the connector provides and grades.
 *  - No UI-side intelligence that belongs in the backend (no filesystem
 *    parsing, no confidence re-grading, no scan orchestration).
 *  - Errors carry machine codes + localizable reason keys; the UI never
 *    string-matches error text.
 */

// ---------------------------------------------------------------------------
// Shared primitives
// ---------------------------------------------------------------------------

/** ISO-8601 timestamp string. */
export type Iso8601 = string;

export type FileCategoryId =
  'graphics' | 'video' | 'audio' | 'document' | 'email' | 'archive' | 'bookmarks' | 'other';

/** Recovery-condition grades come from the connector; the UI never re-grades. */
export type ConfidenceGrade = 'excellent' | 'good' | 'fair' | 'poor' | 'unknown';

export type DeviceKind = 'internal' | 'usb' | 'sd' | 'optical' | 'virtual';
export type VolumeFs =
  | 'ntfs'
  | 'fat32'
  | 'exfat'
  | 'ext4'
  | 'btrfs'
  | 'refs'
  | 'hfsplus'
  | 'zfs'
  | 'raw'
  | 'unknown';

export type DeviceStatus = 'ok' | 'permission-denied' | 'disconnected' | 'locked';

export interface Capacity {
  readonly totalBytes: number;
  readonly usedBytes: number;
  readonly freeBytes: number;
}

export interface ConnectorError extends Error {
  readonly code: ConnectorErrorCode;
  /** i18n key suffix under `errors.` or `preview.reason.` for preview failures. */
  readonly reasonKey?: string;
  readonly detail?: Record<string, unknown>;
}

export type ConnectorErrorCode =
  | 'device-gone'
  | 'device-permission'
  | 'elevation-required'
  | 'scan-not-found'
  | 'scan-not-paused'
  | 'scan-not-running'
  | 'scan-scope-invalid'
  | 'consent-not-found'
  | 'session-corrupt'
  | 'session-not-found'
  | 'destination-overlap'
  | 'destination-invalid'
  | 'destination-unwritable'
  | 'out-of-space'
  | 'image-not-found'
  | 'preview-unsupported'
  | 'preview-too-large'
  | 'preview-damaged'
  | 'preview-encrypted'
  | 'preview-timeout'
  | 'preview-missing-data'
  | 'recovery-item-failed'
  | 'repair-consent-required'
  | 'repair-failed'
  | 'health-unavailable'
  | 'not-implemented'
  | 'unknown';

// ---------------------------------------------------------------------------
// Devices
// ---------------------------------------------------------------------------

export interface Device {
  readonly id: string;
  readonly kind: DeviceKind;
  /** The OS bus-type name ("SATA", "NVMe", "USB", "SD", ...) — the
   * real interface axis behind the kind split (D-ENUM-02). Absent on
   * connectors that cannot see the bus ("Unknown" when the descriptor
   * was unavailable). */
  readonly busType?: string;
  /** e.g. "NVMe SSD" or "USB Mass Storage". */
  readonly model: string;
  /** Friendly label, e.g. "Local Disk (C:)". */
  readonly label: string;
  readonly serial: string;
  readonly capacity: Capacity;
  readonly status: DeviceStatus;
  readonly isSourceOfActiveScan?: boolean;
  readonly volumes: Volume[];
  /** Whole-disk entry point exists (deep search on raw device). */
  readonly canScanWholeDisk: boolean;
  /** Logical sector size (bytes) — the real geometry axis for the
   * sector-position displays (health class line, scan "Reading sector").
   * Volume rows carry the per-volume view; the device row carries the
   * disk-wide truth. */
  readonly sectorSize?: number;
}

export interface Volume {
  readonly id: string;
  readonly deviceId: string;
  readonly label: string;
  readonly letter: string | null;
  readonly fs: VolumeFs;
  readonly capacity: Capacity;
  readonly status: DeviceStatus;
  /** Candidate lost partition found on this device (deep-search entry point). */
  readonly lostPartition?: LostPartitionHint;
  /** Quick search availability — when false the UI must offer deep only. */
  readonly quickScanAvailable: boolean;
  /** Optional connector-exposed sector size override for advanced settings. */
  readonly sectorSize?: number;
  /** Whether BitLocker encryption is enabled on this volume ('unlocked' | 'locked' | boolean). */
  readonly bitlocker?: boolean | 'unlocked' | 'locked';
}

export interface LostPartitionHint {
  readonly id: string;
  readonly startByte: number;
  readonly sizeBytes: number;
  readonly probableFs: VolumeFs;
  readonly note?: string;
}

export interface DevicesDomain {
  listDevices(): Promise<Device[]>;
  /**
   * Device info for a scan target. `deviceId` may be a device id or a volume
   * id — the connector resolves a volume to its owning device (matching
   * startScan's target semantics).
   */
  getDeviceInfo(deviceId: string): Promise<Device>;
  refreshDevices(): Promise<Device[]>;
  /**
   * N-064: the explicit-consent dictionary unlock for a locked BitLocker
   * volume (the engine's shipped common-password set + the caller's
   * candidates). User-initiated only — nothing in the scan/open path ever
   * tries passwords by itself. Throws device-permission when the whole set
   * failed (candidate indexes only in the log — never the password).
   */
  unlockVolume(volumeId: string, passwords: readonly string[]): Promise<void>;
}

// ---------------------------------------------------------------------------
// Scan
// ---------------------------------------------------------------------------

export type ScanStatus =
  | 'idle'
  | 'starting'
  | 'running'
  | 'paused'
  | 'pausing'
  | 'resuming'
  | 'stopping'
  | 'completed'
  | 'cancelled'
  | 'failed';

export type ScanStage =
  'volume-analysis' | 'quick-index' | 'deep-walk' | 'carve' | 'tree-build' | 'done';

/** Which reference-visible phase a stage belongs to (status-line labels). */
export type ScanPhase = 'quick' | 'advanced' | 'organizing';

export function scanPhaseOf(stage: ScanStage): ScanPhase {
  if (stage === 'quick-index' || stage === 'volume-analysis') return 'quick';
  if (stage === 'deep-walk' || stage === 'carve') return 'advanced';
  return 'organizing';
}

export interface ScanParams {
  readonly targetId: string;
  /** Optional folder restriction ("Specify a location" -> Select Folder).
   * Null/omitted = whole volume. The scan ALWAYS runs quick -> advanced
   * automatically (ADR-008): the reference exposes no mode selection. */
  readonly pathScope?: string | null;
  /**
   * M-033: optional full-volume-encryption key (64 hex chars AES-128-XTS
   * or 128 hex chars AES-256-XTS, key||tweak-key) — consumed by the
   * engine's locked-volume open. Null/omitted = the plain open (a locked
   * volume then refuses with device-permission until unlocked).
   */
  readonly bitlockerFvek?: string | null;
}

export interface ScanProgressEvent {
  readonly scanId: string;
  readonly status: ScanStatus;
  readonly stage: ScanStage;
  /** 0..1 within the current stage. */
  readonly stageProgress: number;
  /** 0..1 weighted across stages; monotonic — never regresses. */
  readonly overallProgress: number;
  readonly bytesScanned: number;
  readonly bytesTotal: number;
  readonly filesFound: number;
  readonly dataFoundBytes: number;
  readonly errors: number;
  readonly badSectorMap: readonly BadSectorRange[];
  readonly etaSeconds: number | null;
  readonly throughputBytesPerSecond: number;
  /** True when results so far are safe to recover while the scan runs. */
  readonly recoverableDuringScan: boolean;
  readonly deviceConnected: boolean;
  /** The item currently being examined — the live "Searching: ..." ticker. */
  readonly currentPath: string | null;
}

export interface BadSectorRange {
  readonly startByte: number;
  readonly sizeBytes: number;
}

export interface ScanState {
  readonly scanId: string;
  readonly params: ScanParams;
  readonly target: Device | Volume;
  readonly status: ScanStatus;
  readonly startedAt: Iso8601;
  readonly completedAt: Iso8601 | null;
  readonly progress: ScanProgressEvent;
  /** Present on failed scans. */
  readonly failure?: { code: ConnectorErrorCode; reasonKey?: string };
  /**
   * G-1 (a) re-hydration: the pending pre-scan repair consent, or null.
   * The prompt renders from this when the consent event fired before the
   * page's subscription landed, or after navigating away and back.
   */
  readonly pendingConsent?: ConsentRequestEvent | null;
  /** NG-11: live lost-partition candidate rows (whole-disk scans — engine
   * events joined with the N-061 geometry, never synthesized). */
  readonly lostCandidates?: LostCandidateSummary;
  /** W1c: the completion auto-save landed (the auto session feeds
   * resumeLastSession). */
  readonly autosaved?: boolean;
}

/** The pre-scan repair consent payload (S-008, consent-gated). */
export interface ConsentRequestEvent {
  readonly scanId: string;
  /** The echo token — the ONLY key respondToConsent accepts. */
  readonly requestId: string;
  readonly plan: {
    readonly detected: number;
    readonly fsFamily: string;
    readonly actionCount: number;
    readonly refused: number;
    readonly reason: string;
    readonly advisoryCount: number;
  };
}

/** One discovered lost-partition candidate (the N-061 scored row). */
export interface LostCandidateRow {
  readonly lba: number;
  readonly sectorCount: number | null;
  readonly fs: number | null;
  readonly score: number | null;
}

export interface LostCandidateSummary {
  readonly count: number;
  readonly rows: readonly LostCandidateRow[];
}

export interface ScanDomain {
  startScan(params: ScanParams): Promise<{ scanId: string }>;
  pauseScan(scanId: string): Promise<void>;
  resumeScan(scanId: string): Promise<void>;
  cancelScan(scanId: string): Promise<void>;
  getScanState(scanId: string): Promise<ScanState>;
  /** Subscribe to progress events; returns an unsubscribe function. */
  onScanProgress(scanId: string, cb: (event: ScanProgressEvent) => void): () => void;
  /**
   * W-01 (the dead-letter fix): the terminal event — the bridge emits
   * `scan-finished:<id>` when the run thread latches the terminal state
   * (scan.rs). Subscribing surfaces completion INSTANTLY (the 500 ms state
   * poll is the fallback, not the primary).
   */
  onScanFinished(scanId: string, cb: (event: ScanFinishedEvent) => void): () => void;
  /** W-01: the session-save progress stream (`save-progress:<id>`, 0..1). */
  onSaveProgress(scanId: string, cb: (event: SaveProgressEvent) => void): () => void;
  /**
   * W-01: a lost-partition candidate surfaced mid-scan
   * (`lost-candidate:<id>`); the count is cumulative. The live results tree
   * refreshes as candidates land (browse-while-scan, reference parity).
   */
  onLostCandidate(scanId: string, cb: (event: LostCandidateEvent) => void): () => void;
  /** W-01: the engine's milestone auto-save landed (`scan-autosaved:<id>`). */
  onScanAutosaved(scanId: string, cb: (event: ScanAutosavedEvent) => void): () => void;
  /**
   * G-1 (a): the pre-scan repair consent. The engine's run thread emits
   * this at most once per scan; the prompt ALSO renders from
   * getScanState().pendingConsent (re-hydration for subscribe-late and
   * navigate-away-and-back).
   */
  onConsentRequest(scanId: string, cb: (event: ConsentRequestEvent) => void): () => void;
  /** Answer a consent request (unknown/double ids throw consent-not-found). */
  respondToConsent(requestId: string, allow: boolean): Promise<void>;
  /**
   * Lane C (the observability surface): the live stage telemetry — the
   * axes behind the reference's Reading-sector / Advanced-Scanning
   * status family (corpus UIUX dynamic-ground-truth:88-105). Live scans
   * only; a loaded session's final counters ride getSessionMeta.
   */
  getScanTelemetry(scanId: string): Promise<ScanTelemetry>;
  /**
   * Lane C: the scan-completion grading statistics — the reference's
   * RecoveryAssistant block + the CRecoverability gauge axes. The UI
   * renders the verdict; the connector never invents one.
   */
  getScanAssistantStats(scanId: string): Promise<ScanAssistantStats>;
  /**
   * Lane C: the static carve-format registry (the reference's
   * analyser-list surface — both layers, deterministic order).
   */
  listCarveFormats(): Promise<readonly CarveFormatRow[]>;
  /**
   * Lane C: the per-format carve tally (the reference's found-N-.jpg
   * post-scan report) + the aggregate user-sign slot. Live scans only.
   */
  getCarveTally(scanId: string): Promise<readonly CarveTallyRow[]>;
  /**
   * Lane C: the sector-map data (the reference's AnalysisGridWidget
   * producer): allocation/coverage/found/confidence overlays + bad
   * ranges over a bounded cell grid. `options` selects the byte window
   * and granularity (defaults: whole scan, ~4096 cells).
   */
  getDiskMap(scanId: string, options?: DiskMapOptions): Promise<DiskMapPayload>;
}

// ---------------------------------------------------------------------------
// Lane C — the observability + imaging + maintenance payloads
// (type-maps §12-§14; the bridge commands' response shapes)
// ---------------------------------------------------------------------------

/** One stage's telemetry (cr_ui_stage_tel: permille, engine work units,
 * pause-compensated elapsed). */
export interface StageTelemetry {
  readonly permille: number;
  readonly unitsDone: number;
  readonly unitsTotal: number;
  readonly bytesDone: number;
  readonly elapsedMs: number;
  readonly completed: boolean;
}

/** The full live telemetry snapshot (cr_ui_telemetry + flagged companions). */
export interface ScanTelemetry {
  readonly scanId: string;
  readonly overallPermille: number;
  readonly running: boolean;
  readonly paused: boolean;
  readonly cancelled: boolean;
  readonly elapsedMs: number;
  readonly filesFound: number;
  readonly deletedFound: number;
  readonly carvedFound: number;
  readonly lostPartitions: number;
  readonly bytesScanned: number;
  readonly bytesFound: number;
  readonly errorRanges: number;
  readonly stages: Readonly<Record<'quick' | 'deep' | 'carve', StageTelemetry>>;
  /** Present only when the engine answered the final-stats query. */
  readonly finalStats?: Readonly<{
    readonly filesFound: number;
    readonly deletedFound: number;
    readonly carvedFound: number;
    readonly lostPartitions: number;
    readonly bytesScanned: number;
    readonly bytesUnreadable: number;
    readonly elapsedMs: number;
    readonly errorRanges: number;
    readonly checkRan: boolean;
    readonly checkErrors: number;
    readonly repairRan: boolean;
    readonly repairApplied: number;
    readonly autosaved: boolean;
  }>;
  readonly statsAvailable: boolean;
  readonly checkpointsAvailable: boolean;
  /** Present only when the engine answered the checkpoint query. */
  readonly checkpoints?: number;
}

/** The scan-completion grading statistics (S-023; the og's
 * RecoveryAssistant block — category order: image, video, audio,
 * document, archive, other, email, bookmarks). */
export type AssistantCategoryKey =
  'image' | 'video' | 'audio' | 'document' | 'archive' | 'other' | 'email' | 'bookmarks';

export interface ScanAssistantStats {
  readonly scanId: string;
  readonly totalFileCount: number;
  readonly rawCount: number;
  readonly lostPartitionExistFileCount: number;
  readonly lostPartitionDeleteFileCount: number;
  readonly quickScanExistFileCount: number;
  readonly quickScanDeleteFileCount: number;
  readonly deepScanFileCount: number;
  readonly quickScanDeleteExtend: number;
  readonly categoryCounts: Readonly<Record<AssistantCategoryKey, number>>;
  /** The recoverability gauge axes: the reference forces matched to 0
   * under total < 100 (smallResult true — the gauge never shows). */
  readonly matched: number;
  readonly total: number;
  readonly smallResult: boolean;
  readonly gradedCount: number;
}

/** One carve-format registry row (layer 0 grammar validator, 1 raw-sign
 * prefilter). */
export interface CarveFormatRow {
  readonly index: number;
  readonly layer: 0 | 1;
  readonly ext: string;
  readonly minSizeBytes: number;
  readonly maxSizeBytes: number;
  /** 0..1000 before validator deltas. */
  readonly baseConfidence: number;
  readonly magicLen: number;
  readonly magicOffset: number;
}

/** One per-format found-count slot; the trailing user-sign aggregate row
 * carries formatIndex/ext null and userSigns true. */
export interface CarveTallyRow {
  readonly formatIndex: number | null;
  readonly layer: number | null;
  readonly ext: string | null;
  readonly userSigns: boolean;
  readonly count: number;
}

/** The map view selector: startByte/totalBytes select the byte window,
 * cellBytes the granularity (0 = auto against maxCells), maxCells the
 * payload bound (default 4096, hard cap 65536). */
export interface DiskMapOptions {
  readonly startByte?: number;
  readonly totalBytes?: number;
  readonly cellBytes?: number;
  readonly maxCells?: number;
}

export interface DiskMapGeometry {
  readonly startByte: number;
  readonly totalBytes: number;
  readonly cellBytes: number;
  readonly cellCount: number;
  readonly clusterBytes: number;
  readonly fsFamily: number;
}

/** The sector-map payload (the AnalysisGridWidget producer): per-cell
 * overlays, each null when the engine declined it. allocation: 0 unknown,
 * 1 used, 2 free, 3 bad. coverage: 0 unscanned, 1 scanned. found: distinct
 * covering files (capped 255). confidence: max Confidence ordinal 0..4. */
export interface DiskMapPayload {
  readonly scanId: string;
  readonly geometry: DiskMapGeometry;
  readonly allocation: readonly number[] | null;
  readonly coverage: readonly number[] | null;
  readonly found: readonly number[] | null;
  readonly confidence: readonly number[] | null;
  readonly badRanges: readonly BadSectorRange[];
}

/** The transport truth for a device ("direct" | "image") plus the
 * advisory last-transport-error code (hex, null when none). */
export interface DeviceTransportInfo {
  readonly deviceId: string;
  readonly transport: string;
  readonly lastTransportError: string | null;
}

// ---------------------------------------------------------------------------
// Imaging (lane C — the reference's Create Image flow)
// ---------------------------------------------------------------------------

export type ImageJobStatus = 'running' | 'completed' | 'failed' | 'cancelled';

export interface ImageProgressEvent {
  readonly imageId: string;
  readonly status: ImageJobStatus;
  readonly bytesDone: number;
  readonly bytesTotal: number;
  readonly overallProgress: number;
  readonly throughputBytesPerSecond: number;
  readonly etaSeconds: number | null;
}

export interface ImageFinishedEvent {
  readonly imageId: string;
  readonly status: ImageJobStatus;
  /** True when the engine kept `<path>.partial` (a filesystem truth
   * probe, never parsed from message text). */
  readonly partialKept: boolean;
}

export interface ImageJobState extends ImageProgressEvent {
  readonly disk: number;
  readonly path: string;
  readonly startedAt: Iso8601;
  readonly finishedAt: Iso8601 | null;
  /** Always false — the ABI exports no cancel (the documented divergence
   * from the reference's stop-confirm; the UI renders a truthful
   * runs-to-completion posture, never a dead stop button). */
  readonly cancelSupported: boolean;
  readonly partialKept: boolean;
}

export interface ImagingDomain {
  /**
   * Start a byte-for-byte image of the whole physical disk (`deviceId`
   * must be a disk-N id) to `path` (the engine streams to
   * `<path>.crimg-tmp`, renames on completion + writes a `.sha256`
   * sidecar). Bridge pre-gates: destination on the imaged disk refuses
   * destination-overlap; free space below the byte-for-byte need refuses
   * out-of-space. NO cancel exists (cancelSupported is always false).
   */
  startImageCreation(deviceId: string, path: string): Promise<{ imageId: string }>;
  getImageState(imageId: string): Promise<ImageJobState>;
  /** The progress stream (`image-progress:<imageId>`, coalesced). */
  onImageProgress(imageId: string, cb: (event: ImageProgressEvent) => void): () => void;
  /** The terminal event (`image-finished:<imageId>`). */
  onImageFinished(imageId: string, cb: (event: ImageFinishedEvent) => void): () => void;
}

// ---------------------------------------------------------------------------
// Maintenance (lane C — the privileged one-shots)
// ---------------------------------------------------------------------------

export type FsckVerdict =
  'clean' | 'fixed' | 'scheduled' | 'errors-remain' | 'timeout-still-running' | 'spawn-failed';

export interface FsckResult {
  /** The volume letter, normalized "E:". */
  readonly letter: string;
  /** The RAW chkdsk exit code (259 = STILL_ACTIVE at the engine's
   * documented 15-minute bound; -1 = the spawn itself failed). */
  readonly exitCode: number;
  readonly verdict: FsckVerdict;
  readonly verdictKey: string;
  readonly fix: boolean;
}

export interface BitlockerOffResult {
  readonly letter: string;
  /** OK means ACCEPTED, not plaintext — the OS decrypt continues in the
   * background (the honest unobservable-percentage contract). */
  readonly accepted: boolean;
}

export interface MaintenanceDomain {
  /**
   * The check-phase escalation rung: chkdsk on the volume at `letter`
   * ("E" or "E:\"), check posture by default or fix with `fix: true`.
   * The spawn is hidden (CREATE_NO_WINDOW — no console ever reaches the
   * desktop). Bounded: the engine's internal synchronous bound is 15
   * minutes (exitCode 259 = still running past it).
   */
  runFsck(letter: string, fix?: boolean): Promise<FsckResult>;
  /**
   * The DESTRUCTIVE decrypt twin of the unlock surface: turn BitLocker
   * OFF on the volume at `letter`. Explicit opt-in only — callers gate
   * this behind a confirmation dialog; nothing in the scan/open path
   * ever calls it. Resolves when the request was ACCEPTED.
   */
  turnOffBitlocker(letter: string): Promise<BitlockerOffResult>;
}

// ---------------------------------------------------------------------------
// The W-01 event payloads — the bridge emissions that were dead letters
// through v0.8.x (emitted by events.rs/scan.rs/recovery.rs, never listened).
// Field maps mirror the bridge JSON exactly (event-mapping §2).
// ---------------------------------------------------------------------------

export interface ScanFinishedEvent {
  readonly status: 'completed' | 'cancelled' | 'failed';
  /** The engine failure vocabulary key (present on 'failed'). */
  readonly failure?: string | null;
  /** Whether the completion auto-save landed (rc == 0). */
  readonly autosaved: boolean;
}

export interface SaveProgressEvent {
  /** 0..1 — the bridge's save permille / 1000. */
  readonly fraction: number;
}

export interface LostCandidateEvent {
  readonly lba: number;
  readonly sectorCount: number | null;
  /** The engine's FsKindCode (cr_types.h) — numeric, like the row state. */
  readonly fs: number | null;
  readonly score: number | null;
  /** Cumulative candidate count for the scan. */
  readonly count: number;
}

export interface ScanAutosavedEvent {
  /** 0 = the auto session landed (the bridge's rc). */
  readonly rc: number;
}

export interface SessionLoadProgressEvent {
  readonly path: string;
  readonly permille: number;
  readonly nodesDone: number;
  readonly nodesTotal: number;
}

export interface RecoveryFinishedEvent {
  readonly status: string;
}

// ---------------------------------------------------------------------------
// Results
// ---------------------------------------------------------------------------

export type ResultNodeKind = 'file' | 'directory' | 'group';

export type ResultOrigin = 'existing' | 'deleted' | 'raw' | 'lost-path' | 'unsaved';

export interface ResultNode {
  readonly id: string;
  readonly scanId: string;
  readonly kind: ResultNodeKind;
  readonly name: string;
  readonly path: string | null;
  readonly sizeBytes: number;
  readonly modifiedAt: Iso8601 | null;
  readonly extension: string | null;
  readonly category: FileCategoryId | null;
  readonly confidence: ConfidenceGrade;
  readonly origin: ResultOrigin;
  readonly childCount: number | null;
  readonly thumbnailAvailable: boolean;
  readonly parentId: string | null;
  readonly attributes?: Readonly<Record<string, string>>;
}

export type ResultTreeMode = 'path' | 'type' | 'raw';

export interface TreeNode {
  readonly id: string;
  readonly parentId: string | null;
  readonly label: string;
  readonly icon: 'directory' | 'category' | 'group' | 'device';
  readonly childCount: number;
  readonly fileCount: number;
  readonly sizeBytes: number;
  readonly origin: ResultOrigin;
  readonly special?: 'deleted' | 'lost-path' | 'raw-found' | 'unsaved' | 'existing';
}

export interface TreePage {
  readonly nodes: readonly TreeNode[];
  readonly parentHasMore: boolean;
}

export type SortField = 'name' | 'modified' | 'type' | 'size' | 'confidence';
export type SortDirection = 'asc' | 'desc';

export interface ResultsFilter {
  readonly nodeId: string | null;
  readonly treeMode: ResultTreeMode;
  readonly search: string | null;
  readonly categories: readonly FileCategoryId[] | null;
  readonly dateFrom: Iso8601 | null;
  readonly dateTo: Iso8601 | null;
  readonly sizeMinBytes: number | null;
  readonly sizeMaxBytes: number | null;
  readonly extensions: readonly string[] | null;
  readonly origins: readonly ResultOrigin[] | null;
  /** True = drop zero-byte files ("Hide 0 KB files" toggle). */
  readonly hideEmptyFiles: boolean;
}

export interface ResultsPage {
  readonly nodes: readonly ResultNode[];
  readonly total: number;
  readonly totalBytes: number;
  readonly offset: number;
  readonly pageSize: number;
  /** Wall-time the connector took to resolve this page (perf grid #36). */
  readonly elapsedMs: number;
}

export interface NodeDetail {
  readonly node: ResultNode;
  readonly fullPhysicalPath: string | null;
  readonly createdAt: Iso8601 | null;
  readonly accessedAt: Iso8601 | null;
  readonly confidenceReason: string | null;
  readonly badSectorsOverlap: boolean;
  readonly clusterCount: number | null;
}

export interface ResultsDomain {
  getResultTree(
    scanId: string,
    parentId: string | null,
    mode: ResultTreeMode,
  ): Promise<TreePage>;
  queryResults(
    scanId: string,
    filter: ResultsFilter,
    sort: { field: SortField; direction: SortDirection },
    page: { offset: number; limit: number },
  ): Promise<ResultsPage>;
  getNodeDetail(scanId: string, nodeId: string): Promise<NodeDetail>;
  countResults(scanId: string, filter: ResultsFilter): Promise<number>;
  /**
   * Lane C (S-023, the reference's per-selection re-grade): re-run the
   * grading rules on the given session-tree nodes. Only grade-1 nodes
   * are eligible (grades 0/2 are sticky) — `regraded` counts what
   * actually moved. Session trees only (a live scan's grades are the
   * completion pass's own).
   */
  regradeNodes(scanId: string, nodeIds: readonly string[]): Promise<{ regraded: number }>;
}

// ---------------------------------------------------------------------------
// Preview
// ---------------------------------------------------------------------------

export type PreviewPayload =
  | {
      readonly kind: 'image';
      readonly dataUrl: string;
      readonly width: number;
      readonly height: number;
      readonly bounded: boolean;
    }
  | {
      readonly kind: 'hex';
      readonly bytes: Uint8Array;
      readonly totalSize: number;
      readonly baseOffset: number;
    }
  | {
      readonly kind: 'text';
      readonly text: string;
      readonly encoding: string;
      readonly totalSize: number;
      readonly truncated: boolean;
    }
  | {
      readonly kind: 'pdf';
      readonly pages: readonly {
        readonly dataUrl: string;
        readonly width: number;
        readonly height: number;
      }[];
      readonly pageCount: number;
    }
  | {
      readonly kind: 'av';
      readonly metadata: Readonly<Record<string, string>>;
      readonly thumbnailDataUrl: string | null;
      readonly durationSeconds: number | null;
    }
  | {
      readonly kind: 'office';
      readonly metadata: Readonly<Record<string, string>>;
      readonly extractedText: string | null;
      readonly truncated: boolean;
    }
  | {
      /** M-116: the engine's bounded archive inventory (512-entry hostile
       * bound; `truncated` flags the cut — never a fabricated listing). */
      readonly kind: 'archive';
      readonly entries: readonly {
        readonly name: string;
        readonly size: number;
        readonly compressed: number;
        readonly crcPresent: boolean;
        readonly nameTruncated: boolean;
      }[];
      readonly truncated: boolean;
    };

export interface PreviewDomain {
  /** The connector decides the preview kind from its own file knowledge;
   * the UI never classifies extensions (engine knowledge stays behind the
   * connector boundary). */
  requestPreview(scanId: string, nodeId: string): Promise<PreviewPayload>;
  /**
   * M-102: server-side thumbnails. Loaded SESSIONS generate through the
   * engine (EXIF-oriented, display-shaped, grade-promoting); LIVE scans
   * assemble from preview reads (documented degradation, BF-13). Returns
   * null when the node has no thumbnail (oversized / unsupported / no
   * source) — never a fabricated image.
   */
  getThumbnail(scanId: string, nodeId: string): Promise<{ dataUrl: string } | null>;
}

// ---------------------------------------------------------------------------
// Sessions (.rsf-style scan session persistence)
// ---------------------------------------------------------------------------

export interface SessionMeta {
  readonly id: string;
  readonly fileName: string;
  /** Full store path (the resume/load key). */
  readonly path?: string;
  readonly savedAt: Iso8601 | null;
  readonly targetLabel: string;
  readonly filesFound: number;
  readonly dataFoundBytes: number;
  readonly completed: boolean;
}

export interface SessionsDomain {
  saveSession(scanId: string, path: string | null): Promise<SessionMeta>;
  /** N-060: save-scoped cancel for the save spinner (never touches the
   * scan's cancel token — save-on-scan-cancel stays intact). */
  cancelSave(scanId: string): Promise<void>;
  listSessions(): Promise<SessionMeta[]>;
  loadSession(path: string): Promise<{ scanId: string }>;
  /** W-01 (the dead-letter fix): the session-load progress stream
   * (`session-load-progress:<path>`, coalesced to 5% steps by the bridge). */
  onSessionLoadProgress(
    path: string,
    cb: (event: SessionLoadProgressEvent) => void,
  ): () => void;
  getSessionMeta(path: string): Promise<SessionMeta>;
  /** M-154: load the auto session the last-scan pointer names (the Home
   * "continue last" banner). Throws session-not-found when no live pointer
   * names a present file — the caller falls back to listSessions. */
  resumeLastSession(): Promise<{ scanId: string; path: string }>;
  /**
   * The scan RESUME: continue an incomplete session's scan (load + resume
   * + identity verify + run). Throws session-corrupt on an identity
   * mismatch (the session came from a different volume) and device-gone
   * when the source is absent.
   */
  resumeSession(path: string): Promise<{ scanId: string; targetId: string }>;
}

// ---------------------------------------------------------------------------
// Recovery
// ---------------------------------------------------------------------------

export type CollisionPolicy = 'rename' | 'overwrite' | 'skip';

export interface RecoveryOptions {
  readonly collision: CollisionPolicy;
  readonly rebuildFolders: boolean;
  readonly keepEmptyFolders: boolean;
  readonly fileRepair: boolean;
  readonly verifyOnWrite: boolean;
}

export interface RecoveryJob {
  readonly jobId: string;
  readonly scanId: string;
  readonly nodeIds: readonly string[];
  readonly destination: string;
  readonly options: RecoveryOptions;
  readonly status: RecoveryJobStatus;
}

export type RecoveryJobStatus =
  'validating' | 'running' | 'completed' | 'completed-partial' | 'cancelled' | 'failed';

export interface RecoveryItem {
  readonly nodeId: string;
  readonly name: string;
  readonly sizeBytes: number;
  readonly state: 'pending' | 'running' | 'done' | 'failed' | 'skipped' | 'verifying';
  readonly error?: { code: ConnectorErrorCode; reasonKey?: string };
  readonly savedAs?: string;
}

export interface RecoveryProgressEvent {
  readonly jobId: string;
  readonly status: RecoveryJobStatus;
  readonly overallProgress: number;
  readonly currentFile: string | null;
  readonly currentFileProgress: number;
  readonly completedCount: number;
  readonly failedCount: number;
  readonly skippedCount: number;
  readonly totalCount: number;
  readonly throughputBytesPerSecond: number;
  readonly verifying: boolean;
  readonly etaSeconds: number | null;
}

export interface RecoveryReport {
  readonly jobId: string;
  readonly status: RecoveryJobStatus;
  readonly startedAt: Iso8601;
  readonly finishedAt: Iso8601 | null;
  readonly succeeded: readonly RecoveryItem[];
  readonly failed: readonly RecoveryItem[];
  readonly skipped: readonly RecoveryItem[];
  readonly totalBytesWritten: number;
  readonly verified: boolean;
  /** Exact number of items the job processed (succeeded+failed+skipped),
   *  independent of the bounded per-item lists. */
  readonly processedTotal: number;
  /** True when the item lists were capped (recover-all at scale); counts
   *  remain exact — only the per-item arrays are bounded. */
  readonly truncatedItemLists: boolean;
}

/** What to recover: an explicit node list, or everything matching a filter
 *  (the reference's Recover All — the export loop iterates the whole set). */
export type RecoverySelection =
  | { readonly kind: 'nodes'; readonly nodeIds: readonly string[] }
  | { readonly kind: 'all'; readonly filter: ResultsFilter };

export interface RecoveryDomain {
  startRecovery(
    scanId: string,
    selection: RecoverySelection,
    destination: string,
    options: RecoveryOptions,
  ): Promise<{ jobId: string }>;
  cancelRecovery(jobId: string): Promise<void>;
  getRecoveryState(jobId: string): Promise<RecoveryJob>;
  getRecoveryItems(
    jobId: string,
    page: { offset: number; limit: number },
  ): Promise<readonly RecoveryItem[]>;
  onRecoveryProgress(jobId: string, cb: (event: RecoveryProgressEvent) => void): () => void;
  /** W-01 (the dead-letter fix): the recovery job's terminal event
   * (`recovery-finished:<jobId>`) — instant completion surfacing. */
  onRecoveryFinished(jobId: string, cb: (event: RecoveryFinishedEvent) => void): () => void;
  getRecoveryReport(jobId: string): Promise<RecoveryReport>;
}

// ---------------------------------------------------------------------------
// Health
// ---------------------------------------------------------------------------

export type HealthStatus = 'good' | 'caution' | 'bad';

export interface SmartAttribute {
  readonly id: number;
  readonly name: string;
  readonly current: number | null;
  readonly worst: number | null;
  readonly threshold: number | null;
  readonly raw: number;
  readonly status: 'ok' | 'failing' | 'unknown';
}

export interface DiskHealth {
  readonly deviceId: string;
  readonly status: HealthStatus;
  readonly temperatureC: number | null;
  readonly powerOnHours: number | null;
  readonly diskClass: 'ssd' | 'hdd' | 'unknown';
  readonly wearPercent: number | null;
  readonly trimEnabled: boolean | null;
  readonly reallocatedSectors: number | null;
  readonly pendingSectors: number | null;
  readonly attributes: readonly SmartAttribute[];
  /** The transport that produced the data ("ata-passthrough" |
   * "nvme" | "legacy") — null when unknown. Truthful provenance. */
  readonly transport?: string | null;
  /** NVMe health-log power cycles — null when absent (never fabricated). */
  readonly powerCycles?: number | null;
  /** A readable but empty ATA table — "unknown", never "good". */
  readonly tableEmpty?: boolean;
}

export interface HealthDomain {
  getHealth(deviceId: string): Promise<DiskHealth>;
}

// ---------------------------------------------------------------------------
// Repair (transactional volume structure repair)
// ---------------------------------------------------------------------------

export type RepairFindingKind =
  | 'boot-sector-mismatch'
  | 'backup-dbr-stale'
  | 'fat-table-damaged'
  | 'mft-record-error'
  | 'sectors-per-cluster-corrupt'
  | 'checksum-mismatch';

export interface RepairFinding {
  readonly id: string;
  readonly kind: RepairFindingKind;
  readonly where: string;
  readonly descriptionKey: string;
  readonly fixable: boolean;
  readonly severity: 'info' | 'warning' | 'critical';
}

export interface RepairDryRun {
  readonly deviceId: string;
  readonly findings: readonly RepairFinding[];
  readonly canApply: boolean;
}

export interface RepairUndoEntry {
  readonly index: number;
  readonly action: string;
  readonly where: string;
  readonly at: Iso8601;
  readonly snapshotBytes: number;
}

export interface RepairDomain {
  dryRunRepair(deviceId: string): Promise<RepairDryRun>;
  applyRepair(
    deviceId: string,
    options: { consentToken: string },
  ): Promise<{ applied: number }>;
  getUndoLog(deviceId: string): Promise<readonly RepairUndoEntry[]>;
  undoRepairs(deviceId: string): Promise<{ rolledBack: number }>;
}

// ---------------------------------------------------------------------------
// Lost-partition apply (S-029, consent-gated)
// ---------------------------------------------------------------------------

/** The apply consent payload (the plan the engine asks permission for). */
export interface ApplyConsentEvent {
  readonly deviceId: string;
  /** The echo token — the ONLY key respondToApplyConsent accepts. */
  readonly requestId: string;
  readonly plan: {
    readonly refused: number;
    readonly scope: number;
    readonly startLba: number;
    readonly sectorCount: number;
    readonly mbrType: number;
    readonly slot: number;
    readonly regenerateSignature: number;
    readonly signature: number;
    readonly reason: string;
  };
}

/** The apply outcome, verbatim from the engine (cr_ui_apply_result). */
export interface ApplyResult {
  readonly applied: boolean;
  /** 0 applied; 1 consent denied; 2 refused; 3 table changed since plan;
   * 4 applied but the refresh tail failed (undo valid); 5 error. */
  readonly outcome: number;
  readonly undoEntries: number;
  readonly reason: string;
}

export interface LostPartitionDomain {
  /** Transcribe a scan-state candidate onto the disk's table (consent-gated;
   * whole-disk views only — volume slices are refused by the engine). */
  applyLostPartition(
    deviceId: string,
    candidate: LostCandidateRow,
    rebuildTable?: boolean,
  ): Promise<ApplyResult>;
  /** Restore the last apply's undo log (byte-exact, idempotent). */
  undoLastApply(deviceId: string): Promise<ApplyResult>;
  /** Persist the last apply's undo log (loadable by rrecov fix --undo-apply). */
  saveApplyUndo(deviceId: string, path: string): Promise<void>;
  /** The apply consent event (subscribe before invoking apply). */
  onApplyConsent(deviceId: string, cb: (event: ApplyConsentEvent) => void): () => void;
  /** Answer an apply consent request (echo token). */
  respondToApplyConsent(requestId: string, allow: boolean): Promise<void>;
  /** Re-hydration: the pending apply consent for a device, or null. */
  pendingApplyConsent(deviceId: string): Promise<ApplyConsentEvent | null>;
}

// ---------------------------------------------------------------------------
// System
// ---------------------------------------------------------------------------

export interface AppInfo {
  readonly name: string;
  readonly version: string;
  readonly connectorMode: 'mock' | 'tauri';
  readonly engineVersion: string | null;
  readonly platform: string;
  /** M-048: the active user-taught signature count (the About row). */
  readonly userSigns?: number;
}

export interface MountPoint {
  readonly id: string;
  readonly label: string;
  readonly path: string;
  readonly freeBytes: number;
  readonly totalBytes: number;
}

export interface DestinationValidation {
  readonly allowed: boolean;
  readonly reason?: 'overlap' | 'unwritable' | 'invalid' | 'out-of-space';
  readonly reasonKey?: string;
  readonly freeBytes: number | null;
  readonly neededBytes: number | null;
}

export interface SystemDomain {
  getAppInfo(): Promise<AppInfo>;
  /**
   * D1 (the privilege model): elevation is a LAUNCH-TIME property — the
   * product's embedded manifest requires the administrator token before
   * any UI exists, so there is no runtime posture probe and no
   * relaunch-as-administrator flow. The connector surface reflects the
   * product as it is.
   */
  /** M-048: register a user-signature JSON document (atomic replace; the
   * exe-adjacent defaults are auto-loaded at startup). The webview's file
   * input supplies the document text. */
  loadUserSigns(content: string): Promise<{ count: number }>;
  validateDestination(
    path: string,
    sourceDeviceId: string,
    neededBytes: number,
  ): Promise<DestinationValidation>;
  /**
   * Native folder picker; mock returns from a canned set in dev/browser mode.
   * The optional hint roots the dialog at a volume (scan-scope picking on the
   * scanned device) or asks it to avoid a device (destination picking must
   * not suggest the recovery source); the connector still owns validation of
   * the result.
   */
  pickDirectory(hint?: {
    readonly volumeId?: string | null;
    readonly excludeDeviceId?: string | null;
  }): Promise<string | null>;
  listMountPoints(): Promise<readonly MountPoint[]>;
  openPath(path: string): Promise<void>;
  /**
   * Lane C: the transport truth for a device ("direct" | "image") plus
   * the advisory last-transport-error code — the diagnostics axis behind
   * open failures (never gating anything by itself).
   */
  getDeviceTransport(deviceId: string): Promise<DeviceTransportInfo>;
}

// ---------------------------------------------------------------------------
// The connector
// ---------------------------------------------------------------------------

export interface RecoveryConnector {
  readonly devices: DevicesDomain;
  readonly scan: ScanDomain;
  readonly results: ResultsDomain;
  readonly preview: PreviewDomain;
  readonly sessions: SessionsDomain;
  readonly recovery: RecoveryDomain;
  readonly health: HealthDomain;
  readonly repair: RepairDomain;
  readonly lostPartition: LostPartitionDomain;
  readonly system: SystemDomain;
  readonly imaging: ImagingDomain;
  readonly maintenance: MaintenanceDomain;
}

/** Dev-panel scenario control (mock-only surface; real connector throws). */
export interface ScenarioControl {
  setScenario(scenario: MockScenarioId): void;
  getScenario(): MockScenarioId;
  setSeed(seed: number): void;
  reset(): void;
}

export type MockScenarioId =
  | 'standard'
  | 'large-ntfs'
  | 'sd-card'
  | 'btrfs-disk'
  | 'lost-partition'
  | 'permission-denied'
  | 'disconnect-mid-scan'
  | 'bad-sectors'
  | 'failing-disk'
  | 'out-of-space'
  | 'corrupt-session';
