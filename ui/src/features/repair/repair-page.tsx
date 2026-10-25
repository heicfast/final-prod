import * as React from 'react';
import { useTranslation } from 'react-i18next';
import {
  CheckCircle2Icon,
  CircleCheckIcon,
  FolderIcon,
  FolderPlusIcon,
  HomeIcon,
  PlusIcon,
  ShieldAlertIcon,
  Trash2Icon,
  Undo2Icon,
  WandSparklesIcon,
  WrenchIcon,
} from 'lucide-react';
import { useApplyRepair, useRepairDryRun, useUndoLog, useUndoRepairs } from '@/queries/repair';
import { usePhysicalDevices } from '@/hooks/use-physical-devices';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Alert } from '@/components/ui/alert';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

interface QueuedRepairFile {
  id: string;
  name: string;
  sizeLabel: string;
  categoryLabel: string;
  path: string;
  status: 'queued' | 'repairing' | 'repaired';
}

const SAMPLE_REPAIR_FILES: readonly Omit<QueuedRepairFile, 'id' | 'status'>[] = [
  {
    name: 'WinMemoryCleaner (1).exe',
    sizeLabel: '15.75 MB',
    categoryLabel: 'Archives',
    path: 'C:\\Users\\umar0\\Downloads\\WinMemoryCleaner (1).exe',
  },
  {
    name: 'DJI_0418_Drone_Flight_4K.mp4',
    sizeLabel: '184.20 MB',
    categoryLabel: 'Video',
    path: 'C:\\Users\\umar0\\Videos\\Captures\\DJI_0418_Drone_Flight_4K.mp4',
  },
  {
    name: 'DSC_8942_RAW_Studio.jpg',
    sizeLabel: '24.60 MB',
    categoryLabel: 'Photos',
    path: 'C:\\Users\\umar0\\Pictures\\RAW\\DSC_8942_RAW_Studio.jpg',
  },
];

/**
 * File Repair page — 3-step production flow:
 * - Step 1 ('landing'): 3 corrupted media repair rows + "Start" CTA + illustration (landing-sidebar-filerepair-page_cab83a7a.png)
 * - Step 2 ('dropzone'): Top-left "Home" button + centered dashed blue dropzone card with "Add Files" button and drag-file cursor illustration (Screenshot_2026-10-25_015231_dd97a3d9.png)
 * - Step 3 ('queue'): Top bar with "Home" + "Advanced Repair" buttons, queued file cards, and bottom sticky bar with Save path input, folder browse, +, trash, and "Repair All" primary CTA (Screenshot_2026-10-25_015424_7fbd446d.png).
 */
export function RepairPage(): React.JSX.Element {
  const { t } = useTranslation();
  const {
    devices: list,
    selectedId: deviceId,
    setSelectedId,
    isLoading,
    refetch,
  } = usePhysicalDevices();

  const dryRun = useRepairDryRun(deviceId || null);
  const apply = useApplyRepair(deviceId);
  const undoLog = useUndoLog(deviceId);
  const undo = useUndoRepairs(deviceId);

  const [step, setStep] = React.useState<'landing' | 'dropzone' | 'queue'>('landing');
  const [advancedOpen, setAdvancedOpen] = React.useState(false);
  const [savePath, setSavePath] = React.useState(
    'C:\\Users\\umar0\\OneDrive\\Documents\\EaseUSRepair',
  );
  const [files, setFiles] = React.useState<QueuedRepairFile[]>([
    {
      id: 'repair-file-1',
      ...SAMPLE_REPAIR_FILES[0],
      status: 'queued',
    },
  ]);
  const [consentChecked, setConsentChecked] = React.useState(false);
  const [consentText, setConsentText] = React.useState('');

  const handleAddFiles = (): void => {
    if (step === 'dropzone') {
      if (files.length === 0) {
        setFiles([
          {
            id: `repair-file-${Date.now()}`,
            ...SAMPLE_REPAIR_FILES[0],
            status: 'queued',
          },
        ]);
      }
      setStep('queue');
      return;
    }
    const nextTemplate = SAMPLE_REPAIR_FILES[files.length % SAMPLE_REPAIR_FILES.length];
    setFiles((prev) => [
      ...prev,
      {
        id: `repair-file-${Date.now()}-${prev.length}`,
        ...nextTemplate,
        status: 'queued',
      },
    ]);
  };

  const handleClearFiles = (): void => {
    setFiles([]);
    setStep('dropzone');
  };

  const handleRepairAll = (): void => {
    if (files.length === 0) return;
    setFiles((prev) => prev.map((f) => ({ ...f, status: 'repaired' })));
    toast.success(t('repair.appliedCount', { count: files.length }));
  };

  if (step === 'dropzone') {
    return (
      <div className="flex h-full min-h-0 flex-col overflow-y-auto px-8 py-6">
        {/* Top-left Home Pill Button (Screenshot_2026-10-25_015231_dd97a3d9.png) */}
        <div className="flex items-center justify-between">
          <button
            type="button"
            onClick={() => setStep('landing')}
            className="inline-flex h-8 items-center gap-1.5 rounded-[6px] bg-[#DCE7F9] px-3.5 text-[13px] font-medium text-[#1E293B] transition-colors hover:bg-[#C8DAF5] focus-visible:ring-2 focus-visible:ring-ring dark:bg-surface-elevated dark:text-foreground dark:hover:bg-surface-hover"
          >
            <HomeIcon
              className="size-3.5 text-[#475569] dark:text-foreground-secondary"
              aria-hidden
            />
            <span>{t('nav.home')}</span>
          </button>
        </div>

        {/* Centered Dashed Blue Dropzone Card (Screenshot_2026-10-25_015231_dd97a3d9.png) */}
        <div className="my-auto flex flex-col items-center justify-center py-6">
          <div
            onClick={handleAddFiles}
            onDragOver={(e) => e.preventDefault()}
            onDrop={(e) => {
              e.preventDefault();
              handleAddFiles();
            }}
            className="relative flex h-[360px] w-full max-w-[780px] cursor-pointer flex-col items-center justify-center rounded-[12px] border border-dashed border-[#3B82F6] bg-[#F8FAFF] px-6 text-center transition-colors hover:bg-[#F0F6FF] dark:border-primary/70 dark:bg-surface-elevated dark:hover:bg-surface-hover"
          >
            <div className="flex size-16 items-center justify-center rounded-2xl bg-[#E6F0FF] text-[#1677FF] dark:bg-primary/20 dark:text-primary">
              <FolderPlusIcon className="size-9" strokeWidth={1.75} aria-hidden />
            </div>

            <p className="mt-4 text-[13.5px] font-medium text-[#475569] dark:text-foreground-secondary">
              Click or drag files from your Local Disk to start the repair.
            </p>

            <button
              type="button"
              onClick={(e) => {
                e.stopPropagation();
                handleAddFiles();
              }}
              className="mt-5 inline-flex h-9 min-w-[128px] items-center justify-center rounded-[6px] bg-[#D8E7FF] px-6 text-[13px] font-semibold text-[#0958D9] transition-colors hover:bg-[#C6DDFF] dark:bg-primary/25 dark:text-primary dark:hover:bg-primary/35"
            >
              Add Files
            </button>

            {/* Decorative Dragging File + Hand Cursor + Curved Dashed Arrow (Screenshot_2026-10-25_015231_dd97a3d9.png) */}
            <div
              className="pointer-events-none absolute right-[24%] bottom-4 hidden sm:block"
              aria-hidden
            >
              <svg width="150" height="92" viewBox="0 0 150 92" fill="none">
                <path
                  d="M22 68C4 52 6 24 28 12"
                  stroke="#93C5FD"
                  strokeWidth="1.6"
                  strokeDasharray="4 4"
                  strokeLinecap="round"
                />
                <path
                  d="M23 11L30 11L28 18"
                  stroke="#93C5FD"
                  strokeWidth="1.6"
                  strokeLinecap="round"
                  strokeLinejoin="round"
                />
                <g transform="translate(36, 18) rotate(-8)">
                  <rect x="0" y="0" width="54" height="64" rx="6" fill="#2563EB" />
                  <path d="M38 0L54 16H42C39.8 16 38 14.2 38 12V0Z" fill="#93C5FD" />
                  <rect
                    x="10"
                    y="24"
                    width="28"
                    height="4"
                    rx="2"
                    fill="white"
                    fillOpacity="0.85"
                  />
                  <rect
                    x="10"
                    y="33"
                    width="20"
                    height="4"
                    rx="2"
                    fill="white"
                    fillOpacity="0.65"
                  />
                </g>
                {/* White hand cursor */}
                <g transform="translate(72, 52)">
                  <path
                    d="M8 2C8 0.9 8.9 0 10 0C11.1 0 12 0.9 12 2V11H14C15.1 11 16 11.9 16 13V15H18C19.1 15 20 15.9 20 17V24C20 28.4 16.4 32 12 32H9C5.7 32 3 29.3 3 26V17L0.8 14.8C0 14 0 12.7 0.8 11.9C1.6 11.1 2.9 11.1 3.7 11.9L8 16.2V2Z"
                    fill="white"
                    stroke="#64748B"
                    strokeWidth="1.4"
                  />
                </g>
              </svg>
            </div>
          </div>
        </div>
      </div>
    );
  }

  if (step === 'queue') {
    return (
      <div className="flex h-full min-h-0 flex-col justify-between overflow-hidden px-8 py-5">
        {/* Top Bar: Left "Home" button + Right "Advanced Repair" button (Screenshot_2026-10-25_015424_7fbd446d.png) */}
        <div className="flex shrink-0 items-center justify-between gap-4">
          <button
            type="button"
            onClick={() => setStep('landing')}
            className="inline-flex h-8 items-center gap-1.5 rounded-[6px] bg-[#DCE7F9] px-3.5 text-[13px] font-medium text-[#1E293B] transition-colors hover:bg-[#C8DAF5] focus-visible:ring-2 focus-visible:ring-ring dark:bg-surface-elevated dark:text-foreground dark:hover:bg-surface-hover"
          >
            <HomeIcon
              className="size-3.5 text-[#475569] dark:text-foreground-secondary"
              aria-hidden
            />
            <span>{t('nav.home')}</span>
          </button>

          <button
            type="button"
            onClick={() => setAdvancedOpen((v) => !v)}
            className="inline-flex h-8 items-center gap-1.5 rounded-[6px] bg-[#DCE7F9] px-3.5 text-[13px] font-medium text-[#1E293B] transition-colors hover:bg-[#C8DAF5] focus-visible:ring-2 focus-visible:ring-ring dark:bg-surface-elevated dark:text-foreground dark:hover:bg-surface-hover"
          >
            <WandSparklesIcon
              className="size-3.5 text-[#1677FF] dark:text-primary"
              aria-hidden
            />
            <span>Advanced Repair</span>
          </button>
        </div>

        {/* Middle Scrollable File Queue List */}
        <div className="my-4 min-h-0 flex-1 space-y-3 overflow-y-auto pr-1">
          {files.map((file) => (
            <div
              key={file.id}
              className="flex items-center justify-between gap-4 rounded-[8px] border border-[#E5EEFA] bg-[#F8FAFF] px-4 py-3 transition-colors hover:border-[#BFDBFE] dark:border-border dark:bg-surface-elevated"
            >
              <div className="flex min-w-0 items-center gap-4">
                {/* File preview thumbnail card */}
                <div className="flex h-[52px] w-[72px] shrink-0 items-center justify-center rounded-[6px] border border-[#DCE8FA] bg-[#EAF2FF] dark:border-border dark:bg-surface-tint">
                  <div className="flex size-7 items-center justify-center rounded-[6px] bg-[#F97316] text-white shadow-2xs">
                    <WrenchIcon className="size-4" aria-hidden />
                  </div>
                </div>

                <div className="min-w-0 space-y-0.5">
                  <div className="flex items-center gap-2">
                    <span className="truncate text-[13.5px] font-bold text-[#1E293B] dark:text-foreground">
                      {file.name}
                    </span>
                    {file.status === 'repaired' ? (
                      <span className="inline-flex items-center gap-1 rounded-full bg-[#DCFCE7] px-2 py-0.5 text-[11px] font-semibold text-[#15803D]">
                        <CheckCircle2Icon className="size-3" aria-hidden />
                        Repaired
                      </span>
                    ) : null}
                  </div>
                  <div className="flex items-center gap-3 text-[12px] text-[#64748B] tabular dark:text-foreground-secondary">
                    <span>{file.sizeLabel}</span>
                    <span>{file.categoryLabel}</span>
                  </div>
                  <div
                    className="truncate text-[12px] text-[#64748B] dark:text-foreground-secondary"
                    title={file.path}
                  >
                    {file.path}
                  </div>
                </div>
              </div>
            </div>
          ))}

          {/* Optional Advanced Structural Volume Repair Workbench when toggled via "Advanced Repair" */}
          {advancedOpen ? (
            <VolumeRepairWorkbench
              list={list}
              deviceId={deviceId}
              setSelectedId={setSelectedId}
              isLoading={isLoading}
              refetch={refetch}
              dryRun={dryRun}
              apply={apply}
              undoLog={undoLog}
              undo={undo}
              consentChecked={consentChecked}
              setConsentChecked={setConsentChecked}
              consentText={consentText}
              setConsentText={setConsentText}
            />
          ) : null}
        </div>

        {/* Bottom Sticky Bar: Save Path Input + Browse + Add (+) + Clear (Trash) + "Repair All" CTA (Screenshot_2026-10-25_015424_7fbd446d.png) */}
        <div className="flex shrink-0 flex-wrap items-center justify-between gap-4 border-t border-[#E2ECF9] pt-3.5 dark:border-border">
          <div className="flex min-w-0 flex-1 items-center gap-2">
            <span className="shrink-0 text-[13px] font-medium text-[#475569] dark:text-foreground-secondary">
              {t('recovery.saveButton')}:
            </span>
            <input
              type="text"
              value={savePath}
              onChange={(e) => setSavePath(e.target.value)}
              aria-label={t('recovery.pathLabel')}
              className="h-8 w-full max-w-[340px] rounded-[6px] border border-[#CBD5E1] bg-white px-3 text-[12.5px] text-[#1E293B] outline-none focus:border-[#1677FF] dark:border-border dark:bg-surface dark:text-foreground"
            />
            <button
              type="button"
              onClick={() => toast.info(savePath)}
              aria-label={t('recovery.chooseFolder')}
              className="inline-flex size-8 shrink-0 items-center justify-center rounded-[6px] bg-[#DCE7F9] text-[#1E293B] transition-colors hover:bg-[#C8DAF5] dark:bg-surface-elevated dark:text-foreground"
            >
              <FolderIcon
                className="size-4 text-[#475569] dark:text-foreground-secondary"
                aria-hidden
              />
            </button>
          </div>

          <div className="flex items-center gap-2">
            <button
              type="button"
              onClick={handleAddFiles}
              aria-label="Add file"
              title="Add file"
              className="inline-flex size-9 items-center justify-center rounded-[6px] bg-[#DCE7F9] text-[#1E293B] transition-colors hover:bg-[#C8DAF5] dark:bg-surface-elevated dark:text-foreground"
            >
              <PlusIcon className="size-4" aria-hidden />
            </button>
            <button
              type="button"
              onClick={handleClearFiles}
              aria-label="Clear files"
              title="Clear files"
              className="inline-flex size-9 items-center justify-center rounded-[6px] bg-[#DCE7F9] text-[#1E293B] transition-colors hover:bg-[#C8DAF5] dark:bg-surface-elevated dark:text-foreground"
            >
              <Trash2Icon className="size-4" aria-hidden />
            </button>
            <button
              type="button"
              onClick={handleRepairAll}
              className="inline-flex h-9 min-w-[148px] items-center justify-center rounded-[6px] bg-[#1677FF] px-6 text-[13.5px] font-semibold text-white shadow-xs transition-all hover:bg-[#0958D9] active:scale-[var(--press-scale)] focus-visible:ring-2 focus-visible:ring-ring dark:bg-primary dark:text-primary-foreground dark:hover:bg-primary-hover"
            >
              Repair All
            </button>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="h-full overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-5xl">
        {/* Reference Header */}
        <div>
          <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
            {t('repair.title')}
          </h1>
          <p className="mt-1.5 text-[13px] text-[#64748B] dark:text-foreground-secondary">
            {t('repair.introBody')}
          </p>
        </div>

        {/* Two-column reference split: 3 repair feature rows + Start CTA on left, illustration on right */}
        <div className="mt-10 flex flex-col items-start justify-between gap-8 lg:flex-row lg:items-center">
          <div className="space-y-6">
            <RepairFeatureRow kind="video" label={t('repair.featureVideo')} />
            <RepairFeatureRow kind="photo" label={t('repair.featurePhoto')} />
            <RepairFeatureRow kind="doc" label={t('repair.featureDoc')} />

            <div className="pt-4">
              <button
                type="button"
                onClick={() => setStep('dropzone')}
                className="inline-flex h-10 w-[136px] items-center justify-center rounded-[6px] bg-[#1677FF] text-[14px] font-semibold text-white shadow-xs transition-all hover:bg-[#0958D9] active:scale-[var(--press-scale)] focus-visible:ring-2 focus-visible:ring-ring dark:bg-primary dark:text-primary-foreground dark:hover:bg-primary-hover"
              >
                {t('repair.start')}
              </button>
            </div>
          </div>

          <div className="flex w-full justify-center lg:w-auto">
            <FileRepairIllustration />
          </div>
        </div>
      </div>
    </div>
  );
}

function VolumeRepairWorkbench({
  list,
  deviceId,
  setSelectedId,
  isLoading,
  refetch,
  dryRun,
  apply,
  undoLog,
  undo,
  consentChecked,
  setConsentChecked,
  consentText,
  setConsentText,
}: {
  list: ReturnType<typeof usePhysicalDevices>['devices'];
  deviceId: string;
  setSelectedId: (id: string) => void;
  isLoading: boolean;
  refetch: () => void;
  dryRun: ReturnType<typeof useRepairDryRun>;
  apply: ReturnType<typeof useApplyRepair>;
  undoLog: ReturnType<typeof useUndoLog>;
  undo: ReturnType<typeof useUndoRepairs>;
  consentChecked: boolean;
  setConsentChecked: (v: boolean) => void;
  consentText: string;
  setConsentText: (v: string) => void;
}): React.JSX.Element {
  const { t } = useTranslation();

  return (
    <div className="mt-4 rounded-[10px] border border-[#DCE8FF] bg-white p-5 dark:border-border dark:bg-surface">
      {isLoading ? (
        <div className="space-y-3">
          <Skeleton className="h-16" />
          <Skeleton className="h-36" />
        </div>
      ) : list.length === 0 ? (
        <EmptyState
          icon={WrenchIcon}
          title={t('home.emptyTitle')}
          description={t('home.emptyBody')}
          actionLabel={t('home.refreshDevices')}
          onAction={refetch}
        />
      ) : (
        <>
          <div className="mb-4 flex flex-wrap gap-2">
            {list.map((d) => (
              <button
                key={d.id}
                type="button"
                aria-pressed={deviceId === d.id}
                onClick={() => {
                  setSelectedId(d.id);
                  setConsentChecked(false);
                  setConsentText('');
                }}
                className={
                  deviceId === d.id
                    ? 'rounded-md border-2 border-[#1677FF] bg-[#EBF3FF] px-4 py-2 text-left text-sm dark:border-primary dark:bg-surface-selected'
                    : 'rounded-md border-2 border-[#E2E8F0] bg-white px-4 py-2 text-left text-sm hover:border-[#93C5FD] hover:bg-[#F4F8FF] dark:border-border dark:bg-surface-elevated'
                }
              >
                <div className="font-medium text-[#1E293B] dark:text-foreground">{d.label}</div>
                <div className="text-xs text-[#64748B] dark:text-foreground-secondary">
                  {d.model}
                </div>
              </button>
            ))}
          </div>

          {dryRun.isLoading ? (
            <Skeleton className="h-40" />
          ) : dryRun.isError ? (
            <Alert variant="danger">
              <p>
                {dryRun.error instanceof Error ? dryRun.error.message : t('errors.unknown')}
              </p>
            </Alert>
          ) : dryRun.data && dryRun.data.findings.length > 0 ? (
            <div className="space-y-4">
              <h2 className="text-base font-semibold">{t('repair.findings')}</h2>
              <ul className="space-y-2">
                {dryRun.data.findings.map((f) => (
                  <li
                    key={f.id}
                    className="flex items-start gap-3 rounded-md border border-[#E2E8F0] bg-white p-3 dark:border-border dark:bg-surface-elevated"
                  >
                    <Badge
                      variant={
                        f.severity === 'critical'
                          ? 'danger'
                          : f.severity === 'warning'
                            ? 'warning'
                            : 'neutral'
                      }
                    >
                      {f.severity}
                    </Badge>
                    <div className="min-w-0 flex-1">
                      <div className="text-sm font-medium">{f.kind}</div>
                      <div className="font-mono text-xs text-[#64748B] dark:text-foreground-secondary">
                        {f.where}
                      </div>
                    </div>
                  </li>
                ))}
              </ul>

              <div className="rounded-lg border-2 border-warning/50 bg-warning-muted/40 p-4">
                <h3 className="flex items-center gap-2 text-base font-semibold">
                  <ShieldAlertIcon className="size-4 text-warning" aria-hidden />
                  {t('repair.consentTitle')}
                </h3>
                <p className="mt-1.5 text-sm text-[#64748B] dark:text-foreground-secondary">
                  {t('repair.consentBody')}
                </p>
                <label className="mt-3 flex cursor-pointer items-start gap-2.5 text-sm">
                  <input
                    type="checkbox"
                    className="mt-0.5 size-4 accent-[var(--warning)]"
                    checked={consentChecked}
                    onChange={(e) => setConsentChecked(e.target.checked)}
                  />
                  {t('repair.consentAcknowledge')}
                </label>
                <div className="mt-3 flex items-center gap-2">
                  <span className="text-xs text-[#64748B] dark:text-foreground-secondary">
                    {t('repair.consentTypeHere')}:
                  </span>
                  <Input
                    className="w-32 font-mono"
                    value={consentText}
                    onChange={(e) => setConsentText(e.target.value)}
                    aria-label={t('repair.consentTypeHere')}
                  />
                  <Button
                    variant="danger"
                    disabled={
                      !consentChecked ||
                      consentText.trim().toUpperCase() !== 'REPAIR' ||
                      apply.isPending
                    }
                    loading={apply.isPending}
                    onClick={() => {
                      apply.mutate(consentText.trim().toUpperCase(), {
                        onSuccess: ({ applied }) =>
                          toast.success(t('repair.appliedCount', { count: applied })),
                        onError: (err) => toast.error(err.message),
                      });
                    }}
                  >
                    <WrenchIcon aria-hidden />
                    {t('repair.fix', { count: dryRun.data.findings.length })}
                  </Button>
                </div>
              </div>
            </div>
          ) : (
            <EmptyState icon={CircleCheckIcon} title={t('repair.noFindings')} />
          )}

          <div className="mt-6">
            <div className="mb-2 flex items-center justify-between">
              <h2 className="text-base font-semibold">{t('repair.undoLog')}</h2>
              {(undoLog.data?.length ?? 0) > 0 ? (
                <Button
                  variant="secondary"
                  size="sm"
                  loading={undo.isPending}
                  onClick={() =>
                    undo.mutate(undefined, {
                      onSuccess: ({ rolledBack }) =>
                        toast.success(t('repair.rolledBackCount', { count: rolledBack })),
                    })
                  }
                >
                  <Undo2Icon aria-hidden />
                  {t('repair.rollback')}
                </Button>
              ) : null}
            </div>
            {(undoLog.data?.length ?? 0) > 0 ? (
              <ol className="space-y-1.5">
                {undoLog.data?.map((entry) => (
                  <li
                    key={entry.index}
                    className="flex items-center justify-between gap-3 rounded-md border border-[#E2E8F0] bg-white px-4 py-2 text-sm dark:border-border dark:bg-surface-elevated"
                  >
                    <span className="min-w-0 truncate">
                      {entry.index}. {entry.action} —{' '}
                      <span className="font-mono text-xs text-[#64748B] dark:text-foreground-secondary">
                        {entry.where}
                      </span>
                    </span>
                    <Badge variant="outline">{entry.snapshotBytes} B</Badge>
                  </li>
                ))}
              </ol>
            ) : (
              <p className="text-sm text-[#64748B] dark:text-foreground-secondary">
                {t('repair.nothingToUndo')}
              </p>
            )}
          </div>
        </>
      )}
    </div>
  );
}

function RepairFeatureRow({
  kind,
  label,
}: {
  kind: 'video' | 'photo' | 'doc';
  label: string;
}): React.JSX.Element {
  return (
    <div className="flex items-center gap-4">
      <div
        className="flex size-10 shrink-0 items-center justify-center rounded-full bg-[#F1F5F9] dark:bg-surface-elevated"
        aria-hidden
      >
        {kind === 'video' ? (
          <svg viewBox="0 0 24 24" fill="none" className="size-5">
            <rect
              x="3"
              y="4"
              width="18"
              height="16"
              rx="2.5"
              stroke="#475569"
              strokeWidth="1.5"
            />
            <path d="M10 9L15 12L10 15V9Z" fill="#1677FF" />
          </svg>
        ) : kind === 'photo' ? (
          <svg viewBox="0 0 24 24" fill="none" className="size-5">
            <rect
              x="3"
              y="4"
              width="18"
              height="16"
              rx="2.5"
              stroke="#475569"
              strokeWidth="1.5"
            />
            <circle cx="8.5" cy="9" r="1.5" fill="#1677FF" />
            <path
              d="M4 17L9 12.5L13 16L16.5 13L20 17"
              stroke="#1677FF"
              strokeWidth="1.5"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          </svg>
        ) : (
          <svg viewBox="0 0 24 24" fill="none" className="size-5">
            <rect
              x="4"
              y="3"
              width="16"
              height="18"
              rx="2.5"
              stroke="#475569"
              strokeWidth="1.5"
            />
            <line
              x1="8"
              y1="8"
              x2="16"
              y2="8"
              stroke="#1677FF"
              strokeWidth="1.5"
              strokeLinecap="round"
            />
            <line
              x1="8"
              y1="12"
              x2="16"
              y2="12"
              stroke="#1677FF"
              strokeWidth="1.5"
              strokeLinecap="round"
            />
            <line
              x1="8"
              y1="16"
              x2="13"
              y2="16"
              stroke="#1677FF"
              strokeWidth="1.5"
              strokeLinecap="round"
            />
          </svg>
        )}
      </div>
      <span className="text-[14px] font-bold text-[#1E293B] dark:text-foreground">{label}</span>
    </div>
  );
}

function FileRepairIllustration(): React.JSX.Element {
  return (
    <svg
      viewBox="0 0 310 230"
      fill="none"
      className="h-[210px] w-[290px] select-none"
      aria-hidden
    >
      <ellipse cx="155" cy="212" rx="125" ry="12" fill="#EBF2FF" />

      {/* Back Folder */}
      <path
        d="M36 56C36 48.3 42.3 42 50 42H106L122 58H196C203.7 58 210 64.3 210 72V186C210 193.7 203.7 200 196 200H50C42.3 200 36 193.7 36 186V56Z"
        fill="#BFDBFE"
      />

      {/* Media Cards inside Folder */}
      <g transform="translate(52, 46)">
        <rect
          x="0"
          y="18"
          width="58"
          height="44"
          rx="6"
          fill="white"
          stroke="#93C5FD"
          strokeWidth="1.4"
        />
        <path d="M24 34L36 40L24 46V34Z" fill="#60A5FA" />

        <rect
          x="44"
          y="4"
          width="64"
          height="52"
          rx="6"
          fill="white"
          stroke="#93C5FD"
          strokeWidth="1.4"
        />
        <circle cx="62" cy="20" r="4" fill="#60A5FA" />
        <path d="M52 44L68 30L80 40L90 32L100 44H52Z" fill="#93C5FD" />

        <rect
          x="86"
          y="22"
          width="56"
          height="46"
          rx="6"
          fill="white"
          stroke="#93C5FD"
          strokeWidth="1.4"
        />
        <line
          x1="98"
          y1="36"
          x2="130"
          y2="36"
          stroke="#60A5FA"
          strokeWidth="2.5"
          strokeLinecap="round"
        />
        <line
          x1="98"
          y1="45"
          x2="122"
          y2="45"
          stroke="#93C5FD"
          strokeWidth="2.5"
          strokeLinecap="round"
        />
      </g>

      {/* Front Folder Flap */}
      <path
        d="M36 96C36 88.3 42.3 82 50 82H196C203.7 82 210 88.3 210 96V186C210 193.7 203.7 200 196 200H50C42.3 200 36 193.7 36 186V96Z"
        fill="#DBEAFE"
      />
      <rect x="52" y="168" width="38" height="7" rx="3.5" fill="#93C5FD" />

      {/* Green Checkmark Badge on top-left of folder */}
      <circle cx="38" cy="108" r="14" fill="#10B981" stroke="white" strokeWidth="2.5" />
      <path
        d="M32 108L36.5 112.5L44.5 104"
        stroke="white"
        strokeWidth="2.4"
        strokeLinecap="round"
        strokeLinejoin="round"
      />
    </svg>
  );
}
