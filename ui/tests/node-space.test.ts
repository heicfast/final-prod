import { describe, expect, it } from 'vitest';
import { NodeSpace } from '@/connector/mock/node-space';
import type { ResultsFilter } from '@/connector/contract';

const BASE: ResultsFilter = {
  nodeId: null,
  treeMode: 'path',
  search: null,
  categories: null,
  dateFrom: null,
  dateTo: null,
  sizeMinBytes: null,
  sizeMaxBytes: null,
  confidence: null,
  extensions: null,
  origins: null,
};

describe('NodeSpace — procedural index', () => {
  it('is deterministic for identical seeds', () => {
    const a = new NodeSpace('vol-test', 1234, 40, 800);
    const b = new NodeSpace('vol-test', 1234, 40, 800);
    const pa = a.query(BASE, { field: 'name', direction: 'asc' }, 0, 10);
    const pb = b.query(BASE, { field: 'name', direction: 'asc' }, 0, 10);
    expect(pa.nodes.map((n) => n.name)).toEqual(pb.nodes.map((n) => n.name));
    expect(pa.total).toBe(pb.total);
  });

  it('pages within bounds and reports totals', () => {
    const s = new NodeSpace('vol-test', 7, 30, 500);
    const p1 = s.query(BASE, { field: 'name', direction: 'asc' }, 0, 50);
    const p2 = s.query(BASE, { field: 'name', direction: 'asc' }, 50, 50);
    expect(p1.nodes.length).toBe(50);
    expect(p1.total).toBe(500);
    if (p2.nodes.length > 0) {
      const names = new Set([...p1.nodes, ...p2.nodes].map((n) => n.name));
      expect(names.size).toBe(p1.nodes.length + p2.nodes.length);
    }
  });

  it('applies size and category filters correctly', () => {
    const s = new NodeSpace('vol-test', 99, 20, 600);
    const small = s.query(
      { ...BASE, sizeMaxBytes: 1024 },
      { field: 'size', direction: 'asc' },
      0,
      600,
    );
    expect(small.nodes.every((n) => n.sizeBytes <= 1024)).toBe(true);

    const gfx = s.query(
      { ...BASE, categories: ['graphics'] },
      { field: 'name', direction: 'asc' },
      0,
      600,
    );
    expect(gfx.nodes.every((n) => n.category === 'graphics')).toBe(true);
  });

  it('search matches substrings case-insensitively', () => {
    const s = new NodeSpace('vol-test', 5, 10, 300);
    const probe = s.query(BASE, { field: 'name', direction: 'asc' }, 0, 1).nodes[0];
    const stem = probe.name.slice(0, 3).toLowerCase();
    const hits = s.query(
      { ...BASE, search: stem },
      { field: 'name', direction: 'asc' },
      0,
      300,
    );
    expect(hits.total).toBeGreaterThan(0);
    expect(hits.nodes.every((n) => n.name.toLowerCase().includes(stem))).toBe(true);
  });

  it('path tree children carry precomputed aggregates', () => {
    const s = new NodeSpace('vol-test', 11, 25, 400);
    const children = s.pathTreeChildren(null);
    expect(children.length).toBe(3); // root + lost-paths + raw groups
    const root = children.find((n) => n.id === 'root')!;
    expect(root.fileCount).toBe(400);
    expect(root.sizeBytes).toBe(s.totalSizeBytes);
    const raw = children.find((n) => n.id === 'raw-all')!;
    expect(raw.fileCount).toBe(s.stats().rawFiles);
  });

  it('type tree returns exactly the 8 categories', () => {
    const s = new NodeSpace('vol-test', 12, 10, 200);
    const tree = s.typeTree();
    expect(tree.map((n) => n.label)).toEqual([
      'graphics',
      'video',
      'audio',
      'document',
      'email',
      'archive',
      'bookmarks',
      'other',
    ]);
  });

  it('scopes queries to a selected type node', () => {
    const s = new NodeSpace('vol-test', 21, 10, 300);
    const tree = s.typeTree();
    const graphics = tree.find((n) => n.label === 'graphics')!;
    const hits = s.query(
      { ...BASE, treeMode: 'type', nodeId: graphics.id },
      { field: 'name', direction: 'asc' },
      0,
      300,
    );
    expect(hits.nodes.every((n) => n.category === 'graphics')).toBe(true);
    expect(hits.total).toBe(graphics.fileCount);
  });
});
