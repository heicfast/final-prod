import { create } from 'zustand';
import { createLogger } from '@/lib/logger';

const logger = createLogger('ui-store');

export type ResultsViewMode = 'details' | 'icons';

interface UiState {
  /** Results list presentation. */
  viewMode: ResultsViewMode;
  /** Preview pane visibility + width (px) in the results layout. */
  previewPaneVisible: boolean;
  previewPaneWidth: number;
  /** Dev panel (dev builds only). */
  devPanelOpen: boolean;
  perfOverlayVisible: boolean;
  /** Watchdog install flag (once). */
  setViewMode: (mode: ResultsViewMode) => void;
  togglePreviewPane: () => void;
  setPreviewPaneWidth: (w: number) => void;
  setDevPanelOpen: (open: boolean) => void;
  toggleDevPanel: () => void;
  setPerfOverlay: (visible: boolean) => void;
}

export const useUiStore = create<UiState>((set) => ({
  viewMode: 'details',
  previewPaneVisible: true,
  previewPaneWidth: 320,
  devPanelOpen: false,
  perfOverlayVisible: false,
  setViewMode: (mode) => {
    logger.debug('view-mode', { mode });
    set({ viewMode: mode });
  },
  togglePreviewPane: () => set((s) => ({ previewPaneVisible: !s.previewPaneVisible })),
  setPreviewPaneWidth: (w) =>
    set({ previewPaneWidth: Math.min(560, Math.max(240, Math.round(w))) }),
  setDevPanelOpen: (open) => set({ devPanelOpen: open }),
  toggleDevPanel: () => set((s) => ({ devPanelOpen: !s.devPanelOpen })),
  setPerfOverlay: (visible) => set({ perfOverlayVisible: visible }),
}));
