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

export function useValidateDestination(sourceDeviceId: string) {
  return useMutation({
    mutationFn: (input: { path: string; neededBytes: number }) =>
      connector.system.validateDestination(input.path, sourceDeviceId, input.neededBytes),
  });
}

export function usePickDirectory() {
  return useMutation({
    mutationFn: (hint?: { volumeId?: string | null; excludeDeviceId?: string | null }) =>
      connector.system.pickDirectory(hint),
  });
}
