import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { InfoIcon, ShieldCheckIcon, WrenchIcon } from 'lucide-react';
import { EmptyState } from '@/components/ui/empty-state';

/**
 * File Repair page — the TRUTHFUL surface (NG-12).
 *
 * The pre-fix page faked a repairable-file queue (hard-coded sample files
 * with a stranger's home directory, a 'Repair All' button that flipped
 * statuses without doing anything) on top of bridge commands that
 * truthfully refuse with `not-implemented`. Faking is removed. What repair
 * the product REALLY ships today: the scan-time pre-scan repair — when a
 * scan finds file-system damage it can fix, the scan page raises the
 * consent prompt (G-1 (a), wired end to end), and the approved repair
 * runs transactionally with an undo log. The standalone workbench
 * (pick files → repair) requires the ABI trio the plan §3.8 named
 * (cr_ui_repair_analyze/apply/undo) and stays honestly unavailable until
 * then (docs/INTEGRATION-GAPS.md G-1 (b)).
 */
export function RepairPage(): React.JSX.Element {
  const { t } = useTranslation();

  return (
    <div className="h-full overflow-y-auto px-8 py-6">
      <div className="mx-auto w-full max-w-3xl space-y-5 pb-6">
        <div>
          <h1 className="text-[20px] font-bold tracking-tight text-[#1E293B] dark:text-foreground">
            {t('repair.title')}
          </h1>
          <p className="mt-1 text-[13px] text-[#64748B] dark:text-foreground-secondary">
            {t('repair.introBody')}
          </p>
        </div>

        <EmptyState
          icon={WrenchIcon}
          title={t('repair.workbenchUnavailableTitle')}
          description={t('repair.workbenchUnavailableBody')}
        />

        <div className="rounded-[12px] border border-[#E2ECF9] bg-[#F8FAFD] p-5 dark:border-border dark:bg-surface">
          <div className="flex items-start gap-3">
            <ShieldCheckIcon className="mt-0.5 size-5 shrink-0 text-[#10B981]" aria-hidden />
            <div className="space-y-2">
              <h2 className="text-[14px] font-bold text-[#1E293B] dark:text-foreground">
                {t('repair.availableTodayTitle')}
              </h2>
              <p className="text-[12.5px] leading-relaxed text-[#64748B] dark:text-foreground-secondary">
                {t('repair.availableTodayBody')}
              </p>
            </div>
          </div>
        </div>

        <p className="flex items-start gap-1.5 text-[11.5px] leading-relaxed text-[#94A3B8] dark:text-foreground-secondary">
          <InfoIcon className="mt-0.5 size-3.5 shrink-0" aria-hidden />
          {t('repair.undoNote')}
        </p>
      </div>
    </div>
  );
}
