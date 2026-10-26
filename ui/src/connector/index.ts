/**
 * Connector factory — the import point for the ONE backend the UI has.
 *
 * Components and views import from src/queries — never from here directly
 * (enforced by eslint boundaries + scripts/arch-lint.mjs). The Tauri
 * connector talks to the cr_ui engine bridge (ui/src-tauri/src/commands/,
 * manifest: contracts/command-surface.json); the test-mode alias swaps this
 * module for the labeled test double under `tests/fakes/` (vitest runs and
 * `vite build --mode test` e2e builds — production builds can never reach
 * the double: no env branch exists here).
 */

import type { RecoveryConnector } from './contract';
import { createTauriConnector } from './tauri/tauri-connector';

export const connector: RecoveryConnector = createTauriConnector();
