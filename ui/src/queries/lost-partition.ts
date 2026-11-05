import { useEffect as reactUseEffect } from 'react';
import { useMutation, useQuery } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { LostCandidateRow } from '@/connector/contract';
import { useConsentStore } from '@/stores/consent-store';

/**
 * The lost-partition domain queries (S-029). The apply/undo/save mutations
 * transcribe scan-state candidates onto the disk's table — consent-gated by
 * the bridge (the apply-consent feed mirrors the scan consent's bridge).
 */

export function useApplyLostPartition() {
  return useMutation({
    mutationFn: (input: {
      deviceId: string;
      candidate: LostCandidateRow;
      rebuildTable?: boolean;
    }) =>
      connector.lostPartition.applyLostPartition(
        input.deviceId,
        input.candidate,
        input.rebuildTable,
      ),
  });
}

export function useUndoLastApply() {
  return useMutation({
    mutationFn: (deviceId: string) => connector.lostPartition.undoLastApply(deviceId),
  });
}

export function useSaveApplyUndo() {
  return useMutation({
    mutationFn: (input: { deviceId: string; path: string }) =>
      connector.lostPartition.saveApplyUndo(input.deviceId, input.path),
  });
}

export function useApplyConsentPending(deviceId: string | null) {
  return useQuery({
    queryKey: ['lost-partition', deviceId ?? 'none', 'pending-consent'],
    queryFn: () => connector.lostPartition.pendingApplyConsent(deviceId ?? ''),
    enabled: deviceId !== null,
    // Poll-light: the event feed is the primary source; this is the
    // re-hydration fallback after navigation.
    refetchInterval: 4_000,
  });
}

/** The apply-consent event feed (subscribe before invoking apply; the
 *  store renders the shell modal). Mirrors scan's useConsentBridge. */
export function useApplyConsentBridge(deviceId: string | null): void {
  const setApply = useConsentStore((s) => s.setApply);
  reactUseEffect(() => {
    if (!deviceId) return;
    return connector.lostPartition.onApplyConsent(deviceId, (event) => {
      setApply(event);
    });
  }, [deviceId, setApply]);
}
