import * as React from 'react';
import { useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import { ArchiveRestoreIcon, Clock3Icon, FolderSyncIcon, ShieldCheckIcon } from 'lucide-react';
import {
  useLoadSession,
  useResumeSession,
  useSessionLoadFraction,
  useSessions,
} from '@/queries/sessions';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { formatDate, formatInteger } from '@/lib/format';
import { Button } from '@/components/ui/button';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * Scan Snapshot page (renamed from ChronoSnap Backup in the v4 pass) —
 * accessible from the File Backup sidebar section. Displays the landing
 * overview and saved recovery checkpoints.
 */
export function BackupPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const sessions = useSessions();
  const loadSession = useLoadSession();
  const resumeSession = useResumeSession();
  const setActive = useScanSessionStore((s) => s.setActiveScan);
  // W-01 (the dead-letter fix): the REAL load progress for the row being
  // opened (the bridge's session-load-progress stream, 5%-coalesced).
  const [loadingPath, setLoadingPath] = React.useState<string | null>(null);
  const loadFraction = useSessionLoadFraction(loadingPath);

  const handleOpenSession = (fileName: string): void => {
    setLoadingPath(fileName);
    loadSession.mutate(fileName, {
      onSuccess: ({ scanId }) => {
        setLoadingPath(null);
        void navigate(`/results/${scanId}`);
      },
      onError: (err) => {
        setLoadingPath(null);
        toast.error(err instanceof Error ? err.message : t('errors.unknown'));
      },
    });
  };

  /** The scan RESUME (the CLI's `scan --resume`): continue an incomplete
   * session's scan on its original target (identity-verified by the
   * bridge — a mismatch is the typed session-corrupt refusal). */
  const handleResumeSession = (path: string | undefined): void => {
    if (!path) {
      toast.error(t('backup.resumeNeedsPath'));
      return;
    }
    resumeSession.mutate(path, {
      // v0.8.2 forensic (run 42185123973): the live-scan route is
      // /scan/:deviceId/live — /scan?scanId= is NOT a route (the catch-all
      // fallback swallowed it and the resumed scan ran invisibly). The
      // resume result carries the targetId for exactly this; the progress
      // page reads only the ?scanId= param. The active-scan store entry
      // lights the sidebar's pulsing link, matching useStartScan.
      onSuccess: ({ scanId, targetId }) => {
        setActive({ scanId, targetId });
        void navigate(`/scan/${targetId}/live?scanId=${scanId}`);
      },
      onError: (err) => toast.error(err instanceof Error ? err.message : t('errors.unknown')),
    });
  };

  return (
    <div className="flex h-full min-h-0 flex-col justify-between overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-5xl">
        <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
          {t('backup.title')}
        </h1>
        <p className="mt-1.5 text-[13px] text-[#64748B] dark:text-foreground-secondary">
          {t('backup.subtitle')}
        </p>

        <div className="mt-8 flex flex-col items-center justify-between gap-8 lg:flex-row">
          <div className="max-w-lg space-y-6">
            <div className="flex items-start gap-3.5">
              <div className="flex size-10 shrink-0 items-center justify-center rounded-[8px] border border-[#DCE6F5] bg-[#F4F7FC] text-[#1677FF] dark:border-border dark:bg-surface-elevated dark:text-primary">
                <Clock3Icon className="size-5" aria-hidden />
              </div>
              <div>
                <div className="text-[14px] font-bold text-[#1E293B] dark:text-foreground">
                  {t('backup.featurePointTitle')}
                </div>
                <div className="mt-0.5 text-[12.5px] text-[#64748B] dark:text-foreground-secondary">
                  {t('backup.featurePointBody')}
                </div>
              </div>
            </div>

            <div className="flex items-start gap-3.5">
              <div className="flex size-10 shrink-0 items-center justify-center rounded-[8px] border border-[#DCE6F5] bg-[#F4F7FC] text-[#1677FF] dark:border-border dark:bg-surface-elevated dark:text-primary">
                <FolderSyncIcon className="size-5" aria-hidden />
              </div>
              <div>
                <div className="text-[14px] font-bold text-[#1E293B] dark:text-foreground">
                  {t('backup.featureSyncTitle')}
                </div>
                <div className="mt-0.5 text-[12.5px] text-[#64748B] dark:text-foreground-secondary">
                  {t('backup.featureSyncBody')}
                </div>
              </div>
            </div>

            <div className="flex items-start gap-3.5">
              <div className="flex size-10 shrink-0 items-center justify-center rounded-[8px] border border-[#DCE6F5] bg-[#F4F7FC] text-[#1677FF] dark:border-border dark:bg-surface-elevated dark:text-primary">
                <ShieldCheckIcon className="size-5" aria-hidden />
              </div>
              <div>
                <div className="text-[14px] font-bold text-[#1E293B] dark:text-foreground">
                  {t('backup.featureSafeTitle')}
                </div>
                <div className="mt-0.5 text-[12.5px] text-[#64748B] dark:text-foreground-secondary">
                  {t('backup.featureSafeBody')}
                </div>
              </div>
            </div>

            <button
              type="button"
              onClick={() => void navigate('/')}
              className="mt-2 inline-flex h-10 w-[160px] items-center justify-center rounded-[6px] bg-[#1677FF] text-[14px] font-semibold text-white shadow-xs transition-all hover:bg-[#0958D9] active:scale-[var(--press-scale)] focus-visible:ring-2 focus-visible:ring-ring dark:bg-primary dark:text-primary-foreground dark:hover:bg-primary-hover"
            >
              {t('backup.startSearch')}
            </button>
          </div>

          {/* Saved Session Checkpoints */}
          <div className="w-full max-w-md rounded-[12px] border border-[#EBF2FF] bg-[#F8FAFF] p-5 dark:border-border dark:bg-surface-elevated">
            <h2 className="text-[13px] font-bold text-[#1E293B] dark:text-foreground">
              {t('backup.sessionsTitle')}
            </h2>
            {sessions.isLoading ? (
              <div className="mt-3 space-y-2">
                <Skeleton className="h-14" />
                <Skeleton className="h-14" />
              </div>
            ) : (sessions.data?.length ?? 0) === 0 ? (
              <div className="mt-4 flex flex-col items-center py-6 text-center">
                <ArchiveRestoreIcon
                  className="size-9 text-[#93C5FD] dark:text-primary"
                  aria-hidden
                />
                <p className="mt-2 text-[13px] font-semibold text-[#334155] dark:text-foreground">
                  {t('backup.emptyTitle')}
                </p>
                <p className="mt-1 text-[12px] text-[#64748B] dark:text-foreground-secondary">
                  {t('backup.emptyBody')}
                </p>
              </div>
            ) : (
              <div className="mt-3 space-y-2">
                {sessions.data?.map((session) => (
                  <div
                    key={session.id}
                    className="flex items-center justify-between gap-3 rounded-[8px] border border-[#E2E8F0] bg-white px-3.5 py-2.5 dark:border-border dark:bg-surface"
                  >
                    <div className="min-w-0">
                      <div className="truncate text-[13px] font-semibold text-[#1E293B] dark:text-foreground">
                        {session.targetLabel}
                      </div>
                      <div className="text-[11px] text-[#64748B] tabular dark:text-foreground-secondary">
                        {formatDate(session.savedAt, i18n.language)} ·{' '}
                        {formatInteger(session.filesFound, i18n.language)}
                      </div>
                    </div>
                    <div className="flex shrink-0 items-center gap-2">
                      {!session.completed ? (
                        <Button
                          size="sm"
                          variant="secondary"
                          loading={resumeSession.isPending}
                          onClick={() => handleResumeSession(session.path)}
                        >
                          {t('backup.resume')}
                        </Button>
                      ) : null}
                      <Button
                        size="sm"
                        variant="secondary"
                        loading={loadSession.isPending && loadingPath === session.fileName}
                        onClick={() => handleOpenSession(session.fileName)}
                      >
                        {loadingPath === session.fileName && loadFraction !== null
                          ? `${t('backup.opening')} ${Math.round(loadFraction * 100)}%`
                          : t('backup.open')}
                      </Button>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
