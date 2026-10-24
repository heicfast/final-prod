import * as React from 'react';
import { useNavigate } from 'react-router';
import { useTranslation } from 'react-i18next';
import { HardDriveIcon, UsbIcon } from 'lucide-react';
import { useDevices } from '@/queries/devices';
import { useStartScan } from '@/queries/scan';
import { formatBytes } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * Lost Partition Rescue (reference: landing-sidebar-lostpartition-page_04acc766.png) —
 * displays the 3 partition rescue scenarios (Partition Missing, Partition Cannot
 * Be Opened, BitLocker Partition Missing/Damaged), the Royal Blue "Start" button,
 * and the right-hand segmented disc + robot holding partition wedge illustration.
 */
export function LostPartitionPage(): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const devicesQuery = useDevices();
  const startScan = useStartScan();
  const [selectedId, setSelectedId] = React.useState<string | null>(null);
  const [showDiskPicker, setShowDiskPicker] = React.useState(false);

  const disks = (devicesQuery.data ?? []).filter(
    (d) => d.kind !== 'virtual' && d.canScanWholeDisk,
  );
  const effectiveSelectedId = selectedId ?? disks[0]?.id ?? null;
  const selected = disks.find((d) => d.id === effectiveSelectedId) ?? null;

  const beginScan = (targetDiskId?: string): void => {
    const id = targetDiskId ?? selected?.id;
    if (!id) {
      setShowDiskPicker(true);
      return;
    }
    startScan.mutate(
      { targetId: id },
      {
        onSuccess: ({ scanId }) => {
          void navigate(`/scan/${id}/live?scanId=${scanId}`);
        },
        onError: (err) => {
          const reasonKey = (err as { reasonKey?: string }).reasonKey;
          toast.error(reasonKey ? t(reasonKey) : err.message);
        },
      },
    );
  };

  if (devicesQuery.isLoading) {
    return (
      <div className="p-8">
        <Skeleton className="mb-6 h-8 w-80" />
        <Skeleton className="h-64" />
      </div>
    );
  }

  if (devicesQuery.isError) {
    return (
      <div className="p-8">
        <EmptyState
          icon={HardDriveIcon}
          title={t('errors.title')}
          description={t('errors.deviceGone')}
          actionLabel={t('common.retry')}
          onAction={() => void devicesQuery.refetch()}
        />
      </div>
    );
  }

  return (
    <div className="h-full overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-5xl">
        {/* Reference Header */}
        <div>
          <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B]">
            {t('lostPartition.title')}
          </h1>
          <p className="mt-1.5 text-[13px] text-[#64748B]">{t('lostPartition.subtitle')}</p>
        </div>

        {/* Two-column reference split: 3 partition rows + Start CTA on left, disc + robot illustration on right */}
        <div className="mt-9 flex flex-col items-start justify-between gap-8 lg:flex-row lg:items-center">
          <div className="max-w-lg space-y-6">
            <PartitionScenarioRow
              kind="missing"
              title={t('lostPartition.scenarioMissing.title')}
              body={t('lostPartition.scenarioMissing.body')}
            />
            <PartitionScenarioRow
              kind="unopenable"
              title={t('lostPartition.scenarioUnopenable.title')}
              body={t('lostPartition.scenarioUnopenable.body')}
            />
            <PartitionScenarioRow
              kind="bitlocker"
              title={t('lostPartition.scenarioBitlocker.title')}
              body={t('lostPartition.scenarioBitlocker.body')}
              note={t('lostPartition.bitlockerNote')}
            />

            <div className="flex flex-wrap items-center gap-3 pt-3">
              <button
                type="button"
                disabled={startScan.isPending}
                onClick={() => beginScan()}
                className="inline-flex h-10 w-[136px] items-center justify-center rounded-[6px] bg-[#1677FF] text-[14px] font-semibold text-white shadow-xs transition-colors hover:bg-[#0958D9] focus-visible:ring-2 focus-visible:ring-ring disabled:opacity-50"
              >
                {t('lostPartition.start')}
              </button>
              {disks.length > 1 ? (
                <button
                  type="button"
                  onClick={() => setShowDiskPicker((v) => !v)}
                  className="text-[12.5px] font-medium text-[#64748B] underline decoration-[#CBD5E1] underline-offset-4 hover:text-[#1677FF]"
                >
                  {t('lostPartition.diskList')} ({selected?.label ?? disks[0]?.label})
                </button>
              ) : null}
            </div>
          </div>

          <div className="flex w-full justify-center lg:w-auto">
            <LostPartitionIllustration />
          </div>
        </div>

        {/* Expandable Disk Picker when user wants to switch target disk */}
        {showDiskPicker && disks.length > 0 ? (
          <div className="mt-8 border-t border-[#E2E8F0] pt-5">
            <h2 className="mb-3 text-[13px] font-semibold text-[#475569]">
              {t('lostPartition.diskList')}
            </h2>
            <div
              role="radiogroup"
              aria-label={t('lostPartition.diskList')}
              className="grid grid-cols-1 gap-3 sm:grid-cols-2"
            >
              {disks.map((disk) => {
                const isSel = effectiveSelectedId === disk.id;
                return (
                  <button
                    key={disk.id}
                    type="button"
                    role="radio"
                    aria-checked={isSel}
                    onClick={() => setSelectedId(disk.id)}
                    onDoubleClick={() => beginScan(disk.id)}
                    className={cn(
                      'flex items-center gap-3.5 rounded-[10px] border px-4 py-3 text-left transition-all',
                      'focus-visible:ring-2 focus-visible:ring-ring',
                      isSel
                        ? 'border-[#1677FF] bg-[#F4F8FF] ring-1 ring-[#1677FF]'
                        : 'border-[#E2E8F0] bg-[#F8FAFD] hover:border-[#93C5FD]',
                    )}
                  >
                    <div className="flex size-10 shrink-0 items-center justify-center rounded-md bg-[#EBF3FF] text-[#1677FF]">
                      <UsbIcon className="size-5" strokeWidth={1.5} aria-hidden />
                    </div>
                    <div className="min-w-0 flex-1">
                      <div className="flex items-center gap-2">
                        <span className="truncate text-[13.5px] font-semibold text-[#1E293B]">
                          {disk.label}
                        </span>
                        {disk.volumes.some((v) => v.lostPartition) ? (
                          <Badge variant="primary">{t('home.lostPartitionBadge')}</Badge>
                        ) : null}
                      </div>
                      <div className="text-[12px] text-[#64748B] tabular">
                        {disk.model} · {formatBytes(disk.capacity.totalBytes, i18n.language)}
                      </div>
                    </div>
                  </button>
                );
              })}
            </div>
          </div>
        ) : null}
      </div>
    </div>
  );
}

function PartitionScenarioRow({
  kind,
  title,
  body,
  note,
}: {
  kind: 'missing' | 'unopenable' | 'bitlocker';
  title: string;
  body: string;
  note?: string;
}): React.JSX.Element {
  return (
    <div className="flex items-start gap-4">
      <div
        className="mt-0.5 flex size-10 shrink-0 items-center justify-center rounded-full bg-[#F1F5F9]"
        aria-hidden
      >
        <svg viewBox="0 0 24 24" fill="none" className="size-5">
          <circle cx="11" cy="11" r="7" stroke="#475569" strokeWidth="1.5" />
          <circle cx="11" cy="11" r="2" fill="#475569" />
          {kind === 'missing' ? (
            <circle cx="17" cy="17" r="4.5" fill="#1677FF" />
          ) : kind === 'unopenable' ? (
            <path d="M16 14L20 20H12L16 14Z" fill="#1677FF" />
          ) : (
            <rect x="13.5" y="13.5" width="7" height="6" rx="1.2" fill="#1677FF" />
          )}
        </svg>
      </div>
      <div className="min-w-0">
        <div className="text-[14px] font-bold text-[#1E293B]">{title}</div>
        <p className="mt-0.5 text-[12.5px] leading-relaxed text-[#64748B]">{body}</p>
        {note ? <span className="sr-only">{note}</span> : null}
      </div>
    </div>
  );
}

function LostPartitionIllustration(): React.JSX.Element {
  return (
    <svg
      viewBox="0 0 310 230"
      fill="none"
      className="h-[210px] w-[290px] select-none"
      aria-hidden
    >
      <ellipse cx="155" cy="212" rx="125" ry="12" fill="#EBF2FF" />

      {/* Segmented Disc Platter */}
      <g transform="translate(36, 36)">
        <circle cx="76" cy="76" r="68" fill="#DBEAFE" stroke="#93C5FD" strokeWidth="2" />
        {/* Missing top-right wedge cutout */}
        <path d="M76 76L76 8A68 68 0 0 1 144 76Z" fill="white" />
        {/* Partition segment lines */}
        <line x1="76" y1="76" x2="20" y2="36" stroke="white" strokeWidth="2.5" />
        <line x1="76" y1="76" x2="36" y2="132" stroke="white" strokeWidth="2.5" />
        <line x1="76" y1="76" x2="144" y2="76" stroke="white" strokeWidth="2.5" />
        <circle cx="76" cy="76" r="22" fill="white" stroke="#93C5FD" strokeWidth="2" />
        <circle cx="76" cy="76" r="8" fill="#BFDBFE" />
      </g>

      {/* Extracted Blue Partition Wedge */}
      <path
        d="M128 96L128 38A58 58 0 0 1 186 96Z"
        fill="#3B82F6"
        stroke="#1677FF"
        strokeWidth="1.5"
      />

      {/* Robot Mascot on Right */}
      <g transform="translate(178, 86)">
        <rect
          x="14"
          y="48"
          width="40"
          height="44"
          rx="18"
          fill="white"
          stroke="#93C5FD"
          strokeWidth="2"
        />
        <rect x="22" y="58" width="24" height="16" rx="4" fill="#DBEAFE" />
        <rect x="2" y="16" width="5" height="12" rx="2.5" fill="#1677FF" />
        <rect x="61" y="16" width="5" height="12" rx="2.5" fill="#1677FF" />
        <rect
          x="6"
          y="4"
          width="56"
          height="44"
          rx="21"
          fill="white"
          stroke="#93C5FD"
          strokeWidth="2"
        />
        <rect x="13" y="13" width="42" height="23" rx="11" fill="#0F172A" />
        <ellipse cx="26" cy="24.5" rx="3.5" ry="4.5" fill="#38BDF8" />
        <ellipse cx="42" cy="24.5" rx="3.5" ry="4.5" fill="#38BDF8" />
      </g>
    </svg>
  );
}
