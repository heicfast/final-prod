#!/usr/bin/env node
/**
 * drive-ui.mjs — forensic UI probe: drives the REAL installed product (the
 * shipped v0.7.0 build, launched with WebView2 CDP enabled) exactly as a
 * user would, capturing DOM state, screenshots, console errors, and timings
 * for every reported symptom:
 *
 *   S1  installer popup         (captured by install-probe.ps1, not here)
 *   S2  SMART: one drive / percentage-only / no attributes
 *   S3  scan: quick -> advanced, ~40% crawl, no results ever
 *   S4  other surfaces dead or fake
 *
 * No product code is modified; observation only, via CDP.
 *
 * Usage: OUT_DIR=C:/forensic/evidence node drive-ui.mjs
 * Outputs (under OUT_DIR):
 *   evidence.json          structured capture of every phase
 *   console.jsonl          every console message from the app
 *   pageerror.jsonl        every uncaught exception in the app
 *   screenshots/*.png      step screenshots
 */

import { chromium } from 'playwright-core';
import fs from 'node:fs';
import path from 'node:path';

const CDP_PORT = process.env.CDP_PORT || '9222';
const OUT = process.env.OUT_DIR || 'C:/forensic/evidence';
const SHOTS = path.join(OUT, 'screenshots');
fs.mkdirSync(SHOTS, { recursive: true });

const evidence = { startedUtc: new Date().toISOString(), phases: {} };
const consoleLog = fs.createWriteStream(path.join(OUT, 'console.jsonl'), { flags: 'a' });
const pageerrorLog = fs.createWriteStream(path.join(OUT, 'pageerror.jsonl'), { flags: 'a' });

function log(msg) {
  console.log(`[drive-ui ${new Date().toISOString()}] ${msg}`);
}
function shot(page, name) {
  const file = path.join(SHOTS, `${String(shot.n = (shot.n || 0) + 1).padStart(2, '0')}-${name}.png`);
  return page.screenshot({ path: file, fullPage: true }).catch(e => log(`screenshot ${name} failed: ${e.message}`))
    .then(() => file);
}

async function domSummary(page) {
  return page.evaluate(() => {
    const texts = (sel) => [...document.querySelectorAll(sel)]
      .map(e => (e.textContent || '').trim()).filter(Boolean).slice(0, 250);
    return {
      url: location.href,
      hash: location.hash,
      title: document.title,
      buttons: texts('button'),
      links: texts('a'),
      headings: texts('h1,h2,h3,h4'),
      tables: [...document.querySelectorAll('table')].map(t => ({
        rows: t.rows.length,
        header: t.rows[0] ? [...t.rows[0].cells].map(c => c.textContent.trim()) : [],
      })),
      treeitems: document.querySelectorAll('[role="treeitem"]').length,
      rows: document.querySelectorAll('table tr, [role="row"]').length,
      bodyText: (document.body.innerText || '').slice(0, 6000),
    };
  });
}

function extractScanState(text) {
  const pcts = [...text.matchAll(/(\d{1,3}(?:\.\d+)?)\s*%/g)].map(m => parseFloat(m[1]));
  const stage = /advanced|deep/i.test(text) ? (/quick/i.test(text) ? 'mixed' : 'advanced')
    : (/quick/i.test(text) ? 'quick' : null);
  const sectorMatches = [...text.matchAll(/([\d,.\s]+)\s*\/\s*([\d,.\s]+)\s*(sectors?|blocks?)/gi)].slice(0, 4);
  const foundCounts = [...text.matchAll(/(\d[\d,]*)\s*(?:files?|items?|found)/gi)].slice(0, 8).map(m => m[0]);
  const complete = /search complete|scan complete|congratulations/i.test(text);
  return { pcts, stage, sectorMatches: sectorMatches.map(m => m[0]), foundCounts, complete };
}

async function main() {
  log(`connecting to CDP 127.0.0.1:${CDP_PORT}`);
  let browser = null;
  for (let attempt = 1; attempt <= 10 && !browser; attempt++) {
    try {
      browser = await chromium.connectOverCDP(`http://127.0.0.1:${CDP_PORT}`);
    } catch (e) {
      log(`connect attempt ${attempt} failed: ${e.message.split('\n')[0]}`);
      await new Promise(r => setTimeout(r, 3000));
    }
  }
  if (!browser) throw new Error('could not connect to CDP after 10 attempts');
  const ctx = browser.contexts()[0];
  let page = ctx.pages()[0];
  if (!page) page = await ctx.waitForEvent('page', { timeout: 30000 });

  ctx.on('page', p => { log('new page opened: ' + p.url()); });
  page.on('console', m => {
    consoleLog.write(JSON.stringify({ t: new Date().toISOString(), type: m.type(), text: m.text().slice(0, 2000) }) + '\n');
  });
  page.on('pageerror', e => {
    pageerrorLog.write(JSON.stringify({ t: new Date().toISOString(), message: String(e).slice(0, 4000), stack: (e.stack || '').slice(0, 4000) }) + '\n');
    log('PAGEERROR: ' + String(e).slice(0, 300));
  });

  await page.waitForLoadState('load').catch(() => {});
  await page.waitForTimeout(4000); // app boot + device enumeration settles
  evidence.phases.boot = { summary: await domSummary(page).catch(e => ({ error: String(e) })) };
  await shot(page, 'boot-home');

  // The no-mock gate (the release's core honesty proof): the shipped build
  // must run the REAL engine bridge, not the TEST DOUBLE. Asserted on the
  // live shipped DOM — the app's own runtime signal (src/lib/runtime.ts)
  // plus the double's window hook (src/connector/index.ts) that must never
  // exist in a shipped build. A failure throws: the run is red, never masked.
  const backendGate = await page.evaluate(() => ({
    tauriInternals: '__TAURI_INTERNALS__' in window,
    injectHookPresent: '__rwInjectConsent' in window,
  }));
  evidence.phases.boot.backendGate = backendGate;
  log(`no-mock gate: ${JSON.stringify(backendGate)}`);
  if (!backendGate.tauriInternals || backendGate.injectHookPresent) {
    throw new Error(
      `NO-MOCK GATE FAILED: ${JSON.stringify(backendGate)} — the shipped build must carry the real Tauri bridge (internals injected) and no TEST DOUBLE hook`,
    );
  }

  // ---------------------------------------------------------------- S2: health
  try {
    log('S2 probe: navigating to #/health');
    await page.evaluate(() => { location.hash = '#/health'; });
    await page.waitForTimeout(3500);
    const healthBase = await domSummary(page);
    evidence.phases.health = { initial: healthBase, drives: [] };
    await shot(page, 'health-initial');

    // drive selector: click every element that looks like a drive (contains a
    // size unit) and capture the health surface for each selection. The fresh
    // wizard base renders drive cards as role=radio (run 40468386932 taught
    // this for S3; S2 carries the same lesson — button-only queries find
    // nothing on the v0.7.0 health surface).
    const candidates = await page.evaluate(() => {
      const els = [...document.querySelectorAll('button, [role="tab"], [role="button"], [role="radio"], li')];
      return els.map((e, i) => ({
        idx: i, text: (e.textContent || '').trim().slice(0, 120),
        hasSize: /GB|TB/i.test(e.textContent || ''),
      })).filter(c => c.hasSize);
    });
    log(`health: ${candidates.length} drive-selector candidates`);
    for (const c of candidates.slice(0, 8)) {
      try {
        const clicked = await page.evaluate((idx) => {
          const els = [...document.querySelectorAll('button, [role="tab"], [role="button"], li')];
          els[idx]?.click(); return !!els[idx];
        }, c.idx);
        if (!clicked) continue;
        await page.waitForTimeout(2500);
        const st = await page.evaluate(() => {
          const text = document.body.innerText || '';
          return {
            pcts: [...text.matchAll(/(\d{1,3}(?:\.\d+)?)\s*%/g)].map(m => parseFloat(m[1])),
            attrRows: document.querySelectorAll('table tbody tr, table tr').length,
            tables: [...document.querySelectorAll('table')].map(t => t.rows.length),
            statusWords: (text.match(/\b(Good|Caution|Poor|Bad|Unknown|Unavailable|Not available)\b/g) || []).slice(0, 10),
            hasTemp: /°C|Temperature/i.test(text),
            hasPowerOn: /Power[- ]?On|Hours/i.test(text),
            text: text.slice(0, 3500),
          };
        });
        evidence.phases.health.drives.push({ selectorText: c.text, ...st });
        await shot(page, `health-drive-${evidence.phases.health.drives.length}`);
      } catch (e) {
        evidence.phases.health.drives.push({ selectorText: c.text, error: String(e) });
      }
    }
    log(`S2 captured ${evidence.phases.health.drives.length} drive selections`);
  } catch (e) {
    evidence.phases.health = { error: String(e) };
    log('S2 FAILED: ' + e.message);
    await shot(page, 'health-failed').catch(() => {});
  }

  // ---------------------------------------------------------------- S3: scan
  try {
    log('S3 probe: back to home, select the 64GB seeded vdisk, start scan');
    await page.evaluate(() => { location.hash = '#/'; });
    await page.waitForTimeout(3000);
    const home = await domSummary(page);
    evidence.phases.scan = { home, timeline: [], result: null };
    await shot(page, 'scan-0-home');

    // select the seeded 64GB NTFS vdisk by its deterministic volume label
    // (run 40465852724: the home card renders 'SeedNTFS(X:)63.85 GB/64 GB';
    // the cards are role=radio, NOT role=button — run 40468386932's
    // getByRole('button') timeout taught this)
    const card = page.getByRole('radio', { name: /SeedNTFS/i }).first();
    await card.click({ timeout: 15000 });
    log('scan: clicked the SeedNTFS card');
    await page.waitForTimeout(1500);

    const scanBtn = page.getByRole('button', { name: /Search for Lost Data/i }).first();
    await scanBtn.click({ timeout: 15000 });
    log('scan: clicked Search for Lost Data');
    await page.waitForTimeout(2500);
    await shot(page, 'scan-1-started');

    // poll the scan for up to 28 minutes
    const t0 = Date.now();
    const CAP_MS = 28 * 60 * 1000;
    let lastShot = 0;
    let advancedSeenAt = null;
    // NG-21: the pre-scan repair consent gate. Run 40470191039 sat at
    // 'Quick Scanning 0%' for 28 minutes because the probe never answered
    // the (visible, verified-by-VLM) modal. Two behaviors now:
    //  (a) if the modal appears, the probe ANSWERS it (Deny - stay
    //      read-only, the conservative choice for a forensic run) and
    //      records the plan text verbatim;
    //  (b) on the HEALTHY seed disks the fixed engine must never offer a
    //      repair at all - consentSeen records which world we are in.
    evidence.phases.scan.consent = { seen: false, planText: null, answeredAtMs: null, deniedAtMs: null };
    const answerConsentIfPresent = async () => {
      // The dialog title differs across UI generations: the old wizard said
      // "Pre-scan repair request"; the v0.7.0 base says "Confirm the repair"
      // (i18n consent.repairTitle). Match BOTH — a missed match is the
      // 28-minute stall class (run 40470191039).
      const dlg = page.getByText(/Pre-scan repair request|Confirm the repair/i);
      if (await dlg.count().catch(() => 0)) {
        if (!evidence.phases.scan.consent.seen) {
          evidence.phases.scan.consent.seen = true;
          const bodyText = await page.evaluate(() => (document.body.innerText || '').slice(0, 5000)).catch(() => '');
          evidence.phases.scan.consent.planText = (bodyText.match(/(Pre-scan repair request|Confirm the repair)[\s\S]{0,900}/) || [null])[0];
          await shot(page, 'scan-consent-modal').catch(() => {});
          log('scan: consent modal APPEARED - answering Decline (stay read-only)');
        }
        // "Decline" is the v0.7.0 label (i18n consent.deny); "Deny"/"stay
        // read-only" are the older generations'.
        const deny = page.getByRole('button', { name: /Decline|Deny|stay read-only/i }).first();
        if (await deny.count().catch(() => 0)) {
          await deny.click({ timeout: 5000 }).catch(e => log('consent decline click failed: ' + e.message));
          evidence.phases.scan.consent.deniedAtMs = Date.now() - t0;
        }
      }
    };
    while (Date.now() - t0 < CAP_MS) {
      await answerConsentIfPresent();
      const st = await page.evaluate(() => ({
        url: location.href,
        text: (document.body.innerText || '').slice(0, 5000),
      })).catch(e => ({ url: 'EVAL-FAIL', text: String(e) }));
      const state = extractScanState(st.text || '');
      evidence.phases.scan.timeline.push({
        t: new Date().toISOString(), elapsedSec: Math.round((Date.now() - t0) / 1000),
        url: st.url, ...state,
      });
      if (state.stage === 'advanced' && !advancedSeenAt) {
        advancedSeenAt = Date.now() - t0;
        log(`scan: ADVANCED stage first seen at ${(advancedSeenAt / 1000).toFixed(0)}s`);
        await shot(page, 'scan-2-advanced-transition');
      }
      if (Date.now() - lastShot > 30000) {
        await shot(page, `scan-poll-${Math.round((Date.now() - t0) / 1000)}s`);
        lastShot = Date.now();
      }
      if (state.complete || /\/results\//.test(st.url)) {
        log('scan: COMPLETE detected');
        break;
      }
      await page.waitForTimeout(5000);
    }

    // capture the results surface (auto-navigation or via any visible affordance)
    await page.waitForTimeout(6000);
    const after = await domSummary(page);
    evidence.phases.scan.result = {
      finalUrl: after.url,
      treeitems: after.treeitems, rows: after.rows, tables: after.tables,
      bodyText: after.bodyText.slice(0, 4000),
    };
    await shot(page, 'scan-3-end-state');

    // if we are NOT on a results page, try to reach one directly
    if (!/\/results\//.test(after.url)) {
      const links = after.links.filter(l => /result|view/i.test(l));
      if (links.length) {
        await page.getByRole('link', { name: new RegExp(links[0].replace(/[.*+?^${}()|[\]\\]/g, '\\$&')) }).first().click().catch(() => {});
        await page.waitForTimeout(4000);
        const res = await domSummary(page);
        evidence.phases.scan.manualResultsNav = { clicked: links[0], url: res.url, treeitems: res.treeitems, rows: res.rows, bodyText: res.bodyText.slice(0, 3000) };
        await shot(page, 'scan-4-manual-results');
      }
    }

    // ---------------------------------------------------------------- S3b: results -> recover (+ the stop path when incomplete)
    // The deep scan of the 64GB seeded vdisk does not complete inside the
    // capture window on this runner class - and the PRE-MERGE engine showed
    // the identical stall at 76.1% (old run 42072191860, ABI v2: 43%->76.1%
    // over 28 min, 189 items; this run: 75%->76.1%, 197 items). Identical
    // behavior across two engines 1092 commits apart = environmental /
    // known-perf (the carve matcher over the sparse vdisk's free space; core's
    // PERF-LEVERS frontier), NOT a wiring regression. Recorded as a behavior
    // finding. Either way - completed (celebration modal) or stopped (the
    // real partial-results path) - the recover + session surfaces are then
    // driven on the real engine. Both are GATES: a failure fails the run.
    evidence.phases.recover = { viaStop: !/\/results\//.test(page.url()) };
    try {
      if (!/\/results\//.test(page.url())) {
        log('S3b: stopping the scan (the real partial-results path)');
        await page.getByRole('button', { name: 'Stop', exact: true }).first().click({ timeout: 15000 });
        const stopDlg = page.getByRole('dialog', { name: /Stop Searching for Lost Data\?/i });
        await stopDlg.waitFor({ state: 'visible', timeout: 10000 });
        await shot(page, 'recover-0-stop-dialog');
        await stopDlg.getByRole('button', { name: 'Stop', exact: true }).click({ timeout: 10000 });
        await page.waitForURL(/\/results\//, { timeout: 30000 });
        await page.waitForTimeout(4000);
        const partial = await page.getByRole('status').filter({ hasText: /stopped early/i }).count()
          .catch(() => 0);
        evidence.phases.recover.partialBanner = partial > 0;
        await shot(page, 'recover-1-partial-results');
      } else {
        // Completed inside the window: the celebration modal's own
        // 'Go to Recover' lands on the results surface.
        const goBtn = page.getByRole('button', { name: /go to recover/i }).first();
        if (await goBtn.count().catch(() => 0)) {
          await goBtn.click({ timeout: 10000 });
          await page.waitForURL(/\/results\//, { timeout: 30000 });
          await page.waitForTimeout(4000);
        }
        await shot(page, 'recover-1-complete-results');
      }

        // Save the session (the results explorer's save affordance).
        const save = page.getByRole('button', { name: /save this search/i }).first();
        if (await save.count().catch(() => 0)) {
          await save.click({ timeout: 10000 });
          await page.waitForTimeout(4000); // the save + the toast
          evidence.phases.recover.sessionSaved = true;
          await shot(page, 'recover-2-session-saved');
          log('S3b: session saved from the results explorer');
        } else {
          evidence.phases.recover.sessionSaved = false;
          log('S3b: save-this-search affordance not found');
        }

        // Select the first data row's checkbox, then Recover.
        const rows = page.getByRole('row');
        const rowCount = await rows.count().catch(() => 0);
        evidence.phases.recover.rowsInResults = rowCount;
        if (rowCount > 1) {
          await page.getByLabel(/select row/i).first().click({ timeout: 10000 });
          await page.waitForTimeout(1000);
          const recoverBtn = page.getByRole('button', { name: /recover \d+ items?/i }).first();
          await recoverBtn.click({ timeout: 15000 });
          await page.waitForURL(/\/recover\//, { timeout: 30000 });
          await page.waitForTimeout(3000);
          await shot(page, 'recover-3-destination-page');

          // Destination: a MOUNT row (Temp(D:) - not C:, not the source X:).
          // The native folder picker cannot be driven via CDP by design;
          // the mount list is the product's own in-app destination surface.
          const mount = page.getByRole('button', { name: /Temp\s*\(\s*D:/i }).first();
          if (await mount.count().catch(() => 0)) {
            await mount.click({ timeout: 10000 });
            await page.waitForTimeout(2500); // the F12 overlap validation
            evidence.phases.recover.destination = 'Temp(D:) mount row';
            await shot(page, 'recover-4-destination-chosen');
            const start = page.getByRole('button', { name: /recover \d+ items?/i }).first();
            await start.click({ timeout: 15000 });
            log('S3b: recovery started to the Temp(D:) mount');
            const report = await page
              .getByRole('status')
              .filter({ hasText: /recovery finished|recovery failed|issues/i })
              .first()
              .waitFor({ state: 'visible', timeout: 180000 })
              .catch(() => null);
            const reportText = report
              ? ((await page.getByRole('status').filter({ hasText: /recovery/i }).first().textContent()) || '')
              : null;
            evidence.phases.recover.reportText = (reportText || '').trim().slice(0, 600);
            await shot(page, 'recover-5-report');
            log(`S3b: recovery report: ${(reportText || 'NOT OBSERVED').slice(0, 120)}`);
            if (!report) {
              throw new Error('RECOVERY REPORT NOT OBSERVED within 180s of starting the recovery');
            }
          } else {
            evidence.phases.recover.destination = 'Temp(D:) mount row NOT FOUND';
            await shot(page, 'recover-3-no-mount-rows');
            throw new Error('the destination mount rows did not render (Temp(D:) not found)');
          }
        } else {
          evidence.phases.recover.error = `results grid has ${rowCount} rows - nothing to select`;
          throw new Error(evidence.phases.recover.error);
        }
      } catch (e) {
        (evidence.phases.recover ||= {}).error = String(e);
        log('S3b FAILED: ' + e.message);
        await shot(page, 'recover-failed').catch(() => {});
        throw new Error(`gate: S3b recover surface - ${e.message}`); // fails the run - no masking
      }

    // ---------------------------------------------------------------- S5: session save-resume
    // The saved session (S3b) must appear in the Backup/Scan Snapshot screen
    // with a working Resume affordance. A missing save affordance in S3b is
    // itself a gate failure (the surface must exist, not silently skip).
    if (!evidence.phases.recover?.sessionSaved) {
      throw new Error(
        'gate: S5 session save - the results explorer showed no "Save this search" affordance',
      );
    }
    {
      evidence.phases.sessionResume = {};
      try {
        log('S5: backup screen - the saved session + resume affordance');
        await page.evaluate(() => { location.hash = '#/backup'; });
        await page.waitForTimeout(4000);
        const s = await domSummary(page);
        evidence.phases.sessionResume.summary = {
          headings: s.headings, bodyText: s.bodyText.slice(0, 2500),
        };
        await shot(page, 'session-1-backup-list');
        const resumeBtn = page.getByRole('button', { name: /resume scan/i }).first();
        const resumeCount = await resumeBtn.count().catch(() => 0);
        evidence.phases.sessionResume.resumeAffordance = resumeCount > 0;
        if (resumeCount > 0) {
          await resumeBtn.click({ timeout: 10000 });
          // The resume either lands on a live scan (URL /scan/) or refuses
          // with a toast; both are recorded, the live-scan URL is the gate.
          await page.waitForTimeout(5000);
          evidence.phases.sessionResume.urlAfterResume = page.url();
          await shot(page, 'session-2-after-resume');
          if (!/\/scan\//.test(page.url())) {
            throw new Error(`RESUME did not navigate to a live scan (url: ${page.url()})`);
          }
          log('S5: resume navigated to a live scan - stopping it again (probe hygiene)');
          await page.getByRole('button', { name: 'Stop', exact: true }).first().click({ timeout: 15000 });
          const stopDlg = page.getByRole('dialog', { name: /Stop Searching for Lost Data\?/i });
          await stopDlg.waitFor({ state: 'visible', timeout: 10000 });
          await stopDlg.getByRole('button', { name: 'Stop', exact: true }).click({ timeout: 10000 });
          await page.waitForTimeout(3000);
        } else {
          throw new Error('the saved session shows no Resume affordance on the backup screen');
        }
      } catch (e) {
        evidence.phases.sessionResume.error = String(e);
        log('S5 FAILED: ' + e.message);
        await shot(page, 'session-failed').catch(() => {});
        throw new Error(`gate: S5 session save-resume - ${e.message}`); // fails the run - no masking
      }
    }
  } catch (e) {
    // S3 itself stays a RECORDER (the scan's speed/stall is a behavior
    // finding, not a gate) - but the S3b/S5 GATE failures rethrow past this
    // catch (marked gate:), because a failed recover/session surface must
    // fail the run, never ride green.
    if (String(e).includes('gate:')) throw e;
    (evidence.phases.scan ||= {}).error = String(e);
    log('S3 FAILED: ' + e.message);
    await shot(page, 'scan-failed').catch(() => {});
  }

  // ---------------------------------------------------------------- S4: other surfaces
  evidence.phases.surfaces = {};
  for (const [name, hash] of [['lost-partition', '#/lost-partition'], ['repair', '#/repair'], ['sd-card', '#/sd-card'], ['backup', '#/backup'], ['settings', '#/settings']]) {
    try {
      await page.evaluate(h => { location.hash = h; }, hash);
      await page.waitForTimeout(2500);
      const s = await domSummary(page);
      evidence.phases.surfaces[name] = s;
      await shot(page, `surface-${name}`);
      log(`S4 ${name}: ${s.buttons.length} buttons, headings=${JSON.stringify(s.headings.slice(0, 3))}`);
    } catch (e) {
      evidence.phases.surfaces[name] = { error: String(e) };
    }
  }

  evidence.completedUtc = new Date().toISOString();
  fs.writeFileSync(path.join(OUT, 'evidence.json'), JSON.stringify(evidence, null, 2));
  log('evidence written; done');
  await browser.close().catch(() => {});
  process.exit(0);
}

main().catch(e => {
  log('FATAL: ' + (e.stack || e));
  evidence.fatal = String(e);
  fs.writeFileSync(path.join(OUT, 'evidence.json'), JSON.stringify(evidence, null, 2));
  process.exit(1);
});
