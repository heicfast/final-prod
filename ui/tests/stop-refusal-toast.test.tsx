import { describe, expect, it, afterEach, vi } from 'vitest';
import { render, screen, cleanup, fireEvent, waitFor } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { TooltipProvider } from '@/components/ui/tooltip';
import { ThemeProvider } from '@/app/theme';
import { Toaster } from '@/components/ui/toaster';
import { ScanProgressPage } from '@/features/scan-progress/scan-progress-page';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { connector } from '@/connector';
import '@/i18n';

/**
 * SB-A-08 — the silent stop: the stop-confirm dialog closed, the cancel
 * mutation fired with onSuccess ONLY, the engine refused it — and nothing
 * surfaced. The scan keeps running; the user believes it stopped. The
 * same shape on pause / resume / recovery-cancel. This suite pins the
 * stop path (the most destructive one): a refused stop MUST render an
 * error toast. Driven against the mock connector's own live scan (the
 * lane-c pattern), with cancelScan spied to the refused engine path.
 *
 * Pre-fix (red): the stop-confirm click produced zero toasts.
 * Post-fix (green): toast.error(err.message) renders.
 */

function mount(scanId: string): ReturnType<typeof render> {
  const qc = new QueryClient({ defaultOptions: { queries: { retry: false } } });
  return render(
    <MemoryRouter initialEntries={[`/scan/disk-0/live?scanId=${scanId}`]}>
      <ThemeProvider>
        <QueryClientProvider client={qc}>
          <TooltipProvider>
            <ScanProgressPage />
            <Toaster />
          </TooltipProvider>
        </QueryClientProvider>
      </ThemeProvider>
    </MemoryRouter>,
  );
}

afterEach(() => {
  cleanup();
  vi.restoreAllMocks();
  useScanSessionStore.getState().markScanStopped();
});

describe('Scan stop surfaces a refused cancel (SB-A-08)', () => {
  it('a refused stop renders an error toast — no silent no-op', async () => {
    await connector.devices.refreshDevices();
    const target = (await connector.devices.listDevices())[0]?.volumes[0];
    if (!target) return; // no seeded device row — vacuous guard, never hit
    const { scanId } = await connector.scan.startScan({ targetId: target.id });
    useScanSessionStore.getState().setActiveScan({ scanId, targetId: target.id });

    // The engine refuses the stop (device gone mid-drain — a real path).
    const cancelSpy = vi
      .spyOn(connector.scan, 'cancelScan')
      .mockRejectedValue(new Error('scan device is gone'));

    mount(scanId);

    // The title-bar strip's Stop button opens the confirm dialog (the strip
    // and the dialog both carry the Stop label — the dialog opens last).
    const stopButtons = await waitFor(async () => {
      const all = await screen.findAllByRole('button', { name: 'Stop' });
      expect(all.length).toBeGreaterThanOrEqual(1);
      return all;
    });
    fireEvent.click(stopButtons[0]);

    // The confirm dialog's destructive button (the second Stop once open).
    const confirmButtons = await screen.findAllByRole('button', { name: 'Stop' });
    fireEvent.click(confirmButtons[confirmButtons.length - 1]);

    await waitFor(() => expect(cancelSpy).toHaveBeenCalledWith(scanId));
    expect(await screen.findByText('scan device is gone')).toBeTruthy();
  });
});
