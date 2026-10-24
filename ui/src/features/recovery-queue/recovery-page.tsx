import * as React from 'react';
import { useNavigate, useParams, useSearchParams } from 'react-router';
import { useTranslation } from 'react-i18next';
import type { CollisionPolicy, RecoveryItem, RecoveryOptions } from '@/connector/contract';
import {
  ArrowLeftIcon,
  CheckCircle2Icon,
  CircleAlertIcon,
  DownloadIcon,
  FileCheck2Icon,
  HardDriveIcon,
  ListChecksIcon,
  ShieldAlertIcon,
  WrenchIcon,
  XCircleIcon,
} from 'lucide-react';
import { useMountPoints, usePickDirectory, useValidateDestination } from '@/queries/system';
import {
  useCancelRecovery,
  useRecoveryItems,
  useRecoveryJob,
  useRecoveryProgress,
  useRecoveryReport,
  useStartRecovery,
} from '@/queries/recovery';
import { useScanState } from '@/queries/scan';
import { resolveFilter } from '@/stores/filter-store';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { useSelectionStore } from '@/stores/selection-store';
import { formatBytes, formatDuration, formatPercent } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { Progress } from '@/components/ui/progress';
import { Switch } from '@/components/ui/switch';
import { RadioGroup, RadioGroupItem } from '@/components/ui/radio-group';
import { Badge } from '@/components/ui/badge';
import { Alert } from '@/components/ui/alert';
import { CapacityBar, StatTile } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * Recovery surface — destination choice with hard overlap refusal (connector
 * validateDestination — the UI cannot override), the recovery queue with
 * per-item states, progress, and the exportable report.
 */
export function RecoveryPage(): React.JSX.Element {
  const { scanId = '' } = useParams();
  const [params] = useSearchParams();
  const navigate = useNavigate();
  const { t, i18n } = useTranslation();

  const selection = useSelectionStore();
  // Route markers (?all=1 / ?ids=1) carry intent; the selection itself
  // lives in the store (all-mode = full filtered set + exclusions).
  const recoverAll = params.has('all') || selection.mode === 'all';
  const nodeIds = React.useMemo(
    () => (recoverAll ? [] : Array.from(selection.ids)),
    [recoverAll, selection.ids],
  );
  const selectedCount = recoverAll
    ? Math.max(0, selection.allCount - selection.exclusions.size)
    : selection.ids.size;

  // Sizing for the destination space check: average file size from the
  // scan's own totals (honest engine-derived estimate, not a magic number).
  const scanState = useScanState(scanId);
  const avgFileBytes =
    scanState.data && scanState.data.progress.filesFound > 0
      ? scanState.data.progress.dataFoundBytes / scanState.data.progress.filesFound
      : 2_400_000;
  const neededBytes = Math.max(
    1,
    Math.round((recoverAll ? selection.allCount : selectedCount) * avgFileBytes),
  );

  const mounts = useMountPoints();
  const startRecovery = useStartRecovery(scanId);
  // Source-device id for overlap refusal + picker exclusion: the session
  // store recorded the scan target when the search was started (audit 2.6 —
  // the scan id was previously passed here, making the refusal unreachable).
  const sourceTargetId = useScanSessionStore((s) =>
    scanId ? s.known[scanId]?.targetId : null,
  );
  const validateDestination = useValidateDestination(sourceTargetId);
  const pickDirectory = usePickDirectory();

  const [destination, setDestination] = React.useState('');
  const [validation, setValidation] = React.useState<{
    allowed: boolean;
    reason?: 'overlap' | 'unwritable' | 'invalid' | 'out-of-space';
    reasonKey?: string;
    freeBytes: number | null;
    neededBytes: number | null;
  } | null>(null);
  const [validating, setValidating] = React.useState(false);

  const [options, setOptions] = React.useState<RecoveryOptions>({
    collision: 'rename',
    rebuildFolders: true,
    keepEmptyFolders: false,
    fileRepair: true,
    verifyOnWrite: true,
  });

  const jobId = startRecovery.data?.jobId ?? null;
  const jobQuery = useRecoveryJob(jobId);

  const validate = (path: string): void => {
    if (!path) {
      setValidation(null);
      return;
    }
    setValidating(true);
    validateDestination.mutate(
      { path, neededBytes },
      {
        onSuccess: (v) => setValidation(v),
        onError: () =>
          setValidation({
            allowed: false,
            reason: 'invalid',
            freeBytes: null,
            neededBytes: null,
          }),
        onSettled: () => setValidating(false),
      },
    );
  };

  const jobStatus = jobQuery.data?.status ?? (jobId ? 'running' : null);

  return (
    <div className="flex h-full min-h-0 flex-col overflow-y-auto">
      <div className="mx-auto w-full max-w-3xl flex-1 p-6">
        <div className="mb-6 flex items-center gap-3">
          <Button
            variant="ghost"
            size="icon"
            onClick={() => navigate(-1)}
            aria-label={t('common.back')}
          >
            <ArrowLeftIcon aria-hidden />
          </Button>
          <div>
            <h1 className="text-xl font-semibold">{t('recovery.title')}</h1>
            <p className="mt-0.5 text-sm text-foreground-secondary tabular">
              {selectedCount > 0
                ? t('recovery.start', { count: selectedCount })
                : t('results.status.recoverAll')}
            </p>
          </div>
        </div>

        {jobId ? (
          <RecoveryQueue jobId={jobId} />
        ) : (
          <>
            {/* Destination */}
            <h2 className="mb-3 text-base font-semibold">{t('recovery.destination')}</h2>
            <p className="mb-3 text-sm text-foreground-secondary">
              {t('recovery.destinationHint')}
            </p>

            <div className="space-y-2">
              {mounts.isLoading
                ? Array.from({ length: 3 }, (_, i) => <Skeleton key={i} className="h-16" />)
                : mounts.data?.map((m) => (
                    <DestinationRow
                      key={m.id}
                      label={m.label}
                      path={m.path}
                      freeBytes={m.freeBytes}
                      totalBytes={m.totalBytes}
                      selected={destination === m.path}
                      warning={destination === m.path && validation?.reason === 'overlap'}
                      onSelect={() => {
                        setDestination(m.path);
                        validate(m.path);
                      }}
                    />
                  ))}
            </div>

            <div className="mt-3 grid grid-cols-[1fr_auto] items-end gap-2">
              <div className="space-y-1.5">
                <Label htmlFor="dest-path">{t('recovery.pathLabel')}</Label>
                <Input
                  id="dest-path"
                  value={destination}
                  placeholder="D:\\Recovered Files"
                  onChange={(e) => setDestination(e.target.value)}
                  onBlur={() => validate(destination)}
                  className="font-mono text-xs"
                />
              </div>
              <Button
                variant="secondary"
                onClick={() => {
                  pickDirectory.mutate(
                    { excludeDeviceId: sourceTargetId ?? null },
                    {
                      onSuccess: (picked) => {
                        if (picked) {
                          setDestination(picked);
                          validate(picked);
                        }
                      },
                    },
                  );
                }}
              >
                <HardDriveIcon aria-hidden />
                {t('recovery.chooseFolder')}
              </Button>
            </div>

            {validating ? (
              <p className="mt-2 text-xs text-foreground-secondary">
                {t('recovery.validating')}
              </p>
            ) : null}

            {validation && !validation.allowed ? (
              <Alert variant="danger" className="mt-3">
                <div className="space-y-1">
                  <p className="font-semibold">
                    {validation.reason === 'overlap'
                      ? t('recovery.overlapRefused')
                      : validation.reason === 'out-of-space'
                        ? t('recovery.insufficientSpace')
                        : t('errors.title')}
                  </p>
                  <p>
                    {validation.reason === 'overlap'
                      ? t('recovery.overlapReason')
                      : validation.reason === 'out-of-space' &&
                          validation.neededBytes &&
                          validation.freeBytes
                        ? t('recovery.insufficientSpaceBody', {
                            needed: formatBytes(
                              validation.neededBytes - validation.freeBytes,
                              i18n.language,
                            ),
                          })
                        : t('errors.unknown')}
                  </p>
                </div>
              </Alert>
            ) : null}

            {/* Options */}
            <h2 className="mb-3 mt-8 flex items-center gap-2 text-base font-semibold">
              <ListChecksIcon className="size-4" aria-hidden />
              {t('recovery.queue')}
            </h2>
            <div className="space-y-4 rounded-lg border border-border bg-surface p-4">
              <div className="space-y-1.5">
                <Label>{t('recovery.collisions')}</Label>
                <RadioGroup
                  value={options.collision}
                  onValueChange={(v) =>
                    setOptions((o) => ({ ...o, collision: v as CollisionPolicy }))
                  }
                  className="flex flex-wrap gap-4"
                >
                  {(['rename', 'overwrite', 'skip'] as const).map((c) => (
                    <label key={c} className="flex cursor-pointer items-center gap-2 text-sm">
                      <RadioGroupItem value={c} />
                      {t(`recovery.collision${c[0].toUpperCase()}${c.slice(1)}`)}
                    </label>
                  ))}
                </RadioGroup>
              </div>
              <div className="grid grid-cols-1 gap-3 md:grid-cols-2">
                <Toggle
                  checked={options.rebuildFolders}
                  onChange={(v) => setOptions((o) => ({ ...o, rebuildFolders: v }))}
                  label={t('recovery.folderReconstruction')}
                />
                <Toggle
                  checked={options.keepEmptyFolders}
                  onChange={(v) => setOptions((o) => ({ ...o, keepEmptyFolders: v }))}
                  label={t('recovery.keepEmptyFolders')}
                />
                <Toggle
                  checked={options.fileRepair}
                  onChange={(v) => setOptions((o) => ({ ...o, fileRepair: v }))}
                  label={t('recovery.fileRepair')}
                  icon={<WrenchIcon className="size-3.5" aria-hidden />}
                />
                <Toggle
                  checked={options.verifyOnWrite}
                  onChange={(v) => setOptions((o) => ({ ...o, verifyOnWrite: v }))}
                  label={t('recovery.verifyOn')}
                  icon={<FileCheck2Icon className="size-3.5" aria-hidden />}
                />
              </div>
              {options.verifyOnWrite ? (
                <p className="text-xs text-foreground-secondary">
                  {t('recovery.verifyingNote')}
                </p>
              ) : null}
            </div>
          </>
        )}
      </div>

      {/* Footer CTA */}
      <div className="sticky bottom-0 flex items-center justify-end gap-3 border-t border-border bg-surface/90 px-6 py-3 backdrop-blur">
        {jobId && jobStatus === 'running' ? <CancelRecoveryButton jobId={jobId} /> : null}
        {!jobId ? (
          <>
            <Button variant="secondary" onClick={() => navigate(-1)}>
              {t('common.back')}
            </Button>
            <Button
              variant="primary"
              size="lg"
              disabled={
                !destination || validating || (validation !== null && !validation.allowed)
              }
              title={
                !destination
                  ? t('recovery.destinationRequired')
                  : validation && !validation.allowed
                    ? t('recovery.overlapReason')
                    : undefined
              }
              onClick={() => {
                startRecovery.mutate(
                  {
                    selection: recoverAll
                      ? {
                          kind: 'all',
                          filter: selection.allFilter ?? resolveFilter(),
                        }
                      : { kind: 'nodes', nodeIds },
                    destination,
                    options,
                  },
                  {
                    onError: (err) => toast.error(err.message),
                  },
                );
              }}
            >
              <DownloadIcon aria-hidden />
              {selectedCount > 0
                ? t('recovery.start', { count: selectedCount })
                : t('results.status.recoverAll')}
            </Button>
          </>
        ) : null}
      </div>
    </div>
  );
}

function CancelRecoveryButton({ jobId }: { jobId: string }): React.JSX.Element {
  const { t } = useTranslation();
  const cancel = useCancelRecovery(jobId);
  return (
    <Button variant="dangerOutline" onClick={() => cancel.mutate()} loading={cancel.isPending}>
      {t('recovery.cancel')}
    </Button>
  );
}

function RecoveryQueue({ jobId }: { jobId: string }): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const items = useRecoveryItems(jobId);
  const [visibleCount, setVisibleCount] = React.useState(100);
  const all = items.data ?? [];
  return (
    <div className="space-y-5">
      <QueueProgress jobId={jobId} />
      <div className="space-y-1.5">
        <h2 className="text-base font-semibold">{t('recovery.queue')}</h2>
        <div className="max-h-72 overflow-y-auto rounded-lg border border-border">
          {all.slice(0, visibleCount).map((item) => (
            <div
              key={item.nodeId}
              className="flex items-center gap-3 border-b border-border px-4 py-2 text-sm last:border-0"
            >
              <span className="min-w-0 flex-1 truncate" title={item.name}>
                {item.name}
              </span>
              <span className="shrink-0 text-xs tabular text-foreground-secondary">
                {formatBytes(item.sizeBytes, i18n.language)}
              </span>
              <QueueStateBadge state={item.state} reasonKey={item.error?.reasonKey} />
            </div>
          ))}
          {all.length === 0 ? <Skeleton className="m-2 h-8" /> : null}
          {all.length > visibleCount ? (
            <button
              type="button"
              className="w-full py-2 text-xs text-primary hover:bg-surface-hover"
              onClick={() => setVisibleCount((c) => Math.min(c + 200, all.length))}
            >
              {t('recovery.showMoreItems', {
                shown: Math.min(visibleCount, all.length),
                total: all.length,
              })}
            </button>
          ) : null}
        </div>
      </div>
      <QueueReport jobId={jobId} />
    </div>
  );
}

function QueueProgress({ jobId }: { jobId: string }): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const progress = useRecoveryProgress(jobId);
  if (!progress) return <Skeleton className="h-20" />;
  const done = progress.status !== 'running' && progress.status !== 'validating';
  return (
    <div className="space-y-2 rounded-lg border border-border bg-surface p-4">
      <div className="flex items-center justify-between text-sm">
        <span className="font-medium">
          {done ? t('recovery.reportTitle') : t('recovery.progressOverall')}
        </span>
        <span className="tabular font-semibold text-primary">
          {formatPercent(progress.overallProgress, i18n.language)}
        </span>
      </div>
      <Progress
        value={progress.overallProgress * 100}
        aria-label={t('recovery.progressOverall')}
      />
      <div className="flex flex-wrap gap-x-4 gap-y-1 text-xs text-foreground-secondary tabular">
        {progress.currentFile ? (
          <span className="max-w-64 truncate">
            {t('recovery.progressFile')}: {progress.currentFile}
          </span>
        ) : null}
        <span>
          {progress.completedCount}/{progress.totalCount}
        </span>
        {progress.failedCount > 0 ? (
          <span className="text-danger">{progress.failedCount} ✕</span>
        ) : null}
        {progress.verifying ? (
          <span className="text-primary">{t('recovery.queueItem.verifying')}…</span>
        ) : null}
        {!done && progress.etaSeconds ? (
          <span>{formatDuration(progress.etaSeconds, i18n.language)}</span>
        ) : null}
      </div>
    </div>
  );
}

function QueueReport({ jobId }: { jobId: string }): React.JSX.Element | null {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const { scanId = '' } = useParams();
  const { data } = useRecoveryReport(jobId);
  if (!data || data.status === 'running' || data.status === 'validating') return null;
  const success = data.status === 'completed';
  const partial = data.status === 'completed-partial';
  return (
    <div
      className={cn(
        'space-y-4 rounded-lg border p-5',
        success
          ? 'border-success/40 bg-success-muted'
          : partial
            ? 'border-warning/40 bg-warning-muted'
            : 'border-danger/40 bg-danger-muted',
      )}
      role="status"
    >
      <div className="flex items-center gap-3">
        {success ? (
          <CheckCircle2Icon className="size-6 text-success" aria-hidden />
        ) : partial ? (
          <CircleAlertIcon className="size-6 text-warning" aria-hidden />
        ) : (
          <XCircleIcon className="size-6 text-danger" aria-hidden />
        )}
        <div>
          <h2 className="text-lg font-semibold">
            {success
              ? t('recovery.reportSuccess')
              : partial
                ? t('recovery.reportPartial')
                : t('recovery.reportFailed')}
          </h2>
          <p className="text-sm tabular">
            {t('recovery.reportSummary', {
              done: data.processedTotal - data.failed.length - data.skipped.length,
              total: data.processedTotal,
              failed: data.failed.length,
            })}
          </p>
        </div>
      </div>
      <div className="grid grid-cols-3 gap-3">
        <StatTile
          label={t('recovery.queueItem.done')}
          value={String(data.succeeded.length)}
          tone="success"
        />
        <StatTile
          label={t('recovery.queueItem.failed')}
          value={String(data.failed.length)}
          tone={data.failed.length > 0 ? 'danger' : 'default'}
        />
        <StatTile label={t('recovery.queueItem.skipped')} value={String(data.skipped.length)} />
      </div>
      {data.failed.length > 0 ? (
        <div className="max-h-40 overflow-y-auto rounded-md border border-border bg-surface p-2 text-xs">
          {data.failed.slice(0, 20).map((f) => (
            <div key={f.nodeId} className="flex items-center justify-between gap-3 py-1">
              <span className="truncate">{f.name}</span>
              <span className="text-danger">
                {f.error?.reasonKey ? t(f.error.reasonKey) : ''}
              </span>
            </div>
          ))}
        </div>
      ) : null}
      {data.truncatedItemLists ? (
        <p className="text-xs text-foreground-secondary">
          {t('recovery.listsTruncated', { total: data.processedTotal })}
        </p>
      ) : null}
      <div className="flex items-center justify-between gap-3">
        <span className="text-xs text-foreground-secondary tabular">
          {formatBytes(data.totalBytesWritten, i18n.language)}
          {data.verified ? ` · ${t('recovery.verifyOn')}` : ''}
        </span>
        <div className="flex gap-2">
          <Button variant="secondary" size="sm" onClick={() => navigate(`/results/${scanId}`)}>
            {t('common.close')}
          </Button>
          <Button
            variant="secondary"
            size="sm"
            onClick={() => {
              const text = JSON.stringify(
                {
                  status: data.status,
                  startedAt: data.startedAt,
                  finishedAt: data.finishedAt,
                  succeeded: data.succeeded.map((s) => ({
                    name: s.name,
                    savedAs: s.savedAs ?? s.name,
                    sizeBytes: s.sizeBytes,
                  })),
                  failed: data.failed.map((f) => ({ name: f.name, reason: f.error?.code })),
                  skipped: data.skipped.map((s) => s.name),
                  totalBytesWritten: data.totalBytesWritten,
                  verified: data.verified,
                },
                null,
                2,
              );
              const blob = new Blob([text], { type: 'application/json' });
              const url = URL.createObjectURL(blob);
              const a = document.createElement('a');
              a.href = url;
              a.download = `recovery-report-${jobId}.json`;
              a.click();
              URL.revokeObjectURL(url);
              toast.success(t('recovery.exported'));
            }}
          >
            <DownloadIcon aria-hidden />
            {t('recovery.reportExport')}
          </Button>
        </div>
      </div>
    </div>
  );
}

function QueueStateBadge({
  state,
  reasonKey,
}: {
  state: RecoveryItem['state'];
  reasonKey?: string;
}): React.JSX.Element {
  const { t } = useTranslation();
  const map: Record<
    string,
    { variant: 'neutral' | 'primary' | 'success' | 'warning' | 'danger'; label: string }
  > = {
    pending: { variant: 'neutral', label: t('recovery.queueItem.pending') },
    running: { variant: 'primary', label: t('recovery.queueItem.running') },
    verifying: { variant: 'primary', label: t('recovery.queueItem.verifying') },
    done: { variant: 'success', label: t('recovery.queueItem.done') },
    failed: { variant: 'danger', label: t('recovery.queueItem.failed') },
    skipped: { variant: 'warning', label: t('recovery.queueItem.skipped') },
  };
  const cfg = map[state] ?? map.pending;
  const badge = (
    <Badge variant={cfg.variant}>
      {state === 'running' ? (
        <span
          className="mr-1 inline-block size-1.5 animate-status-pulse rounded-full bg-current"
          aria-hidden
        />
      ) : null}
      {cfg.label}
    </Badge>
  );
  if (state === 'failed' && reasonKey) {
    return (
      <span title={t(reasonKey)} className="shrink-0">
        {badge}
      </span>
    );
  }
  return <span className="shrink-0">{badge}</span>;
}

function DestinationRow({
  label,
  path,
  freeBytes,
  totalBytes,
  selected,
  warning,
  onSelect,
}: {
  label: string;
  path: string;
  freeBytes: number;
  totalBytes: number;
  selected: boolean;
  warning: boolean;
  onSelect: () => void;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  return (
    <button
      type="button"
      onClick={onSelect}
      aria-pressed={selected}
      className={cn(
        'flex w-full items-center gap-4 rounded-lg border-2 bg-surface p-4 text-left transition-colors',
        selected
          ? 'border-primary'
          : 'border-transparent border-border hover:border-border-strong',
      )}
    >
      <span
        className={cn(
          'flex size-4 shrink-0 items-center justify-center rounded-full border-2 transition-colors',
          selected ? 'border-primary' : 'border-border-strong',
        )}
        aria-hidden
      >
        {selected ? <span className="size-2 rounded-full bg-primary" /> : null}
      </span>
      <div className="min-w-0 flex-1">
        <div className="flex items-center gap-2">
          <span className="truncate font-semibold">{label}</span>
          {warning ? <ShieldAlertIcon className="size-4 text-danger" aria-hidden /> : null}
        </div>
        <div className="text-xs text-foreground-secondary font-mono">{path}</div>
      </div>
      <div className="w-40">
        <CapacityBar
          usedBytes={totalBytes - freeBytes}
          totalBytes={totalBytes}
          label={t('recovery.freeSpace', {
            free: formatBytes(freeBytes, i18n.language),
            total: formatBytes(totalBytes, i18n.language),
          })}
        />
      </div>
    </button>
  );
}

function Toggle({
  checked,
  onChange,
  label,
  icon,
}: {
  checked: boolean;
  onChange: (v: boolean) => void;
  label: string;
  icon?: React.ReactNode;
}): React.JSX.Element {
  const id = React.useId();
  return (
    <div className="flex items-center justify-between gap-3 rounded-md border border-border px-3 py-2">
      <Label htmlFor={id} className="gap-1.5 text-sm">
        {icon}
        {label}
      </Label>
      <Switch id={id} checked={checked} onCheckedChange={onChange} />
    </div>
  );
}
