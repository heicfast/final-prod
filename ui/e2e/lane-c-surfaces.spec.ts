import { expect, test, type Page } from '@playwright/test';

/**
 * Lane C surfaces e2e — the sector map (the AnalysisGridWidget parity),
 * the recoverability verdict (the completion grading), and the Create
 * Image flow's truthful no-cancel posture, all against the TEST DOUBLE
 * (the same UI flows the real bridge drives; the engine behavior itself
 * is the bridge-integration lane's cargo tests).
 */

async function startScanAndAwaitCompletion(page: Page): Promise<void> {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });
  await page.getByRole('radio').first().click();
  const scanButton = page.getByRole('button', { name: /search for lost data/i }).first();
  await expect(scanButton).toBeEnabled();
  scanButton.click();
  await expect(page.getByText(/quick scanning|advanced scanning/i).first()).toBeVisible({
    timeout: 20_000,
  });
  // The mock completes a standard scan in ~10-25 s (tick-compressed).
  await expect(page.getByText(/scan completed/i).first()).toBeVisible({ timeout: 90_000 });
}

test('sector map: opens during/after the scan and switches overlays', async ({ page }) => {
  await page.goto('/');
  await page.getByRole('radio').first().click();
  await page
    .getByRole('button', { name: /search for lost data/i })
    .first()
    .click();
  await expect(page.getByRole('button', { name: /sector map/i })).toBeVisible({
    timeout: 30_000,
  });

  // Collapsed by default: no tabs until opened.
  await expect(page.getByRole('tab', { name: /^coverage$/i })).toBeHidden();
  await page.getByRole('button', { name: /sector map/i }).click();
  await expect(page.getByRole('tab', { name: /^coverage$/i })).toBeVisible();
  await expect(page.getByRole('tab', { name: /^allocation$/i })).toBeVisible();

  // Overlay switch: the legend vocabulary changes with the tab.
  await page.getByRole('tab', { name: /^allocation$/i }).click();
  await expect(page.getByText('In use', { exact: true })).toBeVisible();
  await expect(page.getByText('Free', { exact: true })).toBeVisible();
});

test('recoverability: the completion celebration carries the engine verdict', async ({
  page,
}) => {
  await startScanAndAwaitCompletion(page);
  // The grading verdict + the matched/total line (the S-023 axes).
  await expect(page.getByText(/Your files are .* recoverable \(\d+%\)/)).toBeVisible({
    timeout: 20_000,
  });
  await expect(page.getByText(/graded files recoverable/i)).toBeVisible();
  await expect(page.getByText(/Images:/)).toBeVisible();
});

test('create image: the truthful no-cancel running posture', async ({ page }) => {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });

  // The per-card hover action: open the Create Image dialog.
  const action = page.getByRole('button', { name: /^create image$/i }).first();
  await action.click();
  await expect(page.getByRole('dialog')).toBeVisible();
  await expect(page.getByText(/^A byte-for-byte backup image/i)).toBeVisible();

  // The destination picker (the mock honors the source-excluding hint).
  await page.getByRole('button', { name: /browse/i }).click();
  const start = page.getByRole('button', { name: /^create image$/i }).first();
  await expect(start).toBeEnabled({ timeout: 10_000 });
  await start.click();

  // The running posture: the no-cancel note + "continue in background" —
  // never a stop button (the ABI exports no cancel).
  await expect(page.getByText(/runs to completion/i)).toBeVisible({ timeout: 10_000 });
  await expect(page.getByRole('button', { name: /continue in background/i })).toBeVisible({
    timeout: 20_000,
  });
  await expect(page.getByRole('button', { name: /^(stop|cancel)/i })).toBeHidden();

  // The job completes (the mock's compressed clock): the done copy shows.
  await expect(page.getByText(/disk image created/i)).toBeVisible({ timeout: 30_000 });
});
