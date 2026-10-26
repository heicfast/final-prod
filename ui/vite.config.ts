/// <reference types="vitest/config" />
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import tailwindcss from '@tailwindcss/vite';
import { fileURLToPath, URL } from 'node:url';

const host = process.env.TAURI_DEV_HOST;

export default defineConfig(({ mode }) => {
  // Test-mode connector alias (plan §5.5): vitest runs and `vite build
  // --mode test` (the e2e/visual builds) swap the factory module for the
  // labeled test double. EXACT-MATCH regex so `@/connector/contract` and
  // sibling imports keep resolving to the real source. Production builds
  // (mode 'production' / 'development') can never reach the double — no env
  // branch exists in src/.
  const testMode = process.env.VITEST !== undefined || mode === 'test';
  const connectorAlias = testMode
    ? [
        {
          find: /^@\/connector$/,
          replacement: fileURLToPath(
            new URL('./tests/fakes/test-connector.ts', import.meta.url),
          ),
        },
      ]
    : [];

  return {
    plugins: [
      // React Compiler via the Rust port (oxc-transform-react) — plugin-react v6.
      react({ compiler: true }),
      tailwindcss(),
    ],

    // Vite options tailored for Tauri development and only applied in `tauri dev` or `tauri build`
    //
    // 1. prevent Vite from obscuring Rust errors
    clearScreen: false,
    // 2. Tauri expects a fixed port, fail if that port is not available
    server: {
      port: Number(process.env.PORT ?? (process.env.DISABLE_HMR === 'true' ? 3000 : 1420)),
      strictPort: true,
      host: host ?? '0.0.0.0',
      hmr: host
        ? {
            protocol: 'ws',
            host,
            port: 1421,
          }
        : undefined,
      watch: {
        // 3. tell Vite to ignore watching `src-tauri`
        ignored: ['**/src-tauri/**'],
      },
    },
    resolve: {
      alias: [
        // The connector alias MUST come before the bare '@' prefix rule.
        ...connectorAlias,
        {
          find: '@',
          replacement: fileURLToPath(new URL('./src', import.meta.url)),
        },
      ],
    },
    build: {
      target: 'chrome110',
      minify: true,
      sourcemap: false,
      rollupOptions: {
        output: {
          manualChunks(id: string) {
            if (id.includes('i18next')) return 'vendor-i18n';
            if (id.includes('@tanstack') || id.includes('zustand')) return 'vendor-query';
            if (id.includes('react') || id.includes('scheduler')) return 'vendor-react';
            return undefined;
          },
        },
      },
    },
    test: {
      globals: true,
      environment: 'jsdom',
      setupFiles: ['./tests/setup.ts'],
      include: ['tests/**/*.test.{ts,tsx}'],
      css: false,
      coverage: {
        reporter: ['text', 'json-summary', 'html'],
        include: [
          'src/connector/**/*.{ts,tsx}',
          'src/stores/**/*.{ts,tsx}',
          'src/lib/**/*.{ts,tsx}',
          'src/theme/**/*.{ts,tsx}',
          'src/app/shell.tsx',
          'src/app/theme.tsx',
          'src/features/device-selection/**/*.{ts,tsx}',
          'src/features/settings-about/about-dialog.tsx',
          'src/components/ui/{badge,button,checkbox,confidence-badge,dialog,empty-state,progress,radio-group,stepper,toaster,tooltip,window-controls}.tsx',
        ],
        exclude: ['src/**/*.d.ts'],
        thresholds: {
          lines: 70,
          functions: 70,
          statements: 70,
          branches: 60,
        },
      },
    },
  };
});
