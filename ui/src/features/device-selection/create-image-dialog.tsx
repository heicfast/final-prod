import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { FolderOpenIcon, HardDriveIcon, SaveIcon } from 'lucide-react';
import type { Device } from '@/connector/contract';
import {
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
 * Truth rules: NO cancel button (the ABI exports none — cancelSupported
 * is always false; the copy says the run completes, never a dead stop
 * control); the destination pre-check is advisory (the BRIDGE's own gate
 * is the authority — destination-overlap / out-of-space refuse the start
 * server-side); progress is the engine's cumulative-bytes callback, with
 * throughput + ETA derived exactly like the scan page's.
 */

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
  const start = useStartImageCreation();
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
    start.reset();
  } else if (!open && wasOpen) {
    setWasOpen(false);
  }

  const running = imageId !== null && progress?.status === 'running';
  const finished =
    imageId !== null && (progress?.status === 'completed' || progress?.status === 'failed');
  const path =
    folder !== null && fileName.trim() !== '' ? `${folder}\\${fileName.trim()}` : null;

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
    if (!device || path === null) return;
    start.mutate(
      { deviceId: device.id, path },
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

  const onReveal = (): void => {
    if (folder !== null) openPath.mutate(folder);
  };

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
                  size: formatBytes(device.capacity.totalBytes, i18n.language),
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
            <p className="text-[11px] leading-relaxed text-foreground-secondary">
              {t('imaging.sidecarNote', {
                size: formatBytes(device?.capacity.totalBytes ?? 0, i18n.language),
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
            {progress?.status === 'running' ? (
              <p className="rounded-md bg-[#EBF3FF] px-3 py-2 text-[11px] text-[#0B4F9E] dark:bg-primary-muted dark:text-primary">
                {t('imaging.noCancelNote')}
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
                disabled={path === null || start.isPending}
                title={path === null ? t('imaging.destinationPlaceholder') : undefined}
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
          ) : (
            /* Running: no stop control — the truthful no-cancel contract
                (the only exit closes the dialog; the job keeps running and
                the state stays queryable). */
            <Button variant="secondary" onClick={() => onOpenChange(false)}>
              {t('imaging.hide')}
            </Button>
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
