import { describe, expect, it, afterEach } from 'vitest';
import { render, screen, cleanup, waitFor, fireEvent } from '@testing-library/react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { TooltipProvider } from '@/components/ui/tooltip';
import { ThemeProvider } from '@/app/theme';
import { DiskHealthWindowApp } from '@/features/health/disk-health-window';
import type { Device } from '@/connector/contract';
import { connector } from '@/connector';
import '@/i18n';

/**
 * The separate disk-health window (D-P4 reversal) — the reference model's
 * pins, from worklog/REFERENCE-SMART.md:
 *   §4/§5: a no-SMART disk (SD card) stays listed, grades Unknown with an
 *          empty table, never a fabricated green;
 *   §1:    opening jumps to the FIRST Caution/Bad disk (firstCautionIndex);
 *   §3.1:  the S.M.A.R.T. table renders the reference's columns with
 *          per-row status.
 * The mock connector models the SD-card truth (kind 'sd' → unknown +
 * empty attributes + no transport); the window consumes it through the
 * same query layer the real connector serves.
 */

function makeDevice(overrides: Partial<Device>): Device {
  return {
    id: 'dev-x',
    kind: 'internal',
    label: 'Local Disk',
    model: 'Test Disk',
    serial: 'SN-X',
    status: 'ok',
    canScanWholeDisk: true,
    volumes: [],
    capacity: { totalBytes: 500_000_000_000, usedBytes: 300_000_000_000 },
    ...overrides,
  } as Device;
}

function renderWindow(devices: Device[], initialDiskIndex = 0): ReturnType<typeof render> {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false, staleTime: Infinity } },
  });
  qc.setQueryData(['devices'], devices);
  return render(
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>
          <DiskHealthWindowApp initialDiskIndex={initialDiskIndex} />
        </TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>,
  );
}

afterEach(() => {
  cleanup();
});

describe('the mock models the reference SD-card truth', () => {
  it('an SD card grades Unknown with an empty table and no transport', async () => {
    const h = await connector.health.getHealth('dev-sdcard');
    expect(h.status).toBe('unknown');
    expect(h.attributes).toEqual([]);
    expect(h.tableEmpty).toBe(true);
    expect(h.transport).toBeNull();
  });
});

describe('the disk-health window', () => {
  it('jumps to the FIRST caution disk on open (firstCautionIndex parity)', async () => {
    renderWindow([
      makeDevice({ id: 'dev-sys', model: 'SSD EVO 970', serial: 'S1' }),
      makeDevice({ id: 'dev-data', model: 'Data HDD', serial: 'S2' }), // mock: caution
      makeDevice({ id: 'dev-sdcard', kind: 'sd', model: 'SD Card', serial: 'S3' }),
    ]);
    // the mock's dev-data is the caution disk — the window must select it
    await waitFor(
      () => {
        const sel = screen.getByTestId<HTMLSelectElement>('disk-health-selector');
        expect(sel.value).toBe('dev-data');
      },
      { timeout: 4000 },
    );
    // the caution banner (diskhealth.cautionCard) renders for it
    expect(await screen.findByText(/physical failure/i)).toBeTruthy();
    // the status chip says Caution
    expect(screen.getByTestId('disk-health-status').textContent).toMatch(/caution/i);
  });

  it('shows the truthful Unknown state for a no-SMART disk — never fake-green', async () => {
    renderWindow([
      makeDevice({ id: 'dev-sys', model: 'SSD EVO 970', serial: 'S1' }),
      makeDevice({ id: 'dev-sdcard', kind: 'sd', model: 'SD Card', serial: 'S2' }),
    ]);
    // wait for the initial scan to settle on the first device
    await waitFor(
      () => {
        const sel = screen.getByTestId<HTMLSelectElement>('disk-health-selector');
        expect(sel.value).not.toBe('');
      },
      { timeout: 4000 },
    );
    // user picks the SD card
    fireEvent.change(screen.getByTestId('disk-health-selector'), { target: { value: 'dev-sdcard' } });
    // the settled state: the empty-table note renders (the SD query resolved
    // with zero attributes) — truthful, no fabricated rows anywhere
    await waitFor(
      () => {
        expect(screen.getByTestId('disk-health-empty-table')).toBeTruthy();
      },
      { timeout: 4000 },
    );
    // the status chip says Unknown (the reference's gray state), not Good
    expect(screen.getByTestId('disk-health-status').textContent).toMatch(/unknown/i);
    expect(screen.queryByText('Reallocated Sectors Count')).toBeNull();
  });

  it('renders the reference column set including the ST column', async () => {
    renderWindow([makeDevice({ id: 'dev-sys', model: 'SSD EVO 970', serial: 'S1' })]);
    await waitFor(
      () => {
        expect(screen.getByText('Reallocated Sectors Count')).toBeTruthy();
      },
      { timeout: 4000 },
    );
    for (const col of ['ID', 'Attribute', 'Current', 'Threshold', 'RawValue', 'ST']) {
      expect(screen.getByRole('columnheader', { name: col })).toBeTruthy();
    }
  });
});
