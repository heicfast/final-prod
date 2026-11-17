import { useQuery } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { DiskHealth } from '@/connector/contract';
import { queryKeys } from './client';

export function useHealth(deviceId: string | null) {
  return useQuery({
    queryKey: queryKeys.health(deviceId ?? 'none'),
    queryFn: () => connector.health.getHealth(deviceId!),
    enabled: deviceId !== null,
    staleTime: 60_000,
    retry: false,
  });
}

/**
 * One-shot health fetch outside the query cache (arch-lint A1: features
 * reach the connector through this layer only). The disk-health window's
 * open-time all-disks probe uses it to find the first Caution/Bad disk
 * (firstCautionIndex parity) — health-unavailable disks surface as
 * rejections there and are treated as Unknown, never an error.
 */
export function fetchHealth(deviceId: string): Promise<DiskHealth> {
  return connector.health.getHealth(deviceId);
}
