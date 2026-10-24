import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import { queryKeys } from './client';

export function useDevices() {
  return useQuery({
    queryKey: queryKeys.devices,
    queryFn: () => connector.devices.listDevices(),
  });
}

export function useDeviceInfo(deviceId: string | null) {
  return useQuery({
    queryKey: queryKeys.device(deviceId ?? 'none'),
    queryFn: () => connector.devices.getDeviceInfo(deviceId!),
    enabled: deviceId !== null,
  });
}

export function useRefreshDevices() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: () => connector.devices.refreshDevices(),
    onSuccess: (devices) => {
      qc.setQueryData(queryKeys.devices, devices);
    },
  });
}
