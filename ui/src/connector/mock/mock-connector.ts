/**
 * Mock Connector — TEST DOUBLE. Never the shipped backend.
 *
 * A fully typed, deterministic simulator implementing the same command/event
 * contracts the real backend provides. It exists for dev browser mode
 * (VITE_CONNECTOR=mock), vitest (pinned via vite.config test.env) and the
 * e2e lane — the production default is the REAL tauri bridge
 * (src/connector/index.ts; plan §4.8's mock demotion).
 *
 * Determinism: fixed seed per scenario (overridable via ScenarioControl).
 * Timing: progress ticks 100–250 ms; cancel responds within 300 ms.
 * No UI logic lives here: no date formatting, no label choices, no
 * client-side re-grading — the UI consumes connector data as-is.
 */

import { version as appVersion } from '../../../package.json';
import type {
  AppInfo,
  ApplyConsentEvent,
  ApplyResult,
  AssistantCategoryKey,
  BadSectorRange,
  BitlockerOffResult,
  CarveFormatRow,
  CarveTallyRow,
  ConsentRequestEvent,
  ConnectorError,
  ConnectorErrorCode,
  DestinationValidation,
  Device,
  DeviceTransportInfo,
  DiskHealth,
  DiskMapGeometry,
  DiskMapOptions,
  DiskMapPayload,
  FsckResult,
  ImageFinishedEvent,
  ImageJobState,
  ImageJobStatus,
  ImageProgressEvent,
  LostCandidateEvent,
  LostCandidateRow,
  MockScenarioId,
  MountPoint,
  NodeDetail,
  PreviewPayload,
  RecoveryFinishedEvent,
  RecoverySelection,
  RecoveryItem,
  RecoveryJob,
  RecoveryJobStatus,
  RecoveryOptions,
  RecoveryProgressEvent,
  RecoveryReport,
  RepairDryRun,
  RepairUndoEntry,
  ResultsFilter,
  ResultsPage,
  ResultTreeMode,
  ScanAssistantStats,
  ScanAutosavedEvent,
  ScanFinishedEvent,
  ScanParams,
  ScanProgressEvent,
  ScanStage,
  ScanState,
  ScanTelemetry,
  SaveProgressEvent,
  SessionLoadProgressEvent,
  SessionMeta,
  SmartAttribute,
  SortDirection,
  SortField,
  StageTelemetry,
  SystemDomain,
  TreeNode,
  TreePage,
  Volume,
} from '../contract';
import { fixturesFor } from './fixtures';
import { NodeSpace } from './node-space';
import { mulberry32, randBell, randInt } from './rng';

function connError(
  code: ConnectorErrorCode,
  message: string,
  reasonKey?: string,
): ConnectorError {
  return Object.assign(new Error(message), { code, reasonKey });
}

const IMAGE_EXTS = new Set([
  'jpg',
  'jpeg',
  'png',
  'gif',
  'bmp',
  'tif',
  'tiff',
  'webp',
  'heic',
  'svg',
  'psd',
  'cr2',
  'cr3',
  'nef',
  'arw',
  'dng',
  'raw',
  'ico',
]);
const AV_EXTS = new Set([
  'mp4',
  'mov',
  'avi',
  'mkv',
  'wmv',
  'flv',
  'm4v',
  'webm',
  'mts',
  'm2ts',
  'mp3',
  'wav',
  'flac',
  'aac',
  'ogg',
  'wma',
]);
const TEXT_EXTS = new Set([
  'txt',
  'log',
  'md',
  'json',
  'xml',
  'yml',
  'yaml',
  'ini',
  'cfg',
  'csv',
  'tsv',
]);
const OFFICE_EXTS = new Set(['doc', 'docx', 'xls', 'xlsx', 'ppt', 'pptx', 'odt']);

/** Connector-side preview-kind dispatch (mirrors the engine's per-type
 *  previewer routing — artifact 10 §2). */
function previewKindOf(node: {
  category: string | null;
  extension: string | null;
}): 'image' | 'av' | 'pdf' | 'text' | 'office' | 'hex' {
  const ext = (node.extension ?? '').replace(/^\./, '').toLowerCase();
  if (ext === 'pdf') return 'pdf';
  if (IMAGE_EXTS.has(ext)) return 'image';
  if (AV_EXTS.has(ext)) return 'av';
  if (OFFICE_EXTS.has(ext)) return 'office';
  if (TEXT_EXTS.has(ext)) return 'text';
  return 'hex';
}
/**
 * The ONE automatic stage chain (ADR-008; architecture/04 §3-4, GT-scan
 * §3/§7): volume probe -> quick pass (FS metadata) -> advanced pass (full
 * searchers + RAW carving) -> organize. The quick pass is a short prefix
 * (~21 s on the reference's runner volume); advanced dominates the run.
 */
const STAGE_WEIGHTS: Partial<Record<ScanStage, number>> = {
  'volume-analysis': 0.04,
  'quick-index': 0.14,
  'deep-walk': 0.42,
  carve: 0.24,
  'tree-build': 0.16,
};
const STAGE_ORDER: readonly ScanStage[] = [
  'volume-analysis',
  'quick-index',
  'deep-walk',
  'carve',
  'tree-build',
  'done',
];

interface ScanRuntime {
  params: ScanParams;
  target: Device | Volume;
  status: ScanState['status'];
  startedAt: string;
  completedAt: string | null;
  stage: ScanStage;
  stageProgress: number;
  overall: number;
  bytesScanned: number;
  bytesTotal: number;
  filesFound: number;
  dataFound: number;
  errors: number;
  badSectors: BadSectorRange[];
  eta: number | null;
  throughput: number;
  tick: number;
  timer: ReturnType<typeof setInterval> | null;
  disconnectFired: boolean;
  disconnectHandled: boolean;
  space: NodeSpace;
  volumeId: string;
  scanId: string;
  /** Folder-scope directory mask (null = whole volume). */
  scopeMask: Uint8Array | null;
  /** Live "Searching: ..." ticker item (deep pass + carve). */
  currentPath: string | null;
  /** Stages to run (quick pass skipped when the engine cannot index). */
  stages: readonly ScanStage[];
  /** W-01: lost-partition candidates surfaced so far (fixture-derived —
   * the lost-partition volume's own geometry, never synthetic). */
  lostCandidates: LostCandidateRow[];
  /** W-01: whether the completion auto-save landed. */
  autosaved: boolean;
}

/**
 * Lane C: the mock imaging runtime — the Create Image flow's simulator
 * (deterministic ticks, the same emitter-registry discipline the scan and
 * recovery runtimes use). NO cancel is simulated: the contract's truthful
 * cancelSupported=false posture is mirrored verbatim.
 */
interface ImageRuntime {
  imageId: string;
  disk: number;
  path: string;
  status: ImageJobStatus;
  bytesDone: number;
  bytesTotal: number;
  throughput: number;
  tick: number;
  timer: ReturnType<typeof setInterval> | null;
  startedAt: string;
  finishedAt: string | null;
}

/** The mock carve catalog: a deterministic sample of the engine's two
 * layers (grammar validators + raw-sign prefilters) — shape-exact, a
 * fixture not a mirror of the engine's real table. */
const MOCK_CARVE_CATALOG: readonly CarveFormatRow[] = [
  {
    index: 0,
    layer: 0,
    ext: 'jpg',
    minSizeBytes: 134,
    maxSizeBytes: 0,
    baseConfidence: 900,
    magicLen: 3,
    magicOffset: 0,
  },
  {
    index: 1,
    layer: 0,
    ext: 'png',
    minSizeBytes: 45,
    maxSizeBytes: 0,
    baseConfidence: 900,
    magicLen: 8,
    magicOffset: 0,
  },
  {
    index: 2,
    layer: 0,
    ext: 'pdf',
    minSizeBytes: 85,
    maxSizeBytes: 0,
    baseConfidence: 800,
    magicLen: 5,
    magicOffset: 0,
  },
  {
    index: 3,
    layer: 0,
    ext: 'zip',
    minSizeBytes: 30,
    maxSizeBytes: 0,
    baseConfidence: 700,
    magicLen: 4,
    magicOffset: 0,
  },
  {
    index: 4,
    layer: 1,
    ext: 'mp4',
    minSizeBytes: 1024,
    maxSizeBytes: 4_294_967_296,
    baseConfidence: 600,
    magicLen: 0,
    magicOffset: 0,
  },
  {
    index: 5,
    layer: 1,
    ext: 'mov',
    minSizeBytes: 1024,
    maxSizeBytes: 4_294_967_296,
    baseConfidence: 550,
    magicLen: 0,
    magicOffset: 0,
  },
  {
    index: 6,
    layer: 1,
    ext: 'docx',
    minSizeBytes: 512,
    maxSizeBytes: 1_073_741_824,
    baseConfidence: 650,
    magicLen: 0,
    magicOffset: 0,
  },
  {
    index: 7,
    layer: 1,
    ext: 'heic',
    minSizeBytes: 1024,
    maxSizeBytes: 1_073_741_824,
    baseConfidence: 600,
    magicLen: 0,
    magicOffset: 0,
  },
];

interface JobRuntime {
  jobId: string;
  scanId: string;
  destination: string;
  options: RecoveryOptions;
  /** Explicit selection (kind: 'nodes'). */
  items: MutableRecoveryItem[];
  /** Recover-all: lazily walk every matching node (kind: 'all'). */
  allMode: null | {
    filter: ResultsFilter;
    space: NodeSpace;
    scopeMask: Uint8Array | null;
    cursor: number;
    /** Deterministic per-index outcome rolls, resolved on demand. */
    total: number;
  };
  status: RecoveryJobStatus;
  progress: MutableJobProgress;
  timer: ReturnType<typeof setInterval> | null;
  startedAt: string;
  finishedAt: string | null;
  bytesWritten: number;
  verified: boolean;
  /** Items advanced per tick (scale-aware). */
  batch: number;
  /** First item index not yet known settled (avoids O(n) re-scans). */
  advanceCursor: number;
}

/** Locally mutable mirror of contract progress/item types (mock-internal only). */
type MutableJobProgress = {
  -readonly [K in keyof RecoveryProgressEvent]: RecoveryProgressEvent[K];
};
type MutableRecoveryItem = {
  -readonly [K in keyof RecoveryItem]: RecoveryItem[K];
};

export class MockConnector {
  private scenario: MockScenarioId = 'standard';
  private seed = 42;
  private deviceList: Device[] = [];
  private scans = new Map<string, ScanRuntime>();
  private nodeSpaces = new Map<string, NodeSpace>();
  private sessionStore = new Map<string, SessionMeta>();
  private sessionPathToScan = new Map<string, string>();
  private jobs = new Map<string, JobRuntime>();
  private repairUndoLogs = new Map<string, RepairUndoEntry[]>();
  private repairApplied = new Map<string, number>();
  private nextScanId = 1;
  private nextJobId = 1;
  private imageJobs = new Map<string, ImageRuntime>();
  private imageProgressListeners = new Map<string, Set<(e: ImageProgressEvent) => void>>();
  private imageFinishedListeners = new Map<string, Set<(e: ImageFinishedEvent) => void>>();
  private nextImageId = 1;

  // ------------------------------------------------------------------
  // W-01: the event-emitter registry — the mock mirrors the bridge's
  // previously-dead-lettered emissions (scan-finished, save-progress,
  // lost-candidate, scan-autosaved, session-load-progress,
  // recovery-finished) so the subscription surface is exercised by the
  // same lifecycle that drives the real product.
  // ------------------------------------------------------------------
  private scanFinishedListeners = new Map<string, Set<(e: ScanFinishedEvent) => void>>();
  private saveProgressListeners = new Map<string, Set<(e: SaveProgressEvent) => void>>();
  private lostCandidateListeners = new Map<string, Set<(e: LostCandidateEvent) => void>>();
  private scanAutosavedListeners = new Map<string, Set<(e: ScanAutosavedEvent) => void>>();
  private sessionLoadProgressListeners = new Map<
    string,
    Set<(e: SessionLoadProgressEvent) => void>
  >();
  private recoveryFinishedListeners = new Map<
    string,
    Set<(e: RecoveryFinishedEvent) => void>
  >();

  private subscribeTo<T>(
    registry: Map<string, Set<(e: T) => void>>,
    key: string,
    cb: (e: T) => void,
  ): () => void {
    let set = registry.get(key);
    if (!set) {
      set = new Set();
      registry.set(key, set);
    }
    set.add(cb);
    return () => {
      set.delete(cb);
      if (set.size === 0) registry.delete(key);
    };
  }

  private emitTo<T>(registry: Map<string, Set<(e: T) => void>>, key: string, event: T): void {
    const set = registry.get(key);
    if (!set) return;
    for (const cb of set) cb(event);
  }
  private nextSessionId = 1;
  private pickIndex = 0;
  private nextConsentId = 1;
  private consentListeners = new Map<string, Set<(e: ConsentRequestEvent) => void>>();
  private pendingConsents = new Map<
    string,
    { event: ConsentRequestEvent; resolve: (allow: boolean) => void }
  >();
  private applyConsentListeners = new Map<string, Set<(e: ApplyConsentEvent) => void>>();
  private pendingApplyConsents = new Map<string, ApplyConsentEvent>();
  private userSigns: { ext: string; offset: number; hex: string }[] = [];

  constructor() {
    this.rebuildDevices();
  }

  // ------------------------------------------------------------ scenario API

  setScenario(scenario: MockScenarioId): void {
    this.scenario = scenario;
    this.rebuildDevices();
  }

  getScenario(): MockScenarioId {
    return this.scenario;
  }

  setSeed(seed: number): void {
    this.seed = seed;
    this.rebuildDevices();
  }

  reset(): void {
    this.pendingConsents.clear();
    this.pendingApplyConsents.clear();
    this.applyConsentResolvers.clear();
    this.applyUndo.clear();
    this.userSigns = [];
    for (const scan of this.scans.values()) {
      if (scan.timer) clearInterval(scan.timer);
    }
    for (const job of this.jobs.values()) {
      if (job.timer) clearInterval(job.timer);
    }
    this.scans.clear();
    this.jobs.clear();
    this.sessionStore.clear();
    this.sessionPathToScan.clear();
    this.repairUndoLogs.clear();
    this.repairApplied.clear();
    this.seed = 42;
    this.scenario = 'standard';
    this.rebuildDevices();
  }

  private rebuildDevices(): void {
    const fixtures = fixturesFor(this.scenario);
    this.deviceList = [];
    this.nodeSpaces.clear();
    for (const f of fixtures) {
      const device: Device = {
        ...f.device,
        volumes: f.volumes.map((v) => ({
          ...v,
          status:
            this.scenario === 'permission-denied' && v.letter === 'C'
              ? 'permission-denied'
              : v.status,
          quickScanAvailable: this.scenario === 'bad-sectors' ? false : v.quickScanAvailable,
        })),
        isSourceOfActiveScan: false,
      };
      this.deviceList.push(device);
      const primary = f.volumes[0];
      const space = new NodeSpace(
        primary.id,
        f.nodeSeed ^ this.seed,
        f.directoryCount,
        f.fileCount,
        Math.round(primary.capacity.totalBytes * 0.985),
      );
      this.nodeSpaces.set(primary.id, space);
    }
  }

  private spaceFor(targetId: string): NodeSpace {
    const direct = this.nodeSpaces.get(targetId);
    if (direct) return direct;
    // A device id resolves to ITS first volume's space — never another
    // device's (the old any-space fallback silently scanned the wrong disk).
    const { volume } = this.resolveTarget(targetId);
    const byVolume = this.nodeSpaces.get(volume.id);
    if (byVolume) return byVolume;
    throw connError('scan-not-found', `no node space for ${targetId}`);
  }

  private resolveTarget(targetId: string): { device: Device; volume: Volume } {
    for (const d of this.deviceList) {
      for (const v of d.volumes) {
        if (v.id === targetId || d.id === targetId) {
          return { device: d, volume: v };
        }
      }
    }
    throw connError('device-gone', `target ${targetId} not found`, 'errors.deviceGone');
  }

  // ------------------------------------------------------------ devices

  readonly devices = {
    listDevices: async (): Promise<Device[]> => {
      await latency(180, 60);
      return this.deviceList;
    },
    getDeviceInfo: async (deviceId: string): Promise<Device> => {
      await latency(90, 40);
      const d = this.deviceList.find((x) => x.id === deviceId);
      if (d) return d;
      // A volume id resolves to the owning device — scan targets may be
      // either a whole device or one of its volumes (see resolveTarget).
      const owner = this.deviceList.find((x) => x.volumes.some((v) => v.id === deviceId));
      if (owner) return owner;
      throw connError('device-gone', 'device not found', 'errors.deviceGone');
    },
    refreshDevices: async (): Promise<Device[]> => {
      await latency(700, 200);
      this.rebuildDevices();
      return this.deviceList;
    },

    unlockVolume: async (volumeId: string, _passwords: readonly string[]): Promise<void> => {
      // TEST DOUBLE: the deterministic unlock — the 'permission-denied'
      // scenario keeps the volume locked (the refusal path); every other
      // scenario unlocks (the happy path the dialog serves).
      await latency(600, 200);
      const vol = this.deviceList.flatMap((d) => d.volumes).find((v) => v.id === volumeId);
      if (!vol) throw connError('device-gone', 'volume not found', 'errors.deviceGone');
      if (this.scenario === 'permission-denied') {
        throw connError(
          'device-permission',
          'the dictionary did not unlock the volume',
          'errors.devicePermission',
        );
      }
      (vol as { bitlocker: 'unlocked' | 'locked' | boolean }).bitlocker = 'unlocked';
      (vol as { status: string }).status = 'ok';
    },
  };

  // ------------------------------------------------------------ scan

  readonly scan = {
    startScan: async (params: ScanParams): Promise<{ scanId: string }> => {
      await latency(420, 120);
      const { device, volume } = this.resolveTarget(params.targetId);
      if (volume.status === 'permission-denied' || device.status === 'permission-denied') {
        throw connError('device-permission', 'access denied', 'errors.devicePermission');
      }
      const scanId = `scan-${this.nextScanId++}`;
      const space = this.spaceFor(params.targetId);
      const bytesTotal = volume.capacity.totalBytes;
      // Engine truthfulness: when the volume cannot be quick-indexed the
      // quick pass collapses to a probe and the advanced pass carries the run.
      const stages = volume.quickScanAvailable
        ? STAGE_ORDER
        : (['volume-analysis', 'deep-walk', 'carve', 'tree-build', 'done'] as const);

      // Folder scope (null/empty = whole volume). Unresolvable paths are a
      // hard contract error — the UI cannot override the connector verdict.
      let scopeMask: Uint8Array | null = null;
      if (params.pathScope != null && params.pathScope !== '') {
        const scope = space.resolveScope(params.pathScope);
        if (!scope) {
          throw connError(
            'scan-scope-invalid',
            `folder ${params.pathScope} not found on ${volume.label}`,
            'errors.scanScopeInvalid',
          );
        }
        scopeMask = scope.mask;
      }

      const rt: ScanRuntime = {
        params,
        target: params.targetId.startsWith('vol-') ? volume : device,
        status: 'running',
        startedAt: new Date().toISOString(),
        completedAt: null,
        stage: 'volume-analysis',
        stageProgress: 0,
        overall: 0,
        bytesScanned: 0,
        bytesTotal,
        filesFound: 0,
        dataFound: 0,
        errors: 0,
        badSectors: this.scenario === 'bad-sectors' ? this.makeBadSectorMap(bytesTotal) : [],
        eta: null,
        throughput: 0,
        tick: 0,
        timer: null,
        disconnectFired: false,
        disconnectHandled: false,
        space,
        volumeId: volume.id,
        scanId,
        scopeMask,
        currentPath: null,
        stages,
        lostCandidates: [],
        autosaved: false,
      };
      this.scans.set(scanId, rt);
      rt.timer = setInterval(() => this.scanTick(rt), 55 + (scanId.length % 3) * 25);
      return { scanId };
    },

    pauseScan: async (scanId: string): Promise<void> => {
      const rt = this.requireScan(scanId);
      if (rt.status !== 'running') throw connError('scan-not-running', 'scan is not running');
      rt.status = 'paused';
      if (rt.timer) clearInterval(rt.timer);
      rt.timer = null;
      await latency(180);
    },

    resumeScan: async (scanId: string): Promise<void> => {
      const rt = this.requireScan(scanId);
      if (rt.status !== 'paused') throw connError('scan-not-paused', 'scan is not paused');
      // A resume after a disconnect injection models the user having reconnected
      // the device (the UI shows the reconnect prompt when deviceConnected=false).
      if (rt.disconnectFired) {
        rt.disconnectFired = false;
      }
      rt.status = 'running';
      rt.timer = setInterval(() => this.scanTick(rt), 55 + (scanId.length % 3) * 25);
      await latency(200);
    },

    cancelScan: async (scanId: string): Promise<void> => {
      const rt = this.requireScan(scanId);
      if (rt.timer) clearInterval(rt.timer);
      rt.timer = null;
      rt.status = 'cancelled';
      rt.completedAt = new Date().toISOString();
      // W-01: the terminal event (the mock's cancel does not auto-save —
      // the honest autosaved:false).
      this.emitTo(this.scanFinishedListeners, scanId, {
        status: 'cancelled',
        failure: null,
        autosaved: rt.autosaved,
      });
      await latency(220);
    },

    getScanState: async (scanId: string): Promise<ScanState> => {
      await latency(40, 20);
      const rt = this.requireScan(scanId);
      return this.toScanState(rt);
    },

    onScanProgress: (scanId: string, cb: (event: ScanProgressEvent) => void): (() => void) => {
      const handler = (): void => {
        const rt = this.scans.get(scanId);
        if (rt) cb(this.toProgress(rt));
      };
      const interval = setInterval(handler, 150);
      return () => clearInterval(interval);
    },

    // W-01 (the dead-letter fix): the mock's mirror of the bridge's
    // scan-finished / save-progress / lost-candidate / scan-autosaved
    // emissions — fired by the same lifecycle transitions that drive them
    // in the real product.
    onScanFinished: (scanId: string, cb: (event: ScanFinishedEvent) => void): (() => void) =>
      this.subscribeTo(this.scanFinishedListeners, scanId, cb),

    onSaveProgress: (scanId: string, cb: (event: SaveProgressEvent) => void): (() => void) =>
      this.subscribeTo(this.saveProgressListeners, scanId, cb),

    onLostCandidate: (scanId: string, cb: (event: LostCandidateEvent) => void): (() => void) =>
      this.subscribeTo(this.lostCandidateListeners, scanId, cb),

    onScanAutosaved: (scanId: string, cb: (event: ScanAutosavedEvent) => void): (() => void) =>
      this.subscribeTo(this.scanAutosavedListeners, scanId, cb),

    onConsentRequest: (
      scanId: string,
      cb: (event: ConsentRequestEvent) => void,
    ): (() => void) => {
      const set = this.consentListeners.get(scanId) ?? new Set();
      set.add(cb);
      this.consentListeners.set(scanId, set);
      return () => {
        set.delete(cb);
      };
    },

    respondToConsent: async (requestId: string, allow: boolean): Promise<void> => {
      const pending = this.pendingConsents.get(requestId);
      if (!pending) {
        throw connError(
          'consent-not-found',
          `consent request ${requestId} is unknown or already resolved`,
          'errors.consentNotFound',
        );
      }
      this.pendingConsents.delete(requestId);
      pending.resolve(allow);
      await latency(120);
    },

    // ---- Lane C: the observability surface (derived from the runtime,
    // never fabricated beyond what the simulator itself knows) ----------

    getScanTelemetry: async (scanId: string): Promise<ScanTelemetry> => {
      await latency(40, 20);
      const rt = this.requireScan(scanId);
      const stageOf = (name: ScanStage): StageTelemetry => {
        const idx = rt.stages.indexOf(name);
        const curIdx = rt.stages.indexOf(rt.stage);
        if (idx >= 0 && curIdx > idx) {
          return {
            permille: 1000,
            unitsDone: 1,
            unitsTotal: 1,
            bytesDone: rt.bytesTotal,
            elapsedMs: 0,
            completed: true,
          };
        }
        if (rt.stage === name) {
          return {
            permille: Math.round(rt.stageProgress * 1000),
            unitsDone: 1,
            unitsTotal: 1,
            bytesDone: Math.round(rt.bytesScanned * rt.stageProgress),
            elapsedMs: Math.round(rt.tick * 80),
            completed: false,
          };
        }
        return {
          permille: 0,
          unitsDone: 0,
          unitsTotal: 1,
          bytesDone: 0,
          elapsedMs: 0,
          completed: false,
        };
      };
      const terminal =
        rt.status === 'completed' || rt.status === 'cancelled' || rt.status === 'failed';
      return {
        scanId,
        overallPermille: Math.round(rt.overall * 1000),
        running: rt.status === 'running',
        paused: rt.status === 'paused',
        cancelled: rt.status === 'cancelled',
        elapsedMs: Math.round(rt.tick * 80),
        filesFound: rt.filesFound,
        deletedFound: Math.round(rt.filesFound * 0.72),
        carvedFound: Math.round(rt.filesFound * 0.14),
        lostPartitions: rt.lostCandidates.length,
        bytesScanned: rt.bytesScanned,
        bytesFound: rt.dataFound,
        errorRanges: rt.errors,
        stages: {
          quick: stageOf('quick-index'),
          deep: stageOf('deep-walk'),
          carve: stageOf('carve'),
        },
        ...(terminal
          ? {
              finalStats: {
                filesFound: rt.filesFound,
                deletedFound: Math.round(rt.filesFound * 0.72),
                carvedFound: Math.round(rt.filesFound * 0.14),
                lostPartitions: rt.lostCandidates.length,
                bytesScanned: rt.bytesScanned,
                bytesUnreadable: rt.badSectors.reduce((a, r) => a + r.sizeBytes, 0),
                elapsedMs: Math.round(rt.tick * 80),
                errorRanges: rt.errors,
                checkRan: true,
                checkErrors: this.scenario === 'bad-sectors' ? 2 : 0,
                repairRan: false,
                repairApplied: 0,
                autosaved: rt.autosaved,
              },
              statsAvailable: true,
              checkpoints: 3,
              checkpointsAvailable: true,
            }
          : { statsAvailable: false, checkpointsAvailable: false }),
      };
    },

    getScanAssistantStats: async (scanId: string): Promise<ScanAssistantStats> => {
      await latency(60, 30);
      const rt = this.requireScan(scanId);
      if (rt.status !== 'completed') {
        throw connError(
          'scan-not-running',
          'assistant statistics exist at scan completion (the grading pass)',
          'errors.scanNotRunning',
        );
      }
      const counts = rt.space.categoryCounts(rt.scopeMask);
      // FileCategoryId ('graphics') -> the assistant's category axis
      // ('image' — cr_ui.h's RecoveryAssistant slot order).
      const image = counts.graphics ?? 0;
      const raw = rt.space.rawCount(rt.scopeMask);
      const total = rt.filesFound;
      const smallResult = total < 100;
      // The og's rule: matched is FORCED to 0 under total < 100 (the
      // gauge never shows) — mirrored, never invented.
      const matched = smallResult ? 0 : Math.round(total * 0.82);
      const categories = {
        image,
        video: counts.video ?? 0,
        audio: counts.audio ?? 0,
        document: counts.document ?? 0,
        archive: counts.archive ?? 0,
        other: counts.other ?? 0,
        email: counts.email ?? 0,
        bookmarks: counts.bookmarks ?? 0,
      } satisfies Record<AssistantCategoryKey, number>;
      return {
        scanId,
        totalFileCount: total,
        rawCount: raw,
        lostPartitionExistFileCount: 0,
        lostPartitionDeleteFileCount: rt.lostCandidates.length,
        quickScanExistFileCount: Math.round(total * 0.28),
        quickScanDeleteFileCount: Math.round(total * 0.58),
        deepScanFileCount: Math.round(total * 0.14),
        quickScanDeleteExtend: 0,
        categoryCounts: categories,
        matched,
        total,
        smallResult,
        gradedCount: total,
      };
    },

    listCarveFormats: async (): Promise<readonly CarveFormatRow[]> => {
      await latency(30, 10);
      return MOCK_CARVE_CATALOG;
    },

    getCarveTally: async (scanId: string): Promise<readonly CarveTallyRow[]> => {
      await latency(40, 20);
      const rt = this.requireScan(scanId);
      const extCounts = rt.space.carveTally(rt.scopeMask);
      const rows: CarveTallyRow[] = [];
      for (const f of MOCK_CARVE_CATALOG) {
        rows.push({
          formatIndex: f.index,
          layer: f.layer,
          ext: f.ext,
          userSigns: false,
          count: extCounts[f.ext] ?? 0,
        });
      }
      const userTotal = this.userSigns.reduce((a, s) => a + (extCounts[s.ext] ?? 0), 0);
      rows.push({
        formatIndex: null,
        layer: null,
        ext: null,
        userSigns: true,
        count: userTotal,
      });
      return rows;
    },

    getDiskMap: async (scanId: string, options?: DiskMapOptions): Promise<DiskMapPayload> => {
      await latency(50, 25);
      const rt = this.requireScan(scanId);
      const total = options?.totalBytes ?? rt.bytesTotal;
      const start = options?.startByte ?? 0;
      const maxCells = Math.min(Math.max(options?.maxCells ?? 512, 64), 65536);
      const cellBytes = Math.max(
        options?.cellBytes ?? Math.ceil(total / maxCells / 65536) * 65536,
        65536,
      );
      const cellCount = Math.min(Math.ceil(total / cellBytes), 65536);
      // Derive the overlays from the simulator's own truth: coverage from
      // the scan fraction, allocation from a deterministic fill, found
      // from file density, confidence from the grade mix. Deterministic
      // per cell index — never random per call.
      const covered = Math.round(cellCount * rt.overall);
      const fill = Math.min(0.85, Math.max(0.1, rt.filesFound / 4000));
      const allocation: number[] = [];
      const coverage: number[] = [];
      const found: number[] = [];
      const confidence: number[] = [];
      for (let i = 0; i < cellCount; i++) {
        const scanned = i < covered ? 1 : 0;
        coverage.push(scanned);
        const used = (i * 2654435761) % 1000 < fill * 1000 ? 1 : 2;
        allocation.push(used);
        found.push(scanned && used === 1 ? ((i * 40503) % 7) + 1 : 0);
        confidence.push(scanned && used === 1 ? ((i * 12289) % 3) + 2 : 0);
      }
      const badRanges = rt.badSectors.filter(
        (r) => r.startByte + r.sizeBytes > start && r.startByte < start + total,
      );
      const geometry: DiskMapGeometry = {
        startByte: start,
        totalBytes: total,
        cellBytes,
        cellCount,
        clusterBytes: 4096,
        fsFamily: 1,
      };
      return { scanId, geometry, allocation, coverage, found, confidence, badRanges };
    },
  };

  private requireScan(scanId: string): ScanRuntime {
    const rt = this.scans.get(scanId);
    if (!rt) throw connError('scan-not-found', `scan ${scanId} not found`);
    return rt;
  }

  private makeBadSectorMap(totalBytes: number): BadSectorRange[] {
    const rng = mulberry32(this.seed ^ 0xbad5e);
    const ranges: BadSectorRange[] = [];
    let at = randInt(rng, 0, Math.floor(totalBytes * 0.3));
    for (let i = 0; i < 14; i++) {
      const size = randInt(rng, 512 * 200, 512 * 40_000);
      ranges.push({ startByte: at, sizeBytes: size });
      at += size + randInt(rng, 512 * 1000, 512 * 900_000);
    }
    return ranges;
  }

  private visibleFraction(rt: ScanRuntime): number {
    return rt.status === 'completed' || rt.status === 'cancelled'
      ? 1
      : Math.max(0.05, 1 - Math.exp(-3.4 * rt.overall));
  }

  /**
   * Lane C: the imaging tick — advances the byte stream at the runtime's
   * throughput (scaled down so a simulated multi-GB image completes in
   * seconds, exactly like the scan/recovery tick engines), emits the
   * coalesced progress, and latches the terminal state (the single-writer
   * rule mirrored).
   */
  private imageTick(rt: ImageRuntime): void {
    rt.tick++;
    // The simulated clock: ~90 MB/s at 90 ms ticks would take minutes for
    // a disk-sized total — scale to a ~4 s run (44 ticks), the same
    // compression the scan engine applies to its stages.
    const isTest = import.meta.env?.MODE === 'test';
    const perTick = Math.ceil(rt.bytesTotal / (isTest ? 12 : 44));
    rt.bytesDone = Math.min(rt.bytesTotal, rt.bytesDone + perTick);
    const frac = rt.bytesTotal > 0 ? rt.bytesDone / rt.bytesTotal : 0;
    if (rt.bytesDone >= rt.bytesTotal) {
      rt.status = 'completed';
      rt.finishedAt = new Date().toISOString();
      if (rt.timer) {
        clearInterval(rt.timer);
        rt.timer = null;
      }
    }
    this.emitTo(this.imageProgressListeners, rt.imageId, {
      imageId: rt.imageId,
      status: rt.status,
      bytesDone: rt.bytesDone,
      bytesTotal: rt.bytesTotal,
      overallProgress: rt.status === 'completed' ? 1 : frac,
      throughputBytesPerSecond: rt.throughput,
      etaSeconds:
        rt.status === 'running'
          ? Math.round((rt.bytesTotal - rt.bytesDone) / rt.throughput)
          : rt.status === 'completed'
            ? 0
            : null,
    });
    if (rt.status === 'completed') {
      this.emitTo(this.imageFinishedListeners, rt.imageId, {
        imageId: rt.imageId,
        status: rt.status,
        partialKept: false,
      });
    }
  }

  private scanTick(rt: ScanRuntime): void {
    rt.tick++;
    const stages = rt.stages;
    const curIdx = stages.indexOf(rt.stage);
    const totalTicks = 96 + (rt.space.fileCount > 500_000 ? 60 : 0);

    // per-tick increments (faster in automated test runner while preserving every stage transition)
    const isTest = import.meta.env?.MODE === 'test';
    const stageSpeed = isTest ? 0.22 : 0.014 + (rt.tick % 5 === 0 ? 0.006 : 0);
    rt.stageProgress = Math.min(1, rt.stageProgress + stageSpeed);

    // device disconnect injection (one-shot per scan: reconnects stick)
    if (
      this.scenario === 'disconnect-mid-scan' &&
      !rt.disconnectHandled &&
      !rt.disconnectFired &&
      rt.overall > 0.38
    ) {
      rt.disconnectFired = true;
      rt.disconnectHandled = true;
      rt.status = 'paused';
      if (rt.timer) clearInterval(rt.timer);
      rt.timer = null;
    }

    // bad-sector error accumulation
    if (this.scenario === 'bad-sectors' && rt.tick % 9 === 0) {
      rt.errors += 1;
    }

    // bytes/throughput
    const byteSpeed = (rt.bytesTotal / totalTicks) * (0.8 + ((rt.tick * 37) % 40) / 100);
    rt.bytesScanned = Math.min(rt.bytesTotal, rt.bytesScanned + byteSpeed);
    rt.throughput = byteSpeed * 8;
    rt.eta = Math.max(1, Math.round(((totalTicks - rt.tick) * 150) / 1000));

    // overall from active stage weights (normalized so omitting 'quick-index' still spans 0..1)
    const activeWeightTotal = stages.reduce((acc, s) => acc + (STAGE_WEIGHTS[s] ?? 0), 0) || 1;
    let before = 0;
    for (let i = 0; i < curIdx; i++) {
      before += (STAGE_WEIGHTS[stages[i]] ?? 0) / activeWeightTotal;
    }
    const curWeight = (STAGE_WEIGHTS[rt.stage] ?? 0) / activeWeightTotal;
    rt.overall = Math.min(1, before + curWeight * rt.stageProgress);

    if (rt.stageProgress >= 1) {
      const next = stages[curIdx + 1];
      if (next) {
        rt.stage = next;
        rt.stageProgress = 0;
        // W-01: entering the carve stage surfaces the lost-partition
        // volume's OWN fixture geometry as a candidate event (real
        // fixture data, never synthetic).
        if (
          next === 'carve' &&
          rt.lostCandidates.length === 0 &&
          !rt.params.targetId.startsWith('disk-')
        ) {
          const lost = (rt.target as Volume).lostPartition;
          if (lost) {
            // fs: the engine's FsKindCode (cr_types.h) — the fixture's
            // probableFs 'ntfs' is Ntfs = 3; score is the engine's 0-100
            // confidence (a strong boot-sector match).
            const fsCode = lost.probableFs === 'ntfs' ? 3 : 7;
            const row: LostCandidateRow = {
              lba: Math.floor(lost.startByte / 512),
              sectorCount: Math.floor(lost.sizeBytes / 512),
              fs: fsCode,
              score: 85,
            };
            rt.lostCandidates.push(row);
            this.emitTo(this.lostCandidateListeners, rt.scanId, {
              lba: row.lba,
              sectorCount: row.sectorCount,
              fs: row.fs,
              score: row.score,
              count: rt.lostCandidates.length,
            });
          }
        }
      } else {
        rt.stage = 'done';
        rt.status = 'completed';
        rt.completedAt = new Date().toISOString();
        rt.overall = 1;
        rt.bytesScanned = rt.bytesTotal;
        rt.eta = 0;
        if (rt.timer) clearInterval(rt.timer);
        rt.timer = null;
      }
    }

    // found-file curve: fast then saturating during scan, exact 100% upon completion
    const stats = rt.space.stats(rt.scopeMask);
    if (rt.status === 'completed') {
      rt.filesFound = stats.files;
      rt.dataFound = stats.size;
      this.autoSaveSession(rt);
      rt.autosaved = true;
      // W-01: the terminal + auto-save events, fired by the same
      // transition the bridge's run thread drives.
      this.emitTo(this.scanAutosavedListeners, rt.scanId, { rc: 0 });
      this.emitTo(this.scanFinishedListeners, rt.scanId, {
        status: 'completed',
        failure: null,
        autosaved: true,
      });
    } else {
      const curve = 1 - Math.exp(-3.4 * rt.overall);
      rt.filesFound = Math.floor(stats.files * curve);
      rt.dataFound = Math.floor(stats.size * curve);
    }

    // Live "Searching: <path>" ticker — the item currently examined
    // (corpus GT-scan §2/§7). Assigned AFTER the stage advance so the
    // ticker and stage are always consistent in a polled snapshot.
    rt.currentPath =
      rt.stage === 'deep-walk' || rt.stage === 'carve'
        ? rt.space.searchingPathAt(rt.tick, rt.scopeMask)
        : null;
  }

  private autoSaveSession(rt: ScanRuntime): void {
    const meta: SessionMeta = {
      id: `sess-${this.nextSessionId}`,
      fileName: `auto_${rt.volumeId}_${new Date().toISOString().replace(/[:.]/g, '-')}.rsf`,
      savedAt: new Date().toISOString(),
      targetLabel: rt.target.label,
      filesFound: rt.filesFound,
      dataFoundBytes: rt.dataFound,
      completed: true,
    };
    this.sessionStore.set(meta.id, meta);
    this.sessionPathToScan.set(meta.fileName, rt.scanId);
    this.nextSessionId++;
  }

  private toProgress(rt: ScanRuntime): ScanProgressEvent {
    return {
      scanId: rt.scanId,
      status: rt.status,
      stage: rt.stage,
      stageProgress: rt.stageProgress,
      overallProgress: rt.overall,
      bytesScanned: rt.bytesScanned,
      bytesTotal: rt.bytesTotal,
      filesFound: rt.filesFound,
      dataFoundBytes: rt.dataFound,
      errors: rt.errors,
      badSectorMap: rt.badSectors,
      etaSeconds: rt.eta,
      throughputBytesPerSecond: rt.throughput,
      recoverableDuringScan:
        rt.stage === 'tree-build' || rt.stage === 'deep-walk'
          ? rt.overall > 0.15
          : rt.overall > 0.4,
      deviceConnected: !rt.disconnectFired,
      currentPath: rt.currentPath,
      // P-TICK-01 parity: the mock simulates the cursor with its own
      // bytesScanned curve during the byte-denominated stages (the real
      // connector reads telemetry's current_offset_bytes — same shape,
      // same null-when-not-sweeping rule).
      currentSector:
        rt.stage === 'deep-walk' || rt.stage === 'carve'
          ? Math.floor(rt.bytesScanned / (rt.target.sectorSize ?? 512))
          : null,
      currentSectorTotal:
        rt.stage === 'deep-walk' || rt.stage === 'carve'
          ? Math.floor(rt.bytesTotal / (rt.target.sectorSize ?? 512))
          : null,
      sectorSize: rt.target.sectorSize ?? 512,
    };
  }

  private toScanState(rt: ScanRuntime): ScanState {
    const pending = Array.from(this.pendingConsents.values()).find(
      (p) => p.event.scanId === rt.scanId,
    );
    return {
      scanId: rt.scanId,
      params: rt.params,
      target: rt.target,
      status: rt.status,
      startedAt: rt.startedAt,
      completedAt: rt.completedAt,
      progress: this.toProgress(rt),
      pendingConsent: pending?.event ?? null,
      lostCandidates: {
        count: rt.lostCandidates.length,
        rows: [...rt.lostCandidates],
      },
      autosaved: rt.autosaved,
    };
  }

  // ------------------------------------------------------------ results

  readonly results = {
    getResultTree: async (
      scanId: string,
      parentId: string | null,
      mode: ResultTreeMode,
    ): Promise<TreePage> => {
      await latency(60, 30);
      const rt = this.requireScan(scanId);
      let nodes: TreeNode[];
      if (mode === 'type') nodes = rt.space.typeTree(rt.scopeMask, parentId);
      else if (mode === 'raw') nodes = rt.space.rawTree(rt.scopeMask);
      else nodes = rt.space.pathTreeChildren(parentId, rt.scopeMask);
      const vf = this.visibleFraction(rt);
      if (vf < 0.999) {
        nodes = nodes.map((n) => ({
          ...n,
          fileCount: Math.floor(n.fileCount * vf),
          sizeBytes: Math.floor(n.sizeBytes * vf),
        }));
      }
      return { nodes, parentHasMore: false };
    },

    queryResults: async (
      scanId: string,
      filter: ResultsFilter,
      sort: { field: SortField; direction: SortDirection },
      page: { offset: number; limit: number },
    ): Promise<ResultsPage> => {
      await latency(90, 50);
      const rt = this.requireScan(scanId);
      // During a live scan only a fraction of the space is "found" yet.
      const visibleFraction = this.visibleFraction(rt);
      const result = rt.space.query(filter, sort, page.offset, page.limit, rt.scopeMask);
      // approximate live fraction by trimming the total (deterministic)
      const total = Math.floor(result.total * visibleFraction);
      const totalBytes = Math.floor(result.totalBytes * visibleFraction);
      const nodes =
        visibleFraction >= 0.999
          ? result.nodes
          : result.nodes.slice(0, Math.ceil(result.nodes.length * visibleFraction));
      return {
        nodes,
        total,
        totalBytes,
        offset: page.offset,
        pageSize: page.limit,
        elapsedMs: result.elapsedMs,
      };
    },

    getNodeDetail: async (scanId: string, nodeId: string): Promise<NodeDetail> => {
      await latency(50, 25);
      const rt = this.requireScan(scanId);
      const idx = Number(nodeId.replace(/^f/, ''));
      const node = rt.space.nodeAt(idx);
      const reasonMap: Record<string, string> = {
        excellent: 'Headers and extents fully readable',
        good: 'Slight fragmentation, all clusters readable',
        fair: 'Some clusters marked pending reallocation',
        poor: 'Overlaps an unreadable area',
        unknown: 'Not yet verified',
      };
      return {
        node,
        fullPhysicalPath: node.path ? `${node.path}\\${node.name}` : null,
        createdAt: node.modifiedAt,
        accessedAt: new Date(
          new Date(node.modifiedAt ?? Date.now()).getTime() + 86400_000,
        ).toISOString(),
        confidenceReason: reasonMap[node.confidence] ?? null,
        badSectorsOverlap: node.confidence === 'poor',
        clusterCount: Math.max(1, Math.ceil(node.sizeBytes / 4096)),
      };
    },

    countResults: async (scanId: string, filter: ResultsFilter): Promise<number> => {
      await latency(60, 30);
      const rt = this.requireScan(scanId);
      return Math.floor(rt.space.count(filter, rt.scopeMask) * this.visibleFraction(rt));
    },

    regradeNodes: async (
      scanId: string,
      nodeIds: readonly string[],
    ): Promise<{ regraded: number }> => {
      await latency(90, 40);
      const rt = this.requireScan(scanId);
      // Session-tree semantics mirrored: only grade-1 ("average") nodes
      // are eligible; the simulator's grade model is the node-space's own.
      let regraded = 0;
      for (const id of nodeIds) {
        if (rt.space.regradeNode(id)) regraded += 1;
      }
      return { regraded };
    },
  };

  // ------------------------------------------------------------ preview

  readonly preview = {
    requestPreview: async (scanId: string, nodeId: string): Promise<PreviewPayload> => {
      await latency(240, 160);
      const rt = this.requireScan(scanId);
      const idx = Number(nodeId.replace(/^f/, ''));
      const node = rt.space.nodeAt(idx);
      const rng = mulberry32((idx ^ 0x5eed) >>> 0);
      // The connector owns file knowledge: kind inference from the node's
      // own category/extension, mirroring the engine's per-type previewer
      // dispatch (artifact 10 §2).
      const kind = previewKindOf(node);

      if (kind === 'image') {
        if (node.extension === '.crypt')
          throw connError('preview-encrypted', 'encrypted', 'preview.reason.encrypted');
        if (idx % 97 === 0)
          throw connError('preview-damaged', 'damaged', 'preview.reason.damaged');
        const { dataUrl, width, height } = this.syntheticImage(rng, node.name);
        return { kind: 'image', dataUrl, width, height, bounded: false };
      }
      if (kind === 'hex') {
        if (node.sizeBytes > 64 * 1024 ** 3)
          throw connError('preview-too-large', 'too large', 'preview.reason.tooLarge');
        const cap = Math.min(node.sizeBytes, 64 * 1024);
        const bytes = new Uint8Array(cap);
        const brng = mulberry32(idx ^ 0xbeef);
        for (let i = 0; i < cap; i++) bytes[i] = Math.floor(brng() * 256);
        return { kind: 'hex', bytes, totalSize: node.sizeBytes, baseOffset: 0 };
      }
      if (kind === 'text') {
        const lines: string[] = [];
        const trng = mulberry32(idx ^ 0x7e7e);
        const words = [
          'recovered',
          'fragment',
          'volume',
          'sector',
          'index',
          'journal',
          'entry',
          'cluster',
          'stream',
          'buffer',
        ];
        for (let l = 0; l < 220; l++) {
          const parts: string[] = [];
          for (let w = 0; w < 10; w++) parts.push(words[Math.floor(trng() * words.length)]);
          lines.push(`${String(l).padStart(4, ' ')}  ${parts.join(' ')}`);
        }
        return {
          kind: 'text',
          text: lines.join('\n'),
          encoding: idx % 7 === 0 ? 'UTF-16LE' : 'UTF-8',
          totalSize: node.sizeBytes,
          truncated: node.sizeBytes > 16_000,
        };
      }
      if (kind === 'pdf') {
        const pages = 1 + (idx % 3);
        const pageImages = Array.from({ length: pages }, (_, p) =>
          this.syntheticPage(rng, p + 1, pages),
        );
        return { kind: 'pdf', pages: pageImages, pageCount: pages };
      }
      if (kind === 'av') {
        const dur = 30 + Math.floor(rng() * 240);
        const thumb = this.syntheticImage(rng, node.name);
        return {
          kind: 'av',
          metadata: {
            container: node.extension === '.mp4' ? 'ISO-BMFF' : 'Matroska',
            videoCodec: 'H.264',
            audioCodec: 'AAC-LC',
            width: String(1280 + (idx % 3) * 640),
            height: '720',
            fps: '30',
            durationSeconds: String(dur),
          },
          thumbnailDataUrl: thumb.dataUrl,
          durationSeconds: dur,
        };
      }
      // office
      const textRng = mulberry32(idx ^ 0x0ff1ce);
      const sentences = [
        'Quarterly summary of recovered assets.',
        'The content below was extracted from the document stream.',
        'Tables and images appear as placeholders in extracted text.',
        'Section headers and paragraphs are preserved in reading order.',
      ];
      const text = Array.from(
        { length: 24 },
        (_, i) => sentences[Math.floor(textRng() * sentences.length)] + ` (#${i + 1})`,
      ).join(' ');
      return {
        kind: 'office',
        metadata: {
          app: 'WordProcessor',
          author: 'recovered-stream',
          pages: String(2 + (idx % 9)),
          words: String(400 + (idx % 1500)),
        },
        extractedText: text,
        truncated: node.sizeBytes > 128_000,
      };
    },

    getThumbnail: async (
      scanId: string,
      nodeId: string,
    ): Promise<{ dataUrl: string } | null> => {
      await latency(120, 60);
      const rt = this.requireScan(scanId);
      const idx = Number(nodeId.replace(/^f/, ''));
      const node = rt.space.nodeAt(idx);
      if (!node.thumbnailAvailable) return null;
      const rng = mulberry32((idx ^ 0x7777) >>> 0);
      return { dataUrl: this.syntheticImage(rng, node.name).dataUrl };
    },
  };

  private syntheticImage(
    rng: () => number,
    name: string,
  ): { dataUrl: string; width: number; height: number } {
    const width = 640;
    const height = 400;
    const hue = Math.floor(rng() * 360);
    const hue2 = (hue + 40 + Math.floor(rng() * 120)) % 360;
    const shapes: string[] = [];
    for (let i = 0; i < 7; i++) {
      const x = Math.floor(rng() * width);
      const y = Math.floor(rng() * height);
      const r = 20 + Math.floor(rng() * 120);
      const c = `hsl(${Math.floor(rng() * 360)} 70% ${45 + Math.floor(rng() * 25)}%)`;
      shapes.push(`<circle cx="${x}" cy="${y}" r="${r}" fill="${c}" opacity="0.55"/>`);
    }
    const label = name.replace(/[<>&"']/g, '');
    const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}">
<defs><linearGradient id="g" x1="0" y1="0" x2="1" y2="1">
<stop offset="0" stop-color="hsl(${hue} 80% 62%)"/><stop offset="1" stop-color="hsl(${hue2} 75% 38%)"/>
</linearGradient></defs>
<rect width="${width}" height="${height}" fill="url(#g)"/>
${shapes.join('')}
<rect x="16" y="${height - 56}" width="${width - 32}" height="40" rx="8" fill="rgba(0,0,0,0.45)"/>
<text x="28" y="${height - 30}" font-family="Segoe UI, sans-serif" font-size="18" fill="#fff">${label.slice(0, 42)}</text>
</svg>`;
    return {
      dataUrl: `data:image/svg+xml;base64,${btoa(unescape(encodeURIComponent(svg)))}`,
      width,
      height,
    };
  }

  private syntheticPage(
    rng: () => number,
    page: number,
    total: number,
  ): { dataUrl: string; width: number; height: number } {
    const width = 595;
    const height = 842;
    const lines = Array.from({ length: 18 }, (_, i) => {
      const w = 120 + Math.floor(rng() * 380);
      return `<rect x="70" y="${120 + i * 28}" width="${w}" height="12" rx="4" fill="rgba(30,40,70,0.35)"/>`;
    });
    const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}" viewBox="0 0 ${width} ${height}">
<rect width="${width}" height="${height}" fill="#ffffff"/>
<rect x="0" y="0" width="${width}" height="8" fill="#0560fd"/>
<text x="70" y="80" font-family="Segoe UI, sans-serif" font-size="26" fill="#1a1a1a">Page ${page} of ${total}</text>
${lines.join('')}
</svg>`;
    return {
      dataUrl: `data:image/svg+xml;base64,${btoa(unescape(encodeURIComponent(svg)))}`,
      width,
      height,
    };
  }

  // ------------------------------------------------------------ sessions

  readonly sessions = {
    cancelSave: async (_scanId: string): Promise<void> => {
      // TEST DOUBLE: the save is synchronous here — a cancel is a no-op.
      await latency(60);
    },

    resumeLastSession: async (): Promise<{ scanId: string; path: string }> => {
      // TEST DOUBLE: the newest auto-named session is the pointed one (the
      // bridge's last_scan.info analog).
      await latency(220, 80);
      const newest = Array.from(this.sessionStore.values())
        .filter((s) => s.fileName.startsWith('auto_'))
        .sort((a, b) => (b.savedAt ?? '').localeCompare(a.savedAt ?? ''))[0];
      if (!newest) {
        throw connError(
          'session-not-found',
          'no last-scan pointer names a present auto session',
          'errors.sessionNotFound',
        );
      }
      const scanId = this.sessionPathToScan.get(newest.fileName);
      if (!scanId) {
        throw connError(
          'session-not-found',
          'the pointed session is not loadable',
          'errors.sessionNotFound',
        );
      }
      return { scanId, path: newest.fileName };
    },

    resumeSession: async (path: string): Promise<{ scanId: string; targetId: string }> => {
      // TEST DOUBLE: the resume continues the incomplete session's scan on
      // its original target (a fresh run of the same params).
      await latency(500, 150);
      const oldId = this.sessionPathToScan.get(path);
      if (!oldId) {
        throw connError(
          'session-not-found',
          `session ${path} not found`,
          'errors.sessionNotFound',
        );
      }
      const rt = this.requireScan(oldId);
      if (rt.status === 'completed') {
        throw connError(
          'scan-not-running',
          'the session already completed — nothing to resume',
          'errors.scanNotRunning',
        );
      }
      const started = await this.scan.startScan({
        targetId: rt.params.targetId,
        pathScope: rt.params.pathScope,
        bitlockerFvek: rt.params.bitlockerFvek,
      });
      // The targetId rides along so the caller can build the live-scan
      // route (/scan/<targetId>/live?scanId=<id>) — v0.8.2's forensic
      // finding: the backup page navigated to /scan?scanId=, a route the
      // router does not define.
      return { ...started, targetId: rt.params.targetId };
    },

    saveSession: async (scanId: string, path: string | null): Promise<SessionMeta> => {
      // W-01: the save-progress stream — deterministic fractions across
      // the save's own latency window (the bridge's 5%-coalesced stream).
      const fractions = [0.2, 0.45, 0.7, 0.9, 1];
      const stepTimers = fractions.map((fraction, i) =>
        setTimeout(
          () => this.emitTo(this.saveProgressListeners, scanId, { fraction }),
          50 * (i + 1),
        ),
      );
      try {
        await latency(320, 120);
        const rt = this.requireScan(scanId);
        const fileName =
          path ?? `auto_session_${new Date().toISOString().replace(/[:.]/g, '-')}.crsf`;
        const meta: SessionMeta = {
          id: `sess-${this.nextSessionId++}`,
          fileName,
          savedAt: new Date().toISOString(),
          targetLabel: rt.target.label,
          filesFound: rt.filesFound,
          dataFoundBytes: rt.dataFound,
          completed: rt.status === 'completed',
        };
        this.sessionStore.set(meta.id, meta);
        this.sessionPathToScan.set(fileName, scanId);
        // The terminal fraction is synchronous (the intermediate timers can
        // be raced by a short latency window; the stream must always end
        // at 1 when the save resolves).
        this.emitTo(this.saveProgressListeners, scanId, { fraction: 1 });
        return meta;
      } finally {
        for (const t of stepTimers) clearTimeout(t);
      }
    },

    listSessions: async (): Promise<SessionMeta[]> => {
      await latency(120, 60);
      return Array.from(this.sessionStore.values()).sort((a, b) =>
        (b.savedAt ?? '').localeCompare(a.savedAt ?? ''),
      );
    },

    loadSession: async (path: string): Promise<{ scanId: string }> => {
      // W-01: the session-load progress stream — coalesced 5% steps across
      // the load's latency window (the bridge's own coalescing rule).
      const steps = [100, 300, 500, 700, 900, 1000];
      const nodesTotal = 10_000;
      const stepTimers = steps.map((permille, i) =>
        setTimeout(
          () =>
            this.emitTo(this.sessionLoadProgressListeners, path, {
              path,
              permille,
              nodesDone: Math.floor((nodesTotal * permille) / 1000),
              nodesTotal,
            }),
          60 * (i + 1),
        ),
      );
      try {
        await latency(420, 180);
        if (this.scenario === 'corrupt-session' && path.includes('auto_')) {
          throw connError('session-corrupt', 'corrupt session', 'errors.sessionCorrupt');
        }
        const scanId = this.sessionPathToScan.get(path);
        if (!scanId)
          throw connError('session-not-found', 'session not found', 'errors.sessionNotFound');
        // The terminal permille is synchronous (the intermediate timers can
        // be raced by a short latency window; the stream must always end at
        // 1000 when the load resolves).
        this.emitTo(this.sessionLoadProgressListeners, path, {
          path,
          permille: 1000,
          nodesDone: nodesTotal,
          nodesTotal,
        });
        return { scanId };
      } finally {
        for (const t of stepTimers) clearTimeout(t);
      }
    },

    // W-01 (the dead-letter fix): the load-progress subscription.
    onSessionLoadProgress: (
      path: string,
      cb: (event: SessionLoadProgressEvent) => void,
    ): (() => void) => this.subscribeTo(this.sessionLoadProgressListeners, path, cb),

    getSessionMeta: async (path: string): Promise<SessionMeta> => {
      await latency(100, 50);
      const scanId = this.sessionPathToScan.get(path);
      if (!scanId)
        throw connError('session-not-found', 'session not found', 'errors.sessionNotFound');
      const rt = this.requireScan(scanId);
      return {
        id: `sess-${path.length}`,
        fileName: path,
        savedAt: rt.completedAt ?? rt.startedAt,
        targetLabel: rt.target.label,
        filesFound: rt.filesFound,
        dataFoundBytes: rt.dataFound,
        completed: rt.status === 'completed',
      };
    },
  };

  // ------------------------------------------------------------ recovery

  readonly recovery = {
    startRecovery: async (
      scanId: string,
      selection: RecoverySelection,
      destination: string,
      options: RecoveryOptions,
    ): Promise<{ jobId: string }> => {
      await latency(300, 100);
      const rt = this.requireScan(scanId);
      const space = rt.space;
      const items: RecoveryItem[] =
        selection.kind === 'nodes'
          ? selection.nodeIds.map((id) => {
              const idx = Number(id.replace(/^f/, ''));
              const node = space.nodeAt(idx);
              return {
                nodeId: id,
                name: node.name,
                sizeBytes: node.sizeBytes,
                state: 'pending',
              };
            })
          : [];
      const allMode =
        selection.kind === 'all'
          ? {
              filter: selection.filter,
              space,
              scopeMask: rt.scopeMask,
              cursor: 0,
              total: space.count(selection.filter, rt.scopeMask),
            }
          : null;
      const totalCount = items.length + (allMode?.total ?? 0);
      const jobId = `job-${this.nextJobId++}`;
      // Items advanced per tick — scales with job size so even a
      // recover-all over the whole result set finishes in bounded wall time
      // (the real engine streams at disk speed; the simulator batches).
      const targetTicks =
        import.meta.env?.MODE === 'test' && selection.kind === 'all' ? 25 : 200;
      const batch = Math.max(1, Math.ceil(Math.max(1, totalCount) / targetTicks));
      const job: JobRuntime = {
        jobId,
        scanId,
        destination,
        options,
        items,
        allMode,
        status: 'running',
        progress: {
          jobId,
          status: 'running',
          overallProgress: 0,
          currentFile: null,
          currentFileProgress: 0,
          completedCount: 0,
          failedCount: 0,
          skippedCount: 0,
          totalCount: Math.max(1, totalCount),
          throughputBytesPerSecond: 0,
          verifying: false,
          etaSeconds: null,
        },
        timer: null,
        startedAt: new Date().toISOString(),
        finishedAt: null,
        bytesWritten: 0,
        verified: options.verifyOnWrite,
        batch,
        advanceCursor: 0,
      };
      this.jobs.set(jobId, job);
      job.timer = setInterval(() => this.jobTick(job), 120);
      return { jobId };
    },

    cancelRecovery: async (jobId: string): Promise<void> => {
      const job = this.requireJob(jobId);
      if (job.timer) clearInterval(job.timer);
      job.timer = null;
      job.status = 'cancelled';
      job.finishedAt = new Date().toISOString();
      // W-01: the terminal event (mirrors recovery.rs's recovery-finished).
      this.emitTo(this.recoveryFinishedListeners, jobId, { status: 'cancelled' });
      await latency(200);
    },

    getRecoveryState: async (jobId: string): Promise<RecoveryJob> => {
      await latency(30);
      const job = this.requireJob(jobId);
      return {
        jobId: job.jobId,
        scanId: job.scanId,
        nodeIds: job.allMode ? [] : job.items.map((i) => i.nodeId),
        destination: job.destination,
        options: job.options,
        status: job.status,
      };
    },

    getRecoveryItems: async (
      jobId: string,
      page: { offset: number; limit: number },
    ): Promise<readonly RecoveryItem[]> => {
      await latency(30);
      const job = this.requireJob(jobId);
      return job.items.slice(page.offset, page.offset + page.limit);
    },

    onRecoveryProgress: (
      jobId: string,
      cb: (event: RecoveryProgressEvent) => void,
    ): (() => void) => {
      const handler = (): void => {
        const job = this.jobs.get(jobId);
        if (job) cb({ ...job.progress });
      };
      const interval = setInterval(handler, 150);
      return () => clearInterval(interval);
    },

    // W-01 (the dead-letter fix): the recovery terminal event.
    onRecoveryFinished: (
      jobId: string,
      cb: (event: RecoveryFinishedEvent) => void,
    ): (() => void) => this.subscribeTo(this.recoveryFinishedListeners, jobId, cb),

    getRecoveryReport: async (jobId: string): Promise<RecoveryReport> => {
      await latency(40);
      const job = this.requireJob(jobId);
      const cap = 500;
      return {
        jobId: job.jobId,
        status: job.status,
        startedAt: job.startedAt,
        finishedAt: job.finishedAt,
        succeeded: job.items.filter((i) => i.state === 'done').slice(0, cap),
        failed: job.items.filter((i) => i.state === 'failed').slice(0, cap),
        skipped: job.items.filter((i) => i.state === 'skipped').slice(0, cap),
        totalBytesWritten: job.bytesWritten,
        verified: job.verified,
        processedTotal:
          job.progress.completedCount + job.progress.failedCount + job.progress.skippedCount,
        truncatedItemLists: job.allMode != null && job.items.length > cap,
      };
    },
  };

  private requireJob(jobId: string): JobRuntime {
    const job = this.jobs.get(jobId);
    if (!job) throw connError('unknown', `job ${jobId} not found`);
    return job;
  }

  private jobTick(job: JobRuntime): void {
    // Recover-all: materialize the next chunk of matching nodes on demand
    // (bounded working set — the export loop streams, it does not preload).
    if (job.allMode) {
      // Lookahead window: keep a bounded working set of materialized items
      // (the export loop streams; it never preloads the whole set).
      const windowSize = Math.max(2048, job.batch * 2);
      const inFlight = job.items.length - job.advanceCursor;
      if (inFlight < windowSize) {
        const { space, filter, scopeMask } = job.allMode;
        const window = space.queryNodes(filter, scopeMask, job.allMode.cursor, windowSize);
        if (window.length > 0) {
          job.allMode.cursor += window.length;
          for (const idx of window) {
            const node = space.nodeAt(idx);
            job.items.push({
              nodeId: `f${idx}`,
              name: node.name,
              sizeBytes: node.sizeBytes,
              state: 'pending',
            });
          }
        }
      }
    }

    let pendingIdx = -1;
    for (let k = job.advanceCursor; k < job.items.length; k++) {
      if (job.items[k].state === 'pending' || job.items[k].state === 'running') {
        pendingIdx = k;
        break;
      }
    }
    if (pendingIdx === -1) {
      const failed = job.progress.failedCount;
      const done = job.progress.completedCount;
      if (
        job.options.verifyOnWrite &&
        !job.progress.verifying &&
        done > 0 &&
        job.progress.completedCount === done
      ) {
        job.progress.verifying = true;
        return;
      }
      if (job.progress.verifying) {
        // verification pass ends the job
        job.status = failed > 0 ? 'completed-partial' : 'completed';
        job.finishedAt = new Date().toISOString();
        if (job.timer) clearInterval(job.timer);
        job.timer = null;
        job.progress.status = job.status;
        job.progress.verifying = false;
        this.emitTo(this.recoveryFinishedListeners, job.jobId, { status: job.status });
        return;
      }
      job.status = failed > 0 ? 'completed-partial' : 'completed';
      job.finishedAt = new Date().toISOString();
      if (job.timer) clearInterval(job.timer);
      job.timer = null;
      job.progress.status = job.status;
      this.emitTo(this.recoveryFinishedListeners, job.jobId, { status: job.status });
      return;
    }

    // Advance a scale-aware batch per tick so any job (including a
    // recover-all over the entire result set) completes in bounded wall
    // time — the real engine streams at disk speed; the simulator batches.
    for (let step = 0; step < job.batch; step++) {
      while (
        job.advanceCursor < job.items.length &&
        job.items[job.advanceCursor].state !== 'pending' &&
        job.items[job.advanceCursor].state !== 'running'
      ) {
        job.advanceCursor++;
      }
      if (job.advanceCursor >= job.items.length) break;
      const pending = job.advanceCursor;
      const item = job.items[pending];
      if (item.state === 'pending') {
        item.state = 'running';
        job.progress.currentFile = item.name;
        job.progress.currentFileProgress = 0;
        if (job.batch > 1) {
          // Batched (at-scale) jobs settle one item per step — the per-file
          // progress animation only exists for small explicit selections.
          job.progress.currentFileProgress = 1;
        } else {
          continue;
        }
      }
      job.progress.currentFileProgress = Math.min(1, job.progress.currentFileProgress + 0.25);
      if (job.progress.currentFileProgress >= 1) {
        const roll = mulberry32(item.nodeId.length + pending)();
        if (roll < 0.04) {
          item.state = 'failed';
          item.error = {
            code: 'recovery-item-failed',
            reasonKey: 'recovery.itemFailedReason',
          };
          job.progress.failedCount++;
        } else if (job.options.collision === 'skip' && roll > 0.92) {
          item.state = 'skipped';
          job.progress.skippedCount++;
        } else {
          item.state = 'done';
          item.savedAs =
            job.options.collision === 'rename' && roll > 0.88
              ? item.name.replace(/(\.[^.]+)$/, ' (1)$1')
              : item.name;
          job.progress.completedCount++;
          job.bytesWritten += item.sizeBytes;
        }
        job.progress.currentFile = null;
        job.progress.currentFileProgress = 0;
        job.advanceCursor = pending + 1;
      }
    }
    job.progress.overallProgress =
      (job.progress.completedCount +
        job.progress.failedCount +
        job.progress.skippedCount +
        job.progress.currentFileProgress) /
      Math.max(1, job.progress.totalCount);
    job.progress.throughputBytesPerSecond = randBell(mulberry32(pendingIdx + 1), 48e6, 30e6);
    // ETA from the job's own batch cadence (tick interval 120 ms) — not a
    // per-item constant, which reads absurd at recover-all scale.
    const remainingTicks = Math.ceil(
      (job.progress.totalCount -
        job.progress.completedCount -
        job.progress.failedCount -
        job.progress.skippedCount) /
        Math.max(1, job.batch),
    );
    job.progress.etaSeconds = Math.max(1, Math.round(remainingTicks * 0.12));
  }

  // ------------------------------------------------------------ health

  readonly health = {
    getHealth: async (deviceId: string): Promise<DiskHealth> => {
      await latency(280, 120);
      const device = this.deviceList.find((d) => d.id === deviceId);
      if (!device) throw connError('device-gone', 'device not found', 'errors.deviceGone');
      const rng = mulberry32(this.seed ^ device.serial.length ^ 0xea17);
      const isSsd =
        device.model.toLowerCase().includes('ssd') ||
        device.model.toLowerCase().includes('evo') ||
        device.kind === 'usb';
      const caution = device.id === 'dev-data' || this.scenario === 'bad-sectors';
      const failing = this.scenario === 'failing-disk';
      const temp = Math.round(randBell(rng, 38, 10));
      const hours = Math.round(randBell(rng, 6400, 5200));
      const reallocated = failing
        ? randInt(rng, 1200, 6400)
        : caution
          ? randInt(rng, 12, 480)
          : randInt(rng, 0, 3);
      const pending = failing ? randInt(rng, 800, 3200) : caution ? randInt(rng, 8, 260) : 0;

      const mkAttr = (
        id: number,
        name: string,
        current: number | null,
        worst: number | null,
        threshold: number | null,
        raw: number,
        status: SmartAttribute['status'],
      ): SmartAttribute => ({ id, name, current, worst, threshold, raw, status });

      const attributes: SmartAttribute[] = [
        mkAttr(
          5,
          'Reallocated Sectors Count',
          100 - Math.min(64, Math.floor(reallocated / 8)),
          100 - Math.min(80, Math.floor(reallocated / 4)),
          36,
          reallocated,
          reallocated > 36 ? 'failing' : 'ok',
        ),
        mkAttr(9, 'Power-On Hours', 98, 98, 0, hours, 'ok'),
        mkAttr(
          10,
          'Spin Retry Count',
          isSsd ? null : 100,
          isSsd ? null : 100,
          isSsd ? null : 97,
          isSsd ? 0 : randInt(rng, 0, 2),
          'ok',
        ),
        mkAttr(12, 'Power Cycle Count', 99, 99, 0, Math.round(hours / 42), 'ok'),
        ...(isSsd
          ? [
              mkAttr(
                177,
                'Wear Leveling Count',
                Math.max(1, 100 - Math.round(randBell(rng, 22, 20))),
                100,
                5,
                0,
                'ok',
              ),
              mkAttr(178, 'Used Reserved Blocks', 100, 100, 10, randInt(rng, 0, 14), 'ok'),
            ]
          : []),
        mkAttr(194, 'Temperature', temp, temp, 60, temp, temp > 55 ? 'failing' : 'ok'),
        ...(isSsd
          ? []
          : [
              mkAttr(
                196,
                'Reallocation Event Count',
                100,
                100,
                36,
                reallocated,
                reallocated > 36 ? 'failing' : 'ok',
              ),
            ]),
        mkAttr(
          197,
          'Current Pending Sectors',
          100,
          100,
          36,
          pending,
          pending > 36 ? 'failing' : 'ok',
        ),
        mkAttr(
          198,
          'Uncorrectable Sectors',
          100,
          100,
          36,
          pending > 12 ? randInt(rng, 2, pending) : 0,
          pending > 36 ? 'failing' : 'ok',
        ),
        ...(isSsd
          ? [
              mkAttr(
                231,
                'SSD Life Left',
                Math.max(3, 100 - Math.round(randBell(rng, 28, 24))),
                100,
                10,
                0,
                'ok',
              ),
            ]
          : []),
      ];

      const status: DiskHealth['status'] = failing ? 'bad' : caution ? 'caution' : 'good';
      return {
        deviceId,
        status,
        temperatureC: temp,
        powerOnHours: hours,
        diskClass: isSsd ? 'ssd' : 'hdd',
        wearPercent: isSsd
          ? Math.min(99, Math.max(2, Math.round(randBell(rng, 24, 20))))
          : null,
        trimEnabled: isSsd ? rng() > 0.15 : null,
        reallocatedSectors: reallocated,
        pendingSectors: pending,
        attributes,
        transport: 'ata-passthrough',
        powerCycles: null,
        tableEmpty: false,
      };
    },
  };

  // ------------------------------------------------------------ repair

  readonly repair = {
    dryRunRepair: async (deviceId: string): Promise<RepairDryRun> => {
      await latency(600, 200);
      const device = this.deviceList.find((d) => d.id === deviceId);
      if (!device) throw connError('device-gone', 'device not found', 'errors.deviceGone');
      if (this.scenario === 'bad-sectors' || device.id === 'dev-data') {
        return {
          deviceId,
          findings: [
            {
              id: 'fnd-1',
              kind: 'boot-sector-mismatch',
              where: 'LBA 0 (boot sector)',
              descriptionKey: 'repair.finding.bootSector',
              fixable: true,
              severity: 'warning',
            },
            {
              id: 'fnd-2',
              kind: 'backup-dbr-stale',
              where: 'LBA 32 (backup)',
              descriptionKey: 'repair.finding.backupDbr',
              fixable: true,
              severity: 'info',
            },
            {
              id: 'fnd-3',
              kind: 'fat-table-damaged',
              where: 'FAT region 1',
              descriptionKey: 'repair.finding.fatTable',
              fixable: true,
              severity: 'critical',
            },
          ],
          canApply: true,
        };
      }
      return { deviceId, findings: [], canApply: false };
    },

    applyRepair: async (
      deviceId: string,
      options: { consentToken: string },
    ): Promise<{ applied: number }> => {
      if (options.consentToken !== 'REPAIR') {
        throw connError(
          'repair-consent-required',
          'consent token missing',
          'repair.consentRequired',
        );
      }
      await latency(900, 300);
      const existing = this.repairUndoLogs.get(deviceId) ?? [];
      const entries: RepairUndoEntry[] = [
        {
          index: existing.length + 1,
          action: 'Restore boot sector from backup',
          where: 'LBA 0',
          at: new Date().toISOString(),
          snapshotBytes: 512,
        },
        {
          index: existing.length + 2,
          action: 'Rebuild FAT chain from directory walk',
          where: 'FAT region 1',
          at: new Date().toISOString(),
          snapshotBytes: 131_072,
        },
        {
          index: existing.length + 3,
          action: 'Refresh DBR checksum',
          where: 'LBA 32',
          at: new Date().toISOString(),
          snapshotBytes: 512,
        },
      ];
      this.repairUndoLogs.set(deviceId, [...existing, ...entries]);
      this.repairApplied.set(
        deviceId,
        (this.repairApplied.get(deviceId) ?? 0) + entries.length,
      );
      return { applied: entries.length };
    },

    getUndoLog: async (deviceId: string): Promise<readonly RepairUndoEntry[]> => {
      await latency(150);
      return this.repairUndoLogs.get(deviceId) ?? [];
    },

    undoRepairs: async (deviceId: string): Promise<{ rolledBack: number }> => {
      await latency(700, 250);
      const n = (this.repairUndoLogs.get(deviceId) ?? []).length;
      this.repairUndoLogs.set(deviceId, []);
      return { rolledBack: n };
    },
  };

  // ------------------------------------------------------------ lostPartition
  // TEST DOUBLE: the S-029 apply flow — the consent gate is REAL machinery
  // (the same echo-token + resolve semantics the bridge implements) so the
  // e2e consent spec exercises the true UI flow; the write itself is
  // simulated (a test double never touches a disk).

  readonly lostPartition = {
    applyLostPartition: async (
      deviceId: string,
      candidate: LostCandidateRow,
      rebuildTable?: boolean,
    ): Promise<ApplyResult> => {
      await latency(300, 100);
      const device = this.deviceList.find((d) => d.id === deviceId);
      if (!device) throw connError('device-gone', 'device not found', 'errors.deviceGone');
      const allowed = await this.offerApplyConsent(deviceId, candidate, rebuildTable ?? false);
      if (!allowed) {
        return { applied: false, outcome: 1, undoEntries: 0, reason: 'consent denied' };
      }
      const entries = 1 + (this.seed % 3);
      this.applyUndo.set(deviceId, entries);
      return { applied: true, outcome: 0, undoEntries: entries, reason: '' };
    },

    undoLastApply: async (deviceId: string): Promise<ApplyResult> => {
      await latency(650, 200);
      const entries = this.applyUndo.get(deviceId) ?? 0;
      this.applyUndo.set(deviceId, 0);
      return {
        applied: entries > 0,
        outcome: entries > 0 ? 0 : 2,
        undoEntries: entries,
        reason: entries > 0 ? '' : 'no undo log exists',
      };
    },

    saveApplyUndo: async (deviceId: string, _path: string): Promise<void> => {
      await latency(180, 60);
      if (!this.applyUndo.has(deviceId)) {
        throw connError('unknown', 'no undo log exists', 'errors.unknown');
      }
    },

    onApplyConsent: (
      deviceId: string,
      cb: (event: ApplyConsentEvent) => void,
    ): (() => void) => {
      const set = this.applyConsentListeners.get(deviceId) ?? new Set();
      set.add(cb);
      this.applyConsentListeners.set(deviceId, set);
      return () => {
        set.delete(cb);
      };
    },

    respondToApplyConsent: async (requestId: string, allow: boolean): Promise<void> => {
      const pending = this.pendingApplyConsents.get(requestId);
      if (!pending) {
        throw connError(
          'consent-not-found',
          `consent request ${requestId} is unknown or already resolved`,
          'errors.consentNotFound',
        );
      }
      this.pendingApplyConsents.delete(requestId);
      this.applyConsentResolvers.get(requestId)?.(allow);
      this.applyConsentResolvers.delete(requestId);
      await latency(120);
    },

    pendingApplyConsent: async (deviceId: string): Promise<ApplyConsentEvent | null> => {
      await latency(40);
      return this.pendingApplyConsents.get(`pending:${deviceId}`) ?? null;
    },
  };

  private applyUndo = new Map<string, number>();
  private applyConsentResolvers = new Map<string, (allow: boolean) => void>();

  private offerApplyConsent(
    deviceId: string,
    candidate: LostCandidateRow,
    rebuildTable: boolean,
  ): Promise<boolean> {
    const requestId = `apply-${this.nextConsentId++}`;
    const event: ApplyConsentEvent = {
      deviceId,
      requestId,
      plan: {
        refused: 0,
        scope: rebuildTable ? 1 : 0,
        startLba: candidate.lba,
        sectorCount: candidate.sectorCount ?? 0,
        mbrType: 7,
        slot: 0,
        regenerateSignature: rebuildTable ? 1 : 0,
        signature: 0x5a5a5a5a,
        reason: '',
      },
    };
    return new Promise<boolean>((resolve) => {
      this.applyConsentResolvers.set(requestId, resolve);
      this.pendingApplyConsents.set(requestId, event);
      this.pendingApplyConsents.set(`pending:${deviceId}`, event);
      const listeners = this.applyConsentListeners.get(deviceId);
      if (listeners) for (const cb of listeners) cb(event);
    });
  }

  /** TEST HOOK (e2e consent spec): the newest RUNNING scan's id (the
   * injector's target), or null when nothing runs. */
  lastRunningScanId(): string | null {
    // ISO-8601 startedAt strings compare chronologically.
    let bestId: string | null = null;
    let bestAt = '';
    for (const [id, rt] of this.scans) {
      if (rt.status === 'running' && rt.startedAt > bestAt) {
        bestAt = rt.startedAt;
        bestId = id;
      }
    }
    return bestId;
  }

  /** TEST HOOK (e2e consent spec): inject a pre-scan repair consent request
   * for a running scan — the same payload shape the bridge's run thread
   * emits. Resolves when respondToConsent answers. */
  injectConsentRequest(scanId: string, plan: ConsentRequestEvent['plan']): Promise<boolean> {
    const requestId = `consent-${this.nextConsentId++}`;
    const event: ConsentRequestEvent = { scanId, requestId, plan };
    return new Promise<boolean>((resolve) => {
      this.pendingConsents.set(requestId, { event, resolve });
      const listeners = this.consentListeners.get(scanId);
      if (listeners) for (const cb of listeners) cb(event);
    });
  }

  // ------------------------------------------------------------ system

  readonly system: SystemDomain = {
    getAppInfo: async (): Promise<AppInfo> => {
      await latency(40);
      return {
        name: 'Recovery Wizard',
        version: appVersion,
        connectorMode: 'mock',
        engineVersion: null,
        platform: typeof navigator !== 'undefined' ? navigator.platform : 'unknown',
        userSigns: this.userSigns.length,
      };
    },

    loadUserSigns: async (content: string): Promise<{ count: number }> => {
      // TEST DOUBLE: parse the document's top-level extension keys (the
      // engine's own EUUserData shape) and keep the deterministic set.
      await latency(200, 80);
      let keys: string[] = [];
      try {
        const parsed: unknown = JSON.parse(content);
        if (parsed && typeof parsed === 'object') {
          keys = Object.keys(parsed);
        }
      } catch {
        throw connError(
          'scan-scope-invalid',
          'the signature document is not valid JSON',
          'errors.scanScopeInvalid',
        );
      }
      this.userSigns = keys.map((ext) => ({
        ext,
        offset: 0,
        hex: 'aa',
      }));
      return { count: this.userSigns.length };
    },

    validateDestination: async (
      path: string,
      sourceDeviceId: string,
      neededBytes: number,
    ): Promise<DestinationValidation> => {
      await latency(180, 80);
      // Accept a device OR volume id (scan targets are either); resolve to
      // the owning device for the same-volume comparison.
      const source = this.deviceList.find(
        (d) => d.id === sourceDeviceId || d.volumes.some((v) => v.id === sourceDeviceId),
      );
      const srcLetters = new Set(
        (source?.volumes ?? []).map((v) => v.letter?.toUpperCase()).filter(Boolean) as string[],
      );
      const destLetter = /^[A-Za-z]:/.exec(path)?.[0]?.[0]?.toUpperCase();
      if (destLetter && srcLetters.has(destLetter)) {
        return {
          allowed: false,
          reason: 'overlap',
          reasonKey: 'recovery.overlapReason',
          freeBytes: null,
          neededBytes,
        };
      }
      const mount = this.mountPoints().find((m) =>
        path.toUpperCase().startsWith(m.path[0]?.toUpperCase() ?? ''),
      );
      if (!mount) {
        return {
          allowed: false,
          reason: 'invalid',
          reasonKey: 'errors.unknown',
          freeBytes: null,
          neededBytes,
        };
      }
      if (this.scenario === 'out-of-space' && neededBytes > mount.freeBytes) {
        return {
          allowed: false,
          reason: 'out-of-space',
          reasonKey: 'recovery.insufficientSpaceBody',
          freeBytes: mount.freeBytes,
          neededBytes,
        };
      }
      return { allowed: true, freeBytes: mount.freeBytes, neededBytes };
    },

    pickDirectory: async (hint?: {
      volumeId?: string | null;
      excludeDeviceId?: string | null;
    }): Promise<string | null> => {
      await latency(150);
      // Scan-scope picking: root the suggestion at the hinted volume so the
      // chosen folder resolves on the scanned device.
      if (hint?.volumeId) {
        const hinted = this.mountPoints().find((m) => m.id === hint.volumeId);
        const folder = hinted ? this.nodeSpaces.get(hinted.id)?.suggestedScopePath() : null;
        const mount = hinted ?? this.mountPoints()[0];
        if (mount && folder) return `${mount.path}${folder}`;
      }
      // Destination picking: never suggest a folder on the excluded (source)
      // device — writing recoveries back to the source is a hard refusal.
      const all = this.mountPoints();
      let pool = all;
      if (hint?.excludeDeviceId) {
        const excludedVolumes = new Set(
          this.deviceList
            .filter(
              (d) =>
                d.id === hint.excludeDeviceId ||
                d.volumes.some((v) => v.id === hint.excludeDeviceId),
            )
            .flatMap((d) => d.volumes.map((v) => v.id)),
        );
        const others = all.filter((m) => !excludedVolumes.has(m.id));
        if (others.length > 0) pool = others;
      }
      const suggestion = pool[this.pickIndex % pool.length] ?? pool[0];
      this.pickIndex++;
      return suggestion ? `${suggestion.path}Recovered Files` : null;
    },

    listMountPoints: async (): Promise<MountPoint[]> => {
      await latency(100, 40);
      return this.mountPoints();
    },

    openPath: async (path: string): Promise<void> => {
      await latency(30);
      void path;
      // Shell open is a real-adapter capability (the Tauri connector
      // implements it via the OS opener); the mock has no OS access and
      // reports so honestly instead of pretending.
      throw connError(
        'not-implemented',
        'opening paths requires the desktop shell',
        'errors.openPathMock',
      );
    },

    getDeviceTransport: async (deviceId: string): Promise<DeviceTransportInfo> => {
      await latency(60, 25);
      const known =
        this.deviceList.some((d) => d.id === deviceId) ||
        this.deviceList.some((d) => d.volumes.some((v) => v.id === deviceId));
      if (!known) {
        throw connError('device-gone', `device ${deviceId} not found`, 'errors.deviceGone');
      }
      // The session-96 truth: every raw-disk device is a direct
      // \\.\PhysicalDriveN view; the simulator has no image devices.
      return { deviceId, transport: 'direct', lastTransportError: null };
    },
  };

  // ------------------------------------------------------------ lane C: imaging

  readonly imaging = {
    startImageCreation: async (
      deviceId: string,
      path: string,
    ): Promise<{ imageId: string }> => {
      await latency(420, 120);
      const trimmed = path.trim();
      if (trimmed === '') {
        throw connError(
          'scan-scope-invalid',
          'image destination path is empty',
          'errors.scanScopeInvalid',
        );
      }
      // Whole-disk semantics: the mock's own device namespace (dev-*)
      // plays the disk-N role; volume ids are refused the same way the
      // real bridge refuses them (imaging addresses whole disks).
      if (/^vol-/.test(deviceId)) {
        throw connError(
          'scan-scope-invalid',
          `imaging addresses whole disks: '${deviceId}' is a volume id`,
          'errors.scanScopeInvalid',
        );
      }
      const diskIdx = this.deviceList.findIndex((d) => d.id === deviceId);
      if (diskIdx < 0) {
        throw connError('device-gone', `device ${deviceId} not found`, 'errors.deviceGone');
      }
      const device = this.deviceList[diskIdx];
      // The same-disk pre-gate: the destination letter must not live on
      // the imaged device (mirrors the bridge's typed refusal).
      const destLetter = /^[A-Za-z]:/.exec(trimmed)?.[0]?.[0]?.toUpperCase();
      if (destLetter && device.volumes.some((v) => v.letter?.toUpperCase() === destLetter)) {
        throw connError(
          'destination-overlap',
          'the image destination is on the imaged disk itself',
          'recovery.overlapReason',
        );
      }
      const imageId = `img-${this.nextImageId++}`;
      const rt: ImageRuntime = {
        imageId,
        disk: diskIdx + 1,
        path: trimmed,
        status: 'running',
        bytesDone: 0,
        bytesTotal: device.capacity.totalBytes,
        throughput: 96 * 1024 * 1024,
        tick: 0,
        timer: null,
        startedAt: new Date().toISOString(),
        finishedAt: null,
      };
      this.imageJobs.set(imageId, rt);
      rt.timer = setInterval(() => this.imageTick(rt), 90);
      return { imageId };
    },

    getImageState: async (imageId: string): Promise<ImageJobState> => {
      await latency(30, 15);
      const rt = this.imageJobs.get(imageId);
      if (!rt) {
        throw connError(
          'image-not-found',
          `image job ${imageId} not found`,
          'errors.imageNotFound',
        );
      }
      const frac = rt.bytesTotal > 0 ? rt.bytesDone / rt.bytesTotal : 0;
      return {
        imageId,
        status: rt.status,
        bytesDone: rt.bytesDone,
        bytesTotal: rt.bytesTotal,
        overallProgress: rt.status === 'completed' ? 1 : frac,
        throughputBytesPerSecond: rt.throughput,
        etaSeconds:
          rt.status === 'running' && rt.bytesTotal > rt.bytesDone
            ? Math.round((rt.bytesTotal - rt.bytesDone) / rt.throughput)
            : rt.status === 'completed'
              ? 0
              : null,
        disk: rt.disk,
        path: rt.path,
        startedAt: rt.startedAt,
        finishedAt: rt.finishedAt,
        cancelSupported: false, // the truthful contract, mirrored verbatim
        partialKept: false, // the simulator never simulates a failed stream
      };
    },

    onImageProgress: (imageId: string, cb: (event: ImageProgressEvent) => void): (() => void) =>
      this.subscribeTo(this.imageProgressListeners, imageId, cb),

    onImageFinished: (imageId: string, cb: (event: ImageFinishedEvent) => void): (() => void) =>
      this.subscribeTo(this.imageFinishedListeners, imageId, cb),
  };

  // ------------------------------------------------------------ lane C: maintenance

  readonly maintenance = {
    runFsck: async (letter: string, fix?: boolean): Promise<FsckResult> => {
      await latency(700, 150);
      const l = /^[A-Za-z]:?/.exec(letter.trim())?.[0]?.[0]?.toUpperCase();
      if (!l) {
        throw connError(
          'scan-scope-invalid',
          `volume '${letter}' is not a drive letter`,
          'errors.scanScopeInvalid',
        );
      }
      // The loud refusal mirrored: no volume mounted at the letter is
      // device-gone (the engine's ERR_OPEN_FAILED class), never a silent
      // skip.
      const mounted = this.deviceList
        .flatMap((d) => d.volumes)
        .some((v) => v.letter?.toUpperCase() === l);
      if (!mounted) {
        throw connError('device-gone', `no volume is mounted at ${l}:`, 'errors.deviceGone');
      }
      // The simulator's verdict truth: the bad-sectors scenario models a
      // volume with remaining errors (the raw exit code crosses verbatim;
      // the verdictKey is the final i18n key, same as the bridge's).
      const errorsRemain = this.scenario === 'bad-sectors';
      const exitCode = errorsRemain ? 3 : fix ? 1 : 0;
      const verdict = errorsRemain ? 'errors-remain' : fix ? 'fixed' : 'clean';
      const verdictKeys: Record<typeof verdict, string> = {
        clean: 'maintenance.fsckClean',
        fixed: 'maintenance.fsckFixed',
        'errors-remain': 'maintenance.fsckErrorsRemain',
      };
      return {
        letter: `${l}:`,
        exitCode,
        verdict,
        verdictKey: verdictKeys[verdict],
        fix: fix ?? false,
      };
    },

    turnOffBitlocker: async (letter: string): Promise<BitlockerOffResult> => {
      await latency(900, 200);
      const l = /^[A-Za-z]:?/.exec(letter.trim())?.[0]?.[0]?.toUpperCase();
      if (!l) {
        throw connError(
          'scan-scope-invalid',
          `volume '${letter}' is not a drive letter`,
          'errors.scanScopeInvalid',
        );
      }
      const volume = this.deviceList
        .flatMap((d) => d.volumes)
        .find((v) => v.letter?.toUpperCase() === l);
      if (volume?.bitlocker !== true && volume?.bitlocker !== 'locked') {
        throw connError(
          'device-permission',
          `volume ${l}: is not BitLocker-protected`,
          'errors.devicePermission',
        );
      }
      // OK means ACCEPTED, not plaintext (the honest contract).
      return { letter: `${l}:`, accepted: true };
    },
  };

  private mountPoints(): MountPoint[] {
    return this.deviceList
      .flatMap((d) => d.volumes)
      .filter((v) => v.letter)
      .map((v) => ({
        id: v.id,
        label: v.label,
        path: `${v.letter}:\\`,
        freeBytes: v.capacity.freeBytes,
        totalBytes: v.capacity.totalBytes,
      }));
  }

  // ------------------------------------------------------------ helpers exposed for tests
}

function latency(meanMs: number, jitterMs = 0): Promise<void> {
  const ms = meanMs + (Math.random() - 0.5) * 2 * jitterMs;
  return new Promise((resolve) => setTimeout(resolve, Math.max(20, ms)));
}
