import { defineConfig, devices } from '@playwright/test';

/**
 * UI smoke — runs against the built frontend served by `vite preview`.
 * The build is made in TEST MODE (`vite build --mode test`) so the bundle
 * carries the labeled test double (tests/fakes/ - plan §5.5); production
 * builds resolve the real cr_ui bridge connector and never reach the double
 * (verified in ui-ci's bundle assertions). The Tauri shell + the real
 * engine are exercised by the Rust lanes and the release workflow.
 */
export default defineConfig({
  testDir: './e2e',
  timeout: 120_000,
  fullyParallel: false,
  retries: process.env.CI ? 1 : 0,
  workers: 1,
  reporter: process.env.CI ? [['html', { open: 'never' }], ['list']] : [['list']],
  use: {
    baseURL: process.env.SMOKE_BASE_URL ?? 'http://localhost:4173',
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure',
    video: 'off',
    ...devices['Desktop Chrome'],
    viewport: { width: 1280, height: 832 },
  },
  outputDir: './test-results',
  webServer: process.env.SMOKE_BASE_URL
    ? undefined
    : {
        command: 'pnpm build --mode test && pnpm preview --port 4173 --strictPort',
        url: 'http://localhost:4173',
        reuseExistingServer: !process.env.CI,
        timeout: 60_000,
      },
});
