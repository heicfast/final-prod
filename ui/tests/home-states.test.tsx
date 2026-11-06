import { describe, expect, it, afterEach } from 'vitest';
import { render, screen, cleanup } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { TooltipProvider } from '@/components/ui/tooltip';
import { ThemeProvider } from '@/app/theme';
import { HomePage } from '@/features/device-selection/home-page';
import type { Device, Volume } from '@/connector/contract';
import { connector } from '@/connector';
import '@/i18n';

/**
 * State surfaces (mandate §2.1 / EXPERIENCE-SPEC §7): permission-denied and
 * disconnected states must surface from EITHER level the connector reports
 * them at — device OR volume. Regression pin: the mock's permission-denied
 * scenario marks volume C:, and the home page used to read device-level
 * status only, so the state never rendered.
 */

function makeDevice(overrides: Partial<Device> = {}, volumes: Volume[] = []): Device {
  return {
    id: 'dev-1',
    kind: 'internal',
    label: 'Windows (C:)',
    model: 'Test SSD',
    serial: 'SN-1',
    status: 'ok',
    isSourceOfActiveScan: false,
    canScanWholeDisk: true,
    volumes,
    capacity: { totalBytes: 500_000_000_000, usedBytes: 300_000_000_000 },
    ...overrides,
  } as Device;
}

function makeVolume(overrides: Partial<Volume> = {}): Volume {
  return {
    id: 'vol-1',
    label: 'Windows (C:)',
    letter: 'C',
    fs: 'ntfs',
    status: 'ok',
    quickScanAvailable: true,
    capacity: { totalBytes: 500_000_000_000, usedBytes: 300_000_000_000 },
    ...overrides,
  } as Volume;
}

function renderHome(devices: Device[]): ReturnType<typeof render> {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false, staleTime: Infinity } },
  });
  qc.setQueryData(['devices'], devices);
  return render(
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>
          <MemoryRouter initialEntries={['/']}>
            <HomePage />
          </MemoryRouter>
        </TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>,
  );
}

afterEach(() => {
  cleanup();
});

describe('home state surfaces read volume-level status', () => {
  it('renders the permission banner when a VOLUME is permission-denied', async () => {
    const { connector: c } = { connector };
    void c;
    renderHome([
      makeDevice({}, [
        makeVolume({ letter: 'C', status: 'permission-denied', label: 'Windows (C:)' }),
      ]),
    ]);
    // The banner text (home.permissionDenied).
    expect(
      await screen.findByText(/administrator|permission|access/i, undefined, { timeout: 3000 }),
    ).toBeTruthy();
  });

  it('renders the disconnected banner when a VOLUME is disconnected', async () => {
    renderHome([makeDevice({}, [makeVolume({ status: 'disconnected' })])]);
    expect(
      await screen.findByText(/not responding/i, undefined, {
        timeout: 3000,
      }),
    ).toBeTruthy();
  });

  it('disables the scan chip of a permission-denied volume but keeps others live', () => {
    renderHome([
      makeDevice({}, [
        makeVolume({ id: 'vol-c', label: 'Windows (C:)', status: 'permission-denied' }),
        makeVolume({ id: 'vol-d', label: 'Data (D:)' }),
      ]),
    ]);
    const chipC = screen.getByRole('button', { name: 'Windows (C:)' });
    const chipD = screen.getByRole('button', { name: 'Data (D:)' });
    expect(chipC).toBeDisabled();
    expect(chipD).toBeEnabled();
  });
});

describe('D-ENUM-01: the landing page renders every physical device (reference parity)', () => {
  /**
   * Regression pin for the "only one disk appears" defect: the pre-fix page
   * rendered internal-only and hid every non-internal device (USB / SD /
   * removable-bus) behind an unreachable ?media URL param. The reference
   * landing page groups "Internal drives (N)" AND "External drives (%1)" on
   * the SAME page. Verified RED on the old filter, GREEN on the two-group
   * layout.
   */
  const internalDisk = makeDevice({ id: 'disk-0', label: 'Windows (C:)', model: 'NVMe SSD' }, [
    makeVolume({ id: 'vol-c', label: 'Windows (C:)', letter: 'C' }),
  ]);
  const sdCard = makeDevice(
    {
      id: 'disk-2',
      kind: 'sd',
      label: 'SD Card',
      model: 'USB SD Reader',
      volumes: [],
      capacity: {
        totalBytes: 64_000_000_000,
        usedBytes: 12_000_000_000,
        freeBytes: 52_000_000_000,
      },
    },
    [],
  );

  it('renders BOTH the Internal drives and External drives groups', () => {
    renderHome([internalDisk, sdCard]);
    expect(screen.getByRole('heading', { name: /internal drives \(1\)/i })).toBeTruthy();
    expect(screen.getByRole('heading', { name: /external drives \(1\)/i })).toBeTruthy();
  });

  it('renders the SD/removable device card on the landing page (no ?media param)', () => {
    renderHome([internalDisk, sdCard]);
    expect(screen.getByText('SD Card')).toBeTruthy();
    // DeviceCard renders the reference compact label ("Windows(C:)").
    expect(screen.getByText('Windows(C:)')).toBeTruthy();
  });

  it('hides the External drives group only when no external device exists', () => {
    renderHome([internalDisk]);
    expect(screen.getByRole('heading', { name: /internal drives \(1\)/i })).toBeTruthy();
    expect(screen.queryByRole('heading', { name: /external drives/i })).toBeNull();
  });
});
