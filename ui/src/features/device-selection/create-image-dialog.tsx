import * as React from 'react';
import { useTranslation } from 'react-i18next';
import {
  ChevronDownIcon,
  ChevronUpIcon,
  FolderOpenIcon,
  HardDriveIcon,
  SaveIcon,
  SquareIcon,
} from 'lucide-react';
import type { Device } from '@/connector/contract';
import {
  useCancelImageCreation,
  useImageFinishedBridge,
  useImageProgress,
  useImageState,
  useStartImageCreation,
} from '@/queries/imaging';
import { useOpenPath, usePickDirectory } from '@/queries/system';
import { formatBytes, formatDuration } from '@/lib/format';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Progress } from '@/components/ui/progress';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { toast } from 'sonner';

/**
 * Create Image — the reference's first-class home-page flow (strings:14534
 * "Create Image" / "Select a drive or a partition to create a backup disk
 * image"; the DiskBackupDialog chain UIUX/03:125; lane C). Byte-for-byte
 * flat image + .sha256 sidecar (the engine's S-040 contract; the og's
 * .edisk is a registered engine-side divergence, PARITY-DELTAS D-147).
 *
 * Truth rules (D-IMG-1/D-IMG-3 closed — the W9 wiring over the shipped
 * engine/CLI/bridge halves):
 * - CANCEL IS REAL: the Stop control (with the reference's stop-confirm —
 *   "You need to re-create the disk image if you stop it", strings:11672-
 *   11673) rides the bridge's image_create_cancel; the partial is KEPT as
 *   `<path>.partial` and the cancelled state says so plainly.
 * - The More-Option sector range (DiskBackupSaveDlg's from/to pair,
 *   strings:11640-11760): digits-only inputs (the og's [0-9]{1,15}
 *   grammar), empty To = end of disk; the ENGINE owns the bounds refusal
 *   (an out-of-range pair is refused bad-parameter — surfaced as the
 *   typed toast, never a silent truncation).
 * - The destination pre-check is advisory (the BRIDGE's own gate is the
 *   authority — destination-overlap / out-of-space sized to the RANGE's
 *   bytes refuse the start server-side); progress is the engine's
 *   cumulative-bytes callback, with throughput + ETA derived exactly like
 *   the scan page's.
 */

/** The og's sector-grammar: digits only, at most 15 (strings:11640-11760 —
 * the [0-9]{1,15} edit the DiskBackupSaveDlg pair accepts). */
const SECTOR_RE = /^[0-9]{1,15}$/;

export function CreateImageDialog({
  device,
  open,
  onOpenChange,
}: {
  device: Device | null;
  open: boolean;
  onOpenChange: (open: boolean) => void;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const [folder, setFolder] = React.useState<string | null>(null);
  const [fileName, setFileName] = React.useState('');
  const [imageId, setImageId] = React.useState<string | null>(null);
  const [showMore, setShowMore] = React.useState(false);
  const [fromSector, setFromSector] = React.useState('');
  const [toSector, setToSector] = React.useState('');
  const [confirmStop, setConfirmStop] = React.useState(false);
  const start = useStartImageCreation();
  const cancelImage = useCancelImageCreation();
  const pickDirectory = usePickDirectory();
  const openPath = useOpenPath();
  const progressEvent = useImageProgress(imageId);
  // Rehydration + the fallback poll: the event store lives only while a
  // listener subscribes (closing the dialog drops it), and a FINISHED job
  // emits no further events — the state query fills both gaps. The
  // finished-bridge invalidates it instantly at the terminal event.
  useImageFinishedBridge(imageId);
  const state = useImageState(imageId);
  const progress = progressEvent ?? state.data ?? null;

  // Reset per-open state at the open transition (the render-time state
  // adjustment pattern — no effect, no cascading renders: React re-runs
  // the render immediately with the reset values).
  const [wasOpen, setWasOpen] = React.useState(false);
  if (open && !wasOpen) {
    setWasOpen(true);
    setFolder(null);
    setFileName(defaultFileName(device, i18n.language));
    setImageId(null);
    setShowMore(false);
    setFromSector('');
    setToSector('');
    setConfirmStop(false);
    start.reset();
    cancelImage.reset();
  } else if (!open && wasOpen) {
    setWasOpen(false);
  }

  const running = imageId !== null && progress?.status === 'running';
  const finished =
    imageId !== null &&
    (progress?.status === 'completed' ||
      progress?.status === 'failed' ||
      progress?.status === 'cancelled');
  const path =
    folder !== null && fileName.trim() !== '' ? `${folder}\\${fileName.trim()}` : null;

  // The More-Option pair's advisory validity: digits-only inputs, and a
  // From that does not pass the To when both are given. The ENGINE owns
  // the real bounds gate (an out-of-disk pair is its typed refusal).
  const fromOk = fromSector === '' || SECTOR_RE.test(fromSector);
  const toOk = toSector === '' || SECTOR_RE.test(toSector);
  const pairOk = fromSector === '' || toSector === '' || Number(fromSector) <= Number(toSector);
  const rangeValid = fromOk && toOk && pairOk;
  // The range reaches the connector ONLY when at least one field is set;
  // an empty To means "to the end of the disk" (sectorCount 0 — the
  // engine's own end-of-disk convention).
  const sectorSize = device?.sectorSize ?? 512;
  const totalSectors =
    device && device.capacity.totalBytes > 0
      ? Math.floor(device.capacity.totalBytes / sectorSize)
      : 0;
  const range =
    fromSector !== '' || toSector !== ''
      ? {
          firstSector: fromSector === '' ? 0 : Number(fromSector),
          sectorCount:
            toSector === ''
              ? 0
              : Number(toSector) - (fromSector === '' ? 0 : Number(fromSector)) + 1,
        }
      : undefined;
  const rangeBytes =
    range !== undefined && totalSectors > 0
      ? Math.min(
          (range.sectorCount === 0 ? totalSectors - range.firstSector : range.sectorCount) *
            sectorSize,
          device?.capacity.totalBytes ?? 0,
        )
      : null;

  const onPickFolder = (): void => {
    pickDirectory.mutate(
      { excludeDeviceId: device?.id ?? null },
      {
        onSuccess: (picked) => {
          if (picked !== null) setFolder(picked);
        },
      },
    );
  };

  const onStart = (): void => {
    if (!device || path === null || !rangeValid) return;
    start.mutate(
      { deviceId: device.id, path, range },
      {
        onSuccess: ({ imageId: id }) => {
          setImageId(id);
          toast.success(t('imaging.started'));
        },
        onError: (err) => {
          toast.error(
            err instanceof Error && err.message.length > 0
              ? err.message
              : t('imaging.startFailed'),
          );
        },
      },
    );
  };

  const onStop = (): void => {
    if (imageId === null) return;
    cancelImage.mutate(imageId, {
      onSuccess: (ack) => {
        setConfirmStop(false);
        if (ack.status === 'running') {
          toast.info(t('imaging.stopping'));
        } else {
          toast.info(t('imaging.stopNoop'));
        }
      },
      onError: () => {
        toast.error(t('imaging.stopFailed'));
      },
    });
  };

  const onReveal = (): void => {
    if (folder !== null) openPath.mutate(folder);
  };

  const cancelSupported = state.data?.cancelSupported ?? true;

  return (
    <Dialog open={open && device !== null} onOpenChange={onOpenChange}>
      <DialogContent className="w-[520px] max-w-[92vw] rounded-xl p-6">
        <DialogHeader>
          <DialogTitle className="text-base font-bold">{t('imaging.title')}</DialogTitle>
          <DialogDescription>{t('imaging.subtitle')}</DialogDescription>
        </DialogHeader>

        {/* Source device summary */}
        {device ? (
          <div className="mt-3 flex items-center gap-3 rounded-md border border-[#E2E8F0] bg-[#F8FAFD] px-4 py-3 dark:border-border dark:bg-surface">
            <HardDriveIcon
              className="size-5 shrink-0 text-[#1677FF] dark:text-primary"
              aria-hidden
            />
            <div className="min-w-0 flex-1">
              <div className="truncate text-sm font-semibold text-foreground">
                {device.label}
              </div>
              <div className="text-xs text-foreground-secondary tabular">
                {t('imaging.sourceSize', {
                  size: formatBytes(rangeBytes ?? device.capacity.totalBytes, i18n.language),
                })}
              </div>
            </div>
          </div>
        ) : null}

        {imageId === null ? (
          /* Destination folder + filename (the reference's DiskBackupSaveDlg) */
          <div className="mt-3 space-y-2">
            <div className="flex items-center gap-2">
              <Input
                readOnly
                value={folder ?? ''}
                placeholder={t('imaging.destinationPlaceholder')}
                aria-label={t('imaging.destination')}
                className="h-9 flex-1 bg-white text-xs dark:bg-surface"
              />
              <Button
                variant="secondary"
                size="sm"
                className="h-9"
                onClick={onPickFolder}
                disabled={running}
              >
                <FolderOpenIcon className="size-4" aria-hidden />
                {t('imaging.browse')}
              </Button>
            </div>
            <Input
              value={fileName}
              onChange={(e) => setFileName(e.target.value)}
              placeholder={t('imaging.fileNamePlaceholder')}
              aria-label={t('imaging.fileName')}
              className="h-9 bg-white text-xs dark:bg-surface"
              spellCheck={false}
            />

            {/* The More-Option disclosure (DiskBackupSaveDlg's sector-range
                pair, strings:11640-11760) */}
            <button
              type="button"
              className="flex w-full items-center gap-1 pt-1 text-left text-[11px] font-medium text-primary hover:underline"
              onClick={() => setShowMore((v) => !v)}
              aria-expanded={showMore}
            >
              {showMore ? (
                <ChevronUpIcon className="size-3.5" aria-hidden />
              ) : (
                <ChevronDownIcon className="size-3.5" aria-hidden />
              )}
              {t('imaging.moreOptions')}
            </button>
            {showMore ? (
              <div className="space-y-2 rounded-md border border-[#E2E8F0] bg-[#F8FAFD] px-3 py-3 dark:border-border dark:bg-surface">
                <div className="flex items-center gap-2">
                  <div className="min-w-0 flex-1">
                    <label
                      className="mb-1 block text-[11px] font-medium text-foreground-secondary"
                      htmlFor="img-from-sector"
                    >
                      {t('imaging.fromSector')}
                    </label>
                    <Input
                      id="img-from-sector"
                      value={fromSector}
                      onChange={(e) => setFromSector(e.target.value.replace(/[^0-9]/g, ''))}
                      placeholder="0"
                      inputMode="numeric"
                      aria-invalid={!fromOk || !pairOk}
                      className="h-8 bg-white text-xs tabular dark:bg-surface"
                      spellCheck={false}
                    />
                  </div>
                  <div className="min-w-0 flex-1">
                    <label
                      className="mb-1 block text-[11px] font-medium text-foreground-secondary"
                      htmlFor="img-to-sector"
                    >
                      {t('imaging.toSector')}
                    </label>
                    <Input
                      id="img-to-sector"
                      value={toSector}
                      onChange={(e) => setToSector(e.target.value.replace(/[^0-9]/g, ''))}
                      placeholder={t('imaging.toSectorEnd')}
                      inputMode="numeric"
                      aria-invalid={!toOk || !pairOk}
                      className="h-8 bg-white text-xs tabular dark:bg-surface"
                      spellCheck={false}
                    />
                  </div>
                </div>
                {!rangeValid ? (
                  <p className="text-[11px] font-medium text-danger" role="alert">
                    {t('imaging.invalidSector')}
                  </p>
                ) : (
                  <p className="text-[11px] leading-relaxed text-foreground-secondary">
                    {t('imaging.rangeNote', {
                      total: totalSectors.toLocaleString(i18n.language),
                    })}
                  </p>
                )}
              </div>
            ) : null}

            <p className="text-[11px] leading-relaxed text-foreground-secondary">
              {t('imaging.sidecarNote', {
                size: formatBytes(
                  rangeBytes ?? device?.capacity.totalBytes ?? 0,
                  i18n.language,
                ),
              })}
            </p>
          </div>
        ) : (
          /* The progress phase (elapsed/remaining/percent — the reference's
              DiskBackProgressPage strings:11645-11731) */
          <div className="mt-4 space-y-3">
            <Progress
              value={Math.round((progress?.overallProgress ?? 0) * 100)}
              aria-label={t('imaging.progress')}
            />
            <div className="flex items-center justify-between text-xs tabular text-foreground-secondary">
              <span>
                {formatBytes(progress?.bytesDone ?? 0, i18n.language)} /{' '}
                {formatBytes(progress?.bytesTotal ?? 0, i18n.language)}
              </span>
              <span>{Math.round((progress?.overallProgress ?? 0) * 100)}%</span>
            </div>
            <div className="flex items-center justify-between text-xs tabular text-foreground-secondary">
              <span>
                {t('imaging.throughput', {
                  rate: formatBytes(
                    Math.round(progress?.throughputBytesPerSecond ?? 0),
                    i18n.language,
                  ),
                })}
              </span>
              {progress?.etaSeconds != null && progress.etaSeconds > 0 ? (
                <span>
                  {t('imaging.remaining', {
                    time: formatDuration(progress.etaSeconds, i18n.language),
                  })}
                </span>
              ) : null}
            </div>
            {running && state.data?.cancelRequested ? (
              <p className="rounded-md bg-warning-muted px-3 py-2 text-[11px] font-medium text-warning">
                {t('imaging.stopping')}
              </p>
            ) : null}
            {progress?.status === 'completed' ? (
              <p className="rounded-md bg-[#EBF3FF] px-3 py-2 text-[11px] font-semibold text-[#16A34A]">
                {t('imaging.done')}
              </p>
            ) : null}
            {progress?.status === 'failed' ? (
              <p className="rounded-md bg-[#FDE2E2] px-3 py-2 text-[11px] text-[#C92A2A] dark:bg-danger-muted dark:text-danger">
                {t('imaging.failed')}
              </p>
            ) : null}
            {progress?.status === 'cancelled' ? (
              <p className="rounded-md bg-warning-muted px-3 py-2 text-[11px] text-warning">
                {state.data?.partialKept
                  ? t('imaging.cancelledPartial')
                  : t('imaging.cancelled')}
              </p>
            ) : null}
          </div>
        )}

        <DialogFooter className="mt-4">
          {imageId === null ? (
            <>
              <Button variant="secondary" onClick={() => onOpenChange(false)}>
                {t('common.close')}
              </Button>
              <Button
                variant="primary"
                onClick={onStart}
                disabled={path === null || start.isPending || !rangeValid}
                title={
                  path === null
                    ? t('imaging.destinationPlaceholder')
                    : !rangeValid
                      ? t('imaging.invalidSector')
                      : undefined
                }
              >
                <SaveIcon className="size-4" aria-hidden />
                {t('imaging.start')}
              </Button>
            </>
          ) : finished ? (
            <>
              <Button variant="secondary" onClick={onReveal} disabled={folder === null}>
                <FolderOpenIcon className="size-4" aria-hidden />
                {t('imaging.reveal')}
              </Button>
              <Button variant="primary" onClick={() => onOpenChange(false)}>
                {t('common.close')}
              </Button>
            </>
          ) : confirmStop ? (
            /* The reference's stop-confirm (strings:11672-11673): the
               re-create truth stated plainly, Stop vs Keep going. */
            <div className="flex w-full flex-col gap-2" role="alertdialog">
              <p className="text-xs font-medium text-foreground">{t('imaging.stopTitle')}</p>
              <p className="text-[11px] leading-relaxed text-foreground-secondary">
                {t('imaging.stopBody')}
              </p>
              <div className="flex justify-end gap-2">
                <Button
                  variant="secondary"
                  size="sm"
                  onClick={() => setConfirmStop(false)}
                  disabled={cancelImage.isPending}
                >
                  {t('imaging.stopKeepGoing')}
                </Button>
                <Button
                  variant="danger"
                  size="sm"
                  onClick={onStop}
                  disabled={cancelImage.isPending}
                >
                  <SquareIcon className="size-4" aria-hidden />
                  {t('imaging.stop')}
                </Button>
              </div>
            </div>
          ) : (
            /* Running: the Stop control (D-IMG-1 closed — cancel is real)
               + the background-continue exit. */
            <div className="flex w-full items-center justify-between gap-2">
              <Button variant="secondary" onClick={() => onOpenChange(false)}>
                {t('imaging.hide')}
              </Button>
              {cancelSupported ? (
                <Button variant="danger" onClick={() => setConfirmStop(true)}>
                  <SquareIcon className="size-4" aria-hidden />
                  {t('imaging.stop')}
                </Button>
              ) : null}
            </div>
          )}
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}

/** The default file name: `<model> <yyyy-MM-dd>.img` (sanitized; the
 * reference's save dialog defaults to the .edisk family — ours is the
 * engine's flat .img + .sha256 sidecar, PARITY-DELTAS D-147). */
function defaultFileName(device: Device | null, locale: string): string {
  const model = (device?.model ?? 'disk')
    .replace(/[^A-Za-z0-9 _-]+/g, '')
    .trim()
    .replace(/\s+/g, '-');
  const date = new Date()
    .toLocaleDateString(locale, {
      year: 'numeric',
      month: '2-digit',
      day: '2-digit',
    })
    .replaceAll('/', '-');
  return `${model || 'disk'}-${date}.img`;
}
