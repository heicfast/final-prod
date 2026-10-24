import { beforeEach, describe, expect, it } from 'vitest';
import { useSelectionStore } from '@/stores/selection-store';
import { useFilterStore, dateRangeOf, sizeRangeOf } from '@/stores/filter-store';
import { useSortStore } from '@/stores/filter-store';

describe('selection store', () => {
  beforeEach(() => {
    useSelectionStore.getState().clear();
  });

  it('toggles membership and tracks anchor', () => {
    const s = useSelectionStore.getState();
    s.toggle('a');
    s.toggle('b');
    expect(useSelectionStore.getState().selected.size).toBe(2);
    expect(useSelectionStore.getState().anchor).toBe('b');
    s.toggle('b');
    expect(useSelectionStore.getState().selected.has('b')).toBe(false);
  });

  it('setOnly replaces the selection and resets the anchor', () => {
    useSelectionStore.getState().setOnly(['x', 'y', 'z']);
    expect(useSelectionStore.getState().selected.size).toBe(3);
    expect(useSelectionStore.getState().anchor).toBeNull();
  });

  it('add/remove operate in bulk', () => {
    const s = useSelectionStore.getState();
    s.add(['1', '2', '3']);
    s.remove(['2']);
    const cur = useSelectionStore.getState().selected;
    expect(cur.has('1')).toBe(true);
    expect(cur.has('2')).toBe(false);
    expect(cur.size).toBe(2);
  });
});

describe('filter store', () => {
  beforeEach(() => {
    useFilterStore.getState().reset();
  });

  it('tracks active filters', () => {
    const f = useFilterStore.getState();
    expect(f.hasActiveFilters()).toBe(false);
    f.setSearch('report');
    f.commitSearch();
    expect(useFilterStore.getState().hasActiveFilters()).toBe(true);
    useFilterStore.getState().reset();
    expect(useFilterStore.getState().hasActiveFilters()).toBe(false);
  });

  it('toggles categories without duplicates', () => {
    const f = useFilterStore.getState();
    f.toggleCategory('graphics');
    f.toggleCategory('video');
    expect(useFilterStore.getState().categories).toEqual(['graphics', 'video']);
    useFilterStore.getState().toggleCategory('graphics');
    expect(useFilterStore.getState().categories).toEqual(['video']);
  });

  it('parses custom extension lists', () => {
    const f = useFilterStore.getState();
    f.setExtensionInput('*.doc; *.xlsx, ppt');
    f.commitExtensions();
    expect(useFilterStore.getState().extensions).toEqual(['doc', 'xlsx', 'ppt']);
  });
});

describe('date/size range resolution', () => {
  it('maps presets to ISO bounds', () => {
    const any = dateRangeOf('any', null, null);
    expect(any.from).toBeNull();
    const custom = dateRangeOf('custom', '2024-02-01', '2024-03-01');
    expect(custom.from!.startsWith('2024-02-01')).toBe(true);
    expect(custom.to!.startsWith('2024-03-01')).toBe(true);
  });

  it('maps size presets to byte bounds', () => {
    expect(sizeRangeOf('tiny', null, null).maxBytes).toBe(512 * 1024);
    expect(sizeRangeOf('huge', null, null).minBytes).toBe(1024 ** 3);
    const custom = sizeRangeOf('custom', '10', '20');
    expect(custom.minBytes).toBe(10 * 1024 ** 2);
    expect(custom.maxBytes).toBe(20 * 1024 ** 2);
  });
});

describe('sort store', () => {
  it('flips direction when the same field is chosen twice', () => {
    useSortStore.setState({ field: 'name', direction: 'asc' });
    useSortStore.getState().setSort('name');
    expect(useSortStore.getState().direction).toBe('desc');
  });

  it('switches fields with sensible defaults', () => {
    useSortStore.setState({ field: 'name', direction: 'asc' });
    useSortStore.getState().setSort('size');
    expect(useSortStore.getState().field).toBe('size');
    expect(useSortStore.getState().direction).toBe('desc');
  });
});
