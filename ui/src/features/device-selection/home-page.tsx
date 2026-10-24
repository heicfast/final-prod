import * as React from 'react';
import { useNavigate, useSearchParams } from 'react-router';
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
import { useStartScan } from '@/queries/scan';
import { usePickDirectory } from '@/queries/system';
import { formatBytes, formatDate } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { CapacityBar, EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { Hint } from '@/components/ui/tooltip';
import { toast } from 'sonner';

/**
 * Home / Device Selection — the corpus landing page (atlas §5): page title,
 * device tile grid with capacity bars + health hints + lost-partition
 * badges, the "Specify a location" quick-target row (Recycle Bin / Desktop /
 * Select Folder — all REAL scan entries, GT-landing §4), recent sessions,
 * and the "Search for Lost Data" CTA bottom-right.
 *
 * ADR-008: clicking the CTA (or any scan entry) starts the scan DIRECTLY —
 * the app navigates to the live scan surface with no intermediate
 * configuration step.
 */

const RECYCLE_BIN_TARGET = 'vol-recycle-bin';
const DESKTOP_TARGET = 'vol-desktop';

export function HomePage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const [params] = useSearchParams();
  const devicesQuery = useDevices();
  const refresh = useRefreshDevices();
  const sessionsQuery = useSessions();
  const loadSession = useLoadSession();
  const startScan = useStartScan();
  const pickDirectory = usePickDirectory();
  const [selectedId, setSelectedId] = React.useState<string | null>(null);
  const showRemovable = params.has('media');

  const devices = devicesQuery.data ?? [];
  const physical = devices.filter((d) => d.kind !== 'virtual');
  const internal = physical.filter((d) => d.kind === 'internal');
  const removable = physical.filter((d) => d.kind !== 'internal');
  const selected = devices.find((d) => d.id === selectedId) ?? null;

  /** Direct start (ADR-008): device selection → scan, no config step. */
  const beginScan = (targetId: string, pathScope?: string): void => {
    startScan.mutate(
      { targetId, ...(pathScope ? { pathScope } : {}) },
      {
        onSuccess: ({ scanId }) => {
          void navigate(`/scan/${targetId}/live?scanId=${scanId}`);
        },
        onError: (err) => {
          const reasonKey = (err as { reasonKey?: string }).reasonKey;
          toast.error(reasonKey ? t(reasonKey) : err.message);
        },
      },
    );
  };

  const onChooseFolder = (): void => {
    // "Specify a location" → Select Folder (atlas §5): the native folder
    // picker runs FIRST, then the scan starts directly on the chosen scope.
    const hintVolume = devices.flatMap((d) => d.volumes)[0];
    pickDirectory.mutate(
      { volumeId: hintVolume?.id ?? null },
      {
        onSuccess: (picked) => {
          if (!picked) return; // cancelled — stay on Home
          beginScan(hintVolume?.id ?? '', picked);
        },
        onError: (err) => toast.error(err.message),
      },
    );
  };

  const onResume = (path: string): void => {
    void loadSession.mutate(path, {
      onSuccess: ({ scanId }) => {
        toast.success(t('home.resume'));
        void navigate(`/results/${scanId}`);
      },
      onError: (err) => {
        toast.error(
          err instanceof Error
            ? (err as { reasonKey?: string }).reasonKey
              ? t((err as { reasonKey?: string }).reasonKey!)
              : err.message
            : t('errors.sessionCorrupt'),
        );
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
          description={
            devicesQuery.error instanceof Error
              ? devicesQuery.error.message
              : t('errors.unknown')
          }
          actionLabel={t('common.retry')}
          onAction={() => void devicesQuery.refetch()}
          secondaryLabel={t('home.refreshDevices')}
          onSecondary={() => void refresh.mutate()}
        />
      </div>
    );
  }

  const visible = showRemovable ? removable : internal;
  if (physical.length === 0) {
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

  const hasPermissionIssue = physical.some((d) => d.status === 'permission-denied');
  const hasDisconnected = physical.some((d) => d.status === 'disconnected');

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto">
      <div className="mx-auto w-full max-w-5xl flex-1 p-6">
        <div className="mb-6 flex items-end justify-between gap-4">
          <div>
            <h1 className="text-xl font-semibold">{t('home.title')}</h1>
            <p className="mt-1 text-sm text-foreground-secondary">{t('home.subtitle')}</p>
          </div>
          <Button
            variant="ghost"
            size="sm"
            onClick={() => void refresh.mutate()}
            loading={refresh.isPending}
            disabled={refresh.isPending}
          >
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
          {showRemovable ? t('home.removableGroup') : t('home.internalGroup')} ({visible.length}
          )
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
              onScan={() => beginScan(device.id)}
              onScanVolume={(volumeId) => beginScan(volumeId)}
            />
          ))}
        </div>

        <h2 className="mb-3 mt-8 text-sm font-semibold text-foreground-secondary">
          {t('home.quickTargets')}
        </h2>
        <div className="grid grid-cols-3 gap-4">
          <QuickTarget
            icon={Trash2Icon}
            label={t('home.recycleBin')}
            onClick={() => beginScan(RECYCLE_BIN_TARGET)}
            loading={startScan.isPending}
          />
          <QuickTarget
            icon={MonitorSmartphoneIcon}
            label={t('home.desktop')}
            onClick={() => beginScan(DESKTOP_TARGET)}
            loading={startScan.isPending}
          />
          <QuickTarget
            icon={FolderOpenIcon}
            label={t('home.chooseFolder')}
            onClick={onChooseFolder}
            loading={pickDirectory.isPending || startScan.isPending}
          />
        </div>

        {(sessionsQuery.data?.length ?? 0) > 0 ? (
          <>
            <h2 className="mb-3 mt-8 text-sm font-semibold text-foreground-secondary">
              {t('home.recentSessions')}
            </h2>
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
                      {formatDate(s.savedAt, i18n.language)} ·{' '}
                      {s.filesFound.toLocaleString(i18n.language)} {t('scan.filesFound')}
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

      {/* Bottom CTA bar — "Search for Lost Data" (atlas §5: bottom-right) */}
      <div className="sticky bottom-0 flex items-center justify-end gap-3 border-t border-border bg-surface/90 px-6 py-3 backdrop-blur">
        <span className="text-sm text-foreground-secondary">
          {selected
            ? `${selected.label} · ${formatBytes(selected.capacity.totalBytes, i18n.language)}`
            : t('common.empty')}
        </span>
        <Button
          variant="primary"
          size="lg"
          disabled={!selected || startScan.isPending}
          loading={startScan.isPending}
          onClick={() => selected && beginScan(selected.id)}
          aria-keyshortcuts="Enter"
        >
          <SearchIcon aria-hidden />
          {t('home.scanAction')}
        </Button>
      </div>
    </div>
  );
}

function DeviceCard({
  device,
  selected,
  onSelect,
  onScan,
  onScanVolume,
}: {
  device: Device;
  selected: boolean;
  onSelect: () => void;
  onScan: () => void;
  onScanVolume: (volumeId: string) => void;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const Icon =
    device.kind === 'sd' ? MemoryStickIcon : device.kind === 'usb' ? UsbIcon : HardDriveIcon;
  const healthy = device.status === 'ok';
  const locked = device.status === 'permission-denied';
  const offline = device.status === 'disconnected';
  const hasLost = device.volumes.some((v) => v.lostPartition);

  return (
    <div
      className={cn(
        'group flex flex-col gap-3 rounded-lg border-2 bg-surface p-4 shadow-card transition-all duration-150',
        'hover:shadow-overlay',
        selected ? 'border-primary' : 'border-transparent hover:border-border-strong',
        offline && 'opacity-60',
      )}
    >
      {/* Selectable card body — the ONLY interactive element carrying radio
          semantics. The actions row below is a sibling, never a descendant
          (axe nested-interactive: interactive controls must not nest). */}
      <button
        type="button"
        role="radio"
        aria-checked={selected}
        onClick={onSelect}
        className="flex flex-col gap-3 text-left outline-none focus-visible:ring-2 focus-visible:ring-ring"
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
            used: formatBytes(device.capacity.usedBytes, i18n.language),
            total: formatBytes(device.capacity.totalBytes, i18n.language),
          })}
        />
      </button>

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
              onClick={() => onScanVolume(v.id)}
              className="rounded-xs px-1.5 py-0.5 text-xs font-medium text-primary hover:bg-primary-muted focus-visible:ring-2 focus-visible:ring-ring"
            >
              {v.label}
            </button>
          ))}
          <Button
            variant="secondary"
            size="sm"
            onClick={() => onScan()}
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
  loading,
}: {
  icon: React.ComponentType<{ className?: string; strokeWidth?: number }>;
  label: string;
  onClick: () => void;
  loading?: boolean;
}): React.JSX.Element {
  return (
    <button
      type="button"
      onClick={onClick}
      disabled={loading}
      className="flex flex-col items-center gap-2 rounded-lg border border-border bg-surface px-4 py-5 text-sm font-medium shadow-card transition-colors hover:bg-surface-hover focus-visible:ring-2 focus-visible:ring-ring disabled:opacity-60"
    >
      <Icon className="size-7 text-foreground-secondary" strokeWidth={1.5} aria-hidden />
      {label}
    </button>
  );
}
