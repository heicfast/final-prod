import { describe, expect, it, afterEach } from 'vitest';
import { render, screen, cleanup, waitFor } from '@testing-library/react';
import { MemoryRouter } from 'react-router';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { TooltipProvider } from '@/components/ui/tooltip';
import { ThemeProvider } from '@/app/theme';
import { ResultsExplorer } from '@/features/results-explorer/results-explorer';
import { connector } from '@/connector';
import '@/i18n';

/**
 * NG-22 (forensic run 40470191039): the results tree's "existing files"
 * root label must be the SCAN TARGET's real name. The pre-fix code
 * hard-coded 'Windows (C:)' — a scan of SeedNTFS(X:) rendered a root
 * labeled "Windows (C:)" (captured in the run-6 evidence screenshot).
 * The explorer now passes scan state's target label into the tree; the
 * neutral translated string is only the fallback for unresolved targets.
 */

function wrap(ui: React.ReactElement): ReturnType<typeof render> {
  const qc = new QueryClient({
    defaultOptions: { queries: { retry: false, staleTime: Infinity } },
  });
  return render(
    <ThemeProvider>
      <QueryClientProvider client={qc}>
        <TooltipProvider>
          <MemoryRouter>{ui}</MemoryRouter>
        </TooltipProvider>
      </QueryClientProvider>
    </ThemeProvider>,
  );
}

afterEach(() => {
  cleanup();
});

describe('results tree root label (NG-22)', () => {
  it("labels the existing-files root with the scan target's real name", async () => {
    const { scanId } = await connector.scan.startScan({ targetId: 'vol-C' });
    // let the fixture walk land (the fake completes the scan on a timer)
    await waitFor(
      async () => {
        const st = await connector.scan.getScanState(scanId);
        expect(st.status).toBe('completed');
      },
      { timeout: 4000 },
    );

    wrap(<ResultsExplorer scanId={scanId} />);

    // the fake's scan target is VOLUME_C, label 'Local Disk (C:)' — the
    // root must carry THAT label (from scan state), never the old fake
    await waitFor(() => {
      expect(screen.getByText('Local Disk (C:)')).toBeVisible();
    });
    expect(screen.queryByText('Windows (C:)')).toBeNull();
  });
});
