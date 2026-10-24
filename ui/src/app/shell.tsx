import * as React from 'react';
import { NavLink, Outlet, useLocation, useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import { getCurrentWindow } from '@tauri-apps/api/window';
import {
  AppWindowIcon,
  CrosshairIcon,
  FlaskConicalIcon,
  HardDriveIcon,
  HeartPulseIcon,
  InfoIcon,
  LanguagesIcon,
  PaletteIcon,
  PanelLeftCloseIcon,
  PanelLeftOpenIcon,
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
import { WindowControls } from '@/components/ui/window-controls';
import { isTauriRuntime } from '@/lib/runtime';

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
  const sidebarCollapsed = useUiStore((s) => s.sidebarCollapsed);
  const toggleSidebar = useUiStore((s) => s.toggleSidebar);

  const inResultsFlow =
    location.pathname.startsWith('/scan/') ||
    location.pathname.startsWith('/results') ||
    location.pathname.startsWith('/recover');

  /** Windows convention on custom chrome: double-click the title-bar drag
   *  region to toggle maximize/restore (corpus: frameless Qt shell; the OS
   *  provides snap/Aero for drag). Ignored over interactive children. */
  const onTitleBarDoubleClick = (e: React.MouseEvent<HTMLElement>): void => {
    if (!isTauriRuntime()) return;
    if ((e.target as HTMLElement).closest('button, a, input, [role="menu"]')) return;
    void getCurrentWindow().toggleMaximize();
  };

  return (
    <div className="flex h-full min-h-0 flex-col bg-background text-foreground">
      {/* Title bar */}
      <header
        className="flex shrink-0 items-center justify-between border-b border-border bg-surface px-3 select-none"
        style={{ height: 'var(--titlebar-height)' }}
        data-tauri-drag-region
        onDoubleClick={onTitleBarDoubleClick}
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
          <WindowControls />
        </div>
      </header>

      <div className="flex min-h-0 flex-1">
        {/* Sidebar — collapsible (session mandate §2.1; app extension, the
            corpus rail is fixed-width): width animates at --dur-slow while
            labels fade at --dur-fast; icons keep their gutter so they land
            centered on the 52px collapsed rail. State persists. */}
        <nav
          id="sidebar-nav"
          aria-label={t('a11y.sidebarRegion')}
          style={{ width: sidebarCollapsed ? 52 : undefined }}
          className={cn(
            'flex shrink-0 flex-col overflow-x-hidden overflow-y-auto border-r border-border bg-surface-muted/60 py-2',
            !sidebarCollapsed && 'w-[var(--sidebar-width)]',
            'sidebar-width-transition',
          )}
        >
          <div className="mb-1 flex items-center justify-end px-2">
            <button
              type="button"
              onClick={toggleSidebar}
              aria-expanded={!sidebarCollapsed}
              aria-controls="sidebar-nav"
              aria-label={sidebarCollapsed ? t('sidebar.expand') : t('sidebar.collapse')}
              data-testid="sidebar-toggle"
              className="flex size-8 items-center justify-center rounded-sm text-foreground-secondary outline-none transition-colors hover:bg-surface-hover hover:text-foreground focus-visible:ring-2 focus-visible:ring-ring"
            >
              {sidebarCollapsed ? (
                <PanelLeftOpenIcon className="size-4" aria-hidden />
              ) : (
                <PanelLeftCloseIcon className="size-4" aria-hidden />
              )}
            </button>
          </div>
          {!inResultsFlow ? (
            <>
              <SidebarSection label={t('sidebar.dataRecovery')} collapsed={sidebarCollapsed} />
              {NAV_MAIN.map((item) => (
                <SidebarLink key={item.to} {...item} collapsed={sidebarCollapsed} />
              ))}
              <SidebarSection label={t('sidebar.advanced')} collapsed={sidebarCollapsed} />
              {NAV_ADVANCED.map((item) => (
                <SidebarLink key={item.to} {...item} collapsed={sidebarCollapsed} />
              ))}
            </>
          ) : (
            <>
              <SidebarSection label={t('sidebar.dataRecovery')} collapsed={sidebarCollapsed} />
              <SidebarLink
                to="/"
                icon={HardDriveIcon}
                labelKey="nav.backToDevices"
                end
                collapsed={sidebarCollapsed}
              />
              {active ? (
                <button
                  type="button"
                  onClick={() => navigate(`/scan/${active.targetId}/live`)}
                  title={t('home.scanningHint')}
                  className={cn(
                    'mx-2 flex h-8 items-center gap-2.5 rounded-sm px-2.5 text-sm text-primary hover:bg-surface-hover',
                  )}
                >
                  {/* Liveness pulses on the dot only — animating text opacity
                      transiently breaks its contrast ratio (axe-verified). */}
                  <span
                    className="inline-block size-1.5 shrink-0 rounded-full bg-current animate-status-pulse"
                    aria-hidden
                  />
                  <RefreshCcwIcon className="size-4 shrink-0" aria-hidden />
                  <span
                    className={cn(
                      'sidebar-label-fade truncate',
                      sidebarCollapsed && 'opacity-0',
                    )}
                  >
                    {t('home.scanningHint')}
                  </span>
                </button>
              ) : null}
            </>
          )}
          <div className="mt-auto space-y-0.5">
            <SidebarLink
              to="/settings"
              icon={SettingsIcon}
              labelKey="nav.settings"
              end
              collapsed={sidebarCollapsed}
            />
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

function SidebarSection({
  label,
  collapsed,
}: {
  label: string;
  collapsed: boolean;
}): React.JSX.Element {
  return (
    <div
      className={cn(
        'px-4 pb-1 pt-4 text-2xs font-semibold uppercase tracking-wide text-foreground-secondary',
        'sidebar-label-fade',
        collapsed && 'opacity-0',
      )}
    >
      {label}
    </div>
  );
}

function SidebarLink({
  to,
  icon: Icon,
  labelKey,
  end,
  collapsed,
}: {
  to: string;
  icon: React.ComponentType<{ className?: string }>;
  labelKey: string;
  end?: boolean;
  collapsed: boolean;
}): React.JSX.Element {
  const { t } = useTranslation();
  return (
    <NavLink
      to={to}
      end={end}
      title={collapsed ? t(labelKey) : undefined}
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
      <span
        className={cn(
          'sidebar-label-fade truncate',
          collapsed && 'opacity-0 pointer-events-none',
        )}
      >
        {t(labelKey)}
      </span>
    </NavLink>
  );
}
