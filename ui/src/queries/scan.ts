import { useRef, useSyncExternalStore } from 'react';
import { useMutation, useQuery, useQueryClient, type QueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import type {
  ConsentRequestEvent,
  ScanParams,
  ScanProgressEvent,
  ScanState,
} from '@/connector/contract';
import { queryKeys } from './client';
import { useScanSessionStore } from '@/stores/scan-session-store';

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
  // NG-2: a time floor on results refetch while running. Bucket-only
  // refresh left the ENTIRE 40-50% band (the whole deep stage on large
  // volumes) without a single tree/results/count update even as nodes
  // arrived — the reference shows live-growing results during the scan.
  // 15 s bounds bridge load; bucket jumps still refresh immediately.
  const REFRESH_INTERVAL_MS = 15_000;
  let lastRefreshAt = 0;
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
    } else if (
      event.status === 'running' &&
      (bucket > lastBucket || Date.now() - lastRefreshAt >= REFRESH_INTERVAL_MS)
    ) {
      if (bucket > lastBucket) lastBucket = bucket;
      lastRefreshAt = Date.now();
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

// ---------------------------------------------------------------------------
// Consent store — the same external-store pattern, for the G-1 pre-scan
// repair prompt. SET by the consent-request event; CLEARED by the respond
// mutation's success (or its typed stale error — the page decides); primed
// from getScanState().pendingConsent (re-hydration: subscribe-late and
// navigate-away-and-back). The prime only FILLS — a pre-event poll's null
// must never race away a just-arrived request.
// ---------------------------------------------------------------------------

interface ScanConsentStore {
  getSnapshot: () => ConsentRequestEvent | null;
  subscribe: (listener: () => void) => () => void;
  clear: () => void;
}

const consentStores = new Map<string, ScanConsentStore>();

function getConsentStore(scanId: string): ScanConsentStore {
  const existing = consentStores.get(scanId);
  if (existing) return existing;
  let current: ConsentRequestEvent | null = null;
  const listeners = new Set<() => void>();
  const emit = (): void => {
    for (const l of listeners) l();
  };
  const unsubscribe = connector.scan.onConsentRequest(scanId, (event) => {
    current = event;
    emit();
  });
  void connector.scan
    .getScanState(scanId)
    .then((state: ScanState) => {
      if (current === null && state.pendingConsent != null) {
        current = state.pendingConsent;
        emit();
      }
    })
    .catch(() => undefined);
  const store: ScanConsentStore = {
    getSnapshot: () => current,
    subscribe: (listener) => {
      listeners.add(listener);
      return () => {
        listeners.delete(listener);
        if (listeners.size === 0) {
          unsubscribe();
          consentStores.delete(scanId);
        }
      };
    },
    clear: () => {
      if (current !== null) {
        current = null;
        emit();
      }
    },
  };
  consentStores.set(scanId, store);
  return store;
}

/** Clear a scan's pending-consent snapshot (no-op when absent — safe from
 *  the respond mutation when the prompt already unmounted). */
function clearConsentStore(scanId: string): void {
  consentStores.get(scanId)?.clear();
}

/** The pending consent request for the scan (null when none pends) — the
 *  prompt's data source. G-1 (a): event-driven with state re-hydration. */
export function useConsentRequest(scanId: string | null): ConsentRequestEvent | null {
  return useSyncExternalStore(
    scanId ? (listener) => getConsentStore(scanId).subscribe(listener) : noopSubscribe,
    scanId ? () => getConsentStore(scanId).getSnapshot() : () => NULL_SNAPSHOT,
    () => NULL_SNAPSHOT,
  );
}

/** Answer the pending consent (approve executes the transactional repair;
 * deny keeps the scan read-only). On success the store's snapshot clears
 * (the prompt dismisses); the typed consent-not-found (stale id: double
 * respond, cancel, scan ended) is handled by the page — it clears too. */
export function useRespondConsent(scanId: string | null) {
  return useMutation({
    mutationFn: ({ requestId, allow }: { requestId: string; allow: boolean }) =>
      connector.scan.respondToConsent(requestId, allow),
    onSuccess: () => {
      if (scanId) clearConsentStore(scanId);
    },
    onError: (err) => {
      if (scanId && (err as { code?: string }).code === 'consent-not-found') {
        clearConsentStore(scanId);
      }
    },
  });
}

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
      return status === 'running' || status === 'starting' ? 500 : false;
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
