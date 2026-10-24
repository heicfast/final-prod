import { useEffect, useRef, useState } from 'react';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { ScanParams, ScanProgressEvent, ScanState } from '@/connector/contract';
import { queryKeys } from './client';
import { useScanSessionStore } from '@/stores/scan-session-store';

export function useStartScan() {
  const qc = useQueryClient();
  const setActive = useScanSessionStore((s) => s.setActiveScan);
  return useMutation({
    mutationFn: (params: ScanParams) => connector.scan.startScan(params),
    onSuccess: ({ scanId }, params) => {
      setActive({ scanId, targetId: params.targetId, mode: params.mode });
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
 * Live progress via the connector event stream. Uses an external-store
 * subscription (not polling) so the 100–250 ms mock ticks render without
 * extra query churn; the query cache gets a final refresh on status change.
 */
export function useScanProgress(scanId: string | null): ScanProgressEvent | null {
  const [progress, setProgress] = useState<ScanProgressEvent | null>(null);
  const qc = useQueryClient();
  const lastStatus = useRef<string | null>(null);

  useEffect(() => {
    if (!scanId) {
      setProgress(null);
      return;
    }
    const unsubscribe = connector.scan.onScanProgress(scanId, (event) => {
      setProgress(event);
      if (event.status !== lastStatus.current) {
        lastStatus.current = event.status;
        void qc.invalidateQueries({ queryKey: queryKeys.scanState(scanId) });
        if (event.status === 'completed' || event.status === 'cancelled' || event.status === 'failed') {
          void qc.invalidateQueries({ queryKey: ['scan', scanId, 'tree'] });
          void qc.invalidateQueries({ queryKey: ['scan', scanId, 'results'] });
          void qc.invalidateQueries({ queryKey: queryKeys.sessions });
        }
      }
    });
    // prime immediately from state
    connector.scan.getScanState(scanId).then((state: ScanState) => {
      setProgress(state.progress);
    }).catch(() => undefined);
    return unsubscribe;
  }, [scanId, qc]);

  return progress;
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
