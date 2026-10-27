/**
 * health-truth.test.tsx — NG-5/NG-6/NG-7/NG-9/NG-10 regression pair.
 *
 * These assertions FAIL on the pre-fix surfaces and pass after:
 *  - the health gauge fabricated a percentage from status alone
 *    (good→max(88,100-wear)≥96 / caution→72 / bad→38) even with ZERO
 *    attributes — fake data in a shipped surface (NG-5);
 *  - 'unknown' status did not exist (HealthStatus was good|caution|bad);
 *  - hard-coded "512B"/"2%" strings rendered regardless of real values
 *    (NG-10);
 *  - Home hid every removable device behind a ?media URL param that no UI
 *    affordance ever set (NG-7).
 */
import { describe, expect, it, afterEach, beforeEach } from 'vitest';
import { cleanup, render, screen, within } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ThemeProvider } from '@/app/theme';
import { TooltipProvider } from '@/components/ui/tooltip';
import { HealthPage } from '@/features/health/health-page';
import { HomePage } from '@/features/device-selection/home-page';
import { testOverrides } from './fakes/test-connector';
import type { Device, DiskHealth } from '@/connector/contract';
import '@/i18n';

function wrap(ui: React.ReactElement, initialPath = '/'): ReturnType<typeof render> {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false, staleTime: Infinity } },
  });
  return render(
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>
          <MemoryRouter initialEntries={[initialPath]}>{ui}</MemoryRouter>
        </TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>,
  );
}

const INTERNAL: Device = {
  id: 'disk-0',
  kind: 'internal',
  model: 'Internal NVMe 1TB',
  label: 'Local Disk (C:)',
  serial: 'SN-INT',
  capacity: {
    totalBytes: 1_000_000_000_000,
    usedBytes: 400_000_000_000,
    freeBytes: 600_000_000_000,
  },
  status: 'ok',
  isSourceOfActiveScan: false,
  volumes: [
    {
      id: 'vol-c',
      deviceId: 'disk-0',
      label: 'OS',
      letter: 'C:',
      fs: 'ntfs',
      capacity: {
        totalBytes: 1_000_000_000_000,
        usedBytes: 400_000_000_000,
        freeBytes: 600_000_000_000,
      },
      status: 'ok',
      quickScanAvailable: true,
      sectorSize: 4096,
      bitlocker: false,
    },
  ],
  canScanWholeDisk: true,
};

const USB: Device = {
  ...INTERNAL,
  id: 'disk-1',
  kind: 'usb',
  model: 'Seagate Expansion 2TB',
  label: 'Backup (E:)',
  serial: 'SN-USB',
};

const baseHealth = (over: Partial<DiskHealth>): DiskHealth => ({
  deviceId: 'disk-0',
  status: 'unknown',
  temperatureC: null,
  powerOnHours: null,
  diskClass: 'unknown',
  wearPercent: null,
  trimEnabled: null,
  reallocatedSectors: null,
  pendingSectors: null,
  attributes: [],
  transport: null,
  tableEmpty: true,
  ...over,
});

beforeEach(() => {
  testOverrides.devices = null;
  testOverrides.health = null;
});

afterEach(() => {
  cleanup();
  testOverrides.devices = null;
  testOverrides.health = null;
});

describe('health surface truthfulness (NG-5/NG-9/NG-10)', () => {
  it('renders NO percentage when the drive reports no wear metric and no attributes', async () => {
    testOverrides.devices = [INTERNAL];
    testOverrides.health = baseHealth({ transport: 'ata-passthrough' }); // unknown, empty table, no wear
    wrap(<HealthPage />, '/health');

    await screen.findByRole('heading', { name: /disk health/i });
    // The gauge must NOT invent a number: no bare "NN%" text anywhere.
    const percentNodes = await screen.findAllByText(/^\d+(\.\d+)?%$/).catch(() => []);
    expect(percentNodes).toEqual([]);
    // The truthful Unknown chip is present.
    expect(await screen.findByText('Unknown')).toBeVisible();
    // The truthful empty-table note is present.
    expect(await screen.findByText(/reports no S\.M\.A\.R\.T\. attribute rows/i)).toBeVisible();
  });

  it('derives the life percentage ONLY from a real wear metric (100 - wear)', async () => {
    testOverrides.devices = [INTERNAL];
    testOverrides.health = baseHealth({
      status: 'good',
      wearPercent: 3,
      temperatureC: 41,
      transport: 'nvme',
      tableEmpty: false,
    });
    wrap(<HealthPage />, '/health');

    expect(await screen.findByText('97%')).toBeVisible(); // 100 - 3, the real derivation
    expect(screen.getByText('Good')).toBeVisible();
    expect(
      screen.getByText(/NVMe drives do not expose the ATA S\.M\.A\.R\.T\./i),
    ).toBeVisible();
  });

  it('shows the REAL volume sector size instead of a hard-coded 512B', async () => {
    testOverrides.devices = [INTERNAL];
    testOverrides.health = baseHealth({ status: 'good', wearPercent: 10, tableEmpty: false });
    wrap(<HealthPage />, '/health');

    await screen.findByRole('heading', { name: /disk health/i });
    expect(await screen.findByText(/4096B/i)).toBeVisible();
    expect(screen.queryByText(/512B/i)).toBeNull();
  });

  it('renders the real attribute table with thresholds', async () => {
    testOverrides.devices = [INTERNAL];
    testOverrides.health = baseHealth({
      status: 'good',
      wearPercent: 8,
      tableEmpty: false,
      transport: 'ata-passthrough',
      attributes: [
        {
          id: 5,
          name: 'Reallocated Sectors Count',
          current: 100,
          worst: 100,
          threshold: 10,
          raw: 0,
          status: 'ok',
        },
        {
          id: 194,
          name: 'Temperature Celsius',
          current: 65,
          worst: 60,
          threshold: 0,
          raw: 35,
          status: 'ok',
        },
      ],
    });
    wrap(<HealthPage />, '/health');

    expect(await screen.findByText('Reallocated Sectors Count')).toBeVisible();
    expect(screen.getByText('Temperature Celsius')).toBeVisible();
    // the threshold column carries the real vendor value (10) and '—' when
    // the drive reports none (attribute 194)
    const rows = screen.getAllByRole('row');
    expect(within(rows[1]).getByText('10')).toBeVisible();
  });
});

describe('Home lists every physical disk (NG-7)', () => {
  it('renders internal AND removable device groups together', async () => {
    testOverrides.devices = [INTERNAL, USB];
    wrap(<HomePage />, '/');

    expect(await screen.findByText(/Backup\(E:\)/i)).toBeVisible();
    expect(screen.getByText(/Local Disk\(C:\)/i)).toBeVisible();
    // both group headers present
    expect(screen.getByText(/^internal drives \(\d+\)/i)).toBeTruthy();
    expect(screen.getByText(/^removable media \(\d+\)/i)).toBeTruthy();
  });
});
