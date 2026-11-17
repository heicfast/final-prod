/**
 * The separate disk-health window (D-P4 reversal).
 *
 * Corpus ground truth (worklog/REFERENCE-SMART.md §1, §10): the reference
 * product keeps its SMART deep-dive in a SEPARATE surface (DiskHealth.exe,
 * 840×590 frameless, launched from the home "Disk Health" card with
 * `diskHealthIndex=<n>` pre-selecting the disk, single-instance via a named
 * mutex — a second launch focuses the existing window). This module is our
 * equivalent: one WebviewWindow, pre-select by index, single instance,
 * focus-on-reopen.
 *
 * Divergences, documented (PARITY-DELTAS D-P4): we ship the surface as a
 * webview window of the same process (not a second executable — the
 * companion-exe/tray/schtasks guardian packaging is out of scope for this
 * product, recorded DELIBERATE-DIVERGENCE); the pre-select event carries
 * the same meaning as `diskHealthIndex=`.
 */

import { WebviewWindow } from '@tauri-apps/api/webviewWindow';
import { emitTo } from '@tauri-apps/api/event';
import { isTauriRuntime } from './runtime';

export const DISK_HEALTH_WINDOW_LABEL = 'disk-health';

/** The event a re-open emits to retarget the existing window's selector. */
export const DISK_HEALTH_SELECT_EVENT = 'disk-health:select';

/**
 * Open (or focus) the disk-health window. Returns false in a plain browser
 * (dev/preview/e2e — no multi-window shell there; the in-app /health route
 * remains the browser-mode surface). Creation failures surface through the
 * optional onError callback (Tauri emits them on the window's error event).
 */
export async function openDiskHealthWindow(
  diskIndex = 0,
  onError?: (message: string) => void,
): Promise<boolean> {
  if (!isTauriRuntime()) return false;

  const existing = await WebviewWindow.getByLabel(DISK_HEALTH_WINDOW_LABEL);
  if (existing !== null) {
    /* the reference's single-instance behavior: second launch focuses the
     * existing window and retargets it (diskHealthIndex= semantics). */
    await existing.setFocus();
    await emitTo(DISK_HEALTH_WINDOW_LABEL, DISK_HEALTH_SELECT_EVENT, { diskIndex });
    return true;
  }

  const win = new WebviewWindow(DISK_HEALTH_WINDOW_LABEL, {
    /* the boot branch in main.tsx renders the standalone surface for this
     * query; `disk` is the pre-selected index (diskHealthIndex= parity). */
    url: `/?window=disk-health&disk=${diskIndex}`,
    title: 'Disk Health',
    width: 840,
    height: 590,
    minWidth: 720,
    minHeight: 540,
    center: true,
    resizable: true,
    decorations: false,
  });
  if (onError !== undefined) {
    void win.once('tauri://error', (e) => {
      onError(String((e.payload as { message?: string } | null)?.message ?? e.payload));
    });
  }
  void win; /* the constructor promise: creation proceeds in the background,
   * failures surface through the once() handler above */
  return true;
}
