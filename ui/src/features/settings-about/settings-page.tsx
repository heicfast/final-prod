import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { useTheme } from '@/app/theme';
import { useAppInfo, useDriverStatus } from '@/queries/system';
import { getLogBuffer } from '@/lib/logger';
import { setLocale, SUPPORTED_LOCALES } from '@/i18n';
import { Badge } from '@/components/ui/badge';
import { Label } from '@/components/ui/label';
import { Segmented } from '@/components/ui/stepper';
import { Table, TableBody, TableCell, TableRow } from '@/components/ui/table';
import { Button } from '@/components/ui/button';
import { toast } from 'sonner';

export function SettingsPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const { mode, setMode, density, setDensity } = useTheme();
  const appInfo = useAppInfo();
  const driver = useDriverStatus();
  const locale = (SUPPORTED_LOCALES as readonly string[]).includes(i18n.language)
    ? (i18n.language as (typeof SUPPORTED_LOCALES)[number])
    : 'en';

  return (
    <div className="mx-auto w-full max-w-3xl overflow-y-auto p-6">
      <h1 className="mb-6 text-xl font-semibold">{t('settings.title')}</h1>

      <section className="mb-8" aria-labelledby="settings-appearance">
        <h2 id="settings-appearance" className="mb-3 text-base font-semibold">{t('settings.appearance')}</h2>
        <div className="space-y-4 rounded-lg border border-border bg-surface p-4">
          <div className="flex items-center justify-between gap-4">
            <Label>{t('settings.theme')}</Label>
            <Segmented
              ariaLabel={t('settings.theme')}
              value={mode}
              onChange={(v) => setMode(v)}
              options={[
                { value: 'system', label: t('settings.themeSystem') },
                { value: 'light', label: t('settings.themeLight') },
                { value: 'dark', label: t('settings.themeDark') },
                { value: 'high-contrast', label: t('settings.themeHc') },
              ]}
            />
          </div>
          <div className="flex items-center justify-between gap-4">
            <Label>{t('settings.density')}</Label>
            <Segmented
              ariaLabel={t('settings.density')}
              value={density}
              onChange={(v) => setDensity(v)}
              options={[
                { value: 'comfortable', label: t('settings.densityComfortable') },
                { value: 'compact', label: t('settings.densityCompact') },
              ]}
            />
          </div>
          <div className="flex items-center justify-between gap-4">
            <Label>{t('settings.language')}</Label>
            <Segmented
              ariaLabel={t('settings.language')}
              value={locale}
              onChange={(v) => setLocale(v)}
              options={[
                { value: 'en', label: 'English' },
                { value: 'zh-CN', label: '简体中文' },
              ]}
            />
          </div>
        </div>
      </section>

      <section className="mb-8" aria-labelledby="settings-diagnostics">
        <h2 id="settings-diagnostics" className="mb-3 text-base font-semibold">{t('settings.diagnostics')}</h2>
        <div className="rounded-lg border border-border bg-surface p-4">
          <p className="mb-2 text-xs text-foreground-secondary">
            {t('settings.diagnosticsIntro', { count: getLogBuffer().length })}
          </p>
          <Button
            variant="secondary"
            size="sm"
            onClick={() => {
              void navigator.clipboard
                ?.writeText(getLogBuffer().map((e) => `[${e.level}] ${e.scope}: ${e.message}`).join('\n'))
                .then(() => toast.success(t('common.copied')))
                .catch(() => toast.error(t('errors.unknown')));
            }}
          >
            {t('settings.copyLog')}
          </Button>
        </div>
      </section>

      <section aria-labelledby="settings-about">
        <h2 id="settings-about" className="mb-3 text-base font-semibold">{t('settings.aboutTitle')}</h2>
        <div className="overflow-hidden rounded-lg border border-border bg-surface shadow-card">
          <Table>
            <TableBody>
              <TableRow>
                <TableCell className="w-40 text-foreground-secondary">{t('settings.version')}</TableCell>
                <TableCell className="tabular">{appInfo.data?.version ?? '—'}</TableCell>
              </TableRow>
              <TableRow>
                <TableCell className="text-foreground-secondary">{t('settings.connectorMode')}</TableCell>
                <TableCell>
                  <Badge variant={appInfo.data?.connectorMode === 'mock' ? 'warning' : 'success'}>
                    {appInfo.data?.connectorMode === 'mock' ? t('settings.connectorMock') : t('settings.connectorTauri')}
                  </Badge>
                </TableCell>
              </TableRow>
              <TableRow>
                <TableCell className="text-foreground-secondary">{t('settings.engineStatus')}</TableCell>
                <TableCell className="tabular">
                  {driver.data ? `${driver.data.installed ? '✓' : '✕'} ${driver.data.version ?? '—'}${driver.data.running ? '' : ' (idle)'}` : '—'}
                </TableCell>
              </TableRow>
              <TableRow>
                <TableCell className="text-foreground-secondary">{t('settings.licenses')}</TableCell>
                <TableCell className="text-sm text-foreground-secondary">{t('settings.licensesBody')}</TableCell>
              </TableRow>
            </TableBody>
          </Table>
        </div>
      </section>
    </div>
  );
}
