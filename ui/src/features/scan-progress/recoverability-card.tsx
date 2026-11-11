import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { GaugeIcon } from 'lucide-react';
import { useScanAssistant } from '@/queries/imaging';
import type { ScanAssistantStats } from '@/connector/contract';

/**
 * Recoverability card — the reference's completion-time grading surface
 * (UIUX/30:19 "Great news: Your files are highly recoverable"; the
 * RecoveryAssistant block, DRWUI-subsystems:9-52; lane C). The verdict
 * derives from the ENGINE's matched/total gauge axes (never client-side
 * re-grading); the og's small-result rule is mirrored: under 100 graded
 * files the gauge hides (the og forces matched to 0 there).
 */

/** The verdict bands — the reference's three-tier verdict vocabulary. */
function verdictOf(stats: ScanAssistantStats): {
  key: 'high' | 'medium' | 'low';
  ratio: number;
} {
  if (stats.smallResult || stats.total <= 0) {
    return { key: 'medium', ratio: 0 }; // the gauge hides; no verdict shown
  }
  const ratio = stats.matched / stats.total;
  if (ratio >= 0.8) return { key: 'high', ratio };
  if (ratio >= 0.5) return { key: 'medium', ratio };
  return { key: 'low', ratio };
}

export function RecoverabilityCard({
  scanId,
  completed,
}: {
  scanId: string;
  completed: boolean;
}): React.JSX.Element | null {
  const { t, i18n } = useTranslation();
  const assistant = useScanAssistant(scanId, completed);
  const stats = assistant.data ?? null;
  if (!stats) return null;

  const locale = i18n.language;
  const { key, ratio } = verdictOf(stats);
  const showGauge = !stats.smallResult && stats.total > 0;
  const ring =
    key === 'high' ? 'text-[#16A34A]' : key === 'medium' ? 'text-[#F59E0B]' : 'text-[#DC2626]';

  return (
    <div
      className="mt-3 rounded-lg border border-[#E2E8F0] bg-[#F7F8FA] px-4 py-3 text-left dark:border-border dark:bg-surface-muted"
      aria-label={t('scan.recoverability.title')}
    >
      <div className="flex items-center gap-3">
        <div className="relative flex size-11 shrink-0 items-center justify-center">
          {/* The circular gauge (the reference's "Recovery Chances" dial) */}
          <svg viewBox="0 0 44 44" className="size-11 -rotate-90" aria-hidden>
            <circle
              cx="22"
              cy="22"
              r="18"
              fill="none"
              strokeWidth="4"
              className="stroke-[#E2E8F0] dark:stroke-border"
            />
            {showGauge ? (
              <circle
                cx="22"
                cy="22"
                r="18"
                fill="none"
                strokeWidth="4"
                strokeLinecap="round"
                strokeDasharray={`${Math.round(ratio * 113)} 113`}
                className={
                  key === 'high'
                    ? 'stroke-[#16A34A]'
                    : key === 'medium'
                      ? 'stroke-[#F59E0B]'
                      : 'stroke-[#DC2626]'
                }
              />
            ) : null}
          </svg>
          <GaugeIcon className={`absolute size-4 ${ring}`} aria-hidden />
        </div>
        <div className="min-w-0 flex-1">
          <div className="text-xs font-semibold text-foreground">
            {showGauge
              ? t(`scan.recoverability.verdict.${key}`, {
                  percent: Math.round(ratio * 100),
                })
              : t('scan.recoverability.graded', {
                  count: stats.gradedCount.toLocaleString(locale),
                })}
          </div>
          <div className="mt-0.5 text-[11px] text-foreground-secondary tabular">
            {t('scan.recoverability.matched', {
              matched: stats.matched.toLocaleString(locale),
              total: stats.total.toLocaleString(locale),
            })}
          </div>
        </div>
      </div>
      {/* The per-category counts row (the RecoveryAssistant block) */}
      <div className="mt-2.5 flex flex-wrap gap-x-3 gap-y-1 border-t border-[#E2E8F0] pt-2 text-[10px] text-foreground-secondary dark:border-border">
        {CATEGORY_ROWS.map(([k, labelKey]) => (
          <span key={k} className="tabular">
            {t(labelKey)}: {(stats.categoryCounts[k] ?? 0).toLocaleString(locale)}
          </span>
        ))}
      </div>
    </div>
  );
}

/** The category rows shown (the og's top categories; the full 8-slot
 * block stays on the data surface — the completion card is a summary). */
const CATEGORY_ROWS: readonly (readonly [
  'image' | 'video' | 'audio' | 'document' | 'archive',
  string,
])[] = [
  ['image', 'scan.recoverability.category.image'],
  ['video', 'scan.recoverability.category.video'],
  ['audio', 'scan.recoverability.category.audio'],
  ['document', 'scan.recoverability.category.document'],
  ['archive', 'scan.recoverability.category.archive'],
];
