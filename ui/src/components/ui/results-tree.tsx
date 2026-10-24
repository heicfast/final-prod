import * as React from 'react';
import {
  ChevronRightIcon,
  FolderIcon,
  HardDriveIcon,
  LayersIcon,
  ShapesIcon,
} from 'lucide-react';
import { useTranslation } from 'react-i18next';
import type { ResultTreeMode, TreeNode } from '@/connector/contract';
import { useResultTree } from '@/queries/results';
import { formatBytesCompact, formatCountCompact } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Skeleton } from '@/components/ui/skeleton';

/**
 * Left navigation tree for the results explorer — path/type/raw modes with
 * per-level lazy loading through the query layer (never the connector
 * directly). Each expanded level is its own query-backed sub-component.
 */

function TreeIcon({ icon }: { icon: TreeNode['icon'] }): React.JSX.Element {
  switch (icon) {
    case 'category':
      return <ShapesIcon className="size-4 shrink-0" aria-hidden />;
    case 'group':
      return <LayersIcon className="size-4 shrink-0" aria-hidden />;
    case 'device':
      return <HardDriveIcon className="size-4 shrink-0" aria-hidden />;
    default:
      return <FolderIcon className="size-4 shrink-0" aria-hidden />;
  }
}

function treeLabel(node: TreeNode, t: (k: string) => string): string {
  if (node.special === 'lost-path') return t('results.tree.lostPaths');
  if (node.special === 'raw-found') return t('results.tree.rawFound');
  if (node.id === 'root') return t('results.tree.existingFiles');
  if (node.icon === 'category') return t(`scanConfig.categories.${node.label}`);
  if (node.icon === 'group' && node.label === 'raw') return t('results.tree.rawFound');
  return node.label;
}

function TreeRow({
  node,
  depth,
  mode,
  scanId,
  selectedId,
  onSelect,
  expanded,
  onToggleExpand,
}: {
  node: TreeNode;
  depth: number;
  mode: ResultTreeMode;
  scanId: string;
  selectedId: string | null;
  onSelect: (nodeId: string | null) => void;
  expanded: ReadonlySet<string>;
  onToggleExpand: (id: string) => void;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const hasKids =
    mode === 'path' &&
    node.childCount > 0 &&
    node.special !== 'raw-found' &&
    node.id !== 'lost-paths' &&
    node.id !== 'raw-all';
  const isExpanded = expanded.has(node.id);
  const isSelected = selectedId === node.id;
  const label = treeLabel(node, t);

  return (
    <>
      <button
        type="button"
        role="treeitem"
        aria-expanded={hasKids ? isExpanded : undefined}
        aria-selected={isSelected}
        onClick={() => {
          onSelect(node.id);
          if (hasKids) onToggleExpand(node.id);
        }}
        className={cn(
          'flex h-7 w-full items-center gap-1 rounded-xs px-1.5 text-left text-sm outline-none transition-colors',
          'focus-visible:ring-2 focus-visible:ring-ring',
          isSelected
            ? 'bg-surface-selected font-medium text-primary'
            : 'text-foreground-secondary hover:bg-surface-hover hover:text-foreground',
        )}
        style={{ paddingLeft: `${6 + depth * 14}px` }}
      >
        {hasKids ? (
          <ChevronRightIcon
            className={cn(
              'size-3.5 shrink-0 transition-transform duration-150',
              isExpanded && 'rotate-90',
            )}
            aria-hidden
          />
        ) : (
          <span className="w-3.5 shrink-0" aria-hidden />
        )}
        <TreeIcon icon={node.icon} />
        <span className="min-w-0 flex-1 truncate">{label}</span>
        <span className="shrink-0 text-2xs tabular text-foreground-disabled">
          {formatCountCompact(node.fileCount, i18n.language)}
        </span>
        <span className="hidden shrink-0 text-2xs tabular text-foreground-disabled xl:inline">
          {formatBytesCompact(node.sizeBytes, i18n.language)}
        </span>
      </button>
      {hasKids && isExpanded ? (
        <TreeLevel
          scanId={scanId}
          parentId={node.id}
          mode={mode}
          depth={depth + 1}
          selectedId={selectedId}
          onSelect={onSelect}
          expanded={expanded}
          onToggleExpand={onToggleExpand}
        />
      ) : null}
    </>
  );
}

function TreeLevel({
  scanId,
  parentId,
  mode,
  depth,
  selectedId,
  onSelect,
  expanded,
  onToggleExpand,
}: {
  scanId: string;
  parentId: string | null;
  mode: ResultTreeMode;
  depth: number;
  selectedId: string | null;
  onSelect: (nodeId: string | null) => void;
  expanded: ReadonlySet<string>;
  onToggleExpand: (id: string) => void;
}): React.JSX.Element {
  const query = useResultTree(scanId, parentId, mode);
  if (query.isLoading) {
    return (
      <>
        {Array.from({ length: 4 }, (_, i) => (
          <div
            key={i}
            className="flex items-center gap-2 py-1"
            style={{ paddingLeft: `${6 + depth * 14}px` }}
          >
            <Skeleton className="size-4" />
            <Skeleton className="h-3 flex-1" />
          </div>
        ))}
      </>
    );
  }
  if (query.isError || !query.data) {
    return (
      <div className="px-3 py-1 text-xs text-danger">
        {query.error instanceof Error ? query.error.message : 'error'}
      </div>
    );
  }
  return (
    <>
      {query.data.nodes.map((node) => (
        <TreeRow
          key={node.id}
          node={node}
          depth={depth}
          mode={mode}
          scanId={scanId}
          selectedId={selectedId}
          onSelect={onSelect}
          expanded={expanded}
          onToggleExpand={onToggleExpand}
        />
      ))}
    </>
  );
}

export function ResultsTree({
  scanId,
  mode,
  selectedId,
  onSelect,
  className,
}: {
  scanId: string;
  mode: ResultTreeMode;
  selectedId: string | null;
  onSelect: (nodeId: string | null) => void;
  className?: string;
}): React.JSX.Element {
  const { t } = useTranslation();
  // Expanded set is derived per mode; toggling writes the current mode's set.
  const [expandedByMode, setExpandedByMode] = React.useState<
    Record<string, ReadonlySet<string>>
  >({
    path: new Set(['root']),
    type: new Set(),
    raw: new Set(),
  });
  const expanded = expandedByMode[mode] ?? new Set<string>();

  const toggleExpand = (id: string): void => {
    setExpandedByMode((prev) => {
      const current = prev[mode] ?? new Set<string>();
      const next = new Set(current);
      if (next.has(id)) next.delete(id);
      else next.add(id);
      return { ...prev, [mode]: next };
    });
  };

  return (
    <div role="tree" aria-label={t('a11y.sidebarRegion')} className={cn('p-1', className)}>
      <TreeLevel
        scanId={scanId}
        parentId={null}
        mode={mode}
        depth={0}
        selectedId={selectedId}
        onSelect={onSelect}
        expanded={expanded}
        onToggleExpand={toggleExpand}
      />
    </div>
  );
}
