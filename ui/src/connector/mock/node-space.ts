/**
 * Procedural node space — the mock's deterministic result index (ADR-004).
 *
 * Instead of materializing 1.2 M JS objects, we build:
 *  - a materialized directory skeleton (a few thousand real nodes), and
 *  - typed-array records for files, with names/attributes derived
 *    deterministically from the record index.
 *
 * Directory subtree aggregates are precomputed once (single pass) so tree
 * expansion is O(children), and filter queries scan typed arrays directly
 * (~10–40 ms for 1.2 M records — grid #36 budget).
 */

import type {
  ConfidenceGrade,
  FileCategoryId,
  ResultNode,
  ResultOrigin,
  ResultsFilter,
  SortDirection,
  SortField,
  TreeNode,
} from '../contract';
import { mulberry32, pick, randBell, randInt } from './rng';
import { CATEGORY_BY_EXT, DIR_NAMES, EXT_WEIGHTS, NAME_STEMS } from './fixtures';

const EMPTY_FILTER: ResultsFilter = {
  nodeId: null,
  treeMode: 'path',
  search: null,
  categories: null,
  dateFrom: null,
  dateTo: null,
  sizeMinBytes: null,
  sizeMaxBytes: null,
  extensions: null,
  origins: null,
  hideEmptyFiles: false,
};

const ORIGINS: readonly ResultOrigin[] = [
  'existing',
  'deleted',
  'deleted',
  'deleted',
  'raw',
  'lost-path',
];
const CONFIDENCES: readonly ConfidenceGrade[] = [
  'excellent',
  'good',
  'good',
  'good',
  'fair',
  'poor',
  'unknown',
];

export const CATEGORY_IDS: readonly FileCategoryId[] = [
  'graphics',
  'video',
  'audio',
  'document',
  'email',
  'archive',
  'bookmarks',
  'other',
];

interface DirectoryRecord {
  readonly id: string;
  readonly parentId: string | null;
  readonly name: string;
  readonly depth: number;
  fileCount: number;
  sizeBytes: number;
}

const SPECIAL_PATH_GROUPS = ['lost-paths', 'raw-all'] as const;

/** A folder-scoped view over the node space (ADR-004 scope extension). */
export interface NodeScope {
  /** Directory-index mask (1 = inside the scanned subtree). */
  readonly mask: Uint8Array;
  /** Array index of the scope root directory. */
  readonly rootIndex: number;
  /** Directory id (e.g. 'd17') of the scope root. */
  readonly rootId: string;
  /** Scope-root path as the user typed it (normalized). */
  readonly normalizedPath: string;
  /** Subtree file count (precomputed aggregate). */
  readonly files: number;
  /** Subtree size in bytes (precomputed aggregate). */
  readonly sizeBytes: number;
}

function normalizeDirPath(path: string): string {
  return (
    path
      .trim()
      // An absolute path on the target volume is accepted: the engine (and the
      // mock) resolves scopes relative to the volume root, so drop any
      // drive-letter prefix before normalizing separators.
      .replace(/^[a-z]:\\/i, '')
      .replace(/\//g, '\\')
      .replace(/^\\+/, '')
      .replace(/\\+$/, '')
      .toLowerCase()
  );
}

function catIndex(cat: FileCategoryId): number {
  return CATEGORY_IDS.indexOf(cat);
}

export class NodeSpace {
  readonly volumeId: string;
  readonly seed: number;
  readonly dirCount: number;
  readonly fileCount: number;

  private readonly dirs: DirectoryRecord[] = [];
  private readonly dirIndexById = new Map<string, DirectoryRecord>();
  private readonly childrenOf = new Map<string, string[]>();
  /** Normalized full path → dir array index (built lazily for scope resolution). */
  private readonly dirIndexByPath = new Map<string, number>();

  private readonly fDir: Uint32Array;
  private readonly fCat: Uint8Array;
  private readonly fSize: Float64Array;
  private readonly fDay: Uint16Array;
  private readonly fConf: Uint8Array;
  private readonly fOrigin: Uint8Array;
  private readonly fExt: Uint8Array;

  private readonly extPool: readonly string[];

  readonly totalSizeBytes: number;

  constructor(
    volumeId: string,
    seed: number,
    dirCount: number,
    fileCount: number,
    /** Found-data budget: the aggregate generated size is scaled down to
     *  track the volume capacity (corpus GT-scan §8: 147.12 GB found on a
     *  149.45 GB volume — deep coverage approaches, never exceeds, the disk). */
    totalBytesBudget = Number.POSITIVE_INFINITY,
  ) {
    this.volumeId = volumeId;
    this.seed = seed;
    this.dirCount = dirCount;
    this.fileCount = fileCount;

    const rng = mulberry32(seed);

    this.extPool = EXT_WEIGHTS.map((w) => w.ext);
    let acc = 0;
    const extCum: number[] = EXT_WEIGHTS.map((w) => {
      acc += w.weight;
      return acc;
    });
    const extTotal = acc;

    // ---- directory skeleton ----
    const root: DirectoryRecord = {
      id: 'root',
      parentId: null,
      name: volumeId,
      depth: 0,
      fileCount: 0,
      sizeBytes: 0,
    };
    this.dirs.push(root);
    this.dirIndexById.set('root', root);
    this.childrenOf.set('root', []);

    let created = 1;
    let guard = 0;
    while (created < dirCount && guard < dirCount * 6) {
      guard++;
      const climb = Math.floor(rng() * 4);
      let parent = root;
      for (let d = 0; d < climb; d++) {
        const kids = this.childrenOf.get(parent.id) ?? [];
        if (kids.length === 0) break;
        parent = this.dirIndexById.get(kids[Math.floor(rng() * kids.length)]) ?? root;
      }
      const name = pick(rng, DIR_NAMES);
      const id = `d${created}`;
      const rec: DirectoryRecord = {
        id,
        parentId: parent.id,
        name,
        depth: parent.depth + 1,
        fileCount: 0,
        sizeBytes: 0,
      };
      this.dirs.push(rec);
      this.dirIndexById.set(id, rec);
      const siblings = this.childrenOf.get(parent.id) ?? [];
      siblings.push(id);
      this.childrenOf.set(parent.id, siblings);
      created++;
    }

    // ---- file records ----
    const n = fileCount;
    this.fDir = new Uint32Array(n);
    this.fCat = new Uint8Array(n);
    this.fSize = new Float64Array(n);
    this.fDay = new Uint16Array(n);
    this.fConf = new Uint8Array(n);
    this.fOrigin = new Uint8Array(n);
    this.fExt = new Uint8Array(n);

    const catIdxOfExt = new Map<string, number>();
    for (const [ext, cat] of Object.entries(CATEGORY_BY_EXT)) {
      catIdxOfExt.set(ext, catIndex(cat));
    }

    const rawStart = Math.floor(n * 0.86);

    for (let i = 0; i < n; i++) {
      const dirIdx = randInt(rng, 0, this.dirs.length - 1);
      this.fDir[i] = dirIdx;

      const extRoll = rng() * extTotal;
      let extIdx = 0;
      for (let k = 0; k < extCum.length; k++) {
        if (extRoll <= extCum[k]) {
          extIdx = k;
          break;
        }
      }
      this.fExt[i] = extIdx;
      const ext = this.extPool[extIdx] ?? 'bin';
      this.fCat[i] = catIdxOfExt.get(ext) ?? catIndex('other');

      let size: number;
      switch (CATEGORY_BY_EXT[ext]) {
        case 'graphics':
          size = randBell(rng, 3.2e6, 12e6);
          break;
        case 'video':
          size = randBell(rng, 8.4e8, 4.2e9);
          break;
        case 'audio':
          size = randBell(rng, 9e6, 24e6);
          break;
        default:
          size = randBell(rng, 2.4e5, 3e6);
          break;
      }
      this.fSize[i] = Math.max(64, Math.round(size));
      this.fDay[i] = Math.min(0xffff, Math.round(randBell(rng, 1200, 2400)));

      if (i >= rawStart) {
        this.fOrigin[i] = ORIGINS.indexOf('raw');
        this.fConf[i] = i % 3 === 0 ? 3 : 4;
      } else {
        // Weighted origin mix per the corpus model (atlas §5 / GT-scan §3):
        // the existing tree dominates; deleted files and path-lost files
        // (advanced-scan finds whose path could not be reconstructed) follow.
        const r = rng();
        this.fOrigin[i] =
          r < 0.62
            ? ORIGINS.indexOf('existing')
            : r < 0.88
              ? ORIGINS.indexOf('deleted')
              : ORIGINS.indexOf('lost-path');
        this.fConf[i] = Math.floor(rng() * CONFIDENCES.length);
      }
    }

    // ---- capacity scaling: keep the aggregate within the volume budget ----
    let totalSize = 0;
    for (let i = 0; i < n; i++) totalSize += this.fSize[i];
    if (totalSize > totalBytesBudget && totalSize > 0) {
      const scale = totalBytesBudget / totalSize;
      for (let i = 0; i < n; i++) {
        this.fSize[i] = Math.max(64, Math.floor(this.fSize[i] * scale));
      }
      totalSize = 0;
      for (let i = 0; i < n; i++) totalSize += this.fSize[i];
    }

    // ---- precompute per-directory direct counts, then bottom-up subtree sums ----
    const directCount = new Float64Array(this.dirs.length);
    const directSize = new Float64Array(this.dirs.length);
    for (let i = 0; i < n; i++) {
      const d = this.fDir[i];
      directCount[d] += 1;
      directSize[d] += this.fSize[i];
    }
    this.totalSizeBytes = totalSize;
    // pass 1: initialize every directory to its own direct totals
    for (let d = 0; d < this.dirs.length; d++) {
      this.dirs[d].fileCount = directCount[d]!;
      this.dirs[d].sizeBytes = directSize[d]!;
    }
    // pass 2: propagate subtree sums into parents (children always have a
    // higher creation index than their parent, so reverse order is safe)
    for (let d = this.dirs.length - 1; d >= 1; d--) {
      const rec = this.dirs[d];
      const parent = rec.parentId !== null ? this.dirIndexById.get(rec.parentId) : undefined;
      if (parent) {
        parent.fileCount += rec.fileCount;
        parent.sizeBytes += rec.sizeBytes;
      }
    }
  }

  // ------------------------------------------------------------------ names

  fileName(i: number): string {
    const rng = mulberry32((this.seed ^ Math.imul(i + 1, 2654435761)) >>> 0);
    const stem = pick(rng, NAME_STEMS);
    const ext = this.extPool[this.fExt[i]] ?? 'bin';
    const suffix = Math.floor(rng() * 10_000);
    const prefix = i % 20 === 0 ? 'old_' : i % 37 === 0 ? 'new_' : '';
    return `${prefix}${stem}_${String(suffix).padStart(4, '0')}.${ext}`;
  }

  modifiedIso(i: number): string {
    const day = this.fDay[i] ?? 0;
    const t = Date.UTC(2019, 0, 1) + day * 86_400_000 + ((i * 3600) % 86_400) * 1000;
    return new Date(t).toISOString();
  }

  dirPathOf(i: number): string | null {
    const dir = this.dirs[this.fDir[i]];
    if (!dir || dir.id === 'root') return null;
    const parts: string[] = [];
    let cur: DirectoryRecord | undefined = dir;
    let guard = 0;
    while (cur && cur.parentId !== null && guard < 16) {
      parts.unshift(cur.name);
      cur = this.dirIndexById.get(cur.parentId);
      guard++;
    }
    return parts.length > 0 ? parts.join('\\') : null;
  }

  originOf(i: number): ResultOrigin {
    return ORIGINS[this.fOrigin[i]] ?? 'existing';
  }

  categoryOf(i: number): FileCategoryId {
    return CATEGORY_IDS[this.fCat[i]] ?? 'other';
  }

  extensionOf(i: number): string {
    return this.extPool[this.fExt[i]] ?? 'bin';
  }

  nodeAt(i: number): ResultNode {
    const cat = this.categoryOf(i);
    const origin = this.originOf(i);
    return {
      id: `f${i}`,
      scanId: this.volumeId,
      kind: 'file',
      name: this.fileName(i),
      path: origin === 'raw' ? null : this.dirPathOf(i),
      sizeBytes: this.fSize[i] ?? 0,
      modifiedAt: this.modifiedIso(i),
      extension: `.${this.extensionOf(i)}`,
      category: cat,
      confidence: CONFIDENCES[this.fConf[i]] ?? 'unknown',
      origin,
      childCount: null,
      thumbnailAvailable: cat === 'graphics' || cat === 'video',
      parentId: this.fDir[i] !== undefined ? `d${this.fDir[i]}` : null,
    };
  }

  // ------------------------------------------------------------------ scope

  /**
   * Resolve a volume-root-relative folder path into a subtree scope.
   * Returns null when the path does not exist in this space — the connector
   * then raises `scan-scope-invalid`.
   */
  resolveScope(path: string): NodeScope | null {
    const normalizedPath = normalizeDirPath(path);
    if (normalizedPath === '') return null;
    if (this.dirIndexByPath.size === 0) this.buildDirPathIndex();
    const rootIndex = this.dirIndexByPath.get(normalizedPath);
    if (rootIndex === undefined) return null;

    const mask = new Uint8Array(this.dirs.length);
    mask[rootIndex] = 1;
    // BFS over the children map — dir ids map 1:1 to array indexes (d{i}).
    const queue: number[] = [rootIndex];
    for (let head = 0; head < queue.length; head++) {
      const rec = this.dirs[queue[head]];
      for (const kidId of this.childrenOf.get(rec.id) ?? []) {
        const kidIndex = Number(kidId.slice(1));
        if (!mask[kidIndex]) {
          mask[kidIndex] = 1;
          queue.push(kidIndex);
        }
      }
    }
    const root = this.dirs[rootIndex];
    return {
      mask,
      rootIndex,
      rootId: root.id,
      normalizedPath,
      files: root.fileCount,
      sizeBytes: root.sizeBytes,
    };
  }

  /** A volume-root-relative folder that actually exists (for suggestions). */
  suggestedScopePath(): string | null {
    const first = (this.childrenOf.get('root') ?? [])[0];
    if (!first) return null;
    const rec = this.dirIndexById.get(first);
    if (!rec) return null;
    const parts: string[] = [];
    let cur: DirectoryRecord | undefined = rec;
    let guard = 0;
    while (cur && cur.parentId !== null && guard < 32) {
      parts.unshift(cur.name);
      cur = this.dirIndexById.get(cur.parentId);
      guard++;
    }
    return parts.join('\\');
  }

  private buildDirPathIndex(): void {
    for (let d = 0; d < this.dirs.length; d++) {
      const rec = this.dirs[d];
      if (rec.parentId === null) continue; // volume root has no path
      const parts: string[] = [];
      let cur: DirectoryRecord | undefined = rec;
      let guard = 0;
      while (cur && cur.parentId !== null && guard < 32) {
        parts.unshift(cur.name);
        cur = this.dirIndexById.get(cur.parentId);
        guard++;
      }
      const path = parts.join('\\').toLowerCase();
      if (!this.dirIndexByPath.has(path)) this.dirIndexByPath.set(path, d);
    }
  }

  private inScope(i: number, scope: Uint8Array | null | undefined): boolean {
    return scope === null || scope === undefined || scope[this.fDir[i]] === 1;
  }

  // ------------------------------------------------------------------ filter

  private matches(i: number, filter: ResultsFilter, scope: Uint8Array | null): boolean {
    if (!this.inScope(i, scope)) return false;
    if (filter.categories && filter.categories.length > 0) {
      if (!filter.categories.includes(this.categoryOf(i))) return false;
    }
    if (
      filter.origins &&
      filter.origins.length > 0 &&
      !filter.origins.includes(this.originOf(i))
    )
      return false;
    if (filter.hideEmptyFiles && (this.fSize[i] ?? 0) === 0) return false;
    if (filter.sizeMinBytes !== null && (this.fSize[i] ?? 0) < filter.sizeMinBytes)
      return false;
    if (filter.sizeMaxBytes !== null && (this.fSize[i] ?? 0) > filter.sizeMaxBytes)
      return false;
    if (
      filter.dateFrom &&
      new Date(this.modifiedIso(i)).getTime() < new Date(filter.dateFrom).getTime()
    )
      return false;
    if (
      filter.dateTo &&
      new Date(this.modifiedIso(i)).getTime() > new Date(filter.dateTo).getTime()
    )
      return false;
    if (filter.extensions && filter.extensions.length > 0) {
      const ext = this.extensionOf(i);
      if (!filter.extensions.some((e) => e.replace(/^\./, '') === ext)) return false;
    }

    if (filter.nodeId && filter.treeMode === 'path') {
      const scope = filter.nodeId;
      if (SPECIAL_PATH_GROUPS.includes(scope as (typeof SPECIAL_PATH_GROUPS)[number])) {
        if (scope === 'lost-paths' && this.originOf(i) !== 'lost-path') return false;
        if (scope === 'raw-all' && this.originOf(i) !== 'raw') return false;
      } else if (scope !== 'root' && scope.startsWith('d')) {
        // files directly inside this directory (explorer convention)
        if (this.fDir[i] !== Number(scope.slice(1))) return false;
      }
    } else if (filter.nodeId && filter.treeMode === 'type') {
      if (filter.nodeId.startsWith('c')) {
        const wanted = CATEGORY_IDS[Number(filter.nodeId.slice(1))];
        if (wanted && this.categoryOf(i) !== wanted) return false;
      }
    } else if (filter.nodeId && filter.treeMode === 'raw') {
      if (this.originOf(i) !== 'raw') return false;
    }

    if (filter.search) {
      const q = filter.search.toLowerCase();
      if (!this.fileName(i).toLowerCase().includes(q)) return false;
    }
    return true;
  }

  query(
    filter: ResultsFilter,
    sort: { field: SortField; direction: SortDirection },
    offset: number,
    limit: number,
    scope: Uint8Array | null = null,
  ): { nodes: ResultNode[]; total: number; elapsedMs: number } {
    const t0 = performance.now();
    const hits: number[] = [];
    for (let i = 0; i < this.fileCount; i++) {
      if (this.matches(i, filter, scope)) hits.push(i);
    }
    const cmp = this.comparatorFor(sort);
    hits.sort(cmp);
    const page = hits.slice(offset, offset + limit).map((i) => this.nodeAt(i));
    return { nodes: page, total: hits.length, elapsedMs: performance.now() - t0 };
  }

  private comparatorFor(sort: {
    field: SortField;
    direction: SortDirection;
  }): (a: number, b: number) => number {
    const dir = sort.direction === 'asc' ? 1 : -1;
    switch (sort.field) {
      case 'size':
        return (a, b) => ((this.fSize[a] ?? 0) - (this.fSize[b] ?? 0)) * dir;
      case 'modified':
        return (a, b) => ((this.fDay[a] ?? 0) - (this.fDay[b] ?? 0)) * dir;
      case 'type':
        return (a, b) => this.extensionOf(a).localeCompare(this.extensionOf(b)) * dir;
      case 'confidence':
        return (a, b) => ((this.fConf[a] ?? 0) - (this.fConf[b] ?? 0)) * dir;
      case 'name':
      default:
        return (a, b) => this.fileName(a).localeCompare(this.fileName(b)) * dir;
    }
  }

  /** File INDICES matching a filter in natural order (recover-all walk). */
  queryNodes(
    filter: ResultsFilter,
    scope: Uint8Array | null,
    offset: number,
    limit: number,
  ): number[] {
    const hits: number[] = [];
    for (let i = 0; i < this.fileCount && hits.length < offset + limit; i++) {
      if (this.matches(i, filter, scope)) hits.push(i);
    }
    return hits.slice(offset, offset + limit);
  }

  /**
   * A plausible deep-pass "Searching: <path>" item for the given tick —
   * deterministic, derived from the space itself (corpus GT-scan §2: the
   * ticker shows the file currently being examined deep in the tree).
   */
  searchingPathAt(tick: number, scope: Uint8Array | null): string | null {
    const base = Math.floor(tick * 7) % Math.max(1, this.fileCount);
    for (let k = 0; k < 32; k++) {
      const i = (base + k * 131) % this.fileCount;
      if (!this.matches(i, EMPTY_FILTER, scope)) continue;
      const node = this.nodeAt(i);
      if (node.kind !== 'file') continue;
      const dir = node.path ?? '';
      return `${dir}\\${node.name}`;
    }
    return null;
  }

  count(filter: ResultsFilter, scope: Uint8Array | null = null): number {
    let total = 0;
    for (let i = 0; i < this.fileCount; i++) {
      if (this.matches(i, filter, scope)) total++;
    }
    return total;
  }

  // ------------------------------------------------------------------ trees

  /** Path-mode tree children (directories + special groups at top level). */
  pathTreeChildren(parentId: string | null, scope: Uint8Array | null = null): TreeNode[] {
    if (parentId === null) {
      // A scoped scan presents the scanned folder itself as the tree root.
      if (scope) {
        const scopeRootIdx = scope.indexOf(1);
        if (scopeRootIdx >= 0) {
          const rec = this.dirs[scopeRootIdx];
          const topDirs = (this.childrenOf.get(rec.id) ?? [])
            .map((id) => Number(id.slice(1)))
            .filter((idx) => scope[idx] === 1);
          return [
            {
              id: rec.id,
              parentId: null,
              label: rec.name,
              icon: 'directory',
              childCount: topDirs.length,
              fileCount: rec.fileCount,
              sizeBytes: rec.sizeBytes,
              origin: 'existing',
            },
          ];
        }
      }
      const rootRec = this.dirIndexById.get('root')!;
      const lostCount = this.countByOrigin('lost-path', scope);
      const lostSize = this.sizeByOrigin('lost-path', scope);
      const rawCount = this.countByOrigin('raw', scope);
      const rawSize = this.sizeByOrigin('raw', scope);
      return [
        {
          id: 'root',
          parentId: null,
          label: 'root',
          icon: 'device',
          childCount: this.childrenOf.get('root')?.length ?? 0,
          fileCount: rootRec.fileCount,
          sizeBytes: rootRec.sizeBytes,
          origin: 'existing',
        },
        {
          id: 'lost-paths',
          parentId: null,
          label: 'lost-paths',
          icon: 'group',
          childCount: 0,
          fileCount: lostCount,
          sizeBytes: lostSize,
          origin: 'lost-path',
          special: 'lost-path',
        },
        {
          id: 'raw-all',
          parentId: null,
          label: 'raw',
          icon: 'group',
          childCount: 0,
          fileCount: rawCount,
          sizeBytes: rawSize,
          origin: 'raw',
          special: 'raw-found',
        },
      ];
    }
    if (parentId === 'lost-paths' || parentId === 'raw-all') {
      return []; // flat groups — their files are listed via query()
    }
    const kidIds = this.childrenOf.get(parentId) ?? [];
    const out: TreeNode[] = [];
    for (const kid of kidIds) {
      const rec = this.dirIndexById.get(kid);
      if (!rec) continue;
      if (scope && scope[Number(kid.slice(1))] !== 1) continue;
      out.push({
        id: rec.id,
        parentId: rec.parentId,
        label: rec.name,
        icon: 'directory',
        childCount: this.childrenOf.get(rec.id)?.length ?? 0,
        fileCount: rec.fileCount,
        sizeBytes: rec.sizeBytes,
        origin: 'existing',
      });
    }
    return out;
  }

  private countByOrigin(origin: ResultOrigin, scope: Uint8Array | null = null): number {
    let c = 0;
    for (let i = 0; i < this.fileCount; i++) {
      if (this.originOf(i) === origin && this.inScope(i, scope)) c++;
    }
    return c;
  }

  private sizeByOrigin(origin: ResultOrigin, scope: Uint8Array | null = null): number {
    let s = 0;
    for (let i = 0; i < this.fileCount; i++) {
      if (this.originOf(i) === origin && this.inScope(i, scope)) s += this.fSize[i] ?? 0;
    }
    return s;
  }

  private categoryAggregates(
    scope: Uint8Array | null = null,
  ): { count: number; size: number }[] {
    const counts = new Float64Array(CATEGORY_IDS.length);
    const sizes = new Float64Array(CATEGORY_IDS.length);
    for (let i = 0; i < this.fileCount; i++) {
      if (!this.inScope(i, scope)) continue;
      counts[this.fCat[i]] += 1;
      sizes[this.fCat[i]] += this.fSize[i] ?? 0;
    }
    return Array.from({ length: CATEGORY_IDS.length }, (_, idx) => ({
      count: counts[idx] ?? 0,
      size: sizes[idx] ?? 0,
    }));
  }

  typeTree(scope: Uint8Array | null = null): TreeNode[] {
    const agg = this.categoryAggregates(scope);
    return CATEGORY_IDS.map((cat, idx) => ({
      id: `c${idx}`,
      parentId: 'root',
      label: cat,
      icon: 'category' as const,
      childCount: 0,
      fileCount: agg[idx].count,
      sizeBytes: agg[idx].size,
      origin: 'existing' as const,
    }));
  }

  rawTree(scope: Uint8Array | null = null): TreeNode[] {
    const count = this.countByOrigin('raw', scope);
    const size = this.sizeByOrigin('raw', scope);
    return [
      {
        id: 'raw-all',
        parentId: null,
        label: 'raw',
        icon: 'group' as const,
        childCount: 0,
        fileCount: count,
        sizeBytes: size,
        origin: 'raw' as const,
        special: 'raw-found' as const,
      },
    ];
  }

  stats(scope: Uint8Array | null = null): { files: number; size: number; rawFiles: number } {
    if (scope) {
      const scopeRootIdx = scope.indexOf(1);
      const rec = scopeRootIdx >= 0 ? this.dirs[scopeRootIdx] : null;
      return {
        files: rec?.fileCount ?? 0,
        size: rec?.sizeBytes ?? 0,
        rawFiles: this.countByOrigin('raw', scope),
      };
    }
    return {
      files: this.fileCount,
      size: this.totalSizeBytes,
      rawFiles: this.countByOrigin('raw'),
    };
  }
}
