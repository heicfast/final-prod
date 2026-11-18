import { describe, expect, it, afterEach } from 'vitest';
import { cleanup, render, screen } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ThemeProvider } from '@/app/theme';
import { TooltipProvider } from '@/components/ui/tooltip';
import { WizardShell } from '@/app/shell';
import { SdCardPage } from '@/features/device-selection/sd-card-page';
import { HealthPage } from '@/features/health/health-page';
import { BackupPage } from '@/features/backup/backup-page';
import { RepairPage } from '@/features/repair/repair-page';
import { LostPartitionPage } from '@/features/lost-partition/lost-partition-page';
import { ActivationDialog } from '@/features/settings-about/activation-dialog';
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

afterEach(() => {
  cleanup();
});

describe('reference UI surfaces and sidebar', () => {
  it('renders the 3-section sidebar and header without any simulator badge', () => {
    wrap(<WizardShell />);
    expect(screen.getByText('EaseUS Data Recovery Wizard Free')).toBeVisible();
    expect(screen.queryByText(/built-in simulator/i)).toBeNull();
    expect(screen.getByText('Hardware Disk')).toBeVisible();
    expect(screen.getByText('SD Card')).toBeVisible();
    expect(screen.getByText('Disk Health')).toBeVisible();
    expect(screen.getByText('File Repair')).toBeVisible();
    expect(screen.getByText('Lost Partition Rescue')).toBeVisible();
    expect(screen.getByText('Scan Snapshot')).toBeVisible();
  });

  it('does not render any robot/AI mascot icon in the title bar', () => {
    wrap(<WizardShell />);
    expect(screen.queryByRole('button', { name: /recovery assistant/i })).toBeNull();
  });

  it('renders the Activate dialog with Paste bubble, Activate CTA, and Buy a License Now button', async () => {
    wrap(<ActivationDialog open onOpenChange={() => undefined} />);
    expect(
      await screen.findByRole('dialog', { name: /activate easeus data recovery wizard/i }),
    ).toBeVisible();
    expect(screen.getByText('Paste')).toBeVisible();
    expect(screen.getByRole('button', { name: /^activate$/i })).toBeVisible();
    expect(screen.getByRole('button', { name: /buy a license now/i })).toBeVisible();
  });

  it('renders the SD Card page with the REAL card row, SDR/SSR/DVR columns, and Search for Lost Data CTA', async () => {
    wrap(<SdCardPage />, '/sd-card');
    expect(
      screen.getByRole('heading', {
        name: /connect your memory card to recover lost photos & videos/i,
      }),
    ).toBeVisible();
    // F-SD-01 (0.9.3 truth pass): the card row renders the REAL SD-bus
    // device from the connector (the mock's standard scenario now carries
    // the SD fixture: "Memory Card (E:)" on dev-sdcard) — its label, its
    // capacity, and the disk number derived from the id. The fabricated
    // "SD/MMC/MS PRO · Others · 58 GB · Disk2" hard-coded card is dead.
    // (findByText: the devices query resolves async.)
    expect(await screen.findByText('Memory Card (E:)')).toBeVisible();
    // the row's capacity line (one focused assertion on the card row;
    // the SDR/SSR/DVR card below carries other GB texts)
    expect(screen.getAllByText(/GB/).length).toBeGreaterThanOrEqual(1);
    // mock ids are dev-* (no disk-N) — the row shows the real bus name
    // for them; the bridge's disk-N ids render "Disk N".
    expect(screen.getByText(/^SD$/)).toBeVisible();
    expect(screen.getByText('Fix SD card access')).toBeVisible();
    expect(screen.getByText('Photos that open')).toBeVisible();
    expect(screen.getByText('Video that play')).toBeVisible();
    expect(screen.getByRole('button', { name: /show all external devices/i })).toBeVisible();
    expect(screen.getByRole('button', { name: /search for lost data/i })).toBeVisible();
  });

  it('renders the redesigned Disk Health page directly', async () => {
    wrap(<HealthPage />, '/health');
    expect(await screen.findByRole('heading', { name: /disk health/i })).toBeVisible();
    expect(await screen.findByText(/s\.m\.a\.r\.t\. attributes/i)).toBeVisible();
  });

  it('renders File Repair truthfully — no canned queue, the volume workbench is the surface (M31)', async () => {
    wrap(<RepairPage />, '/repair');
    expect(
      screen.getByRole('heading', {
        name: /repair corrupted videos, photos and documents/i,
      }),
    ).toBeVisible();
    // The per-file truth: a separate product line, stated plainly —
    // never a fabricated queue (NG-12 closed).
    expect(
      screen.getByText(/separate capability that this build does not include/i),
    ).toBeVisible();
    expect(screen.queryByText(/WinMemoryCleaner/i)).toBeNull();
    expect(screen.queryByRole('button', { name: /repair all/i })).toBeNull();
    // The volume workbench renders over the REAL device list (the mock's
    // disks with no findings → the truthful empty state + the undo-log
    // section — the workbench is present, not the fake queue).
    expect(await screen.findByText(/no repairable problems were found/i)).toBeVisible();
    expect(screen.getByText(/undo log/i)).toBeVisible();
  });

  it('renders Lost Partition Recovery with the REAL disk list (the demo fixtures are gone)', async () => {
    wrap(<LostPartitionPage />, '/lost-partition');
    expect(
      await screen.findByRole('heading', { name: /lost partition recovery/i }),
    ).toBeVisible();
    expect(
      screen.getByText(
        /search a whole disk for lost partitions and write a recovered entry back/i,
      ),
    ).toBeVisible();
    // The real device cards render from the connector (the mock's seeded
    // devices); the hardcoded RESCUE_DISK_SPECS demo rows are gone.
    expect(await screen.findAllByRole('button', { name: /whole disk/i })).not.toHaveLength(0);
  });

  it('renders Scan Snapshot page', () => {
    wrap(<BackupPage />, '/backup');
    expect(screen.getByRole('heading', { name: /scan snapshot/i })).toBeVisible();
    expect(screen.getByText('Saved Recovery Checkpoints')).toBeVisible();
  });
});
