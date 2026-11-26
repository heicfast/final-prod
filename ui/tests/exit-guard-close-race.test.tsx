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
 * SB-B-02 (the P0 half the X-button defect): the close-requested handler
 * MUST return its promise to the onCloseRequested wrapper.
 *
 * The REAL @tauri-apps/api 2.12.4 wrapper (window.js onCloseRequested) does:
 *
 *     return this.listen(TauriEvent.WINDOW_CLOSE_REQUESTED, async (event) => {
 *         const evt = new CloseRequestedEvent(event);
 *         await handler(evt);
 *         if (!evt.isPreventDefault()) {
 *             await this.destroy();
 *         }
 *     });
 *
 * A handler that fires its async logic with `void` (fire-and-forget) returns
 * undefined to that `await` — the wrapper checks isPreventDefault() BEFORE
 * the guard's async state read resolves, and destroys the window mid-scan
 * while the dialog is still being decided. This suite pins the wrapper's
 * exact semantics (mock below is a faithful transcription of the installed
 * node_modules/@tauri-apps/api/window.js) so the race cannot come back.
 *
 * Pre-fix behavior (red): fireClose() with a live scan → destroy() called,
 * dialog never wins. Post-fix: destroy() NOT called by the wrapper,
 * preventDefault lands, dialog offers the three-way choice.
 */

const destroy = vi.fn();
type Handler = (event: {
  preventDefault(): void;
  isPreventDefault(): boolean;
}) => void | Promise<void>;
const closeHandlers: ((event: { preventDefault(): void }) => void)[] = [];

vi.mock('@tauri-apps/api/window', () => ({
  getCurrentWindow: () => ({
    destroy,
    // FAITHFUL to @tauri-apps/api 2.12.4 window.js:2030-2036 — the wrapper
    // awaits the handler, then destroys when it was not prevented.
    onCloseRequested: (cb: Handler) => {
      closeHandlers.push(async (event) => {
        const evt = {
          prevented: false,
          preventDefault() {
            evt.prevented = true;
          },
          isPreventDefault() {
            return evt.prevented;
          },
        };
        await cb(evt);
        if (!evt.isPreventDefault()) destroy();
      });
      return Promise.resolve(() => undefined);
    },
  }),
}));

function fireClose(): void {
  expect(closeHandlers.length).toBeGreaterThan(0);
  for (const h of closeHandlers) {
    void h({ preventDefault: () => undefined });
  }
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

describe('ExitGuard close race (SB-B-02) — faithful onCloseRequested wrapper', () => {
  it('a live scan: the wrapper must NOT destroy while the guard decides — preventDefault lands first', async () => {
    const stateSpy = vi
      .spyOn(connector.scan, 'getScanState')
      .mockResolvedValue({ status: 'running' } as Awaited<
        ReturnType<typeof connector.scan.getScanState>
      >);
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    fireClose();
    // The wrapper's isPreventDefault() check runs only after the handler's
    // async state read resolves — the dialog must win, the window must live.
    await waitFor(() => expect(screen.findByRole('dialog')).toBeTruthy());
    await waitFor(() => {
      expect(screen.getByRole('button', { name: /save and exit/i })).toBeVisible();
    });
    expect(destroy).not.toHaveBeenCalled();
    stateSpy.mockRestore();
  });

  it('a terminal scan state: the wrapper itself closes (destroy fires, no dialog)', async () => {
    const stateSpy = vi
      .spyOn(connector.scan, 'getScanState')
      .mockResolvedValue({ status: 'completed' } as Awaited<
        ReturnType<typeof connector.scan.getScanState>
      >);
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    fireClose();
    await waitFor(() => expect(destroy).toHaveBeenCalledTimes(1));
    expect(screen.queryByRole('dialog')).toBeNull();
    stateSpy.mockRestore();
  });

  it('no known active scan → the default close proceeds via the wrapper destroy', async () => {
    useScanSessionStore.getState().markScanStopped();
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    fireClose();
    await waitFor(() => expect(destroy).toHaveBeenCalledTimes(1));
    expect(screen.queryByRole('dialog')).toBeNull();
  });

  it('the dialog paths still destroy the window themselves (wrapper not involved)', async () => {
    const stateSpy = vi
      .spyOn(connector.scan, 'getScanState')
      .mockResolvedValue({ status: 'running' } as Awaited<
        ReturnType<typeof connector.scan.getScanState>
      >);
    setTauriRuntime(true);
    render(wrap(<ExitGuard />));
    await waitFor(() => expect(closeHandlers.length).toBeGreaterThan(0));
    fireClose();
    fireEvent.click(await screen.findByRole('button', { name: /exit without saving/i }));
    await waitFor(() => expect(destroy).toHaveBeenCalledTimes(1));
    stateSpy.mockRestore();
  });
});
