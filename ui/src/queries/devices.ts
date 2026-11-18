import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import { queryKeys } from './client';

export function useDevices() {
  return useQuery({
    queryKey: queryKeys.devices,
    queryFn: () => connector.devices.listDevices(),
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

/** N-064: the explicit-consent dictionary unlock for a locked volume. */
export function useUnlockVolume() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (input: { volumeId: string; passwords: string[] }) =>
      connector.devices.unlockVolume(input.volumeId, input.passwords),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.devices }),
  });
}

/** M04/BF-12: import a disk-image file as a scannable target (the og's
 * "Import a disk image" flow). The devices query invalidates so the row
 * lands in the list; the returned row is the scan target. */
export function useImportImageFile() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string) => connector.devices.importImageFile(path),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.devices }),
  });
}
