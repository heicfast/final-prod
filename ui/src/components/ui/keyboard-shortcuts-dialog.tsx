import * as React from 'react';
import { KeyboardIcon } from 'lucide-react';
import { useTranslation } from 'react-i18next';
import { Dialog, DialogContent, DialogDescription, DialogHeader, DialogTitle } from './dialog';

/**
 * Discoverable keyboard map. The app deliberately keeps this short: these
 * are global shortcuts that work on every route and never collide with text
 * entry. Component-local controls retain their native keyboard semantics.
 */
export function KeyboardShortcutsDialog({
  open,
  onOpenChange,
}: {
  open: boolean;
  onOpenChange: (open: boolean) => void;
}): React.JSX.Element {
  const { t } = useTranslation();
  const shortcuts = [
    { key: '?', label: t('shortcuts.openHelp') },
    { key: 'Esc', label: t('shortcuts.dismiss') },
    { key: 'Tab', label: t('shortcuts.nextControl') },
    { key: 'Shift + Tab', label: t('shortcuts.previousControl') },
    { key: 'Enter / Space', label: t('shortcuts.activate') },
  ];

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-md">
        <DialogHeader>
          <div className="mb-1 flex size-11 items-center justify-center self-start rounded-md bg-primary-muted text-primary">
            <KeyboardIcon className="size-5" aria-hidden />
          </div>
          <DialogTitle>{t('shortcuts.title')}</DialogTitle>
          <DialogDescription>{t('shortcuts.description')}</DialogDescription>
        </DialogHeader>
        <dl className="divide-y divide-border rounded-md border border-border bg-surface">
          {shortcuts.map((shortcut) => (
            <div
              key={shortcut.key}
              className="flex items-center justify-between gap-4 px-3 py-2.5"
            >
              <dt className="text-sm text-foreground-secondary">{shortcut.label}</dt>
              <dd>
                <kbd className="rounded-xs border border-border-strong bg-surface-muted px-1.5 py-0.5 font-mono text-xs text-foreground">
                  {shortcut.key}
                </kbd>
              </dd>
            </div>
          ))}
        </dl>
      </DialogContent>
    </Dialog>
  );
}
