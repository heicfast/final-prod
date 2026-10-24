import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { useSearchParams } from 'react-router';
import { CircleCheckIcon, ShieldAlertIcon, Undo2Icon, WrenchIcon } from 'lucide-react';
import { useDevices } from '@/queries/devices';
import { useApplyRepair, useRepairDryRun, useUndoLog, useUndoRepairs } from '@/queries/repair';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Alert } from '@/components/ui/alert';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * Volume Repair (RawFixer surface) — dry-run findings, typed consent gate,
 * apply, undo log. This is a write path: maximum gravity, no dark patterns.
 */
export function RepairPage(): React.JSX.Element {
  const { t } = useTranslation();
  const [params] = useSearchParams();
  const devices = useDevices();
  const list = devices.data ?? [];
  const [pickedDeviceId, setPickedDeviceId] = React.useState<string | null>(
    params.get('device'),
  );
  const deviceId: string | null = pickedDeviceId ?? list[0]?.id ?? null;

  const dryRun = useRepairDryRun(deviceId);
  const apply = useApplyRepair(deviceId);
  const undoLog = useUndoLog(deviceId);
  const undo = useUndoRepairs(deviceId);

  const [consentChecked, setConsentChecked] = React.useState(false);
  const [consentText, setConsentText] = React.useState('');

  if (devices.isLoading) {
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
          icon={WrenchIcon}
          title={t('home.emptyTitle')}
          description={t('home.emptyBody')}
          actionLabel={t('home.refreshDevices')}
          onAction={() => void devices.refetch()}
        />
      </div>
    );
  }

  return (
    <div className="mx-auto w-full max-w-3xl overflow-y-auto p-6">
      <div className="mb-6">
        <h1 className="text-xl font-semibold">{t('repair.title')}</h1>
        <p className="mt-0.5 text-sm text-foreground-secondary">{t('repair.introBody')}</p>
      </div>

      <div className="mb-5 flex flex-wrap gap-2">
        {list.map((d) => (
          <button
            key={d.id}
            type="button"
            aria-pressed={deviceId === d.id}
            onClick={() => {
              setPickedDeviceId(d.id);
              setConsentChecked(false);
              setConsentText('');
            }}
            className={
              deviceId === d.id
                ? 'rounded-md border-2 border-primary bg-primary-muted px-4 py-2 text-left text-sm'
                : 'rounded-md border-2 border-border bg-surface px-4 py-2 text-left text-sm hover:border-border-strong'
            }
          >
            <div className="font-medium">{d.label}</div>
            <div className="text-xs text-foreground-secondary">{d.model}</div>
          </button>
        ))}
      </div>

      {dryRun.isLoading ? (
        <Skeleton className="h-40" />
      ) : dryRun.isError ? (
        <Alert variant="danger">
          <p>{dryRun.error instanceof Error ? dryRun.error.message : t('errors.unknown')}</p>
        </Alert>
      ) : dryRun.data && dryRun.data.findings.length > 0 ? (
        <div className="space-y-4">
          <h2 className="text-base font-semibold">{t('repair.findings')}</h2>
          <ul className="space-y-2">
            {dryRun.data.findings.map((f) => (
              <li
                key={f.id}
                className="flex items-start gap-3 rounded-md border border-border bg-surface p-3"
              >
                <Badge
                  variant={
                    f.severity === 'critical'
                      ? 'danger'
                      : f.severity === 'warning'
                        ? 'warning'
                        : 'neutral'
                  }
                >
                  {f.severity}
                </Badge>
                <div className="min-w-0 flex-1">
                  <div className="text-sm font-medium">{f.kind}</div>
                  <div className="text-xs text-foreground-secondary font-mono">{f.where}</div>
                </div>
              </li>
            ))}
          </ul>

          {/* Consent gate */}
          <div className="rounded-lg border-2 border-warning/50 bg-warning-muted/40 p-4">
            <h3 className="flex items-center gap-2 text-base font-semibold">
              <ShieldAlertIcon className="size-4 text-warning" aria-hidden />
              {t('repair.consentTitle')}
            </h3>
            <p className="mt-1.5 text-sm text-foreground-secondary">
              {t('repair.consentBody')}
            </p>
            <label className="mt-3 flex cursor-pointer items-start gap-2.5 text-sm">
              <input
                type="checkbox"
                className="mt-0.5 size-4 accent-[var(--warning)]"
                checked={consentChecked}
                onChange={(e) => setConsentChecked(e.target.checked)}
              />
              {t('repair.consentAcknowledge')}
            </label>
            <div className="mt-3 flex items-center gap-2">
              <span className="text-xs text-foreground-secondary">
                {t('repair.consentTypeHere')}:
              </span>
              <Input
                className="w-32 font-mono"
                value={consentText}
                onChange={(e) => setConsentText(e.target.value)}
                aria-label={t('repair.consentTypeHere')}
              />
              <Button
                variant="danger"
                disabled={
                  !consentChecked ||
                  consentText.trim().toUpperCase() !== 'REPAIR' ||
                  apply.isPending
                }
                loading={apply.isPending}
                onClick={() => {
                  apply.mutate(consentText.trim().toUpperCase(), {
                    onSuccess: ({ applied }) =>
                      toast.success(t('repair.applied') + ` (${applied})`),
                    onError: (err) => toast.error(err.message),
                  });
                }}
              >
                <WrenchIcon aria-hidden />
                {t('repair.fix', { count: dryRun.data.findings.length })}
              </Button>
            </div>
          </div>
        </div>
      ) : (
        <EmptyState icon={CircleCheckIcon} title={t('repair.noFindings')} />
      )}

      {/* Undo log */}
      <div className="mt-8">
        <div className="mb-2 flex items-center justify-between">
          <h2 className="text-base font-semibold">{t('repair.undoLog')}</h2>
          {(undoLog.data?.length ?? 0) > 0 ? (
            <Button
              variant="secondary"
              size="sm"
              loading={undo.isPending}
              onClick={() =>
                undo.mutate(undefined, {
                  onSuccess: ({ rolledBack }) =>
                    toast.success(t('repair.rolledBack') + ` (${rolledBack})`),
                })
              }
            >
              <Undo2Icon aria-hidden />
              {t('repair.rollback')}
            </Button>
          ) : null}
        </div>
        {(undoLog.data?.length ?? 0) > 0 ? (
          <ol className="space-y-1.5">
            {undoLog.data?.map((entry) => (
              <li
                key={entry.index}
                className="flex items-center justify-between gap-3 rounded-md border border-border bg-surface px-4 py-2 text-sm"
              >
                <span className="min-w-0 truncate">
                  {entry.index}. {entry.action} —{' '}
                  <span className="font-mono text-xs text-foreground-secondary">
                    {entry.where}
                  </span>
                </span>
                <Badge variant="outline">{entry.snapshotBytes} B</Badge>
              </li>
            ))}
          </ol>
        ) : (
          <p className="text-sm text-foreground-secondary">{t('repair.nothingToUndo')}</p>
        )}
      </div>
    </div>
  );
}
