import { describe, expect, it, afterEach } from 'vitest';
import { cleanup, fireEvent, render, screen } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ThemeProvider } from '@/app/theme';
import { TooltipProvider } from '@/components/ui/tooltip';
import { WizardShell } from '@/app/shell';
import { SdCardPage } from '@/features/device-selection/sd-card-page';
import { ToolsPage } from '@/features/tools/tools-page';
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
    expect(screen.getByText('More Recovery Tools')).toBeVisible();
    expect(screen.getByText('File Repair')).toBeVisible();
    expect(screen.getByText('Lost Partition Rescue')).toBeVisible();
    expect(screen.getByText('ChronoSnap Backup')).toBeVisible();
  });

  it('opens the robot helper popover ("Hi! Guess you need:") from the title bar', async () => {
    wrap(<WizardShell />);
    fireEvent.click(screen.getByRole('button', { name: /recovery assistant/i }));
    expect(await screen.findByText(/hi! guess you need:/i)).toBeVisible();
    expect(screen.getByText(/unable to find your hard drive\?/i)).toBeVisible();
    expect(screen.getByText(/try advanced file repair/i)).toBeVisible();
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

  it('renders the SD Card page with SDR, SSR, DVR columns and detection banner', () => {
    wrap(<SdCardPage />, '/sd-card');
    expect(
      screen.getByRole('heading', {
        name: /connect your memory card to recover lost photos & videos/i,
      }),
    ).toBeVisible();
    expect(screen.getByText('Fix SD card access')).toBeVisible();
    expect(screen.getByText('Photos that open')).toBeVisible();
    expect(screen.getByText('Video that play')).toBeVisible();
    expect(screen.getByRole('button', { name: /show all external devices/i })).toBeVisible();
  });

  it('renders More Recovery Tools with all 6 reference cards and opens tool dialog', async () => {
    wrap(<ToolsPage />, '/tools');
    expect(screen.getByRole('heading', { name: /more recovery tools/i })).toBeVisible();
    expect(screen.getByText('Crashed PC Rescue')).toBeVisible();
    expect(screen.getByText('NAS and Linux')).toBeVisible();
    expect(screen.getByText('RAID Recovery')).toBeVisible();
    expect(screen.getByText('Cloud Recovery')).toBeVisible();
    expect(screen.getByText('Image Recovery')).toBeVisible();
    expect(screen.getByText('Disk Health')).toBeVisible();
    fireEvent.click(screen.getByText('Crashed PC Rescue'));
    expect(await screen.findByRole('dialog', { name: /crashed pc rescue/i })).toBeVisible();
  });

  it('renders File Repair landing view with 3 repair rows and Start CTA', () => {
    wrap(<RepairPage />, '/repair');
    expect(
      screen.getByRole('heading', {
        name: /repair corrupted videos, photos and documents/i,
      }),
    ).toBeVisible();
    expect(screen.getByText('Repair corrupted or unplayable videos')).toBeVisible();
    expect(screen.getByText('Repair damaged or partially missing photos')).toBeVisible();
    expect(screen.getByText('Repair damaged or cannot be opened documents')).toBeVisible();
    expect(screen.getByRole('button', { name: /^start$/i })).toBeVisible();
  });

  it('renders Lost Partition Rescue landing view with 3 partition rows and Start CTA', async () => {
    wrap(<LostPartitionPage />, '/lost-partition');
    expect(
      await screen.findByRole('heading', { name: /lost partition rescue/i }),
    ).toBeVisible();
    expect(screen.getByText('Partition Missing')).toBeVisible();
    expect(screen.getByText('Partition Cannot Be Opened')).toBeVisible();
    expect(screen.getByText('BitLocker Partition Missing/Damaged')).toBeVisible();
    expect(screen.getByRole('button', { name: /^start$/i })).toBeVisible();
  });

  it('renders ChronoSnap Backup page', () => {
    wrap(<BackupPage />, '/backup');
    expect(screen.getByRole('heading', { name: /chronosnap backup/i })).toBeVisible();
    expect(screen.getByText('Saved Recovery Checkpoints')).toBeVisible();
  });
});
