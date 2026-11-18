import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { listen } from '@tauri-apps/api/event';
import {
  CircleAlertIcon,
  CircleDashedIcon,
  HardDriveIcon,
  HeartPulseIcon,
  RefreshCcwIcon,
  ShieldCheckIcon,
  ThermometerIcon,
  TimerIcon,
} from 'lucide-react';
import { useDevices } from '@/queries/devices';
import { useHealth, fetchHealth } from '@/queries/health';
import type { Device, DiskHealth } from '@/connector/contract';
import { formatDriveGb, formatInteger } from '@/lib/format';
import { cn } from '@/lib/utils';
import { isTauriRuntime } from '@/lib/runtime';
import { DISK_HEALTH_SELECT_EVENT } from '@/lib/disk-health-window';
import { Skeleton } from '@/components/ui/skeleton';
import {
  Table,
  TableBody,
  TableCell,
  TableHead,
  TableHeader,
  TableRow,
} from '@/components/ui/table';
import { WindowControls } from '@/components/ui/window-controls';
import { BrandBriefcaseLogo } from '@/components/ui/brand-icons';

/**
 * The separate disk-health window (D-P4 reversal) — the standalone
 * equivalent of the reference's DiskHealth.exe companion (worklog/
 * REFERENCE-SMART.md §1, §10.1-3). One window, every disk the OS exposes,
 * the reference's presentation model:
 *
 * - disk selector ("Which disk do you care about?"), auto-jumping to the
 *   first Caution/Bad disk on open (firstCautionIndex);
 * - the 4-state verdict everywhere (Unknown/Good/Caution/Bad — never a
 *   percentage; a no-SMART disk grades Unknown (gray), never fake-green);
 * - the S.M.A.R.T. table with the reference's columns
 *   ID / Attribute / Current / Threshold / RawValue / ST and per-row
 *   status colors;
 * - the temperature card, truthful transport line, manual refresh + a
 *   background poll.
 *
 * The refresh poll interval is a documented divergence: the reference's
 * RefreshThread interval is corpus-UNKNOWN (VERIFICATION §10); we use the
 * CrystalDiskInfo-lineage default (5 minutes) — REFERENCE-SMART §7.
 */

/** The 5-minute background poll (see the module doc). */
const REFRESH_POLL_MS = 5 * 60 * 1000;

function statusTone(status: DiskHealth['status']): {
  text: string;
  chip: string;
  label: string;
} {
  switch (status) {
    case 'good':
      return {
        text: 'text-[#15803D]',
        chip: 'bg-[#DCFCE7] text-[#15803D] dark:bg-success/20 dark:text-success',
        label: 'health.statusGood',
      };
    case 'caution':
      return {
        text: 'text-[#B45309]',
        chip: 'bg-[#FEF3C7] text-[#B45309] dark:bg-warning/20 dark:text-warning',
        label: 'health.statusCaution',
      };
    case 'bad':
      return {
        text: 'text-[#B91C1C]',
        chip: 'bg-[#FEE2E2] text-[#B91C1C] dark:bg-danger/20 dark:text-danger',
        label: 'health.statusBad',
      };
    default:
      return {
        text: 'text-[#64748B]',
        chip: 'bg-[#F1F5F9] text-[#64748B] dark:bg-surface-tint dark:text-foreground-secondary',
        label: 'health.statusUnknown',
      };
  }
}

function rowStatusTone(status: 'ok' | 'caution' | 'failing' | 'unknown'): string {
  switch (status) {
    case 'ok':
      return 'bg-[#DCFCE7] text-[#15803D] dark:bg-success/20 dark:text-success';
    case 'caution':
      return 'bg-[#FEF3C7] text-[#B45309] dark:bg-warning/20 dark:text-warning';
    case 'failing':
      return 'bg-[#FEE2E2] text-[#B91C1C] dark:bg-danger/20 dark:text-danger';
    default:
      return 'bg-[#F1F5F9] text-[#64748B] dark:bg-surface-tint dark:text-foreground-secondary';
  }
}

function deviceDisplayName(device: Device): string {
  return device.model ?? device.label;
}

export function DiskHealthWindowApp(props: { initialDiskIndex: number }): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const devicesQuery = useDevices();
  // D-ENUM-01 (same evidence as the landing page: forensic run 44249495727
  // saw 2 selector candidates on a 4-disk machine): the all-disks selector
  // renders EVERY device in the payload — an attached virtual disk is a
  // real disk with real (or honestly-absent) SMART; the reference's
  // DiskHealth companion lists it too.
  const devices = React.useMemo(() => devicesQuery.data ?? [], [devicesQuery.data]);

  const [selectedId, setSelectedId] = React.useState<string | null>(null);
  /* the one-time first-caution jump: null = not decided yet. */
  const jumpedRef = React.useRef(false);
  /* user intent wins over the probe: once the user picks a disk (or a
   * re-open event retargets the window), the in-flight initial probe must
   * NOT snap the selection back — the race the window test caught: the
   * probe takes ~0.5-3 s on real hardware (one health read per disk), and
   * a user choosing a disk in that window would have their selection
   * overwritten by the probe's completion. */
  const userSelectedRef = React.useRef(false);
  const [initialScan, setInitialScan] = React.useState(props.initialDiskIndex >= 0);

  const health = useHealth(selectedId);
  const selectedDevice = devices.find((d) => d.id === selectedId) ?? null;

  /* The reference's firstCautionIndex: on open, look at every disk's
   * health and jump to the first Caution/Bad one; otherwise the pre-
   * selected disk (diskHealthIndex= parity), otherwise the first disk.
   * Health-unavailable disks (SD cards, unsupported bridges) participate
   * as Unknown — they never win the jump, they stay listed (§5). */
  React.useEffect(() => {
    if (jumpedRef.current || devices.length === 0) return;
    jumpedRef.current = true;
    let cancelled = false;
    (async () => {
      const results = await Promise.allSettled(devices.map((d) => fetchHealth(d.id)));
      setInitialScan(false);
      /* the user (or a re-open event) already chose — their choice stands. */
      if (cancelled || userSelectedRef.current) return;
      let firstAttention = -1;
      results.forEach((r, idx) => {
        if (firstAttention === -1 && r.status === 'fulfilled') {
          if (r.value.status === 'caution' || r.value.status === 'bad') firstAttention = idx;
        }
      });
      const idx =
        firstAttention !== -1
          ? firstAttention
          : props.initialDiskIndex >= 0 && props.initialDiskIndex < devices.length
            ? props.initialDiskIndex
            : 0;
      setSelectedId(devices[idx]?.id ?? devices[0]?.id ?? null);
    })();
    return () => {
      cancelled = true;
    };
    // eslint-disable-next-line react-hooks/exhaustive-deps -- one-shot on first device list
  }, [devices.length]);

  /* Re-open retarget (the reference's single-instance + diskHealthIndex=):
   * a second launcher click focuses this window and re-selects the disk. */
  React.useEffect(() => {
    if (!isTauriRuntime()) return;
    const unlisten = listen<{ diskIndex: number }>(DISK_HEALTH_SELECT_EVENT, (e) => {
      const idx = e.payload.diskIndex;
      if (idx >= 0 && idx < devices.length) {
        userSelectedRef.current = true; /* a retarget is user intent too */
        setSelectedId(devices[idx].id);
      }
    });
    return () => {
      void unlisten.then((f) => f());
    };
  }, [devices]);

  /* The background refresh poll (5 min — the documented divergence). */
  React.useEffect(() => {
    const id = window.setInterval(() => {
      void devicesQuery.refetch();
      void health.refetch();
    }, REFRESH_POLL_MS);
    return () => window.clearInterval(id);
    // eslint-disable-next-line react-hooks/exhaustive-deps -- the poll re-arms per selection
  }, [selectedId]);

  const onManualRefresh = (): void => {
    void devicesQuery.refetch();
    void health.refetch();
  };

  const status = health.data?.status ?? 'unknown';
  const tone = statusTone(status);

  return (
    <div className="flex h-full min-h-0 flex-col bg-[#EBF2FF] text-foreground dark:bg-background">
      {/* Title bar — the product's frameless chrome (drag + controls) */}
      <header
        className="flex shrink-0 items-center justify-between bg-[#EBF2FF] px-3.5 select-none dark:bg-surface"
        style={{ height: 'var(--titlebar-height)' }}
        data-tauri-drag-region
      >
        <div className="flex items-center gap-2" data-tauri-drag-region>
          <BrandBriefcaseLogo className="size-5" />
          <span className="text-[12.5px] font-semibold tracking-tight text-[#1E293B] dark:text-foreground">
            {t('app.name')}
          </span>
          <span className="text-[12.5px] font-medium text-[#64748B] dark:text-foreground-secondary">
            · {t('diskhealth.title')}
          </span>
        </div>
        <div className="flex items-center gap-1">
          <WindowControls />
        </div>
      </header>

      <main className="min-h-0 flex-1 overflow-y-auto bg-white p-5 dark:bg-background">
        <div className="mx-auto w-full max-w-[760px] space-y-4">
          {/* Disk selector — "Which disk do you care about?" (§1) */}
          <div className="flex items-center justify-between gap-3">
            <label
              htmlFor="disk-health-selector"
              className="text-[13px] font-semibold text-[#1E293B] dark:text-foreground"
            >
              {t('diskhealth.pickDisk')}
            </label>
            <div className="flex items-center gap-2">
              <button
                type="button"
                onClick={onManualRefresh}
                disabled={health.isFetching || devicesQuery.isFetching}
                data-testid="disk-health-refresh"
                className="inline-flex h-8 items-center gap-1.5 rounded-[6px] border border-[#DCE6F5] bg-white px-3 text-[12px] font-semibold text-[#334155] transition-colors hover:bg-[#F4F8FF] disabled:opacity-50 dark:border-border dark:bg-surface dark:text-foreground dark:hover:bg-surface-hover"
              >
                <RefreshCcwIcon
                  className={cn('size-3.5', health.isFetching && 'animate-spin')}
                  aria-hidden
                />
                {t('diskhealth.refresh')}
              </button>
              <select
                id="disk-health-selector"
                value={selectedId ?? ''}
                onChange={(e) => {
                  userSelectedRef.current = true; /* user intent wins over the probe */
                  setSelectedId(e.target.value);
                }}
                disabled={devices.length === 0}
                data-testid="disk-health-selector"
                className="h-8 min-w-[220px] rounded-[6px] border border-[#DCE6F5] bg-white px-2.5 text-[12.5px] font-medium text-[#1E293B] outline-none focus-visible:ring-2 focus-visible:ring-ring dark:border-border dark:bg-surface dark:text-foreground"
              >
                {devices.length === 0 ? <option value="">{t('home.emptyTitle')}</option> : null}
                {devices.map((d, idx) => (
                  <option key={d.id} value={d.id}>
                    {t('diskhealth.diskEntry', {
                      index: idx,
                      name: deviceDisplayName(d),
                      size: formatDriveGb(d.capacity.totalBytes),
                    })}
                  </option>
                ))}
              </select>
            </div>
          </div>

          {/* Loading / empty / content */}
          {devicesQuery.isLoading || initialScan ? (
            <div className="space-y-3">
              <Skeleton className="h-24 rounded-[12px]" />
              <Skeleton className="h-56 rounded-[12px]" />
            </div>
          ) : devices.length === 0 ? (
            <div className="flex flex-col items-center justify-center gap-2 rounded-[12px] border border-[#DCE6F5] py-14 dark:border-border">
              <HardDriveIcon className="size-8 text-[#94A3B8]" aria-hidden />
              <p className="text-[13px] text-[#64748B] dark:text-foreground-secondary">
                {t('home.emptyBody')}
              </p>
            </div>
          ) : (
            <>
              {/* Status banner + temperature card */}
              <div className="grid grid-cols-1 gap-4 sm:grid-cols-2">
                <div
                  className={cn(
                    'flex items-center gap-4 rounded-[12px] border p-4',
                    status === 'good'
                      ? 'border-[#DCE8FF] bg-gradient-to-br from-[#F4F8FF] to-[#EEF5FF] dark:border-border dark:from-surface-elevated dark:to-surface-tint'
                      : status === 'caution'
                        ? 'border-warning/40 bg-warning-muted/60'
                        : status === 'bad'
                          ? 'border-danger/40 bg-danger-muted/60'
                          : 'border-border bg-surface-elevated/60',
                  )}
                >
                  <HeartPulseIcon className={cn('size-9 shrink-0', tone.text)} aria-hidden />
                  <div className="min-w-0">
                    <div className="flex items-center gap-2">
                      <span
                        className={cn(
                          'inline-flex items-center gap-1 rounded-full px-2.5 py-0.5 text-[12px] font-semibold',
                          tone.chip,
                        )}
                        data-testid="disk-health-status"
                      >
                        {status === 'bad' || status === 'caution' ? (
                          <CircleAlertIcon className="size-3.5" aria-hidden />
                        ) : status === 'good' ? (
                          <ShieldCheckIcon className="size-3.5" aria-hidden />
                        ) : (
                          <CircleDashedIcon className="size-3.5" aria-hidden />
                        )}
                        {t(tone.label)}
                      </span>
                    </div>
                    <div className="mt-1 truncate text-[14px] font-bold text-[#1E293B] dark:text-foreground">
                      {selectedDevice ? deviceDisplayName(selectedDevice) : '—'}
                    </div>
                    <div className="mt-0.5 font-mono text-[11px] text-[#64748B] dark:text-foreground-secondary">
                      SN: {selectedDevice?.serial ?? '—'} ·{' '}
                      {selectedDevice ? formatDriveGb(selectedDevice.capacity.totalBytes) : '—'}
                    </div>
                  </div>
                </div>

                {/* Temperature + powered-on card (the reference's Temprature card) */}
                <div className="flex items-center justify-around gap-3 rounded-[12px] border border-[#DCE6F5] bg-white p-4 dark:border-border dark:bg-surface">
                  <div className="flex flex-col items-center gap-1">
                    <ThermometerIcon
                      className={cn(
                        'size-6',
                        (health.data?.temperatureC ?? 0) > 60
                          ? 'text-[#B45309]'
                          : 'text-[#1677FF] dark:text-primary',
                      )}
                      aria-hidden
                    />
                    <span className="text-[18px] font-bold tabular text-[#1E293B] dark:text-foreground">
                      {health.data?.temperatureC != null
                        ? `${Math.round(health.data.temperatureC)}°C`
                        : '—'}
                    </span>
                    <span className="text-[11px] text-[#64748B] dark:text-foreground-secondary">
                      {t('health.temperature')}
                    </span>
                  </div>
                  <div className="flex flex-col items-center gap-1">
                    <TimerIcon
                      className="size-6 text-[#1677FF] dark:text-primary"
                      aria-hidden
                    />
                    <span className="text-[18px] font-bold tabular text-[#1E293B] dark:text-foreground">
                      {health.data?.powerOnHours != null
                        ? formatInteger(health.data.powerOnHours, i18n.language)
                        : '—'}
                    </span>
                    <span className="text-[11px] text-[#64748B] dark:text-foreground-secondary">
                      {t('health.powerOnHours')}
                    </span>
                  </div>
                </div>
              </div>

              {/* The caution card (§1: "Your disk may have a physical failure") */}
              {status === 'caution' || status === 'bad' ? (
                <div className="rounded-[10px] border border-warning/40 bg-warning-muted/50 px-4 py-3 text-[12.5px] leading-relaxed text-[#B45309] dark:text-warning">
                  {t('diskhealth.cautionCard')}
                </div>
              ) : status === 'unknown' ? (
                <div className="rounded-[10px] border border-border bg-surface-elevated/60 px-4 py-3 text-[12.5px] leading-relaxed text-[#64748B] dark:text-foreground-secondary">
                  {t('health.unknownNote')}
                </div>
              ) : null}

              {/* S.M.A.R.T. table — the reference's columns
                  ID / Attribute / Current / Threshold / RawValue / ST (§3.1) */}
              <div className="overflow-hidden rounded-[12px] border border-[#E2ECF9] dark:border-border">
                <div className="flex items-center justify-between border-b border-[#E2ECF9] bg-[#F8FBFF] px-4 py-2.5 dark:border-border dark:bg-surface-muted/60">
                  <span className="text-[12.5px] font-semibold text-[#1E293B] dark:text-foreground">
                    {t('health.smartTable')}
                  </span>
                  <span className="font-mono text-[10.5px] text-[#94A3B8] dark:text-foreground-disabled">
                    {health.data?.transport ?? t('diskhealth.noTransport')}
                  </span>
                </div>
                {health.isLoading ? (
                  <div className="space-y-2 p-4">
                    <Skeleton className="h-6" />
                    <Skeleton className="h-6" />
                    <Skeleton className="h-6" />
                  </div>
                ) : (health.data?.attributes.length ?? 0) === 0 ? (
                  <div className="flex flex-col items-center gap-1.5 px-4 py-10">
                    <CircleDashedIcon className="size-6 text-[#94A3B8]" aria-hidden />
                    <p
                      className="text-[12.5px] text-[#64748B] dark:text-foreground-secondary"
                      data-testid="disk-health-empty-table"
                    >
                      {t('diskhealth.emptyTable')}
                    </p>
                  </div>
                ) : (
                  <Table>
                    <TableHeader>
                      <TableRow>
                        <TableHead className="pl-4">{t('health.smartCol.id')}</TableHead>
                        <TableHead>{t('health.smartCol.attribute')}</TableHead>
                        <TableHead className="text-right">
                          {t('health.smartCol.current')}
                        </TableHead>
                        <TableHead className="text-right">
                          {t('health.smartCol.threshold')}
                        </TableHead>
                        <TableHead className="text-right">
                          {t('health.smartCol.rawValue')}
                        </TableHead>
                        <TableHead className="pr-4 text-right">
                          {t('health.smartCol.st')}
                        </TableHead>
                      </TableRow>
                    </TableHeader>
                    <TableBody>
                      {(health.data?.attributes ?? []).map((a) => (
                        <TableRow key={a.id}>
                          <TableCell className="pl-4 font-mono text-[12px] text-[#64748B] dark:text-foreground-secondary">
                            {a.id}
                          </TableCell>
                          <TableCell className="text-[12.5px] text-[#334155] dark:text-foreground">
                            {a.name}
                          </TableCell>
                          <TableCell className="text-right text-[12.5px] text-[#334155] tabular dark:text-foreground">
                            {a.current ?? '—'}
                          </TableCell>
                          <TableCell className="text-right text-[12.5px] text-[#64748B] tabular dark:text-foreground-secondary">
                            {a.threshold ?? '—'}
                          </TableCell>
                          <TableCell className="text-right font-mono text-[12px] text-[#334155] tabular dark:text-foreground">
                            {formatInteger(a.raw, i18n.language)}
                          </TableCell>
                          <TableCell className="pr-4 text-right">
                            <span
                              className={cn(
                                'inline-flex items-center gap-1 rounded-full px-2 py-0.5 text-[11px] font-semibold',
                                rowStatusTone(a.status),
                              )}
                            >
                              {a.worst ?? '—'}
                            </span>
                          </TableCell>
                        </TableRow>
                      ))}
                    </TableBody>
                  </Table>
                )}
              </div>
            </>
          )}
        </div>
      </main>
    </div>
  );
}
