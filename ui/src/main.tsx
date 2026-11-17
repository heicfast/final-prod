import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import '@/styles/index.css';
import '@/i18n';
import { App } from './app/router';
import { Providers } from './app/providers';
import { DiskHealthWindowApp } from './features/health/disk-health-window';

/**
 * Boot: the shell runs as ONE webview per window with the same dist. The
 * main window renders the router (App); the SEPARATE disk-health window
 * (D-P4 reversal — the reference's DiskHealth.exe companion equivalent)
 * is created with `?window=disk-health&disk=<n>` and boots the standalone
 * surface directly, no router, no shell chrome: its window IS the app.
 * The query (not the hash) carries the window kind — the hash router
 * never sees it.
 */
function initialDiskIndex(): number {
  const raw = new URLSearchParams(window.location.search).get('disk');
  const n = raw === null ? Number.NaN : Number(raw);
  return Number.isInteger(n) && n >= 0 ? n : 0;
}

const isDiskHealthWindow =
  new URLSearchParams(window.location.search).get('window') === 'disk-health';

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Providers>
      {isDiskHealthWindow ? (
        <DiskHealthWindowApp initialDiskIndex={initialDiskIndex()} />
      ) : (
        <App />
      )}
    </Providers>
  </StrictMode>,
);
