import { describe, expect, it } from 'vitest';
import { render, screen } from '@testing-library/react';
import { SplashScreen } from '@/features/splash/splash-screen';
import { KeyboardShortcutsDialog } from '@/components/ui/keyboard-shortcuts-dialog';
import '@/i18n';

describe('first-run and keyboard discoverability', () => {
  it('announces the real device-enumeration state instead of presenting a blank frame', () => {
    render(<SplashScreen phase="enumerating" />);
    expect(screen.getByRole('main')).toHaveAttribute('aria-busy', 'true');
    expect(screen.getByText(/reading connected drives/i)).toBeVisible();
    expect(screen.getByText(/never changed during a search/i)).toBeVisible();
  });

  it('publishes a compact keyboard map with native key semantics', async () => {
    render(<KeyboardShortcutsDialog open onOpenChange={() => undefined} />);
    expect(await screen.findByRole('dialog', { name: /keyboard shortcuts/i })).toBeVisible();
    expect(screen.getByText('Shift + Tab')).toBeVisible();
    expect(screen.getByText('Enter / Space')).toBeVisible();
  });
});
