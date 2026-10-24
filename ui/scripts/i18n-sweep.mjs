#!/usr/bin/env node
/**
 * i18n-sweep.mjs — i18n surface analysis (T12 governance).
 *
 * analyze() reports:
 *  - defined: keys in src/i18n/en.ts
 *  - referenced: keys referenced from src (t('…') static, t(`…${…}`) template
 *    families, labelKey/reasonKey/descriptionKey indirection, connector
 *    reason-literal families)
 *  - unused: defined but never referenced (dead surface — must be 0)
 *  - missing: referenced but not defined (raw-key render bugs — must be 0)
 *
 * Enforced by tests/i18n-sweep.test.ts; also runnable as a CLI report.
 */

import { readFileSync, readdirSync, statSync } from 'node:fs';
import { join } from 'node:path';

const ROOT = join(import.meta.dirname, '..');
const SRC = join(ROOT, 'src');

function walk(dir, out = []) {
  for (const f of readdirSync(dir)) {
    const p = join(dir, f);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (/\.(tsx?|css|html)$/.test(f)) out.push(p);
  }
  return out;
}

/** Extract the flattened key set from the en.ts object literal. */
export function definedKeys(enSource) {
  const defined = new Set();
  const ns = [];
  let buf = '';
  let idx = 0;
  const line = enSource;
  while (idx < line.length) {
    const ch = line[idx];
    if (ch === '{') {
      ns.push(buf);
      buf = '';
      idx++;
      continue;
    }
    if (ch === '}') {
      ns.pop();
      idx++;
      continue;
    }
    const m = /^([A-Za-z0-9_]+)\s*:\s*/.exec(line.slice(idx));
    if (m) {
      const key = m[1];
      idx += m[0].length;
      const rest = line.slice(idx);
      if (rest.startsWith('{')) {
        ns.push(key);
        idx++;
      } else {
        defined.add([...ns, key].filter(Boolean).join('.'));
        const twin = /^([A-Za-z0-9_]+)_other\s*:/.exec(rest);
        if (twin) defined.add([...ns, `${key}_other`].filter(Boolean).join('.'));
        const v = /^(['"`])((?:\\.|(?!\1).)*)\1\s*,/.exec(rest);
        idx += v ? v[0].length : 0;
      }
      continue;
    }
    idx++;
  }
  return defined;
}

export function analyze() {
  const files = walk(SRC);
  const corpus = files.map((p) => ({ p, s: readFileSync(p, 'utf8') }));
  const defined = definedKeys(readFileSync(join(SRC, 'i18n/en.ts'), 'utf8'));

  const used = new Set();
  const DYNAMIC = new Set();
  for (const { p, s } of corpus) {
    if (p.endsWith('.html') || p.endsWith('.css')) continue;
    for (const m of s.matchAll(/\bt\(\s*'([^']+)'/g)) used.add(m[1]);
    for (const m of s.matchAll(/\bt\(\s*"([^"]+)"/g)) used.add(m[1]);
    for (const m of s.matchAll(/\bt\(\s*`([^`$]*)`/g)) used.add(m[1]);
    for (const m of s.matchAll(/\bt\(\s*`([^`$]*)\$\{/g)) DYNAMIC.add(m[1]);
    for (const m of s.matchAll(/labelKey:\s*'([^']+)'/g)) used.add(m[1]);
    for (const m of s.matchAll(/reasonKey[:=]\s*'([^']+)'/g)) used.add(m[1]);
    for (const m of s.matchAll(/descriptionKey:\s*'([^']+)'/g)) used.add(m[1]);
  }
  // Connector reason/finding literals (connError positional args, mock
  // descriptionKey values) and any other key-shaped literals in the codebase.
  for (const { s } of corpus) {
    for (const m of s.matchAll(
      /'((?:errors|preview\.reason|recovery|repair|about)\.[A-Za-z0-9_.]+)'/g,
    ))
      used.add(m[1]);
  }
  // Ternary/conditional keyed calls — t(cond ? 'a.b' : 'c.d') — and any other
  // reference a key-shaped literal ANYWHERE in source. Over-approximation is
  // deliberate: a false "used" only weakens the dead-key check, while a false
  // "unused" deletes a live key and renders raw keys at runtime (this exact
  // regression shipped scan.quickScanning once — the e2e caught it on
  // Windows; the unit suite could not).
  const NAMESPACES =
    'app|about|common|nav|sidebar|home|categories|scan|results|preview|recovery|health|repair|settings|errors|a11y|devPanel|lostPartition';
  for (const { s } of corpus) {
    for (const m of s.matchAll(new RegExp(`'((${NAMESPACES})\\.[A-Za-z0-9_.]+)'`, 'g')))
      used.add(m[1]);
  }
  // Expand template families against defined keys.
  for (const prefix of DYNAMIC) {
    for (const k of defined) if (k.startsWith(prefix)) used.add(k);
  }
  // i18next plural pairs: referencing `X` with a count also resolves `X_other`
  // (and vice versa) — one entry, two keys. Treat pairs as used together.
  const usedWithTwins = new Set(used);
  for (const u of used) {
    if (u.endsWith('_other')) usedWithTwins.add(u.slice(0, -6));
    else usedWithTwins.add(`${u}_other`);
  }

  const unused = [...defined].filter((k) => !usedWithTwins.has(k)).sort();
  const missing = [...used].filter((k) => !defined.has(k)).sort();
  return { defined: [...defined].sort(), used: [...used].sort(), unused, missing };
}

// CLI report when run directly.
const invokedDirectly =
  typeof process !== 'undefined' &&
  process.argv[1] &&
  import.meta.url === `file://${process.argv[1]}`;
if (invokedDirectly) {
  const { defined, used, unused, missing } = analyze();
  console.log(`defined=${defined.length} referenced=${used.length}`);
  console.log(`\n== UNUSED (${unused.length}) ==`);
  for (const k of unused) console.log(k);
  console.log(`\n== REFERENCED-BUT-MISSING (${missing.length}) ==`);
  for (const k of missing) console.log(k);
}
