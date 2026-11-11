import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { ChevronDownIcon, MapIcon } from 'lucide-react';
import { useDiskMap } from '@/queries/imaging';
import { formatBytes } from '@/lib/format';
import { cn } from '@/lib/utils';

/**
 * Sector Map — the reference's AnalysisGridWidget surface (UIUX/08:11,
 * UIUX/13:47, lane C): a collapsible per-cell view of the scanned volume
 * with four overlays (coverage / allocation / found density / confidence)
 * plus the bad-range truth. The data is the ENGINE's own (cr_ui_disk_map
 * through the disk_map command — never synthesized client-side); the
 * panel only merges the engine's cells down to the display grid.
 */

type Overlay = 'coverage' | 'allocation' | 'found' | 'confidence';

const OVERLAYS: readonly Overlay[] = ['coverage', 'allocation', 'found', 'confidence'];

/** The display grid bound: the engine payload is capped at 1024 cells;
 * every cell becomes one DOM node, so the merge is identity at this
 * bound and still correct if a future caller raises the cap. */
const DISPLAY_CELLS = 1024;

/** Merge engine cells to at most `max` display cells (adjacent cells
 * fold with the most-significant state winning: the reference's grid
 * paints bad > found/used > scanned > free > unknown). */
function mergeCells(
  cells: readonly number[] | null,
  max: number,
  rank: (v: number) => number,
): number[] | null {
  if (cells === null) return null;
  if (cells.length <= max) return [...cells];
  const factor = Math.ceil(cells.length / max);
  const out: number[] = [];
  for (let i = 0; i < cells.length; i += factor) {
    let best = cells[i] ?? 0;
    let bestRank = rank(best);
    for (let k = i + 1; k < i + factor && k < cells.length; k++) {
      const v = cells[k] ?? 0;
      const r = rank(v);
      if (r > bestRank) {
        best = v;
        bestRank = r;
      }
    }
    out.push(best);
  }
  return out;
}

/** The overlay cell → Tailwind class map (theme tokens only). */
const CELL_CLASS: Record<Overlay, readonly string[]> = {
  // coverage: 0 unscanned, 1 scanned
  coverage: ['bg-[#DCE6F5] dark:bg-surface-tint', 'bg-[#93C5FD] dark:bg-primary/50'],
  // allocation: 0 unknown, 1 used, 2 free, 3 bad
  allocation: [
    'bg-[#DCE6F5] dark:bg-surface-tint',
    'bg-[#1677FF] dark:bg-primary-fill',
    'bg-[#86EFAC] dark:bg-[#365314]',
    'bg-[#F87171] dark:bg-danger-fill',
  ],
  // found: distinct files per cell (capped 255) → 4 bands
  found: [
    'bg-[#DCE6F5] dark:bg-surface-tint',
    'bg-[#BFDBFE] dark:bg-primary/30',
    'bg-[#60A5FA] dark:bg-primary/60',
    'bg-[#2563EB] dark:bg-primary/80',
    'bg-[#1D4ED8] dark:bg-primary-fill',
  ],
  // confidence: max ordinal 0..4 (poor → excellent)
  confidence: [
    'bg-[#DCE6F5] dark:bg-surface-tint',
    'bg-[#FCA5A5] dark:bg-danger/60',
    'bg-[#FCD34D] dark:bg-warning/60',
    'bg-[#86EFAC] dark:bg-[#365314]',
    'bg-[#1677FF] dark:bg-primary-fill',
  ],
};

function cellClass(overlay: Overlay, value: number): string {
  const classes = CELL_CLASS[overlay];
  if (overlay === 'found') {
    // 0 → band 0; 1-2 → 1; 3-8 → 2; 9-40 → 3; >40 → 4 (log bands — the
    // display is density, not a linear scale that one big file would max)
    const band = value <= 0 ? 0 : value <= 2 ? 1 : value <= 8 ? 2 : value <= 40 ? 3 : 4;
    return classes[band] ?? classes[0];
  }
  return classes[value] ?? classes[0];
}

/** The merge rank per overlay (higher wins the merged cell). */
function rankOf(overlay: Overlay): (v: number) => number {
  switch (overlay) {
    case 'coverage':
      return (v) => v;
    case 'allocation':
      return (v) => (v === 3 ? 3 : v === 1 ? 2 : v === 2 ? 1 : 0);
    case 'found':
      return (v) => Math.min(4, v <= 0 ? 0 : v <= 2 ? 1 : v <= 8 ? 2 : v <= 40 ? 3 : 4);
    case 'confidence':
      return (v) => v;
  }
}

export function SectorMapPanel({
  scanId,
  live,
}: {
  scanId: string;
  live: boolean;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const [open, setOpen] = React.useState(false);
  const [overlay, setOverlay] = React.useState<Overlay>('coverage');
  // The map query only runs while the panel is open (a collapsed strip
  // pays nothing) — or once when a completed scan's panel mounts open.
  const map = useDiskMap(scanId, live && open, DISPLAY_CELLS);

  const payload = map.data ?? null;
  const overlayCells = React.useMemo(
    () =>
      payload
        ? mergeCells(
            overlay === 'coverage'
              ? payload.coverage
              : overlay === 'allocation'
                ? payload.allocation
                : overlay === 'found'
                  ? payload.found
                  : payload.confidence,
            DISPLAY_CELLS,
            rankOf(overlay),
          )
        : null,
    [payload, overlay],
  );

  const badCount = payload?.badRanges.length ?? 0;
  const legendItems = legendFor(overlay, t);

  return (
    <div className="shrink-0 border-t border-[#C8DAF5] bg-[#EDF3FC] dark:border-border dark:bg-surface">
      {/* Toggle strip */}
      <button
        type="button"
        onClick={() => {
          setOpen((v) => !v);
          if (!open) void map.refetch();
        }}
        aria-expanded={open}
        className="flex h-8 w-full items-center gap-2 px-4 text-xs font-medium text-[#0B4F9E] transition-colors hover:bg-[#E2ECFA] focus-visible:ring-2 focus-visible:ring-ring dark:text-primary dark:hover:bg-surface-hover"
      >
        <MapIcon className="size-3.5" aria-hidden />
        {t('scan.map.title')}
        {payload ? (
          <span className="text-foreground-secondary tabular">
            {t('scan.map.cellSize', {
              size: formatBytes(payload.geometry.cellBytes, i18n.language),
            })}
          </span>
        ) : null}
        {badCount > 0 ? (
          <span className="text-danger tabular">
            {t('scan.map.badRanges', { count: badCount })}
          </span>
        ) : null}
        <ChevronDownIcon
          className={cn('ml-auto size-3.5 transition-transform', open && 'rotate-180')}
          aria-hidden
        />
      </button>

      {open ? (
        <div className="px-4 pb-3">
          {/* Overlay selector */}
          <div
            role="tablist"
            aria-label={t('scan.map.title')}
            className="mb-2 flex items-center gap-1"
          >
            {OVERLAYS.map((o) => (
              <button
                key={o}
                type="button"
                role="tab"
                aria-selected={overlay === o}
                onClick={() => setOverlay(o)}
                className={cn(
                  'rounded-xs px-2 py-0.5 text-[11px] font-medium transition-colors focus-visible:ring-2 focus-visible:ring-ring',
                  overlay === o
                    ? 'bg-[#1677FF] text-white dark:bg-primary-fill dark:text-primary-foreground'
                    : 'text-[#0B4F9E] hover:bg-[#DCE8FF] dark:text-primary dark:hover:bg-surface-hover',
                )}
              >
                {t(`scan.map.overlay.${o}`)}
              </button>
            ))}
            {map.isFetching ? (
              <span className="ml-1 text-[10px] text-foreground-secondary">
                {t('scan.map.refreshing')}
              </span>
            ) : null}
          </div>

          {/* The cell grid */}
          <div
            className="grid gap-px rounded-sm bg-white/60 p-px dark:bg-black/30"
            style={{ gridTemplateColumns: 'repeat(auto-fill, 8px)' }}
            role="img"
            aria-label={t('scan.map.gridLabel', { overlay: t(`scan.map.overlay.${overlay}`) })}
          >
            {overlayCells === null
              ? skeletonCells()
              : overlayCells.map((v, i) => (
                  <div key={i} className={cn('h-2 w-2 rounded-[1px]', cellClass(overlay, v))} />
                ))}
          </div>

          {/* Legend */}
          <div className="mt-2 flex flex-wrap items-center gap-x-3 gap-y-1 text-[10px] text-foreground-secondary">
            {legendItems.map((item) => (
              <span key={item.label} className="inline-flex items-center gap-1">
                <span
                  className={cn('inline-block size-2 rounded-[1px]', item.className)}
                  aria-hidden
                />
                {item.label}
              </span>
            ))}
          </div>
        </div>
      ) : null}
    </div>
  );
}

function skeletonCells(): React.JSX.Element[] {
  return Array.from({ length: 256 }, (_, i) => (
    <div
      key={i}
      className="h-2 w-2 animate-pulse rounded-[1px] bg-[#DCE6F5] dark:bg-surface-tint"
    />
  ));
}

interface LegendItem {
  readonly label: string;
  readonly className: string;
}

function legendFor(overlay: Overlay, t: (k: string) => string): readonly LegendItem[] {
  switch (overlay) {
    case 'coverage':
      return [
        { label: t('scan.map.legend.scanned'), className: CELL_CLASS.coverage[1] },
        { label: t('scan.map.legend.unscanned'), className: CELL_CLASS.coverage[0] },
      ];
    case 'allocation':
      return [
        { label: t('scan.map.legend.used'), className: CELL_CLASS.allocation[1] },
        { label: t('scan.map.legend.free'), className: CELL_CLASS.allocation[2] },
        { label: t('scan.map.legend.bad'), className: CELL_CLASS.allocation[3] },
        { label: t('scan.map.legend.unknown'), className: CELL_CLASS.allocation[0] },
      ];
    case 'found':
      return [
        { label: t('scan.map.legend.fewFiles'), className: CELL_CLASS.found[1] },
        { label: t('scan.map.legend.manyFiles'), className: CELL_CLASS.found[4] },
        { label: t('scan.map.legend.none'), className: CELL_CLASS.found[0] },
      ];
    case 'confidence':
      return [
        { label: t('scan.map.legend.poor'), className: CELL_CLASS.confidence[1] },
        { label: t('scan.map.legend.fair'), className: CELL_CLASS.confidence[2] },
        { label: t('scan.map.legend.good'), className: CELL_CLASS.confidence[3] },
        { label: t('scan.map.legend.excellent'), className: CELL_CLASS.confidence[4] },
      ];
  }
}
