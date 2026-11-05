/**
 * The consent store — the single UI-side state for BOTH consent families
 * (G-1 (a) pre-scan repair, S-029 lost-partition apply).
 *
 * The engine blocks its OWN run/apply thread until a decision crosses; the
 * prompt is NON-DISMISSABLE (Escape/outside-click prevented — a non-answer
 * is a user decision, plan §4.5; the scan's Stop button force-denies via
 * cancel). The store is global so navigation never abandons a pending
 * prompt: re-hydration comes from getScanState().pendingConsent /
 * pendingApplyConsent() when the event fired before a page subscribed.
 */
import { create } from 'zustand';
import type { ApplyConsentEvent, ConsentRequestEvent } from '@/connector/contract';

interface ConsentState {
  repair: ConsentRequestEvent | null;
  apply: ApplyConsentEvent | null;
  setRepair: (event: ConsentRequestEvent | null) => void;
  setApply: (event: ApplyConsentEvent | null) => void;
  clearRepair: (requestId: string) => void;
  clearApply: (requestId: string) => void;
}

export const useConsentStore = create<ConsentState>((set) => ({
  repair: null,
  apply: null,
  setRepair: (event) => set({ repair: event }),
  setApply: (event) => set({ apply: event }),
  clearRepair: (requestId) =>
    set((s) => (s.repair?.requestId === requestId ? { repair: null } : s)),
  clearApply: (requestId) =>
    set((s) => (s.apply?.requestId === requestId ? { apply: null } : s)),
}));
