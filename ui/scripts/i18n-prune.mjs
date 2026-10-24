#!/usr/bin/env node
/**
 * i18n-prune.mjs — one-shot dead-key purge (T12).
 * Removes every key reported UNUSED by analyze() from BOTH catalogs
 * (en + zh-CN must stay surface-identical), then rewrites the files as
 * canonical TS literals. Run `pnpm format` afterwards.
 *
 * The en⇄zh parity test and tests/i18n-sweep.test.ts (unused must be 0)
 * keep the surface honest from here on.
 */
import { writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { analyze } from './i18n-sweep.mjs';

const ROOT = import.meta.dirname + '/..';
const EN_PATH = join(ROOT, 'src/i18n/en.ts');
const ZH_PATH = join(ROOT, 'src/i18n/zh-CN.ts');

const { unused } = analyze();
if (unused.length === 0) {
  console.log('nothing to prune');
  process.exit(0);
}
const kill = new Set(unused);

const enMod = await import(EN_PATH);
const zhMod = await import(ZH_PATH);
const en = structuredClone(enMod.en);
const zh = structuredClone(zhMod.zhCN);

function prune(obj, path = []) {
  let touched = 0;
  for (const key of Object.keys(obj)) {
    const full = [...path, key].join('.');
    if (obj[key] !== null && typeof obj[key] === 'object') {
      touched += prune(obj[key], [...path, key]);
      if (Object.keys(obj[key]).length === 0) {
        delete obj[key]; // empty namespace after pruning
        touched++;
      }
    } else if (kill.has(full) || kill.has(`${full}_other`)) {
      delete obj[key];
      touched++;
    }
  }
  return touched;
}

const removedEn = prune(en);
const removedZh = prune(zh);
if (removedEn !== removedZh) {
  console.error(`parity divergence: removed ${removedEn} from en but ${removedZh} from zh`);
  process.exit(1);
}

function serialize(obj, indent = 2) {
  const pad = ' '.repeat(indent);
  const lines = ['{'];
  for (const [k, v] of Object.entries(obj)) {
    const key = /^[A-Za-z0-9_]+$/.test(k) ? k : JSON.stringify(k);
    if (v !== null && typeof v === 'object') {
      lines.push(`${pad}${key}: ${serialize(v, indent + 2)},`);
    } else {
      lines.push(`${pad}${key}: ${JSON.stringify(v)},`);
    }
  }
  lines.push(' '.repeat(Math.max(0, indent - 2)) + '}');
  return lines.join('\n');
}

const enOut = `/** English resources — original copy for the Recovery Wizard UI. */\n\nexport const en = ${serialize(en)};\n\nexport type TranslationSchema = typeof en;\n`;
const zhOut = `/** Simplified Chinese resources — original copy for the Recovery Wizard UI. */\n\nimport type { TranslationSchema } from './en';\n\nexport const zhCN: TranslationSchema = ${serialize(zh)};\n`;
writeFileSync(EN_PATH, enOut);
writeFileSync(ZH_PATH, zhOut);
console.log(`pruned ${removedEn} keys from each catalog (${unused.length} reported unused)`);
