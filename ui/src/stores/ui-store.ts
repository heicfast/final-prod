import { create } from 'zustand';
import { createLogger } from '@/lib/logger';

const logger = createLogger('ui-store');

export type ResultsViewMode = 'details' | 'icons';

const SIDEBAR_KEY = 'rw.ui.sidebarCollapsed';
const VIEW_MODE_KEY = 'rw.ui.viewMode';
const PREVIEW_WIDTH_KEY = 'rw.ui.previewPaneWidth';

function readStoredSidebarCollapsed(): boolean {
  try {
    return localStorage.getItem(SIDEBAR_KEY) === '1';
  } catch {
    /* storage unavailable */
  }
  return false;
}

/** T14 (the persistence audit): the results presentation and the preview
 * pane width are user preferences — they survive restarts like the
 * theme/density/locale/sidebar settings (the reference's own Qt settings
 * persisted both). */
function readStoredViewMode(): ResultsViewMode {
  try {
    return localStorage.getItem(VIEW_MODE_KEY) === 'icons' ? 'icons' : 'details';
  } catch {
    /* storage unavailable */
  }
  return 'details';
}

function readStoredPreviewPaneWidth(): number {
  try {
    const v = Number(localStorage.getItem(PREVIEW_WIDTH_KEY));
    if (Number.isFinite(v) && v >= 240 && v <= 560) return Math.round(v);
  } catch {
    /* storage unavailable */
  }
  return 320;
}

interface UiState {
  /** Results list presentation. */
  viewMode: ResultsViewMode;
  /** Preview pane width (px) in the results layout (corpus: QSplitter,
   *  always present, drag-resizable). */
  previewPaneWidth: number;
  /** Sidebar collapse (app extension per the session mandate; persisted
   *  across sessions like the theme/density preferences). */
  sidebarCollapsed: boolean;
  /** Dev panel (dev builds only). */
  devPanelOpen: boolean;
  perfOverlayVisible: boolean;
  /** Watchdog install flag (once). */
  setViewMode: (mode: ResultsViewMode) => void;
  setPreviewPaneWidth: (w: number) => void;
  setSidebarCollapsed: (collapsed: boolean) => void;
  toggleSidebar: () => void;
  setDevPanelOpen: (open: boolean) => void;
  toggleDevPanel: () => void;
  setPerfOverlay: (visible: boolean) => void;
}

export const useUiStore = create<UiState>((set) => ({
  viewMode: readStoredViewMode(),
  previewPaneWidth: readStoredPreviewPaneWidth(),
  sidebarCollapsed: readStoredSidebarCollapsed(),
  devPanelOpen: false,
  perfOverlayVisible: false,
  setViewMode: (mode) => {
    logger.debug('view-mode', { mode });
    try {
      localStorage.setItem(VIEW_MODE_KEY, mode);
    } catch {
      /* storage unavailable */
    }
    set({ viewMode: mode });
  },
  setPreviewPaneWidth: (w) => {
    const clamped = Math.min(560, Math.max(240, Math.round(w)));
    try {
      localStorage.setItem(PREVIEW_WIDTH_KEY, String(clamped));
    } catch {
      /* storage unavailable */
    }
    set({ previewPaneWidth: clamped });
  },
  setSidebarCollapsed: (collapsed) => {
    logger.debug('sidebar-collapsed', { collapsed });
    try {
      localStorage.setItem(SIDEBAR_KEY, collapsed ? '1' : '0');
    } catch {
      /* storage unavailable */
    }
    set({ sidebarCollapsed: collapsed });
  },
  toggleSidebar: () =>
    set((s) => {
      const collapsed = !s.sidebarCollapsed;
      try {
        localStorage.setItem(SIDEBAR_KEY, collapsed ? '1' : '0');
      } catch {
        /* storage unavailable */
      }
      return { sidebarCollapsed: collapsed };
    }),
  setDevPanelOpen: (open) => set({ devPanelOpen: open }),
  toggleDevPanel: () => set((s) => ({ devPanelOpen: !s.devPanelOpen })),
  setPerfOverlay: (visible) => set({ perfOverlayVisible: visible }),
}));
