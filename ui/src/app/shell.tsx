import * as React from 'react';
import { NavLink, Outlet, useLocation, useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import {
  AppWindowIcon,
  CrosshairIcon,
  FlaskConicalIcon,
  HardDriveIcon,
  HeartPulseIcon,
  InfoIcon,
  LanguagesIcon,
  PaletteIcon,
  RefreshCcwIcon,
  SettingsIcon,
  UsbIcon,
  WrenchIcon,
} from 'lucide-react';
import { cn } from '@/lib/utils';
import { useTheme, type ThemeMode } from './theme';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { useUiStore } from '@/stores/ui-store';
import { DevPanel, PerfOverlay } from '@/features/dev-panel/dev-panel';
import { Button } from '@/components/ui/button';
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from '@/components/ui/menus';
import { AboutDialog } from '@/features/settings-about/about-dialog';
import { useConnectorMode } from '@/queries/dev';
import { setLocale, SUPPORTED_LOCALES } from '@/i18n';

/**
 * App shell: custom title bar (frameless window), left sidebar with the
 * wizard's information architecture, and the routed content area.
 */

const NAV_MAIN = [
  { to: '/', icon: HardDriveIcon, labelKey: 'sidebar.hardwareDisk', end: true },
  { to: '/?media=1', icon: UsbIcon, labelKey: 'sidebar.removable', end: false },
  { to: '/lost-partition', icon: CrosshairIcon, labelKey: 'sidebar.lostPartition', end: false },
] as const;

const NAV_ADVANCED = [
  { to: '/health', icon: HeartPulseIcon, labelKey: 'sidebar.diskHealth', end: false },
  { to: '/repair', icon: WrenchIcon, labelKey: 'sidebar.volumeRepair', end: false },
] as const;

/** Title-bar quick theme menu (corpus: hamburger → Theme submenu; the
 *  adopted palette set from ADR-007 replaces the reference's Light/Dark pair). */
const THEME_MENU_ITEMS: readonly { mode: ThemeMode; labelKey: string }[] = [
  { mode: 'system', labelKey: 'settings.themeSystem' },
  { mode: 'light', labelKey: 'settings.themeLight' },
  { mode: 'dark', labelKey: 'settings.themeDark' },
  { mode: 'ember', labelKey: 'settings.themeEmber' },
  { mode: 'tide', labelKey: 'settings.themeTide' },
  { mode: 'blossom', labelKey: 'settings.themeBlossom' },
  { mode: 'high-contrast', labelKey: 'settings.themeHc' },
];

export function WizardShell(): React.JSX.Element {
  const { t } = useTranslation();
  const { mode, setMode } = useTheme();
  const connectorModeValue = useConnectorMode();
  const active = useScanSessionStore((s) => s.active);
  const navigate = useNavigate();
  const location = useLocation();
  const [aboutOpen, setAboutOpen] = React.useState(false);

  const inResultsFlow =
    location.pathname.startsWith('/scan/') ||
    location.pathname.startsWith('/results') ||
    location.pathname.startsWith('/recover');

  return (
    <div className="flex h-full min-h-0 flex-col bg-background text-foreground">
      {/* Title bar */}
      <header
        className="flex shrink-0 items-center justify-between border-b border-border bg-surface px-3 select-none"
        style={{ height: 'var(--titlebar-height)' }}
        data-tauri-drag-region
      >
        <div className="flex items-center gap-2">
          <AppWindowIcon className="size-4 text-primary" aria-hidden />
          <span className="text-sm font-semibold">{t('app.name')}</span>
          <span className="rounded-full border border-primary-muted-border bg-primary-muted px-1.5 text-2xs font-semibold text-primary uppercase">
            {connectorModeValue === 'mock'
              ? t('settings.connectorMock')
              : connectorModeValue === 'tauri'
                ? t('settings.connectorTauri')
                : '…'}
          </span>
        </div>
        <div className="flex items-center gap-1">
          {import.meta.env.DEV ? (
            <Button
              variant="ghost"
              size="icon"
              aria-label={t('devPanel.openPanel')}
              className="size-7"
              onClick={() => useUiStore.getState().toggleDevPanel()}
              data-testid="dev-panel-toggle"
            >
              <FlaskConicalIcon aria-hidden />
            </Button>
          ) : null}
          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <Button
                variant="ghost"
                size="icon"
                aria-label={t('settings.language')}
                className="size-7"
              >
                <LanguagesIcon aria-hidden />
              </Button>
            </DropdownMenuTrigger>
            <DropdownMenuContent align="end">
              {SUPPORTED_LOCALES.map((loc) => (
                <DropdownMenuItem key={loc} onClick={() => setLocale(loc)}>
                  {loc === 'en' ? 'English' : '简体中文'}
                </DropdownMenuItem>
              ))}
            </DropdownMenuContent>
          </DropdownMenu>
          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <Button
                variant="ghost"
                size="icon"
                aria-label={t('settings.theme')}
                className="size-7"
              >
                <PaletteIcon aria-hidden />
              </Button>
            </DropdownMenuTrigger>
            <DropdownMenuContent align="end">
              {THEME_MENU_ITEMS.map((item) => (
                <React.Fragment key={item.mode}>
                  {item.mode === 'high-contrast' ? <DropdownMenuSeparator /> : null}
                  <DropdownMenuItem
                    onClick={() => setMode(item.mode)}
                    aria-checked={mode === item.mode}
                  >
                    {t(item.labelKey)}
                  </DropdownMenuItem>
                </React.Fragment>
              ))}
            </DropdownMenuContent>
          </DropdownMenu>
          <Button
            variant="ghost"
            size="icon"
            aria-label={t('nav.settings')}
            className="size-7"
            onClick={() => navigate('/settings')}
          >
            <SettingsIcon aria-hidden />
          </Button>
          <Button
            variant="ghost"
            size="icon"
            aria-label={t('nav.about')}
            className="size-7"
            onClick={() => setAboutOpen(true)}
          >
            <InfoIcon aria-hidden />
          </Button>
        </div>
      </header>

      <div className="flex min-h-0 flex-1">
        {/* Sidebar */}
        <nav
          aria-label={t('a11y.sidebarRegion')}
          className="flex w-[var(--sidebar-width)] shrink-0 flex-col overflow-y-auto border-r border-border bg-surface-muted/60 py-2"
        >
          {!inResultsFlow ? (
            <>
              <SidebarSection label={t('sidebar.dataRecovery')} />
              {NAV_MAIN.map((item) => (
                <SidebarLink key={item.to} {...item} />
              ))}
              <SidebarSection label={t('sidebar.advanced')} />
              {NAV_ADVANCED.map((item) => (
                <SidebarLink key={item.to} {...item} />
              ))}
            </>
          ) : (
            <>
              <SidebarSection label={t('sidebar.dataRecovery')} />
              <SidebarLink to="/" icon={HardDriveIcon} labelKey="nav.backToDevices" end />
              {active ? (
                <button
                  type="button"
                  onClick={() => navigate(`/scan/${active.targetId}/live`)}
                  className={cn(
                    'mx-2 flex h-8 items-center gap-2 rounded-sm px-2.5 text-sm text-primary hover:bg-surface-hover',
                    'animate-status-pulse',
                  )}
                >
                  <RefreshCcwIcon className="size-4" aria-hidden />
                  {t('home.scanningHint')}
                </button>
              ) : null}
            </>
          )}
          <div className="mt-auto space-y-0.5">
            <SidebarLink to="/settings" icon={SettingsIcon} labelKey="nav.settings" end />
          </div>
        </nav>

        {/* Content */}
        <main
          id="main-content"
          aria-label={t('a11y.mainRegion')}
          className="min-h-0 min-w-0 flex-1 overflow-hidden"
        >
          <Outlet />
        </main>
      </div>

      {/* Dev-only surfaces: lazy-gated so production builds never include them */}
      {import.meta.env.DEV ? <DevSurfaces /> : null}
      <AboutDialog open={aboutOpen} onOpenChange={setAboutOpen} />
    </div>
  );
}

function DevSurfaces(): React.JSX.Element {
  return (
    <>
      <DevPanel />
      <PerfOverlay />
    </>
  );
}

function SidebarSection({ label }: { label: string }): React.JSX.Element {
  return (
    <div className="px-4 pb-1 pt-4 text-2xs font-semibold uppercase tracking-wide text-foreground-disabled">
      {label}
    </div>
  );
}

function SidebarLink({
  to,
  icon: Icon,
  labelKey,
  end,
}: {
  to: string;
  icon: React.ComponentType<{ className?: string }>;
  labelKey: string;
  end?: boolean;
}): React.JSX.Element {
  const { t } = useTranslation();
  return (
    <NavLink
      to={to}
      end={end}
      className={({ isActive }) =>
        cn(
          'mx-2 flex h-8 items-center gap-2.5 rounded-sm px-2.5 text-sm outline-none transition-colors',
          'focus-visible:ring-2 focus-visible:ring-ring',
          isActive
            ? 'bg-primary-muted font-medium text-primary'
            : 'text-foreground-secondary hover:bg-surface-hover hover:text-foreground',
        )
      }
    >
      <Icon className="size-4 shrink-0" aria-hidden />
      <span className="truncate">{t(labelKey)}</span>
    </NavLink>
  );
}
