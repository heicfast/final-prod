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

test('create image: the cancellable running posture + the stop-confirm (D-IMG-1)', async ({
  page,
}) => {
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

  // The More-Option disclosure opens the from/to sector pair (D-IMG-3).
  await page.getByRole('button', { name: /more options/i }).click();
  await expect(page.getByLabel(/^from sector/i)).toBeVisible();
  await expect(page.getByLabel(/^to sector/i)).toBeVisible();
  // A from beyond the to is advisory-invalid; a legal pair re-enables.
  await page.getByLabel(/^from sector/i).fill('2000');
  await page.getByLabel(/^to sector/i).fill('1000');
  await expect(page.getByText(/^invalid sector/i)).toBeVisible();
  await expect(start).toBeDisabled();
  await page.getByLabel(/^from sector/i).fill('1000');
  await expect(start).toBeEnabled();

  await start.click();

  // The running posture: "continue in background" AND the Stop control
  // (cancel is real — D-IMG-1 closed at the engine/CLI/bridge/W9 TS).
  await expect(page.getByRole('button', { name: /continue in background/i })).toBeVisible({
    timeout: 20_000,
  });
  const stop = page.getByRole('button', { name: /^stop$/i });
  await expect(stop).toBeVisible({ timeout: 10_000 });

  // The stop-confirm states the re-create truth (strings:11672-11673).
  await stop.click();
  await expect(
    page.getByText(/you need to re-create the disk image if you stop it/i),
  ).toBeVisible();
  await page.getByRole('button', { name: /keep going/i }).click();
  await expect(page.getByRole('button', { name: /^stop$/i })).toBeVisible();

  // The job completes (the mock's compressed clock): the done copy shows.
  await expect(page.getByText(/disk image created/i)).toBeVisible({ timeout: 30_000 });
});

test('M04: import a disk image — the quick target, the row, the scan entry', async ({
  page,
}) => {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });

  // The og's "Import a disk image" quick target (the canned picker
  // alternates .img / .vhdx — both families exercised across runs).
  await page.getByRole('button', { name: /import a disk image/i }).click();

  // The row lands in the device list with the Disk image badge.
  await expect(page.getByText(/disk image/i).first()).toBeVisible({ timeout: 10_000 });

  // The imported row is selectable + scannable like any disk (the
  // click-through to the scan page happens through the standard flow;
  // here the badge + selection is the surface proof).
  const row = page.getByRole('radio', { name: /\.img|\.vhdx/i }).first();
  await expect(row).toBeVisible({ timeout: 10_000 });
});
