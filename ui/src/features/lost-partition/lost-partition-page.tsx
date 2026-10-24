import * as React from 'react';
import { useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import {
  CircleHelpIcon,
  FolderSearchIcon,
  HardDriveIcon,
  LockIcon,
  UsbIcon,
} from 'lucide-react';
import { useDevices } from '@/queries/devices';
import { useStartScan } from '@/queries/scan';
import { formatBytes } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * Lost Partition Rescue (corpus atlas §5, DrwLostDiskPage): the sidebar
 * destination with three scenario cards — Partition Missing / Partition
 * Cannot Be Opened / BitLocker Partition Missing-Damaged — plus the disk
 * list and the Start CTA. The BitLocker card is a truthful degraded state
 * (the unlock chain is out of scope this session — UI-MAP §0).
 */
export function LostPartitionPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const devicesQuery = useDevices();
  const startScan = useStartScan();
  const [selectedId, setSelectedId] = React.useState<string | null>(null);

  const disks = (devicesQuery.data ?? []).filter(
    (d) => d.kind !== 'virtual' && d.canScanWholeDisk,
  );
  const selected = disks.find((d) => d.id === selectedId) ?? null;

  const beginScan = (): void => {
    if (!selected) return;
    startScan.mutate(
      { targetId: selected.id },
      {
        onSuccess: ({ scanId }) => {
          void navigate(`/scan/${selected.id}/live?scanId=${scanId}`);
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
      <div className="p-6">
        <Skeleton className="mb-6 h-8 w-80" />
        <div className="mb-6 grid grid-cols-1 gap-4 md:grid-cols-3">
          {Array.from({ length: 3 }, (_, i) => (
            <Skeleton key={i} className="h-32" />
          ))}
        </div>
        <Skeleton className="h-48" />
      </div>
    );
  }

  if (devicesQuery.isError) {
    return (
      <div className="p-6">
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
    <div className="flex h-full min-h-0 flex-col overflow-y-auto">
      <div className="mx-auto w-full max-w-4xl flex-1 p-6">
        <h1 className="text-xl font-semibold">{t('lostPartition.title')}</h1>
        <p className="mt-1 text-sm text-foreground-secondary">{t('lostPartition.subtitle')}</p>

        <div className="mt-6 grid grid-cols-1 gap-4 md:grid-cols-3">
          <ScenarioCard
            icon={FolderSearchIcon}
            title={t('lostPartition.scenarioMissing.title')}
            body={t('lostPartition.scenarioMissing.body')}
          />
          <ScenarioCard
            icon={CircleHelpIcon}
            title={t('lostPartition.scenarioUnopenable.title')}
            body={t('lostPartition.scenarioUnopenable.body')}
          />
          <ScenarioCard
            icon={LockIcon}
            title={t('lostPartition.scenarioBitlocker.title')}
            body={t('lostPartition.scenarioBitlocker.body')}
            note={t('lostPartition.bitlockerNote')}
          />
        </div>

        <h2 className="mb-3 mt-8 text-sm font-semibold text-foreground-secondary">
          {t('lostPartition.diskList')}
        </h2>
        {disks.length === 0 ? (
          <EmptyState
            icon={HardDriveIcon}
            title={t('home.emptyTitle')}
            description={t('home.emptyBody')}
            actionLabel={t('home.refreshDevices')}
            onAction={() => void devicesQuery.refetch()}
          />
        ) : (
          <div role="radiogroup" aria-label={t('lostPartition.diskList')} className="space-y-2">
            {disks.map((disk) => {
              const isSel = selectedId === disk.id;
              return (
                <button
                  key={disk.id}
                  type="button"
                  role="radio"
                  aria-checked={isSel}
                  onClick={() => setSelectedId(disk.id)}
                  className={cn(
                    'flex w-full items-center gap-4 rounded-lg border-2 bg-surface p-4 text-left shadow-card transition-all',
                    'focus-visible:ring-2 focus-visible:ring-ring',
                    isSel ? 'border-primary' : 'border-transparent hover:border-border-strong',
                  )}
                >
                  <div className="flex size-11 items-center justify-center rounded-md bg-primary-muted text-primary">
                    <UsbIcon className="size-5" strokeWidth={1.5} aria-hidden />
                  </div>
                  <div className="min-w-0 flex-1">
                    <div className="flex items-center gap-2">
                      <span className="truncate text-sm font-semibold">{disk.label}</span>
                      {disk.volumes.some((v) => v.lostPartition) ? (
                        <Badge variant="primary">{t('home.lostPartitionBadge')}</Badge>
                      ) : null}
                    </div>
                    <div className="text-xs text-foreground-secondary tabular">
                      {disk.model} · {formatBytes(disk.capacity.totalBytes, i18n.language)}
                    </div>
                  </div>
                </button>
              );
            })}
          </div>
        )}
      </div>

      <div className="sticky bottom-0 flex items-center justify-end gap-3 border-t border-border bg-surface/90 px-6 py-3 backdrop-blur">
        <span className="text-sm text-foreground-secondary">
          {selected ? selected.label : t('common.empty')}
        </span>
        <Button
          variant="primary"
          size="lg"
          disabled={!selected || startScan.isPending}
          loading={startScan.isPending}
          onClick={beginScan}
        >
          {t('lostPartition.start')}
        </Button>
      </div>
    </div>
  );
}

function ScenarioCard({
  icon: Icon,
  title,
  body,
  note,
}: {
  icon: React.ComponentType<{ className?: string; strokeWidth?: number }>;
  title: string;
  body: string;
  note?: string;
}): React.JSX.Element {
  return (
    <div className="flex flex-col gap-2 rounded-lg border border-border bg-surface p-4 shadow-card">
      <div className="flex items-center gap-2.5">
        <div className="flex size-9 items-center justify-center rounded-md bg-primary-muted text-primary">
          <Icon className="size-5" strokeWidth={1.5} aria-hidden />
        </div>
        <span className="text-sm font-semibold">{title}</span>
      </div>
      <p className="text-xs leading-relaxed text-foreground-secondary">{body}</p>
      {note ? (
        <p className="mt-auto rounded-xs bg-warning-muted px-2 py-1.5 text-2xs text-warning">
          {note}
        </p>
      ) : null}
    </div>
  );
}
