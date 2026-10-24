import * as React from 'react';
import {
  BotMessageSquareIcon,
  FileWarningIcon,
  FolderSearchIcon,
  HardDriveDownloadIcon,
} from 'lucide-react';
import { useTranslation } from 'react-i18next';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogTitle,
} from '@/components/ui/dialog';
import { Popover, PopoverContent, PopoverTrigger } from '@/components/ui/menus';
import { Button } from '@/components/ui/button';
import { cn } from '@/lib/utils';

export type RecoveryPath = 'deleted' | 'missing' | 'damaged';

const PATHS: readonly { id: RecoveryPath; icon: typeof FileWarningIcon }[] = [
  { id: 'deleted', icon: FileWarningIcon },
  { id: 'missing', icon: HardDriveDownloadIcon },
  { id: 'damaged', icon: FolderSearchIcon },
];

/** Non-modal counterpart of the reference assistant flyout. It makes no AI
 * claim and moves into the modal only after the user chooses a real intent. */
export function RecoveryGuideLauncher({
  onChoose,
}: {
  onChoose: (path: RecoveryPath) => void;
}): React.JSX.Element {
  const { t } = useTranslation();
  return (
    <Popover>
      <PopoverTrigger asChild>
        <Button variant="secondary" size="sm">
          <BotMessageSquareIcon aria-hidden />
          {t('guide.trigger')}
        </Button>
      </PopoverTrigger>
      <PopoverContent align="end" className="w-72 p-2">
        <p className="px-1 pb-2 pt-1 text-xs font-semibold text-foreground">
          {t('guide.panelTitle')}
        </p>
        {PATHS.map(({ id, icon: Icon }) => (
          <button
            key={id}
            type="button"
            onClick={() => onChoose(id)}
            className="flex w-full items-start gap-2 rounded-sm px-2 py-2 text-left transition-colors hover:bg-surface-hover focus-visible:ring-2 focus-visible:ring-ring"
          >
            <Icon className="mt-0.5 size-4 shrink-0 text-primary" aria-hidden />
            <span>
              <span className="block text-xs font-semibold">{t(`guide.${id}`)}</span>
              <span className="mt-0.5 block text-2xs leading-relaxed text-foreground-secondary">
                {t(`guide.${id}Body`)}
              </span>
            </span>
          </button>
        ))}
      </PopoverContent>
    </Popover>
  );
}

/** The reference's full-screen solution panel, rebuilt as a focused,
 * keyboard-safe three-step guide tied to real downstream paths. */
export function RecoveryGuideDialog({
  open,
  path,
  onOpenChange,
  onStart,
}: {
  open: boolean;
  path: RecoveryPath | null;
  onOpenChange: (open: boolean) => void;
  onStart: (path: RecoveryPath) => void;
}): React.JSX.Element {
  const { t } = useTranslation();
  const [step, setStep] = React.useState<1 | 2 | 3>(1);
  if (!path) return <Dialog open={false} onOpenChange={onOpenChange} />;

  const handleOpenChange = (next: boolean): void => {
    if (!next) setStep(1);
    onOpenChange(next);
  };

  const Icon = PATHS.find((candidate) => candidate.id === path)?.icon ?? FileWarningIcon;
  const heading =
    step === 1
      ? t(`guide.${path}`)
      : step === 2
        ? t('guide.confirmTitle')
        : t('guide.resultTitle');
  const description =
    step === 1
      ? t(`guide.${path}Body`)
      : step === 2
        ? t(`guide.${path}Check`)
        : t(`guide.${path}Result`);

  return (
    <Dialog open={open} onOpenChange={handleOpenChange}>
      <DialogContent className="max-w-xl overflow-hidden p-0">
        <div className="bg-[linear-gradient(145deg,#1677ff,#0958d9)] p-6 text-primary-foreground">
          <div className="flex items-center justify-between text-xs font-semibold text-white/80">
            <span>{t('guide.title')}</span>
            <span>{t('guide.step', { current: step })}</span>
          </div>
          <div className="mt-5 flex items-start gap-3">
            <span className="grid size-11 shrink-0 place-items-center rounded-md bg-white/15">
              <Icon className="size-6" aria-hidden />
            </span>
            <div>
              <DialogTitle className="text-xl text-white">{heading}</DialogTitle>
              <DialogDescription className="mt-1 text-white/85">
                {description}
              </DialogDescription>
            </div>
          </div>
        </div>
        <div className="p-6">
          <div className="flex items-center gap-2" aria-label={t('guide.progress')}>
            {[1, 2, 3].map((index) => (
              <span
                key={index}
                className={cn(
                  'h-1 flex-1 rounded-full',
                  index <= step ? 'bg-primary' : 'bg-surface-muted',
                )}
              />
            ))}
          </div>
          <p className="mt-5 text-sm leading-relaxed text-foreground-secondary">
            {step === 1
              ? t('guide.stepOneBody')
              : step === 2
                ? t('guide.stepTwoBody')
                : t('guide.stepThreeBody')}
          </p>
          <DialogFooter className="mt-7">
            {step > 1 ? (
              <Button variant="secondary" onClick={() => setStep((step - 1) as 1 | 2)}>
                {t('guide.back')}
              </Button>
            ) : null}
            <Button
              variant="primary"
              onClick={() => {
                if (step < 3) setStep((step + 1) as 2 | 3);
                else onStart(path);
              }}
            >
              {step === 3 ? t('guide.start') : t('guide.next')}
            </Button>
          </DialogFooter>
        </div>
      </DialogContent>
    </Dialog>
  );
}
