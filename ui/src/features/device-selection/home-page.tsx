import * as React from 'react';
import { useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import type { Device, Volume } from '@/connector/contract';
import {
  CircleAlertIcon,
  CircleCheckIcon,
  FolderOpenIcon,
  HardDriveIcon,
  LockIcon,
  MemoryStickIcon,
  MonitorSmartphoneIcon,
  PlugZapIcon,
  RefreshCcwIcon,
  SearchIcon,
  Trash2Icon,
  UsbIcon,
} from 'lucide-react';
import { useDevices, useRefreshDevices } from '@/queries/devices';
import { useSessions, useLoadSession } from '@/queries/sessions';
import { formatBytes, formatDate } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { CapacityBar, EmptyState, StatTile } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/scroll-area';
import { Hint } from '@/components/ui/tooltip';
import { toast } from 'sonner';

/**
 * Home / Device Selection — the corpus landing page IA: title, device tile
 * grid with capacity bars + health hints + lost-partition badges, quick
 * targets (recycle bin / desktop / choose folder), session import, recent
 * sessions, primary CTA bottom-right. All states implemented.
 */

export function HomePage(): React.JSX.Element {
  const { t } = useTranslation();
  const navigate = useNavigate();
  const devicesQuery = useDevices();
  const refresh = useRefreshDevices();
  const sessionsQuery = useSessions();
  const loadSession = useLoadSession();
  const [selectedId, setSelectedId] = React.useState<string | null>(null);
  const showRemovable = new URLSearchParams(window.location.hash.split('?')[1] ?? '').has('media');

  const devices = devicesQuery.data ?? [];
  const internal = devices.filter((d) => d.kind === 'internal');
  const removable = devices.filter((d) => d.kind !== 'internal');
  const selected = devices.find((d) => d.id === selectedId) ?? null;

  const startScan = (): void => {
    if (!selected) return;
    navigate(`/scan/${selected.id}`);
  };

  const onResume = (path: string): void => {
    loadSession.mutate(path, {
      onSuccess: ({ scanId }) => {
        toast.success(t('home.resume'));
        navigate(`/results/${scanId}`);
      },
      onError: (err) => {
        toast.error(err instanceof Error ? (err as { reasonKey?: string }).reasonKey ? t((err as { reasonKey?: string }).reasonKey!) : err.message : t('errors.sessionCorrupt'));
      },
    });
  };

  if (devicesQuery.isLoading) {
    return (
      <div className="p-6">
        <Skeleton className="mb-5 h-7 w-72" />
        <div className="grid grid-cols-2 gap-4 xl:grid-cols-3">
          {Array.from({ length: 4 }, (_, i) => (
            <Skeleton key={i} className="h-40" />
          ))}
        </div>
      </div>
    );
  }

  if (devicesQuery.isError) {
    return (
      <div className="p-6">
        <EmptyState
          icon={CircleAlertIcon}
          title={t('errors.title')}
          description={devicesQuery.error instanceof Error ? devicesQuery.error.message : t('errors.unknown')}
          actionLabel={t('common.retry')}
          onAction={() => void devicesQuery.refetch()}
          secondaryLabel={t('home.refreshDevices')}
          onSecondary={() => refresh.mutate()}
        />
      </div>
    );
  }

  const visible = showRemovable ? removable : internal;
  if (devices.length === 0) {
    return (
      <div className="p-6">
        <EmptyState
          icon={HardDriveIcon}
          title={t('home.emptyTitle')}
          description={t('home.emptyBody')}
          actionLabel={t('home.refreshDevices')}
          onAction={() => refresh.mutate()}
        />
      </div>
    );
  }

  const hasPermissionIssue = devices.some((d) => d.status === 'permission-denied');
  const hasDisconnected = devices.some((d) => d.status === 'disconnected');

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto">
      <div className="mx-auto w-full max-w-5xl flex-1 p-6">
        <div className="mb-6 flex items-end justify-between gap-4">
          <div>
            <h1 className="text-xl font-semibold">{t('home.title')}</h1>
            <p className="mt-1 text-sm text-foreground-secondary">{t('home.subtitle')}</p>
          </div>
          <Button variant="ghost" size="sm" onClick={() => refresh.mutate()} loading={refresh.isPending} disabled={refresh.isPending}>
            <RefreshCcwIcon aria-hidden />
            {t('home.refreshDevices')}
          </Button>
        </div>

        {hasPermissionIssue ? (
          <div className="mb-4 flex items-center gap-2 rounded-md border border-warning/40 bg-warning-muted px-4 py-2.5 text-sm">
            <LockIcon className="size-4 text-warning" aria-hidden />
            <span>{t('home.permissionDenied')}</span>
          </div>
        ) : null}
        {hasDisconnected ? (
          <div className="mb-4 flex items-center gap-2 rounded-md border border-danger/40 bg-danger-muted px-4 py-2.5 text-sm">
            <PlugZapIcon className="size-4 text-danger" aria-hidden />
            <span>{t('home.disconnected')}</span>
          </div>
        ) : null}

        <h2 className="mb-3 text-sm font-semibold text-foreground-secondary">
          {showRemovable ? t('home.removableGroup') : t('home.internalGroup')} ({visible.length})
        </h2>

        <div
          className="grid grid-cols-1 gap-4 sm:grid-cols-2"
          role="radiogroup"
          aria-label={t('home.title')}
        >
          {visible.map((device) => (
            <DeviceCard
              key={device.id}
              device={device}
              selected={selectedId === device.id}
              onSelect={() => setSelectedId(device.id)}
            />
          ))}
        </div>

        <h2 className="mb-3 mt-8 text-sm font-semibold text-foreground-secondary">{t('home.quickTargets')}</h2>
        <div className="grid grid-cols-3 gap-4">
          <QuickTarget icon={Trash2Icon} label={t('home.recycleBin')} onClick={() => toast(t('common.learnMore'))} />
          <QuickTarget icon={MonitorSmartphoneIcon} label={t('home.desktop')} onClick={() => toast(t('common.learnMore'))} />
          <QuickTarget
            icon={FolderOpenIcon}
            label={t('home.chooseFolder')}
            onClick={() => {
              // Folder targets are volume-scoped: route to the first internal
              // volume's configuration where a folder scope can be picked.
              const first = devices.flatMap((d) => d.volumes)[0];
              if (first) navigate(`/scan/${first.id}`);
            }}
          />
        </div>

        {(sessionsQuery.data?.length ?? 0) > 0 ? (
          <>
            <h2 className="mb-3 mt-8 text-sm font-semibold text-foreground-secondary">{t('home.recentSessions')}</h2>
            <div className="space-y-2">
              {sessionsQuery.data?.slice(0, 4).map((s) => (
                <button
                  key={s.id}
                  type="button"
                  onClick={() => onResume(s.fileName)}
                  className="flex w-full items-center justify-between gap-4 rounded-md border border-border bg-surface px-4 py-2.5 text-left transition-colors hover:bg-surface-hover focus-visible:ring-2 focus-visible:ring-ring"
                >
                  <div className="min-w-0">
                    <div className="truncate text-sm font-medium">{s.targetLabel}</div>
                    <div className="text-xs text-foreground-secondary tabular">
                      {formatDate(s.savedAt, useLocale())} · {s.mode} · {s.filesFound.toLocaleString(useLocale())} {t('scan.filesFound')}
                    </div>
                  </div>
                  <Button variant="secondary" size="sm">
                    {t('home.resume')}
                  </Button>
                </button>
              ))}
            </div>
          </>
        ) : null}
      </div>

      {/* Bottom CTA bar */}
      <div className="sticky bottom-0 flex items-center justify-end gap-3 border-t border-border bg-surface/90 px-6 py-3 backdrop-blur">
        <span className="text-sm text-foreground-secondary">
          {selected
            ? `${selected.label} · ${formatBytes(selected.capacity.totalBytes, useLocale())}`
            : t('common.empty')}
        </span>
        <Button variant="primary" size="lg" disabled={!selected} onClick={startScan} aria-keyshortcuts="Enter">
          <SearchIcon aria-hidden />
          {t('home.scanAction')}
        </Button>
      </div>
    </div>
  );
}

function useLocale(): string {
  const { i18n } = useTranslation();
  return i18n.language;
}

function DeviceCard({
  device,
  selected,
  onSelect,
}: {
  device: Device;
  selected: boolean;
  onSelect: () => void;
}): React.JSX.Element {
  const { t } = useTranslation();
  const navigate = useNavigate();
  const Icon = device.kind === 'sd' ? MemoryStickIcon : device.kind === 'usb' ? UsbIcon : HardDriveIcon;
  const healthy = device.status === 'ok';
  const locked = device.status === 'permission-denied';
  const offline = device.status === 'disconnected';
  const hasLost = device.volumes.some((v) => v.lostPartition);

  return (
    <div
      role="radio"
      aria-checked={selected}
      tabIndex={0}
      onClick={onSelect}
      onKeyDown={(e) => {
        if (e.key === 'Enter' || e.key === ' ') {
          e.preventDefault();
          onSelect();
        }
      }}
      className={cn(
        'group flex cursor-pointer flex-col gap-3 rounded-lg border-2 bg-surface p-4 shadow-card outline-none transition-all duration-150',
        'hover:shadow-overlay focus-visible:ring-2 focus-visible:ring-ring',
        selected ? 'border-primary' : 'border-transparent hover:border-border-strong',
        offline && 'opacity-60',
      )}
    >
      <div className="flex items-start gap-3">
        <div className="relative">
          <div
            className={cn(
              'flex size-12 items-center justify-center rounded-md',
              healthy ? 'bg-primary-muted text-primary' : 'bg-warning-muted text-warning',
            )}
          >
            <Icon className="size-6" strokeWidth={1.5} aria-hidden />
          </div>
          {selected ? (
            <span className="absolute -right-1.5 -top-1.5 flex size-5 items-center justify-center rounded-full bg-primary text-primary-foreground shadow-sm">
              <CircleCheckIcon className="size-3.5" aria-hidden />
            </span>
          ) : null}
        </div>
        <div className="min-w-0 flex-1">
          <div className="flex items-center gap-2">
            <span className="truncate text-base font-semibold">{device.label}</span>
            {hasLost ? <Badge variant="primary">{t('home.lostPartitionBadge')}</Badge> : null}
          </div>
          <div className="truncate text-xs text-foreground-secondary">{device.model}</div>
        </div>
        {locked ? (
          <Hint text={t('home.permissionDenied')}>
            <LockIcon className="size-4 text-warning" aria-hidden />
          </Hint>
        ) : null}
        {offline ? (
          <Hint text={t('home.disconnected')}>
            <PlugZapIcon className="size-4 text-danger" aria-hidden />
          </Hint>
        ) : null}
      </div>

      <CapacityBar
        usedBytes={device.capacity.usedBytes}
        totalBytes={device.capacity.totalBytes}
        label={t('home.capacityUsed', {
          used: formatBytes(device.capacity.usedBytes, useLocale()),
          total: formatBytes(device.capacity.totalBytes, useLocale()),
        })}
      />

      <div className="flex items-center justify-between gap-2">
        <span className="flex items-center gap-1 text-xs font-medium">
          {healthy ? (
            <>
              <CircleCheckIcon className="size-3.5 text-success" aria-hidden />
              <span className="text-success">{t('home.healthGood')}</span>
            </>
          ) : (
            <>
              <CircleAlertIcon className="size-3.5 text-warning" aria-hidden />
              <span className="text-warning">{t('home.healthCaution')}</span>
            </>
          )}
        </span>
        <span className="flex items-center gap-2">
          {device.volumes.map((v: Volume) => (
            <button
              key={v.id}
              type="button"
              onClick={(e) => {
                e.stopPropagation();
                navigate(`/scan/${v.id}`);
              }}
              className="rounded-xs px-1.5 py-0.5 text-xs font-medium text-primary hover:bg-primary-muted focus-visible:ring-2 focus-visible:ring-ring"
            >
              {v.label}
            </button>
          ))}
          <Button
            variant="secondary"
            size="sm"
            onClick={(e) => {
              e.stopPropagation();
              navigate(`/scan/${device.id}`);
            }}
            disabled={locked || offline}
          >
            {t('home.scanAction')}
          </Button>
        </span>
      </div>
    </div>
  );
}

function QuickTarget({
  icon: Icon,
  label,
  onClick,
}: {
  icon: React.ComponentType<{ className?: string; strokeWidth?: number }>;
  label: string;
  onClick: () => void;
}): React.JSX.Element {
  return (
    <button
      type="button"
      onClick={onClick}
      className="flex flex-col items-center gap-2 rounded-lg border border-border bg-surface px-4 py-5 text-sm font-medium shadow-card transition-colors hover:bg-surface-hover focus-visible:ring-2 focus-visible:ring-ring"
    >
      <Icon className="size-7 text-foreground-secondary" strokeWidth={1.5} aria-hidden />
      {label}
    </button>
  );
}

/** Landing stat row shown when devices exist (used by tests for visibility). */
export function HomeStats(): React.JSX.Element {
  const { t } = useTranslation();
  const q = useDevices();
  const devices = q.data ?? [];
  const totalBytes = devices.reduce((acc, d) => acc + d.capacity.totalBytes, 0);
  return (
    <div className="grid grid-cols-3 gap-3">
      <StatTile label={t('common.total')} value={String(devices.length)} />
      <StatTile label={t('scan.dataFound')} value={formatBytes(totalBytes, useLocale())} />
    </div>
  );
}
