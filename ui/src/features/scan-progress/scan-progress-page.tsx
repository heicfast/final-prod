import * as React from 'react';
import { useNavigate, useSearchParams } from 'react-router';
import { useTranslation } from 'react-i18next';
import { useQueryClient } from '@tanstack/react-query';
import {
  CircleAlertIcon,
  Clock3Icon,
  PartyPopperIcon,
  PauseIcon,
  PlayIcon,
  SquareIcon,
  UnplugIcon,
} from 'lucide-react';
import { scanPhaseOf } from '@/connector/contract';
import {
  useCancelScan,
  usePauseScan,
  useResumeScan,
  useScanProgress,
  useScanState,
} from '@/queries/scan';
import { queryKeys } from '@/queries/client';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { formatBytes, formatCountCompact, formatDuration, formatPercent } from '@/lib/format';
import { cn } from '@/lib/utils';
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
 * corpus bottom control cluster (atlas §6, GT-scan §2/§7): circular gauge,
 * the reference status-line formats ("Quick/Advanced Scanning, Files Found:
 * N (size)" / "Remaining time: … | Reading sector: X/Y"), the full-width
 * "Searching: <path>" ticker, pause/stop, and the completion celebration.
 * The scan runs INSIDE the results surface, per the corpus state machine.
 */
export function ScanProgressPage(): React.JSX.Element {
  const [params] = useSearchParams();
  const scanIdFromUrl = params.get('scanId');
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const qc = useQueryClient();

  const active = useScanSessionStore((s) => s.active);
  const markStopped = useScanSessionStore((s) => s.markScanStopped);
  const markSeen = useScanSessionStore((s) => s.markCelebrationSeen);
  const seenFor = useScanSessionStore((s) => s.celebrationSeenFor);
  const setActiveStatus = useScanSessionStore((s) => s.setActiveStatus);

  const scanId = scanIdFromUrl ?? active?.scanId ?? null;

  const progress = useScanProgress(scanId);
  const scanState = useScanState(scanId);
  const pause = usePauseScan(scanId);
  const resume = useResumeScan(scanId);
  const cancel = useCancelScan(scanId);
  const [stopConfirmOpen, setStopConfirmOpen] = React.useState(false);
  const [stopStats, setStopStats] = React.useState<{ files: number; time: string } | null>(
    null,
  );

  const status = progress?.status ?? 'starting';
  const overall = progress?.overallProgress ?? 0;
  const completed = status === 'completed';
  const deviceLost = progress?.deviceConnected === false && status === 'paused';
  const phase = scanPhaseOf(progress?.stage ?? 'volume-analysis');

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

  /** Elapsed since the engine recorded the scan start (real, not derived).
   *  A 1 s ticker keeps it live while the scan runs; completion freezes it
   *  at the engine's own timestamps (pure render — no Date.now() inline). */
  const [nowMs, setNowMs] = React.useState(() => Date.now());
  React.useEffect(() => {
    if (completed) return;
    const id = window.setInterval(() => setNowMs(Date.now()), 1_000);
    return () => window.clearInterval(id);
  }, [completed]);
  const usedTime = React.useMemo(() => {
    const startedAt = scanState.data?.startedAt;
    if (!startedAt) return null;
    const ref = scanState.data?.completedAt ? Date.parse(scanState.data.completedAt) : nowMs;
    const secs = Math.max(0, (ref - Date.parse(startedAt)) / 1000);
    return formatDuration(secs, i18n.language);
  }, [scanState.data, nowMs, i18n.language]);

  if (!scanId) {
    return <ResultsPageFallback />;
  }

  const sectorsScanned = Math.floor((progress?.bytesScanned ?? 0) / 512);
  const sectorsTotal = Math.floor((progress?.bytesTotal ?? 0) / 512);

  return (
    <div className="flex h-full min-h-0 flex-col">
      {/* Live results (partial) — browse-while-scanning (GT-scan §3.2) */}
      <div className="min-h-0 flex-1">
        <ResultsExplorer
          scanId={scanId}
          liveMode={!completed}
          recoverableDuringScan={progress?.recoverableDuringScan ?? false}
          showFooter={false}
        />
      </div>

      {/* "Searching: <path>" ticker — full-width strip above the control
          cluster (corpus GT-scan §2; hidden when there is nothing to show). */}
      {!completed && progress?.currentPath ? (
        <div
          className="flex h-6 shrink-0 items-center gap-2 border-t border-border bg-[#cfe2ff] px-3 text-xs text-primary"
          role="status"
        >
          <span
            className={cn(
              'inline-block size-1.5 rounded-full bg-current',
              status === 'running' && 'animate-status-pulse',
            )}
            aria-hidden
          />
          <span className="truncate font-mono">{progress.currentPath}</span>
        </div>
      ) : null}

      {/* Bottom status cluster (atlas §6) */}
      <div className="flex min-h-[88px] shrink-0 items-center gap-3 border-t border-border bg-surface px-3 py-2.5 sm:px-4">
        <ProgressGauge value={overall} size={58} label={t('a11y.gauge')} />
        <div className="min-w-0 flex-1 space-y-1 text-xs">
          {/* Reference status line: "Quick/Advanced Scanning , Files Found: N (size)" */}
          <div
            className="flex flex-wrap items-center gap-x-3 gap-y-0.5"
            role="status"
            aria-live="polite"
          >
            <span
              className={cn(
                'text-md font-semibold',
                status === 'paused' ? 'text-danger' : 'text-foreground',
              )}
            >
              {status === 'paused'
                ? t('scan.pausedTitle')
                : completed
                  ? t('scan.completedStatus')
                  : t(phase === 'quick' ? 'scan.quickScanning' : 'scan.advancedScanning', {
                      percent: formatPercent(overall, i18n.language),
                    })}
            </span>
            <span className="text-foreground-secondary tabular">
              {t('scan.filesFoundWithSize', {
                count: formatCountCompact(progress?.filesFound ?? 0, i18n.language),
                size: formatBytes(progress?.dataFoundBytes ?? 0, i18n.language),
              })}
            </span>
            {(progress?.errors ?? 0) > 0 ? (
              <Hint text={t('scan.badSectorMap')}>
                <Badge variant="danger">
                  <CircleAlertIcon aria-hidden />
                  {t('scan.errorsCount', { count: progress!.errors })}
                </Badge>
              </Hint>
            ) : null}
          </div>
          {/* Reference second line: "Remaining time: NNmNNs | Reading sector: X/Y" */}
          <div className="flex flex-wrap items-center gap-x-3 gap-y-0.5 text-foreground-secondary tabular">
            {progress?.etaSeconds != null && progress.etaSeconds > 0 && !completed ? (
              <span>
                {t('scan.remaining', {
                  time: formatDuration(progress.etaSeconds, i18n.language),
                })}
              </span>
            ) : null}
            {!completed ? (
              <span>
                {t('scan.readingSector', {
                  current: sectorsScanned.toLocaleString(i18n.language),
                  total: sectorsTotal.toLocaleString(i18n.language),
                })}
              </span>
            ) : null}
            {completed && usedTime ? (
              <span>{t('scan.usedTime', { time: usedTime })}</span>
            ) : null}
          </div>
        </div>
        <div className="flex shrink-0 items-center gap-1.5">
          {status === 'running' ? (
            <Button
              variant="ghost"
              size="icon"
              onClick={() => void pause.mutate()}
              disabled={pause.isPending}
              aria-label={t('scan.pause')}
              title={t('scan.pause')}
            >
              <PauseIcon aria-hidden />
            </Button>
          ) : status === 'paused' && !deviceLost ? (
            <Button
              variant="ghost"
              size="icon"
              onClick={() => void resume.mutate()}
              disabled={resume.isPending}
              aria-label={t('scan.resume')}
              title={t('scan.resume')}
            >
              <PlayIcon aria-hidden />
            </Button>
          ) : null}
          {!completed ? (
            <Button
              variant="dangerOutline"
              size="icon"
              onClick={() => {
                setStopStats({
                  files: progress?.filesFound ?? 0,
                  time: usedTime ?? formatDuration(progress?.etaSeconds ?? 0, i18n.language),
                });
                setStopConfirmOpen(true);
              }}
              aria-label={t('scan.stop')}
              title={t('scan.stop')}
            >
              <SquareIcon aria-hidden />
            </Button>
          ) : null}
          <Button
            variant="primary"
            size="lg"
            className="ml-2 min-w-40 rounded-lg px-5"
            disabled={!completed && !progress?.recoverableDuringScan}
            title={
              !completed && !progress?.recoverableDuringScan
                ? t('scan.recoverNowUnsafe')
                : undefined
            }
            onClick={() => void navigate(`/recover/${scanId}?all=1`)}
          >
            <Clock3Icon aria-hidden />
            {t('results.status.recoverAll')}
          </Button>
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

      {/* Stop confirmation (CScanCommonPopDlg: stat tiles + risk framing) */}
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
                    void qc.invalidateQueries({ queryKey: queryKeys.sessions });
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

      {/* Celebration modal (corpus §7: standalone 540×380, no Esc dismiss,
          stats + Go to Recover CTA). */}
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
          onEscapeKeyDown={(e) => e.preventDefault()}
        >
          <div className="mx-auto mb-3 flex size-14 items-center justify-center rounded-full bg-success-muted text-success">
            <PartyPopperIcon className="size-7" strokeWidth={1.5} aria-hidden />
          </div>
          <DialogTitle className="text-hero font-semibold">
            {t('scan.completedTitle')}
          </DialogTitle>
          <DialogDescription className="mt-1">{t('scan.completedVerdict')}</DialogDescription>
          <div className="mt-5 grid grid-cols-3 gap-3 text-left">
            <MiniStat
              label={t('scan.statsFiles')}
              value={formatCountCompact(progress?.filesFound ?? 0, i18n.language)}
            />
            <MiniStat
              label={t('scan.statsSize')}
              value={formatBytes(progress?.dataFoundBytes ?? 0, i18n.language)}
            />
            <MiniStat label={t('scan.statsTime')} value={usedTime ?? '—'} />
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
            {t('scan.goToRecover')}
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
        className={cn('text-lg font-semibold tabular', tone === 'warning' && 'text-warning')}
      >
        {value}
      </div>
    </div>
  );
}
