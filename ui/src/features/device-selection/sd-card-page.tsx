import { MemoryStickIcon, RefreshCcwIcon, ScanSearchIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router';
import { useDevices, useRefreshDevices } from '@/queries/devices';
import { useStartScan } from '@/queries/scan';
import { Button } from '@/components/ui/button';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { formatBytes } from '@/lib/format';
import { toast } from 'sonner';

/** SD-card landing screen: a connection state when empty and real removable
 * device cards when detected. Every primary action starts the real scan. */
export function SdCardPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const devices = useDevices();
  const refresh = useRefreshDevices();
  const start = useStartScan();
  const removable = (devices.data ?? []).filter(
    (device) => device.kind === 'sd' || device.kind === 'usb',
  );
  const scan = (targetId: string): void => {
    start.mutate(
      { targetId },
      {
        onSuccess: ({ scanId }) => void navigate(`/scan/${targetId}/live?scanId=${scanId}`),
        onError: (error) =>
          toast.error(error instanceof Error ? error.message : t('errors.unknown')),
      },
    );
  };

  if (devices.isLoading) {
    return (
      <div className="p-6">
        <Skeleton className="mx-auto h-72 max-w-3xl" />
      </div>
    );
  }
  return (
    <div className="h-full overflow-y-auto">
      <section className="mx-auto w-full max-w-4xl p-6">
        <div className="text-center">
          <span className="mx-auto grid size-14 place-items-center rounded-xl bg-primary-muted text-primary">
            <MemoryStickIcon className="size-7" aria-hidden />
          </span>
          <h1 className="mt-4 text-xl font-semibold">{t('sdCard.title')}</h1>
          <p className="mx-auto mt-2 max-w-xl text-sm text-foreground-secondary">
            {t('sdCard.subtitle')}
          </p>
        </div>
        {removable.length === 0 ? (
          <div className="mt-8">
            <EmptyState
              icon={MemoryStickIcon}
              title={t('sdCard.emptyTitle')}
              description={t('sdCard.emptyBody')}
              actionLabel={t('sdCard.refresh')}
              onAction={() => void refresh.mutate()}
            />
          </div>
        ) : (
          <div className="mt-8 grid gap-3 sm:grid-cols-2">
            {removable.map((device) => (
              <article
                key={device.id}
                className="rounded-lg border border-border bg-surface p-4 shadow-card"
              >
                <div className="flex items-start gap-3">
                  <MemoryStickIcon className="mt-0.5 size-6 text-primary" aria-hidden />
                  <div className="min-w-0 flex-1">
                    <h2 className="truncate text-sm font-semibold">{device.label}</h2>
                    <p className="mt-0.5 text-xs text-foreground-secondary">
                      {device.model} · {formatBytes(device.capacity.totalBytes, i18n.language)}
                    </p>
                  </div>
                </div>
                <Button
                  className="mt-4"
                  size="sm"
                  variant="primary"
                  loading={start.isPending}
                  onClick={() => scan(device.id)}
                >
                  <ScanSearchIcon aria-hidden />
                  {t('sdCard.start')}
                </Button>
              </article>
            ))}
          </div>
        )}
        <Button
          className="mx-auto mt-6"
          variant="ghost"
          size="sm"
          loading={refresh.isPending}
          onClick={() => void refresh.mutate()}
        >
          <RefreshCcwIcon aria-hidden />
          {t('sdCard.refresh')}
        </Button>
      </section>
    </div>
  );
}
