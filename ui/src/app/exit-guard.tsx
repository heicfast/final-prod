import * as React from 'react';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { useTranslation } from 'react-i18next';
import { HardDriveDownloadIcon } from 'lucide-react';
import { connector } from '@/connector';
import { isTauriRuntime } from '@/lib/runtime';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { useSaveSession } from '@/queries/sessions';
import { Button } from '@/components/ui/button';
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
 * ExitGuard — the close-requested save guard (W-02).
 *
 * The pre-fix shell had NO window-close handler: closing the window while
 * a scan was running ended the process with the session unsaved — the
 * only recovery path was the engine's milestone auto-saves. The guard
 * intercepts the close (title-bar X, system menu, OS shutdown dialog),
 * reads the ENGINE's own scan state (one fresh read — the store can lag
 * the engine because navigation unmounts the state poller), and offers:
 * Save and exit (a real session save, then close) / Keep scanning /
 * Exit without saving. A terminal or unknown scan state closes directly.
 *
 * Browser context (dev/e2e): inactive — the host owns tab close.
 */

/** Non-terminal statuses — everything the engine treats as live work. */
const ACTIVE_STATUSES: ReadonlySet<string> = new Set([
  'starting',
  'running',
  'paused',
  'pausing',
  'resuming',
  'stopping',
]);

/** Structural type of the tauri CloseRequestedEvent (kept mock-friendly). */
interface CloseRequestedLike {
  preventDefault(): void;
}

export function ExitGuard(): React.JSX.Element | null {
  const { t } = useTranslation();
  const active = useScanSessionStore((s) => s.active);
  const saveSession = useSaveSession(active?.scanId ?? null);
  const [confirmOpen, setConfirmOpen] = React.useState(false);
  const [saving, setSaving] = React.useState(false);

  React.useEffect(() => {
    if (!isTauriRuntime()) return;
    const win = getCurrentWindow();
    let disposed = false;
    let unlisten: (() => void) | undefined;
    const onRequested = async (event: CloseRequestedLike): Promise<void> => {
      if (!active) return; // no known scan → let the default close proceed
      let status: string | undefined;
      try {
        status = (await connector.scan.getScanState(active.scanId)).status;
      } catch {
        status = undefined; // scan gone (not-found) → close is safe
      }
      if (status !== undefined && ACTIVE_STATUSES.has(status)) {
        event.preventDefault();
        setConfirmOpen(true);
        return;
      }
      void win.destroy();
    };
    void win
      .onCloseRequested((event) => {
        void onRequested(event);
      })
      .then((fn) => {
        if (disposed) fn();
        else unlisten = fn;
      });
    return () => {
      disposed = true;
      unlisten?.();
    };
  }, [active]);

  if (!isTauriRuntime()) return null;

  const doSaveAndExit = (): void => {
    if (!active) return;
    setSaving(true);
    saveSession.mutate(null, {
      onSuccess: () => {
        void getCurrentWindow().destroy();
      },
      onError: (err) => {
        // The save refused (e.g. the drain window) — stay open, honestly.
        setSaving(false);
        setConfirmOpen(false);
        toast.error(err instanceof Error ? err.message : t('errors.unknown'));
      },
    });
  };

  const doExitWithoutSaving = (): void => {
    void getCurrentWindow().destroy();
  };

  return (
    <Dialog
      open={confirmOpen}
      onOpenChange={(open) => {
        if (saving) return; // a save in flight owns the dialog
        setConfirmOpen(open);
      }}
    >
      <DialogContent className="sm:max-w-[440px]">
        <DialogHeader>
          <DialogTitle className="flex items-center gap-2">
            <HardDriveDownloadIcon className="size-5 text-primary" aria-hidden />
            {t('exitGuard.title')}
          </DialogTitle>
          <DialogDescription>{t('exitGuard.body')}</DialogDescription>
        </DialogHeader>
        <DialogFooter className="gap-2 sm:gap-0">
          <Button variant="secondary" onClick={() => setConfirmOpen(false)} disabled={saving}>
            {t('exitGuard.keepScanning')}
          </Button>
          <Button
            variant="ghost"
            className="text-danger hover:bg-danger-muted"
            onClick={doExitWithoutSaving}
            disabled={saving}
          >
            {t('exitGuard.exitWithoutSaving')}
          </Button>
          <Button onClick={doSaveAndExit} disabled={saving}>
            {saving ? t('common.processing') : t('exitGuard.saveAndExit')}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
