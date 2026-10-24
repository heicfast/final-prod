import { describe, expect, it } from 'vitest';
import { fireEvent, render, screen } from '@testing-library/react';
import { SplashScreen } from '@/features/splash/splash-screen';
import { ActivationDialog } from '@/features/settings-about/activation-dialog';
import { RecoveryGuideLauncher } from '@/features/device-selection/recovery-guide-dialog';
import '@/i18n';

describe('reference states 01–10', () => {
  it('renders distinct boot and device-enumeration splash states', () => {
    const { rerender } = render(<SplashScreen phase="booting" />);
    expect(screen.getByText(/preparing your recovery workspace/i)).toBeVisible();
    rerender(<SplashScreen phase="enumerating" />);
    expect(screen.getByText(/reading connected drives/i)).toBeVisible();
  });

  it('makes activation failure explicit instead of claiming a license was activated', async () => {
    render(<ActivationDialog open onOpenChange={() => undefined} />);
    fireEvent.change(screen.getByLabelText(/product key/i), {
      target: { value: 'ABCDE-ABCDE-ABCDE-ABCDE' },
    });
    fireEvent.click(screen.getByRole('button', { name: /^activate$/i }));
    expect(await screen.findByRole('alert')).toHaveTextContent(/no license was activated/i);
  });

  it('opens an assistant-style intent flyout that only offers real paths', async () => {
    render(<RecoveryGuideLauncher onChoose={() => undefined} />);
    fireEvent.click(screen.getByRole('button', { name: /help me choose/i }));
    expect(await screen.findByText(/what happened to your files/i)).toBeVisible();
    expect(screen.getByText(/i deleted files/i)).toBeVisible();
    expect(screen.getByText(/drive or partition is missing/i)).toBeVisible();
  });
});
