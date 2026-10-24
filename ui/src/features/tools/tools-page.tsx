import * as React from 'react';
import {
  ActivityIcon,
  CloudIcon,
  CrosshairIcon,
  FileImageIcon,
  HardDriveIcon,
  NetworkIcon,
  ShieldAlertIcon,
  WrenchIcon,
} from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router';
import { Button } from '@/components/ui/button';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';

const TOOLS = [
  {
    route: '/lost-partition',
    icon: CrosshairIcon,
    title: 'lostTitle',
    body: 'lostBody',
    available: true,
  },
  {
    route: '/repair',
    icon: WrenchIcon,
    title: 'repairTitle',
    body: 'repairBody',
    available: true,
  },
  {
    route: '/health',
    icon: ActivityIcon,
    title: 'healthTitle',
    body: 'healthBody',
    available: true,
  },
  {
    route: null,
    icon: HardDriveIcon,
    title: 'crashedTitle',
    body: 'crashedBody',
    available: false,
  },
  { route: null, icon: NetworkIcon, title: 'nasTitle', body: 'nasBody', available: false },
  {
    route: null,
    icon: ShieldAlertIcon,
    title: 'raidTitle',
    body: 'raidBody',
    available: false,
  },
  { route: null, icon: CloudIcon, title: 'cloudTitle', body: 'cloudBody', available: false },
  {
    route: null,
    icon: FileImageIcon,
    title: 'imageTitle',
    body: 'imageBody',
    available: false,
  },
] as const;

/** Tool-grid and acknowledgement frame from the reference. Real routes open;
 * provider-backed tools explain their requirement instead of pretending to run. */
export function ToolsPage(): React.JSX.Element {
  const { t } = useTranslation();
  const navigate = useNavigate();
  const [unavailable, setUnavailable] = React.useState<string | null>(null);

  return (
    <div className="h-full overflow-y-auto">
      <section className="mx-auto w-full max-w-5xl p-6">
        <h1 className="text-xl font-semibold">{t('tools.title')}</h1>
        <p className="mt-1 max-w-2xl text-sm text-foreground-secondary">
          {t('tools.subtitle')}
        </p>
        <div className="mt-7 grid gap-4 sm:grid-cols-2 xl:grid-cols-3">
          {TOOLS.map(({ route, icon: Icon, title, body, available }) => (
            <article
              key={title}
              className="flex min-h-52 flex-col rounded-lg border border-border bg-surface p-5 shadow-card transition-shadow hover:shadow-overlay"
            >
              <div className="flex size-11 items-center justify-center rounded-md bg-primary-muted text-primary">
                <Icon className="size-5" strokeWidth={1.6} aria-hidden />
              </div>
              <h2 className="mt-5 text-base font-semibold">{t(`tools.${title}`)}</h2>
              <p className="mt-2 text-sm leading-relaxed text-foreground-secondary">
                {t(`tools.${body}`)}
              </p>
              <Button
                className="mt-auto self-start"
                variant="secondary"
                size="sm"
                onClick={() =>
                  available && route ? void navigate(route) : setUnavailable(title)
                }
              >
                {available ? t('tools.openTool') : t('tools.requirements')}
              </Button>
            </article>
          ))}
        </div>
      </section>
      <Dialog
        open={unavailable !== null}
        onOpenChange={(open) => !open && setUnavailable(null)}
      >
        <DialogContent className="max-w-md">
          <DialogHeader>
            <DialogTitle>{t('tools.providerTitle')}</DialogTitle>
            <DialogDescription>
              {unavailable ? t(`tools.${unavailable}`) : ''}
            </DialogDescription>
          </DialogHeader>
          <p className="text-sm text-foreground-secondary">{t('tools.providerBody')}</p>
          <DialogFooter>
            <Button variant="secondary" onClick={() => setUnavailable(null)}>
              {t('common.close')}
            </Button>
          </DialogFooter>
        </DialogContent>
      </Dialog>
    </div>
  );
}
