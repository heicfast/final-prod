import * as React from 'react';
import { useNavigate, useParams, useSearchParams } from 'react-router';
import { useTranslation } from 'react-i18next';
import { useQueryClient } from '@tanstack/react-query';
import {
  CircleAlertIcon,
  PartyPopperIcon,
  PauseIcon,
  PlayIcon,
  SquareIcon,
  UnplugIcon,
} from 'lucide-react';
import { useCancelScan, usePauseScan, useResumeScan, useScanProgress } from '@/queries/scan';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { formatBytes, formatCountCompact, formatDuration, formatPercent, formatThroughput } from '@/lib/format';
import { ProgressGauge } from '@/components/ui/progress';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Hint } from '@/components/ui/tooltip';
import { ResultsExplorer } from '@/features/results-explorer/results-explorer';
import { toast } from 'sonner';
import { ResultsPageFallback } from '@/features/results-explorer/page-fallback';

/**
 * Scan Progress — the 3-pane results layout live during the scan, plus the
 * bottom status cluster (weighted-monotonic circular gauge, stage line,
 * counters, pause/stop) and the completion celebration. Scan runs INSIDE
 * the results surface, per the corpus state machine.
 */
export function ScanProgressPage(): React.JSX.Element {
  const { deviceId = '' } = useParams();
  const [params] = useSearchParams();
  const scanIdFromUrl = params.get('scanId');
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const qc = useQueryClient();

  const active = useScanSessionStore((s) => s.active);
  const known = useScanSessionStore((s) => s.known);
  const markStopped = useScanSessionStore((s) => s.markScanStopped);
  const markSeen = useScanSessionStore((s) => s.markCelebrationSeen);
  const seenFor = useScanSessionStore((s) => s.celebrationSeenFor);

  const scanId = scanIdFromUrl ?? active?.scanId ?? null;
  const session = scanId ? known[scanId] : null;
  const targetId = session?.targetId ?? deviceId;

  const progress = useScanProgress(scanId);
  const pause = usePauseScan(scanId);
  const resume = useResumeScan(scanId);
  const cancel = useCancelScan(scanId);
  const [stopConfirmOpen, setStopConfirmOpen] = React.useState(false);

  const status = progress?.status ?? 'starting';
  const overall = progress?.overallProgress ?? 0;
  const completed = status === 'completed';
  const deviceLost = progress?.deviceConnected === false && status === 'paused';

  // Auto-redirect shortly after completion unless the user is browsing.
  React.useEffect(() => {
    if (!completed || !scanId || seenFor === scanId) return;
    const id = window.setTimeout(() => {
      navigate(`/results/${scanId}`);
    }, 1200);
    return () => window.clearTimeout(id);
  }, [completed, scanId, seenFor, navigate]);

  if (!scanId) {
    return <ResultsPageFallback />;
  }

  const startedAt = performance.now();

  return (
    <div className="flex h-full min-h-0 flex-col">
      {/* Scan banner strip */}
      {!completed ? (
        <div
          className={cnStrip(
            'flex shrink-0 items-center gap-3 border-b border-border px-4 py-1.5 text-sm',
            status === 'paused' ? 'bg-danger-muted text-danger' : 'bg-primary-muted text-primary',
          )}
          role="status"
          aria-live="polite"
        >
          <span className={cnStrip('inline-block size-2 rounded-full bg-current', status === 'running' && 'animate-status-pulse')} />
          <span className="font-medium">
            {status === 'paused' ? t('scan.pausedTitle') : t(`scan.stage.${progress?.stage ?? 'volume-analysis'}`)}
          </span>
          <span className="tabular">{formatPercent(overall, i18n.language)}</span>
          <span className="ml-auto truncate text-xs opacity-80">{t('scan.liveResultsNote')}</span>
        </div>
      ) : null}

      {/* Live results (partial) */}
      <div className="min-h-0 flex-1">
        <ResultsExplorer scanId={scanId} targetId={targetId} liveMode={!completed} recoverableDuringScan={progress?.recoverableDuringScan ?? false} />
      </div>

      {/* Bottom status cluster */}
      <div className="flex shrink-0 items-center gap-4 border-t border-border bg-surface px-4 py-2.5">
        <ProgressGauge value={overall} size={52} label={t('a11y.gauge')} />
        <div className="min-w-0 flex-1 space-y-0.5 text-xs">
          <div className="flex items-center gap-3">
            <span className="font-medium">{t('scan.overall')}:</span>
            <span className="tabular font-semibold text-primary">{formatPercent(overall, i18n.language)}</span>
            <span className="text-foreground-secondary tabular">
              {t('scan.filesFound')}: <strong className="text-foreground">{formatCountCompact(progress?.filesFound ?? 0, i18n.language)}</strong>
            </span>
            <span className="text-foreground-secondary tabular">
              {t('scan.dataFound')}: <strong className="text-foreground">{formatBytes(progress?.dataFoundBytes ?? 0, i18n.language)}</strong>
            </span>
          </div>
          <div className="flex items-center gap-3 text-foreground-secondary tabular">
            <span>{t('scan.scanned')}: {formatBytes(progress?.bytesScanned ?? 0, i18n.language)} / {formatBytes(progress?.bytesTotal ?? 0, i18n.language)}</span>
            <span>{t('scan.throughput')}: {formatThroughput(progress?.throughputBytesPerSecond ?? 0, i18n.language)}</span>
            {progress?.etaSeconds != null && progress.etaSeconds > 0 ? (
              <span>{t('scan.remaining', { time: formatDuration(progress.etaSeconds, i18n.language) })}</span>
            ) : null}
            {(progress?.errors ?? 0) > 0 ? (
              <Hint text={t('scan.badSectorMap')}>
                <Badge variant="danger">
                  <CircleAlertIcon aria-hidden />
                  {t('scan.errorsCount', { count: progress!.errors })}
                </Badge>
              </Hint>
            ) : null}
          </div>
        </div>
        <div className="flex items-center gap-2">
          {status === 'running' ? (
            <Button variant="secondary" size="md" onClick={() => pause.mutate()} disabled={pause.isPending}>
              <PauseIcon aria-hidden />
              {t('scan.pause')}
            </Button>
          ) : status === 'paused' && !deviceLost ? (
            <Button variant="secondary" size="md" onClick={() => resume.mutate()} disabled={resume.isPending}>
              <PlayIcon aria-hidden />
              {t('scan.resume')}
            </Button>
          ) : null}
          {!completed ? (
            <Button variant="dangerOutline" size="md" onClick={() => setStopConfirmOpen(true)}>
              <SquareIcon aria-hidden />
              {t('scan.stop')}
            </Button>
          ) : null}
        </div>
      </div>

      {/* Device-lost reconnect prompt */}
      {deviceLost ? (
        <div className="fixed inset-x-0 bottom-24 mx-auto w-fit rounded-lg border border-warning/50 bg-surface p-4 shadow-dialog" role="alertdialog" aria-label={t('scan.deviceLostTitle')}>
          <div className="flex items-start gap-3">
            <UnplugIcon className="mt-0.5 size-5 text-warning" aria-hidden />
            <div>
              <p className="font-semibold">{t('scan.deviceLostTitle')}</p>
              <p className="text-sm text-foreground-secondary">{t('scan.deviceLostBody')}</p>
            </div>
            <Button
              variant="primary"
              size="sm"
              onClick={() => {
                resume.mutate(undefined, {
                  onSuccess: () => toast.success(t('scan.resume')),
                  onError: (err) => toast.error(err.message),
                });
              }}
            >
              {t('scan.reconnect')}
            </Button>
          </div>
        </div>
      ) : null}

      {/* Stop confirmation */}
      {stopConfirmOpen ? (
        <div className="fixed inset-0 z-40 flex items-center justify-center bg-[var(--overlay-scrim)]" role="dialog" aria-modal="true" aria-label={t('scan.stopTitle')}>
          <div className="w-full max-w-sm rounded-lg border border-border bg-surface p-5 shadow-dialog">
            <h2 className="text-base font-semibold">{t('scan.stopTitle')}</h2>
            <p className="mt-1.5 text-sm text-foreground-secondary">{t('scan.stopBody')}</p>
            <div className="mt-4 grid grid-cols-2 gap-2">
              <MiniStat label={t('scan.statsFiles')} value={formatCountCompact(progress?.filesFound ?? 0, i18n.language)} />
              <MiniStat label={t('scan.statsTime')} value={formatDuration((performance.now() - startedAt) / 1000, i18n.language)} />
            </div>
            <div className="mt-5 flex justify-end gap-2">
              <Button variant="secondary" onClick={() => setStopConfirmOpen(false)}>
                {t('scan.keepGoing')}
              </Button>
              <Button
                variant="danger"
                onClick={() => {
                  setStopConfirmOpen(false);
                  cancel.mutate(undefined, {
                    onSuccess: () => {
                      markStopped();
                      void qc.invalidateQueries({ queryKey: ['sessions'] });
                      toast(t('common.stop'));
                      navigate(`/results/${scanId}`);
                    },
                  });
                }}
              >
                {t('scan.stopConfirm')}
              </Button>
            </div>
          </div>
        </div>
      ) : null}

      {/* Celebration modal */}
      {completed && seenFor !== scanId ? (
        <div className="fixed inset-0 z-40 flex items-center justify-center bg-[var(--overlay-scrim)]" role="dialog" aria-modal="true" aria-label={t('scan.completedTitle')}>
          <div className="w-[540px] max-w-[92vw] rounded-xl border border-border bg-surface p-6 text-center shadow-dialog">
            <div className="mx-auto mb-3 flex size-14 items-center justify-center rounded-full bg-success-muted text-success">
              <PartyPopperIcon className="size-7" strokeWidth={1.5} aria-hidden />
            </div>
            <h2 className="text-hero font-semibold">{t('scan.completedTitle')}</h2>
            <p className="mt-1 text-sm text-foreground-secondary">{t('scan.completedBody')}</p>
            <div className="mt-5 grid grid-cols-2 gap-3 text-left">
              <MiniStat label={t('scan.statsFiles')} value={formatCountCompact(progress?.filesFound ?? 0, i18n.language)} />
              <MiniStat label={t('scan.statsSize')} value={formatBytes(progress?.dataFoundBytes ?? 0, i18n.language)} />
              <MiniStat label={t('scan.statsTime')} value={formatDuration(((progress?.etaSeconds ?? 0) + 180) * 0, i18n.language) || '—'} />
              <MiniStat label={t('scan.statsErrors')} value={String(progress?.errors ?? 0)} tone={(progress?.errors ?? 0) > 0 ? 'warning' : 'default'} />
            </div>
            <Button
              variant="primary"
              size="xl"
              className="mt-6 w-full"
              onClick={() => {
                markSeen(scanId);
                navigate(`/results/${scanId}`);
              }}
            >
              {t('scan.goToResults')}
            </Button>
          </div>
        </div>
      ) : null}
    </div>
  );
}

function MiniStat({ label, value, tone = 'default' }: { label: string; value: string; tone?: 'default' | 'warning' }): React.JSX.Element {
  return (
    <div className="rounded-md border border-border bg-surface-muted px-3 py-2">
      <div className="text-xs text-foreground-secondary">{label}</div>
      <div className={cnStrip('text-lg font-semibold tabular', tone === 'warning' && 'text-warning')}>{value}</div>
    </div>
  );
}

function cnStrip(...parts: (string | false | undefined | null)[]): string {
  return parts.filter(Boolean).join(' ');
}

export { EmptyState };
