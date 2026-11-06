import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { useQueryClient } from '@tanstack/react-query';
import { ShieldAlertIcon } from 'lucide-react';
import { useConsentStore } from '@/stores/consent-store';
import { queryKeys } from '@/queries/client';
import { useRespondToConsent } from '@/queries/scan';
import { Button } from '@/components/ui/button';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { toast } from 'sonner';

/**
 * The global consent modal — BOTH families (pre-scan repair G-1 (a),
 * lost-partition apply S-029). Mounted once in the shell so a pending
 * prompt survives any navigation (the store is global; re-hydration rides
 * getScanState().pendingConsent / pendingApplyConsent()).
 *
 * Non-dismissable (plan §4.5): Escape and outside-click are prevented — a
 * non-answer is a user decision and the engine thread waits. The scan's own
 * Stop button force-denies through cancel (the bridge closes the gate
 * FIRST, so cancel can never deadlock).
 */
export function ConsentModal(): React.JSX.Element | null {
  const { t } = useTranslation();
  const qc = useQueryClient();
  const repair = useConsentStore((s) => s.repair);
  const apply = useConsentStore((s) => s.apply);
  const clearRepair = useConsentStore((s) => s.clearRepair);
  const clearApply = useConsentStore((s) => s.clearApply);
  const [busy, setBusy] = React.useState(false);
  const respond = useRespondToConsent();

  const event = repair ?? apply;
  const isRepair = repair !== null;

  if (!event) return null;

  const answer = (allow: boolean): void => {
    const requestId = event.requestId;
    setBusy(true);
    respond.mutate(
      { requestId, allow },
      {
        onSuccess: () => {
          if (isRepair) {
            clearRepair(requestId);
            if (repair)
              void qc.invalidateQueries({ queryKey: queryKeys.scanState(repair.scanId) });
          } else {
            clearApply(requestId);
          }
        },
        onError: (err) => {
          toast.error(err instanceof Error ? err.message : t('errors.unknown'));
        },
        onSettled: () => setBusy(false),
      },
    );
  };

  const onAllow = (): void => answer(true);
  const onDeny = (): void => answer(false);

  const plan = isRepair ? null : (apply as ApplyPlanShape).plan;
  const repairPlan = repair?.plan ?? null;

  return (
    <Dialog open onOpenChange={() => undefined}>
      <DialogContent
        className="max-w-md"
        showCloseButton={false}
        onEscapeKeyDown={(e) => e.preventDefault()}
        onPointerDownOutside={(e) => e.preventDefault()}
        onInteractOutside={(e) => e.preventDefault()}
      >
        <DialogHeader>
          <DialogTitle className="flex items-center gap-2">
            <ShieldAlertIcon className="size-5 text-warning" aria-hidden />
            {isRepair ? t('consent.repairTitle') : t('consent.applyTitle')}
          </DialogTitle>
          <DialogDescription>
            {isRepair ? t('consent.repairBody') : t('consent.applyBody')}
          </DialogDescription>
        </DialogHeader>

        {repairPlan ? (
          <dl className="space-y-1.5 text-[13px]">
            <ConsentRow label={t('consent.filesystem')} value={repairPlan.fsFamily || '—'} />
            <ConsentRow
              label={t('consent.actionCount')}
              value={t('consent.actionCountValue', { count: repairPlan.actionCount })}
            />
            <ConsentRow
              label={t('consent.advisoryCount')}
              value={t('consent.advisoryCountValue', { count: repairPlan.advisoryCount })}
            />
            {repairPlan.refused ? (
              <ConsentRow label={t('consent.refused')} value={repairPlan.reason} />
            ) : null}
          </dl>
        ) : null}

        {plan ? (
          <dl className="space-y-1.5 text-[13px]">
            <ConsentRow label={t('consent.startLba')} value={String(plan.startLba)} />
            <ConsentRow label={t('consent.sectorCount')} value={String(plan.sectorCount)} />
            <ConsentRow
              label={t('consent.scope')}
              value={plan.scope === 1 ? t('consent.scopeRebuild') : t('consent.scopeEntry')}
            />
            <ConsentRow label={t('consent.mbrSlot')} value={`#${plan.slot}`} />
            {plan.regenerateSignature ? (
              <ConsentRow label={t('consent.signature')} value={t('consent.regenerates')} />
            ) : null}
            {plan.refused ? (
              <ConsentRow label={t('consent.refused')} value={plan.reason} />
            ) : null}
          </dl>
        ) : null}

        <p className="text-xs text-foreground-secondary">{t('consent.writesNote')}</p>

        <DialogFooter>
          <Button variant="secondary" onClick={onDeny} disabled={busy}>
            {t('consent.deny')}
          </Button>
          <Button
            variant="primary"
            onClick={onAllow}
            disabled={
              busy ||
              (repairPlan?.refused ?? 0) > 0 ||
              repairPlan?.actionCount === 0 ||
              (plan?.refused ?? 0) > 0
            }
          >
            {t('consent.approve')}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}

interface ApplyPlanShape {
  plan: {
    refused: number;
    scope: number;
    startLba: number;
    sectorCount: number;
    mbrType: number;
    slot: number;
    regenerateSignature: number;
    signature: number;
    reason: string;
  };
}

function ConsentRow({ label, value }: { label: string; value: string }): React.JSX.Element {
  return (
    <div className="flex items-baseline justify-between gap-4">
      <dt className="text-foreground-secondary">{label}</dt>
      <dd className="text-right font-medium">{value}</dd>
    </div>
  );
}
