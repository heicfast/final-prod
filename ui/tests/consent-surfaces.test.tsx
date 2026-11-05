import { describe, expect, it, vi } from 'vitest';
import { render, screen, fireEvent, waitFor } from '@testing-library/react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { MockConnector } from '@/connector/mock/mock-connector';
import { useConsentStore } from '@/stores/consent-store';
import { UnlockVolumeDialog } from '@/features/device-selection/unlock-volume-dialog';
import { ConsentModal } from '@/features/consent/consent-modal';
import { ElevationBanner } from '@/features/consent/elevation-banner';
import { connector } from '@/connector';
import type { ReactElement } from 'react';

/**
 * The P4 surface tests: the consent machinery (store + modal + the mock's
 * echo-token flow), the elevation banner, and the N-064 unlock dialog —
 * every NEW wiring proven against the TEST DOUBLE (the real-adapter seam
 * is proven by connector-tauri.test.ts; the Rust side by ui-ci's cargo
 * check/clippy; the end-to-end flow by e2e/consent.spec.ts).
 */

function wrap(ui: ReactElement): { unmount: () => void } {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false }, mutations: { retry: false } },
  });
  const { unmount } = render(<QueryClientProvider client={qc}>{ui}</QueryClientProvider>);
  return { unmount };
}

describe('the consent store (both families)', () => {
  it('sets, re-hydrates, and clears per request id', () => {
    const s = useConsentStore.getState();
    s.setRepair({
      scanId: 'scan-1',
      requestId: 'consent-1',
      plan: {
        detected: 1,
        fsFamily: 'ntfs',
        actionCount: 1,
        refused: 0,
        reason: '',
        advisoryCount: 0,
      },
    });
    expect(useConsentStore.getState().repair?.requestId).toBe('consent-1');
    // a different id does not clear another family's prompt
    useConsentStore.getState().clearApply('apply-9');
    expect(useConsentStore.getState().repair).not.toBeNull();
    useConsentStore.getState().clearRepair('consent-1');
    expect(useConsentStore.getState().repair).toBeNull();

    useConsentStore.getState().setApply({
      deviceId: 'disk-1',
      requestId: 'apply-1',
      plan: {
        refused: 0,
        scope: 0,
        startLba: 4096,
        sectorCount: 100,
        mbrType: 7,
        slot: 0,
        regenerateSignature: 0,
        signature: 1,
        reason: '',
      },
    });
    expect(useConsentStore.getState().apply?.requestId).toBe('apply-1');
    useConsentStore.getState().clearApply('apply-1');
    expect(useConsentStore.getState().apply).toBeNull();
  });
});

describe('the mock consent flow (the echo-token machinery)', () => {
  it('inject -> respond resolves the gate; late responds are consent-not-found', async () => {
    const mock = new MockConnector();
    const devices = await mock.devices.listDevices();
    const target = devices[0].volumes[0].id;
    const { scanId } = await mock.scan.startScan({ targetId: target });
    expect(scanId).toBeTruthy();

    const gate = mock.injectConsentRequest(scanId, {
      detected: 1,
      fsFamily: 'ntfs',
      actionCount: 2,
      refused: 0,
      reason: 'test',
      advisoryCount: 0,
    });
    // The pending prompt is re-hydratable from state.
    let requestId = '';
    await waitFor(async () => {
      const state = await mock.scan.getScanState(scanId);
      requestId = state.pendingConsent?.requestId ?? '';
      expect(requestId).toBeTruthy();
    });
    await mock.scan.respondToConsent(requestId, true);
    await expect(gate).resolves.toBe(true);

    await expect(mock.scan.respondToConsent('consent-999', false)).rejects.toMatchObject({
      code: 'consent-not-found',
    });
  });

  it('the state carries pendingConsent while the gate waits', async () => {
    const mock = new MockConnector();
    const devices = await mock.devices.listDevices();
    const { scanId } = await mock.scan.startScan({ targetId: devices[0].volumes[0].id });
    mock.injectConsentRequest(scanId, {
      detected: 1,
      fsFamily: 'fat',
      actionCount: 1,
      refused: 0,
      reason: '',
      advisoryCount: 2,
    });
    await waitFor(async () => {
      const state = await mock.scan.getScanState(scanId);
      expect(state.lostCandidates).toEqual({ count: 0, rows: [] });
      expect(state.pendingConsent?.plan.actionCount).toBe(1);
    });
  });
});

describe('the consent modal (both families render + answer)', () => {
  it('renders the repair plan and denies through the connector', async () => {
    const respondSpy = vi
      .spyOn(connector.scan, 'respondToConsent')
      .mockResolvedValue(undefined);
    useConsentStore.getState().setRepair({
      scanId: 'scan-1',
      requestId: 'consent-1',
      plan: {
        detected: 1,
        fsFamily: 'ntfs',
        actionCount: 2,
        refused: 0,
        reason: '',
        advisoryCount: 1,
      },
    });
    wrap(<ConsentModal />);
    expect(await screen.findByRole('dialog', { name: /confirm the repair/i })).toBeVisible();
    expect(screen.getByText(/2 write actions/i)).toBeVisible();
    fireEvent.click(screen.getByRole('button', { name: /decline/i }));
    await waitFor(() => {
      expect(respondSpy).toHaveBeenCalledWith('consent-1', false);
      expect(useConsentStore.getState().repair).toBeNull();
    });
    respondSpy.mockRestore();
  });

  it('renders the apply plan and approves through the connector', async () => {
    const respondSpy = vi
      .spyOn(connector.scan, 'respondToConsent')
      .mockResolvedValue(undefined);
    useConsentStore.getState().setApply({
      deviceId: 'disk-1',
      requestId: 'apply-2',
      plan: {
        refused: 0,
        scope: 1,
        startLba: 4096,
        sectorCount: 204800,
        mbrType: 7,
        slot: 1,
        regenerateSignature: 1,
        signature: 0x5a5a5a5a,
        reason: '',
      },
    });
    wrap(<ConsentModal />);
    expect(
      await screen.findByRole('dialog', { name: /confirm the partition entry write/i }),
    ).toBeVisible();
    expect(screen.getByText(/rebuild the table/i)).toBeVisible();
    fireEvent.click(screen.getByRole('button', { name: /approve/i }));
    await waitFor(() => {
      expect(respondSpy).toHaveBeenCalledWith('apply-2', true);
      expect(useConsentStore.getState().apply).toBeNull();
    });
    respondSpy.mockRestore();
  });

  it('approves stay disabled for a refused plan (default-deny truth)', async () => {
    useConsentStore.getState().setRepair({
      scanId: 'scan-1',
      requestId: 'consent-3',
      plan: {
        detected: 1,
        fsFamily: 'ntfs',
        actionCount: 0,
        refused: 1,
        reason: 'no safe repair',
        advisoryCount: 0,
      },
    });
    wrap(<ConsentModal />);
    expect(await screen.findByRole('dialog', { name: /confirm the repair/i })).toBeVisible();
    expect(screen.getByRole('button', { name: /approve/i })).toBeDisabled();
    useConsentStore.getState().clearRepair('consent-3');
  });
});

describe('the elevation banner (the user-mode posture)', () => {
  it('is hidden for an administrator and shown with the relaunch action otherwise', async () => {
    const postureSpy = vi
      .spyOn(connector.system, 'getSecurityPosture')
      .mockResolvedValue({ administrator: true });
    const { unmount } = wrap(<ElevationBanner />);
    await waitFor(() => expect(postureSpy).toHaveBeenCalled());
    expect(screen.queryByRole('alert')).toBeNull();
    postureSpy.mockRestore();
    unmount();

    const postureSpy2 = vi
      .spyOn(connector.system, 'getSecurityPosture')
      .mockResolvedValue({ administrator: false });
    wrap(<ElevationBanner />);
    expect(await screen.findByRole('alert')).toBeVisible();
    expect(screen.getByRole('button', { name: /restart as administrator/i })).toBeVisible();
    postureSpy2.mockRestore();
  });
});

describe('the N-064 unlock dialog', () => {
  it('unlocks through the connector on a credential', async () => {
    const unlockSpy = vi.spyOn(connector.devices, 'unlockVolume').mockResolvedValue(undefined);
    wrap(<UnlockVolumeDialog volumeId="vol-E" open onOpenChange={() => undefined} />);
    expect(await screen.findByRole('dialog', { name: /unlock the volume/i })).toBeVisible();
    fireEvent.change(screen.getByPlaceholderText(/volume password/i), {
      target: { value: 'secret' },
    });
    const action = screen.getByRole('button', { name: /^unlock$/i });
    expect(action).toBeEnabled();
    fireEvent.click(action);
    await waitFor(() => expect(unlockSpy).toHaveBeenCalledWith('vol-E', ['secret']));
    unlockSpy.mockRestore();
  });

  it('stays disabled without any credential and stays open on refusal', async () => {
    const unlockSpy = vi.spyOn(connector.devices, 'unlockVolume').mockRejectedValue(
      Object.assign(new Error('the dictionary did not unlock the volume'), {
        code: 'device-permission',
      }),
    );
    const onOpenChange = vi.fn();
    wrap(<UnlockVolumeDialog volumeId="vol-E" open onOpenChange={onOpenChange} />);
    expect(await screen.findByRole('dialog', { name: /unlock the volume/i })).toBeVisible();
    // no credential: the action is disabled (never a credentialless attempt)
    expect(screen.getByRole('button', { name: /^unlock$/i })).toBeDisabled();
    fireEvent.change(screen.getByPlaceholderText(/64 or 128 hex characters/i), {
      target: { value: 'ab' },
    });
    expect(screen.getByRole('button', { name: /^unlock$/i })).toBeEnabled();
    fireEvent.click(screen.getByRole('button', { name: /^unlock$/i }));
    await waitFor(() => expect(unlockSpy).toHaveBeenCalled());
    // the dialog stays open on failure (the user can retry)
    expect(onOpenChange).not.toHaveBeenCalledWith(false);
    unlockSpy.mockRestore();
  });
});

describe("the mock's new domains (the P4 contract members)", () => {
  it('lostPartition: consent-gated apply + undo + pending re-hydration', async () => {
    const mock = new MockConnector();
    const devices = await mock.devices.listDevices();
    const deviceId = devices[0].id;
    const seen: string[] = [];
    const off = mock.lostPartition.onApplyConsent(deviceId, (e) => seen.push(e.requestId));
    const apply = mock.lostPartition.applyLostPartition(
      deviceId,
      { lba: 4096, sectorCount: 204800, fs: 0, score: 80 },
      true,
    );
    await waitFor(() => expect(seen).toHaveLength(1));
    const pending = await mock.lostPartition.pendingApplyConsent(deviceId);
    expect(pending?.requestId).toBe(seen[0]);
    await mock.lostPartition.respondToApplyConsent(seen[0], true);
    const result = await apply;
    expect(result.applied).toBe(true);
    expect(result.outcome).toBe(0);
    expect(result.undoEntries).toBeGreaterThan(0);

    const undone = await mock.lostPartition.undoLastApply(deviceId);
    expect(undone.applied).toBe(true);

    await expect(
      mock.lostPartition.respondToApplyConsent(seen[0], false),
    ).rejects.toMatchObject({
      code: 'consent-not-found',
    });
    off();
  });

  it('lostPartition: a denied consent writes nothing (outcome 1)', async () => {
    const mock = new MockConnector();
    const devices = await mock.devices.listDevices();
    const deviceId = devices[0].id;
    const seen: string[] = [];
    const off = mock.lostPartition.onApplyConsent(deviceId, (e) => seen.push(e.requestId));
    const apply = mock.lostPartition.applyLostPartition(deviceId, {
      lba: 8192,
      sectorCount: 100,
      fs: 1,
      score: 40,
    });
    await waitFor(() => expect(seen).toHaveLength(1));
    await mock.lostPartition.respondToApplyConsent(seen[0], false);
    const result = await apply;
    expect(result.applied).toBe(false);
    expect(result.outcome).toBe(1);
    off();
  });

  it('sessions: cancelSave no-op + resumeLast fallback + resume rules', async () => {
    const mock = new MockConnector();
    await expect(mock.sessions.cancelSave('scan-1')).resolves.toBeUndefined();

    await expect(mock.sessions.resumeLastSession()).rejects.toMatchObject({
      code: 'session-not-found',
    });

    const devices = await mock.devices.listDevices();
    const { scanId } = await mock.scan.startScan({ targetId: devices[0].volumes[0].id });
    await mock.scan.cancelScan(scanId);
    const saved = await mock.sessions.saveSession(scanId, null);
    expect(saved.fileName.startsWith('auto_')).toBe(true);
    const last = await mock.sessions.resumeLastSession();
    expect(last.path).toBe(saved.fileName);
  });

  it('system: posture follows the scenario; user signs parse the document', async () => {
    const mock = new MockConnector();
    let posture = await mock.system.getSecurityPosture();
    expect(posture.administrator).toBe(true);
    mock.setScenario('permission-denied');
    posture = await mock.system.getSecurityPosture();
    expect(posture.administrator).toBe(false);
    mock.setScenario('standard');

    const doc =
      '{ ".xyz": { "signs": ["0,AA"], "mime": "x/test" }, ".zz": { "signs": ["4,BB"] } }';
    const { count } = await mock.system.loadUserSigns(doc);
    expect(count).toBe(2);
    const info = await mock.system.getAppInfo();
    expect(info.userSigns).toBe(2);
    await expect(mock.system.loadUserSigns('not json')).rejects.toMatchObject({
      code: 'scan-scope-invalid',
    });

    await expect(mock.system.relaunchElevated()).rejects.toMatchObject({
      code: 'not-implemented',
    });
  });

  it('devices: unlockVolume flips the locked volume (and the refusal path)', async () => {
    const mock = new MockConnector();
    mock.setScenario('standard');
    const devices = await mock.devices.listDevices();
    const locked = devices.flatMap((d) => d.volumes).find((v) => v.status === 'locked');
    if (locked) {
      await mock.devices.unlockVolume(locked.id, ['pw']);
      const after = await mock.devices.listDevices();
      const vol = after.flatMap((d) => d.volumes).find((v) => v.id === locked.id);
      expect(vol?.status).toBe('ok');
    }
    const anyVolume = (await mock.devices.listDevices()).flatMap((d) => d.volumes)[0];
    mock.setScenario('permission-denied');
    await expect(mock.devices.unlockVolume(anyVolume.id, ['pw'])).rejects.toMatchObject({
      code: 'device-permission',
    });
    mock.setScenario('standard');
  });
});
