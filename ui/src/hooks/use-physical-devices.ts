import { useSearchParams } from 'react-router';
import { useDevices } from '@/queries/devices';
import type { Device } from '@/connector/contract';

/**
 * Physical devices (drives/cards) for pickers — excludes the "Specify a
 * location" virtual targets (Recycle Bin / Desktop), which are scan
 * entries, not hardware surfaces. Supports `?device=` preselection and
 * falls back to the first physical device.
 */
export function usePhysicalDevices(): {
  devices: Device[];
  selectedId: string;
  setSelectedId: (id: string) => void;
  isLoading: boolean;
  isError: boolean;
  refetch: () => void;
} {
  const [params, setParams] = useSearchParams();
  const query = useDevices();
  // D-ENUM-01 (run 44249495727's evidence): every device the payload
  // carries is a physically-enumerated disk — the bridge's bus-kind arm
  // (96917f6c, D-ENUM-05) no longer classifies file-backed disks as
  // 'virtual', and this picker-side filter would re-hide them for the
  // repair/health pages alone. The shell locations (Recycle Bin /
  // Desktop quick-targets) are hardcoded UI rows, never payload rows —
  // 'virtual' can no longer reach here. When BF-12's image targets join
  // the payload, their visibility must be a DELIBERATE decision, never
  // a silent filter (the audit-trail standard the other consumers carry).
  const devices = query.data ?? [];
  const paramId = params.get('device');
  const selectedId =
    paramId && devices.some((d) => d.id === paramId) ? paramId : (devices[0]?.id ?? '');
  return {
    devices,
    selectedId,
    setSelectedId: (id) => {
      const next = new URLSearchParams(params);
      next.set('device', id);
      setParams(next, { replace: true });
    },
    isLoading: query.isLoading,
    isError: query.isError,
    refetch: () => void query.refetch(),
  };
}
