import * as React from 'react';
import { useTranslation } from 'react-i18next';
import { LockOpenIcon } from 'lucide-react';
import { useUnlockVolume } from '@/queries/devices';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
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
 * The N-064 unlock dialog — the explicit-consent action for a locked
 * BitLocker volume. Password candidates ride the engine's shipped
 * dictionary set; the FVEK field feeds the M-033 key open (64/128 hex).
 * Nothing in the scan/open path ever tries passwords by itself.
 */
export function UnlockVolumeDialog({
  volumeId,
  open,
  onOpenChange,
}: {
  volumeId: string;
  open: boolean;
  onOpenChange: (open: boolean) => void;
}): React.JSX.Element {
  const { t } = useTranslation();
  const unlock = useUnlockVolume();
  const [password, setPassword] = React.useState('');
  const [fvek, setFvek] = React.useState('');

  const onUnlock = (): void => {
    const candidates = password ? [password] : [];
    unlock.mutate(
      { volumeId, passwords: candidates },
      {
        onSuccess: () => {
          toast.success(t('unlock.success'));
          onOpenChange(false);
          setPassword('');
          setFvek('');
        },
        onError: (err) => {
          toast.error(err instanceof Error ? err.message : t('errors.unknown'));
        },
      },
    );
  };

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-sm">
        <DialogHeader>
          <DialogTitle className="flex items-center gap-2">
            <LockOpenIcon className="size-5 text-primary" aria-hidden />
            {t('unlock.title')}
          </DialogTitle>
          <DialogDescription>{t('unlock.body')}</DialogDescription>
        </DialogHeader>
        <div className="space-y-3">
          <label className="block space-y-1.5">
            <span className="text-[13px] font-medium">{t('unlock.passwordLabel')}</span>
            <Input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              placeholder={t('unlock.passwordPlaceholder')}
              autoComplete="off"
            />
          </label>
          <label className="block space-y-1.5">
            <span className="text-[13px] font-medium">{t('unlock.fvekLabel')}</span>
            <Input
              value={fvek}
              onChange={(e) => setFvek(e.target.value)}
              placeholder={t('unlock.fvekPlaceholder')}
              spellCheck={false}
              autoComplete="off"
            />
            <span className="block text-xs text-foreground-secondary">
              {t('unlock.fvekHint')}
            </span>
          </label>
        </div>
        <DialogFooter>
          <Button
            variant="secondary"
            onClick={() => onOpenChange(false)}
            disabled={unlock.isPending}
          >
            {t('common.close')}
          </Button>
          <Button onClick={onUnlock} disabled={unlock.isPending || (!password && !fvek)}>
            {unlock.isPending ? t('common.processing') : t('unlock.action')}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
