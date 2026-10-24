import { useEffect, useRef, useState } from 'react';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { RecoveryOptions, RecoveryProgressEvent } from '@/connector/contract';
import { queryKeys } from './client';

export function useStartRecovery(scanId: string | null) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (input: { nodeIds: readonly string[]; destination: string; options: RecoveryOptions }) =>
      connector.recovery.startRecovery(scanId!, input.nodeIds, input.destination, input.options),
    onSuccess: ({ jobId }) => {
      void qc.invalidateQueries({ queryKey: queryKeys.job(jobId) });
    },
  });
}

export function useRecoveryProgress(jobId: string | null): RecoveryProgressEvent | null {
  const [progress, setProgress] = useState<RecoveryProgressEvent | null>(null);
  const qc = useQueryClient();
  const lastStatus = useRef<string | null>(null);

  useEffect(() => {
    if (!jobId) {
      setProgress(null);
      return;
    }
    const unsubscribe = connector.recovery.onRecoveryProgress(jobId, (event) => {
      setProgress(event);
      if (event.status !== lastStatus.current) {
        lastStatus.current = event.status;
        void qc.invalidateQueries({ queryKey: queryKeys.job(jobId) });
        void qc.invalidateQueries({ queryKey: queryKeys.jobItems(jobId) });
        if (event.status !== 'running' && event.status !== 'validating') {
          void qc.invalidateQueries({ queryKey: queryKeys.report(jobId) });
        }
      }
    });
    return unsubscribe;
  }, [jobId, qc]);

  return progress;
}

export function useRecoveryJob(jobId: string | null) {
  return useQuery({
    queryKey: queryKeys.job(jobId ?? 'none'),
    queryFn: () => connector.recovery.getRecoveryState(jobId!),
    enabled: jobId !== null,
  });
}

export function useRecoveryItems(jobId: string | null) {
  return useQuery({
    queryKey: queryKeys.jobItems(jobId ?? 'none'),
    queryFn: () => connector.recovery.getRecoveryItems(jobId!, { offset: 0, limit: 500 }),
    enabled: jobId !== null,
  });
}

export function useCancelRecovery(jobId: string | null) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: () => connector.recovery.cancelRecovery(jobId!),
    onSuccess: () => {
      void qc.invalidateQueries({ queryKey: queryKeys.job(jobId!) });
      void qc.invalidateQueries({ queryKey: queryKeys.report(jobId!) });
    },
  });
}

export function useRecoveryReport(jobId: string | null) {
  return useQuery({
    queryKey: queryKeys.report(jobId ?? 'none'),
    queryFn: () => connector.recovery.getRecoveryReport(jobId!),
    enabled: jobId !== null,
  });
}
