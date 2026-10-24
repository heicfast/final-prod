import { useQuery } from '@tanstack/react-query';
import { connector } from '@/connector';
import type { PreviewKind } from '@/connector/contract';
import { queryKeys } from './client';

export function usePreview(scanId: string | null, nodeId: string | null, kind: PreviewKind | null) {
  return useQuery({
    queryKey: queryKeys.preview(scanId ?? 'none', nodeId ?? 'none', kind ?? 'none'),
    queryFn: () => connector.preview.requestPreview(scanId!, nodeId!, kind!),
    enabled: scanId !== null && nodeId !== null && kind !== null,
    staleTime: 60_000,
    retry: false, // preview failures are terminal diagnostics, not transient
  });
}
