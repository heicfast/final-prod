import { describe, expect, it } from 'vitest';
import { MockConnector } from '@/connector/mock/mock-connector';
import { createTauriConnector } from '@/connector/tauri/tauri-connector';
import type { ResultsFilter } from '@/connector/contract';

const BASE_FILTER: ResultsFilter = {
  nodeId: null,
  treeMode: 'path',
  search: null,
  categories: null,
  dateFrom: null,
  dateTo: null,
  sizeMinBytes: null,
  sizeMaxBytes: null,
  confidence: null,
  extensions: null,
  origins: null,
};

async function waitForScanCompletion(
  c: MockConnector,
  scanId: string,
  timeoutMs = 60_000,
): Promise<void> {
  await new Promise<void>((resolve, reject) => {
    const timeout = setTimeout(
      () => reject(new Error('scan did not finish in time')),
      timeoutMs,
    );
    const unsub = c.scan.onScanProgress(scanId, (e) => {
      if (e.status === 'completed' || e.status === 'failed' || e.status === 'cancelled') {
        clearTimeout(timeout);
        unsub();
        resolve();
      }
    });
  });
}

describe('MockConnector — devices', () => {
  it('lists devices deterministically for a fixed seed', async () => {
    const a = new MockConnector();
    const b = new MockConnector();
    const listA = await a.devices.listDevices();
    const listB = await b.devices.listDevices();
    expect(listA.map((d) => d.id)).toEqual(listB.map((d) => d.id));
    expect(listA.length).toBeGreaterThan(0);
    expect(listA.every((d) => d.volumes.length > 0)).toBe(true);
  });

  it('exposes quick-scan availability truthfully (bad-sectors scenario blocks it)', async () => {
    const c = new MockConnector();
    c.setScenario('bad-sectors');
    const list = await c.devices.listDevices();
    expect(list.some((d) => d.volumes.some((v) => !v.quickScanAvailable))).toBe(true);
  });
});

describe('MockConnector — scan lifecycle', () => {
  it('runs a scan to completion with monotonic overall progress', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const target = devices[0].volumes[0].id;
    const { scanId } = await c.scan.startScan({
      targetId: target,
      mode: 'quick',
      categories: ['graphics', 'document'],
      badSectorPolicy: 'skip',
      retries: 3,
    });
    const seen: number[] = [];
    await new Promise<void>((resolve, reject) => {
      const timeout = setTimeout(
        () => reject(new Error('scan did not complete in time')),
        60_000,
      );
      const unsub = c.scan.onScanProgress(scanId, (e) => {
        seen.push(e.overallProgress);
        if (e.status === 'completed' || e.status === 'failed' || e.status === 'cancelled') {
          clearTimeout(timeout);
          unsub();
          resolve();
        }
      });
    });
    expect(seen.length).toBeGreaterThan(2);
    for (let i = 1; i < seen.length; i++) {
      expect(seen[i]).toBeGreaterThanOrEqual(seen[i - 1] - 1e-9);
    }
    const state = await c.scan.getScanState(scanId);
    expect(state.status).toBe('completed');
    expect(state.progress.filesFound).toBeGreaterThan(0);
  }, 90_000);

  it('pauses and resumes within the timing budget', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const { scanId } = await c.scan.startScan({
      targetId: devices[0].volumes[0].id,
      mode: 'deep',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await new Promise((r) => setTimeout(r, 400));
    const t0 = Date.now();
    await c.scan.pauseScan(scanId);
    expect(Date.now() - t0).toBeLessThan(600);
    const paused = await c.scan.getScanState(scanId);
    expect(paused.status).toBe('paused');
    await c.scan.resumeScan(scanId);
    const resumed = await c.scan.getScanState(scanId);
    expect(resumed.status).toBe('running');
    await c.scan.cancelScan(scanId);
  }, 30_000);

  it('cancels within 300ms budget', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const { scanId } = await c.scan.startScan({
      targetId: devices[0].volumes[0].id,
      mode: 'quick',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await new Promise((r) => setTimeout(r, 300));
    const t0 = Date.now();
    await c.scan.cancelScan(scanId);
    expect(Date.now() - t0).toBeLessThanOrEqual(300);
  }, 30_000);

  it('injects a disconnect mid-scan and recovers via resume', async () => {
    const c = new MockConnector();
    c.setScenario('disconnect-mid-scan');
    const devices = await c.devices.listDevices();
    const { scanId } = await c.scan.startScan({
      targetId: devices[0].volumes[0].id,
      mode: 'deep',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await new Promise<void>((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('no disconnect')), 45_000);
      const unsub = c.scan.onScanProgress(scanId, (e) => {
        if (!e.deviceConnected) {
          clearTimeout(timeout);
          unsub();
          resolve();
        }
      });
    });
    const state = await c.scan.getScanState(scanId);
    expect(state.status).toBe('paused');
    expect(state.progress.deviceConnected).toBe(false);
    // resume models the user having reconnected the device
    await c.scan.resumeScan(scanId);
    const after = await c.scan.getScanState(scanId);
    expect(after.progress.deviceConnected).toBe(true);
    await c.scan.cancelScan(scanId);
  }, 60_000);
});

describe('MockConnector — results', () => {
  it('pages deterministically and filters by category', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const { scanId } = await c.scan.startScan({
      targetId: devices[0].volumes[0].id,
      mode: 'quick',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await new Promise((r) => setTimeout(r, 700));
    const page1 = await c.results.queryResults(
      scanId,
      BASE_FILTER,
      { field: 'name', direction: 'asc' },
      { offset: 0, limit: 50 },
    );
    expect(page1.nodes.length).toBeGreaterThan(0);
    expect(page1.total).toBeGreaterThanOrEqual(page1.nodes.length);

    const gfx = await c.results.queryResults(
      scanId,
      { ...BASE_FILTER, categories: ['graphics'] },
      { field: 'name', direction: 'asc' },
      { offset: 0, limit: 50 },
    );
    if (gfx.nodes.length > 0) {
      expect(gfx.nodes.every((n) => n.category === 'graphics')).toBe(true);
    }

    const tree = await c.results.getResultTree(scanId, null, 'type');
    expect(tree.nodes.length).toBe(8);
    await c.scan.cancelScan(scanId);
  }, 30_000);

  it('reports filter latency under 200ms on the standard scenario (grid #36)', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const { scanId } = await c.scan.startScan({
      targetId: devices[0].volumes[0].id,
      mode: 'quick',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await new Promise((r) => setTimeout(r, 700));
    const page = await c.results.queryResults(
      scanId,
      { ...BASE_FILTER, search: 'report' },
      { field: 'name', direction: 'asc' },
      { offset: 0, limit: 200 },
    );
    expect(page.elapsedMs).toBeLessThan(200);
    await c.scan.cancelScan(scanId);
  }, 30_000);
});

describe('MockConnector — previews', () => {
  it('produces deterministic image previews and diagnostic failures', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const { scanId } = await c.scan.startScan({
      targetId: devices[0].volumes[0].id,
      mode: 'quick',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await new Promise((r) => setTimeout(r, 700));
    const page = await c.results.queryResults(
      scanId,
      BASE_FILTER,
      { field: 'name', direction: 'asc' },
      { offset: 0, limit: 100 },
    );
    const imageNode = page.nodes.find((n) => n.category === 'graphics');
    if (imageNode) {
      const p1 = await c.preview.requestPreview(scanId, imageNode.id, 'image');
      const p2 = await c.preview.requestPreview(scanId, imageNode.id, 'image');
      expect(p1.kind).toBe('image');
      if (p1.kind === 'image' && p2.kind === 'image') {
        expect(p1.dataUrl).toBe(p2.dataUrl);
      }
    }
    const damaged = page.nodes[96];
    if (damaged) {
      await expect(c.preview.requestPreview(scanId, damaged.id, 'image')).rejects.toMatchObject(
        {
          reasonKey: expect.stringContaining('preview.reason'),
        },
      );
    }
    await c.scan.cancelScan(scanId);
  }, 30_000);
});

describe('MockConnector — recovery', () => {
  it('refuses destinations that overlap the source volume', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const source = devices[0];
    const letter = source.volumes[0].letter;
    const validation = await c.system.validateDestination(
      `${letter}:\\Recovered`,
      source.id,
      1024,
    );
    expect(validation.allowed).toBe(false);
    expect(validation.reason).toBe('overlap');
  });

  it('runs a recovery job to completion with per-item states', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const source = devices[0];
    const { scanId } = await c.scan.startScan({
      targetId: source.volumes[0].id,
      mode: 'quick',
      categories: [],
      badSectorPolicy: 'skip',
      retries: 1,
    });
    await waitForScanCompletion(c, scanId);
    const page = await c.results.queryResults(
      scanId,
      BASE_FILTER,
      { field: 'name', direction: 'asc' },
      { offset: 0, limit: 6 },
    );
    expect(page.nodes.length).toBeGreaterThanOrEqual(5);
    const dest = await c.system.pickDirectory();
    const { jobId } = await c.recovery.startRecovery(
      scanId,
      page.nodes.slice(0, 5).map((n) => n.id),
      dest!,
      {
        collision: 'rename',
        rebuildFolders: true,
        keepEmptyFolders: false,
        fileRepair: true,
        verifyOnWrite: true,
      },
    );
    await new Promise<void>((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('job did not finish')), 60_000);
      const unsub = c.recovery.onRecoveryProgress(jobId, (e) => {
        if (e.status !== 'running' && e.status !== 'validating') {
          clearTimeout(timeout);
          unsub();
          resolve();
        }
      });
    });
    const report = await c.recovery.getRecoveryReport(jobId);
    const total = report.succeeded.length + report.failed.length + report.skipped.length;
    expect(total).toBe(5);
    expect(report.status).toBeOneOf(['completed', 'completed-partial']);
  }, 90_000);
});

describe('MockConnector — health & repair', () => {
  it('returns SMART tables with the documented field set', async () => {
    const c = new MockConnector();
    const devices = await c.devices.listDevices();
    const h = await c.health.getHealth(devices[0].id);
    expect(h.attributes.length).toBeGreaterThan(6);
    for (const a of h.attributes) {
      expect(a).toHaveProperty('id');
      expect(a).toHaveProperty('name');
      expect(a).toHaveProperty('raw');
      expect(a).toHaveProperty('status');
    }
    expect(['good', 'caution', 'bad']).toContain(h.status);
  });

  it('requires the typed consent token for repair', async () => {
    const c = new MockConnector();
    c.setScenario('bad-sectors');
    const devices = await c.devices.listDevices();
    const dry = await c.repair.dryRunRepair(devices[0].id);
    expect(dry.findings.length).toBeGreaterThan(0);
    await expect(
      c.repair.applyRepair(devices[0].id, { consentToken: 'YES' }),
    ).rejects.toMatchObject({
      code: 'repair-consent-required',
    });
    const { applied } = await c.repair.applyRepair(devices[0].id, { consentToken: 'REPAIR' });
    expect(applied).toBeGreaterThan(0);
    const log = await c.repair.getUndoLog(devices[0].id);
    expect(log.length).toBe(applied);
    const undo = await c.repair.undoRepairs(devices[0].id);
    expect(undo.rolledBack).toBe(applied);
  });
});

describe('Tauri stub adapter', () => {
  it('throws NotImplementedError on every domain', async () => {
    const stub = createTauriConnector();
    await expect(async () => stub.devices.listDevices()).rejects.toThrow(/not implemented/i);
    await expect(async () => stub.system.getAppInfo()).rejects.toThrow(/CONNECTOR-CONTRACT/i);
  });
});
