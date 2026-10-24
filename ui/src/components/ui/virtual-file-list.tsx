import * as React from 'react';
import { useVirtualizer } from '@tanstack/react-virtual';
import type { ResultNode, SortField } from '@/connector/contract';
import { ArrowDownIcon, ArrowUpIcon, ChevronsUpDownIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { cn } from '@/lib/utils';
import { formatBytes, formatDate } from '@/lib/format';
import { ConfidencePillInline } from './confidence-badge';
import { Checkbox } from './checkbox';
import { FileIconFor } from './file-icon';

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
  rows,
  total,
  rowHeight,
  selected,
  activeId,
  onRowClick,
  onRowDoubleClick,
  onCheckboxChange,
  onSelectAll,
  allSelected,
  sort,
  onSortChange,
  onVisibleRangeChange,
  loadMore,
  hasNextPage,
  className,
  viewportRef,
}: {
  rows: readonly ResultNode[];
  total: number;
  rowHeight: number;
  selected: ReadonlySet<string>;
  activeId: string | null;
  onRowClick: (node: ResultNode, index: number, e: React.MouseEvent) => void;
  onRowDoubleClick: (node: ResultNode) => void;
  onCheckboxChange: (node: ResultNode) => void;
  onSelectAll: (checked: boolean) => void;
  allSelected: boolean;
  sort: { field: SortField; direction: 'asc' | 'desc' };
  onSortChange: (field: SortField) => void;
  onVisibleRangeChange?: (start: number, end: number) => void;
  loadMore?: () => void;
  hasNextPage?: boolean;
  className?: string;
  viewportRef?: React.RefObject<HTMLDivElement | null>;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const localRef = React.useRef<HTMLDivElement | null>(null);
  const ref = viewportRef ?? localRef;

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

  React.useEffect(() => {
    const range = virtualizer.range;
    if (range && onVisibleRangeChange) {
      onVisibleRangeChange(range.startIndex, range.endIndex);
    }
  }, [virtualizer.range, onVisibleRangeChange]);

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
              rows.length > 0 && allSelected
                ? true
                : selected.size > 0
                  ? 'indeterminate'
                  : false
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
            const isSelected = selected.has(node.id);
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
      </div>
    </div>
  );
}
