import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { CircleCheckIcon, ShieldAlertIcon, Undo2Icon, WrenchIcon } from 'lucide-react';
import { useApplyRepair, useRepairDryRun, useUndoLog, useUndoRepairs } from '@/queries/repair';
import { usePhysicalDevices } from '@/hooks/use-physical-devices';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Alert } from '@/components/ui/alert';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * File Repair page — the TRUTHFUL posture (M31 / NG-12, W8): the canned
 * fake queue is GONE. Per-file content repair (the og's EUFileRepair
 * family — corrupted videos/photos/documents) is a separate product line
 * (M09 / S-050 / D-137, the API-DELTA register); this build does not
 * ship it, and the page says so plainly instead of fabricating a queue.
 *
 * The REAL repair surface this page carries: the volume repair workbench
 * over the connector's typed repair methods (dryRun/apply/undoLog/undo —
 * the same transactional semantics as the in-scan pre-scan repair chain,
 * S-008 consent + the undo log). On the real backend the repair methods
 * throw the typed not-implemented refusal (the W7 ABI trio is the
 * recorded follow-up) — the workbench renders that refusal honestly.
 */
export function RepairPage(): React.JSX.Element {
  const { t } = useTranslation();
  const {
    devices: list,
    selectedId: deviceId,
    setSelectedId,
    isLoading,
    refetch,
  } = usePhysicalDevices();

  const dryRun = useRepairDryRun(deviceId || null);
  const apply = useApplyRepair(deviceId);
  const undoLog = useUndoLog(deviceId);
  const undo = useUndoRepairs(deviceId);

  const [consentChecked, setConsentChecked] = React.useState(false);
  const [consentText, setConsentText] = React.useState('');

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto px-8 py-6">
      {/* Header */}
      <div>
        <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
          {t('repair.title')}
        </h1>
        <p className="mt-1 text-[13px] text-[#64748B] dark:text-foreground-secondary">
          {t('repair.introBody')}
        </p>
      </div>

      {/* The per-file truth (M31): no fabricated queue, no dead dropzone —
          the capability this page's name promises is a separate product
          line, and the page states its absence. */}
      <Alert variant="info" className="mt-4">
        <p className="text-[13px] leading-relaxed">{t('repair.perFileNote')}</p>
      </Alert>

      {/* The REAL surface: the volume repair workbench (always primary). */}
      <div className="mt-5">
        <VolumeRepairWorkbench
          list={list}
          deviceId={deviceId}
          setSelectedId={setSelectedId}
          isLoading={isLoading}
          refetch={refetch}
          dryRun={dryRun}
          apply={apply}
          undoLog={undoLog}
          undo={undo}
          consentChecked={consentChecked}
          setConsentChecked={setConsentChecked}
          consentText={consentText}
          setConsentText={setConsentText}
        />
      </div>
    </div>
  );
}

function VolumeRepairWorkbench({
  list,
  deviceId,
  setSelectedId,
  isLoading,
  refetch,
  dryRun,
  apply,
  undoLog,
  undo,
  consentChecked,
  setConsentChecked,
  consentText,
  setConsentText,
}: {
  list: ReturnType<typeof usePhysicalDevices>['devices'];
  deviceId: string;
  setSelectedId: (id: string) => void;
  isLoading: boolean;
  refetch: () => void;
  dryRun: ReturnType<typeof useRepairDryRun>;
  apply: ReturnType<typeof useApplyRepair>;
  undoLog: ReturnType<typeof useUndoLog>;
  undo: ReturnType<typeof useUndoRepairs>;
  consentChecked: boolean;
  setConsentChecked: (v: boolean) => void;
  consentText: string;
  setConsentText: (v: string) => void;
}): React.JSX.Element {
  const { t } = useTranslation();

  return (
    <div className="rounded-[10px] border border-[#DCE8FF] bg-white p-5 dark:border-border dark:bg-surface">
      {isLoading ? (
        <div className="space-y-3">
          <Skeleton className="h-16" />
          <Skeleton className="h-36" />
        </div>
      ) : list.length === 0 ? (
        <EmptyState
          icon={WrenchIcon}
          title={t('home.emptyTitle')}
          description={t('home.emptyBody')}
          actionLabel={t('home.refreshDevices')}
          onAction={refetch}
        />
      ) : (
        <>
          <div className="mb-4 flex flex-wrap gap-2">
            {list.map((d) => (
              <button
                key={d.id}
                type="button"
                aria-pressed={deviceId === d.id}
                onClick={() => {
                  setSelectedId(d.id);
                  setConsentChecked(false);
                  setConsentText('');
                }}
                className={
                  deviceId === d.id
                    ? 'rounded-md border-2 border-[#1677FF] bg-[#EBF3FF] px-4 py-2 text-left text-sm dark:border-primary dark:bg-surface-selected'
                    : 'rounded-md border-2 border-[#E2E8F0] bg-white px-4 py-2 text-left text-sm hover:border-[#93C5FD] hover:bg-[#F4F8FF] dark:border-border dark:bg-surface-elevated'
                }
              >
                <div className="font-medium text-[#1E293B] dark:text-foreground">{d.label}</div>
                <div className="text-xs text-[#64748B] dark:text-foreground-secondary">
                  {d.model}
                </div>
              </button>
            ))}
          </div>

          {dryRun.isLoading ? (
            <Skeleton className="h-40" />
          ) : dryRun.isError ? (
            <Alert variant="danger">
              <p>
                {dryRun.error instanceof Error ? dryRun.error.message : t('errors.unknown')}
              </p>
            </Alert>
          ) : dryRun.data && dryRun.data.findings.length > 0 ? (
            <div className="space-y-4">
              <h2 className="text-base font-semibold">{t('repair.findings')}</h2>
              <ul className="space-y-2">
                {dryRun.data.findings.map((f) => (
                  <li
                    key={f.id}
                    className="flex items-start gap-3 rounded-md border border-[#E2E8F0] bg-white p-3 dark:border-border dark:bg-surface-elevated"
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
                      <div className="font-mono text-xs text-[#64748B] dark:text-foreground-secondary">
                        {f.where}
                      </div>
                    </div>
                  </li>
                ))}
              </ul>

              <div className="rounded-lg border-2 border-warning/50 bg-warning-muted/40 p-4">
                <h3 className="flex items-center gap-2 text-base font-semibold">
                  <ShieldAlertIcon className="size-4 text-warning" aria-hidden />
                  {t('repair.consentTitle')}
                </h3>
                <p className="mt-1.5 text-sm text-[#64748B] dark:text-foreground-secondary">
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
                  <span className="text-xs text-[#64748B] dark:text-foreground-secondary">
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
                          toast.success(t('repair.appliedCount', { count: applied })),
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

          <div className="mt-6">
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
                        toast.success(t('repair.rolledBackCount', { count: rolledBack })),
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
                    className="flex items-center justify-between gap-3 rounded-md border border-[#E2E8F0] bg-white px-4 py-2 text-sm dark:border-border dark:bg-surface-elevated"
                  >
                    <span className="min-w-0 truncate">
                      {entry.index}. {entry.action} —{' '}
                      <span className="font-mono text-xs text-[#64748B] dark:text-foreground-secondary">
                        {entry.where}
                      </span>
                    </span>
                    <Badge variant="outline">{entry.snapshotBytes} B</Badge>
                  </li>
                ))}
              </ol>
            ) : (
              <p className="text-sm text-[#64748B] dark:text-foreground-secondary">
                {t('repair.nothingToUndo')}
              </p>
            )}
          </div>
        </>
      )}
    </div>
  );
}
