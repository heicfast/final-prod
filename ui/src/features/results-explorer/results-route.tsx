import * as React from 'react';
import { useParams } from 'react-router';
import { useScanState } from '@/queries/scan';
import { ResultsExplorer } from './results-explorer';
import { ResultsPageFallback } from './page-fallback';
import { Skeleton } from '@/components/ui/scroll-area';

/**
 * Results route guard — verifies the scan session exists via the connector
 * (live or loaded), otherwise falls back to Home with no crash. The 3-pane
 * explorer itself renders for both live scans and saved sessions.
 */
export function ResultsRoute(): React.JSX.Element {
  const { scanId = '' } = useParams();
  const state = useScanState(scanId || null);

  if (!scanId) return <ResultsPageFallback />;

  if (state.isLoading) {
    return (
      <div className="space-y-2 p-4">
        <Skeleton className="h-11" />
        <div className="flex gap-2">
          <Skeleton className="h-96 w-56" />
          <Skeleton className="h-96 flex-1" />
          <Skeleton className="hidden h-96 w-80 xl:block" />
        </div>
      </div>
    );
  }

  if (state.isError || !state.data) {
    return <ResultsPageFallback />;
  }

  const targetId = state.data.params.targetId;
  return <ResultsExplorer scanId={scanId} targetId={targetId} liveMode={false} />;
}
