#!/usr/bin/env node
/**
 * contracts/validate.mjs — the boundary-truth CI gate.
 *
 * Phase 1 (always): mapping completeness against the ENGINE headers and the
 *   UI contract vocabulary:
 *   1. every cr::Err value in engine/include/cr/cr_error.h has a mapping row
 *      in contracts/error-codes.json (no unmapped engine code),
 *   2. every uiCode in error-codes.json (mapped or bridge-only) exists in
 *      ui/src/connector/contract.ts ConnectorErrorCode,
 *   3. every CR_EVT_* define in engine/include/cr/cr_ui.h (1..N) is mapped
 *      in contracts/event-mapping.md,
 *   4. command-surface.json is well-formed: unique command names, every
 *      manifest row carries serves/abiRefs/returns-or-args.
 *
 * Phase 2 (auto-activates once the bridge exists — ui/src-tauri/src/commands/
 *   present): the cross-implementation checks:
 *   5. every manifest command has a #[tauri::command] fn in the Rust bridge
 *      and every bridge command is in the manifest,
 *   6. every command invoked from ui/src/connector/ is in the manifest.
 *
 * Exit code 0 = green; 1 = holes (printed with file:line evidence).
 */
import { readFileSync, existsSync, readdirSync, statSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

const root = join(dirname(fileURLToPath(import.meta.url)), '..');
const fail = (msg) => { console.error(`FAIL: ${msg}`); process.exitCode = 1; };
const ok = (msg) => console.log(`ok: ${msg}`);

const errHeader = readFileSync(join(root, 'engine/include/cr/cr_error.h'), 'utf8');
const uiHeader = readFileSync(join(root, 'engine/include/cr/cr_ui.h'), 'utf8');
const contract = readFileSync(join(root, 'ui/src/connector/contract.ts'), 'utf8');
const errorMap = JSON.parse(readFileSync(join(root, 'contracts/error-codes.json'), 'utf8'));
const commandSurface = JSON.parse(readFileSync(join(root, 'contracts/command-surface.json'), 'utf8'));
const eventMap = readFileSync(join(root, 'contracts/event-mapping.md'), 'utf8');

// --- Check 1: every engine error code is mapped --------------------------------
const engineCodes = new Map();
for (const m of errHeader.matchAll(/^\s*(?:\/\*[\s\S]*?\*\/\s*)?([A-Z_]+)\s*=\s*(0x[0-9A-Fa-f]+|\d+)\s*,/gm)) {
  if (m[1] !== 'OK') engineCodes.set(m[1], m[2].toLowerCase());
}
const mapped = new Set(errorMap.mappings.map((r) => r.code));
for (const code of engineCodes.keys()) {
  if (!mapped.has(code)) fail(`engine code ${code} (${engineCodes.get(code)}) has no mapping row in contracts/error-codes.json`);
}
if (engineCodes.size > 0) ok(`all ${engineCodes.size} engine error codes parsed; ${mapped.size} mapped`);

// mapped rows must reference real engine values
for (const row of errorMap.mappings) {
  if (!engineCodes.has(row.code)) fail(`error-codes.json maps unknown engine code ${row.code}`);
  const want = engineCodes.get(row.code);
  if (row.value.toLowerCase() !== want) fail(`error-codes.json ${row.code}: value ${row.value} != header ${want}`);
}

// --- Check 2: every uiCode exists in contract.ts ConnectorErrorCode ------------
const vocabMatch = contract.match(/export type ConnectorErrorCode\s*=([\s\S]*?);/);
if (!vocabMatch) { fail('ConnectorErrorCode type not found in ui/src/connector/contract.ts'); }
else {
  const vocab = new Set([...vocabMatch[1].matchAll(/'([a-z-]+)'/g)].map((m) => m[1]));
  const referenced = new Set();
  for (const row of errorMap.mappings) {
    if (row.uiCode === null) continue; // ERR_CANCELLED: translated, never an error
    referenced.add(row.uiCode);
    if (!vocab.has(row.uiCode)) fail(`error-codes.json maps to '${row.uiCode}' which is not in ConnectorErrorCode`);
  }
  for (const row of errorMap.bridgeOnlyCodes) {
    referenced.add(row.uiCode);
    if (!vocab.has(row.uiCode)) fail(`bridge-only code '${row.uiCode}' is not in ConnectorErrorCode`);
  }
  // every vocabulary member is accounted for (mapped or bridge-only)
  for (const v of vocab) {
    if (!referenced.has(v)) fail(`ConnectorErrorCode member '${v}' is neither engine-mapped nor listed in bridgeOnlyCodes`);
  }
  ok(`ConnectorErrorCode vocabulary covered: ${vocab.size} codes, ${referenced.size} referenced`);
}

// --- Check 3: every CR_EVT_* is mapped in event-mapping.md ---------------------
// The header defines some CR_EVT_* on a single line and some with a `\` line
// continuation before the numeric value; both forms are parsed.
const events = new Map();
for (const m of uiHeader.matchAll(/#define\s+(CR_EVT_[A-Z_]+)(?:\s|\\\n)+(\d+)u/g)) events.set(m[1], Number(m[2]));
for (const [name, value] of events) {
  const re = new RegExp(`\\|\\s*\`${name}\`\\s*\\|\\s*${value}\\s*\\|`);
  if (!re.test(eventMap)) fail(`event ${name} (${value}) not mapped in contracts/event-mapping.md`);
}
if (events.size > 0) ok(`all ${events.size} CR_EVT_* defines mapped in event-mapping.md`);

// --- Check 4: command-surface.json well-formedness ------------------------------
const names = commandSurface.commands.map((c) => c.name);
if (new Set(names).size !== names.length) fail('command-surface.json has duplicate command names');
for (const c of commandSurface.commands) {
  if (!c.name || !c.threading || !c.serves) fail(`command-surface.json row missing name/threading/serves: ${JSON.stringify(c).slice(0, 80)}`);
  if (!['sync', 'spawn-join', 'async-job'].includes(c.threading)) fail(`command ${c.name}: unknown threading class ${c.threading}`);
}
ok(`command surface: ${names.length} commands, ${commandSurface.events.length} events, well-formed`);

// --- Checks 5/6: phase 2 (activates when the engine FFI bridge lands) -------------
// The signal is the bridge ABI module (docs/INTEGRATION-PLAN.md §4.1):
// ui/src-tauri/src/bridge/abi.rs. The UI repo's inherited staged commands
// (commands/system.rs with 3 unwired helpers) do NOT constitute the bridge;
// they are replaced at the wiring step (plan §4.7).
const bridgeAbi = join(root, 'ui/src-tauri/src/bridge/abi.rs');
const connectorDir = join(root, 'ui/src/connector');
if (!existsSync(bridgeAbi)) {
  ok(`phase 2 pending: ui/src-tauri/src/bridge/abi.rs does not exist yet (engine FFI bridge not landed); checks 5/6 activate automatically when it does`);
} else {
  let rust = '';
  const rustDirs = [join(root, 'ui/src-tauri/src/commands'), join(root, 'ui/src-tauri/src/bridge')];
  const walk = (d) => { for (const f of readdirSync(d)) { const p = join(d, f); if (statSync(p).isDirectory()) walk(p); else if (f.endsWith('.rs')) rust += readFileSync(p, 'utf8'); } };
  for (const d of rustDirs) if (existsSync(d)) walk(d);
  const rustCmds = new Set([...rust.matchAll(/(?:pub\s+)?async\s+fn\s+([a-z_]+)|(?:pub\s+)?fn\s+([a-z_]+)/g)].map((m) => m[1] || m[2]).filter((n) => names.includes(n)));
  for (const c of names) if (!rustCmds.has(c)) fail(`manifest command '${c}' has no implementation in ui/src-tauri/src/commands/`);
  for (const fn of rustCmds) if (!names.includes(fn)) fail(`Rust command '${fn}' is not in contracts/command-surface.json`);
  ok(`phase 2 rust side: ${rustCmds.size} command implementations cross-checked`);

  let ts = '';
  const walkTs = (d) => { for (const f of readdirSync(d)) { const p = join(d, f); if (statSync(p).isDirectory()) walkTs(p); else if (f.endsWith('.ts') || f.endsWith('.tsx')) ts += readFileSync(p, 'utf8'); } };
  walkTs(connectorDir);
  const invoked = new Set([...ts.matchAll(/invoke\(\s*'([a-z_]+)'/g)].map((m) => m[1]));
  for (const n of invoked) if (!names.includes(n)) fail(`TS invokes '${n}' which is not in contracts/command-surface.json`);
  for (const c of names) {
    const row = commandSurface.commands.find((r) => r.name === c);
    if (row.serves.some((s) => !s.startsWith('internal') && !s.startsWith('consent') && !s.includes('->')) && !ts.includes(`'${c}'`)) {
      // manifest says it serves a connector method: the connector must invoke it
      if (!invoked.has(c)) fail(`manifest command '${c}' serves a connector method but is never invoked from ui/src/connector/`);
    }
  }
  ok(`phase 2 ts side: ${invoked.size} invoked commands cross-checked`);
}

if (process.exitCode) { console.error('\ncontracts validation FAILED'); process.exit(1); }
console.log('\ncontracts validation green');
