#!/usr/bin/env node
/**
 * drive-ui.mjs — forensic UI probe: drives the REAL installed product (the
 * shipped v0.6.0-rc1 build, launched with WebView2 CDP enabled) exactly as a
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
  const browser = await chromium.connectOverCDP(`http://127.0.0.1:${CDP_PORT}`);
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

  // ---------------------------------------------------------------- S2: health
  try {
    log('S2 probe: navigating to #/health');
    await page.evaluate(() => { location.hash = '#/health'; });
    await page.waitForTimeout(3500);
    const healthBase = await domSummary(page);
    evidence.phases.health = { initial: healthBase, drives: [] };
    await shot(page, 'health-initial');

    // drive selector: click every button that looks like a drive (contains a
    // size unit) and capture the health surface for each selection
    const candidates = await page.evaluate(() => {
      const els = [...document.querySelectorAll('button, [role="tab"], [role="button"], li')];
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
            statusWords: (text.match(/\b(Good|Caution|Bad|Unknown|Unavailable|Not available)\b/g) || []).slice(0, 10),
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

    // find the device card for the 64GB virtual disk (disk1.vhdx)
    const cardIdx = await page.evaluate(() => {
      const els = [...document.querySelectorAll('div, section, article, button')];
      for (let i = els.length - 1; i >= 0; i--) {
        const t = els[i].textContent || '';
        if (/Virtual Disk/i.test(t) && /64\s*GB/i.test(t) && t.length < 900) return i;
      }
      return -1;
    });
    log(`scan: 64GB vdisk card index ${cardIdx}`);
    if (cardIdx < 0) throw new Error('64GB virtual-disk card not found on home page');
    await page.evaluate((i) => {
      [...document.querySelectorAll('div, section, article, button')][i].click();
    }, cardIdx);
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
    while (Date.now() - t0 < CAP_MS) {
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
  } catch (e) {
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
