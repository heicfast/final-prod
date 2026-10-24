import * as React from 'react';
import { KeyRoundIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from '@/components/ui/dialog';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';

/**
 * License entry is intentionally honest: this UI accepts a product-key shape
 * but never claims activation without a configured licensing provider.
 */
export function ActivationDialog({
  open,
  onOpenChange,
}: {
  open: boolean;
  onOpenChange: (open: boolean) => void;
}): React.JSX.Element {
  const { t } = useTranslation();
  const [key, setKey] = React.useState('');
  const [submitted, setSubmitted] = React.useState(false);
  const validShape = /^[A-Z0-9]{5}(?:-[A-Z0-9]{5}){3}$/i.test(key.trim());

  const handleOpenChange = (next: boolean): void => {
    if (!next) {
      setKey('');
      setSubmitted(false);
    }
    onOpenChange(next);
  };

  return (
    <Dialog open={open} onOpenChange={handleOpenChange}>
      <DialogContent className="max-w-lg">
        <DialogHeader className="items-center text-center">
          <div className="mb-2 flex size-14 items-center justify-center rounded-lg bg-primary text-primary-foreground shadow-overlay">
            <KeyRoundIcon className="size-7" aria-hidden />
          </div>
          <DialogTitle>{t('activation.title')}</DialogTitle>
          <DialogDescription>{t('activation.description')}</DialogDescription>
        </DialogHeader>
        <div className="space-y-2">
          <label
            htmlFor="activation-key"
            className="text-xs font-semibold text-foreground-secondary"
          >
            {t('activation.keyLabel')}
          </label>
          <div className="flex gap-2">
            <Input
              id="activation-key"
              autoFocus
              value={key}
              onChange={(event) => setKey(event.target.value.toUpperCase())}
              placeholder={t('activation.placeholder')}
              aria-describedby="activation-note activation-error"
            />
            <Button variant="primary" disabled={!validShape} onClick={() => setSubmitted(true)}>
              {t('activation.activate')}
            </Button>
          </div>
          <p id="activation-note" className="text-xs text-foreground-secondary">
            {t('activation.note')}
          </p>
          {submitted ? (
            <p
              id="activation-error"
              role="alert"
              className="rounded-sm bg-warning-muted px-3 py-2 text-xs text-warning"
            >
              {t('activation.providerUnavailable')}
            </p>
          ) : null}
        </div>
        <DialogFooter>
          <Button variant="secondary" onClick={() => handleOpenChange(false)}>
            {t('common.close')}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}
