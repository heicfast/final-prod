import { describe, expect, it, afterEach } from 'vitest';
import { render, screen, cleanup, fireEvent, waitFor } from '@testing-library/react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { TooltipProvider } from '@/components/ui/tooltip';
import { ThemeProvider } from '@/app/theme';
import { SectorMapPanel } from '@/features/scan-progress/sector-map-panel';
import { RecoverabilityCard } from '@/features/scan-progress/recoverability-card';
import { CreateImageDialog } from '@/features/imaging/create-image-dialog';
import { connector } from '@/connector';
import type { Device, DiskMapPayload } from '@/connector/contract';
import '@/i18n';

/**
 * Lane C surfaces: the sector map (the AnalysisGridWidget producer), the
 * recoverability card (the S-023 assistant surface), and the Create Image
 * dialog (the reference's DiskBackup chain) — driven against the mock
 * connector's own data, never synthesized fixtures for the sake of it.
 */

function renderTree(children: React.ReactNode): ReturnType<typeof render> {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false, staleTime: Infinity } },
  });
  return render(
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>{children}</TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>,
  );
}

afterEach(() => {
  cleanup();
});

describe('SectorMapPanel', () => {
  it('renders collapsed, opens, and paints the engine overlays', async () => {
    await connector.devices.refreshDevices();
    const target = (await connector.devices.listDevices())[0]?.volumes[0];
    if (!target) return;
    const { scanId } = await connector.scan.startScan({ targetId: target.id });
    await new Promise<void>((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('scan timeout')), 60_000);
      const off = connector.scan.onScanProgress(scanId, (e) => {
        if (e.status === 'completed' || e.status === 'failed' || e.status === 'cancelled') {
          clearTimeout(timer);
          off();
          resolve();
        }
      });
    });

    renderTree(<SectorMapPanel scanId={scanId} live={false} />);

    // Collapsed by default: the toggle strip is there, the tabs are not.
    const toggle = screen.getByRole('button', { name: /sector map/i });
    expect(screen.queryByRole('tab', { name: /coverage/i })).toBeNull();

    fireEvent.click(toggle);
    const coverageTab = await screen.findByRole('tab', { name: /coverage/i });
    expect(coverageTab.getAttribute('aria-selected')).toBe('true');
    // The completed scan's map is fully covered — the legend shows both states.
    expect(await screen.findByText('Scanned', { exact: true })).toBeInTheDocument();
    expect(screen.getByText('Not scanned', { exact: true })).toBeInTheDocument();

    // Overlay switch re-renders the legend (allocation's vocabulary).
    fireEvent.click(screen.getByRole('tab', { name: /allocation/i }));
    await waitFor(() => {
      expect(
        screen.getByRole('tab', { name: /allocation/i }).getAttribute('aria-selected'),
      ).toBe('true');
    });
  });

  it('merges oversized cell arrays down to the display bound', async () => {
    // A payload bigger than the display bound folds with the ranked merge.
    const big = 4096;
    const payload: DiskMapPayload = {
      scanId: 'scan-x',
      geometry: {
        startByte: 0,
        totalBytes: big * 65536,
        cellBytes: 65536,
        cellCount: big,
        clusterBytes: 4096,
        fsFamily: 1,
      },
      allocation: Array.from({ length: big }, (_, i) => (i % 7 === 0 ? 3 : i % 2)),
      coverage: Array.from({ length: big }, (_, i) => (i < big / 2 ? 1 : 0)),
      found: Array.from({ length: big }, () => 0),
      confidence: Array.from({ length: big }, () => 0),
      badRanges: [],
    };
    const qc = new QueryClient({ defaultOptions: { queries: { retry: false } } });
    qc.setQueryData(['scan', 'scan-x', 'disk-map', 1024], payload);
    render(
      <ThemeProvider>
        <QueryClientProvider client={qc}>
          <TooltipProvider>
            <SectorMapPanel scanId="scan-x" live={false} />
          </TooltipProvider>
        </QueryClientProvider>
      </ThemeProvider>,
    );
    fireEvent.click(screen.getByRole('button', { name: /sector map/i }));
    fireEvent.click(await screen.findByRole('tab', { name: /allocation/i }));
    // The merge kept the bad cells (rank 3 wins): the legend carries them.
    expect(await screen.findByText('Unreadable', { exact: true })).toBeInTheDocument();
  });
});

describe('RecoverabilityCard', () => {
  it('renders the engine verdict from a completed scan', async () => {
    await connector.devices.refreshDevices();
    const target = (await connector.devices.listDevices())[0]?.volumes[0];
    if (!target) return;
    const { scanId } = await connector.scan.startScan({ targetId: target.id });
    await new Promise<void>((resolve, reject) => {
      const timer = setTimeout(() => reject(new Error('scan timeout')), 60_000);
      const off = connector.scan.onScanProgress(scanId, (e) => {
        if (e.status === 'completed' || e.status === 'failed' || e.status === 'cancelled') {
          clearTimeout(timer);
          off();
          resolve();
        }
      });
    });

    renderTree(<RecoverabilityCard scanId={scanId} completed />);
    // The standard scenario grades > 100 files: the gauge verdict shows.
    expect(
      await screen.findByText(/Your files are highly recoverable \(\d+%\)/),
    ).toBeInTheDocument();
    expect(screen.getByText(/Images:/)).toBeInTheDocument();
  });

  it('renders nothing without a graded answer', () => {
    const { container } = renderTree(<RecoverabilityCard scanId="scan-unknown" completed />);
    // The query is disabled-failed (no such scan): the card hides itself.
    expect(container.firstChild).toBeNull();
  });
});

describe('CreateImageDialog', () => {
  const device: Device = {
    id: 'dev-sys',
    kind: 'internal',
    model: 'Test SSD 500GB',
    label: 'Windows (C:)',
    serial: 'SN-1',
    status: 'ok',
    isSourceOfActiveScan: false,
    canScanWholeDisk: true,
    volumes: [],
    capacity: {
      totalBytes: 500_000_000_000,
      usedBytes: 300_000_000_000,
      freeBytes: 200_000_000_000,
    },
  };

  it('opens with the destination form, starts, and reaches the no-cancel running posture', async () => {
    await connector.devices.refreshDevices();
    const devices = await connector.devices.listDevices();
    const src = devices.find((d) => d.volumes.some((v) => v.letter === 'C')) ?? devices[0];
    // Route the picker to a folder on ANOTHER device (the same-disk gate
    // would refuse) — the mock's destination picker respects the hint.
    renderTree(
      <CreateImageDialog device={src ?? device} open onOpenChange={() => undefined} />,
    );

    expect(screen.getByText(/^A byte-for-byte backup image/i)).toBeInTheDocument();
    // Start requires a destination: the primary action is disabled.
    const startBtn = await screen.findByRole('button', { name: /create image/i });
    expect(startBtn).toBeDisabled();

    // Pick a destination through the connector-owned picker (excludes src).
    fireEvent.click(screen.getByRole('button', { name: /browse/i }));
    await waitFor(() => expect(startBtn).not.toBeDisabled(), { timeout: 5_000 });

    fireEvent.click(startBtn);
    // The running posture: the truthful no-cancel note + the hide action
    // (never a stop button).
    expect(
      await screen.findByText(/runs to completion/i, { exact: false }),
    ).toBeInTheDocument();
    await waitFor(
      () => {
        expect(
          screen.getByRole('button', { name: /continue in background/i }),
        ).toBeInTheDocument();
      },
      { timeout: 10_000 },
    );
    expect(screen.queryByRole('button', { name: /^(stop|cancel)/i })).toBeNull();
  });
});
