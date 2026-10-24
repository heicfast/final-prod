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
import {
  formatBytes,
  formatCountCompact,
  formatDuration,
  formatPercent,
  formatThroughput,
} from '@/lib/format';
import { ProgressGauge } from '@/components/ui/progress';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { Hint } from '@/components/ui/tooltip';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
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
  const setActiveStatus = useScanSessionStore((s) => s.setActiveStatus);

  const scanId = scanIdFromUrl ?? active?.scanId ?? null;
  const session = scanId ? known[scanId] : null;
  const targetId = session?.targetId ?? deviceId;

  const progress = useScanProgress(scanId);
  const pause = usePauseScan(scanId);
  const resume = useResumeScan(scanId);
  const cancel = useCancelScan(scanId);
  const [stopConfirmOpen, setStopConfirmOpen] = React.useState(false);
  const [stopStats, setStopStats] = React.useState<{ files: number; time: string } | null>(
    null,
  );
  const scanStartIso = React.useRef<string | null>(null);

  const status = progress?.status ?? 'starting';
  const overall = progress?.overallProgress ?? 0;
  const completed = status === 'completed';
  const deviceLost = progress?.deviceConnected === false && status === 'paused';

  // Record scan start once for elapsed-time stats (ref write, not state).
  React.useEffect(() => {
    if (progress && scanStartIso.current === null) {
      scanStartIso.current = new Date(
        Date.now() - (progress.overallProgress > 0 ? 1 : 0) * 0,
      ).toISOString();
    }
  }, [progress]);

  // A completed scan is no longer 'active' — the sidebar stops pulsing.
  React.useEffect(() => {
    if (
      progress?.status === 'completed' ||
      progress?.status === 'cancelled' ||
      progress?.status === 'failed'
    ) {
      setActiveStatus(scanId ?? '', progress.status);
    }
  }, [progress?.status, scanId, setActiveStatus]);

  if (!scanId) {
    return <ResultsPageFallback />;
  }

  return (
    <div className="flex h-full min-h-0 flex-col">
      {/* Scan banner strip */}
      {!completed ? (
        <div
          className={cnStrip(
            'flex shrink-0 items-center gap-3 border-b border-border px-4 py-1.5 text-sm',
            status === 'paused'
              ? 'bg-danger-muted text-danger'
              : 'bg-primary-muted text-primary',
          )}
          role="status"
          aria-live="polite"
        >
          <span
            className={cnStrip(
              'inline-block size-2 rounded-full bg-current',
              status === 'running' && 'animate-status-pulse',
            )}
          />
          <span className="font-medium">
            {status === 'paused'
              ? t('scan.pausedTitle')
              : t(`scan.stage.${progress?.stage ?? 'volume-analysis'}`)}
          </span>
          <span className="tabular">{formatPercent(overall, i18n.language)}</span>
          <span className="ml-auto truncate text-xs opacity-80">
            {t('scan.liveResultsNote')}
          </span>
        </div>
      ) : null}

      {/* Live results (partial) */}
      <div className="min-h-0 flex-1">
        <ResultsExplorer
          scanId={scanId}
          targetId={targetId}
          liveMode={!completed}
          recoverableDuringScan={progress?.recoverableDuringScan ?? false}
        />
      </div>

      {/* Bottom status cluster */}
      <div className="flex shrink-0 items-center gap-4 border-t border-border bg-surface px-4 py-2.5">
        <ProgressGauge value={overall} size={52} label={t('a11y.gauge')} />
        <div className="min-w-0 flex-1 space-y-0.5 text-xs">
          <div className="flex items-center gap-3">
            <span className="font-medium">{t('scan.overall')}:</span>
            <span className="tabular font-semibold text-primary">
              {formatPercent(overall, i18n.language)}
            </span>
            <span className="text-foreground-secondary tabular">
              {t('scan.filesFound')}:{' '}
              <strong className="text-foreground">
                {formatCountCompact(progress?.filesFound ?? 0, i18n.language)}
              </strong>
            </span>
            <span className="text-foreground-secondary tabular">
              {t('scan.dataFound')}:{' '}
              <strong className="text-foreground">
                {formatBytes(progress?.dataFoundBytes ?? 0, i18n.language)}
              </strong>
            </span>
          </div>
          <div className="flex items-center gap-3 text-foreground-secondary tabular">
            <span>
              {t('scan.scanned')}: {formatBytes(progress?.bytesScanned ?? 0, i18n.language)} /{' '}
              {formatBytes(progress?.bytesTotal ?? 0, i18n.language)}
            </span>
            <span>
              {t('scan.throughput')}:{' '}
              {formatThroughput(progress?.throughputBytesPerSecond ?? 0, i18n.language)}
            </span>
            {progress?.etaSeconds != null && progress.etaSeconds > 0 ? (
              <span>
                {t('scan.remaining', {
                  time: formatDuration(progress.etaSeconds, i18n.language),
                })}
              </span>
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
            <Button
              variant="secondary"
              size="md"
              onClick={() => void pause.mutate()}
              disabled={pause.isPending}
            >
              <PauseIcon aria-hidden />
              {t('scan.pause')}
            </Button>
          ) : status === 'paused' && !deviceLost ? (
            <Button
              variant="secondary"
              size="md"
              onClick={() => void resume.mutate()}
              disabled={resume.isPending}
            >
              <PlayIcon aria-hidden />
              {t('scan.resume')}
            </Button>
          ) : null}
          {!completed ? (
            <Button
              variant="dangerOutline"
              size="md"
              onClick={() => {
                const started = scanStartIso.current ?? new Date().toISOString();
                setStopStats({
                  files: progress?.filesFound ?? 0,
                  time: formatDuration(
                    Math.max(0, (Date.now() - Date.parse(started)) / 1000),
                    i18n.language,
                  ),
                });
                setStopConfirmOpen(true);
              }}
            >
              <SquareIcon aria-hidden />
              {t('scan.stop')}
            </Button>
          ) : null}
        </div>
      </div>

      {/* Device-lost reconnect prompt — modal, non-dismissable: the scan
          stays paused until the source device comes back. */}
      {deviceLost ? (
        <Dialog open onOpenChange={() => undefined}>
          <DialogContent
            showCloseButton={false}
            className="max-w-md"
            onEscapeKeyDown={(e) => e.preventDefault()}
            onInteractOutside={(e) => e.preventDefault()}
          >
            <div className="flex items-start gap-3">
              <UnplugIcon className="mt-0.5 size-5 shrink-0 text-warning" aria-hidden />
              <DialogHeader className="min-w-0">
                <DialogTitle className="text-base">{t('scan.deviceLostTitle')}</DialogTitle>
                <DialogDescription>{t('scan.deviceLostBody')}</DialogDescription>
              </DialogHeader>
            </div>
            <DialogFooter>
              <Button
                variant="primary"
                onClick={() => {
                  void resume.mutate(undefined, {
                    onSuccess: () => toast.success(t('scan.resume')),
                    onError: (err) => toast.error(err.message),
                  });
                }}
              >
                {t('scan.reconnect')}
              </Button>
            </DialogFooter>
          </DialogContent>
        </Dialog>
      ) : null}

      {/* Stop confirmation */}
      <Dialog open={stopConfirmOpen} onOpenChange={setStopConfirmOpen}>
        <DialogContent className="max-w-sm" aria-label={t('scan.stopTitle')}>
          <DialogHeader>
            <DialogTitle className="text-base">{t('scan.stopTitle')}</DialogTitle>
            <DialogDescription>{t('scan.stopBody')}</DialogDescription>
          </DialogHeader>
          <div className="grid grid-cols-2 gap-2">
            <MiniStat
              label={t('scan.statsFiles')}
              value={formatCountCompact(
                stopStats?.files ?? progress?.filesFound ?? 0,
                i18n.language,
              )}
            />
            <MiniStat label={t('scan.statsTime')} value={stopStats?.time ?? '—'} />
          </div>
          <DialogFooter>
            <Button variant="secondary" onClick={() => setStopConfirmOpen(false)}>
              {t('scan.keepGoing')}
            </Button>
            <Button
              variant="danger"
              onClick={() => {
                setStopConfirmOpen(false);
                void cancel.mutate(undefined, {
                  onSuccess: () => {
                    markStopped();
                    void qc.invalidateQueries({ queryKey: ['sessions'] });
                    toast(t('common.stop'));
                    void navigate(`/results/${scanId}`);
                  },
                });
              }}
            >
              {t('scan.stopConfirm')}
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>

      {/* Celebration modal */}
      <Dialog
        open={completed && seenFor !== scanId}
        onOpenChange={(open) => {
          if (!open && scanId) {
            markSeen(scanId);
            void navigate(`/results/${scanId}`);
          }
        }}
      >
        <DialogContent
          showCloseButton={false}
          className="w-[540px] max-w-[92vw] rounded-xl p-6 text-center"
          aria-label={t('scan.completedTitle')}
        >
          <div className="mx-auto mb-3 flex size-14 items-center justify-center rounded-full bg-success-muted text-success">
            <PartyPopperIcon className="size-7" strokeWidth={1.5} aria-hidden />
          </div>
          <DialogTitle className="text-hero font-semibold">
            {t('scan.completedTitle')}
          </DialogTitle>
          <DialogDescription className="mt-1">{t('scan.completedBody')}</DialogDescription>
          <div className="mt-5 grid grid-cols-2 gap-3 text-left">
            <MiniStat
              label={t('scan.statsFiles')}
              value={formatCountCompact(
                stopStats?.files ?? progress?.filesFound ?? 0,
                i18n.language,
              )}
            />
            <MiniStat
              label={t('scan.statsSize')}
              value={formatBytes(progress?.dataFoundBytes ?? 0, i18n.language)}
            />
            <MiniStat
              label={t('scan.statsTime')}
              value={
                formatDuration(((progress?.etaSeconds ?? 0) + 180) * 0, i18n.language) || '—'
              }
            />
            <MiniStat
              label={t('scan.statsErrors')}
              value={String(progress?.errors ?? 0)}
              tone={(progress?.errors ?? 0) > 0 ? 'warning' : 'default'}
            />
          </div>
          <Button
            variant="primary"
            size="xl"
            className="mt-6 w-full"
            onClick={() => {
              markSeen(scanId);
              void navigate(`/results/${scanId}`);
            }}
          >
            {t('scan.goToResults')}
          </Button>
        </DialogContent>
      </Dialog>
    </div>
  );
}

function MiniStat({
  label,
  value,
  tone = 'default',
}: {
  label: string;
  value: string;
  tone?: 'default' | 'warning';
}): React.JSX.Element {
  return (
    <div className="rounded-md border border-border bg-surface-muted px-3 py-2">
      <div className="text-xs text-foreground-secondary">{label}</div>
      <div
        className={cnStrip(
          'text-lg font-semibold tabular',
          tone === 'warning' && 'text-warning',
        )}
      >
        {value}
      </div>
    </div>
  );
}

function cnStrip(...parts: (string | false | undefined | null)[]): string {
  return parts.filter(Boolean).join(' ');
}
