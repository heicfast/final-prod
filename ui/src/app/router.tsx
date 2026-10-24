import * as React from 'react';
import { createHashRouter, RouterProvider } from 'react-router';
import { WizardShell } from './shell';
import { ErrorBoundary } from './error-boundary';
import { HomePage } from '@/features/device-selection/home-page';
import { ScanProgressPage } from '@/features/scan-progress/scan-progress-page';
import { ResultsPageFallback } from '@/features/results-explorer/page-fallback';
import { RecoveryPage } from '@/features/recovery-queue/recovery-page';
import { HealthPage } from '@/features/health/health-page';
import { RepairPage } from '@/features/repair/repair-page';
import { SettingsPage } from '@/features/settings-about/settings-page';
import { LostPartitionPage } from '@/features/lost-partition/lost-partition-page';
import { ResultsRoute } from '@/features/results-explorer/results-route';
import { StartupGate } from './startup-gate';
import { ToolsPage } from '@/features/tools/tools-page';
import { BackupPage } from '@/features/backup/backup-page';
import { SdCardPage } from '@/features/device-selection/sd-card-page';

/**
 * Router (ADR-001): React Router v7 data mode with a HASH history — hash
 * routing behaves identically in WebView2's custom protocol, plain browser
 * dev/preview, and any static host.
 *
 * Flow (ADR-008, corpus-verified): Home → scan starts DIRECTLY from device
 * selection — there is no configuration step between selection and the
 * scan (the reference's scanClickedSlot flips straight to the detail page
 * and starts scanning). Scan progress and results are the same surface at
 * different states (/scan/:id/live while running, /results/:scanId after).
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
      { path: 'scan/:deviceId/live', element: <ScanProgressPage /> },
      { path: 'results/:scanId', element: <ResultsRoute /> },
      { path: 'recover/:scanId', element: <RecoveryPage /> },
      { path: 'health', element: <HealthPage /> },
      { path: 'repair', element: <RepairPage /> },
      { path: 'settings', element: <SettingsPage /> },
      { path: 'lost-partition', element: <LostPartitionPage /> },
      { path: 'tools', element: <ToolsPage /> },
      { path: 'backup', element: <BackupPage /> },
      { path: 'sd-card', element: <SdCardPage /> },
      { path: '*', element: <ResultsPageFallback /> },
    ],
  },
]);

export function App(): React.JSX.Element {
  return (
    <StartupGate>
      <RouterProvider router={router} />
    </StartupGate>
  );
}
