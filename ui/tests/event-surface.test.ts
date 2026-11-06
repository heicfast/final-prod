import { describe, expect, it, vi, beforeEach, afterEach } from 'vitest';
import { cleanup } from '@testing-library/react';
import { connector, scenarioControl } from '@/connector';
import type {
  LostCandidateEvent,
  RecoveryFinishedEvent,
  ScanAutosavedEvent,
  ResultsFilter,
  ScanFinishedEvent,
} from '@/connector/contract';

const BASE_FILTER: ResultsFilter = {
  nodeId: null,
  treeMode: 'path',
  search: null,
  categories: null,
  dateFrom: null,
  dateTo: null,
  sizeMinBytes: null,
  sizeMaxBytes: null,
  extensions: null,
  origins: null,
  hideEmptyFiles: false,
};

/**
 * W-01 — the dead-letter regression pins. Through v0.8.x the bridge
 * emitted scan-finished / save-progress / lost-candidate / scan-autosaved /
 * session-load-progress / recovery-finished and NOTHING listened. The
 * contract now carries all six subscription surfaces; these pins drive
 * the mock's real lifecycle flows (the same transitions that emit in the
 * bridge) and assert each subscription FIRES.
 */

const wait = (ms: number): Promise<void> => new Promise((r) => setTimeout(r, ms));

async function waitForEvent<T>(
  subscribe: (cb: (e: T) => void) => () => void,
  timeoutMs = 15_000,
): Promise<T> {
  return new Promise<T>((resolve, reject) => {
    const timer = setTimeout(() => {
      unsubscribe();
      reject(new Error('event did not fire within the timeout'));
    }, timeoutMs);
    const unsubscribe = subscribe((e) => {
      clearTimeout(timer);
      unsubscribe();
      resolve(e);
    });
  });
}

describe('W-01: every bridge emission has a listener surface (the mock mirror)', () => {
  beforeEach(() => {
    vi.clearAllMocks();
  });

  afterEach(() => {
    cleanup();
  });

  it('scan-finished + scan-autosaved fire when the scan completes', async () => {
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-sys' });
    expect(scanId).toBeTruthy();
    const [f, a] = await Promise.all([
      waitForEvent<ScanFinishedEvent>((cb) => connector.scan.onScanFinished(scanId, cb)),
      waitForEvent<ScanAutosavedEvent>((cb) => connector.scan.onScanAutosaved(scanId, cb)),
    ]);
    expect(f.status).toBe('completed');
    expect(f.autosaved).toBe(true);
    expect(a.rc).toBe(0);
  }, 30_000);

  it('scan-finished fires with cancelled on a stop', async () => {
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-sys' });
    const finished = waitForEvent<ScanFinishedEvent>((cb) =>
      connector.scan.onScanFinished(scanId, cb),
    );
    await connector.scan.cancelScan(scanId);
    const f = await finished;
    expect(f.status).toBe('cancelled');
    expect(f.autosaved).toBe(false);
  }, 30_000);

  it('lost-candidate fires for the lost-partition fixture volume', async () => {
    // The lost-partition fixture volume lives in the 'lost-partition'
    // scenario's device set (fixtures.ts:443) — switch the mock to it.
    const control = scenarioControl();
    expect(control).not.toBeNull();
    control!.setScenario('lost-partition');
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-lost-main' });
    const candidate = waitForEvent<LostCandidateEvent>((cb) =>
      connector.scan.onLostCandidate(scanId, cb),
    );
    const c = await candidate;
    expect(c.count).toBe(1);
    expect(c.lba).toBeGreaterThan(0);
    expect(c.sectorCount).toBeGreaterThan(0);
    // fs: the engine's FsKindCode — Ntfs = 3.
    expect(c.fs).toBe(3);
    await connector.scan.cancelScan(scanId).catch(() => undefined);
    control!.setScenario('standard');
  }, 30_000);

  it('save-progress streams fractions during a save', async () => {
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-sys' });
    const fractions: number[] = [];
    const done = new Promise<void>((resolve) => {
      const unsubscribe = connector.scan.onSaveProgress(scanId, (e) => {
        fractions.push(e.fraction);
        if (e.fraction >= 1) {
          unsubscribe();
          resolve();
        }
      });
    });
    await connector.sessions.saveSession(scanId, null);
    await done;
    expect(fractions.length).toBeGreaterThan(1);
    expect(fractions[0]).toBeGreaterThan(0);
    expect(fractions[fractions.length - 1]).toBe(1);
    await connector.scan.cancelScan(scanId).catch(() => undefined);
  }, 30_000);

  it('session-load-progress streams while a session loads', async () => {
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-sys' });
    const meta = await connector.sessions.saveSession(scanId, 'unit-load-progress.crsf');
    const steps: number[] = [];
    const done = new Promise<void>((resolve) => {
      const unsubscribe = connector.sessions.onSessionLoadProgress(meta.fileName, (e) => {
        steps.push(e.permille);
        if (e.permille >= 1000) {
          unsubscribe();
          resolve();
        }
      });
    });
    await connector.sessions.loadSession(meta.fileName);
    await done;
    expect(steps.length).toBeGreaterThan(1);
    expect(steps[steps.length - 1]).toBe(1000);
    await connector.scan.cancelScan(scanId).catch(() => undefined);
  }, 30_000);

  it('recovery-finished fires when the job completes', async () => {
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-sys' });
    // Wait for results to exist, then recover one node.
    await wait(1200);
    const tree = await connector.results.getResultTree(scanId, null, 'type');
    void tree;
    const page = await connector.results.queryResults(
      scanId,
      BASE_FILTER,
      {
        field: 'size',
        direction: 'desc',
      },
      { offset: 0, limit: 5 },
    );
    const file = page.nodes.find((n) => n.kind === 'file');
    expect(file).toBeTruthy();
    const { jobId } = await connector.recovery.startRecovery(
      scanId,
      { kind: 'nodes', nodeIds: [file!.id] },
      'D:\\Recovered',
      {
        verifyOnWrite: false,
        keepEmptyFolders: false,
        fileRepair: false,
        collision: 'overwrite',
        rebuildFolders: true,
      },
    );
    const finished = waitForEvent<RecoveryFinishedEvent>(
      (cb) => connector.recovery.onRecoveryFinished(jobId, cb),
      60_000,
    );
    const f = await finished;
    expect(['completed', 'completed-partial', 'cancelled']).toContain(f.status);
    await connector.scan.cancelScan(scanId).catch(() => undefined);
  }, 90_000);
});
