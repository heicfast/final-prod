import * as React from 'react';
import { getCurrentWindow } from '@tauri-apps/api/window';
import { useTranslation } from 'react-i18next';
import { useQueryClient } from '@tanstack/react-query';
import { HardDriveDownloadIcon } from 'lucide-react';
import { readScanState } from '@/queries/scan';
import { queryKeys } from '@/queries/client';
import { connector } from '@/connector';
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

/** Non-terminal statuses — everything the engine treats as live work. */
const ACTIVE_STATUSES: ReadonlySet<string> = new Set([
  'starting',
  'running',
  'paused',
  'pausing',
  'resuming',
  'stopping',
]);

/** Bounded stop-drain window: the engine honors cancel at loop-head
 * checkpoints, but an in-flight raw read on an unhealthy transport
 * finishes or fails on its own first (F-B-16 — the documented honest
 * limit). 120 s bounds the wait; on expiry the truth surfaces (an error
 * toast) instead of an unbounded hang or a fake completion. */
const STOP_DRAIN_TIMEOUT_MS = 120_000;

/** Poll the engine's own state until it leaves the live set — bounded by
 * the drain deadline, throwing the honest (localized) timeout on expiry. */
const waitForTerminal = async (
  scanId: string,
  timeoutMs: number,
  timeoutMessage: string,
): Promise<void> => {
  const deadline = Date.now() + timeoutMs;
  for (;;) {
    let status: string | undefined;
    try {
      status = (await readScanState(scanId)).status;
    } catch {
      return; // scan gone (not-found) → nothing left to wait for
    }
    if (status === undefined || !ACTIVE_STATUSES.has(status)) return;
    if (Date.now() >= deadline) {
      throw new Error(timeoutMessage);
    }
    await new Promise<void>((resolve) => setTimeout(resolve, 500));
  }
};

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
      if (status !== undefined && ACTIVE_STATUSES.has(status)) {
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
    void (async () => {
      try {
        // SB-B-01: the bridge's session_save refuses while the engine is
        // busy — for a REAL reason: the session serializer walks the live
        // result tree without the tree mutex (cr_records.cpp serialize_tree
        // vs the scan thread's add_file), so a mid-run save would race it.
        // The engine's own partial-session mechanism is the stop tail
        // (S-057/W1c — the og's stop→SaveScan parity: a stopped scan's
        // partial results persist exactly like a finished run's). The
        // honest Save-and-exit: stop the scan, wait for the terminal
        // latch, then the REAL named save, then close.
        let status: string | undefined;
        try {
          status = (await readScanState(scanId)).status;
        } catch {
          status = undefined; // scan gone → nothing live to stop
        }
        if (status !== undefined && ACTIVE_STATUSES.has(status)) {
          try {
            await connector.scan.cancelScan(scanId);
          } catch (err) {
            // A cancel refusal only matters if the scan is still live
            // (e.g. it raced to terminal between the read and the stop).
            const now = await readScanState(scanId)
              .then((s) => s.status)
              .catch(() => undefined);
            if (now !== undefined && ACTIVE_STATUSES.has(now)) throw err;
          }
          await waitForTerminal(scanId, STOP_DRAIN_TIMEOUT_MS, t('exitGuard.stopTimeout'));
          // Terminal: the store and the query caches follow the engine.
          markScanStopped();
          void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId) });
          void qc.invalidateQueries({ queryKey: queryKeys.sessions });
        }
        await connector.sessions.saveSession(scanId, null);
        void qc.invalidateQueries({ queryKey: queryKeys.sessions });
        void getCurrentWindow().destroy();
      } catch (err) {
        // A refused stop, a stop-drain timeout, or a refused save — stay
        // open, honestly: nothing was destroyed, nothing was faked.
        setSaving(false);
        setConfirmOpen(false);
        toast.error(err instanceof Error ? err.message : t('errors.unknown'));
      }
    })();
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
