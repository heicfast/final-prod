import { describe, expect, it, afterEach, vi } from 'vitest';
import { cleanup, fireEvent, render, screen, waitFor } from '@testing-library/react';
import { MemoryRouter, Routes, Route } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { ThemeProvider } from '@/app/theme';
import { TooltipProvider } from '@/components/ui/tooltip';
import { BackupPage } from '@/features/backup/backup-page';
import { connector } from '@/connector';
import type { SessionMeta } from '@/connector/contract';
import '@/i18n';

/**
 * The v0.8.2 regression pin (forensic run 42185123973 on the installed
 * release): the Backup screen's Resume navigated to `/scan?scanId=<id>` —
 * a route the router does NOT define (the only scan route is
 * `scan/:deviceId/live`), so the catch-all fallback swallowed the
 * navigation and the resumed scan ran invisibly. The resume result now
 * carries the targetId and the page navigates the real live-scan route.
 */

const sessions: SessionMeta[] = [
  {
    id: 'sess-1',
    fileName: 'auto_SeedEXFAT_11_06_08_06_45.crsf',
    savedAt: '2026-11-06T08:06:45Z',
    targetLabel: '(no volume label)',
    filesFound: 68,
    dataFoundBytes: 137_000_000,
    completed: false,
    path: 'C:\\ProgramData\\HeicFast\\Recovery Wizard\\sessions\\auto_SeedEXFAT.crsf',
  },
];

function LiveScanMarker(): React.JSX.Element {
  return <div data-testid="live-scan-route">live scan route reached</div>;
}

function FallbackMarker(): React.JSX.Element {
  return <div data-testid="fallback-route">fallback route reached</div>;
}

function renderBackup(): ReturnType<typeof render> {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false, staleTime: Infinity } },
  });
  qc.setQueryData(['sessions'], sessions);
  return render(
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>
          <MemoryRouter initialEntries={['/backup']}>
            <Routes>
              <Route path="/backup" element={<BackupPage />} />
              <Route path="/scan/:deviceId/live" element={<LiveScanMarker />} />
              <Route path="*" element={<FallbackMarker />} />
            </Routes>
          </MemoryRouter>
        </TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>,
  );
}

describe('the Backup screen resume navigation (the invisible-resume regression)', () => {
  it('navigates the live-scan route (/scan/:targetId/live), not the fallback', async () => {
    const resumeSpy = vi
      .spyOn(connector.sessions, 'resumeSession')
      .mockResolvedValue({ scanId: 'scan-9', targetId: 'vol-scan-9' });

    renderBackup();

    const resumeBtn = await screen.findAllByRole('button', { name: /resume scan/i });
    fireEvent.click(resumeBtn[0]);

    await waitFor(() => {
      expect(screen.getByTestId('live-scan-route')).toBeVisible();
    });
    expect(screen.queryByTestId('fallback-route')).toBeNull();
    expect(resumeSpy).toHaveBeenCalledWith(sessions[0].path);
    resumeSpy.mockRestore();
  });

  afterEach(() => {
    cleanup();
    vi.restoreAllMocks();
  });
});
