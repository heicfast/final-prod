import { ArchiveRestoreIcon, Clock3Icon, SearchIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router';
import { useSessions, useLoadSession } from '@/queries/sessions';
import { Button } from '@/components/ui/button';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { formatDate, formatInteger } from '@/lib/format';
import { toast } from 'sonner';

/**
 * The reference's backup entry is represented honestly as an archive of real
 * saved recovery sessions. It never promises a filesystem backup service the
 * engine cannot provide; opening a checkpoint performs the existing resume
 * operation and routes to its actual result set.
 */
export function BackupPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const sessions = useSessions();
  const load = useLoadSession();

  const open = (path: string): void => {
    load.mutate(path, {
      onSuccess: ({ scanId }) => void navigate(`/results/${scanId}`),
      onError: (error) =>
        toast.error(error instanceof Error ? error.message : t('errors.unknown')),
    });
  };

  return (
    <div className="h-full overflow-y-auto">
      <section className="mx-auto w-full max-w-4xl p-6">
        <div className="flex items-start gap-3">
          <div className="mt-0.5 flex size-10 items-center justify-center rounded-md bg-primary-muted text-primary">
            <Clock3Icon className="size-5" aria-hidden />
          </div>
          <div>
            <h1 className="text-xl font-semibold">{t('backup.title')}</h1>
            <p className="mt-1 max-w-2xl text-sm text-foreground-secondary">
              {t('backup.subtitle')}
            </p>
          </div>
        </div>

        <h2 className="mt-8 text-sm font-semibold text-foreground-secondary">
          {t('backup.sessionsTitle')}
        </h2>
        {sessions.isLoading ? (
          <div className="mt-3 space-y-2">
            <Skeleton className="h-20" />
            <Skeleton className="h-20" />
          </div>
        ) : sessions.isError ? (
          <div className="mt-3">
            <EmptyState
              icon={ArchiveRestoreIcon}
              title={t('errors.title')}
              description={
                sessions.error instanceof Error ? sessions.error.message : t('errors.unknown')
              }
              actionLabel={t('common.retry')}
              onAction={() => void sessions.refetch()}
            />
          </div>
        ) : (sessions.data?.length ?? 0) === 0 ? (
          <div className="mt-3">
            <EmptyState
              icon={ArchiveRestoreIcon}
              title={t('backup.emptyTitle')}
              description={t('backup.emptyBody')}
              actionLabel={t('backup.startSearch')}
              onAction={() => void navigate('/')}
            />
          </div>
        ) : (
          <div className="mt-3 space-y-2">
            {sessions.data?.map((session) => (
              <article
                key={session.id}
                className="flex items-center gap-4 rounded-md border border-border bg-surface p-4 shadow-card"
              >
                <ArchiveRestoreIcon className="size-5 shrink-0 text-primary" aria-hidden />
                <div className="min-w-0 flex-1">
                  <h3 className="truncate text-sm font-semibold">{session.targetLabel}</h3>
                  <p className="mt-0.5 truncate text-xs text-foreground-secondary tabular">
                    {formatDate(session.savedAt, i18n.language)} ·{' '}
                    {formatInteger(session.filesFound, i18n.language)}
                  </p>
                </div>
                <Button
                  size="sm"
                  variant="secondary"
                  loading={load.isPending}
                  onClick={() => open(session.fileName)}
                >
                  <SearchIcon aria-hidden />
                  {t('backup.open')}
                </Button>
              </article>
            ))}
          </div>
        )}
      </section>
    </div>
  );
}
