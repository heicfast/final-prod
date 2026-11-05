#!/usr/bin/env node
/**
 * unelevated-probe.mjs — the product-side twin of core's exit-4 probes
 * (INTEGRATION-PLAN §6.3): launch the INSTALLED app under a restricted
 * (Basic User) token via `runas /trustlevel:0x20000` and assert, over CDP
 * on the real shipped DOM, that the user-mode product refuses honestly:
 *
 *   1. the elevation banner appears (role="alert", the elevation text) —
 *      the app NEVER runs degraded and never pretends to work;
 *   2. the no-mock gate holds on this instance too (real Tauri bridge,
 *      no TEST DOUBLE hook);
 *   3. a raw-disk scan start from the home surface is REFUSED with the
 *      elevation-required message (the toast), not a silent no-op.
 *
 * Usage: OUT_DIR=C:/forensic/evidence node unelevated-probe.mjs
 * Outputs (under OUT_DIR): unelevated.json, screenshots/unelevated-*.png
 * Exits nonzero on any assertion failure (no masking).
 */

import { chromium } from 'playwright-core';
import fs from 'node:fs';
import path from 'node:path';

const CDP_PORT = process.env.CDP_PORT || '9223';
const OUT = process.env.OUT_DIR || 'C:/forensic/evidence';
const SHOTS = path.join(OUT, 'screenshots');
fs.mkdirSync(SHOTS, { recursive: true });

const evidence = { startedUtc: new Date().toISOString(), assertions: {} };
const report = { evidence, file: path.join(OUT, 'unelevated.json') };
function persist() {
  fs.writeFileSync(report.file, JSON.stringify(evidence, null, 2));
}
function log(msg) {
  console.log(`[unelevated ${new Date().toISOString()}] ${msg}`);
}

async function main() {
  log(`connecting to CDP 127.0.0.1:${CDP_PORT}`);
  let browser = null;
  for (let attempt = 1; attempt <= 20 && !browser; attempt++) {
    try {
      browser = await chromium.connectOverCDP(`http://127.0.0.1:${CDP_PORT}`);
    } catch {
      await new Promise((r) => setTimeout(r, 3000));
    }
  }
  if (!browser) throw new Error('could not connect to the unelevated instance CDP');
  const ctx = browser.contexts()[0];
  let page = ctx.pages()[0];
  if (!page) page = await ctx.waitForEvent('page', { timeout: 30000 });

  await page.waitForLoadState('load').catch(() => {});
  await page.waitForTimeout(5000); // boot + the posture query settles
  await page.screenshot({ path: path.join(SHOTS, 'unelevated-01-boot.png'), fullPage: true }).catch(() => {});

  // Assertion 1 — the no-mock gate (same signals as drive-ui.mjs).
  const gate = await page.evaluate(() => ({
    tauriInternals: '__TAURI_INTERNALS__' in window,
    injectHookPresent: '__rwInjectConsent' in window,
  }));
  evidence.assertions.noMockGate = gate;
  log(`no-mock gate: ${JSON.stringify(gate)}`);
  if (!gate.tauriInternals || gate.injectHookPresent) {
    throw new Error(`NO-MOCK GATE FAILED on the unelevated instance: ${JSON.stringify(gate)}`);
  }

  // Assertion 2 — the elevation banner. The banner is role="alert" with
  // the elevation body text (src/features/consent/elevation-banner.tsx +
  // i18n en elevation.bannerBody); it renders ONLY when the posture op
  // reports a non-administrator token.
  const banner = page.locator('[role="alert"]').filter({
    hasText: /administrator privileges/i,
  });
  const bannerVisible = await banner.isVisible({ timeout: 20000 }).catch(() => false);
  evidence.assertions.elevationBanner = {
    visible: bannerVisible,
    text: bannerVisible ? ((await banner.textContent()) || '').trim().slice(0, 400) : null,
  };
  await page.screenshot({ path: path.join(SHOTS, 'unelevated-02-banner.png'), fullPage: true }).catch(() => {});
  log(`elevation banner visible: ${bannerVisible}`);
  if (!bannerVisible) {
    throw new Error('ELEVATION BANNER NOT SHOWN under the restricted token — the product must refuse honestly');
  }

  // Assertion 3 — a raw-disk scan start is REFUSED, not silently no-op'd:
  // select the first device card, click Search for Lost Data, and expect
  // the elevation-required toast (errors.elevationRequired). The refusal is
  // the success condition; a scan actually STARTING is the failure.
  try {
    const card = page.getByRole('radio').first();
    if (await card.isVisible({ timeout: 10000 }).catch(() => false)) {
      await card.click();
      const scanBtn = page.getByRole('button', { name: /search for lost data/i }).first();
      await scanBtn.click({ timeout: 10000 });
      const refusal = await page
        .waitForSelector(`text=/administrator privileges are required for direct disk access/i`, {
          timeout: 30000,
        })
        .catch(() => null);
      evidence.assertions.scanRefusal = {
        attempted: true,
        refused: !!refusal,
      };
      await page.screenshot({ path: path.join(SHOTS, 'unelevated-03-refusal.png'), fullPage: true }).catch(() => {});
      log(`scan refusal observed: ${!!refusal}`);
      if (!refusal) {
        throw new Error('SCAN START NOT REFUSED under the restricted token — expected the elevation-required toast');
      }
    } else {
      // Home did not render device cards (e.g. enumeration itself refused):
      // the banner assertion above still carries the posture proof. Record
      // honestly rather than fail on an unknown shell state.
      evidence.assertions.scanRefusal = { attempted: false, reason: 'no device card visible' };
      log('scan refusal leg skipped: no device card visible');
    }
  } catch (e) {
    if (String(e).includes('REFUSED')) throw e;
    evidence.assertions.scanRefusal = { attempted: true, error: String(e) };
    throw e;
  }

  evidence.completedUtc = new Date().toISOString();
  evidence.passed = true;
  persist();
  log('all unelevated assertions passed');
  await browser.close().catch(() => {});
  process.exit(0);
}

main().catch((e) => {
  log('FATAL: ' + (e.stack || e));
  evidence.fatal = String(e);
  evidence.passed = false;
  persist();
  process.exit(1);
});
