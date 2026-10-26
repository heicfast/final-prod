import { expect, test, type Page } from '@playwright/test';

/**
 * G-1 (a) — the pre-scan repair consent flow, end to end against the
 * labeled test double (the e2e build carries the double, never the real
 * connector). The Desktop quick-target is the double's damaged-volume
 * fixture: its pre-scan analysis finds a repairable plan, the engine's run
 * thread BLOCKS inside the consent callback (the stage walk is not
 * scheduled until the answer), and the scan page's non-dismissable prompt
 * is the only answer path. Approve = the transactional repair narrative;
 * deny = the scan stays read-only; both unblock. (Cancel's force-deny is
 * bridge machinery, covered by the bridge CI gates — not reproducible
 * against the double, whose cancel path mirrors it but has no engine.)
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

/** The consent dialog locator — unambiguous against the celebration modal. */
function consentDialog(page: Page) {
  return page.getByRole('dialog').filter({ hasText: /pre-scan repair request/i });
}

test('consent prompt blocks the scan, deny keeps it read-only, and the scan completes', async ({
  page,
}) => {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });

  // Start the damaged-volume fixture scan (Desktop quick-target).
  await page.getByRole('button', { name: /^desktop$/i }).click();

  // The non-dismissable consent prompt appears (the run thread is blocked
  // in the callback — no stage progress while it pends).
  const prompt = consentDialog(page);
  await expect(prompt).toBeVisible({ timeout: 15_000 });
  await expect(prompt.getByText(/file system: ntfs/i)).toBeVisible();
  await expect(prompt.getByText(/repair actions: 3/i)).toBeVisible();
  await expect(prompt.getByRole('button', { name: /approve repair/i })).toBeEnabled();
  await page.screenshot({ path: 'test-results/30-consent-prompt.png' });

  // Deny — the prompt dismisses (respondToConsent resolves the gate) and the
  // read-only scan proceeds through its stages to the completion modal.
  await prompt.getByRole('button', { name: /deny/i }).click();
  await expect(prompt).toBeHidden({ timeout: 10_000 });

  const modal = page.getByRole('dialog', { name: /search complete/i });
  await expect(modal).toBeVisible({ timeout: 30_000 });
  await page.screenshot({ path: 'test-results/31-consent-denied-complete.png' });

  expect(consoleErrors).toEqual([]);
});

test('consent re-hydrates from scan state after navigating away and back', async ({ page }) => {
  // The prompt's store primes from getScanState().pendingConsent — the
  // consent must survive the page unmounting (the event already fired, the
  // store's subscription is gone) and re-render when the scan page comes
  // back. This is the subscribe-late / navigate-away-and-back path the
  // bridge's scan-state re-hydration field exists for.
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });

  await page.getByRole('button', { name: /^desktop$/i }).click();
  const prompt = consentDialog(page);
  await expect(prompt).toBeVisible({ timeout: 15_000 });

  // Away (the store unsubscribes; the engine stays blocked) and back.
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /choose where to look/i })).toBeVisible({
    timeout: 15_000,
  });
  await page.goBack();
  await expect(consentDialog(page)).toBeVisible({ timeout: 15_000 });

  // Approve executes the transactional-repair narrative — the toast confirms
  // and the walk resumes (the tree is the same fixture either way).
  await consentDialog(page)
    .getByRole('button', { name: /approve repair/i })
    .click();
  await expect(consentDialog(page)).toBeHidden({ timeout: 10_000 });

  const modal = page.getByRole('dialog', { name: /search complete/i });
  await expect(modal).toBeVisible({ timeout: 30_000 });
  expect(consoleErrors).toEqual([]);
});
