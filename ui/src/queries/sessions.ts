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
