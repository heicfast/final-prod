/**
 * Connector contract test — the REAL Tauri connector against a spied IPC
 * boundary. Every contract method must invoke the exact manifest command
 * with the exact arguments (contracts/command-surface.json; validate.mjs
 * phase 2 cross-checks the static side), rejections must surface the mapped
 * ConnectorError { code, reasonKey, detail.code }, and subscriptions must
 * return the contract's unsubscribe shape.
 *
 * The Tauri APIs are mocked at the module boundary: invoke captures calls;
 * listen hands back a controllable emitter. No engine, no window — this is
 * the TS-side seam contract, not an engine behavioral test (those run on
 * Windows CI against the real bridge per the plan §9 gates).
 */

import { beforeEach, describe, expect, it, vi } from 'vitest';
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
  listen: (event: string, cb: (e: { payload: unknown }) => void) => {
    listeners.set(event, (payload: unknown) => cb({ payload }));
    return Promise.resolve(() => {
      listeners.delete(event);
    });
  },
}));

import { createTauriConnector, REASON_KEYS } from '@/connector/tauri/tauri-connector';

const conn = createTauriConnector();

function lastCall(): InvokeCall {
  return calls[calls.length - 1];
}

beforeEach(() => {
  calls = [];
  listeners = new Map();
  invokeImpl = (cmd: string, args: Record<string, unknown>) => {
    calls.push({ cmd, args });
    return null;
  };
});

describe('the tauri connector command surface (the manifest)', () => {
  it('every domain method is a function (the wired-domains gate)', () => {
    for (const [domain, methods] of Object.entries(conn)) {
      for (const [name, fn] of Object.entries(methods as Record<string, unknown>)) {
        expect(typeof fn, `${domain}.${name} is a function`).toBe('function');
      }
    }
  });

  it('devices invoke the manifest commands', async () => {
    await conn.devices.listDevices();
    expect(lastCall().cmd).toBe('devices_list');
    await conn.devices.refreshDevices();
    expect(lastCall().cmd).toBe('devices_refresh');
    await conn.devices.unlockVolume('vol-E', ['pw1', 'pw2']);
    expect(lastCall()).toEqual({
      cmd: 'volume_unlock_dictionary',
      args: { volumeId: 'vol-E', passwords: ['pw1', 'pw2'] },
    });
  });

  it('scan invokes the manifest commands with exact args', async () => {
    await conn.scan.startScan({
      targetId: 'disk-1',
      pathScope: 'C:\\Users',
      bitlockerFvek: 'ab',
    });
    expect(lastCall()).toEqual({
      cmd: 'scan_start',
      args: { targetId: 'disk-1', pathScope: 'C:\\Users', bitlockerFvek: 'ab' },
    });
    await conn.scan.startScan({ targetId: 'vol-C' });
    expect(lastCall().args).toEqual({
      targetId: 'vol-C',
      pathScope: null,
      bitlockerFvek: null,
    });
    await conn.scan.pauseScan('scan-1');
    expect(lastCall()).toEqual({ cmd: 'scan_pause', args: { scanId: 'scan-1' } });
    await conn.scan.resumeScan('scan-1');
    expect(lastCall().cmd).toBe('scan_resume');
    await conn.scan.cancelScan('scan-1');
    expect(lastCall().cmd).toBe('scan_cancel');
    await conn.scan.getScanState('scan-1');
    expect(lastCall()).toEqual({ cmd: 'scan_state', args: { scanId: 'scan-1' } });
    await conn.scan.respondToConsent('consent-1', true);
    expect(lastCall()).toEqual({
      cmd: 'consent_respond',
      args: { requestId: 'consent-1', allow: true },
    });
  });

  it('results invoke the manifest commands', async () => {
    const filter = {
      nodeId: null,
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
    await conn.results.getResultTree('scan-1', null, 'path');
    expect(lastCall()).toEqual({
      cmd: 'results_tree',
      args: { scanId: 'scan-1', parentId: null, mode: 'path' },
    });
    await conn.results.queryResults(
      'scan-1',
      filter,
      { field: 'size', direction: 'desc' },
      { offset: 0, limit: 200 },
    );
    expect(lastCall()).toEqual({
      cmd: 'results_query',
      args: {
        scanId: 'scan-1',
        filter,
        sort: { field: 'size', direction: 'desc' },
        page: { offset: 0, limit: 200 },
      },
    });
    await conn.results.countResults('scan-1', filter);
    expect(lastCall().cmd).toBe('results_count');
    await conn.results.getNodeDetail('scan-1', 'f12');
    expect(lastCall()).toEqual({
      cmd: 'node_detail',
      args: { scanId: 'scan-1', nodeId: 'f12' },
    });
  });

  it('preview + sessions invoke the manifest commands', async () => {
    invokeImpl = (cmd: string, args: Record<string, unknown>) => {
      calls.push({ cmd, args });
      return { kind: 'hex', bytes: [1, 2], totalSize: 2, baseOffset: 0 };
    };
    await conn.preview.requestPreview('scan-1', 'f3');
    expect(lastCall()).toEqual({
      cmd: 'preview_open',
      args: { scanId: 'scan-1', nodeId: 'f3' },
    });
    await conn.preview.getThumbnail('scan-1', 'f3');
    expect(lastCall().cmd).toBe('preview_thumbnail');

    await conn.sessions.saveSession('scan-1', null);
    expect(lastCall()).toEqual({ cmd: 'session_save', args: { scanId: 'scan-1', path: null } });
    await conn.sessions.cancelSave('scan-1');
    expect(lastCall()).toEqual({ cmd: 'scan_save_cancel', args: { scanId: 'scan-1' } });
    await conn.sessions.listSessions();
    expect(lastCall().cmd).toBe('sessions_list');
    await conn.sessions.loadSession('auto_x.crsf');
    expect(lastCall()).toEqual({ cmd: 'session_load', args: { path: 'auto_x.crsf' } });
    await conn.sessions.getSessionMeta('auto_x.crsf');
    expect(lastCall()).toEqual({ cmd: 'session_meta', args: { path: 'auto_x.crsf' } });
    await conn.sessions.resumeLastSession();
    expect(lastCall().cmd).toBe('session_resume_last');
    await conn.sessions.resumeSession('auto_x.crsf');
    expect(lastCall()).toEqual({ cmd: 'session_resume', args: { path: 'auto_x.crsf' } });
  });

  it('recovery + health + repair invoke the manifest commands', async () => {
    await conn.recovery.startRecovery(
      'scan-1',
      { kind: 'nodes', nodeIds: ['f1', 'd2'] },
      'D:\\Out',
      {
        collision: 'rename',
        rebuildFolders: true,
        keepEmptyFolders: false,
        fileRepair: false,
        verifyOnWrite: true,
      },
    );
    expect(lastCall().cmd).toBe('recovery_start');
    await conn.recovery.cancelRecovery('job-1');
    expect(lastCall()).toEqual({ cmd: 'recovery_cancel', args: { jobId: 'job-1' } });
    await conn.recovery.getRecoveryState('job-1');
    expect(lastCall().cmd).toBe('recovery_state');
    await conn.recovery.getRecoveryItems('job-1', { offset: 0, limit: 100 });
    expect(lastCall().cmd).toBe('recovery_items');
    await conn.recovery.getRecoveryReport('job-1');
    expect(lastCall().cmd).toBe('recovery_report');
    await conn.health.getHealth('disk-0');
    expect(lastCall()).toEqual({ cmd: 'health_read', args: { deviceId: 'disk-0' } });
    await conn.repair.dryRunRepair('disk-0');
    expect(lastCall()).toEqual({ cmd: 'repair_dry_run', args: { deviceId: 'disk-0' } });
    await conn.repair.applyRepair('disk-0', { consentToken: 'tok' });
    expect(lastCall().cmd).toBe('repair_apply');
    await conn.repair.getUndoLog('disk-0');
    expect(lastCall().cmd).toBe('repair_undo_log');
    await conn.repair.undoRepairs('disk-0');
    expect(lastCall().cmd).toBe('repair_undo');
  });

  it('lostPartition invokes the manifest commands (the S-029 surface)', async () => {
    const candidate = { lba: 4096, sectorCount: 204800, fs: 0, score: 85 };
    await conn.lostPartition.applyLostPartition('disk-1', candidate, true);
    expect(lastCall()).toEqual({
      cmd: 'lost_partition_apply',
      args: { deviceId: 'disk-1', candidate, rebuildTable: true },
    });
    await conn.lostPartition.undoLastApply('disk-1');
    expect(lastCall()).toEqual({ cmd: 'lost_partition_undo', args: { deviceId: 'disk-1' } });
    await conn.lostPartition.saveApplyUndo('disk-1', 'C:\\undo.bin');
    expect(lastCall()).toEqual({
      cmd: 'lost_partition_save_undo',
      args: { deviceId: 'disk-1', path: 'C:\\undo.bin' },
    });
    await conn.lostPartition.respondToApplyConsent('apply-1', false);
    expect(lastCall()).toEqual({
      cmd: 'consent_respond',
      args: { requestId: 'apply-1', allow: false },
    });
    await conn.lostPartition.pendingApplyConsent('disk-1');
    expect(lastCall()).toEqual({
      cmd: 'apply_consent_pending',
      args: { deviceId: 'disk-1' },
    });
  });

  it('system invokes the manifest commands', async () => {
    await conn.system.getAppInfo();
    expect(lastCall().cmd).toBe('system_app_info');
    await conn.system.loadUserSigns('{ ".xyz": { "signs": ["0,AA"] } }');
    expect(lastCall()).toEqual({
      cmd: 'system_load_user_signs',
      args: { content: '{ ".xyz": { "signs": ["0,AA"] } }' },
    });
    await conn.system.validateDestination('D:\\Out', 'disk-1', 1024);
    expect(lastCall()).toEqual({
      cmd: 'system_validate_destination',
      args: { path: 'D:\\Out', sourceDeviceId: 'disk-1', neededBytes: 1024 },
    });
    await conn.system.pickDirectory({ volumeId: 'vol-C', excludeDeviceId: null });
    expect(lastCall()).toEqual({
      cmd: 'system_pick_directory',
      args: { hint: { volumeId: 'vol-C', excludeDeviceId: null } },
    });
    await conn.system.pickDirectory();
    expect(lastCall().args).toEqual({ hint: null });
    await conn.system.listMountPoints();
    expect(lastCall().cmd).toBe('system_list_mounts');
    await conn.system.openPath('D:\\Out');
    expect(lastCall()).toEqual({ cmd: 'system_open_path', args: { path: 'D:\\Out' } });
  });
});

describe('the tauri connector error model', () => {
  it('maps bridgeCode rejections directly', async () => {
    invokeImpl = () => {
      throw { code: 0, bridgeCode: 'device-permission', message: 'locked' };
    };
    const err = (await conn.devices.listDevices().catch((e: unknown) => e)) as ConnectorError;
    expect(err.code).toBe('device-permission');
    expect(err.reasonKey).toBe('errors.devicePermission');
  });

  it('maps numeric engine codes through the table', async () => {
    const cases: readonly [number, ConnectorErrorCode, string][] = [
      [0x50020007, 'device-gone', 'errors.deviceGone'],
      [0x500c0001, 'elevation-required', 'errors.elevationRequired'],
      [0x50050001, 'destination-overlap', 'recovery.overlapReason'],
      [0x50060001, 'device-permission', 'errors.devicePermission'],
      [0x50070001, 'health-unavailable', 'errors.healthUnavailable'],
      [0x50090001, 'unknown', 'errors.unknown'],
    ];
    for (const [numeric, code, reasonKey] of cases) {
      invokeImpl = () => {
        throw { code: numeric, bridgeCode: null, message: 'engine' };
      };
      const err = (await conn.health
        .getHealth('disk-0')
        .catch((e: unknown) => e)) as ConnectorError;
      expect(err.code, `0x${numeric.toString(16)}`).toBe(code);
      expect(err.reasonKey).toBe(reasonKey);
      expect((err.detail as { code: number } | undefined)?.code).toBe(numeric);
    }
  });

  it('unknown numeric codes degrade to unknown with a message', async () => {
    invokeImpl = () => {
      throw { code: 0xdeadbeef };
    };
    const err = (await conn.devices.listDevices().catch((e: unknown) => e)) as ConnectorError;
    expect(err.code).toBe('unknown');
    expect(err.message).toContain('without a message');
  });

  it('REASON_KEYS covers the entire ConnectorErrorCode vocabulary', () => {
    const vocabulary: readonly ConnectorErrorCode[] = [
      'device-gone',
      'device-permission',
      'elevation-required',
      'scan-not-found',
      'scan-not-paused',
      'scan-not-running',
      'scan-scope-invalid',
      'consent-not-found',
      'session-corrupt',
      'session-not-found',
      'destination-overlap',
      'destination-invalid',
      'destination-unwritable',
      'out-of-space',
      'preview-unsupported',
      'preview-too-large',
      'preview-damaged',
      'preview-encrypted',
      'preview-timeout',
      'preview-missing-data',
      'recovery-item-failed',
      'repair-consent-required',
      'repair-failed',
      'health-unavailable',
      'not-implemented',
      'unknown',
    ];
    for (const code of vocabulary) {
      expect(typeof REASON_KEYS[code], `REASON_KEYS[${code}]`).toBe('string');
      expect(REASON_KEYS[code].length).toBeGreaterThan(0);
    }
  });
});

describe('the tauri connector subscriptions', () => {
  it('subscribes to the scan/recovery/consent/apply events and unsubscribes', async () => {
    let seen: string | null = null;
    const off1 = conn.scan.onScanProgress('scan-1', () => undefined);
    const off2 = conn.scan.onConsentRequest('scan-1', () => undefined);
    const off3 = conn.recovery.onRecoveryProgress('job-1', () => undefined);
    const off4 = conn.lostPartition.onApplyConsent('disk-1', () => undefined);
    await new Promise((r) => setTimeout(r, 0));
    expect(listeners.has('scan-progress:scan-1')).toBe(true);
    expect(listeners.has('consent-request:scan-1')).toBe(true);
    expect(listeners.has('recovery-progress:job-1')).toBe(true);
    expect(listeners.has('apply-consent-request:disk-1')).toBe(true);

    // payloads cross to the callback verbatim
    const cb = (p: unknown): void => {
      seen = JSON.stringify(p);
    };
    listeners.set('scan-progress:scan-9', cb);
    listeners.get('scan-progress:scan-9')?.({ scanId: 'scan-9', status: 'running' });
    expect(seen).toContain('running');

    for (const off of [off1, off2, off3, off4]) off();
    // the unlisten promises resolve on the next microtask
    await new Promise((r) => setTimeout(r, 0));
    expect(listeners.has('scan-progress:scan-1')).toBe(false);
    expect(listeners.has('consent-request:scan-1')).toBe(false);
    expect(listeners.has('recovery-progress:job-1')).toBe(false);
    expect(listeners.has('apply-consent-request:disk-1')).toBe(false);
  });

  it('normalizes hex preview bytes to Uint8Array', async () => {
    invokeImpl = () => ({
      kind: 'hex',
      bytes: [104, 105],
      totalSize: 2,
      baseOffset: 0,
    });
    const payload = await conn.preview.requestPreview('scan-1', 'f1');
    expect(payload.kind).toBe('hex');
    expect(payload.kind === 'hex' && payload.bytes instanceof Uint8Array).toBe(true);
    expect(payload.kind === 'hex' && Array.from(payload.bytes)).toEqual([104, 105]);
  });

  it('a null preview rejects with the typed missing-data error', async () => {
    invokeImpl = () => null;
    const err = (await conn.preview
      .requestPreview('scan-1', 'f1')
      .catch((e: unknown) => e)) as ConnectorError;
    expect(err.code).toBe('preview-missing-data');
  });
});

// ---------------------------------------------------------------------------
// Lane C — the API-expansion surface (imaging, diagnostics, maintenance)
// ---------------------------------------------------------------------------

describe('the tauri connector lane C surface (the manifest)', () => {
  it('imaging invokes the manifest commands', async () => {
    await conn.imaging.startImageCreation('disk-1', 'E:\\backup.img');
    expect(lastCall()).toEqual({
      cmd: 'image_create_start',
      args: { deviceId: 'disk-1', path: 'E:\\backup.img' },
    });
    await conn.imaging.getImageState('img-1');
    expect(lastCall()).toEqual({
      cmd: 'image_create_state',
      args: { imageId: 'img-1' },
    });
  });

  it('the observability methods invoke the manifest commands', async () => {
    await conn.scan.getScanTelemetry('scan-1');
    expect(lastCall()).toEqual({ cmd: 'scan_telemetry', args: { scanId: 'scan-1' } });
    await conn.scan.getScanAssistantStats('scan-1');
    expect(lastCall()).toEqual({
      cmd: 'scan_assistant_stats',
      args: { scanId: 'scan-1' },
    });
    invokeImpl = (cmd: string, args: Record<string, unknown>) => {
      calls.push({ cmd, args });
      return { formats: [], count: 0 };
    };
    await conn.scan.listCarveFormats();
    expect(lastCall()).toEqual({ cmd: 'carve_catalog', args: {} });
    invokeImpl = (cmd: string, args: Record<string, unknown>) => {
      calls.push({ cmd, args });
      return { slots: [], slotCount: 0 };
    };
    await conn.scan.getCarveTally('scan-1');
    expect(lastCall()).toEqual({ cmd: 'scan_carve_tally', args: { scanId: 'scan-1' } });
    await conn.scan.getDiskMap('scan-1', { maxCells: 1024 });
    expect(lastCall()).toEqual({
      cmd: 'disk_map',
      args: { scanId: 'scan-1', options: { maxCells: 1024 } },
    });
    await conn.scan.getDiskMap('scan-1');
    expect(lastCall().args).toEqual({ scanId: 'scan-1', options: null });
    await conn.results.regradeNodes('scan-1', ['f1', 'f2']);
    expect(lastCall()).toEqual({
      cmd: 'session_regrade',
      args: { scanId: 'scan-1', nodeIds: ['f1', 'f2'] },
    });
  });

  it('system.getDeviceTransport + maintenance invoke the manifest commands', async () => {
    await conn.system.getDeviceTransport('disk-1');
    expect(lastCall()).toEqual({
      cmd: 'device_transport',
      args: { deviceId: 'disk-1' },
    });
    invokeImpl = (cmd: string, args: Record<string, unknown>) => {
      calls.push({ cmd, args });
      return {
        letter: 'E:',
        exitCode: 0,
        verdict: 'clean',
        verdictKey: 'maintenance.fsckClean',
        fix: true,
      };
    };
    await conn.maintenance.runFsck('E', true);
    expect(lastCall()).toEqual({
      cmd: 'volume_fsck',
      args: { letter: 'E', fix: true },
    });
    await conn.maintenance.turnOffBitlocker('E');
    expect(lastCall()).toEqual({
      cmd: 'volume_bitlocker_off',
      args: { letter: 'E' },
    });
  });

  it('guards the fsck verdictKey through the known-verdict map', async () => {
    invokeImpl = () => ({
      letter: 'E:',
      exitCode: 3,
      verdict: 'errors-remain',
      verdictKey: 'maintenance.fsckErrorsRemain',
      fix: false,
    });
    const known = await conn.maintenance.runFsck('E');
    expect(known.verdictKey).toBe('maintenance.fsckErrorsRemain');
    invokeImpl = () => ({
      letter: 'E:',
      exitCode: 7,
      verdict: 'verdict-from-the-future',
      verdictKey: 'maintenance.fsckErrorsRemain',
      fix: false,
    });
    const unknown = await conn.maintenance.runFsck('E');
    // a stale bridge build never puts a raw key on screen
    expect(unknown.verdictKey).toBe('errors.unknown');
  });

  it('subscribes to the image events and unsubscribes', async () => {
    const off1 = conn.imaging.onImageProgress('img-1', () => undefined);
    const off2 = conn.imaging.onImageFinished('img-1', () => undefined);
    await new Promise((r) => setTimeout(r, 0));
    expect(listeners.has('image-progress:img-1')).toBe(true);
    expect(listeners.has('image-finished:img-1')).toBe(true);
    off1();
    off2();
    await new Promise((r) => setTimeout(r, 0));
    expect(listeners.has('image-progress:img-1')).toBe(false);
    expect(listeners.has('image-finished:img-1')).toBe(false);
  });

  it('maps the lane C bridge codes (image-not-found, overlap, out-of-space)', async () => {
    invokeImpl = () => {
      throw { code: 0, bridgeCode: 'image-not-found', message: 'no such job' };
    };
    const err = (await conn.imaging
      .getImageState('img-9')
      .catch((e: unknown) => e)) as ConnectorError;
    expect(err.code).toBe('image-not-found');
    expect(err.reasonKey).toBe('errors.imageNotFound');

    invokeImpl = () => {
      throw { code: 0, bridgeCode: 'destination-overlap', message: 'on the source disk' };
    };
    const overlap = (await conn.imaging
      .startImageCreation('disk-1', 'C:\\x.img')
      .catch((e: unknown) => e)) as ConnectorError;
    expect(overlap.code).toBe('destination-overlap');
    expect(overlap.reasonKey).toBe('recovery.overlapReason');

    invokeImpl = () => {
      throw { code: 0, bridgeCode: 'out-of-space', message: 'not enough room' };
    };
    const space = (await conn.imaging
      .startImageCreation('disk-1', 'E:\\x.img')
      .catch((e: unknown) => e)) as ConnectorError;
    expect(space.code).toBe('out-of-space');
    expect(space.reasonKey).toBe('recovery.insufficientSpaceBody');
  });
});
