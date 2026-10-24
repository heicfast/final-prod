import * as React from 'react';
import { useNavigate, useParams, useSearchParams } from 'react-router';
import { useTranslation } from 'react-i18next';
import { useForm } from 'react-hook-form';
import { zodResolver } from '@hookform/resolvers/zod';
import { z } from 'zod';
import type { BadSectorPolicy, FileCategoryId, ScanMode } from '@/connector/contract';
import {
  ArrowLeftIcon,
  CogIcon,
  FolderOpenIcon,
  GaugeIcon,
  RadarIcon,
  ScanSearchIcon,
  ShieldAlertIcon,
  ZapIcon,
} from 'lucide-react';
import { useDeviceInfo } from '@/queries/devices';
import { useStartScan } from '@/queries/scan';
import { usePickDirectory } from '@/queries/system';
import { formatBytes } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Label } from '@/components/ui/label';
import { RadioGroup, RadioGroupItem } from '@/components/ui/radio-group';
import { Alert } from '@/components/ui/alert';
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from '@/components/ui/select';
import { CategoryIcon } from '@/components/ui/file-icon';
import { CapacityBar, EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

const ALL_CATEGORIES: readonly FileCategoryId[] = [
  'graphics',
  'video',
  'audio',
  'document',
  'email',
  'archive',
  'bookmarks',
  'other',
];

const schema = z.object({
  mode: z.enum(['quick', 'deep', 'deep-carve']),
  categories: z.array(
    z.enum([
      'graphics',
      'video',
      'audio',
      'document',
      'email',
      'archive',
      'bookmarks',
      'other',
    ]),
  ),
  scopeMode: z.enum(['volume', 'folder']),
  pathScope: z.string(),
  sectorSize: z.enum(['auto', '512', '4096']),
  badSectorPolicy: z.enum(['stop', 'skip', 'zero-fill']),
  retries: z.number().int().min(0).max(10),
});

type FormValues = z.infer<typeof schema>;

export function ScanConfigPage(): React.JSX.Element {
  const { deviceId = '' } = useParams();
  const [params] = useSearchParams();
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const deviceQuery = useDeviceInfo(deviceId);

  const startScan = useStartScan();
  const pickDirectory = usePickDirectory();

  const form = useForm<FormValues>({
    resolver: zodResolver(schema),
    defaultValues: {
      mode: 'deep',
      categories: [...ALL_CATEGORIES],
      // ?folder=1 (Home → “choose folder” quick target) preselects the
      // folder mode so the user only needs to pick the folder.
      scopeMode: params.has('folder') ? 'folder' : 'volume',
      pathScope: '',
      sectorSize: 'auto',
      badSectorPolicy: 'skip',
      retries: 3,
    },
  });

  const values = form.watch();
  const device = deviceQuery.data ?? null;
  const volume =
    device?.volumes.find((v) => v.id === deviceId) ??
    (device && device.volumes.length === 1 ? device.volumes[0] : null);
  const quickBlocked = volume ? !volume.quickScanAvailable : false;

  const onSubmit = (v: FormValues): void => {
    const targetId = volume?.id ?? device?.id ?? deviceId;
    startScan.mutate(
      {
        targetId,
        mode: v.mode,
        categories: v.categories,
        ...(v.scopeMode === 'folder' && v.pathScope.trim() !== ''
          ? { pathScope: v.pathScope.trim() }
          : {}),
        badSectorPolicy: v.badSectorPolicy,
        retries: v.retries,
        ...(v.sectorSize !== 'auto' ? { sectorSize: Number(v.sectorSize) } : {}),
      },
      {
        onSuccess: ({ scanId }) => {
          void navigate(`/scan/${targetId}/live?scanId=${scanId}`);
        },
        onError: (err) => {
          const reasonKey = (err as { reasonKey?: string }).reasonKey;
          toast.error(reasonKey ? t(reasonKey) : err.message);
        },
      },
    );
  };

  if (deviceQuery.isLoading) {
    return (
      <div className="p-6">
        <Skeleton className="mb-6 h-24" />
        <Skeleton className="mb-6 h-40" />
        <Skeleton className="h-56" />
      </div>
    );
  }

  if (deviceQuery.isError || !device) {
    return (
      <div className="p-6">
        <EmptyState
          icon={ShieldAlertIcon}
          title={t('errors.title')}
          description={
            deviceQuery.error instanceof Error
              ? deviceQuery.error.message
              : t('errors.deviceGone')
          }
          actionLabel={t('common.retry')}
          onAction={() => void deviceQuery.refetch()}
          secondaryLabel={t('nav.backToDevices')}
          onSecondary={() => navigate('/')}
        />
      </div>
    );
  }

  return (
    <form
      onSubmit={form.handleSubmit(onSubmit)}
      className="flex h-full min-h-0 flex-col overflow-y-auto"
    >
      <div className="mx-auto w-full max-w-4xl flex-1 p-6">
        <div className="mb-6 flex items-center justify-between gap-4">
          <div className="flex items-center gap-3">
            <Button
              type="button"
              variant="ghost"
              size="icon"
              onClick={() => navigate('/')}
              aria-label={t('common.back')}
            >
              <ArrowLeftIcon aria-hidden />
            </Button>
            <div>
              <h1 className="text-xl font-semibold">{t('scanConfig.title')}</h1>
              <p className="mt-0.5 text-sm text-foreground-secondary">
                {t('scanConfig.deviceSummary')}: {device.label}
              </p>
            </div>
          </div>
        </div>

        {/* Device summary */}
        <div className="mb-6 flex items-center gap-4 rounded-lg border border-border bg-surface p-4 shadow-card">
          <div className="flex size-12 items-center justify-center rounded-md bg-primary-muted text-primary">
            <ScanSearchIcon className="size-6" strokeWidth={1.5} aria-hidden />
          </div>
          <div className="min-w-0 flex-1">
            <div className="text-base font-semibold">{volume?.label ?? device.label}</div>
            <div className="text-xs text-foreground-secondary">
              {(volume?.fs ?? 'raw').toUpperCase()} ·{' '}
              {volume
                ? formatBytes(volume.capacity.totalBytes, i18n.language)
                : formatBytes(device.capacity.totalBytes, i18n.language)}
            </div>
          </div>
          <div className="w-40">
            <CapacityBar
              usedBytes={(volume ?? device).capacity.usedBytes}
              totalBytes={(volume ?? device).capacity.totalBytes}
            />
          </div>
        </div>

        <h2 className="mb-3 text-base font-semibold">{t('scanConfig.modeTitle')}</h2>
        <RadioGroup
          value={values.mode}
          onValueChange={(v) => form.setValue('mode', v as ScanMode)}
          className="grid grid-cols-1 gap-3 md:grid-cols-3"
        >
          <ModeCard
            icon={ZapIcon}
            value="quick"
            title={t('scanConfig.quick.title')}
            body={t('scanConfig.quick.body')}
            disabled={quickBlocked}
            disabledReason={quickBlocked ? t('scanConfig.quickUnavailable') : undefined}
            current={values.mode}
          />
          <ModeCard
            icon={GaugeIcon}
            value="deep"
            title={t('scanConfig.deep.title')}
            body={t('scanConfig.deep.body')}
            current={values.mode}
          />
          <ModeCard
            icon={RadarIcon}
            value="deep-carve"
            title={t('scanConfig.carve.title')}
            body={t('scanConfig.carve.body')}
            current={values.mode}
          />
        </RadioGroup>

        {quickBlocked ? (
          <Alert variant="warning" className="mt-3">
            <p>{t('scanConfig.quickUnavailable')}</p>
          </Alert>
        ) : null}

        {/* Scan location — corpus “Specify a location”: whole drive or a
            specific folder (validated by the connector at start time). */}
        {volume ? (
          <>
            <h2 className="mb-3 mt-8 text-base font-semibold">{t('scanConfig.scopeTitle')}</h2>
            <div className="rounded-lg border border-border bg-surface p-4">
              <RadioGroup
                value={values.scopeMode}
                onValueChange={(v) => form.setValue('scopeMode', v as 'volume' | 'folder')}
                className="flex flex-wrap gap-4"
              >
                <label className="flex cursor-pointer items-center gap-2 text-sm">
                  <RadioGroupItem value="volume" />
                  {t('scanConfig.scopeVolume')}
                </label>
                <label className="flex cursor-pointer items-center gap-2 text-sm">
                  <RadioGroupItem value="folder" />
                  {t('scanConfig.scopeFolder')}
                </label>
              </RadioGroup>
              {values.scopeMode === 'folder' ? (
                <div className="mt-3 space-y-1.5">
                  <Label htmlFor="path-scope">{t('scanConfig.scopeFolderLabel')}</Label>
                  <div className="grid grid-cols-[1fr_auto] items-start gap-2">
                    <Input
                      id="path-scope"
                      value={values.pathScope}
                      placeholder={t('scanConfig.scopePlaceholder')}
                      onChange={(e) => form.setValue('pathScope', e.target.value)}
                      className="font-mono text-xs"
                      aria-describedby="path-scope-hint"
                    />
                    <Button
                      type="button"
                      variant="secondary"
                      loading={pickDirectory.isPending}
                      disabled={pickDirectory.isPending}
                      onClick={() => {
                        pickDirectory.mutate(
                          { volumeId: volume.id },
                          {
                            onSuccess: (picked) => {
                              if (picked) form.setValue('pathScope', picked);
                            },
                            onError: (err) => toast.error(err.message),
                          },
                        );
                      }}
                    >
                      <FolderOpenIcon aria-hidden />
                      {t('scanConfig.scopeBrowse')}
                    </Button>
                  </div>
                  <p id="path-scope-hint" className="text-xs text-foreground-secondary">
                    {t('scanConfig.scopeHint')}
                  </p>
                </div>
              ) : null}
            </div>
          </>
        ) : null}

        <h2 className="mb-3 mt-8 text-base font-semibold">{t('scanConfig.typesTitle')}</h2>
        <div className="grid grid-cols-2 gap-2 md:grid-cols-4">
          {ALL_CATEGORIES.map((cat) => {
            const checked = values.categories.includes(cat);
            return (
              <label
                key={cat}
                className={cn(
                  'flex cursor-pointer items-center gap-2.5 rounded-md border p-3 text-sm transition-colors',
                  'focus-within:ring-2 focus-within:ring-ring',
                  checked
                    ? 'border-primary bg-primary-muted'
                    : 'border-border bg-surface hover:bg-surface-hover',
                )}
              >
                <input
                  type="checkbox"
                  className="sr-only"
                  checked={checked}
                  onChange={() => {
                    const next = checked
                      ? values.categories.filter((c) => c !== cat)
                      : [...values.categories, cat];
                    form.setValue('categories', next);
                  }}
                />
                <CategoryIcon category={cat} />
                <span className="min-w-0 flex-1 truncate font-medium">
                  {t(`scanConfig.categories.${cat}`)}
                </span>
                {checked ? (
                  <span className="text-2xs text-primary" aria-hidden>
                    ✓
                  </span>
                ) : null}
              </label>
            );
          })}
        </div>
        <div className="mt-2 flex items-center gap-3 text-xs">
          <button
            type="button"
            className="text-primary hover:underline"
            onClick={() => form.setValue('categories', [...ALL_CATEGORIES])}
          >
            {t('scanConfig.selectAll')}
          </button>
          <button
            type="button"
            className="text-primary hover:underline"
            onClick={() => form.setValue('categories', [])}
          >
            {t('scanConfig.clearAll')}
          </button>
        </div>

        <h2 className="mb-3 mt-8 flex items-center gap-2 text-base font-semibold">
          <CogIcon className="size-4" aria-hidden />
          {t('scanConfig.advanced')}
        </h2>
        <div className="grid grid-cols-1 gap-4 rounded-lg border border-border bg-surface p-4 md:grid-cols-3">
          <div className="space-y-1.5">
            <Label htmlFor="sector-size">{t('scanConfig.sectorSize')}</Label>
            <Select
              value={values.sectorSize}
              onValueChange={(v) => form.setValue('sectorSize', v as FormValues['sectorSize'])}
              disabled={!volume?.sectorSize}
            >
              <SelectTrigger id="sector-size" aria-label={t('scanConfig.sectorSize')}>
                <SelectValue placeholder={t('scanConfig.sectorSizeAuto')} />
              </SelectTrigger>
              <SelectContent>
                <SelectItem value="auto">{t('scanConfig.sectorSizeAuto')}</SelectItem>
                <SelectItem value="512">512 B</SelectItem>
                <SelectItem value="4096">4096 B</SelectItem>
              </SelectContent>
            </Select>
          </div>
          <div className="space-y-1.5">
            <Label>{t('scanConfig.badSectorPolicy')}</Label>
            <RadioGroup
              value={values.badSectorPolicy}
              onValueChange={(v) => form.setValue('badSectorPolicy', v as BadSectorPolicy)}
              className="space-y-1"
            >
              <PolicyOption
                value="stop"
                label={t('scanConfig.badSectorStop')}
                destructive={false}
              />
              <PolicyOption
                value="skip"
                label={t('scanConfig.badSectorSkip')}
                destructive={false}
              />
              <PolicyOption
                value="zero-fill"
                label={t('scanConfig.badSectorZeroFill')}
                destructive
              />
            </RadioGroup>
          </div>
          <div className="space-y-1.5">
            <Label htmlFor="retries">{t('scanConfig.retryPolicy')}</Label>
            <input
              id="retries"
              type="number"
              min={0}
              max={10}
              className="h-8 w-24 rounded-sm border border-border bg-surface px-2 text-sm tabular outline-none focus-visible:ring-2 focus-visible:ring-ring/40"
              value={values.retries}
              onChange={(e) => form.setValue('retries', Number(e.target.value))}
            />
          </div>
        </div>
        {values.badSectorPolicy === 'zero-fill' ? (
          <Alert variant="danger" className="mt-3">
            <p>{t('scanConfig.zeroFillWarning')}</p>
          </Alert>
        ) : null}
      </div>

      <div className="sticky bottom-0 flex items-center justify-end gap-3 border-t border-border bg-surface/90 px-6 py-3 backdrop-blur">
        <Button type="button" variant="secondary" onClick={() => navigate('/')}>
          {t('common.back')}
        </Button>
        <Button
          type="submit"
          variant="primary"
          size="lg"
          loading={startScan.isPending}
          disabled={
            startScan.isPending ||
            values.categories.length === 0 ||
            (values.scopeMode === 'folder' && values.pathScope.trim() === '')
          }
          title={
            values.categories.length === 0
              ? t('scanConfig.typesTitle')
              : values.scopeMode === 'folder' && values.pathScope.trim() === ''
                ? t('scanConfig.scopeRequired')
                : undefined
          }
        >
          {t('scanConfig.start')}
        </Button>
      </div>
    </form>
  );
}

function ModeCard({
  icon: Icon,
  value,
  title,
  body,
  current,
  disabled,
  disabledReason,
}: {
  icon: React.ComponentType<{ className?: string }>;
  value: ScanMode;
  title: string;
  body: string;
  current: ScanMode;
  disabled?: boolean;
  disabledReason?: string;
}): React.JSX.Element {
  const { t } = useTranslation();
  const active = current === value && !disabled;
  return (
    <label
      className={cn(
        'relative flex cursor-pointer flex-col gap-2 rounded-lg border-2 p-4 transition-all',
        active
          ? 'border-primary bg-primary-muted/40'
          : 'border-border bg-surface hover:border-border-strong',
        disabled && 'cursor-not-allowed opacity-60',
        'focus-within:ring-2 focus-within:ring-ring',
      )}
      title={disabledReason}
    >
      <RadioGroupItem value={value} className="absolute right-3 top-3" disabled={disabled} />
      <div className="flex items-center gap-2">
        <Icon
          className={cn('size-5', active ? 'text-primary' : 'text-foreground-secondary')}
          aria-hidden
        />
        <span className="font-semibold">{title}</span>
      </div>
      <p className="text-xs leading-relaxed text-foreground-secondary">
        {disabledReason ?? body}
      </p>
      {disabled ? (
        <span className="text-2xs font-semibold uppercase text-warning" role="status">
          {t('common.error')}
        </span>
      ) : null}
    </label>
  );
}

function PolicyOption({
  value,
  label,
  destructive,
}: {
  value: string;
  label: string;
  destructive: boolean;
}): React.JSX.Element {
  return (
    <label className="flex cursor-pointer items-center gap-2 text-sm">
      <RadioGroupItem value={value} />
      <span className={destructive ? 'text-danger' : undefined}>{label}</span>
    </label>
  );
}
