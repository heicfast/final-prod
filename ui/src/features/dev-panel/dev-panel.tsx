import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { FlaskConicalIcon, GaugeIcon, XIcon } from 'lucide-react';
import { useUiStore } from '@/stores/ui-store';
import { Button } from '@/components/ui/button';
import { Label } from '@/components/ui/label';
import { useTheme } from '@/app/theme';
import { setLocale } from '@/i18n';

/**
 * Developer panel — dev builds only (import.meta.env.DEV gated at the mount
 * site in the shell). The scenario/seed controls left with the simulated
 * backend; the legitimate dev tools remain: language and density overrides,
 * performance overlay. (docs/INTEGRATION-PLAN.md section 5.1)
 */

export function DevPanel(): React.JSX.Element {
  const { t } = useTranslation();
  const ui = useUiStore();
  const { density, setDensity } = useTheme();

  if (!ui.devPanelOpen) return <></>;

  return (
    <aside
      className="fixed bottom-4 right-4 z-[60] w-72 rounded-lg border border-border bg-surface p-4 shadow-dialog"
      aria-label={t('devPanel.title')}
      data-testid="dev-panel"
    >
      <div className="mb-3 flex items-center justify-between">
        <span className="flex items-center gap-1.5 text-sm font-semibold">
          <FlaskConicalIcon className="size-4 text-primary" aria-hidden />
          {t('devPanel.title')}
        </span>
        <Button
          variant="ghost"
          size="icon"
          className="size-6"
          onClick={() => ui.setDevPanelOpen(false)}
          aria-label={t('devPanel.closePanel')}
        >
          <XIcon aria-hidden />
        </Button>
      </div>

      <div className="space-y-3 text-sm">
        <div className="space-y-1">
          <Label>{t('devPanel.language')}</Label>
          <div className="flex gap-2">
            <Button variant="ghost" size="sm" onClick={() => setLocale('en')}>
              EN
            </Button>
            <Button variant="ghost" size="sm" onClick={() => setLocale('zh-CN')}>
              中文
            </Button>
            <Button
              variant="ghost"
              size="sm"
              onClick={() => setDensity(density === 'comfortable' ? 'compact' : 'comfortable')}
            >
              {density === 'comfortable' ? '→ compact' : '→ comfortable'}
            </Button>
          </div>
        </div>

        <div className="space-y-1">
          <Label>{t('devPanel.perfOverlay')}</Label>
          <Button
            variant={ui.perfOverlayVisible ? 'primary' : 'secondary'}
            size="sm"
            onClick={() => ui.setPerfOverlay(!ui.perfOverlayVisible)}
          >
            <GaugeIcon aria-hidden />
            {ui.perfOverlayVisible ? 'ON' : 'OFF'}
          </Button>
        </div>
      </div>
    </aside>
  );
}

/** FPS overlay (dev only) — rAF-based meter for grid #35/#38 eyeballing. */
export function PerfOverlay(): React.JSX.Element | null {
  const visible = useUiStore((s) => s.perfOverlayVisible);
  const [fps, setFps] = React.useState(0);
  const [longTasks, setLongTasks] = React.useState(0);

  React.useEffect(() => {
    if (!visible) return;
    let frames = 0;
    let last = performance.now();
    let raf = 0;
    const loop = (): void => {
      frames++;
      const now = performance.now();
      if (now - last >= 1000) {
        setFps(Math.round((frames * 1000) / (now - last)));
        frames = 0;
        last = now;
      }
      raf = requestAnimationFrame(loop);
    };
    raf = requestAnimationFrame(loop);
    let obs: PerformanceObserver | null = null;
    try {
      obs = new PerformanceObserver((list) => {
        setLongTasks((n) => n + list.getEntries().length);
      });
      obs.observe({ type: 'longtask', buffered: false });
    } catch {
      /* unsupported */
    }
    return () => {
      cancelAnimationFrame(raf);
      obs?.disconnect();
    };
  }, [visible]);

  if (!visible) return null;
  return (
    <div
      className="fixed left-4 bottom-4 z-[60] rounded-md border border-border bg-surface/95 px-3 py-2 font-mono text-2xs shadow-overlay"
      aria-hidden
      data-testid="perf-overlay"
    >
      <div>{fps} fps</div>
      <div>long tasks: {longTasks}</div>
    </div>
  );
}
