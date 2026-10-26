import { useMutation, useQuery } from '@tanstack/react-query';
import { connector } from '@/connector';
import { queryKeys } from './client';

export function useAppInfo() {
  return useQuery({
    queryKey: queryKeys.appInfo,
    queryFn: () => connector.system.getAppInfo(),
    staleTime: Infinity,
  });
}

export function useDriverStatus() {
  return useQuery({
    queryKey: queryKeys.driverStatus,
    queryFn: () => connector.system.getDriverStatus(),
    staleTime: Infinity,
  });
}

export function useMountPoints() {
  return useQuery({
    queryKey: queryKeys.mounts,
    queryFn: () => connector.system.listMountPoints(),
    staleTime: 30_000,
  });
}

/** `sourceDeviceId` is the scan TARGET (device or volume id) — the
 *  connector resolves volumes to their owning device for the overlap
 *  refusal (same-volume writes are a hard block, architecture/04 §7.3). */
export function useValidateDestination(sourceDeviceId: string | null) {
  return useMutation({
    mutationFn: (input: { path: string; neededBytes: number }) =>
      connector.system.validateDestination(input.path, sourceDeviceId ?? '', input.neededBytes),
  });
}

export function usePickDirectory() {
  return useMutation({
    mutationFn: (hint?: { volumeId?: string | null; excludeDeviceId?: string | null }) =>
      connector.system.pickDirectory(hint),
  });
}

/**
 * Open-location flow (the corpus context-menu item): resolve the node's real
 * on-disk path through the results detail, then reveal it in the system
 * explorer. Only existing-origin rows have a live location — the connector
 * refuses the rest truthfully (deleted/carved content is not on disk).
 */
export function useOpenLocation(scanId: string) {
  return useMutation({
    mutationFn: async (nodeId: string): Promise<void> => {
      const detail = await connector.results.getNodeDetail(scanId, nodeId);
      if (!detail.fullPhysicalPath) {
        throw Object.assign(
          new Error('This item has no on-disk location (deleted or carved content).'),
          { code: 'unknown', reasonKey: 'results.context.openLocationMissing' },
        );
      }
      await connector.system.openPath(detail.fullPhysicalPath);
    },
  });
}
