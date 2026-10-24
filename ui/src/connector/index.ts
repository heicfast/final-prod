/**
 * Connector factory — the ONLY place that decides which backend the UI talks to.
 *
 * VITE_CONNECTOR=mock (default) → deterministic simulator
 * VITE_CONNECTOR=tauri          → real IPC adapter (currently a loud stub)
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
  return v === 'tauri' ? 'tauri' : 'mock';
}

function createConnector(): RecoveryConnector {
  return connectorMode() === 'tauri' ? createTauriConnector() : createMockConnector();
}

function createMockConnector(): RecoveryConnector {
  return new MockConnector();
}

export const connector: RecoveryConnector = createConnector();

/**
 * Dev-panel hook: scenario control when the backend is the simulator.
 * Returns null on the real connector (the panel hides these controls).
 */
export function scenarioControl(): ScenarioControl | null {
  if (connectorMode() !== 'mock') return null;
  const mock = connector as unknown as MockConnector;
  return {
    setScenario: (scenario: MockScenarioId) => mock.setScenario(scenario),
    getScenario: () => mock.getScenario(),
    setSeed: (seed: number) => mock.setSeed(seed),
    reset: () => mock.reset(),
  };
}
