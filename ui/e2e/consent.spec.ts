import { expect, test, type Page } from '@playwright/test';

/**
 * The consent gate e2e (plan §4.8, ported): the pre-scan repair consent
 * against the TEST DOUBLE — deny-completes, re-hydration after
 * navigate-away, and the non-dismissable rule (Escape never closes the
 * prompt; only a decision or a scan cancel does).
 *
 * The injection rides the mock-only window hook (connector/index.ts): the
 * REAL backend fires the consent from the engine's run thread, which a
 * browser test cannot reach by design — the UI flow under test is the
 * same either way (the shell modal + the echo-token respond).
 */

async function startScan(page: Page): Promise<void> {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });
  await page.getByRole('radio').first().click();
  const scanButton = page.getByRole('button', { name: /search for lost data/i }).first();
  await expect(scanButton).toBeEnabled();
  await scanButton.click();
  await expect(page.getByText(/quick scanning|advanced scanning/i).first()).toBeVisible({
    timeout: 20_000,
  });
}

async function injectConsent(page: Page): Promise<string | null> {
  return page.evaluate(() => {
    const hook = (window as unknown as { __rwInjectConsent?: () => string | null })
      .__rwInjectConsent;
    return hook ? hook() : null;
  });
}

test('consent: deny keeps the scan running and closes the prompt', async ({ page }) => {
  await startScan(page);

  const scanId = await injectConsent(page);
  expect(scanId, 'the injection found the running scan').toBeTruthy();

  const dialog = page.getByRole('dialog', { name: /confirm the repair/i });
  await expect(dialog).toBeVisible({ timeout: 10_000 });
  await page.screenshot({ path: 'test-results/consent-01-prompt.png' });

  // Non-dismissable: Escape does NOT close it (a non-answer is a user
  // decision — the engine thread waits).
  await page.keyboard.press('Escape');
  await expect(dialog).toBeVisible();

  // The plan rows render (2 write actions + 1 advisory from the injected plan).
  await expect(dialog.getByText(/2 write actions/i)).toBeVisible();
  await expect(dialog.getByText(/1 advisory/i)).toBeVisible();

  // Deny: the prompt resolves, the scan keeps running to completion.
  await dialog.getByRole('button', { name: /decline/i }).click();
  await expect(dialog).not.toBeVisible({ timeout: 5_000 });
  const modal = page.getByRole('dialog', { name: /search complete/i });
  await expect(modal).toBeVisible({ timeout: 90_000 });
  await page.screenshot({ path: 'test-results/consent-02-denied-completes.png' });
});

test('consent: re-hydration after navigating away and back', async ({ page }) => {
  await startScan(page);
  await injectConsent(page);

  const dialog = page.getByRole('dialog', { name: /confirm the repair/i });
  await expect(dialog).toBeVisible({ timeout: 10_000 });

  // Navigate away (the prompt is global — the shell keeps it), then to a
  // fresh route and back: the store still carries the pending request.
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible();
  // The store is in-memory SPA state: same tab, same store — the prompt
  // persists across client-side navigation (the modal is shell-mounted).
  await expect(dialog).toBeVisible();

  // Answer from the rehydrated prompt.
  await dialog.getByRole('button', { name: /approve/i }).click();
  await expect(dialog).not.toBeVisible({ timeout: 5_000 });
  const modal = page.getByRole('dialog', { name: /search complete/i });
  await expect(modal).toBeVisible({ timeout: 90_000 });
});
