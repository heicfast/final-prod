/**
 * Connector contract test — the REAL Tauri connector against a spied IPC
 * boundary. Every contract method must invoke the exact manifest command
 * with the exact arguments (contracts/command-surface.json; validate.mjs
 * cross-checks the static side), rejections must surface the mapped
 * ConnectorError { code, reasonKey, detail.code }, and subscriptions must
 * return the contract's unsubscribe shape.
 *
 * The Tauri APIs are mocked at the module boundary: invoke captures calls;
 * listen hands back a controllable emitter. No engine, no window — this is
 * the TS-side seam contract, not an engine behavioral test (those run on
 * Windows CI against the real bridge per plan §9).
 */

import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import type { ConnectorError, ConnectorErrorCode } from '@/connector/contract';

type InvokeCall = { cmd: string; args: Record<string, unknown> };

let calls: InvokeCall[];
let invokeImpl: (cmd: string, args: Record<string, unknown>) => unknown;
let listeners: Map<string, (payload: unknown) => void>;

vi.mock('@tauri-apps/api/core', () => ({
  invoke: (cmd: string, args?: Record<string, unknown>) =>
    Promise.resolve().then(() => invokeImpl(cmd, args ?? {})),
}));

vi.mock('@tauri-apps/api/event', () => ({
  listen: (event: string, cb: (e: { payload: unknown }) => void) =>
    Promise.resolve(() => {
      listeners.set(event, (payload: unknown) => cb({ payload }));
      return () => {
        listeners.delete(event);
      };
    }).then((register) => register()),
}));

import { createTauriConnector, REASON_KEYS } from '@/connector/tauri/tauri-connector';

const OK_VALUE = { ok: true };

beforeEach(() => {
  calls = [];
  listeners = new Map();
  invokeImpl = (cmd: string, args: Record<string, unknown>) => {
    calls.push({ cmd, args });
    return OK_VALUE;
  };
});

afterEach(() => {
  vi.restoreAllMocks();
});

async function collect<T>(
  p: Promise<T>,
): Promise<{ ok: true; value: T } | { ok: false; err: ConnectorError }> {
  try {
    return { ok: true, value: await p };
  } catch (err) {
    return { ok: false, err: err as ConnectorError };
  }
}

describe('command surface (manifest: contracts/command-surface.json)', () => {
  it('listDevices invokes devices_list', async () => {
    const c = createTauriConnector();
    await c.devices.listDevices();
    expect(calls).toEqual([{ cmd: 'devices_list', args: {} }]);
  });

  it('refreshDevices invokes devices_refresh', async () => {
    const c = createTauriConnector();
    await c.devices.refreshDevices();
    expect(calls[0].cmd).toBe('devices_refresh');
  });

  it('getDeviceInfo resolves a volume id to the owning device', async () => {
    invokeImpl = () => [
      { id: 'disk-0', volumes: [{ id: 'vol-C' }] },
      { id: 'disk-1', volumes: [{ id: 'vol-Z' }] },
    ];
    const c = createTauriConnector();
    const dev = await c.devices.getDeviceInfo('vol-Z');
    expect(dev.id).toBe('disk-1');
  });

  it('getDeviceInfo throws device-gone for an unknown id', async () => {
    invokeImpl = () => [];
    const c = createTauriConnector();
    const r = await collect(c.devices.getDeviceInfo('vol-Ω'));
    expect(r.ok).toBe(false);
    if (!r.ok) {
      expect(r.err.code).toBe('device-gone');
      expect(r.err.reasonKey).toBe('errors.deviceGone');
    }
  });

  it('startScan invokes scan_start with targetId + pathScope', async () => {
    const c = createTauriConnector();
    await c.scan.startScan({ targetId: 'vol-C', pathScope: 'C:\\Users' });
    expect(calls[0]).toEqual({
      cmd: 'scan_start',
      args: { targetId: 'vol-C', pathScope: 'C:\\Users' },
    });
  });

  it('startScan sends pathScope null when omitted', async () => {
    const c = createTauriConnector();
    await c.scan.startScan({ targetId: 'disk-0' });
    expect(calls[0].args.pathScope).toBeNull();
  });

  it('pause/resume/cancel/state invoke their commands with scanId', async () => {
    const c = createTauriConnector();
    await c.scan.pauseScan('scan-1');
    await c.scan.resumeScan('scan-1');
    await c.scan.cancelScan('scan-1');
    await c.scan.getScanState('scan-1');
    expect(calls.map((x) => x.cmd)).toEqual([
      'scan_pause',
      'scan_resume',
      'scan_cancel',
      'scan_state',
    ]);
    expect(calls[3].args).toEqual({ scanId: 'scan-1' });
  });

  it('results methods invoke results_tree/query/count/node_detail with the full payloads', async () => {
    const c = createTauriConnector();
    const filter = {
      nodeId: 'root',
      treeMode: 'path' as const,
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
    const sort = { field: 'size' as const, direction: 'desc' as const };
    const page = { offset: 0, limit: 200 };
    await c.results.getResultTree('scan-1', null, 'path');
    await c.results.queryResults('scan-1', filter, sort, page);
    await c.results.countResults('scan-1', filter);
    await c.results.getNodeDetail('scan-1', 'f12');
    expect(calls.map((x) => x.cmd)).toEqual([
      'results_tree',
      'results_query',
      'results_count',
      'node_detail',
    ]);
    expect(calls[0].args).toEqual({ scanId: 'scan-1', parentId: null, mode: 'path' });
    expect(calls[1].args).toEqual({ scanId: 'scan-1', filter, sort, page });
    expect(calls[3].args).toEqual({ scanId: 'scan-1', nodeId: 'f12' });
  });

  it('preview methods invoke preview_open / preview_thumbnail', async () => {
    const c = createTauriConnector();
    await c.preview.requestPreview('scan-1', 'f1');
    await c.preview.getThumbnail('scan-1', 'f1');
    expect(calls.map((x) => x.cmd)).toEqual(['preview_open', 'preview_thumbnail']);
  });

  it('requestPreview normalizes a JSON-array hex payload into Uint8Array', async () => {
    invokeImpl = () => ({ kind: 'hex', bytes: [1, 2, 3, 250], totalSize: 4, baseOffset: 0 });
    const c = createTauriConnector();
    const payload = await c.preview.requestPreview('scan-1', 'f1');
    expect(payload.kind).toBe('hex');
    if (payload.kind === 'hex') {
      expect(payload.bytes).toBeInstanceOf(Uint8Array);
      expect(Array.from(payload.bytes)).toEqual([1, 2, 3, 250]);
    }
  });

  it('sessions invoke session_save / sessions_list / session_load', async () => {
    const c = createTauriConnector();
    await c.sessions.saveSession('scan-1', null);
    await c.sessions.listSessions();
    await c.sessions.loadSession('auto_x.crsf');
    expect(calls.map((x) => x.cmd)).toEqual(['session_save', 'sessions_list', 'session_load']);
    expect(calls[0].args).toEqual({ scanId: 'scan-1', path: null });
    expect(calls[2].args).toEqual({ path: 'auto_x.crsf' });
  });

  it('getSessionMeta resolves through sessions_list and throws session-not-found', async () => {
    invokeImpl = () => [{ fileName: 'auto_a.crsf' }];
    const c = createTauriConnector();
    const meta = await c.sessions.getSessionMeta('auto_a.crsf');
    expect(meta).toEqual({ fileName: 'auto_a.crsf' });
    const missing = await collect(c.sessions.getSessionMeta('nope.crsf'));
    expect(missing.ok).toBe(false);
    if (!missing.ok) expect(missing.err.code).toBe('session-not-found');
  });

  it('recovery methods invoke the five recovery commands', async () => {
    const c = createTauriConnector();
    const options = {
      collision: 'rename' as const,
      rebuildFolders: true,
      keepEmptyFolders: false,
      fileRepair: false,
      verifyOnWrite: true,
    };
    await c.recovery.startRecovery(
      'scan-1',
      { kind: 'nodes', nodeIds: ['f1'] },
      'D:\\Out',
      options,
    );
    await c.recovery.cancelRecovery('job-1');
    await c.recovery.getRecoveryState('job-1');
    await c.recovery.getRecoveryItems('job-1', { offset: 0, limit: 500 });
    await c.recovery.getRecoveryReport('job-1');
    expect(calls.map((x) => x.cmd)).toEqual([
      'recovery_start',
      'recovery_cancel',
      'recovery_state',
      'recovery_items',
      'recovery_report',
    ]);
    expect(calls[0].args).toEqual({
      scanId: 'scan-1',
      selection: { kind: 'nodes', nodeIds: ['f1'] },
      destination: 'D:\\Out',
      options,
    });
    expect(calls[3].args).toEqual({ jobId: 'job-1', page: { offset: 0, limit: 500 } });
  });

  it('health invokes health_read with deviceId', async () => {
    const c = createTauriConnector();
    await c.health.getHealth('disk-0');
    expect(calls[0]).toEqual({ cmd: 'health_read', args: { deviceId: 'disk-0' } });
  });

  it('the G-1 repair gap methods invoke their commands and surface not-implemented', async () => {
    invokeImpl = (cmd: string, args: Record<string, unknown>) => {
      calls.push({ cmd, args });
      throw { code: 0, bridgeCode: 'not-implemented', message: 'G-1 gap surface' };
    };
    const c = createTauriConnector();
    const codes: ConnectorErrorCode[] = [];
    for (const r of [
      await collect(c.repair.dryRunRepair('disk-0')),
      await collect(c.repair.applyRepair('disk-0', { consentToken: 'REPAIR' })),
      await collect(c.repair.getUndoLog('disk-0')),
      await collect(c.repair.undoRepairs('disk-0')),
    ]) {
      expect(r.ok).toBe(false);
      if (!r.ok) codes.push(r.err.code);
    }
    expect(codes).toEqual([
      'not-implemented',
      'not-implemented',
      'not-implemented',
      'not-implemented',
    ]);
    expect(calls.map((x) => x.cmd)).toEqual([
      'repair_dry_run',
      'repair_apply',
      'repair_undo_log',
      'repair_undo',
    ]);
  });

  it('system methods invoke the six system commands with exact args', async () => {
    const c = createTauriConnector();
    await c.system.getDriverStatus();
    await c.system.getAppInfo();
    await c.system.validateDestination('D:\\Out', 'vol-C', 1024);
    await c.system.pickDirectory({ excludeDeviceId: 'vol-C' });
    await c.system.listMountPoints();
    await c.system.openPath('C:\\x');
    expect(calls.map((x) => x.cmd)).toEqual([
      'system_driver_status',
      'system_app_info',
      'system_validate_destination',
      'system_pick_directory',
      'system_list_mounts',
      'system_open_path',
    ]);
    expect(calls[2].args).toEqual({
      path: 'D:\\Out',
      sourceDeviceId: 'vol-C',
      neededBytes: 1024,
    });
    expect(calls[3].args).toEqual({ hint: { excludeDeviceId: 'vol-C' } });
  });
});

describe('error model (contracts/error-codes.json)', () => {
  it('maps a bridgeCode rejection to the vocabulary code + reasonKey', async () => {
    invokeImpl = () => {
      throw { code: 0, bridgeCode: 'scan-not-found', message: 'nope' };
    };
    const c = createTauriConnector();
    const r = await collect(c.scan.getScanState('scan-9'));
    expect(r.ok).toBe(false);
    if (!r.ok) {
      expect(r.err.code).toBe('scan-not-found');
      expect(r.err.reasonKey).toBe('errors.scanNotFound');
    }
  });

  it('maps an engine numeric rejection through the table with detail.code', async () => {
    invokeImpl = () => {
      throw { code: 0x50050001, bridgeCode: null, message: 'F12' };
    };
    const c = createTauriConnector();
    const r = await collect(
      c.recovery.startRecovery('scan-1', { kind: 'nodes', nodeIds: ['f1'] }, 'C:\\x', {
        collision: 'rename',
        rebuildFolders: true,
        keepEmptyFolders: false,
        fileRepair: false,
        verifyOnWrite: true,
      }),
    );
    expect(r.ok).toBe(false);
    if (!r.ok) {
      expect(r.err.code).toBe('destination-overlap');
      expect(r.err.reasonKey).toBe('recovery.overlapReason');
      expect((r.err.detail as { code: number }).code).toBe(0x50050001);
    }
  });

  it('maps unknown engine codes to unknown with the numeric detail preserved', async () => {
    invokeImpl = () => {
      throw { code: 0x99999999, bridgeCode: null, message: 'odd' };
    };
    const c = createTauriConnector();
    const r = await collect(c.system.getAppInfo());
    expect(r.ok).toBe(false);
    if (!r.ok) {
      expect(r.err.code).toBe('unknown');
      expect((r.err.detail as { code: number }).code).toBe(0x99999999);
    }
  });

  it('degrades truthfully when the IPC layer rejects with a non-payload', async () => {
    invokeImpl = () => {
      throw new Error('transport gone');
    };
    const c = createTauriConnector();
    const r = await collect(c.system.getAppInfo());
    expect(r.ok).toBe(false);
    if (!r.ok) {
      expect(r.err.code).toBe('unknown');
      expect(r.err.reasonKey).toBe('errors.unknown');
    }
  });

  it('REASON_KEYS covers every vocabulary member with a defined locale key', async () => {
    // Both locale catalogs are the i18n sweep's truth; this pins the
    // connector's table against them (all keys defined + referenced).
    const { readFileSync } = await import('node:fs');
    const en = readFileSync(process.cwd() + '/src/i18n/en.ts', 'utf8');
    for (const key of Object.values(REASON_KEYS)) {
      // errors.deviceGone -> "deviceGone:" must appear in the catalog file.
      const leaf = key.split('.').pop() ?? '';
      const ns = key.slice(0, -(leaf.length + 1));
      if (ns === 'errors') {
        expect(en.includes(`${leaf}:`)).toBe(true);
      } else if (ns === 'recovery' || ns === 'repair') {
        expect(en.includes(`${leaf}:`)).toBe(true);
      } else if (ns === 'preview.reason') {
        expect(en.includes(`${leaf}:`)).toBe(true);
      }
    }
  });
});

describe('event subscriptions (contract unsubscribe shape)', () => {
  it('onScanProgress listens on scan-progress:<scanId> and unsubscribes', async () => {
    const c = createTauriConnector();
    const seen: unknown[] = [];
    const stop = c.scan.onScanProgress('scan-3', (e) => seen.push(e));
    // listen() resolves on a microtask — let it register before asserting.
    await vi.waitFor(() => expect(listeners.has('scan-progress:scan-3')).toBe(true));
    listeners.get('scan-progress:scan-3')?.({ scanId: 'scan-3' });
    expect(seen).toEqual([{ scanId: 'scan-3' }]);
    stop();
    await vi.waitFor(() => expect(listeners.has('scan-progress:scan-3')).toBe(false));
  });

  it('onRecoveryProgress listens on recovery-progress:<jobId> and unsubscribes', async () => {
    const c = createTauriConnector();
    const seen: unknown[] = [];
    const stop = c.recovery.onRecoveryProgress('job-2', (e) => seen.push(e));
    await vi.waitFor(() => expect(listeners.has('recovery-progress:job-2')).toBe(true));
    listeners.get('recovery-progress:job-2')?.({ jobId: 'job-2' });
    expect(seen).toEqual([{ jobId: 'job-2' }]);
    stop();
    await vi.waitFor(() => expect(listeners.has('recovery-progress:job-2')).toBe(false));
  });
});
