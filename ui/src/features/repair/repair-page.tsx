import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { CircleCheckIcon, ShieldAlertIcon, Undo2Icon, WrenchIcon } from 'lucide-react';
import { useApplyRepair, useRepairDryRun, useUndoLog, useUndoRepairs } from '@/queries/repair';
import { usePhysicalDevices } from '@/hooks/use-physical-devices';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Alert } from '@/components/ui/alert';
import { Badge } from '@/components/ui/badge';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/skeleton';
import { toast } from 'sonner';

/**
 * File Repair page (reference: landing-sidebar-filerepair-page_cab83a7a.png) —
 * displays the 3 corrupted media repair rows, the "Start" CTA button, and the
 * right-hand folder + robot holding puzzle piece illustration. Clicking "Start"
 * opens the interactive repair workbench (dry-run findings, consent gate, undo).
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

  const [workbenchOpen, setWorkbenchOpen] = React.useState(false);
  const [consentChecked, setConsentChecked] = React.useState(false);
  const [consentText, setConsentText] = React.useState('');

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
                onClick={() => setWorkbenchOpen((v) => !v)}
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

        {/* Interactive Volume & File Repair Workbench (expanded when Start is clicked) */}
        {workbenchOpen ? (
          <div className="mt-10 border-t border-[#E2E8F0] pt-6 dark:border-border">
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
                <div className="mb-5 flex flex-wrap gap-2">
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
                          : 'rounded-md border-2 border-[#E2E8F0] bg-white px-4 py-2 text-left text-sm hover:border-[#93C5FD] hover:bg-[#F4F8FF] dark:border-border dark:bg-surface-elevated dark:hover:border-primary/60 dark:hover:bg-surface-hover'
                      }
                    >
                      <div className="font-medium text-[#1E293B] dark:text-foreground">
                        {d.label}
                      </div>
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
                      {dryRun.error instanceof Error
                        ? dryRun.error.message
                        : t('errors.unknown')}
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
        ) : null}
      </div>
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

      {/* Robot Mascot on Right holding Blue Puzzle Piece */}
      <g transform="translate(182, 82)">
        {/* Blue Puzzle Piece */}
        <g transform="translate(-14, 18)">
          <rect x="4" y="4" width="32" height="32" rx="5" fill="#1677FF" />
          <circle cx="20" cy="4" r="6" fill="#1677FF" />
          <circle cx="36" cy="20" r="6" fill="#1677FF" />
        </g>

        {/* Robot Body & Head */}
        <g transform="translate(22, 16)">
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
      </g>
    </svg>
  );
}
