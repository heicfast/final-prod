import * as React from 'react';
import { useVirtualizer, type Virtualizer } from '@tanstack/react-virtual';
import type { ResultNode, SortField } from '@/connector/contract';
import { ArrowDownIcon, ArrowUpIcon, ChevronsUpDownIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { cn } from '@/lib/utils';
import { formatBytes, formatDate } from '@/lib/format';
import { ConfidencePillInline } from './confidence-badge';
import { Checkbox } from './checkbox';
import { FileIconFor } from './file-icon';
import { useThumbnail } from '@/queries/results';

/**
 * Virtualized results table (1 M+ rows). Row height from the density token;
 * columns fixed-width so horizontal scrolling is stable; selection via the
 * shared selection store contract passed in as props.
 */

const COLUMN_WIDTHS: Record<string, number> = {
  checkbox: 40,
  name: 320,
  modified: 140,
  type: 110,
  size: 100,
  condition: 120,
};

/** Shared row-membership test across both selection modes. */
function rowSelected(
  selectionMode: 'ids' | 'all',
  selectedIds: ReadonlySet<string>,
  exclusions: ReadonlySet<string>,
  id: string,
): boolean {
  return selectionMode === 'all' ? !exclusions.has(id) : selectedIds.has(id);
}

/** The infinite-scroll loader row (shared by both view renderers). */
function LoaderRow({ top, height, label }: { top: number; height: number; label: string }) {
  return (
    <div
      className="absolute inset-x-0 flex items-center justify-center"
      style={{ top, height }}
    >
      <span className="text-xs text-foreground-secondary">{label}</span>
    </div>
  );
}

function SortHeader({
  field,
  label,
  currentField,
  currentDirection,
  onSort,
  align = 'left',
  width,
}: {
  field: SortField;
  label: string;
  currentField: SortField;
  currentDirection: 'asc' | 'desc';
  onSort: (f: SortField) => void;
  align?: 'left' | 'right';
  width: number;
}): React.JSX.Element {
  const { t } = useTranslation();
  const active = currentField === field;
  const Icon = active
    ? currentDirection === 'asc'
      ? ArrowUpIcon
      : ArrowDownIcon
    : ChevronsUpDownIcon;
  return (
    <button
      type="button"
      onClick={() => onSort(field)}
      aria-label={`${label} — ${active ? (currentDirection === 'asc' ? t('a11y.sortDescending') : t('a11y.sortAscending')) : t('a11y.sortAscending')}`}
      className={cn(
        'flex h-8 select-none items-center gap-1 border-b border-border px-3 text-xs font-semibold text-foreground-secondary transition-colors hover:bg-surface-hover/50 hover:text-foreground',
        'focus-visible:ring-2 focus-visible:ring-ring focus-visible:ring-inset',
        align === 'right' && 'justify-end',
      )}
      style={{ width }}
    >
      <span className="truncate">{label}</span>
      <Icon
        className={cn(
          'size-3 shrink-0',
          active ? 'opacity-90' : 'opacity-0 group-hover/column:opacity-40',
        )}
        aria-hidden
      />
    </button>
  );
}

export function VirtualFileList({
  scanId,
  rows,
  total,
  rowHeight,
  viewMode = 'details',
  selectionMode = 'ids',
  selectedIds,
  exclusions,
  activeId,
  onRowClick,
  onRowDoubleClick,
  onCheckboxChange,
  onSelectAll,
  allSelected,
  someSelected = false,
  sort,
  onSortChange,
  loadMore,
  hasNextPage,
  className,
}: {
  scanId: string;
  rows: readonly ResultNode[];
  total: number;
  rowHeight: number;
  viewMode?: 'details' | 'icons';
  selectionMode?: 'ids' | 'all';
  selectedIds: ReadonlySet<string>;
  exclusions: ReadonlySet<string>;
  activeId: string | null;
  onRowClick: (node: ResultNode, index: number, e: React.MouseEvent) => void;
  onRowDoubleClick: (node: ResultNode) => void;
  onCheckboxChange: (node: ResultNode) => void;
  onSelectAll: (checked: boolean) => void;
  allSelected: boolean;
  someSelected?: boolean;
  sort: { field: SortField; direction: 'asc' | 'desc' };
  onSortChange: (field: SortField) => void;
  loadMore?: () => void;
  hasNextPage?: boolean;
  className?: string;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const ref = React.useRef<HTMLDivElement | null>(null);

  const virtualizer = useVirtualizer({
    count: rows.length + (hasNextPage ? 1 : 0),
    getScrollElement: () => ref.current,
    estimateSize: () => rowHeight,
    overscan: 16,
  });

  React.useEffect(() => {
    const el = ref.current;
    if (!el || !loadMore) return;
    let cooldown = false;
    const onScroll = (): void => {
      if (cooldown) return;
      const nearBottom = el.scrollTop + el.clientHeight >= el.scrollHeight - 400;
      if (nearBottom && hasNextPage) {
        cooldown = true;
        loadMore();
        window.setTimeout(() => {
          cooldown = false;
        }, 120);
      }
    };
    el.addEventListener('scroll', onScroll, { passive: true });
    return () => el.removeEventListener('scroll', onScroll);
  }, [loadMore, hasNextPage, ref]);

  const totalWidth =
    COLUMN_WIDTHS.checkbox +
    COLUMN_WIDTHS.name +
    COLUMN_WIDTHS.modified +
    COLUMN_WIDTHS.type +
    COLUMN_WIDTHS.size +
    COLUMN_WIDTHS.condition;

  return (
    <div
      className={cn('flex h-full min-h-0 flex-col', className)}
      role="grid"
      aria-rowcount={total}
    >
      <div
        className="group/column flex shrink-0 border-b border-border bg-surface text-sm"
        role="row"
      >
        <div
          className="flex items-center justify-center border-b border-border"
          style={{ width: COLUMN_WIDTHS.checkbox }}
        >
          <Checkbox
            checked={
              rows.length > 0 && allSelected ? true : someSelected ? 'indeterminate' : false
            }
            onCheckedChange={(v: boolean | 'indeterminate') => onSelectAll(v === true)}
            aria-label={t('results.selection.selectAll')}
          />
        </div>
        <SortHeader
          field="name"
          label={t('results.column.name')}
          currentField={sort.field}
          currentDirection={sort.direction}
          onSort={onSortChange}
          width={COLUMN_WIDTHS.name}
        />
        <SortHeader
          field="modified"
          label={t('results.column.modified')}
          currentField={sort.field}
          currentDirection={sort.direction}
          onSort={onSortChange}
          width={COLUMN_WIDTHS.modified}
        />
        <SortHeader
          field="type"
          label={t('results.column.type')}
          currentField={sort.field}
          currentDirection={sort.direction}
          onSort={onSortChange}
          width={COLUMN_WIDTHS.type}
        />
        <SortHeader
          field="size"
          label={t('results.column.size')}
          currentField={sort.field}
          currentDirection={sort.direction}
          onSort={onSortChange}
          width={COLUMN_WIDTHS.size}
          align="right"
        />
        <SortHeader
          field="confidence"
          label={t('results.column.condition')}
          currentField={sort.field}
          currentDirection={sort.direction}
          onSort={onSortChange}
          width={COLUMN_WIDTHS.condition}
        />
      </div>
      <div
        ref={ref}
        className="min-h-0 flex-1 overflow-y-auto overflow-x-hidden"
        tabIndex={0}
        aria-label={t('results.column.name')}
      >
        {viewMode === 'icons' ? (
          <IconGridBody
            scanId={scanId}
            rows={rows}
            rowHeight={rowHeight}
            selectionMode={selectionMode}
            selectedIds={selectedIds}
            exclusions={exclusions}
            activeId={activeId}
            virtualizer={virtualizer}
            onRowClick={onRowClick}
            onRowDoubleClick={onRowDoubleClick}
            onCheckboxChange={onCheckboxChange}
          />
        ) : (
          <div
            style={{
              height: virtualizer.getTotalSize(),
              position: 'relative',
              width: totalWidth,
            }}
          >
            {virtualizer.getVirtualItems().map((vRow) => {
              const idx = vRow.index;
              const isLoader = idx >= rows.length;
              const node = rows[idx];
              if (isLoader) {
                return (
                  <div
                    key="loader"
                    className="absolute inset-x-0 flex items-center justify-center"
                    style={{ top: vRow.start, height: rowHeight }}
                  >
                    <span className="text-xs text-foreground-secondary">
                      {t('common.loading')}
                    </span>
                  </div>
                );
              }
              const isSelected =
                selectionMode === 'all' ? !exclusions.has(node.id) : selectedIds.has(node.id);
              const isActive = activeId === node.id;
              return (
                <div
                  key={node.id}
                  role="row"
                  aria-selected={isSelected}
                  data-active={isActive || undefined}
                  className={cn(
                    'absolute inset-x-0 flex cursor-default items-center text-sm transition-colors',
                    isSelected
                      ? 'bg-surface-selected/60'
                      : isActive
                        ? 'bg-surface-hover'
                        : 'hover:bg-surface-hover/50',
                  )}
                  style={{ top: vRow.start, height: rowHeight }}
                  onClick={(e) => onRowClick(node, idx, e)}
                  onDoubleClick={() => onRowDoubleClick(node)}
                >
                  <div
                    className="flex h-full items-center justify-center"
                    style={{ width: COLUMN_WIDTHS.checkbox }}
                    onClick={(e) => e.stopPropagation()}
                  >
                    <Checkbox
                      checked={isSelected}
                      onCheckedChange={() => onCheckboxChange(node)}
                      aria-label={t('a11y.selectRow')}
                    />
                  </div>
                  <div
                    className="flex h-full items-center gap-2 px-3"
                    style={{ width: COLUMN_WIDTHS.name }}
                  >
                    <FileIconFor node={node} />
                    <span className="truncate" title={node.name}>
                      {node.name}
                    </span>
                  </div>
                  <div
                    className="flex h-full items-center px-3 text-foreground-secondary"
                    style={{ width: COLUMN_WIDTHS.modified }}
                  >
                    <span className="truncate tabular">
                      {formatDate(node.modifiedAt, i18n.language)}
                    </span>
                  </div>
                  <div
                    className="flex h-full items-center px-3 text-foreground-secondary"
                    style={{ width: COLUMN_WIDTHS.type }}
                  >
                    <span className="truncate uppercase">
                      {node.extension?.replace('.', '') ?? ''}
                    </span>
                  </div>
                  <div
                    className="flex h-full items-center justify-end px-3 tabular"
                    style={{ width: COLUMN_WIDTHS.size }}
                  >
                    {formatBytes(node.sizeBytes, i18n.language)}
                  </div>
                  <div
                    className="flex h-full items-center px-3"
                    style={{ width: COLUMN_WIDTHS.condition }}
                  >
                    <ConfidencePillInline grade={node.confidence} />
                  </div>
                </div>
              );
            })}
          </div>
        )}
      </div>
    </div>
  );
}

/** Icon (tile) view — corpus view toggle list⇄icon (atlas §8). Tiles show
 *  thumbnail-or-type-icon, two-line-clamped name, and size; checkbox on
 *  hover/focus; selection + activation mirror the details rows. */
function IconGridBody({
  scanId,
  rows,
  rowHeight,
  selectionMode,
  selectedIds,
  exclusions,
  activeId,
  virtualizer,
  onRowClick,
  onRowDoubleClick,
  onCheckboxChange,
}: {
  scanId: string;
  rows: readonly ResultNode[];
  rowHeight: number;
  selectionMode: 'ids' | 'all';
  selectedIds: ReadonlySet<string>;
  exclusions: ReadonlySet<string>;
  activeId: string | null;
  virtualizer: Virtualizer<HTMLDivElement, Element>;
  onRowClick: (node: ResultNode, index: number, e: React.MouseEvent) => void;
  onRowDoubleClick: (node: ResultNode) => void;
  onCheckboxChange: (node: ResultNode) => void;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  return (
    <div style={{ height: virtualizer.getTotalSize(), position: 'relative' }}>
      {virtualizer.getVirtualItems().map((vRow) => {
        const idx = vRow.index;
        const isLoader = idx >= rows.length;
        const node = rows[idx];
        if (isLoader) {
          return (
            <LoaderRow
              key="loader"
              top={vRow.start}
              height={rowHeight}
              label={t('common.loading')}
            />
          );
        }
        const isSelected = rowSelected(selectionMode, selectedIds, exclusions, node.id);
        const isActive = activeId === node.id;
        return (
          <div
            key={node.id}
            role="row"
            aria-selected={isSelected}
            data-active={isActive || undefined}
            className={cn(
              'group/tile absolute flex cursor-default items-center gap-3 rounded-md border p-2 transition-colors',
              isSelected
                ? 'border-primary bg-surface-selected/60'
                : isActive
                  ? 'border-border-strong bg-surface-hover'
                  : 'border-transparent hover:bg-surface-hover/50',
            )}
            style={{ top: vRow.start, height: rowHeight, left: 8, right: 8 }}
            onClick={(e) => onRowClick(node, idx, e)}
            onDoubleClick={() => onRowDoubleClick(node)}
          >
            <div className="flex h-full items-center" onClick={(e) => e.stopPropagation()}>
              <Checkbox
                checked={isSelected}
                onCheckedChange={() => onCheckboxChange(node)}
                aria-label={t('a11y.selectRow')}
                className={cn(
                  'transition-opacity',
                  isSelected
                    ? 'opacity-100'
                    : 'opacity-0 group-hover/tile:opacity-100 focus-visible:opacity-100',
                )}
              />
            </div>
            <IconTile scanId={scanId} node={node} />
            <div className="min-w-0 flex-1">
              <div className="line-clamp-2 text-sm" title={node.name}>
                {node.name}
              </div>
              <div className="mt-0.5 text-xs text-foreground-secondary tabular">
                {formatBytes(node.sizeBytes, i18n.language)}
              </div>
            </div>
          </div>
        );
      })}
    </div>
  );
}

function IconTile({ scanId, node }: { scanId: string; node: ResultNode }): React.JSX.Element {
  const thumb = useThumbnail(scanId, node.id, node.thumbnailAvailable);
  if (thumb.data?.dataUrl) {
    return (
      <img
        src={thumb.data.dataUrl}
        alt=""
        className="size-16 shrink-0 rounded-sm object-cover"
        loading="lazy"
      />
    );
  }
  return (
    <div className="flex size-16 shrink-0 items-center justify-center rounded-sm bg-surface-tint">
      <FileIconFor node={node} />
    </div>
  );
}
