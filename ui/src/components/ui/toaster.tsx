import { Toaster as Sonner } from 'sonner';
import { useTheme } from '@/app/theme';

/** Toast surface bound to the token themes. */
function Toaster(): React.JSX.Element {
  const { resolved } = useTheme();
  return (
    <Sonner
      theme={resolved}
      position="bottom-right"
      offset={16}
      gap={8}
      toastOptions={{
        classNames: {
          toast:
            '!rounded-md !border !border-border !bg-surface !text-foreground !shadow-overlay !text-sm',
          description: '!text-foreground-secondary',
          actionButton: '!bg-primary !text-primary-foreground !rounded-xs',
          cancelButton:
            '!bg-surface-muted !text-foreground-secondary !rounded-xs !border !border-border',
        },
      }}
    />
  );
}

export { Toaster };
