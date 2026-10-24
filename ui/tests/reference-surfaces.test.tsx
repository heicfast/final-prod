import { describe, expect, it, vi } from 'vitest';
import { fireEvent, render, screen } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { ToolsPage } from '@/features/tools/tools-page';
import { RecoveryGuideDialog } from '@/features/device-selection/recovery-guide-dialog';
import '@/i18n';

describe('reference-derived entry surfaces', () => {
  it('renders real tools and explicit provider requirements for the full reference grid', async () => {
    render(
      <MemoryRouter>
        <ToolsPage />
      </MemoryRouter>,
    );
    expect(screen.getByRole('heading', { name: /more recovery tools/i })).toBeVisible();
    expect(screen.getAllByRole('button', { name: /open tool/i })).toHaveLength(3);
    fireEvent.click(screen.getAllByRole('button', { name: /view requirements/i })[0]);
    expect(
      await screen.findByRole('dialog', { name: /additional recovery provider required/i }),
    ).toBeVisible();
  });

  it('turns a guided choice into one explicit next action', async () => {
    const onStart = vi.fn();
    render(
      <RecoveryGuideDialog
        open
        path="deleted"
        onOpenChange={() => undefined}
        onStart={onStart}
      />,
    );
    fireEvent.click(screen.getByRole('button', { name: /continue/i }));
    fireEvent.click(screen.getByRole('button', { name: /continue/i }));
    expect(await screen.findByText(/targeted search of the recycle bin/i)).toBeVisible();
    fireEvent.click(screen.getByRole('button', { name: /start this path/i }));
    expect(onStart).toHaveBeenCalledWith('deleted');
  });
});
