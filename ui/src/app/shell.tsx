import * as React from 'react';
import { NavLink, Outlet, useLocation, useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import {
  AppWindowIcon,
  CrosshairIcon,
  HardDriveIcon,
  HeartPulseIcon,
  InfoIcon,
  LanguagesIcon,
  RefreshCcwIcon,
  SettingsIcon,
  UsbIcon,
  WrenchIcon,
} from 'lucide-react';
import { cn } from '@/lib/utils';
import { useTheme } from './theme';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { Button } from '@/components/ui/button';
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from '@/components/ui/menus';
import { setLocale, SUPPORTED_LOCALES } from '@/i18n';
import { connectorMode } from '@/connector';
import { toast } from 'sonner';

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

export function WizardShell(): React.JSX.Element {
  const { t } = useTranslation();
  const { setMode } = useTheme();
  const active = useScanSessionStore((s) => s.active);
  const navigate = useNavigate();
  const location = useLocation();

  const inResultsFlow =
    location.pathname.startsWith('/scan/') || location.pathname.startsWith('/results') || location.pathname.startsWith('/recover');

  return (
    <div className="flex h-full min-h-0 flex-col bg-background text-foreground">
      {/* Title bar */}
      <header
        className="flex h-9 shrink-0 items-center justify-between border-b border-border bg-surface px-3 select-none"
        style={{ height: 'var(--titlebar-height)' }}
        data-tauri-drag-region
      >
        <div className="flex items-center gap-2">
          <AppWindowIcon className="size-4 text-primary" aria-hidden />
          <span className="text-sm font-semibold">{t('app.name')}</span>
          <span className="rounded-full border border-primary-muted-border bg-primary-muted px-1.5 text-2xs font-semibold text-primary uppercase">
            {connectorMode() === 'mock' ? t('settings.connectorMock') : t('settings.connectorTauri')}
          </span>
        </div>
        <div className="flex items-center gap-1">
          <DropdownMenu>
            <DropdownMenuTrigger asChild>
              <Button variant="ghost" size="icon" aria-label={t('settings.language')} className="size-7">
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
              <Button variant="ghost" size="icon" aria-label={t('settings.theme')} className="size-7">
                <SettingsIcon aria-hidden />
              </Button>
            </DropdownMenuTrigger>
            <DropdownMenuContent align="end">
              <DropdownMenuItem onClick={() => setMode('system')}>{t('settings.themeSystem')}</DropdownMenuItem>
              <DropdownMenuItem onClick={() => setMode('light')}>{t('settings.themeLight')}</DropdownMenuItem>
              <DropdownMenuItem onClick={() => setMode('dark')}>{t('settings.themeDark')}</DropdownMenuItem>
              <DropdownMenuSeparator />
              <DropdownMenuItem onClick={() => setMode('high-contrast')}>{t('settings.themeHc')}</DropdownMenuItem>
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
            onClick={() => toast(t('common.learnMore'))}
          >
            <InfoIcon aria-hidden />
          </Button>
        </div>
      </header>

      <div className="flex min-h-0 flex-1">
        {/* Sidebar */}
        <nav aria-label={t('a11y.sidebarRegion')} className="flex w-[var(--sidebar-width)] shrink-0 flex-col overflow-y-auto border-r border-border bg-surface-muted/60 py-2">
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
              <SidebarSection label={t('nav.settings')} />
              <SidebarLink to="/settings" icon={SettingsIcon} labelKey="nav.settings" end />
            </>
          )}
          <div className="mt-auto space-y-0.5">
            <SidebarLink to="/settings" icon={SettingsIcon} labelKey="nav.settings" end />
          </div>
        </nav>

        {/* Content */}
        <main id="main-content" aria-label={t('a11y.mainRegion')} className="min-h-0 min-w-0 flex-1 overflow-hidden">
          <Outlet />
        </main>
      </div>
    </div>
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
