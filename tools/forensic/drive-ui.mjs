#!/usr/bin/env node
/**
 * drive-ui.mjs — forensic UI probe: drives the REAL installed product (the
 * shipped release build, launched with WebView2 CDP enabled) exactly as a
 * user would, capturing DOM state, screenshots, console errors, and timings
 * for every reported symptom:
 *
 *   S1   installer popup        (captured by install-probe.ps1, not here)
 *   S2   SMART: one drive / percentage-only / no attributes
 *   S2b  the save/resume vehicle: the 8GB SeedEXFAT scan stopped
 *        mid-deep (healthy), drained, saved as an INCOMPLETE session,
 *        resumed from the Backup screen, and run to completion — the
 *        deterministic carrier for the session gates (run 42148493806:
 *        the 64GB stall's stopped scan stays busy for an unbounded
 *        drain — BF-25; run 42170466748: a completed session's row has
 *        no Resume affordance, only Open)
 *   S3   scan: quick -> advanced, the reported ~76% stall (recorder)
 *   S3b  stop -> partial results -> recover -> report (gates)
 *   S4   other surfaces dead or fake
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

  // ------------------------------------------------- S2b: the save/resume vehicle
  // The save/resume gates need a NON-BUSY scan, and the RESUME affordance
  // needs an INCOMPLETE session (the Backup screen renders 'Resume scan'
  // only for !completed rows — run 42170466748: a completed session's row
  // carries only 'Open'). The vehicle: start the 8GB SeedEXFAT scan, stop
  // it mid-deep (a HEALTHY running scan — the 64GB leg below is the stall
  // recorder whose stopped scan never drains, BF-25), wait for the drain
  // (the 'stopped early' banner latches when the engine's run thread
  // returns), save the incomplete session, resume it from the Backup
  // screen, and let the resumed scan run to completion. Gates: the drain
  // banner, the save toast, the Resume affordance, the live-scan URL after
  // resume, and the resumed scan's completion. A failure fails the run.
  try {
    evidence.phases.saveResume = { scan: { timeline: [], consent: { seen: false, planText: null } } };
    log('S2b: home, select the 8GB SeedEXFAT vdisk, start the save/resume vehicle scan');
    await page.evaluate(() => { location.hash = '#/'; });
    await page.waitForTimeout(3000);
    await shot(page, 'save-0-home');

    const exCard = page.getByRole('radio', { name: /SeedEXFAT/i }).first();
    await exCard.click({ timeout: 15000 });
    log('S2b: clicked the SeedEXFAT card');
    await page.waitForTimeout(1500);

    const exScanBtn = page.getByRole('button', { name: /Search for Lost Data/i }).first();
    await exScanBtn.click({ timeout: 15000 });
    log('S2b: clicked Search for Lost Data (SeedEXFAT)');
    await page.waitForTimeout(2500);
    await shot(page, 'save-1-started');

    // Run until the ADVANCED stage is 15 s in. On this exFAT seed the
    // quick stage is a sub-second metadata walk and the deep stage is
    // nearly empty (the NTFS remnant sweep is NTFS-only), so 'advanced
    // +15 s' lands INSIDE the carve matcher — run 42176600928's measured
    // reality (overall 75.6% = 0.6% into carve at the stop). That is
    // accepted deliberately: the stop exercises the carve-window drain
    // (BF-25's general case), the drain gate below waits out the
    // matcher's grind, and the resulting session is genuinely incomplete
    // (carve pending) — exactly what the Resume affordance needs.
    const q0 = Date.now();
    const Q_CAP_MS = 16 * 60 * 1000;
    let qLastShot = 0;
    let advancedSeenAt = null;
    const answerConsentQ = async () => {
      const dlg = page.getByText(/Pre-scan repair request|Confirm the repair/i);
      if (await dlg.count().catch(() => 0)) {
        if (!evidence.phases.saveResume.scan.consent.seen) {
          evidence.phases.saveResume.scan.consent.seen = true;
          const bodyText = await page.evaluate(() => (document.body.innerText || '').slice(0, 5000)).catch(() => '');
          evidence.phases.saveResume.scan.consent.planText =
            (bodyText.match(/(Pre-scan repair request|Confirm the repair)[\s\S]{0,900}/) || [null])[0];
          await shot(page, 'save-consent-modal').catch(() => {});
          log('S2b: consent modal APPEARED - answering Decline (stay read-only)');
        }
        const deny = page.getByRole('button', { name: /Decline|Deny|stay read-only/i }).first();
        if (await deny.count().catch(() => 0)) {
          await deny.click({ timeout: 5000 }).catch(e => log('consent decline click failed: ' + e.message));
        }
      }
    };
    while (Date.now() - q0 < Q_CAP_MS) {
      await answerConsentQ();
      const st = await page.evaluate(() => ({
        url: location.href,
        text: (document.body.innerText || '').slice(0, 5000),
      })).catch(e => ({ url: 'EVAL-FAIL', text: String(e) }));
      const state = extractScanState(st.text || '');
      evidence.phases.saveResume.scan.timeline.push({
        t: new Date().toISOString(), elapsedSec: Math.round((Date.now() - q0) / 1000),
        url: st.url, ...state,
      });
      if (Date.now() - qLastShot > 30000) {
        await shot(page, `save-poll-${Math.round((Date.now() - q0) / 1000)}s`);
        qLastShot = Date.now();
      }
      if (state.stage === 'advanced' && !advancedSeenAt) {
        advancedSeenAt = Date.now() - q0;
        log(`S2b: ADVANCED stage first seen at ${(advancedSeenAt / 1000).toFixed(0)}s`);
      }
      // 15 s into the deep stage (or the cap): STOP — the incomplete
      // session this leg needs.
      if ((advancedSeenAt !== null && Date.now() - q0 - advancedSeenAt >= 15000) || state.complete) {
        break;
      }
      await page.waitForTimeout(5000);
    }
    evidence.phases.saveResume.scan.advancedSeenAtMs = advancedSeenAt;
    await shot(page, 'save-2-before-stop');

    // The stop: dialog → confirm → results (the cancel invoke resolves
    // while the run thread drains — the UI navigates on invoke success).
    const stopBtn = page.getByRole('button', { name: 'Stop', exact: true }).first();
    if (await stopBtn.isVisible({ timeout: 5000 }).catch(() => false)) {
      await stopBtn.click({ timeout: 15000 });
      const stopDlg = page.getByRole('dialog', { name: /Stop Searching for Lost Data\?/i });
      await stopDlg.waitFor({ state: 'visible', timeout: 10000 });
      await shot(page, 'save-3-stop-dialog');
      await stopDlg.getByRole('button', { name: 'Stop', exact: true }).click({ timeout: 10000 });
    } else {
      // Completed before the stop point (unexpected on 8GB, but honest):
      // there is no incomplete session to resume — record and fail the
      // gate (the resume surface went unproven this run).
      evidence.phases.saveResume.scan.completedEarly = true;
      throw new Error('gate: S2b vehicle - the scan completed before the mid-deep stop point (no incomplete session to resume)');
    }
    await page.waitForURL(/\/results\//, { timeout: 30000 });
    await page.waitForTimeout(2000);
    await shot(page, 'save-4-stopped-results');

    // The during-drain save probe — the db600f4c honesty proof, never
    // before exercised on a shipped binary: a save clicked while the run
    // thread is still draining MUST be visible (the refusal toast), never
    // a silent no-op. Run 42176600928's io evidence: the stop landed
    // inside the carve matcher (the exFAT deep stage is nearly empty —
    // 'advanced+15 s' IS carve on this seed), reads cease ~90 s after the
    // cancel, and the matcher grinds on in memory for minutes before the
    // thread observes the token — so at this instant busy is still 1 and
    // the refusal is the EXPECTED outcome. A SUCCESS here is equally fine
    // (the drain already finished). SILENCE is the only failure.
    {
      const earlyBtn = page.getByRole('button', { name: /save this search/i }).first();
      if (await earlyBtn.count().catch(() => 0)) {
        await earlyBtn.click({ timeout: 10000 });
        const earlyToasts = [];
        const earlyUntil = Date.now() + 30000;
        while (Date.now() < earlyUntil && earlyToasts.length === 0) {
          const toasts = await page
            .locator('[data-sonner-toast]')
            .allTextContents()
            .catch(() => []);
          for (const t of toasts) {
            const text = t.trim().slice(0, 200);
            if (text && !earlyToasts.includes(text)) earlyToasts.push(text);
          }
          if (earlyToasts.length === 0) await page.waitForTimeout(400);
        }
        evidence.phases.saveResume.duringDrainSave = { toasts: earlyToasts };
        log(`S2b: during-drain save: ${JSON.stringify(earlyToasts)}`);
        await shot(page, 'save-4b-during-drain-save');
        if (earlyToasts.length === 0) {
          throw new Error('gate: S2b during-drain save - the refused save produced NO toast (the never-silent rule violated)');
        }
      } else {
        evidence.phases.saveResume.duringDrainSave = { skipped: 'save affordance not found on the stopped results page' };
      }
    }

    // The DRAIN gate: the 'stopped early' banner (role=status) renders
    // only when the engine's run thread has returned (status=cancelled
    // latches at run() exit — BF-25's lesson, GENERALIZED by run
    // 42176600928: any stop that lands inside the carve matcher waits out
    // the matcher's remaining grind — reads stop ~90 s after the cancel,
    // the signature match then runs minutes more in memory with no
    // cancellation checkpoint). The window is sized from the measured
    // baseline: the fresh 8GB scan ran 11.8 min end to end (run
    // 42170466748); a stop ~2 min in leaves ~9-10 min of matcher — 15 min
    // covers it with margin. It must STILL land: a drain longer than the
    // scan itself would take is a real product failure — red.
    let drained = false;
    try {
      await page
        .getByRole('status')
        .filter({ hasText: /stopped early/i })
        .first()
        .waitFor({ state: 'visible', timeout: 900000 });
      drained = true;
    } catch {
      drained = false;
    }
    evidence.phases.saveResume.drainBannerVisible = drained;
    log(`S2b: drain banner (stopped early) visible: ${drained}`);
    if (!drained) {
      throw new Error('gate: S2b drain - the stopped scan never showed the partial-results banner within 15 min (the carve-matcher grind exceeds the fresh-scan baseline of 11.8 min — an unbounded drain)');
    }

    // Save the INCOMPLETE session (the results explorer's affordance).
    // Toasts live ~4 s (sonner's default) — poll FAST so a success toast
    // can never auto-dismiss between two slow samples (run 42148493806's
    // empty toast lists taught the timing), and record every toast
    // verbatim. A save clicked inside the drain is refused with a VISIBLE
    // toast (the onError fix) — retry through it.
    const saveBtn = page.getByRole('button', { name: /save this search/i }).first();
    if (!(await saveBtn.count().catch(() => 0))) {
      throw new Error('gate: S2b session save - the results explorer showed no "Save this search" affordance');
    }
    evidence.phases.saveResume.save = { via: 'stopped-mid-deep scan' };
    evidence.phases.saveResume.save.attempts = [];
    let savedOk = false;
    for (let attempt = 1; attempt <= 15 && !savedOk; attempt++) {
      await saveBtn.click({ timeout: 10000 });
      const seenToasts = [];
      const pollUntil = Date.now() + 6500;
      while (Date.now() < pollUntil && !savedOk) {
        const toasts = await page
          .locator('[data-sonner-toast]')
          .allTextContents()
          .catch(() => []);
        for (const t of toasts) {
          const text = t.trim().slice(0, 200);
          if (text && !seenToasts.includes(text)) seenToasts.push(text);
        }
        if (seenToasts.some((t) => /search saved/i.test(t))) {
          savedOk = true;
        } else {
          await page.waitForTimeout(400);
        }
      }
      evidence.phases.saveResume.save.attempts.push({ attempt, toasts: seenToasts });
      log(`S2b: save attempt ${attempt}: ${JSON.stringify(seenToasts)}`);
      if (!savedOk) {
        await page.waitForTimeout(2500); // the drain window
      }
    }
    evidence.phases.saveResume.save.saved = savedOk;
    await shot(page, 'save-5-session-saved');
    if (!savedOk) {
      throw new Error(
        `gate: S2b session save - the save never landed (15 attempts; toasts: ${JSON.stringify(evidence.phases.saveResume.save.attempts.map(a => a.toasts))})`,
      );
    }
    log('S2b: incomplete session saved from the results explorer');

    // S5: the saved session must appear on the Backup screen with a
    // working Resume affordance (incomplete rows render 'Resume scan' +
    // 'Open'), and the resume must land on a live scan.
    evidence.phases.saveResume.backup = {};
    await page.evaluate(() => { location.hash = '#/backup'; });
    await page.waitForTimeout(4000);
    const bk = await domSummary(page);
    evidence.phases.saveResume.backup.summary = { headings: bk.headings, bodyText: bk.bodyText.slice(0, 2500) };
    await shot(page, 'save-6-backup-list');
    const resumeBtn = page.getByRole('button', { name: /resume scan/i }).first();
    const resumeCount = await resumeBtn.count().catch(() => 0);
    evidence.phases.saveResume.backup.resumeAffordance = resumeCount > 0;
    if (resumeCount === 0) {
      throw new Error('gate: S5 session resume - the saved incomplete session shows no Resume affordance on the Backup screen');
    }
    evidence.phases.saveResume.backup.clickedRow = (await resumeBtn.textContent().catch(() => '')) || '';
    await resumeBtn.click({ timeout: 10000 });
    // The load+resume+run of the session's tree can take seconds — wait
    // for the navigation, not a fixed sleep. The live-scan ROUTE is
    // /scan/:deviceId/live?scanId=<id> (run 42185123973: the v0.8.2-era
    // product navigated /scan?scanId= — not a route; the catch-all
    // fallback swallowed it and the waitForURL pattern /\/scan\// never
    // matched either. The v0.8.3 fix navigates the real route; this gate
    // now demands its exact shape — the fallback can never pass it).
    // Diagnostics (runs 42185123973 + 42192131100 both timed out here
    // with NO toast and NO navigation — the invoke itself appears to
    // hang): poll the URL + toasts every second and record everything on
    // timeout so the failure carries its own evidence.
    {
      const deadline = Date.now() + 60000;
      const seenToasts = [];
      let urlNow = page.url();
      while (Date.now() < deadline) {
        urlNow = page.url();
        if (/\/scan\/[^/]+\/live/.test(urlNow)) break;
        const toasts = await page
          .locator('[data-sonner-toast]')
          .allTextContents()
          .catch(() => []);
        for (const t of toasts) {
          const text = t.trim().slice(0, 200);
          if (text && !seenToasts.includes(text)) seenToasts.push(text);
        }
        await page.waitForTimeout(1000);
      }
      evidence.phases.saveResume.resumePoll = { url: page.url(), toasts: seenToasts };
      log(`S2b: resume poll: url=${page.url()} toasts=${JSON.stringify(seenToasts)}`);
      if (!/\/scan\/[^/]+\/live/.test(page.url())) {
        const body = await page
          .evaluate(() => (document.body.innerText || '').slice(0, 2000))
          .catch(() => '');
        evidence.phases.saveResume.resumePoll.bodyText = body;
        throw new Error(
          `gate: S5 resume - never reached the live-scan route (url: ${page.url()}; toasts: ${JSON.stringify(seenToasts)}; body: ${body.slice(0, 300)})`,
        );
      }
    }
    await page.waitForTimeout(5000);
    evidence.phases.saveResume.resume = { urlAfterResume: page.url() };
    await shot(page, 'save-7-after-resume');
    log('S2b: resume navigated to a live scan');

    // The resumed scan continues the deep stage from its checkpoint, then
    // carve, then completes. Measured completions on this seed: the fresh
    // scan 11.8 min (run 42170466748), the resume 15.6 min (run 42201790752)
    // and 16.03 min (run 42212963611 — which COMPLETED at 962 s, five
    // seconds past this loop's former 960 s cap: the carve matcher's
    // variance is minutes, so the cap must clear it with margin).
    // Completion is the gate: a resumed-scan stall would be a real find.
    const r0 = Date.now();
    const R_CAP_MS = 22 * 60 * 1000;
    let rLastShot = 0;
    let rCompleted = false;
    evidence.phases.saveResume.resumedScan = { timeline: [] };
    while (Date.now() - r0 < R_CAP_MS) {
      await answerConsentQ();
      const st = await page.evaluate(() => ({
        url: location.href,
        text: (document.body.innerText || '').slice(0, 5000),
      })).catch(e => ({ url: 'EVAL-FAIL', text: String(e) }));
      const state = extractScanState(st.text || '');
      evidence.phases.saveResume.resumedScan.timeline.push({
        t: new Date().toISOString(), elapsedSec: Math.round((Date.now() - r0) / 1000),
        url: st.url, ...state,
      });
      if (Date.now() - rLastShot > 30000) {
        await shot(page, `resume-poll-${Math.round((Date.now() - r0) / 1000)}s`);
        rLastShot = Date.now();
      }
      if (state.complete) {
        rCompleted = true;
        log('S2b: RESUMED scan COMPLETE');
        break;
      }
      await page.waitForTimeout(5000);
    }
    evidence.phases.saveResume.resumedScan.completed = rCompleted;
    if (!rCompleted) {
      throw new Error('gate: S2b resume completion - the resumed scan did not complete within 22 min (measured baselines: fresh 11.8 min, resumed 15.6-16.0 min; runs 42170466748/42201790752/42212963611)');
    }
    await page.waitForTimeout(4000); // the celebration modal renders
    await shot(page, 'save-8-resumed-complete');
    const goBtn = page.getByRole('button', { name: /go to recover/i }).first();
    if (await goBtn.count().catch(() => 0)) {
      await goBtn.click({ timeout: 10000 });
    } else {
      log('S2b: celebration modal not found (already dismissed?) - continuing from the live page');
    }
    await page.waitForURL(/\/results\//, { timeout: 30000 });
    await page.waitForTimeout(4000);
    await shot(page, 'save-9-resumed-results');
    log('S2b: save/resume vehicle complete - the app is on the resumed results surface');
  } catch (e) {
    (evidence.phases.saveResume ||= {}).error = String(e);
    log('S2b/S5 FAILED: ' + e.message);
    await shot(page, 'save-failed').catch(() => {});
    if (String(e).includes('gate:')) throw e;
    throw new Error(`gate: S2b/S5 save-resume - ${e.message}`); // fails the run - no masking
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

    // Poll the scan for up to 15 minutes. This leg is a RECORDER (the
    // stall is a behavior finding, not a gate) and the full 28-minute
    // corpus already exists (run 42072191860 pre-merge, run 42148493806
    // on v0.7.0: frozen at 76.1% from ~+6 min in both). 15 minutes still
    // captures the frozen signature (9+ minutes of frozen pcts and
    // found-counts) while leaving the job's 90-minute budget room for
    // the save/resume vehicle's carve-matcher drain window (BF-25).
    const t0 = Date.now();
    const CAP_MS = 15 * 60 * 1000;
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
    // finding (BF-22). The stop of the STALLED scan has its own recorded
    // behavior (BF-25): the run thread stays inside the slow carve window,
    // so busy stays set and the partial banner never latches — the recover
    // surface itself is unaffected (recovery runs from the live scan's
    // results, run 42148493806 evidence) and remains a GATE here; the
    // session-save + resume gates moved to the deterministic completion
    // vehicle above (S2b). A failure fails the run.
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
        // BF-25: the "stopped early" banner latches only when the engine's
        // run thread returns (status=cancelled); inside the stall's drain
        // window it will NOT be there yet. Recorded as observed-at-nav,
        // never gated on — the banner's own truth is the drain's.
        const partial = await page.getByRole('status').filter({ hasText: /stopped early/i }).count()
          .catch(() => 0);
        evidence.phases.recover.partialBannerAtNav = partial > 0;
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

          // Destination: a MOUNT row. The native folder picker cannot be
          // driven via CDP by design; the mount list is the product's own
          // in-app destination surface. The rows render the VOLUME LABEL
          // (bridge system_list_mounts: fixed_str(&v.label)) - no drive
          // letter - so match by label: 'Temp' (the windows-2022 scratch
          // volume, 220 GB) with 'SeedEXFAT' (Y:, 8 GB) as the fallback.
          // 'Windows' is C: (overlap-flagged) and 'SeedNTFS' is the source.
          const mount =
            (await page.getByRole('button', { name: /^Temp\b/i }).first().count().catch(() => 0))
              ? page.getByRole('button', { name: /^Temp\b/i }).first()
              : page.getByRole('button', { name: /^SeedEXFAT\b/i }).first();
          if (await mount.count().catch(() => 0)) {
            await mount.click({ timeout: 10000 });
            await page.waitForTimeout(2500); // the F12 overlap validation
            evidence.phases.recover.destination = await mount.textContent().then(t => (t || '').trim().slice(0, 80)).catch(() => 'mount row');
            await shot(page, 'recover-4-destination-chosen');
            const start = page.getByRole('button', { name: /recover \d+ items?/i }).first();
            await start.click({ timeout: 15000 });
            log('S3b: recovery started');
            // The report wait. Run 42138026793 rejected this wait in 268 ms
            // on WebView2 (non-timeout; the report WAS already visible in
            // the same-instant screenshot 71) while the identical pattern
            // resolves in ~1.3 s on Chromium (local repro #1/#2). Rather
            // than trust the ARIA engine over CDP/WebView2, this uses the
            // CSS engine, captures any rejection VERBATIM (the forensic
            // record must show it), retries once, and dumps the live
            // role=status inventory on failure.
            const reportWait = async (ms) => {
              try {
                await page
                  .locator('[role="status"]')
                  .filter({ hasText: /recovery finished|recovery failed|issues/i })
                  .first()
                  .waitFor({ state: 'visible', timeout: ms });
                return { ok: true };
              } catch (e) {
                return { ok: false, error: `${e.name}: ${String(e.message).slice(0, 400)}` };
              }
            };
            let report = await reportWait(180000);
            if (!report.ok) {
              log(`S3b: report wait rejected (${report.error}); retrying once after 2s`);
              await page.waitForTimeout(2000);
              report = await reportWait(60000);
            }
            const reportText = report.ok
              ? ((await page.locator('[role="status"]').filter({ hasText: /recovery/i }).first().textContent().catch(() => '')) || '')
              : '';
            evidence.phases.recover.reportText = reportText.trim().slice(0, 600);
            evidence.phases.recover.reportWait = report;
            await shot(page, 'recover-5-report');
            log(`S3b: recovery report: ${(reportText.trim() || 'NOT OBSERVED').slice(0, 120)}`);
            if (!report.ok) {
              const inventory = await page
                .evaluate(() =>
                  [...document.querySelectorAll('[role="status"]')].map((el) => ({
                    text: (el.textContent || '').trim().slice(0, 100),
                    visible: !!(el.offsetWidth || el.offsetHeight),
                  })),
                )
                .catch(() => 'evaluate failed');
              evidence.phases.recover.statusInventory = inventory;
              log('S3b: role=status inventory: ' + JSON.stringify(inventory).slice(0, 500));
              throw new Error(`RECOVERY REPORT WAIT FAILED: ${report.error}`);
            }
          } else {
            evidence.phases.recover.destination = 'no usable mount row found (Temp / SeedEXFAT)';
            await shot(page, 'recover-3-no-mount-rows');
            throw new Error('the destination mount rows did not render (no Temp/SeedEXFAT row)');
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

      // S5 (session save-resume) used to live here, riding the stopped
      // 64GB scan. Run 42148493806 proved that vehicle unsound (BF-25:
      // the stalled scan's run thread never returns, busy stays set, the
      // save is refused forever); the gates now run on the deterministic
      // completion vehicle (S2b, above) before this leg starts.
  } catch (e) {
    // S3 itself stays a RECORDER (the scan's speed/stall is a behavior
    // finding, not a gate) - but the S3b GATE failures rethrow past this
    // catch (marked gate:), because a failed recover surface must
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
