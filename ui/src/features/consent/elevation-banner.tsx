import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { ShieldAlertIcon } from 'lucide-react';
import { useRelaunchElevated, useSecurityPosture } from '@/queries/system';
import { Button } from '@/components/ui/button';
import { toast } from 'sonner';

/**
 * The elevation banner — the user-mode product's primary not-elevated
 * signal (plan §4.3.5). Raw-disk work (scan/recover/health/repair) needs
 * the administrator token: the engine refuses with ERR_ELEVATION_REQUIRED
 * (0x500C0001) and the product NEVER runs degraded or pretends to work.
 * The banner offers the relaunch-as-administrator action (the standard OS
 * UAC prompt — the host-triggers-its-own-elevation contract, cr_ui.h).
 */
export function ElevationBanner(): React.JSX.Element | null {
  const { t } = useTranslation();
  const posture = useSecurityPosture();
  const relaunch = useRelaunchElevated();

  if (posture.isLoading || posture.isError || !posture.data) return null;
  if (posture.data.administrator) return null;

  return (
    <div
      role="alert"
      className="flex shrink-0 items-center gap-3 border-b border-amber-300 bg-amber-50 px-4 py-2 text-[13px] text-amber-900 dark:border-amber-500/40 dark:bg-amber-500/10 dark:text-amber-200"
    >
      <ShieldAlertIcon className="size-4 shrink-0" aria-hidden />
      <p className="min-w-0 flex-1">{t('elevation.bannerBody')}</p>
      <Button
        size="sm"
        variant="secondary"
        className="h-7 shrink-0 border-amber-400 bg-transparent text-amber-900 hover:bg-amber-100 dark:text-amber-200 dark:hover:bg-amber-500/20"
        disabled={relaunch.isPending}
        onClick={() =>
          relaunch.mutate(undefined, {
            onError: (err) =>
              toast.error(err instanceof Error ? err.message : t('errors.unknown')),
          })
        }
      >
        {relaunch.isPending ? t('common.processing') : t('elevation.relaunch')}
      </Button>
    </div>
  );
}
