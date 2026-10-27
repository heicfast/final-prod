import * as React from 'react';
import { useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import {
  CheckIcon,
  HardDriveIcon,
  InfoIcon,
  LockOpenIcon,
  RefreshCcwIcon,
  SearchIcon,
  UsbIcon,
} from 'lucide-react';
import { useDevices, useRefreshDevices } from '@/queries/devices';
import { useStartScan } from '@/queries/scan';
import { formatDriveGb } from '@/lib/format';
import { cn } from '@/lib/utils';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { Drive3dIcon } from '@/components/ui/brand-icons';
import { toast } from 'sonner';

/**
 * Lost Partition Rescue — the REAL surface (NG-11).
 *
 * The pre-fix page rendered a canned disk table (hard-coded specs, fake
 * partitions with fake LBAs and a fake 'Smart Partition Scan Completed'
 * step without any scan). This page is wired to the real pipeline: the
 * selected disk's WHOLE-DISK scan (the bridge arms whole_disk for disk
 * targets — engine lost-partition discovery + the Lost Files tree), real
 * progress on the scan page, real results. Writing a recovered partition
 * back to the partition table is honestly marked not-in-this-UI: it ships
 * today through the `rrecov apply` CLI beside the app (the bridge/UI apply
 * flow is the recorded follow-up).
 */
export function LostPartitionPage(): React.JSX.Element {
  const { t } = useTranslation();
  const navigate = useNavigate();
  const devicesQuery = useDevices();
  const refresh = useRefreshDevices();
  const startScan = useStartScan();

  const [selectedId, setSelectedId] = React.useState<string | null>(null);

  const disks = (devicesQuery.data ?? []).filter(
    (d) => d.kind !== 'virtual' && d.canScanWholeDisk,
  );
  const selected = disks.find((d) => d.id === selectedId) ?? disks[0] ?? null;

  const beginSearch = (targetDiskId: string): void => {
    startScan.mutate(
      { targetId: targetDiskId },
      {
        onSuccess: ({ scanId }) => {
          void navigate(`/scan/${targetDiskId}/live?scanId=${scanId}`);
        },
        onError: (err) => {
          const reasonKey = (err as { reasonKey?: string }).reasonKey;
          toast.error(reasonKey ? t(reasonKey) : err.message);
        },
      },
    );
  };

  if (devicesQuery.isLoading) {
    return (
      <div className="p-8">
        <Skeleton className="mb-6 h-8 w-80" />
        <Skeleton className="h-64" />
      </div>
    );
  }

  if (devicesQuery.isError) {
    return (
      <div className="p-8">
        <EmptyState
          icon={HardDriveIcon}
          title={t('errors.title')}
          description={t('errors.deviceGone')}
          actionLabel={t('common.retry')}
          onAction={() => void devicesQuery.refetch()}
        />
      </div>
    );
  }

  return (
    <div className="h-full overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-5xl space-y-5 pb-6">
        <div className="flex items-start justify-between gap-4">
          <div>
            <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
              {t('lostPartition.title')}
            </h1>
            <p className="mt-1 text-[13px] text-[#64748B] dark:text-foreground-secondary">
              {t('lostPartition.subtitle')}
            </p>
          </div>
          <button
            type="button"
            onClick={() => void refresh.mutate()}
            aria-label={t('home.refreshDevices')}
            title={t('home.refreshDevices')}
            className="flex size-9 items-center justify-center rounded-full border border-[#E2ECF9] bg-white text-[#64748B] transition-colors hover:border-[#93C5FD] hover:text-[#1677FF] dark:border-border dark:bg-surface dark:text-foreground-secondary dark:hover:text-primary"
          >
            <RefreshCcwIcon className="size-4" aria-hidden />
          </button>
        </div>

        {disks.length === 0 ? (
          <EmptyState
            icon={HardDriveIcon}
            title={t('lostPartition.noDisksTitle')}
            description={t('lostPartition.noDisksBody')}
            actionLabel={t('common.retry')}
            onAction={() => void refresh.mutate()}
          />
        ) : (
          <>
            <div
              className="grid grid-cols-1 gap-4 sm:grid-cols-2"
              role="radiogroup"
              aria-label={t('lostPartition.diskList')}
            >
              {disks.map((disk) => {
                const isSelected = selected?.id === disk.id;
                const locked = disk.status === 'permission-denied';
                const freeBytes = disk.capacity.freeBytes;
                return (
                  <button
                    key={disk.id}
                    type="button"
                    role="radio"
                    aria-checked={isSelected}
                    onClick={() => setSelectedId(disk.id)}
                    className={cn(
                      'group relative flex items-center gap-4 rounded-[10px] border px-5 py-4 text-left transition-all duration-150 outline-none focus-visible:ring-2 focus-visible:ring-ring',
                      isSelected
                        ? 'border-[#1677FF] bg-[#F4F8FF] ring-1 ring-[#1677FF] dark:border-primary dark:bg-primary-muted/40 dark:ring-primary'
                        : 'border-transparent bg-[#F8FAFD] hover:border-[#93C5FD] hover:bg-[#EEF4FF] dark:border-border dark:bg-surface dark:hover:border-border-strong dark:hover:bg-surface-hover',
                    )}
                  >
                    {isSelected ? (
                      <span
                        className="absolute -right-2 -top-2 z-10 flex size-5 items-center justify-center rounded-full bg-[#1677FF] text-white shadow-xs ring-2 ring-white dark:bg-primary-fill dark:text-primary-foreground dark:ring-background"
                        aria-hidden
                      >
                        <CheckIcon className="size-3 stroke-[3]" />
                      </span>
                    ) : null}
                    <Drive3dIcon isSystemDrive={false} bitlocker={false} />
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center gap-2">
                        <span className="truncate text-[14px] font-semibold text-[#1E293B] dark:text-foreground">
                          {disk.label}
                        </span>
                        {disk.kind !== 'internal' ? (
                          <UsbIcon className="size-3.5 text-[#64748B]" aria-hidden />
                        ) : null}
                        {locked ? (
                          <LockOpenIcon
                            className="size-3.5 text-warning"
                            aria-label={t('home.permissionDenied')}
                          />
                        ) : null}
                      </div>
                      <div className="truncate text-[12px] text-[#64748B] dark:text-foreground-secondary">
                        {disk.model} ·{' '}
                        {disk.volumes.length > 0
                          ? disk.volumes.map((v) => v.letter ?? v.label).join(' ')
                          : t('lostPartition.noVolumes')}
                      </div>
                      <div className="mt-1.5 text-[12px] text-[#64748B] tabular dark:text-foreground-secondary">
                        {formatDriveGb(freeBytes)}/{formatDriveGb(disk.capacity.totalBytes)}
                      </div>
                    </div>
                  </button>
                );
              })}
            </div>

            <div className="flex items-center justify-between gap-4 rounded-[12px] border border-[#E2ECF9] bg-[#F8FAFD] px-5 py-4 dark:border-border dark:bg-surface">
              <p className="max-w-[46rem] text-[12px] leading-relaxed text-[#64748B] dark:text-foreground-secondary">
                <InfoIcon className="mr-1.5 inline size-3.5" aria-hidden />
                {t('lostPartition.explainer')}
              </p>
              <button
                type="button"
                onClick={() => selected && beginSearch(selected.id)}
                disabled={selected == null || startScan.isPending}
                className="inline-flex h-9 shrink-0 items-center justify-center gap-2 rounded-[8px] bg-[#1677FF] px-4 text-[13px] font-semibold text-white shadow-2xs transition-all hover:bg-[#0958D9] active:scale-[var(--press-scale)] focus-visible:ring-2 focus-visible:ring-ring disabled:opacity-50 dark:bg-primary-fill dark:text-primary-foreground dark:hover:bg-primary-fill-hover"
              >
                <SearchIcon className="size-4" aria-hidden />
                {startScan.isPending
                  ? t('lostPartition.searching')
                  : t('lostPartition.searchCta')}
              </button>
            </div>

            <p className="text-[11.5px] leading-relaxed text-[#94A3B8] dark:text-foreground-secondary">
              {t('lostPartition.applyNote')}
            </p>
          </>
        )}
      </div>
    </div>
  );
}
