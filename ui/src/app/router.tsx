import * as React from 'react';
import { createHashRouter, Navigate, RouterProvider } from 'react-router';
import { WizardShell } from './shell';
import { ErrorBoundary } from './error-boundary';
import { HomePage } from '@/features/device-selection/home-page';
import { ScanConfigPage } from '@/features/scan-configuration/scan-config-page';
import { ScanProgressPage } from '@/features/scan-progress/scan-progress-page';
import { ResultsPageFallback } from '@/features/results-explorer/page-fallback';
import { RecoveryPage } from '@/features/recovery-queue/recovery-page';
import { HealthPage } from '@/features/health/health-page';
import { RepairPage } from '@/features/repair/repair-page';
import { SettingsPage } from '@/features/settings-about/settings-page';
import { ResultsRoute } from '@/features/results-explorer/results-route';

/**
 * Router (ADR-001): React Router v7 data mode with a HASH history — hash
 * routing behaves identically in WebView2's custom protocol, plain browser
 * dev/preview, and any static host. Guards: scan/results routes fall back to
 * Home when no session exists (see ResultsRoute).
 */
export const router = createHashRouter([
  {
    path: '/',
    element: (
      <ErrorBoundary>
        <WizardShell />
      </ErrorBoundary>
    ),
    children: [
      { index: true, element: <HomePage /> },
      { path: 'scan/:deviceId', element: <ScanConfigPage /> },
      { path: 'scan/:deviceId/live', element: <ScanProgressPage /> },
      { path: 'results/:scanId', element: <ResultsRoute /> },
      { path: 'recover/:scanId', element: <RecoveryPage /> },
      { path: 'health', element: <HealthPage /> },
      { path: 'repair', element: <RepairPage /> },
      { path: 'settings', element: <SettingsPage /> },
      { path: 'lost-partition', element: <Navigate to="/" replace /> },
      { path: '*', element: <ResultsPageFallback /> },
    ],
  },
]);

export function App(): React.JSX.Element {
  return <RouterProvider router={router} />;
}
