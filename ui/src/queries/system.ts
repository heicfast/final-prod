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

// ---------------------------------------------------------------------------
// The user-mode posture (v4): TokenElevation probe + relaunch + user signs
// ---------------------------------------------------------------------------

export function useSecurityPosture() {
  return useQuery({
    queryKey: queryKeys.securityPosture,
    queryFn: () => connector.system.getSecurityPosture(),
    // The token cannot change for this process's lifetime — but a cheap
    // refetch-on-focus keeps the banner truthful after a relaunch attempt.
    staleTime: 60_000,
    retry: false,
  });
}

export function useRelaunchElevated() {
  return useMutation({
    mutationFn: () => connector.system.relaunchElevated(),
  });
}

export function useLoadUserSigns() {
  return useMutation({
    mutationFn: (path: string) => connector.system.loadUserSigns(path),
  });
}
