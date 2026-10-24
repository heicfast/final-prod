import { DatabaseZapIcon, HardDriveIcon, ScanSearchIcon, ShieldCheckIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';

export type SplashPhase = 'booting' | 'enumerating';

/**
 * The first two reference frames are a sequential boot surface: a calm first
 * paint followed by actual device enumeration, never simulated recovery work.
 */
export function SplashScreen({ phase }: { phase: SplashPhase }): React.JSX.Element {
  const { t } = useTranslation();
  const enumerating = phase === 'enumerating';
  return (
    <main
      className="relative grid min-h-screen place-items-center overflow-hidden bg-[#eaf4ff] p-6 text-foreground"
      aria-busy="true"
      aria-live="polite"
      aria-label={enumerating ? t('splash.loading') : t('splash.booting')}
    >
      <div className="pointer-events-none absolute inset-0 overflow-hidden" aria-hidden>
        <div className="absolute left-[14%] top-[12%] size-72 rounded-full bg-white/55 blur-3xl" />
        <div className="absolute bottom-[8%] right-[14%] size-80 rounded-full bg-[#cfe5ff] blur-3xl" />
      </div>
      <section className="relative w-full max-w-sm rounded-lg border border-white/90 bg-white/78 p-6 shadow-dialog backdrop-blur">
        <div className="relative mx-auto mb-5 h-28 w-44" aria-hidden>
          <div className="absolute left-1/2 top-2 h-16 w-28 -translate-x-1/2 -skew-y-[8deg] rounded-lg border border-primary/20 bg-white shadow-card" />
          <div className="absolute left-1/2 top-5 grid size-12 -translate-x-1/2 place-items-center rounded-md bg-primary text-primary-foreground shadow-overlay">
            <HardDriveIcon className="size-6" strokeWidth={1.6} />
          </div>
          <div className="absolute bottom-0 left-2 grid size-10 place-items-center rounded-md border border-primary/20 bg-[#e9f3ff] text-primary shadow-card">
            <DatabaseZapIcon className="size-5" />
          </div>
          <div className="absolute bottom-1 right-2 grid size-10 place-items-center rounded-md border border-primary/20 bg-white text-primary shadow-card">
            <ScanSearchIcon className={enumerating ? 'size-5 animate-pulse' : 'size-5'} />
          </div>
          <span className="absolute bottom-5 left-12 right-12 h-px bg-primary/30" />
        </div>
        <h1 className="text-center text-base font-semibold">{t('app.name')}</h1>
        <p className="mt-1 text-center text-sm text-foreground-secondary">
          {enumerating ? t('splash.loading') : t('splash.booting')}
        </p>
        <div className="mt-5 h-1 overflow-hidden rounded-full bg-primary/10" aria-hidden>
          <div
            className={
              enumerating
                ? 'h-full w-2/3 rounded-full bg-primary animate-[progress-stripes_1.1s_linear_infinite]'
                : 'h-full w-1/3 rounded-full bg-primary transition-all duration-300'
            }
          />
        </div>
        <p className="mt-4 flex items-center justify-center gap-1.5 text-xs text-foreground-secondary">
          <ShieldCheckIcon className="size-3.5 text-success" aria-hidden />
          {t('splash.safety')}
        </p>
      </section>
    </main>
  );
}
