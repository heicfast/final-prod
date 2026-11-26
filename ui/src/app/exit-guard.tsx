import * as React from 'react';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { useTranslation } from 'react-i18next';
import { useQueryClient } from '@tanstack/react-query';
import { HardDriveDownloadIcon } from 'lucide-react';
import { readScanState, ACTIVE_SCAN_STATUSES } from '@/queries/scan';
import { queryKeys } from '@/queries/client';
import { stopScanThenSaveSession } from '@/queries/sessions';
import { isTauriRuntime } from '@/lib/runtime';
import { useScanSessionStore } from '@/stores/scan-session-store';
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
 * The handler MUST return its promise to the onCloseRequested wrapper
 * (SB-B-02): @tauri-apps/api 2.12.4 awaits the handler and then checks
 * `evt.isPreventDefault()` — a fire-and-forget handler returns before the
 * async state read resolves, so the wrapper destroys the window while the
 * guard is still deciding (and the dialog can never win). The wrapper's
 * own destroy() needs the `core:window:allow-destroy` capability grant —
 * without it the X button cannot close the window at all.
 *
 * Browser context (dev/e2e): inactive — the host owns tab close.
 */

/** Non-terminal statuses — everything the engine treats as live work.
 * The shared definition lives in queries/scan.ts (ACTIVE_SCAN_STATUSES). */

/** Bounded stop-drain window: the engine honors cancel at loop-head
 * checkpoints, but an in-flight raw read on an unhealthy transport
 * finishes or fails on its own first (F-B-16 — the documented honest
 * limit). 120 s bounds the wait; on expiry the truth surfaces (an error
 * toast) instead of an unbounded hang or a fake completion. */
const STOP_DRAIN_TIMEOUT_MS = 120_000;

/** Structural type of the tauri CloseRequestedEvent (kept mock-friendly). */
interface CloseRequestedLike {
  preventDefault(): void;
}

export function ExitGuard(): React.JSX.Element | null {
  const { t } = useTranslation();
  const qc = useQueryClient();
  const active = useScanSessionStore((s) => s.active);
  const markScanStopped = useScanSessionStore((s) => s.markScanStopped);
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
        status = (await readScanState(active.scanId)).status;
      } catch {
        status = undefined; // scan gone (not-found) → close is safe
      }
      if (status !== undefined && ACTIVE_SCAN_STATUSES.has(status)) {
        event.preventDefault();
        setConfirmOpen(true);
        return;
      }
      // Terminal or unknown state: not prevented — the onCloseRequested
      // wrapper's own destroy() (fired after this handler resolves) closes
      // the window. Destroying here as well would race the wrapper into a
      // rejected second invoke on a gone window.
    };
    void win
      .onCloseRequested((event) => onRequested(event))
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
    // The scanId is captured at click time: the store's `active` is cleared
    // when the scan stops below, and the flow must still address the SAME
    // scan for the save (the captured id, never a null closure re-read).
    const scanId = active.scanId;
    setSaving(true);
    // SB-B-01: the flow lives in the queries layer (arch-lint A1 — the
    // connector boundary): read the engine state, stop the live scan,
    // wait for the terminal latch (bounded drain deadline, honest
    // localized timeout), then the REAL named save. The engine refuses a
    // mid-run save for a real reason — serialize_tree walks the live
    // result tree off the tree mutex — and the engine's own
    // partial-session mechanism is the stop tail (S-057/W1c, the og's
    // stop→SaveScan parity).
    void stopScanThenSaveSession(scanId, STOP_DRAIN_TIMEOUT_MS, t('exitGuard.stopTimeout'))
      .then(() => {
        // Terminal + saved: the store and the query caches follow the
        // engine, then the window closes.
        markScanStopped();
        void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId) });
        void qc.invalidateQueries({ queryKey: queryKeys.sessions });
        void getCurrentWindow().destroy();
      })
      .catch((err: unknown) => {
        // A refused stop, a stop-drain timeout, or a refused save — stay
        // open, honestly: nothing was destroyed, nothing was faked.
        setSaving(false);
        setConfirmOpen(false);
        toast.error(err instanceof Error ? err.message : t('errors.unknown'));
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
