/**
 * Imaging queries — the Create Image flow's React surface (lane C).
 *
 * The pattern mirrors queries/scan.ts: the progress stream is an
 * external store (one store per imageId, shared subscribers, unsubscribe
 * when the last listener goes), the state query is the fallback poll,
 * and the terminal event invalidates the query cache.
 */

import { useEffect as reactUseEffect, useSyncExternalStore } from 'react';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import type {
  ImageFinishedEvent,
  ImageJobState,
  ImageProgressEvent,
} from '@/connector/contract';

// ---------------------------------------------------------------------------
// Progress store — external-store pattern over the image event stream.
// ---------------------------------------------------------------------------

interface ImageProgressStore {
  getSnapshot: () => ImageProgressEvent | null;
  subscribe: (listener: () => void) => () => void;
}

const imageStores = new Map<string, ImageProgressStore>();

function getImageStore(imageId: string): ImageProgressStore {
  const existing = imageStores.get(imageId);
  if (existing) return existing;
  let current: ImageProgressEvent | null = null;
  const listeners = new Set<() => void>();
  const emit = (): void => {
    for (const l of listeners) l();
  };
  const unsubscribeProgress = connector.imaging.onImageProgress(imageId, (event) => {
    current = event;
    emit();
  });
  const store: ImageProgressStore = {
    getSnapshot: () => current,
    subscribe: (listener) => {
      listeners.add(listener);
      return () => {
        listeners.delete(listener);
        if (listeners.size === 0) {
          unsubscribeProgress();
          imageStores.delete(imageId);
        }
      };
    },
  };
  imageStores.set(imageId, store);
  return store;
}

const NULL_SNAPSHOT: null = null;

function noopSubscribe(): () => void {
  return () => undefined;
}

/** Live image-creation progress (the event stream; no polling). */
export function useImageProgress(imageId: string | null): ImageProgressEvent | null {
  const store = useSyncExternalStore(
    imageId ? getImageStore(imageId).subscribe : noopSubscribe,
    imageId ? getImageStore(imageId).getSnapshot : () => NULL_SNAPSHOT,
    () => NULL_SNAPSHOT,
  );
  return store;
}

/** Terminal-event bridge: completion surfaces INSTANTLY (the state query
 * confirms; nothing is invented here). */
export function useImageFinishedBridge(imageId: string | null): void {
  const qc = useQueryClient();
  reactUseEffect(() => {
    if (!imageId) return;
    return connector.imaging.onImageFinished(imageId, (_event: ImageFinishedEvent) => {
      void qc.invalidateQueries({ queryKey: ['imaging', imageId, 'state'] });
    });
  }, [imageId, qc]);
}

/** The state query (the fallback poll while the job runs). */
export function useImageState(imageId: string | null) {
  return useQuery({
    queryKey: ['imaging', imageId ?? 'none', 'state'],
    queryFn: (): Promise<ImageJobState> => connector.imaging.getImageState(imageId!),
    enabled: imageId !== null,
    refetchInterval: (query) => {
      const status = query.state.data?.status;
      return status === 'running' ? 1_000 : false;
    },
  });
}

/** Start an image creation (the dialog's action). */
export function useStartImageCreation() {
  return useMutation({
    mutationFn: (input: { deviceId: string; path: string }) =>
      connector.imaging.startImageCreation(input.deviceId, input.path),
  });
}

// ---------------------------------------------------------------------------
// The sector map — the AnalysisGridWidget surface (lane C).
// ---------------------------------------------------------------------------

/** The map query: bounded cells, refetch only while the scan runs (the
 * map is a coarse view; 2.5 s is smooth without cache thrash). */
export function useDiskMap(scanId: string | null, live: boolean, maxCells = 1024) {
  return useQuery({
    queryKey: ['scan', scanId ?? 'none', 'disk-map', maxCells],
    queryFn: () => connector.scan.getDiskMap(scanId!, { maxCells }),
    enabled: scanId !== null,
    refetchInterval: () => (live ? 2_500 : false),
  });
}

// ---------------------------------------------------------------------------
// The recoverability verdict — the S-023 assistant surface (lane C).
// ---------------------------------------------------------------------------

/** The assistant-stats query: valid at completion (a running scan throws
 * scan-not-running — the typed honest answer — so the query enables only
 * for terminal scans). */
export function useScanAssistant(scanId: string | null, completed: boolean) {
  return useQuery({
    queryKey: ['scan', scanId ?? 'none', 'assistant'],
    queryFn: () => connector.scan.getScanAssistantStats(scanId!),
    enabled: scanId !== null && completed,
    staleTime: Infinity, // the grading pass runs once at completion
    retry: false, // scan-not-running for a not-yet-graded scan is the
    // honest typed answer, not a transient failure to retry
  });
}
