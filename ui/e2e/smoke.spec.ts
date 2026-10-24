import { expect, test, type Page } from '@playwright/test';

/**
 * Golden-path UI smoke (grid #55): home → select device → scan config →
 * start scan → progress → results → filter → preview → select → recover →
 * report. Verifies state transitions and that no console errors or
 * unhandled rejections occur along the way.
 */

let consoleErrors: string[] = [];

function attachErrorCollector(page: Page): void {
  consoleErrors = [];
  page.on('console', (msg) => {
    if (msg.type() === 'error') consoleErrors.push(msg.text());
  });
  page.on('pageerror', (err) => consoleErrors.push(String(err)));
}

test.beforeEach(({ page }) => {
  attachErrorCollector(page);
});

test('golden path: scan → results → recover', async ({ page }) => {
  // 1. Home renders with device cards
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });
  await expect(page.getByText(/internal drives/i)).toBeVisible();

  // 2. Select the first device and open scan configuration
  await page.getByRole('radio').first().click();
  const scanButton = page.getByRole('button', { name: /search for lost data/i });
  await expect(scanButton).toBeEnabled();
  await scanButton.click();
  await expect(page.getByRole('heading', { name: /search setup/i })).toBeVisible();

  // 3. Start the scan (deep, defaults)
  await page.getByRole('button', { name: /^start searching$/i }).click();
  await expect(page.getByRole('status')).toBeVisible({ timeout: 20_000 });

  // 4. Wait for completion modal (mock quick/deep runs finish < 60s)
  const modal = page.getByRole('dialog', { name: /search complete/i });
  await expect(modal).toBeVisible({ timeout: 90_000 });
  await page.screenshot({ path: 'test-results/10-scan-complete.png' });

  // 5. Open results
  await modal.getByRole('button', { name: /open found files/i }).click();
  await expect(page.getByRole('grid')).toBeVisible({ timeout: 20_000 });
  await expect(page.getByRole('row').nth(1)).toBeVisible();
  await page.screenshot({ path: 'test-results/20-results.png' });

  // 6. Filter bar opens and filters
  await page.getByRole('button', { name: /^filter$/i }).click();
  await expect(page.getByRole('group', { name: /file type/i })).toBeVisible();
  await page.screenshot({ path: 'test-results/30-filter-open.png' });

  // 7. Search
  const search = page.getByLabel(/search by name/i);
  await search.fill('a');
  await search.press('Enter');
  await page.waitForTimeout(600);

  // 8. Select the first row (checkbox) — preview pane updates
  await page
    .getByLabel(/select row/i)
    .first()
    .click();
  await expect(page.getByText(/recovery condition/i)).toBeVisible();
  await page.screenshot({ path: 'test-results/40-preview.png' });

  // 9. Recover the selection
  await page.getByRole('button', { name: /recover 1 item/i }).click();
  await expect(page.getByRole('heading', { name: /recover files/i })).toBeVisible();

  // 10. Choose destination and start
  await page.getByRole('button', { name: /choose folder/i }).click();
  await page.getByRole('button', { name: /recover 1 item/i }).click();

  // 11. Report appears
  const report = page.getByRole('status').filter({ hasText: /recovery finished/i });
  await expect(report).toBeVisible({ timeout: 60_000 });
  await page.screenshot({ path: 'test-results/50-report.png' });

  // 12. No console errors along the whole path
  const benign = consoleErrors.filter(
    (e) => e.includes('Download the React DevTools') || e.includes('[vite]'),
  );
  expect(consoleErrors.filter((e) => !benign.includes(e))).toEqual([]);
});

test('state surfaces: empty scan route falls back to home prompt', async ({ page }) => {
  await page.goto('/#/results/scan-does-not-exist');
  await expect(page.getByText(/try again, or go back/i)).toBeVisible({ timeout: 15_000 });
});
