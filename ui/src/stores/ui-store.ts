import { create } from 'zustand';
import { createLogger } from '@/lib/logger';

const logger = createLogger('ui-store');

export type ResultsViewMode = 'details' | 'icons';

interface UiState {
  /** Results list presentation. */
  viewMode: ResultsViewMode;
  /** Preview pane width (px) in the results layout (corpus: QSplitter,
   *  always present, drag-resizable). */
  previewPaneWidth: number;
  /** Dev panel (dev builds only). */
  devPanelOpen: boolean;
  perfOverlayVisible: boolean;
  /** Watchdog install flag (once). */
  setViewMode: (mode: ResultsViewMode) => void;
  setPreviewPaneWidth: (w: number) => void;
  setDevPanelOpen: (open: boolean) => void;
  toggleDevPanel: () => void;
  setPerfOverlay: (visible: boolean) => void;
}

export const useUiStore = create<UiState>((set) => ({
  viewMode: 'details',
  previewPaneWidth: 320,
  devPanelOpen: false,
  perfOverlayVisible: false,
  setViewMode: (mode) => {
    logger.debug('view-mode', { mode });
    set({ viewMode: mode });
  },
  setPreviewPaneWidth: (w) =>
    set({ previewPaneWidth: Math.min(560, Math.max(240, Math.round(w))) }),
  setDevPanelOpen: (open) => set({ devPanelOpen: open }),
  toggleDevPanel: () => set((s) => ({ devPanelOpen: !s.devPanelOpen })),
  setPerfOverlay: (visible) => set({ perfOverlayVisible: visible }),
}));
