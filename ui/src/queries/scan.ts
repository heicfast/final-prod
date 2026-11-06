import { useEffect as reactUseEffect, useRef, useState, useSyncExternalStore } from 'react';
import { useMutation, useQuery, useQueryClient, type QueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { ScanParams, ScanProgressEvent, ScanState } from '@/connector/contract';
import { queryKeys } from './client';
import { useScanSessionStore } from '@/stores/scan-session-store';
import { useConsentStore } from '@/stores/consent-store';

// ---------------------------------------------------------------------------
// Progress store — external-store pattern over the connector event stream.
// One store per scanId; subscribers share the raw event; the store also
// drives query-cache invalidations on status changes.
// ---------------------------------------------------------------------------

interface ScanProgressStore {
  getSnapshot: () => ScanProgressEvent | null;
  subscribe: (listener: () => void) => () => void;
}

const scanStores = new Map<string, ScanProgressStore>();

function getScanStore(scanId: string, qc: () => QueryClient): ScanProgressStore {
  const existing = scanStores.get(scanId);
  if (existing) return existing;
  let current: ScanProgressEvent | null = null;
  const listeners = new Set<() => void>();
  let lastStatus: string | null = null;
  let lastBucket = -1;
  const emit = (): void => {
    for (const l of listeners) l();
  };
  const unsubscribe = connector.scan.onScanProgress(scanId, (event) => {
    current = event;
    const invalidate = qc() as {
      invalidateQueries: (q: { queryKey: readonly unknown[] }) => Promise<unknown>;
    };
    const bucket = Math.floor(event.overallProgress * 10);
    if (event.status !== lastStatus) {
      lastStatus = event.status;
      void invalidate.invalidateQueries({ queryKey: queryKeys.scanState(scanId) });
      if (
        event.status === 'completed' ||
        event.status === 'cancelled' ||
        event.status === 'failed'
      ) {
        void invalidate.invalidateQueries({ queryKey: ['scan', scanId, 'tree'] });
        void invalidate.invalidateQueries({ queryKey: ['scan', scanId, 'results'] });
        void invalidate.invalidateQueries({ queryKey: ['scan', scanId, 'count'] });
        void invalidate.invalidateQueries({ queryKey: queryKeys.sessions });
      }
    } else if (event.status === 'running' && bucket > lastBucket) {
      lastBucket = bucket;
      void invalidate.invalidateQueries({ queryKey: ['scan', scanId, 'tree'] });
      void invalidate.invalidateQueries({ queryKey: ['scan', scanId, 'results'] });
      void invalidate.invalidateQueries({ queryKey: ['scan', scanId, 'count'] });
    }
    emit();
  });
  // prime from state asynchronously (arrival triggers a re-render via emit)
  void connector.scan
    .getScanState(scanId)
    .then((state: ScanState) => {
      if (current === null) {
        current = state.progress;
        emit();
      }
    })
    .catch(() => undefined);
  const store: ScanProgressStore = {
    getSnapshot: () => current,
    subscribe: (listener) => {
      listeners.add(listener);
      return () => {
        listeners.delete(listener);
        if (listeners.size === 0) {
          unsubscribe();
          scanStores.delete(scanId);
        }
      };
    },
  };
  scanStores.set(scanId, store);
  return store;
}

const NULL_SNAPSHOT: null = null;

export function useStartScan() {
  const qc = useQueryClient();
  const setActive = useScanSessionStore((s) => s.setActiveScan);
  return useMutation({
    mutationFn: (params: ScanParams) => connector.scan.startScan(params),
    onSuccess: ({ scanId }, params) => {
      setActive({ scanId, targetId: params.targetId });
      void qc.invalidateQueries({ queryKey: ['scan', scanId] });
    },
  });
}

export function useScanState(scanId: string | null) {
  return useQuery({
    queryKey: queryKeys.scanState(scanId ?? 'none'),
    queryFn: () => connector.scan.getScanState(scanId!),
    enabled: scanId !== null,
    refetchInterval: (query) => {
      const status = query.state.data?.status;
      // 'stopping' MUST keep the poll alive (v0.8.0 forensic, runs
      // 42178485694 + 42176600928): the engine's cancel is observed by
      // the run thread only at its next checkpoint, so the state reads
      // 'stopping' for the whole drain window. Stopping the poll there
      // froze the query at 'stopping' FOREVER — the "stopped early"
      // partial-results banner (gated on status === 'cancelled') could
      // never render after a stop, even though the thread had long
      // returned and the save (busy=0) already worked. The TEST DOUBLE's
      // synchronous cancel never showed a 'stopping' phase, which is why
      // every unit/e2e proof passed while the real product never showed
      // the banner.
      return status === 'running' || status === 'starting' || status === 'stopping'
        ? 500
        : false;
    },
  });
}

/**
 * Live progress via the connector event stream (external store — no polling,
 * no setState-in-effect). The 100–250 ms mock ticks render directly.
 */
export function useScanProgress(scanId: string | null): ScanProgressEvent | null {
  const qc = useQueryClient();
  const getQueryClient = useRef(() => qc); // ref: stable getter; call via .current()
  const store = useSyncExternalStore(
    scanId
      ? (listener) => getScanStore(scanId, () => getQueryClient.current()).subscribe(listener)
      : noopSubscribe,
    scanId
      ? () => getScanStore(scanId, () => getQueryClient.current()).getSnapshot()
      : () => NULL_SNAPSHOT,
    () => NULL_SNAPSHOT,
  );
  return store;
}

function noopSubscribe(): () => void {
  return () => undefined;
}

export function usePauseScan(scanId: string | null) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: () => connector.scan.pauseScan(scanId!),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId!) }),
  });
}

export function useResumeScan(scanId: string | null) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: () => connector.scan.resumeScan(scanId!),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId!) }),
  });
}

export function useCancelScan(scanId: string | null) {
  const qc = useQueryClient();
  const markStopped = useScanSessionStore((s) => s.markScanStopped);
  return useMutation({
    mutationFn: () => connector.scan.cancelScan(scanId!),
    onSuccess: () => {
      markStopped();
      void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId!) });
      void qc.invalidateQueries({ queryKey: queryKeys.sessions });
    },
  });
}

// ---------------------------------------------------------------------------
// The consent bridge (G-1 (a)) — the global store fed by the scan's consent
// event + state re-hydration. Mounted by the scan-progress surface (where
// the scanId lives); the MODAL lives in the shell (navigation-safe).
// ---------------------------------------------------------------------------

export function useConsentBridge(scanId: string | null): void {
  const setRepair = useConsentStore((s) => s.setRepair);
  const { data: state } = useScanState(scanId);
  reactUseEffect(() => {
    if (!scanId) return;
    return connector.scan.onConsentRequest(scanId, (event) => {
      setRepair(event);
    });
  }, [scanId, setRepair]);
  // Re-hydration: the pending consent from state covers subscribe-late and
  // navigate-away-and-back (the event may have fired before this mount).
  reactUseEffect(() => {
    if (state?.pendingConsent) {
      setRepair(state.pendingConsent);
    }
  }, [state?.pendingConsent, setRepair]);
}

/** Answer a consent request (echo token; both families share the command). */
export function useRespondToConsent() {
  return useMutation({
    mutationFn: (input: { requestId: string; allow: boolean }) =>
      connector.scan.respondToConsent(input.requestId, input.allow),
  });
}

// ---------------------------------------------------------------------------
// W-01 — the dead-letter bridges. The bridge (and the mock) emit
// scan-finished / save-progress / lost-candidate / scan-autosaved; these
// hooks are the UI's subscription surface (the 500 ms state poll is the
// FALLBACK, not the primary).
// ---------------------------------------------------------------------------

/** Terminal-event bridge: completion surfaces INSTANTLY (cache
 * invalidations mirror the poller's terminal path — the poll then
 * confirms; no state is invented here). */
export function useScanFinishedBridge(scanId: string | null): void {
  const qc = useQueryClient();
  reactUseEffect(() => {
    if (!scanId) return;
    return connector.scan.onScanFinished(scanId, () => {
      void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId) });
      void qc.invalidateQueries({ queryKey: ['scan', scanId, 'tree'] });
      void qc.invalidateQueries({ queryKey: ['scan', scanId, 'results'] });
      void qc.invalidateQueries({ queryKey: ['scan', scanId, 'count'] });
      void qc.invalidateQueries({ queryKey: queryKeys.sessions });
    });
  }, [scanId, qc]);
}

/** Live save fraction (0..1) while a session save runs; null when idle.
 * The state is keyed by scanId (a stale fraction from a previous scan can
 * never leak across ids — no synchronous reset in the effect body). */
export function useSaveProgressFraction(scanId: string | null): number | null {
  const [state, setState] = useState<{ scanId: string; fraction: number } | null>(null);
  const settle = useRef<number | undefined>(undefined);
  reactUseEffect(() => {
    if (!scanId) return;
    const clear = (): void => {
      clearTimeout(settle.current);
      setState(null);
    };
    const unsubscribe = connector.scan.onSaveProgress(scanId, (event) => {
      setState({ scanId, fraction: event.fraction });
      if (event.fraction >= 1) {
        // hold the full bar briefly, then reset (the save mutation's own
        // completion resets the button anyway).
        clearTimeout(settle.current);
        settle.current = setTimeout(clear, 600) as unknown as number;
      }
    });
    return () => {
      unsubscribe();
      clearTimeout(settle.current);
    };
  }, [scanId]);
  return state !== null && state.scanId === scanId ? state.fraction : null;
}

/** Lost-candidate bridge: the live results tree grows while scanning
 * (browse-while-scan, reference parity). Invalidation is throttled to one
 * per second — the engine can emit hundreds of candidates on a carved
 * disk and the query cache must not thrash. */
export function useLostCandidateBridge(scanId: string | null): void {
  const qc = useQueryClient();
  reactUseEffect(() => {
    if (!scanId) return;
    let pending = false;
    const flush = (): void => {
      pending = false;
      void qc.invalidateQueries({ queryKey: ['scan', scanId, 'tree'] });
      void qc.invalidateQueries({ queryKey: ['scan', scanId, 'count'] });
    };
    const timer = setInterval(() => {
      if (pending) flush();
    }, 1000);
    const unsubscribe = connector.scan.onLostCandidate(scanId, () => {
      pending = true;
    });
    return () => {
      unsubscribe();
      clearInterval(timer);
    };
  }, [scanId, qc]);
}

/** The last milestone auto-save (the scan page's honest autosave stamp).
 * Keyed by scanId — a stamp from a previous scan never leaks. */
export function useScanAutosaveStamp(scanId: string | null): { rc: number; at: string } | null {
  const [state, setState] = useState<{ scanId: string; rc: number; at: string } | null>(null);
  reactUseEffect(() => {
    if (!scanId) return;
    return connector.scan.onScanAutosaved(scanId, (event) => {
      setState({ scanId, rc: event.rc, at: new Date().toLocaleTimeString() });
    });
  }, [scanId]);
  return state !== null && state.scanId === scanId ? { rc: state.rc, at: state.at } : null;
}

/** One fresh, uncached engine read of a scan's state (the exit guard's
 * authority — the store can lag the engine because navigation unmounts
 * the poller; the engine is the truth). */
export function readScanState(scanId: string): Promise<ScanState> {
  return connector.scan.getScanState(scanId);
}
