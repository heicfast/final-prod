/**
 * Connector factory — the ONLY place that decides which backend the UI talks to.
 *
 * VITE_CONNECTOR=tauri (default) → the REAL cr_ui engine bridge (the shipped
 *                                 product's only backend)
 * VITE_CONNECTOR=mock            → the deterministic simulator (TEST DOUBLE —
 *                                 dev browser mode, vitest, e2e builds)
 *
 * The default flipped with the P4 bridge (plan §4.8): a shipped build must
 * never carry the simulator. The mock stays pinned for `pnpm dev` (browser
 * dev without the shell), vitest (vite.config test.env) and the e2e lane.
 *
 * Components and views import from src/queries — never from here directly
 * (enforced by eslint boundaries + scripts/arch-lint.mjs).
 */

import type { MockScenarioId, RecoveryConnector, ScenarioControl } from './contract';
import { MockConnector } from './mock/mock-connector';
import { createTauriConnector } from './tauri/tauri-connector';

export type ConnectorMode = 'mock' | 'tauri';

export function connectorMode(): ConnectorMode {
  const v = (import.meta.env as { readonly VITE_CONNECTOR?: unknown }).VITE_CONNECTOR;
  return v === 'mock' ? 'mock' : 'tauri';
}

/**
 * Build the backend pair once: the connector the UI talks to, plus the
 * concrete simulator instance when the backend IS the simulator (the dev
 * panel's scenario controls need it; the real connector returns null).
 */
function createBackend(): { connector: RecoveryConnector; mock: MockConnector | null } {
  if (connectorMode() === 'tauri') return { connector: createTauriConnector(), mock: null };
  const mock = new MockConnector();
  return { connector: mock, mock };
}

const backend = createBackend();

export const connector: RecoveryConnector = backend.connector;

// E2E TEST HOOK (the consent spec, plan §4.8): when the TEST DOUBLE backend
// is active, expose the consent injector on the window so Playwright can
// drive the pre-scan repair prompt (the real backend fires it from the
// engine's run thread — unreachable from a browser test by design). The
// hook resolves the newest RUNNING scan and injects the plan; it returns
// the scan id (or null when nothing is running). NEVER present in shipped
// builds (the tauri backend's mock is null).
if (backend.mock !== null && typeof window !== 'undefined') {
  const mock = backend.mock;
  (window as { __rwInjectConsent?: () => string | null }).__rwInjectConsent = () => {
    const scanId = mock.lastRunningScanId();
    if (scanId === null) return null;
    void mock.injectConsentRequest(scanId, {
      detected: 1,
      fsFamily: 'ntfs',
      actionCount: 2,
      refused: 0,
      reason: 'boot sector backup mismatch (test plan)',
      advisoryCount: 1,
    });
    return scanId;
  };
}

/**
 * Dev-panel hook: scenario control when the backend is the simulator.
 * Returns null on the real connector (the panel hides these controls).
 */
export function scenarioControl(): ScenarioControl | null {
  const mock = backend.mock;
  if (!mock) return null;
  return {
    setScenario: (scenario: MockScenarioId) => mock.setScenario(scenario),
    getScenario: () => mock.getScenario(),
    setSeed: (seed: number) => mock.setSeed(seed),
    reset: () => mock.reset(),
  };
}
