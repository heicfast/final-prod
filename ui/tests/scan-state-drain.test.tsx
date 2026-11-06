import { describe, expect, it, afterEach, vi } from 'vitest';
import { render, screen, cleanup } from '@testing-library/react';
import { QueryClient, QueryClientProvider } from '@tanstack/react-query';
import { useScanState } from '@/queries/scan';
import { connector } from '@/connector';
import type { ScanState } from '@/connector/contract';
import '@/i18n';

/**
 * The v0.8.2 regression pin (forensic runs 42176600928 + 42178485694 on
 * the installed release): the scanState query MUST keep polling while the
 * engine drains a stop (status 'stopping' — the run thread observes the
 * cancel token only at its next checkpoint). The refetchInterval used to
 * turn off on every status but running/starting, so the query froze at
 * 'stopping' FOREVER: the "stopped early" partial-results banner (gated
 * on status === 'cancelled') could never render after a real stop, even
 * though the engine thread had long returned and the session save already
 * worked. The TEST DOUBLE's synchronous cancel (mock-connector.ts:482 sets
 * 'cancelled' immediately — no stopping phase) is why every unit/e2e proof
 * stayed green while the shipped product never showed the banner.
 */

function makeState(status: ScanState['status']): ScanState {
  return {
    scanId: 'scan-1',
    params: { targetId: 'dev-1' },
    target: { id: 'dev-1', kind: 'internal', label: 'Seed (X:)', status: 'ok' },
    status,
    startedAt: '2026-11-06T07:06:36Z',
    completedAt: status === 'cancelled' ? '2026-11-06T07:07:05Z' : null,
    progress: {
      scanId: 'scan-1',
      status,
      stage: 'deep-scan',
      stageProgress: 0.5,
      overallProgress: 0.756,
      bytesScanned: 4_000_000_000,
      bytesTotal: 8_000_000_000,
      filesFound: 69,
      dataFoundBytes: 137_000_000,
      errors: 0,
    },
    pendingConsent: null,
    lostCandidates: { count: 0, rows: [] },
    autosaved: false,
  } as unknown as ScanState;
}

function StatusProbe({ scanId }: { scanId: string }) {
  const state = useScanState(scanId);
  return <div data-testid="scan-status">{state.data?.status ?? 'loading'}</div>;
}

describe('useScanState polls through the stopping phase (the stopped-scan banner regression)', () => {
  it('keeps refetching while stopping and lands the terminal cancelled state', async () => {
    // The real engine's sequence on a stop: 'stopping' for the whole drain
    // window, then the terminal 'cancelled' once run() returns.
    const script: ScanState['status'][] = ['stopping', 'stopping', 'cancelled'];
    let calls = 0;
    const spy = vi.spyOn(connector.scan, 'getScanState').mockImplementation(() => {
      const status = script[Math.min(calls, script.length - 1)];
      calls += 1;
      return Promise.resolve(makeState(status));
    });

    const qc = new QueryClient({
      defaultOptions: { queries: { retry: false, staleTime: 0 } },
    });
    render(
      <QueryClientProvider client={qc}>
        <StatusProbe scanId="scan-1" />
      </QueryClientProvider>,
    );

    // The drain window: the first poll says 'stopping'. With the v0.8.1
    // interval this is where polling died and the test would hang at
    // 'stopping' until timeout; with the fix the 500 ms poll continues.
    expect(await screen.findByText('stopping')).toBeDefined();

    await screen.findByText('cancelled', {}, { timeout: 5000 });
    // At least the three scripted fetches happened (the poll survived the
    // stopping phase instead of parking after the first one).
    expect(calls).toBeGreaterThanOrEqual(3);
    expect(spy).toHaveBeenCalledTimes(calls);
    spy.mockRestore();
  });

  afterEach(() => {
    cleanup();
    vi.restoreAllMocks();
  });
});
