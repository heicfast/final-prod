import { create } from 'zustand';

/**
 * Selection state for the results explorer. Set-backed so large selections
 * (100k+ nodes) stay cheap. Anchor supports shift-range selection.
 */
interface SelectionState {
  selected: ReadonlySet<string>;
  anchor: string | null;
  toggle: (id: string) => void;
  add: (ids: readonly string[]) => void;
  remove: (ids: readonly string[]) => void;
  setOnly: (ids: readonly string[]) => void;
  setAnchor: (id: string | null) => void;
  clear: () => void;
}

function clone(selected: ReadonlySet<string>): Set<string> {
  return new Set(selected);
}

export const useSelectionStore = create<SelectionState>((set) => ({
  selected: new Set<string>(),
  anchor: null,
  toggle: (id) =>
    set((state) => {
      const next = clone(state.selected);
      if (next.has(id)) next.delete(id);
      else next.add(id);
      return { selected: next, anchor: id };
    }),
  add: (ids) =>
    set((state) => {
      const next = clone(state.selected);
      for (const id of ids) next.add(id);
      return { selected: next };
    }),
  remove: (ids) =>
    set((state) => {
      const next = clone(state.selected);
      for (const id of ids) next.delete(id);
      return { selected: next };
    }),
  setOnly: (ids) =>
    set(() => ({
      selected: new Set(ids),
      anchor: null,
    })),
  setAnchor: (id) => set({ anchor: id }),
  clear: () => set({ selected: new Set<string>(), anchor: null }),
}));
