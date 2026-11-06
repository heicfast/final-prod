import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { ShieldAlertIcon } from 'lucide-react';
import { useRelaunchElevated, useSecurityPosture } from '@/queries/system';
import { Button } from '@/components/ui/button';
import { toast } from 'sonner';

/**
 * The elevation banner — the user-mode product's not-elevated EDGE-CASE
 * signal (D-PRIV-01: since 0.9.0 the exe carries a requireAdministrator
 * manifest, so a normal launch is always elevated and this banner is the
 * honest fallback for non-standard contexts — e.g. a scheduled task or a
 * shell that stripped the token). Raw-disk work (scan/recover/health/
 * repair) needs the administrator token: the engine refuses with
 * ERR_ELEVATION_REQUIRED (0x500C0001) and the product NEVER runs degraded
 * or pretends to work. The banner offers the relaunch-as-administrator
 * action (the standard OS UAC prompt — the host-triggers-its-own-elevation
 * contract, cr_ui.h). Styled exclusively on the semantic warning tokens
 * (D-UI-01: the pre-fix raw Tailwind amber classes bypassed the token
 * system and clashed with the app chrome in every theme family).
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
      className="flex shrink-0 items-center gap-3 border-b border-warning/40 bg-warning-muted px-4 py-2 text-[13px] text-foreground"
    >
      <ShieldAlertIcon className="size-4 shrink-0 text-warning" aria-hidden />
      <p className="min-w-0 flex-1">{t('elevation.bannerBody')}</p>
      <Button
        size="sm"
        variant="secondary"
        className="h-7 shrink-0 border-warning/60 bg-transparent text-warning hover:bg-warning-muted"
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
