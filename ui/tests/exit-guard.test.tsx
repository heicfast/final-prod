import * as React from 'react';
import { describe, expect, it, vi, beforeEach, afterEach } from 'vitest';
import { render, screen, fireEvent, cleanup, waitFor } from '@testing-library/react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { TooltipProvider } from '@/components/ui/tooltip';
import { ThemeProvider } from '@/app/theme';
import { ExitGuard } from '@/app/exit-guard';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { connector } from '@/connector';
import '@/i18n';

/**
 * ExitGuard (W-02): closing the window while a scan runs must offer the
 * save path instead of silently killing the session. Regression pin —
 * the pre-fix shell had NO close-requested handler at all: the X button
 * ended the process with the session unsaved.
 *
 * - close with an ACTIVE engine scan → preventDefault + the dialog
 *   (Save and exit / Keep scanning / Exit without saving)
 * - "Save and exit" runs the REAL saveSession then destroys the window
 * - a terminal (or unknown) scan state → no dialog, destroy directly
 * - no known scan → the default close proceeds (no preventDefault)
 */

const destroy = vi.fn();
type Evt = {
  prevented: boolean;
  preventDefault: ReturnType<typeof vi.fn>;
  isPreventDefault(): boolean;
};
const closeHandlers: ((evt: Evt) => Promise<void>)[] = [];

vi.mock('@tauri-apps/api/window', () => ({
  getCurrentWindow: () => ({
    destroy,
    // Faithful to @tauri-apps/api 2.12.4 (window.js onCloseRequested):
    // the wrapper awaits the handler and destroys the window itself when
    // the handler did not preventDefault (SB-B-02 — the close is the
    // wrapper's job; the guard only decides whether to prevent).
    onCloseRequested: (cb: (evt: Evt) => void | Promise<void>) => {
      closeHandlers.push(async (evt) => {
        await cb(evt);
        if (!evt.isPreventDefault()) destroy();
      });
      return Promise.resolve(() => undefined);
    },
  }),
}));

function fireClose(): Evt {
  const evt: Evt = {
    prevented: false,
    preventDefault: vi.fn(() => {
      evt.prevented = true;
    }),
    isPreventDefault() {
      return evt.prevented;
    },
  };
  expect(closeHandlers.length).toBeGreaterThan(0);
  for (const h of closeHandlers) void h(evt);
  return evt;
}

function wrap(ui: React.ReactElement): React.ReactElement {
  const qc = new QueryClient({ defaultOptions: { queries: { retry: false } } });
  return (
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>{ui}</TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>
  );
}

function setTauriRuntime(on: boolean): void {
  if (on) {
    Object.defineProperty(window, '__TAURI_INTERNALS__', { value: {}, configurable: true });
  } else {
    delete (window as { __TAURI_INTERNALS__?: unknown }).__TAURI_INTERNALS__;
  }
}

beforeEach(() => {
  vi.clearAllMocks();
  closeHandlers.length = 0;
  useScanSessionStore.getState().setActiveScan({ scanId: 'scan-1', targetId: 'disk-0' });
});

afterEach(() => {
  cleanup();
  setTauriRuntime(false);
  useScanSessionStore.getState().markScanStopped();
  useScanSessionStore.getState().setActiveStatus('scan-1', 'cancelled');
});

describe('ExitGuard — the close-requested save guard (W-02)', () => {
  it('prevents the close and offers the dialog while the engine scan is live', async () => {
    const stateSpy = vi
      .spyOn(connector.scan, 'getScanState')
      .mockResolvedValue({ status: 'running' } as Awaited<
        ReturnType<typeof connector.scan.getScanState>
      >);
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    const evt = fireClose();
    await waitFor(() => expect(evt.preventDefault).toHaveBeenCalled());
    expect(await screen.findByRole('dialog')).toBeVisible();
    expect(screen.getByRole('button', { name: /save and exit/i })).toBeVisible();
    expect(screen.getByRole('button', { name: /keep scanning/i })).toBeVisible();
    expect(screen.getByRole('button', { name: /exit without saving/i })).toBeVisible();
    expect(destroy).not.toHaveBeenCalled();
    stateSpy.mockRestore();
  });

  it('Save and exit runs the REAL session save, then destroys the window', async () => {
    const stateSpy = vi
      .spyOn(connector.scan, 'getScanState')
      .mockResolvedValue({ status: 'running' } as Awaited<
        ReturnType<typeof connector.scan.getScanState>
      >);
    const saveSpy = vi
      .spyOn(connector.sessions, 'saveSession')
      .mockResolvedValue({} as Awaited<ReturnType<typeof connector.sessions.saveSession>>);
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    fireClose();
    fireEvent.click(await screen.findByRole('button', { name: /save and exit/i }));
    await waitFor(() => expect(saveSpy).toHaveBeenCalledWith('scan-1', null));
    await waitFor(() => expect(destroy).toHaveBeenCalled());
    stateSpy.mockRestore();
    saveSpy.mockRestore();
  });

  it('a terminal scan state closes directly (no dialog)', async () => {
    const stateSpy = vi
      .spyOn(connector.scan, 'getScanState')
      .mockResolvedValue({ status: 'completed' } as Awaited<
        ReturnType<typeof connector.scan.getScanState>
      >);
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    const evt = fireClose();
    // The close is the wrapper's destroy (SB-B-02): the guard does not
    // prevent a terminal scan — the window closes, no dialog.
    await waitFor(() => expect(destroy).toHaveBeenCalledTimes(1));
    expect(evt.preventDefault).not.toHaveBeenCalled();
    expect(screen.queryByRole('dialog')).toBeNull();
    stateSpy.mockRestore();
  });

  it('no known active scan → the default close proceeds (no preventDefault)', async () => {
    useScanSessionStore.getState().markScanStopped();
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    const evt = fireClose();
    // The default close proceeding IS the wrapper's destroy (SB-B-02 —
    // without the allow-destroy capability grant this invoke is denied and
    // the window can never close; pinned by window-capability-gate.test.ts).
    await waitFor(() => expect(destroy).toHaveBeenCalledTimes(1));
    expect(evt.preventDefault).not.toHaveBeenCalled();
    expect(screen.queryByRole('dialog')).toBeNull();
  });
});
