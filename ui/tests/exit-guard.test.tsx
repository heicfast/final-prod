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
type CloseHandler = (event: { preventDefault(): void }) => void;
const closeHandlers: CloseHandler[] = [];

vi.mock('@tauri-apps/api/window', () => ({
  getCurrentWindow: () => ({
    destroy,
    onCloseRequested: (cb: CloseHandler) => {
      closeHandlers.push(cb);
      return Promise.resolve(() => undefined);
    },
  }),
}));

function fireClose(): { preventDefault: ReturnType<typeof vi.fn> } {
  const preventDefault = vi.fn();
  expect(closeHandlers.length).toBeGreaterThan(0);
  for (const h of closeHandlers) h({ preventDefault });
  return { preventDefault };
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
    const { preventDefault } = fireClose();
    await waitFor(() => expect(preventDefault).toHaveBeenCalled());
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
    const { preventDefault } = fireClose();
    await waitFor(() => expect(destroy).toHaveBeenCalled());
    expect(preventDefault).not.toHaveBeenCalled();
    expect(screen.queryByRole('dialog')).toBeNull();
    stateSpy.mockRestore();
  });

  it('no known active scan → the default close proceeds (no preventDefault)', async () => {
    useScanSessionStore.getState().markScanStopped();
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    const { preventDefault } = fireClose();
    expect(preventDefault).not.toHaveBeenCalled();
    expect(destroy).not.toHaveBeenCalled();
    expect(screen.queryByRole('dialog')).toBeNull();
  });
});
