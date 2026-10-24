import * as React from 'react';
import { useTranslation } from 'react-i18next';
import {
  ThermometerIcon,
  HeartPulseIcon,
  TimerIcon,
  ZapIcon,
  HardDriveIcon,
} from 'lucide-react';
import { useHealth } from '@/queries/health';
import { usePhysicalDevices } from '@/hooks/use-physical-devices';
import { formatInteger } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Badge } from '@/components/ui/badge';
import { EmptyState, StatTile } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from '@/components/ui/table';

/** Disk Health — SMART table + summary card, exactly the fields the connector returns. */
export function HealthPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const {
    devices: list,
    selectedId: deviceId,
    setSelectedId,
    isLoading,
    refetch,
  } = usePhysicalDevices();

  const health = useHealth(deviceId || null);

  if (isLoading) {
    return (
      <div className="p-6">
        <Skeleton className="mb-4 h-32" />
        <Skeleton className="h-64" />
      </div>
    );
  }

  if (list.length === 0) {
    return (
      <div className="p-6">
        <EmptyState
          icon={HardDriveIcon}
          title={t('home.emptyTitle')}
          description={t('home.emptyBody')}
          actionLabel={t('home.refreshDevices')}
          onAction={refetch}
        />
      </div>
    );
  }

  return (
    <div className="mx-auto w-full max-w-4xl overflow-y-auto p-6">
      <h1 className="mb-1 text-xl font-semibold">{t('health.title')}</h1>
      <p className="mb-4 text-sm text-foreground-secondary">{t('health.pickDisk')}</p>

      <div className="mb-6 flex flex-wrap gap-2">
        {list.map((d) => (
          <button
            key={d.id}
            type="button"
            aria-pressed={deviceId === d.id}
            onClick={() => setSelectedId(d.id)}
            className={cn(
              'rounded-md border-2 px-4 py-2 text-left text-sm transition-colors',
              deviceId === d.id
                ? 'border-primary bg-primary-muted'
                : 'border-border bg-surface hover:border-border-strong',
            )}
          >
            <div className="font-medium">{d.label}</div>
            <div className="text-xs text-foreground-secondary">{d.model}</div>
          </button>
        ))}
      </div>

      {health.isLoading ? (
        <>
          <Skeleton className="mb-4 h-24" />
          <Skeleton className="h-64" />
        </>
      ) : health.isError ? (
        <EmptyState
          icon={HeartPulseIcon}
          title={t('health.notAvailable')}
          description={health.error instanceof Error ? health.error.message : undefined}
          actionLabel={t('common.retry')}
          onAction={() => void health.refetch()}
        />
      ) : health.data ? (
        <>
          <div
            className={cn(
              'mb-5 flex items-center gap-4 rounded-lg border p-4',
              health.data.status === 'good'
                ? 'border-success/40 bg-success-muted'
                : health.data.status === 'caution'
                  ? 'border-warning/40 bg-warning-muted'
                  : 'border-danger/40 bg-danger-muted',
            )}
          >
            <HeartPulseIcon
              className={cn(
                'size-8',
                health.data.status === 'good'
                  ? 'text-success'
                  : health.data.status === 'caution'
                    ? 'text-warning'
                    : 'text-danger',
              )}
              aria-hidden
            />
            <div>
              <div className="text-lg font-semibold">
                {health.data.status === 'good'
                  ? t('health.statusGood')
                  : health.data.status === 'caution'
                    ? t('health.statusCaution')
                    : t('health.statusBad')}
              </div>
              {health.data.status !== 'good' ? (
                <p className="text-sm text-foreground-secondary">{t('health.cautionNote')}</p>
              ) : null}
            </div>
          </div>

          <div className="mb-5 grid grid-cols-2 gap-3 md:grid-cols-4">
            <StatTile
              label={t('health.temperature')}
              value={
                health.data.temperatureC != null
                  ? `${formatInteger(health.data.temperatureC, i18n.language)} °C`
                  : '—'
              }
              icon={ThermometerIcon}
              tone={
                health.data.temperatureC != null && health.data.temperatureC > 55
                  ? 'warning'
                  : 'default'
              }
            />
            <StatTile
              label={t('health.powerOnHours')}
              value={
                health.data.powerOnHours != null
                  ? formatInteger(health.data.powerOnHours, i18n.language)
                  : '—'
              }
              icon={TimerIcon}
            />
            <StatTile
              label={t('health.class')}
              value={
                health.data.diskClass === 'ssd'
                  ? t('health.ssd')
                  : health.data.diskClass === 'hdd'
                    ? t('health.hdd')
                    : '—'
              }
            />
            {health.data.diskClass === 'ssd' ? (
              <StatTile
                label={t('health.trim')}
                value={
                  health.data.trimEnabled == null
                    ? '—'
                    : health.data.trimEnabled
                      ? t('health.trimEnabled')
                      : t('health.trimDisabled')
                }
                icon={ZapIcon}
                tone={health.data.trimEnabled === false ? 'warning' : 'default'}
              />
            ) : (
              <StatTile
                label={t('health.wear')}
                value={
                  health.data.wearPercent != null
                    ? formatInteger(health.data.wearPercent, i18n.language)
                    : '—'
                }
              />
            )}
          </div>

          <h2 className="mb-2 text-base font-semibold">{t('health.smartTable')}</h2>
          <div className="overflow-hidden rounded-lg border border-border bg-surface shadow-card">
            <Table>
              <TableHeader>
                <TableRow>
                  <TableHead className="w-14">{t('health.smartCol.id')}</TableHead>
                  <TableHead>{t('health.smartCol.attribute')}</TableHead>
                  <TableHead className="w-20 text-right">
                    {t('health.smartCol.current')}
                  </TableHead>
                  <TableHead className="w-20 text-right">
                    {t('health.smartCol.worst')}
                  </TableHead>
                  <TableHead className="w-20 text-right">
                    {t('health.smartCol.threshold')}
                  </TableHead>
                  <TableHead className="w-24 text-right">{t('health.smartCol.raw')}</TableHead>
                  <TableHead className="w-24">{t('health.smartCol.status')}</TableHead>
                </TableRow>
              </TableHeader>
              <TableBody>
                {health.data.attributes.map((a) => (
                  <TableRow key={a.id}>
                    <TableCell className="text-foreground-secondary tabular">{a.id}</TableCell>
                    <TableCell className="font-medium">{a.name}</TableCell>
                    <TableCell className="text-right tabular">{a.current ?? '—'}</TableCell>
                    <TableCell className="text-right tabular">{a.worst ?? '—'}</TableCell>
                    <TableCell className="text-right tabular">{a.threshold ?? '—'}</TableCell>
                    <TableCell className="text-right tabular">
                      {formatInteger(a.raw, i18n.language)}
                    </TableCell>
                    <TableCell>
                      <Badge
                        variant={
                          a.status === 'ok'
                            ? 'success'
                            : a.status === 'failing'
                              ? 'danger'
                              : 'neutral'
                        }
                      >
                        {a.status === 'ok'
                          ? t('health.smartOk')
                          : a.status === 'failing'
                            ? t('health.smartFailing')
                            : '—'}
                      </Badge>
                    </TableCell>
                  </TableRow>
                ))}
              </TableBody>
            </Table>
          </div>
        </>
      ) : null}
    </div>
  );
}
