import * as React from 'react';
import { useTranslation } from 'react-i18next';
import type { PreviewKind, PreviewPayload, ResultNode } from '@/connector/contract';
import {
  FileWarningIcon,
  ImageIcon,
  Maximize2Icon,
  MinusIcon,
  PlusIcon,
  TextIcon,
} from 'lucide-react';
import { useNodeDetail, useThumbnail } from '@/queries/results';
import { usePreview } from '@/queries/preview';
import { formatBytes, formatDateTime } from '@/lib/format';
import { cn } from '@/lib/utils';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Skeleton } from '@/components/ui/scroll-area';
import { ConfidenceBadge } from '@/components/ui/confidence-badge';
import { Hint } from '@/components/ui/tooltip';
import { hexWorker } from '@/workers/hex-worker-client';

/**
 * Preview pane — bounded per-kind rendering with connector-driven failure
 * diagnostics. Hex/text parse in a worker (never on the main thread);
 * image/pdf/av/office render connector-provided bounded artifacts.
 */

function kindOfNode(node: ResultNode): PreviewKind {
  const ext = node.extension?.replace('.', '').toLowerCase() ?? '';
  if (
    [
      'jpg',
      'jpeg',
      'png',
      'gif',
      'bmp',
      'webp',
      'svg',
      'heic',
      'tif',
      'tiff',
      'cr2',
      'cr3',
      'nef',
      'arw',
      'dng',
      'raw',
      'ico',
    ].includes(ext)
  )
    return 'image';
  if (['mp4', 'mov', 'avi', 'mkv', 'wmv', 'm4v', 'webm', 'mts', 'm2ts'].includes(ext))
    return 'av';
  if (['wav', 'mp3', 'flac', 'aac', 'ogg', 'wma', 'm4a'].includes(ext)) return 'av';
  if (['pdf'].includes(ext)) return 'pdf';
  if (['txt', 'log', 'md', 'csv', 'tsv', 'ini', 'json', 'xml', 'html', 'htm'].includes(ext))
    return 'text';
  if (['doc', 'docx', 'xls', 'xlsx', 'ppt', 'pptx', 'odt'].includes(ext)) return 'office';
  if (
    ['exe', 'dll', 'db', 'sqlite', 'bin', 'dat', 'tmp', 'pst', 'zip', 'rar', '7z'].includes(ext)
  )
    return 'hex';
  return 'hex';
}

export function PreviewPane({
  scanId,
  nodeId,
}: {
  scanId: string;
  nodeId: string | null;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const detail = useNodeDetail(scanId, nodeId);
  const node = detail.data?.node ?? null;
  const kind = node ? kindOfNode(node) : null;
  const preview = usePreview(scanId, nodeId, kind);

  return (
    <div className="flex h-full min-h-0 flex-col">
      {node ? (
        <>
          <div className="flex items-start gap-3 border-b border-border p-3">
            <div className="mt-0.5 size-10 shrink-0 overflow-hidden rounded-md bg-surface-tint">
              <Thumbnail scanId={scanId} nodeId={node.id} available={node.thumbnailAvailable} />
            </div>
            <div className="min-w-0 flex-1">
              <p className="truncate text-sm font-semibold" title={node.name}>
                {node.name}
              </p>
              <div className="mt-0.5 flex flex-wrap items-center gap-x-3 gap-y-0.5 text-xs text-foreground-secondary tabular">
                <span>{formatBytes(node.sizeBytes, i18n.language)}</span>
                <span>{formatDateTime(node.modifiedAt, i18n.language)}</span>
                <span className="uppercase">{node.extension}</span>
              </div>
            </div>
          </div>

          <div className="flex items-center justify-between border-b border-border px-3 py-1.5">
            <ConfidenceBadge grade={node.confidence} scanId={scanId} nodeId={node.id} />
            <Badge
              variant={
                node.origin === 'existing'
                  ? 'neutral'
                  : node.origin === 'raw'
                    ? 'primary'
                    : 'warning'
              }
            >
              {node.origin === 'existing'
                ? t('preview.statusExisting')
                : node.origin === 'raw'
                  ? t('preview.statusRaw')
                  : t('preview.statusDeleted')}
            </Badge>
          </div>

          <div className="min-h-0 flex-1 overflow-auto p-3">
            <PreviewBody previewQuery={preview} kind={kind} />
          </div>

          {detail.data?.confidenceReason ? (
            <div className="border-t border-border px-3 py-2 text-xs text-foreground-secondary">
              {detail.data.confidenceReason}
            </div>
          ) : null}
        </>
      ) : (
        <div className="flex flex-1 flex-col items-center justify-center gap-2 p-6 text-center text-foreground-disabled">
          <ImageIcon className="size-8" strokeWidth={1.25} aria-hidden />
          <p className="text-xs">{t('preview.title')}</p>
        </div>
      )}
    </div>
  );
}

function Thumbnail({
  scanId,
  nodeId,
  available,
}: {
  scanId: string;
  nodeId: string;
  available: boolean;
}): React.JSX.Element {
  const { data } = useThumbnail(scanId, nodeId, available);
  if (data?.dataUrl) {
    return <img src={data.dataUrl} alt="" className="size-full object-cover" />;
  }
  return (
    <div className="flex size-full items-center justify-center">
      <ImageIcon className="size-4 text-foreground-disabled" aria-hidden />
    </div>
  );
}

function PreviewBody({
  previewQuery,
  kind,
}: {
  previewQuery: ReturnType<typeof usePreview>;
  kind: PreviewKind | null;
}): React.JSX.Element | null {
  const { t } = useTranslation();

  if (previewQuery.isLoading) {
    return (
      <div className="space-y-2">
        <Skeleton className="h-40" />
        <Skeleton className="h-4 w-2/3" />
        <Skeleton className="h-4 w-1/2" />
      </div>
    );
  }

  if (previewQuery.isError) {
    const err = previewQuery.error as { reasonKey?: string; message?: string };
    const reasonKey = err?.reasonKey;
    return (
      <div className="flex flex-col items-center justify-center gap-2 py-10 text-center">
        <FileWarningIcon className="size-8 text-warning" strokeWidth={1.5} aria-hidden />
        <p className="text-sm font-semibold">{t('preview.failedTitle')}</p>
        <p className="max-w-56 text-xs text-foreground-secondary">
          {reasonKey ? t(reasonKey) : (err?.message ?? t('errors.unknown'))}
        </p>
      </div>
    );
  }

  const payload = previewQuery.data;
  if (!payload || !kind) return null;

  switch (payload.kind) {
    case 'image':
      return <ImagePreview payload={payload} />;
    case 'hex':
      // key forces a fresh subcomponent per payload — state resets by
      // remount instead of synchronous setState in an effect.
      return <HexPreview key={`${previewQuery.dataUpdatedAt}`} payload={payload} />;
    case 'text':
      return <TextPreview payload={payload} />;
    case 'pdf':
      return <PdfPreview payload={payload} />;
    case 'av':
      return <AvPreview payload={payload} />;
    case 'office':
      return <OfficePreview payload={payload} />;
  }
  return null;
}

// --------------------------------------------------------------------- kinds

function ImagePreview({
  payload,
}: {
  payload: Extract<PreviewPayload, { kind: 'image' }>;
}): React.JSX.Element {
  const { t } = useTranslation();
  const [zoom, setZoom] = React.useState(1);
  return (
    <div className="flex h-full flex-col">
      <div className="flex items-center justify-end gap-1 pb-2">
        <Button
          variant="ghost"
          size="icon"
          className="size-6"
          aria-label={t('preview.zoomOut')}
          onClick={() => setZoom((z) => Math.max(0.25, z / 1.25))}
        >
          <MinusIcon aria-hidden />
        </Button>
        <span className="w-12 text-center text-2xs tabular text-foreground-secondary">
          {Math.round(zoom * 100)}%
        </span>
        <Button
          variant="ghost"
          size="icon"
          className="size-6"
          aria-label={t('preview.zoomIn')}
          onClick={() => setZoom((z) => Math.min(8, z * 1.25))}
        >
          <PlusIcon aria-hidden />
        </Button>
        <Button
          variant="ghost"
          size="icon"
          className="size-6"
          aria-label={t('preview.fit')}
          onClick={() => setZoom(1)}
        >
          <Maximize2Icon aria-hidden />
        </Button>
      </div>
      <div className="flex min-h-0 flex-1 items-center justify-center overflow-auto rounded-md bg-surface-tint p-2">
        <img
          src={payload.dataUrl}
          alt=""
          style={{ transform: `scale(${zoom})`, transformOrigin: 'center' }}
          className="max-h-full transition-transform duration-150"
        />
      </div>
    </div>
  );
}

function TextPreview({
  payload,
}: {
  payload: Extract<PreviewPayload, { kind: 'text' }>;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  return (
    <div className="flex h-full flex-col gap-2">
      {payload.truncated ? (
        <p className="text-xs text-foreground-secondary">
          {t('preview.truncated', {
            size: formatBytes(payload.text.length, i18n.language),
            total: formatBytes(payload.totalSize, i18n.language),
          })}
        </p>
      ) : null}
      <div className="flex items-center gap-2 text-xs">
        <TextIcon className="size-3.5" aria-hidden />
        <Badge variant="outline">
          {t('preview.encoding')}: {payload.encoding}
        </Badge>
      </div>
      <pre className="min-h-0 flex-1 overflow-auto rounded-md bg-surface-muted p-3 font-mono text-xs leading-relaxed whitespace-pre-wrap">
        {payload.text}
      </pre>
    </div>
  );
}

function PdfPreview({
  payload,
}: {
  payload: Extract<PreviewPayload, { kind: 'pdf' }>;
}): React.JSX.Element {
  const { t } = useTranslation();
  const [page, setPage] = React.useState(0);
  const current = payload.pages[page];
  return (
    <div className="flex h-full flex-col gap-2">
      <img
        src={current?.dataUrl}
        alt={`page ${page + 1}`}
        className="mx-auto max-h-full rounded-md border border-border shadow-card"
      />
      <div className="flex items-center justify-center gap-2 pt-1 text-xs tabular">
        <Button
          variant="ghost"
          size="sm"
          onClick={() => setPage((p) => Math.max(0, p - 1))}
          disabled={page === 0}
          aria-label={t('preview.prevPage')}
        >
          ‹
        </Button>
        <span>{t('preview.pageOf', { page: page + 1, total: payload.pageCount })}</span>
        <Button
          variant="ghost"
          size="sm"
          onClick={() => setPage((p) => Math.min(payload.pageCount - 1, p + 1))}
          disabled={page >= payload.pageCount - 1}
          aria-label={t('preview.nextPage')}
        >
          ›
        </Button>
      </div>
    </div>
  );
}

function AvPreview({
  payload,
}: {
  payload: Extract<PreviewPayload, { kind: 'av' }>;
}): React.JSX.Element {
  const { t } = useTranslation();
  return (
    <div className="space-y-3">
      {payload.thumbnailDataUrl ? (
        <img
          src={payload.thumbnailDataUrl}
          alt=""
          className="w-full rounded-md border border-border"
        />
      ) : null}
      <dl className="space-y-1 text-xs">
        {Object.entries(payload.metadata).map(([k, v]) => (
          <div key={k} className="flex justify-between gap-3">
            <dt className="text-foreground-secondary">{k}</dt>
            <dd className="font-medium tabular">{v}</dd>
          </div>
        ))}
      </dl>
      {payload.durationSeconds ? (
        <p className="text-xs text-foreground-secondary tabular">
          {Math.floor(payload.durationSeconds / 60)}:
          {String(payload.durationSeconds % 60).padStart(2, '0')}
        </p>
      ) : null}
      {!payload.thumbnailDataUrl ? (
        <Hint text={t('preview.reason.unsupportedKind')}>
          <Badge variant="neutral">{t('preview.unsupportedTitle')}</Badge>
        </Hint>
      ) : null}
    </div>
  );
}

function OfficePreview({
  payload,
}: {
  payload: Extract<PreviewPayload, { kind: 'office' }>;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  return (
    <div className="space-y-3">
      <dl className="space-y-1 text-xs">
        {Object.entries(payload.metadata).map(([k, v]) => (
          <div key={k} className="flex justify-between gap-3">
            <dt className="text-foreground-secondary">{k}</dt>
            <dd className="font-medium tabular">{v}</dd>
          </div>
        ))}
      </dl>
      {payload.extractedText ? (
        <>
          {payload.truncated ? (
            <p className="text-xs text-foreground-secondary">
              {t('preview.truncated', {
                size: formatBytes(payload.extractedText.length, i18n.language),
                total: formatBytes(Number(payload.metadata.pages ?? 1) * 24_000, i18n.language),
              })}
            </p>
          ) : null}
          <pre className="max-h-64 overflow-auto rounded-md bg-surface-muted p-3 text-xs whitespace-pre-wrap">
            {payload.extractedText}
          </pre>
        </>
      ) : null}
    </div>
  );
}

function HexPreview({
  payload,
}: {
  payload: Extract<PreviewPayload, { kind: 'hex' }>;
}): React.JSX.Element {
  const { t, i18n } = useTranslation();
  const [rows, setRows] = React.useState<
    { offset: number; hex: string; ascii: string }[] | null
  >(null);
  const [error, setError] = React.useState<string | null>(null);

  React.useEffect(() => {
    let cancelled = false;
    hexWorker(payload.bytes, payload.baseOffset)
      .then((r) => {
        if (!cancelled) setRows(r);
      })
      .catch(() => {
        if (!cancelled) setError(t('errors.unknown'));
      });
    return () => {
      cancelled = true;
    };
  }, [payload.bytes, payload.baseOffset, t]);

  if (error) {
    return <p className="text-xs text-danger">{error}</p>;
  }
  if (!rows) {
    return <Skeleton className="h-48" />;
  }

  return (
    <div className="space-y-2">
      <p className="text-xs text-foreground-secondary tabular">
        {t('preview.firstBytes', {
          size: formatBytes(payload.bytes.byteLength, i18n.language),
        })}
        {payload.totalSize > payload.bytes.byteLength
          ? ` ${t('preview.truncated', { size: formatBytes(payload.bytes.byteLength, i18n.language), total: formatBytes(payload.totalSize, i18n.language) })}`
          : ''}
      </p>
      <div className="overflow-x-auto rounded-md bg-surface-muted p-2">
        <table className="w-full font-mono text-2xs leading-relaxed tabular">
          <tbody>
            {rows.map((r) => (
              <tr key={r.offset}>
                <td className="pr-3 text-foreground-disabled">
                  {r.offset.toString(16).padStart(8, '0')}
                </td>
                <td className="whitespace-pre pr-3">{r.hex}</td>
                <td className="whitespace-pre text-foreground-secondary">{r.ascii}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}

export { cn };
