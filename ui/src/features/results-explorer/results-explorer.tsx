import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router';
import type { ResultNode, ResultTreeMode } from '@/connector/contract';
import {
  ArrowLeftIcon,
  ChevronRightIcon,
  FilterIcon,
  FolderTreeIcon,
  HomeIcon,
  LayoutGridIcon,
  ListIcon,
  RotateCcwIcon,
  SaveIcon,
  SearchIcon,
  SparklesIcon,
  Trash2Icon,
  UploadIcon,
  XIcon,
} from 'lucide-react';
import { useResultsPages, useResultTree } from '@/queries/results';
import { useSaveSession } from '@/queries/sessions';
import { useFilterStore, useSortStore, resolveFilter } from '@/stores/filter-store';
import { useSelectionStore } from '@/stores/selection-store';
import { useUiStore } from '@/stores/ui-store';
import { formatBytes, formatCountCompact } from '@/lib/format';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { ResultsTree } from '@/components/ui/results-tree';
import { VirtualFileList } from '@/components/ui/virtual-file-list';
import { Segmented } from '@/components/ui/stepper';
import { EmptyState } from '@/components/ui/empty-state';
import { Skeleton } from '@/components/ui/scroll-area';
import { Input } from '@/components/ui/input';
import {
  ContextMenu,
  ContextMenuContent,
  ContextMenuItem,
  ContextMenuSeparator,
  ContextMenuTrigger,
} from '@/components/ui/menus';
import { FilterBar } from './filter-bar';
import { PreviewPane } from '@/features/preview/preview-pane';
import { toast } from 'sonner';

/**
 * Results Explorer — the 3-pane surface (tree / virtualized list / preview)
 * with toolbar (nav cluster, breadcrumb, view toggle, filter, search),
 * toggleable filter bar, context menu, and the contextual bottom CTA.
 * Also renders (live mode) while the scan runs.
 */
export function ResultsExplorer({
  scanId,
  targetId: _targetId,
  liveMode = false,
  recoverableDuringScan = true,
}: {
  scanId: string;
  targetId: string;
  liveMode?: boolean;
  recoverableDuringScan?: boolean;
}): React.JSX.Element {
  void _targetId;
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();
  const filter = useFilterStore();
  const sortStore = useSortStore();
  const selection = useSelectionStore();
  const ui = useUiStore();
  const saveSession = useSaveSession(scanId);

  const [filterBarOpen, setFilterBarOpen] = React.useState(false);
  const [searchDraft, setSearchDraft] = React.useState(filter.search);
  const [activeNodeId, setActiveNodeId] = React.useState<string | null>(null);

  const filterPayload = React.useMemo(
    () => ({ ...resolveFilter(), nodeId: filter.treeNodeId, treeMode: filter.treeMode }),
    [
      filter.treeNodeId,
      filter.treeMode,
      filter.searchApplied,
      filter.categories,
      filter.datePreset,
      filter.dateFrom,
      filter.dateTo,
      filter.sizePreset,
      filter.sizeMin,
      filter.sizeMax,
      filter.confidence,
      filter.extensions,
      filter.showDeletedOnly,
    ],
  );

  const sort = { field: sortStore.field, direction: sortStore.direction };
  const pages = useResultsPages(scanId, filterPayload, sort);
  const rootTree = useResultTree(scanId, null, filter.treeMode);

  const rows = React.useMemo(
    () => pages.data?.pages.flatMap((p) => p.nodes) ?? [],
    [pages.data],
  );
  const total = pages.data?.pages[pages.data.pages.length - 1]?.total ?? 0;

  const rowHeight = ui.viewMode === 'details' ? 36 : 84;
  const selectedCount = selection.selected.size;
  const selectedBytes = React.useMemo(
    () =>
      rows.filter((r) => selection.selected.has(r.id)).reduce((acc, r) => acc + r.sizeBytes, 0),
    [rows, selection.selected],
  );

  const onRowClick = (node: ResultNode, index: number, e: React.MouseEvent): void => {
    setActiveNodeId(node.id);
    if (e.shiftKey) {
      // range select from last anchor within the visible rows
      const anchorIdx = rows.findIndex((r) => r.id === selection.anchor);
      if (anchorIdx >= 0) {
        const [from, to] = anchorIdx < index ? [anchorIdx, index] : [index, anchorIdx];
        selection.setOnly(rows.slice(from, to + 1).map((r) => r.id));
        return;
      }
    }
    if (e.ctrlKey || e.metaKey) {
      selection.toggle(node.id);
    }
    selection.setAnchor(node.id);
  };

  const onCheckboxChange = (node: ResultNode): void => {
    selection.toggle(node.id);
  };

  const onRecover = (ids?: readonly string[]): void => {
    const finalIds = ids ?? Array.from(selection.selected);
    if (finalIds.length === 0) {
      // Recover-all path
      void navigate(`/recover/${scanId}?all=1`);
      return;
    }
    void navigate(`/recover/${scanId}?ids=${finalIds.slice(0, 500).join(',')}`);
  };

  const treeModeLabel: Record<ResultTreeMode, string> = {
    path: t('results.pathTab'),
    type: t('results.typeTab'),
    raw: t('results.tree.rawFound'),
  };

  return (
    <div className="flex h-full min-h-0 flex-col bg-surface">
      {/* Toolbar */}
      <div className="flex h-11 shrink-0 items-center gap-2 border-b border-border px-2.5">
        <div className="flex items-center gap-0.5">
          <Button
            variant="ghost"
            size="icon"
            aria-label={t('nav.home')}
            onClick={() => navigate('/')}
          >
            <HomeIcon aria-hidden />
          </Button>
          <Button
            variant="ghost"
            size="icon"
            aria-label={t('common.back')}
            onClick={() => navigate(-1)}
          >
            <ArrowLeftIcon aria-hidden />
          </Button>
          <Button
            variant="ghost"
            size="icon"
            aria-label={t('nav.upOneLevel')}
            onClick={() => filter.setTreeNode(null)}
            disabled={filter.treeMode !== 'path' || !filter.treeNodeId}
          >
            <RotateCcwIcon aria-hidden />
          </Button>
        </div>

        {/* Breadcrumb / search-results chip */}
        <div className="flex min-w-0 flex-1 items-center gap-1 overflow-hidden rounded-md border border-border bg-surface-muted/60 px-2.5 py-1 text-sm">
          {filter.searchApplied ? (
            <>
              <SearchIcon className="size-3.5 shrink-0 text-foreground-secondary" aria-hidden />
              <span className="truncate">
                {t('results.searchPlaceholder')}: “{filter.searchApplied}”
              </span>
              <button
                type="button"
                className="ml-auto rounded-full p-0.5 hover:bg-surface-hover"
                aria-label={t('results.clearFilter')}
                onClick={() => {
                  filter.setSearch('');
                  filter.commitSearch();
                  setSearchDraft('');
                }}
              >
                <XIcon className="size-3.5" aria-hidden />
              </button>
            </>
          ) : (
            <>
              <FolderTreeIcon
                className="size-3.5 shrink-0 text-foreground-secondary"
                aria-hidden
              />
              <span className="truncate font-medium">
                {filter.treeMode === 'path'
                  ? (rootTree.data?.nodes.find((n) => n.id === filter.treeNodeId)?.label ??
                    treeModeLabel[filter.treeMode])
                  : treeModeLabel[filter.treeMode]}
              </span>
              <ChevronRightIcon
                className="size-3.5 shrink-0 text-foreground-disabled"
                aria-hidden
              />
              <span className="truncate text-foreground-secondary tabular">
                {t('results.status.found', {
                  count: formatCountCompact(total, i18n.language),
                  size: formatBytes(0, i18n.language),
                }).replace(/ · .*$/, '')}
              </span>
            </>
          )}
        </div>

        <Segmented
          ariaLabel={t('results.views.details')}
          value={ui.viewMode}
          onChange={(v) => ui.setViewMode(v)}
          options={[
            { value: 'details', label: '', icon: ListIcon },
            { value: 'icons', label: '', icon: LayoutGridIcon },
          ]}
        />
        <Button
          variant={filterBarOpen ? 'primary' : 'secondary'}
          size="sm"
          onClick={() => setFilterBarOpen((v) => !v)}
          aria-expanded={filterBarOpen}
        >
          <FilterIcon aria-hidden />
          {t('results.filter')}
          {filter.hasActiveFilters() ? (
            <Badge variant="neutral" className="ml-1 px-1.5">
              •
            </Badge>
          ) : null}
        </Button>
        <div className="relative w-48">
          <SearchIcon
            className="pointer-events-none absolute left-2.5 top-1/2 size-3.5 -translate-y-1/2 text-foreground-disabled"
            aria-hidden
          />
          <Input
            className="h-7 pl-8 text-xs"
            placeholder={t('results.searchPlaceholder')}
            value={searchDraft}
            onChange={(e) => setSearchDraft(e.target.value)}
            onKeyDown={(e) => {
              if (e.key === 'Enter') {
                filter.setSearch(searchDraft);
                filter.commitSearch();
              }
            }}
            aria-label={t('results.searchPlaceholder')}
          />
        </div>
      </div>

      {/* Filter bar */}
      {filterBarOpen ? <FilterBar /> : null}

      {/* 3-pane */}
      <div className="flex min-h-0 flex-1">
        {/* Left tree */}
        <aside
          className="w-56 shrink-0 overflow-y-auto border-r border-border"
          aria-label={t('results.pathTab')}
        >
          <div className="border-b border-border p-2">
            <Segmented
              ariaLabel={t('results.pathTab')}
              value={filter.treeMode === 'raw' ? 'type' : filter.treeMode}
              onChange={(v: ResultTreeMode) => filter.setTreeMode(v)}
              className="w-full"
              options={[
                { value: 'path', label: t('results.pathTab') },
                { value: 'type', label: t('results.typeTab') },
              ]}
            />
          </div>
          <ResultsTree
            scanId={scanId}
            mode={filter.treeMode}
            selectedId={filter.treeNodeId}
            onSelect={(nodeId) => filter.setTreeNode(nodeId)}
          />
        </aside>

        {/* Center list */}
        <div className="min-w-0 flex-1">
          <ContextMenu>
            <ContextMenuTrigger asChild>
              <div className="h-full">
                {pages.isLoading ? (
                  <div className="space-y-px p-2">
                    {Array.from({ length: 14 }, (_, i) => (
                      <Skeleton key={i} className="h-8" />
                    ))}
                  </div>
                ) : rows.length === 0 ? (
                  <EmptyState
                    icon={filter.hasActiveFilters() ? FilterIcon : FolderTreeIcon}
                    title={
                      filter.hasActiveFilters()
                        ? t('results.emptyFilterTitle')
                        : t('results.emptyTitle')
                    }
                    description={
                      filter.hasActiveFilters()
                        ? t('results.emptyFilterBody')
                        : t('results.emptyBody')
                    }
                    actionLabel={
                      filter.hasActiveFilters() ? t('results.clearFilter') : undefined
                    }
                    onAction={filter.hasActiveFilters() ? () => filter.reset() : undefined}
                  />
                ) : (
                  <VirtualFileList
                    rows={rows}
                    total={total}
                    rowHeight={rowHeight}
                    selected={selection.selected}
                    activeId={activeNodeId}
                    onRowClick={onRowClick}
                    onRowDoubleClick={(node) => setActiveNodeId(node.id)}
                    onCheckboxChange={onCheckboxChange}
                    onSelectAll={(checked) =>
                      checked ? selection.setOnly(rows.map((r) => r.id)) : selection.clear()
                    }
                    allSelected={selectedCount === rows.length && rows.length > 0}
                    sort={sort}
                    onSortChange={(field) => sortStore.setSort(field)}
                    loadMore={() => void pages.fetchNextPage()}
                    hasNextPage={pages.hasNextPage}
                  />
                )}
              </div>
            </ContextMenuTrigger>
            <ContextMenuContent>
              <ContextMenuItem onClick={() => onRecover()}>
                <UploadIcon aria-hidden />
                {t('results.context.recover')}
              </ContextMenuItem>
              <ContextMenuItem onClick={() => activeNodeId && setActiveNodeId(activeNodeId)}>
                <SearchIcon aria-hidden />
                {t('results.context.preview')}
              </ContextMenuItem>
              <ContextMenuSeparator />
              <ContextMenuItem disabled title={t('settings.connectorTauri')}>
                <FolderTreeIcon aria-hidden />
                {t('results.context.openLocation')}
              </ContextMenuItem>
              <ContextMenuItem
                onClick={() =>
                  selection.setOnly(
                    rows.filter((r) => r.origin !== 'existing').map((r) => r.id),
                  )
                }
              >
                <Trash2Icon aria-hidden />
                {t('results.context.selectUnrecovered')}
              </ContextMenuItem>
              <ContextMenuItem onClick={() => onRecover()}>
                <SparklesIcon aria-hidden />
                {t('results.context.smartSelect')}
              </ContextMenuItem>
              <ContextMenuSeparator />
              <ContextMenuItem onClick={() => selection.clear()}>
                <XIcon aria-hidden />
                {t('results.context.clearSelection')}
              </ContextMenuItem>
            </ContextMenuContent>
          </ContextMenu>
        </div>

        {/* Right preview */}
        {ui.previewPaneVisible ? (
          <aside
            className="shrink-0 overflow-y-auto border-l border-border"
            style={{ width: ui.previewPaneWidth }}
            aria-label={t('preview.title')}
          >
            <PreviewPane scanId={scanId} nodeId={activeNodeId} />
          </aside>
        ) : null}
      </div>

      {/* Bottom status bar */}
      <div className="flex h-14 shrink-0 items-center justify-between gap-4 border-t border-border bg-surface px-4">
        <div className="flex min-w-0 items-center gap-3 text-sm">
          {liveMode ? (
            <Badge variant="warning" className="animate-status-pulse">
              {t('scan.liveResults')}
            </Badge>
          ) : null}
          <span className="truncate tabular text-foreground-secondary">
            {selectedCount > 0
              ? `${t('results.selection.count', { count: selectedCount })} · ${formatBytes(selectedBytes, i18n.language)}`
              : t('results.status.found', {
                  count: formatCountCompact(total, i18n.language),
                  size: formatBytes(totalOf(pages.data?.pages), i18n.language),
                })}
          </span>
        </div>
        <div className="flex items-center gap-2">
          <Button
            variant="ghost"
            size="sm"
            onClick={() =>
              saveSession.mutate(null, {
                onSuccess: () => toast.success(t('results.session.saved')),
              })
            }
          >
            <SaveIcon aria-hidden />
            {t('results.session.save')}
          </Button>
          <Button
            variant="primary"
            size="lg"
            onClick={() => onRecover()}
            disabled={liveMode && !recoverableDuringScan}
            title={liveMode && !recoverableDuringScan ? t('scan.recoverNowUnsafe') : undefined}
          >
            {selectedCount > 0
              ? t('results.status.recoverSelected', { count: selectedCount })
              : t('results.status.recoverAll')}
          </Button>
        </div>
      </div>
    </div>
  );
}

function totalOf(pagesData: readonly { total: number }[] | undefined): number {
  if (!pagesData || pagesData.length === 0) return 0;
  return pagesData[pagesData.length - 1].total;
}
