import { useEffect, useState } from 'react';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { SessionMeta } from '@/connector/contract';
import { queryKeys } from './client';
import { readScanState, ACTIVE_SCAN_STATUSES } from './scan';

export function useSessions() {
  return useQuery({
    queryKey: queryKeys.sessions,
    queryFn: () => connector.sessions.listSessions(),
  });
}

export function useSaveSession(scanId: string | null) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string | null) => connector.sessions.saveSession(scanId!, path),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.sessions }),
  });
}

export function useLoadSession() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string) => connector.sessions.loadSession(path),
    onSuccess: ({ scanId }) => {
      void qc.invalidateQueries({ queryKey: ['scan', scanId] });
      void qc.invalidateQueries({ queryKey: queryKeys.sessions });
    },
  });
}

export function useCancelSave() {
  return useMutation({
    mutationFn: (scanId: string) => connector.sessions.cancelSave(scanId),
  });
}

/** The scan RESUME: continue an incomplete session's scan (load + resume +
 * identity verify + run). Throws session-corrupt on an identity mismatch. */
export function useResumeSession() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string) => connector.sessions.resumeSession(path),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.sessions }),
  });
}

/** M-154: load the auto session the last-scan pointer names (the Home
 * "continue last" banner; falls back to the list on session-not-found). */
export function useResumeLastSession() {
  return useMutation({
    mutationFn: () => connector.sessions.resumeLastSession(),
  });
}

/** W-01 (the dead-letter fix): the live load fraction (0..1) for a session
 * file being loaded; null while idle. The bridge coalesces to 5% steps.
 * Keyed by path — a stale fraction never leaks across files. */
export function useSessionLoadFraction(path: string | null): number | null {
  const [state, setState] = useState<{ path: string; fraction: number } | null>(null);
  useEffect(() => {
    if (!path) return;
    return connector.sessions.onSessionLoadProgress(path, (event) => {
      setState({ path, fraction: event.permille / 1000 });
    });
  }, [path]);
  return state !== null && state.path === path ? state.fraction : null;
}

// ---------------------------------------------------------------------------
// SB-B-01: the honest Save-and-exit flow. The engine's session serializer
// walks the live result tree without the tree mutex, so a mid-run save
// would race the scan thread (the bridge truthfully refuses while busy);
// the engine's own partial-session mechanism is the stop tail (S-057/W1c
// — the og's stop→SaveScan parity). The flow: read the engine state →
// stop the live scan → wait for the terminal latch (bounded drain
// deadline, honest timeout error — never a hang, never a fake
// completion) → the REAL named save. Lives in the queries layer (arch-lint
// A1: the connector boundary).
// ---------------------------------------------------------------------------

/** Bounded stop-drain poll: the engine's OWN state until it leaves the
 * live set (an in-flight raw read on an unhealthy transport finishes or
 * fails before the cancel is observed — F-B-16's documented honest
 * limit; the deadline bounds it and the localized timeout message
 * surfaces the truth). */
async function waitForTerminal(
  scanId: string,
  timeoutMs: number,
  timeoutMessage: string,
): Promise<void> {
  const deadline = Date.now() + timeoutMs;
  for (;;) {
    let status: string | undefined;
    try {
      status = (await readScanState(scanId)).status;
    } catch {
      return; // scan gone (not-found) → nothing left to wait for
    }
    if (status === undefined || !ACTIVE_SCAN_STATUSES.has(status)) return;
    if (Date.now() >= deadline) {
      throw new Error(timeoutMessage);
    }
    await new Promise<void>((resolve) => setTimeout(resolve, 500));
  }
}

/** Save-and-exit: stop the live scan first, then the REAL named save
 * (the scanId is captured by the caller — the store's `active` clears
 * when the scan stops; the save must still address the SAME scan). */
export async function stopScanThenSaveSession(
  scanId: string,
  timeoutMs: number,
  timeoutMessage: string,
): Promise<SessionMeta> {
  let status: string | undefined;
  try {
    status = (await connector.scan.getScanState(scanId)).status;
  } catch {
    status = undefined; // scan gone → nothing live to stop
  }
  if (status !== undefined && ACTIVE_SCAN_STATUSES.has(status)) {
    try {
      await connector.scan.cancelScan(scanId);
    } catch (err) {
      // A cancel refusal only matters if the scan is still live (e.g. it
      // raced to terminal between the read and the stop).
      const now = await connector.scan
        .getScanState(scanId)
        .then((s) => s.status)
        .catch(() => undefined);
      if (now !== undefined && ACTIVE_SCAN_STATUSES.has(now)) throw err;
    }
    await waitForTerminal(scanId, timeoutMs, timeoutMessage);
  }
  return connector.sessions.saveSession(scanId, null);
}
