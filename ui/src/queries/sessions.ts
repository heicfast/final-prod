import { useEffect, useState } from 'react';
import { useMutation, useQuery, useQueryClient } from '@tanstack/react-query';
import { connector } from '@/connector';
import { queryKeys } from './client';

export function useSessions() {
  return useQuery({
    queryKey: queryKeys.sessions,
    queryFn: () => connector.sessions.listSessions(),
  });
}

export function useSaveSession(scanId: string | null) {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string | null) => connector.sessions.saveSession(scanId!, path),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.sessions }),
  });
}

export function useLoadSession() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string) => connector.sessions.loadSession(path),
    onSuccess: ({ scanId }) => {
      void qc.invalidateQueries({ queryKey: ['scan', scanId] });
      void qc.invalidateQueries({ queryKey: queryKeys.sessions });
    },
  });
}

export function useCancelSave() {
  return useMutation({
    mutationFn: (scanId: string) => connector.sessions.cancelSave(scanId),
  });
}

/** The scan RESUME: continue an incomplete session's scan (load + resume +
 * identity verify + run). Throws session-corrupt on an identity mismatch. */
export function useResumeSession() {
  const qc = useQueryClient();
  return useMutation({
    mutationFn: (path: string) => connector.sessions.resumeSession(path),
    onSuccess: () => void qc.invalidateQueries({ queryKey: queryKeys.sessions }),
  });
}

/** M-154: load the auto session the last-scan pointer names (the Home
 * "continue last" banner; falls back to the list on session-not-found). */
export function useResumeLastSession() {
  return useMutation({
    mutationFn: () => connector.sessions.resumeLastSession(),
  });
}

/** W-01 (the dead-letter fix): the live load fraction (0..1) for a session
 * file being loaded; null while idle. The bridge coalesces to 5% steps.
 * Keyed by path — a stale fraction never leaks across files. */
export function useSessionLoadFraction(path: string | null): number | null {
  const [state, setState] = useState<{ path: string; fraction: number } | null>(null);
  useEffect(() => {
    if (!path) return;
    return connector.sessions.onSessionLoadProgress(path, (event) => {
      setState({ path, fraction: event.permille / 1000 });
    });
  }, [path]);
  return state !== null && state.path === path ? state.fraction : null;
}
