/**
 * TEST DOUBLE — the connector the component/e2e suites run against.
 *
 * ⚠ NEVER SHIPPED: this module lives under tests/fakes/ and is wired ONLY
 * through the Vite resolve-alias override that activates in test mode
 * (vitest runs and `vite build --mode test` e2e builds — see vite.config.ts).
 * Production builds resolve `@/connector` to src/connector/index.ts and can
 * never reach this file; scripts/arch-lint.mjs rule A7 forbids importing
 * tests/fakes/** from src/** (plan §5.5).
 *
 * What it is: a small deterministic RecoveryConnector implementing the
 * contract against fixed in-memory fixtures — enough for the component
 * tests' pre-seeded caches and the e2e golden path (scan → results →
 * recover). What it is NOT: a simulation of engine behavior. Where the real
 * bridge refuses (the G-1 repair gaps), the double refuses identically.
 */

import type {
  BadSectorRange,
  ConsentRequestEvent,
  ConnectorError,
  ConnectorErrorCode,
  Device,
  DiskHealth,
  PreviewPayload,
  RecoveryConnector,
  RecoveryItem,
  RecoveryJob,
  RecoveryReport,
  ResultsFilter,
  ResultsPage,
  ResultNode,
  ScanProgressEvent,
  ScanState,
  SessionMeta,
  TreeNode,
  NodeDetail,
  DriverStatus,
  AppInfo,
  MountPoint,
  DestinationValidation,
  RepairDryRun,
  RepairUndoEntry,
  RecoveryProgressEvent,
  ScanParams,
} from '../../src/connector/contract';

function connError(code: ConnectorErrorCode, message: string): ConnectorError {
  return Object.assign(new Error(message), { code });
}

// ---------------------------------------------------------------------------
// Fixture world (small, deterministic, hand-checkable)
// ---------------------------------------------------------------------------

const SCAN_BYTES = 512 * 1024 * 1024;

const VOLUME_C = {
  id: 'vol-C',
  deviceId: 'disk-0',
  label: 'Local Disk (C:)',
  letter: 'C:',
  fs: 'ntfs',
  capacity: {
    totalBytes: SCAN_BYTES,
    usedBytes: 256 * 1024 * 1024,
    freeBytes: 256 * 1024 * 1024,
  },
  status: 'ok',
  quickScanAvailable: true,
  sectorSize: 512,
  bitlocker: false,
} as const;

const DEVICE: Device = {
  id: 'disk-0',
  kind: 'internal',
  model: 'TestDisk 512G',
  label: 'Local Disk (C:)',
  serial: 'TEST-0001',
  capacity: VOLUME_C.capacity,
  status: 'ok',
  isSourceOfActiveScan: false,
  volumes: [VOLUME_C],
  canScanWholeDisk: true,
};

type FixtureFile = {
  id: string;
  name: string;
  path: string;
  sizeBytes: number;
  modifiedAt: string;
  extension: string;
  category: ResultNode['category'];
  origin: ResultNode['origin'];
  confidence: ResultNode['confidence'];
  parentId: string;
};

const FILES: readonly FixtureFile[] = [
  {
    id: 'f100',
    name: 'report.docx',
    path: 'Users/test/Documents',
    sizeBytes: 184_320,
    modifiedAt: '2026-01-15T10:30:00.000Z',
    extension: '.docx',
    category: 'document',
    origin: 'existing',
    confidence: 'excellent',
    parentId: 'd7',
  },
  {
    id: 'f101',
    name: 'photo.jpg',
    path: 'Users/test/Pictures',
    sizeBytes: 2_411_520,
    modifiedAt: '2025-11-02T08:12:00.000Z',
    extension: '.jpg',
    category: 'graphics',
    origin: 'existing',
    confidence: 'good',
    parentId: 'd8',
  },
  {
    id: 'f102',
    name: 'notes.txt',
    path: 'Users/test/Documents',
    sizeBytes: 640,
    modifiedAt: '2026-02-20T16:45:00.000Z',
    extension: '.txt',
    category: 'document',
    origin: 'deleted',
    confidence: 'good',
    parentId: 'd7',
  },
  {
    id: 'f103',
    name: 'carve_0001.jpg',
    path: '',
    sizeBytes: 512_000,
    modifiedAt: '2025-08-19T12:00:00.000Z',
    extension: '.jpg',
    category: 'graphics',
    origin: 'raw',
    confidence: 'fair',
    parentId: 'd1',
  },
  {
    id: 'f104',
    name: 'old-registry.dat',
    path: 'Windows/System32/config',
    sizeBytes: 81_920,
    modifiedAt: '2024-06-01T04:00:00.000Z',
    extension: '.dat',
    category: 'other',
    origin: 'lost-path',
    confidence: 'poor',
    parentId: 'd2',
  },
];

const DIRS: readonly TreeNode[] = [
  {
    id: 'd1',
    parentId: null,
    label: 'Users',
    icon: 'directory',
    childCount: 1,
    fileCount: 1,
    sizeBytes: 512_000,
    origin: 'existing',
  },
  {
    id: 'd2',
    parentId: null,
    label: 'Windows',
    icon: 'directory',
    childCount: 0,
    fileCount: 1,
    sizeBytes: 81_920,
    origin: 'existing',
  },
  {
    id: 'd7',
    parentId: 'd1',
    label: 'Documents',
    icon: 'directory',
    childCount: 0,
    fileCount: 2,
    sizeBytes: 184_960,
    origin: 'existing',
  },
  {
    id: 'd8',
    parentId: 'd1',
    label: 'Pictures',
    icon: 'directory',
    childCount: 0,
    fileCount: 1,
    sizeBytes: 2_411_520,
    origin: 'existing',
  },
];

function toResultNode(f: FixtureFile, scanId: string): ResultNode {
  return {
    id: f.id,
    scanId,
    kind: 'file',
    name: f.name,
    path: f.origin === 'raw' ? null : f.path,
    sizeBytes: f.sizeBytes,
    modifiedAt: f.modifiedAt,
    extension: f.extension,
    category: f.category,
    confidence: f.confidence,
    origin: f.origin,
    childCount: null,
    thumbnailAvailable: f.category === 'graphics',
    parentId: f.parentId,
  };
}

const STAGES: readonly ScanProgressEvent['stage'][] = [
  'volume-analysis',
  'quick-index',
  'deep-walk',
  'carve',
  'tree-build',
];

interface ScanRuntime {
  scanId: string;
  params: ScanParams;
  status: ScanState['status'];
  stageIndex: number;
  startedAt: string;
  completedAt: string | null;
  badSectors: readonly BadSectorRange[];
  timers: ReturnType<typeof setTimeout>[];
  /** G-1: the pending pre-scan repair consent (null when none/answered).
   *  While non-null the stage walk is NOT scheduled — mirroring the real
   *  bridge, where the engine's run thread BLOCKS inside the consent
   *  callback until consent_respond answers. */
  pendingConsent: ConsentRequestEvent | null;
  /** True when this scan already emitted its consent (fires at most once). */
  consentEmitted: boolean;
  /** True once an answer resumed the walk (or no consent was needed). */
  walkScheduled: boolean;
  /** The recorded answer (null = unanswered / no consent fired) — fixture
   *  state so tests can assert the approve/deny narrative explicitly. */
  consentApproved: boolean | null;
}

interface JobRuntime {
  jobId: string;
  scanId: string;
  destination: string;
  nodeIds: readonly string[];
  status: 'running' | 'completed';
  done: number;
  timers: ReturnType<typeof setTimeout>[];
}

const nowIso = (): string => new Date().toISOString();

// ---------------------------------------------------------------------------
// The double
// ---------------------------------------------------------------------------

class TestConnector implements RecoveryConnector {
  private scans = new Map<string, ScanRuntime>();
  private jobs = new Map<string, JobRuntime>();
  private progressCbs = new Map<string, (e: ScanProgressEvent) => void>();
  private consentCbs = new Map<string, (e: ConsentRequestEvent) => void>();
  private recoveryCbs = new Map<string, (e: RecoveryProgressEvent) => void>();
  private nextScan = 1;
  private nextConsent = 1;
  private nextJob = 1;
  private sessionStore: SessionMeta[] = [];

  readonly devices = {
    listDevices: (): Promise<Device[]> => Promise.resolve([DEVICE]),
    getDeviceInfo: (deviceId: string): Promise<Device> => {
      if (deviceId === DEVICE.id || deviceId === VOLUME_C.id) {
        return Promise.resolve(DEVICE);
      }
      return Promise.reject(connError('device-gone', `device ${deviceId} not found`));
    },
    refreshDevices: (): Promise<Device[]> => Promise.resolve([DEVICE]),
  };

  readonly scan = {
    startScan: (params: ScanParams): Promise<{ scanId: string }> => {
      // The virtual Home quick-targets resolve to the system volume in the
      // real bridge (with an implicit pathScope). The double mirrors the
      // SUCCESS (it must succeed exactly where production succeeds); the
      // fixed fixture tree stands in for the resolved results. Everything
      // else still fails with device-gone, exactly like production.
      const resolvesToVolumeC =
        params.targetId === DEVICE.id ||
        params.targetId === VOLUME_C.id ||
        params.targetId === 'vol-recycle-bin' ||
        params.targetId === 'vol-desktop';
      if (!resolvesToVolumeC) {
        return Promise.reject(
          connError('device-gone', `target ${params.targetId} is not present`),
        );
      }
      const scanId = `scan-${this.nextScan++}`;
      const rt: ScanRuntime = {
        scanId,
        params,
        status: 'running',
        stageIndex: 0,
        startedAt: nowIso(),
        completedAt: null,
        badSectors: [],
        timers: [],
        pendingConsent: null,
        consentEmitted: false,
        walkScheduled: false,
        consentApproved: null,
      };
      this.scans.set(scanId, rt);
      // G-1 fixture: the Desktop quick-target stands in for a volume whose
      // pre-scan analysis found a repairable plan — the consent fires ~80ms
      // in (pre-stage, mirroring 'before any engine read') and the walk
      // stays BLOCKED until respondToConsent answers. Every other target
      // walks immediately, exactly like before.
      if (params.targetId === 'vol-desktop') {
        rt.timers.push(setTimeout(() => this.fireConsent(rt), 80));
      } else {
        this.scheduleWalk(rt);
      }
      return Promise.resolve({ scanId });
    },

    pauseScan: (scanId: string): Promise<void> => {
      const rt = this.requireScan(scanId);
      if (rt.status !== 'running') {
        return Promise.reject(connError('scan-not-running', 'scan is not running'));
      }
      rt.status = 'paused';
      return Promise.resolve();
    },

    resumeScan: (scanId: string): Promise<void> => {
      const rt = this.requireScan(scanId);
      if (rt.status !== 'paused') {
        return Promise.reject(connError('scan-not-paused', 'scan is not paused'));
      }
      rt.status = 'running';
      return Promise.resolve();
    },

    cancelScan: (scanId: string): Promise<void> => {
      const rt = this.requireScan(scanId);
      rt.timers.forEach(clearTimeout);
      // Mirror the real bridge: cancel force-denies a pending consent (the
      // gate resolves deny; no event fires — the prompt's dismissal rides
      // the page's cancel navigation, exactly like production).
      if (rt.pendingConsent !== null) {
        rt.pendingConsent = null;
        rt.consentApproved = false;
      }
      rt.status = 'cancelled';
      rt.completedAt = nowIso();
      this.emitProgress(rt);
      return Promise.resolve();
    },

    getScanState: (scanId: string): Promise<ScanState> => {
      const rt = this.requireScan(scanId);
      const progress = this.progressOf(rt);
      return Promise.resolve({
        scanId,
        params: rt.params,
        target: VOLUME_C,
        status: rt.status,
        startedAt: rt.startedAt,
        completedAt: rt.completedAt,
        progress,
        pendingConsent: rt.pendingConsent,
      });
    },

    onScanProgress: (scanId: string, cb: (event: ScanProgressEvent) => void): (() => void) => {
      this.progressCbs.set(scanId, cb);
      const rt = this.scans.get(scanId);
      if (rt) cb(this.progressOf(rt));
      return () => {
        this.progressCbs.delete(scanId);
      };
    },

    // G-1 (a): the consent surface, mirroring the real bridge's semantics —
    // fires at most once per scan; respondToConsent is the only answer
    // path; a stale id (already answered / cancelled) rejects with the
    // typed consent-not-found.
    onConsentRequest: (
      scanId: string,
      cb: (event: ConsentRequestEvent) => void,
    ): (() => void) => {
      this.consentCbs.set(scanId, cb);
      const rt = this.scans.get(scanId);
      if (rt?.pendingConsent) cb(rt.pendingConsent);
      return () => {
        this.consentCbs.delete(scanId);
      };
    },

    respondToConsent: (requestId: string, allow: boolean): Promise<void> => {
      const entry = [...this.scans.values()].find(
        (rt) => rt.pendingConsent?.requestId === requestId,
      );
      if (!entry) {
        return Promise.reject(
          connError(
            'consent-not-found',
            `consent request '${requestId}' is unknown or already resolved`,
          ),
        );
      }
      entry.pendingConsent = null;
      entry.consentApproved = allow;
      // The engine unblocks: approve = the repair applied before the stages;
      // deny = read-only. The walk is the same fixture either way (the
      // narrative differs, not the tree) — consentApproved records which.
      if (entry.status === 'running' && !entry.walkScheduled) {
        this.scheduleWalk(entry);
      }
      return Promise.resolve();
    },
  };

  readonly results = {
    getResultTree: (
      scanId: string,
      parentId: string | null,
      mode: string,
    ): Promise<{ nodes: readonly TreeNode[]; parentHasMore: boolean }> => {
      this.requireScan(scanId);
      if (mode === 'type') {
        if (parentId === null) {
          const cats = ['graphics', 'document', 'other'] as const;
          return Promise.resolve({
            nodes: cats.map((c) => ({
              id: `c-${c}`,
              parentId: null,
              label: c,
              icon: 'category',
              childCount: 1,
              fileCount: FILES.filter((f) => f.category === c).length,
              sizeBytes: FILES.filter((f) => f.category === c).reduce(
                (a, f) => a + f.sizeBytes,
                0,
              ),
              origin: 'existing',
            })),
            parentHasMore: false,
          });
        }
        const cat = parentId.replace(/^c-/, '');
        const exts = [
          ...new Set(FILES.filter((f) => f.category === cat).map((f) => f.extension)),
        ];
        return Promise.resolve({
          nodes: exts.map((e) => ({
            id: `ext:${cat}:${e.replace('.', '')}`,
            parentId,
            label: e.replace('.', '').toUpperCase(),
            icon: 'directory',
            childCount: 0,
            fileCount: FILES.filter((f) => f.category === cat && f.extension === e).length,
            sizeBytes: FILES.filter((f) => f.category === cat && f.extension === e).reduce(
              (a, f) => a + f.sizeBytes,
              0,
            ),
            origin: 'existing',
          })),
          parentHasMore: false,
        });
      }
      if (mode === 'raw') {
        const raw = FILES.filter((f) => f.origin === 'raw');
        return Promise.resolve({
          nodes: [
            {
              id: 'raw-all',
              parentId: null,
              label: 'raw',
              icon: 'group',
              childCount: 0,
              fileCount: raw.length,
              sizeBytes: raw.reduce((a, f) => a + f.sizeBytes, 0),
              origin: 'raw',
              special: 'raw-found',
            },
          ],
          parentHasMore: false,
        });
      }
      if (parentId === null) {
        const structured = FILES.filter((f) => f.origin !== 'raw');
        const lost = FILES.filter((f) => f.origin === 'lost-path');
        const raw = FILES.filter((f) => f.origin === 'raw');
        return Promise.resolve({
          nodes: [
            {
              id: 'root',
              parentId: null,
              label: 'root',
              icon: 'device',
              childCount: DIRS.filter((d) => d.parentId === null).length,
              fileCount: structured.length,
              sizeBytes: structured.reduce((a, f) => a + f.sizeBytes, 0),
              origin: 'existing',
            },
            {
              id: 'lost-paths',
              parentId: null,
              label: 'lost-paths',
              icon: 'group',
              childCount: lost.length > 0 ? 1 : 0,
              fileCount: lost.length,
              sizeBytes: lost.reduce((a, f) => a + f.sizeBytes, 0),
              origin: 'lost-path',
              special: 'lost-path',
            },
            {
              id: 'raw-all',
              parentId: null,
              label: 'raw',
              icon: 'group',
              childCount: 0,
              fileCount: raw.length,
              sizeBytes: raw.reduce((a, f) => a + f.sizeBytes, 0),
              origin: 'raw',
              special: 'raw-found',
            },
          ],
          parentHasMore: false,
        });
      }
      if (parentId === 'lost-paths') {
        const lost = FILES.filter((f) => f.origin === 'lost-path');
        return Promise.resolve({
          nodes: [
            {
              id: 'd2',
              parentId: 'lost-paths',
              label: 'Windows',
              icon: 'directory',
              childCount: 0,
              fileCount: lost.length,
              sizeBytes: lost.reduce((a, f) => a + f.sizeBytes, 0),
              origin: 'lost-path',
            },
          ],
          parentHasMore: false,
        });
      }
      return Promise.resolve({
        nodes: DIRS.filter((d) => d.parentId === parentId),
        parentHasMore: false,
      });
    },

    queryResults: (
      scanId: string,
      filter: ResultsFilter,
      sort: { field: string; direction: 'asc' | 'desc' },
      page: { offset: number; limit: number },
    ): Promise<ResultsPage> => {
      this.requireScan(scanId);
      let hits = FILES.slice();
      if (filter.origins) {
        hits = hits.filter((f) => (filter.origins ?? []).includes(f.origin));
      }
      if (filter.categories) {
        hits = hits.filter((f) => (filter.categories ?? []).includes(f.category ?? 'other'));
      }
      if (filter.nodeId === 'raw-all') hits = hits.filter((f) => f.origin === 'raw');
      if (filter.nodeId === 'lost-paths') hits = hits.filter((f) => f.origin === 'lost-path');
      if (filter.search) {
        const needle = filter.search.toLowerCase();
        hits = hits.filter((f) => f.name.toLowerCase().includes(needle));
      }
      if (filter.hideEmptyFiles) hits = hits.filter((f) => f.sizeBytes > 0);
      const dir = sort.direction === 'asc' ? 1 : -1;
      hits = hits.slice().sort((a, b) => {
        const key = (f: FixtureFile): string | number =>
          sort.field === 'size'
            ? f.sizeBytes
            : sort.field === 'modified'
              ? f.modifiedAt
              : f.name;
        const ka = key(a);
        const kb = key(b);
        return ka < kb ? -dir : ka > kb ? dir : 0;
      });
      const window = hits.slice(page.offset, page.offset + page.limit);
      return Promise.resolve({
        nodes: window.map((f) => toResultNode(f, scanId)),
        total: hits.length,
        totalBytes: hits.reduce((a, f) => a + f.sizeBytes, 0),
        offset: page.offset,
        pageSize: page.limit,
        elapsedMs: 1,
      });
    },

    getNodeDetail: (scanId: string, nodeId: string): Promise<NodeDetail> => {
      this.requireScan(scanId);
      const f = FILES.find((x) => x.id === nodeId);
      if (!f) return Promise.reject(connError('unknown', `node ${nodeId} not found`));
      return Promise.resolve({
        node: toResultNode(f, scanId),
        fullPhysicalPath:
          f.origin === 'existing' ? `C:\\${f.path.replace(/\//g, '\\')}\\${f.name}` : null,
        createdAt: f.modifiedAt,
        accessedAt: f.modifiedAt,
        confidenceReason:
          f.confidence === 'excellent'
            ? 'Headers and extents fully readable'
            : 'Slight fragmentation, all clusters readable',
        badSectorsOverlap: f.confidence === 'poor',
        clusterCount: Math.max(1, Math.ceil(f.sizeBytes / 4096)),
      });
    },

    countResults: (scanId: string, filter: ResultsFilter): Promise<number> => {
      this.requireScan(scanId);
      let hits = FILES.slice();
      if (filter.origins) hits = hits.filter((f) => (filter.origins ?? []).includes(f.origin));
      if (filter.nodeId === 'raw-all') hits = hits.filter((f) => f.origin === 'raw');
      if (filter.nodeId === 'lost-paths') hits = hits.filter((f) => f.origin === 'lost-path');
      return Promise.resolve(hits.length);
    },
  };

  readonly preview = {
    requestPreview: (scanId: string, nodeId: string): Promise<PreviewPayload> => {
      this.requireScan(scanId);
      const f = FILES.find((x) => x.id === nodeId);
      if (!f) return Promise.reject(connError('unknown', `node ${nodeId} not found`));
      if (f.extension === '.txt') {
        return Promise.resolve({
          kind: 'text',
          text: 'recovered fixture text line one\nline two\n',
          encoding: 'UTF-8',
          totalSize: f.sizeBytes,
          truncated: false,
        });
      }
      if (f.extension === '.jpg') {
        return Promise.resolve({
          kind: 'image',
          dataUrl: `data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64"><rect width="64" height="64" fill="%233B82F6"/></svg>`,
          width: 64,
          height: 64,
          bounded: false,
        });
      }
      return Promise.resolve({
        kind: 'hex',
        bytes: new Uint8Array(64),
        totalSize: f.sizeBytes,
        baseOffset: 0,
      });
    },

    getThumbnail: (scanId: string, nodeId: string): Promise<{ dataUrl: string } | null> => {
      this.requireScan(scanId);
      const f = FILES.find((x) => x.id === nodeId);
      if (!f || f.category !== 'graphics') return Promise.resolve(null);
      return Promise.resolve({
        dataUrl:
          'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32"><rect width="32" height="32" fill="%2393C5FD"/></svg>',
      });
    },
  };

  readonly sessions = {
    saveSession: (scanId: string, path: string | null): Promise<SessionMeta> => {
      const rt = this.requireScan(scanId);
      const meta: SessionMeta = {
        id: `sess-${this.sessionStore.length + 1}`,
        fileName: path ?? `auto_local_volume_${nowIso().replace(/[:.]/g, '-')}.crsf`,
        savedAt: nowIso(),
        targetLabel: VOLUME_C.label,
        filesFound: FILES.length,
        dataFoundBytes: FILES.reduce((a, f) => a + f.sizeBytes, 0),
        completed: rt.status === 'completed',
      };
      this.sessionStore.unshift(meta);
      return Promise.resolve(meta);
    },

    listSessions: (): Promise<SessionMeta[]> => Promise.resolve(this.sessionStore.slice()),

    loadSession: (path: string): Promise<{ scanId: string }> => {
      const meta = this.sessionStore.find((s) => s.fileName === path);
      if (!meta) {
        return Promise.reject(connError('session-not-found', `session ${path} not found`));
      }
      const scanId = `scan-${this.nextScan++}`;
      this.scans.set(scanId, {
        scanId,
        params: { targetId: 'vol-C' },
        status: 'completed',
        stageIndex: STAGES.length - 1,
        startedAt: meta.savedAt,
        completedAt: meta.savedAt,
        badSectors: [],
        timers: [],
        // A loaded session never runs — no consent surface (bridge parity:
        // ScanKind::Session arms nothing).
        pendingConsent: null,
        consentEmitted: false,
        walkScheduled: true,
        consentApproved: null,
      });
      return Promise.resolve({ scanId });
    },

    getSessionMeta: (path: string): Promise<SessionMeta> => {
      const meta = this.sessionStore.find((s) => s.fileName === path);
      if (!meta) {
        return Promise.reject(connError('session-not-found', `session ${path} not found`));
      }
      return Promise.resolve(meta);
    },
  };

  readonly recovery = {
    startRecovery: (
      scanId: string,
      selection:
        { kind: 'nodes'; nodeIds: readonly string[] } | { kind: 'all'; filter: ResultsFilter },
      destination: string,
    ): Promise<{ jobId: string }> => {
      this.requireScan(scanId);
      const nodeIds = selection.kind === 'nodes' ? selection.nodeIds : FILES.map((f) => f.id);
      const jobId = `job-${this.nextJob++}`;
      const job: JobRuntime = {
        jobId,
        scanId,
        destination,
        nodeIds,
        status: 'running',
        done: 0,
        timers: [],
      };
      this.jobs.set(jobId, job);
      job.timers.push(
        setTimeout(() => {
          job.status = 'completed';
          job.done = nodeIds.length;
          this.emitRecovery(job);
        }, 180),
      );
      return Promise.resolve({ jobId });
    },

    cancelRecovery: (jobId: string): Promise<void> => {
      const job = this.jobs.get(jobId);
      if (job) {
        job.timers.forEach(clearTimeout);
        job.status = 'completed';
        job.done = job.nodeIds.length;
      }
      return Promise.resolve();
    },

    getRecoveryState: (jobId: string): Promise<RecoveryJob> => {
      const job = this.jobs.get(jobId);
      if (!job) return Promise.reject(connError('unknown', `job ${jobId} not found`));
      return Promise.resolve({
        jobId,
        scanId: job.scanId,
        nodeIds: job.nodeIds,
        destination: job.destination,
        options: {
          collision: 'rename',
          rebuildFolders: true,
          keepEmptyFolders: false,
          fileRepair: false,
          verifyOnWrite: true,
        },
        status: job.status,
      });
    },

    getRecoveryItems: (
      jobId: string,
      page: { offset: number; limit: number },
    ): Promise<readonly RecoveryItem[]> => {
      const job = this.jobs.get(jobId);
      if (!job) return Promise.reject(connError('unknown', `job ${jobId} not found`));
      const items: RecoveryItem[] = job.nodeIds.map((id) => {
        const f = FILES.find((x) => x.id === id);
        return {
          nodeId: id,
          name: f?.name ?? id,
          sizeBytes: f?.sizeBytes ?? 0,
          state: job.status === 'completed' ? 'done' : 'pending',
          savedAs: job.status === 'completed' ? (f?.name ?? id) : undefined,
        };
      });
      return Promise.resolve(items.slice(page.offset, page.offset + page.limit));
    },

    onRecoveryProgress: (
      jobId: string,
      cb: (event: RecoveryProgressEvent) => void,
    ): (() => void) => {
      this.recoveryCbs.set(jobId, cb);
      const job = this.jobs.get(jobId);
      if (job) cb(this.recoveryOf(job));
      return () => {
        this.recoveryCbs.delete(jobId);
      };
    },

    getRecoveryReport: (jobId: string): Promise<RecoveryReport> => {
      const job = this.jobs.get(jobId);
      if (!job) return Promise.reject(connError('unknown', `job ${jobId} not found`));
      const items: RecoveryItem[] = job.nodeIds.map((id) => {
        const f = FILES.find((x) => x.id === id);
        return {
          nodeId: id,
          name: f?.name ?? id,
          sizeBytes: f?.sizeBytes ?? 0,
          state: job.status === 'completed' ? 'done' : 'pending',
          savedAs: job.status === 'completed' ? (f?.name ?? id) : undefined,
        };
      });
      return Promise.resolve({
        jobId,
        status: job.status,
        startedAt: nowIso(),
        finishedAt: job.status === 'completed' ? nowIso() : null,
        succeeded: items,
        failed: [],
        skipped: [],
        totalBytesWritten: items.reduce((a, i) => a + i.sizeBytes, 0),
        verified: job.status === 'completed',
        processedTotal: items.length,
        truncatedItemLists: false,
      });
    },
  };

  readonly health = {
    getHealth: (deviceId: string): Promise<DiskHealth> => {
      if (deviceId !== DEVICE.id) {
        return Promise.reject(connError('device-gone', `device ${deviceId} not found`));
      }
      return Promise.resolve({
        deviceId,
        status: 'good',
        temperatureC: 34,
        powerOnHours: 5_200,
        diskClass: 'ssd',
        wearPercent: 12,
        trimEnabled: true,
        reallocatedSectors: 0,
        pendingSectors: 0,
        attributes: [
          {
            id: 5,
            name: 'Reallocated Sectors Count',
            current: 100,
            worst: 100,
            threshold: null,
            raw: 0,
            status: 'ok',
          },
          {
            id: 194,
            name: 'Temperature',
            current: 34,
            worst: 40,
            threshold: null,
            raw: 34,
            status: 'ok',
          },
        ],
      });
    },
  };

  readonly repair = {
    // Mirrors the REAL bridge's G-1 gap refusals (docs/INTEGRATION-GAPS.md):
    // the double must fail exactly where production fails — never better.
    dryRunRepair: (deviceId: string): Promise<RepairDryRun> =>
      Promise.reject(
        connError(
          'not-implemented',
          `repair.dryRunRepair (standalone repair workbench): ${deviceId}`,
        ),
      ),
    applyRepair: (deviceId: string): Promise<{ applied: number }> =>
      Promise.reject(connError('not-implemented', `repair.applyRepair: ${deviceId}`)),
    getUndoLog: (deviceId: string): Promise<readonly RepairUndoEntry[]> =>
      Promise.reject(connError('not-implemented', `repair.getUndoLog: ${deviceId}`)),
    undoRepairs: (deviceId: string): Promise<{ rolledBack: number }> =>
      Promise.reject(connError('not-implemented', `repair.undoRepairs: ${deviceId}`)),
  };

  readonly system = {
    getDriverStatus: (): Promise<DriverStatus> =>
      Promise.resolve({
        installed: true,
        version: '1.0',
        running: true,
        note: 'test double: driver status is fixture data',
      }),

    getAppInfo: (): Promise<AppInfo> =>
      Promise.resolve({
        name: 'Recovery Wizard',
        version: '0.0.0-test',
        engineVersion: null,
        platform: 'test',
      }),

    validateDestination: (
      path: string,
      _sourceDeviceId: string,
      neededBytes: number,
    ): Promise<DestinationValidation> => {
      const letter = /^([A-Za-z]):/.exec(path)?.[1]?.toUpperCase();
      if (letter === 'C') {
        return Promise.resolve({
          allowed: false,
          reason: 'overlap',
          reasonKey: 'recovery.overlapReason',
          freeBytes: null,
          neededBytes,
        });
      }
      if (!letter) {
        return Promise.resolve({
          allowed: false,
          reason: 'invalid',
          reasonKey: 'errors.destinationInvalid',
          freeBytes: null,
          neededBytes,
        });
      }
      return Promise.resolve({
        allowed: true,
        freeBytes: 1_000_000_000,
        neededBytes,
      });
    },

    pickDirectory: (): Promise<string | null> => Promise.resolve('D:\\Recovered Files'),

    listMountPoints: (): Promise<readonly MountPoint[]> =>
      Promise.resolve([
        {
          id: 'vol-C',
          label: VOLUME_C.label,
          path: 'C:\\',
          freeBytes: VOLUME_C.capacity.freeBytes,
          totalBytes: VOLUME_C.capacity.totalBytes,
        },
        {
          id: 'vol-D',
          label: 'Data (D:)',
          path: 'D:\\',
          freeBytes: 1_000_000_000,
          totalBytes: 2_000_000_000,
        },
      ]),

    openPath: (): Promise<void> => Promise.resolve(),
  };

  // ---------------------------------------------------------------- helpers

  /** The deterministic stage walk (5 ticks x 60ms) — fast enough for e2e
   *  waits, staged enough to exercise the progress UI. Scheduled at start
   *  for clean targets, and only after the consent answers for the
   *  consent-fixture target (the blocked-run-thread mirror). */
  private scheduleWalk(rt: ScanRuntime): void {
    if (rt.walkScheduled) return;
    rt.walkScheduled = true;
    for (let tick = 0; tick < STAGES.length; tick++) {
      rt.timers.push(
        setTimeout(
          () => {
            rt.stageIndex = tick;
            this.emitProgress(rt);
          },
          60 * (tick + 1),
        ),
      );
    }
    rt.timers.push(
      setTimeout(
        () => {
          rt.status = 'completed';
          rt.completedAt = nowIso();
          this.emitProgress(rt);
        },
        60 * (STAGES.length + 1),
      ),
    );
  }

  /** Fire the fixture's repair-plan consent (at most once — cr_ui.h S-008). */
  private fireConsent(rt: ScanRuntime): void {
    if (rt.consentEmitted || rt.status !== 'running') return;
    rt.consentEmitted = true;
    rt.pendingConsent = {
      scanId: rt.scanId,
      requestId: `consent-${this.nextConsent++}`,
      plan: {
        detected: 1,
        fsFamily: 'ntfs',
        actionCount: 3,
        refused: 0,
        reason: 'MFT mirror mismatch — rebuild $MFT mirror (fixture plan)',
        advisoryCount: 1,
      },
    };
    this.emitConsentState(rt.scanId);
  }

  /** Push the pending-consent REQUEST to the scan's subscriber — fires only
   *  while one pends, matching the real connector's event shape (resolve
   *  flows through respondToConsent's success / the page's cancel
   *  navigation, never through this callback). */
  private emitConsentState(scanId: string): void {
    const cb = this.consentCbs.get(scanId);
    const rt = this.scans.get(scanId);
    if (cb && rt?.pendingConsent) cb(rt.pendingConsent);
  }

  private requireScan(scanId: string): ScanRuntime {
    const rt = this.scans.get(scanId);
    if (!rt) throw connError('scan-not-found', `scan ${scanId} not found`);
    return rt;
  }

  private progressOf(rt: ScanRuntime): ScanProgressEvent {
    const overall = rt.status === 'completed' ? 1 : (rt.stageIndex + 0.5) / STAGES.length;
    return {
      scanId: rt.scanId,
      status: rt.status,
      stage:
        rt.status === 'completed' ? 'done' : STAGES[Math.min(rt.stageIndex, STAGES.length - 1)],
      stageProgress: rt.status === 'completed' ? 1 : 0.5,
      overallProgress: overall,
      bytesScanned: Math.round(SCAN_BYTES * overall),
      bytesTotal: SCAN_BYTES,
      filesFound: rt.status === 'completed' ? FILES.length : Math.max(1, rt.stageIndex),
      dataFoundBytes:
        rt.status === 'completed' ? FILES.reduce((a, f) => a + f.sizeBytes, 0) : 0,
      errors: 0,
      badSectorMap: rt.badSectors,
      etaSeconds: rt.status === 'completed' ? 0 : 1,
      throughputBytesPerSecond: 8_000_000,
      recoverableDuringScan: rt.stageIndex >= 2,
      deviceConnected: true,
      currentPath: null,
    };
  }

  private emitProgress(rt: ScanRuntime): void {
    const cb = this.progressCbs.get(rt.scanId);
    if (cb) cb(this.progressOf(rt));
  }

  private recoveryOf(job: JobRuntime): RecoveryProgressEvent {
    const done = job.status === 'completed' ? job.nodeIds.length : 0;
    return {
      jobId: job.jobId,
      status: job.status,
      overallProgress: job.status === 'completed' ? 1 : 0.1,
      currentFile: null,
      currentFileProgress: 0,
      completedCount: done,
      failedCount: 0,
      skippedCount: 0,
      totalCount: job.nodeIds.length,
      throughputBytesPerSecond: 4_000_000,
      verifying: false,
      etaSeconds: job.status === 'completed' ? 0 : 1,
    };
  }

  private emitRecovery(job: JobRuntime): void {
    const cb = this.recoveryCbs.get(job.jobId);
    if (cb) cb(this.recoveryOf(job));
  }
}

export const connector: RecoveryConnector = new TestConnector();
