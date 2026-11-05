import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router';
import {
  ArrowLeftIcon,
  HardDriveIcon,
  HomeIcon,
  LockOpenIcon,
  RefreshCcwIcon,
  ShieldCheckIcon,
  Undo2Icon,
} from 'lucide-react';
import { useDevices, useRefreshDevices } from '@/queries/devices';
import { useScanState, useStartScan } from '@/queries/scan';
import { useConsentStore } from '@/stores/consent-store';
import {
  useApplyConsentBridge,
  useApplyConsentPending,
  useApplyLostPartition,
  useSaveApplyUndo,
  useUndoLastApply,
} from '@/queries/lost-partition';
import type { Device, LostCandidateRow } from '@/connector/contract';
import { formatBytes, formatInteger } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * Lost Partition — the S-029 surface, REAL DATA ONLY.
 *
 * The v3-era page shipped hardcoded demo fixtures (RESCUE_DISK_SPECS — the
 * fake surface the consolidation mandate removes). This page now runs the
 * true flow: pick a REAL disk -> whole-disk scan (the engine's lost-partition
 * discovery) -> the live candidate rows from scan state (the N-061 scored
 * geometry, never synthesized) -> the consent-gated apply + undo + undo-file
 * save, with the outcome surfaced verbatim (0 applied … 5 error).
 */

/** lost::CandidateFs — the engine's serialized family values. */
const CANDIDATE_FS: Readonly<Record<number, string>> = {
  0: 'NTFS',
  1: 'FAT',
  2: 'exFAT',
  4: 'GPT-table',
  5: 'Unknown',
  6: 'ext2',
  7: 'HFS+',
  8: 'Btrfs',
  9: 'ZFS',
  10: 'ReFS',
  11: 'BitLocker',
};

interface ActiveSearch {
  deviceId: string;
  scanId: string;
}

export function LostPartitionPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const devices = useDevices();
  const refresh = useRefreshDevices();
  const startScan = useStartScan();
  const applyLostPartition = useApplyLostPartition();
  const undoLastApply = useUndoLastApply();
  const saveApplyUndo = useSaveApplyUndo();
  const setApply = useConsentStore((s) => s.setApply);
  const [search, setSearch] = React.useState<ActiveSearch | null>(null);
  const pendingConsent = useApplyConsentPending(search?.deviceId ?? null);
  const [selected, setSelected] = React.useState<LostCandidateRow | null>(null);

  const scanState = useScanState(search?.scanId ?? null);
  const candidates = scanState.data?.lostCandidates?.rows ?? [];
  const scanning =
    scanState.isLoading ||
    ['starting', 'running', 'pausing', 'resuming', 'stopping'].includes(
      scanState.data?.status ?? 'starting',
    );

  // The apply-consent feed (event + re-hydration; the modal lives in the
  // shell and the store carries it).
  useApplyConsentBridge(search?.deviceId ?? null);
  React.useEffect(() => {
    if (pendingConsent.data) setApply(pendingConsent.data);
  }, [pendingConsent.data, setApply]);

  const onStartSearch = (device: Device): void => {
    startScan.mutate(
      { targetId: device.id },
      {
        onSuccess: ({ scanId }) => {
          setSearch({ deviceId: device.id, scanId });
          setSelected(null);
        },
        onError: (err) => toast.error(err instanceof Error ? err.message : t('errors.unknown')),
      },
    );
  };

  const onApply = (): void => {
    if (!search || !selected) return;
    applyLostPartition.mutate(
      { deviceId: search.deviceId, candidate: selected, rebuildTable: false },
      {
        onSuccess: (result) => {
          toast.message(
            result.applied
              ? t('lostPartition.applyApplied', { count: result.undoEntries })
              : t('lostPartition.applyOutcome', { outcome: result.outcome }),
            { description: result.reason || undefined },
          );
        },
        onError: (err) => {
          toast.error(err instanceof Error ? err.message : t('errors.unknown'));
        },
      },
    );
  };

  const onUndo = (): void => {
    if (!search) return;
    undoLastApply.mutate(search.deviceId, {
      onSuccess: (result) => {
        toast.message(
          result.applied
            ? t('lostPartition.undoDone', { count: result.undoEntries })
            : t('lostPartition.undoEmpty'),
        );
      },
      onError: (err) => {
        toast.error(err instanceof Error ? err.message : t('errors.unknown'));
      },
    });
  };

  if (search) {
    return (
      <div className="flex h-full min-h-0 flex-col overflow-y-auto px-8 py-6">
        <div className="mx-auto w-full max-w-5xl">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
                {t('lostPartition.searchTitle')}
              </h1>
              <p className="mt-1 text-[13px] text-[#64748B] dark:text-foreground-secondary">
                {scanning
                  ? t('lostPartition.searchingBody')
                  : t('lostPartition.resultsBody', {
                      count: candidates.length,
                    })}
              </p>
            </div>
            <Button variant="secondary" size="sm" onClick={() => setSearch(null)}>
              <ArrowLeftIcon className="size-4" aria-hidden />
              {t('lostPartition.backToDisks')}
            </Button>
          </div>

          {scanning ? (
            <div className="mt-6 space-y-3">
              <Skeleton className="h-14" />
              <Skeleton className="h-14" />
              <Skeleton className="h-14" />
            </div>
          ) : candidates.length === 0 ? (
            <div className="mt-6">
              <EmptyState
                icon={LockOpenIcon}
                title={t('lostPartition.emptyTitle')}
                description={t('lostPartition.emptyBody')}
              />
            </div>
          ) : (
            <div className="mt-4 overflow-hidden rounded-[10px] border border-[#E2E8F0] dark:border-border">
              <table className="w-full text-[13px]">
                <thead>
                  <tr className="border-b border-[#E2E8F0] bg-[#F8FAFD] text-left text-[12px] text-[#64748B] dark:border-border dark:bg-surface dark:text-foreground-secondary">
                    <th className="px-3.5 py-2 font-medium">{t('lostPartition.colLba')}</th>
                    <th className="px-3.5 py-2 font-medium">{t('lostPartition.colSectors')}</th>
                    <th className="px-3.5 py-2 font-medium">{t('lostPartition.colSize')}</th>
                    <th className="px-3.5 py-2 font-medium">{t('lostPartition.colFs')}</th>
                    <th className="px-3.5 py-2 font-medium">{t('lostPartition.colScore')}</th>
                  </tr>
                </thead>
                <tbody>
                  {candidates.map((c, i) => {
                    const selectable = c.sectorCount !== null;
                    const active = selected?.lba === c.lba;
                    return (
                      <tr
                        key={`${c.lba}-${i}`}
                        onClick={() => selectable && setSelected(c)}
                        className={cn(
                          'border-b border-[#EEF2F7] last:border-0 dark:border-border',
                          selectable
                            ? 'cursor-pointer hover:bg-[#F4F8FF] dark:hover:bg-surface-hover'
                            : 'opacity-60',
                          active && 'bg-[#F4F8FF] dark:bg-primary-muted/40',
                        )}
                      >
                        <td className="px-3.5 py-2.5 tabular">
                          {formatInteger(c.lba, i18n.language)}
                        </td>
                        <td className="px-3.5 py-2.5 tabular">
                          {c.sectorCount !== null
                            ? formatInteger(c.sectorCount, i18n.language)
                            : '—'}
                        </td>
                        <td className="px-3.5 py-2.5 tabular">
                          {c.sectorCount !== null
                            ? formatBytes(c.sectorCount * 512, i18n.language)
                            : '—'}
                        </td>
                        <td className="px-3.5 py-2.5">
                          {c.fs !== null ? (CANDIDATE_FS[c.fs] ?? `#${c.fs}`) : '—'}
                        </td>
                        <td className="px-3.5 py-2.5 tabular">
                          {c.score !== null ? formatInteger(c.score, i18n.language) : '—'}
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          )}

          <div className="mt-5 flex items-center gap-3">
            <Button
              disabled={!selected || applyLostPartition.isPending || scanning}
              loading={applyLostPartition.isPending}
              onClick={onApply}
            >
              <ShieldCheckIcon className="size-4" aria-hidden />
              {t('lostPartition.applyAction')}
            </Button>
            <Button
              variant="secondary"
              disabled={applyLostPartition.isPending || undoLastApply.isPending}
              loading={undoLastApply.isPending}
              onClick={onUndo}
            >
              <Undo2Icon className="size-4" aria-hidden />
              {t('lostPartition.undoAction')}
            </Button>
            <Button
              variant="ghost"
              disabled={saveApplyUndo.isPending}
              loading={saveApplyUndo.isPending}
              onClick={() => {
                if (!search) return;
                const path = `undo-${search.deviceId}.bin`;
                saveApplyUndo.mutate(
                  { deviceId: search.deviceId, path },
                  {
                    onSuccess: () => toast.success(t('lostPartition.undoSaved', { path })),
                    onError: (err) =>
                      toast.error(err instanceof Error ? err.message : t('errors.unknown')),
                  },
                );
              }}
            >
              {t('lostPartition.saveUndoAction')}
            </Button>
            <p className="text-xs text-foreground-secondary">
              {t('lostPartition.applyConsentNote')}
            </p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-5xl">
        <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
          {t('lostPartition.title')}
        </h1>
        <p className="mt-1.5 text-[13px] text-[#64748B] dark:text-foreground-secondary">
          {t('lostPartition.subtitle')}
        </p>

        {devices.isLoading ? (
          <div className="mt-6 space-y-3">
            <Skeleton className="h-16" />
            <Skeleton className="h-16" />
          </div>
        ) : (devices.data?.length ?? 0) === 0 ? (
          <div className="mt-6">
            <EmptyState
              icon={HardDriveIcon}
              title={t('home.emptyTitle')}
              description={t('home.emptyBody')}
            />
          </div>
        ) : (
          <div className="mt-5 grid grid-cols-1 gap-3 md:grid-cols-2">
            {devices.data?.map((d) => (
              <button
                key={d.id}
                type="button"
                disabled={startScan.isPending}
                onClick={() => onStartSearch(d)}
                className={cn(
                  'flex items-center gap-4 rounded-[10px] border border-transparent bg-[#F8FAFD] px-5 py-4 text-left transition-all hover:border-[#93C5FD] hover:bg-[#EEF4FF] dark:bg-surface dark:hover:border-border-strong dark:hover:bg-surface-hover',
                  startScan.isPending && 'opacity-60',
                )}
              >
                <HardDriveIcon
                  className="size-8 shrink-0 text-[#1677FF] dark:text-primary"
                  aria-hidden
                />
                <div className="min-w-0 flex-1">
                  <div className="truncate text-[14px] font-semibold text-[#1E293B] dark:text-foreground">
                    {d.label}
                  </div>
                  <div className="text-[12px] text-[#64748B] tabular dark:text-foreground-secondary">
                    {formatBytes(d.capacity.totalBytes, i18n.language)} · {d.serial || d.model}
                  </div>
                </div>
                <Badge variant="primary">{t('lostPartition.diskBadge')}</Badge>
              </button>
            ))}
          </div>
        )}

        <div className="mt-5 flex items-center justify-between">
          <Button variant="ghost" size="sm" onClick={() => void navigate('/')}>
            <HomeIcon className="size-4" aria-hidden />
            {t('common.back')}
          </Button>
          <Button
            variant="ghost"
            size="sm"
            loading={refresh.isPending}
            onClick={() => void refresh.mutate()}
          >
            <RefreshCcwIcon className="size-4" aria-hidden />
            {t('home.refreshDevices')}
          </Button>
        </div>
      </div>
    </div>
  );
}
