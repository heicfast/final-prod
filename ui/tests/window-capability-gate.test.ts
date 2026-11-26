import { describe, expect, it } from 'vitest';
import { readFileSync, readdirSync, statSync } from 'node:fs';
import { join, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

/**
 * SB-B-02 (the capability half of the P0 X-button defect): every window API
 * call the frontend makes must be permitted by the window's capability file.
 *
 * The defect: `exit-guard.tsx` (and the @tauri-apps/api 2.12.4
 * onCloseRequested wrapper itself) call `destroy()` — the ACL-gated
 * `plugin:window|destroy` command — while `capabilities/default.json`
 * granted close/minimize/toggle-maximize/is-maximized/start-dragging/set-focus
 * but NOT `core:window:allow-destroy`. On a frameless window whose only
 * close affordance is the custom X, a denied destroy means the window can
 * never close: the title-bar X becomes a dead button in every state.
 *
 * Gate logic (static, runs in CI on every push):
 *  1. scan `ui/src` for files importing the Tauri window/webview APIs and
 *     collect every window-instance method they call;
 *  2. assert each used method's permission is granted in the capability of
 *     the window(s) that render the using file (main = default.json,
 *     disk-health = disk-health.json; shared chrome files must be granted
 *     in BOTH);
 *  3. methods covered by `core:window:default` (via core:default) are
 *     exempt (verified against the Tauri 2 reference ACL: get-all-windows,
 *     is-maximized and the read-only getters are in the default set;
 *     destroy/close/minimize/toggle-maximize are NOT).
 */

const uiRoot = join(dirname(fileURLToPath(import.meta.url)), '..', 'src');
const capabilitiesDir = join(
  dirname(fileURLToPath(import.meta.url)),
  '..',
  'src-tauri',
  'capabilities',
);

/** method → required permission (tauri 2 ACL: plugin:window|<m> → allow-<m>). */
const METHOD_PERMISSIONS: Record<string, string> = {
  close: 'core:window:allow-close',
  destroy: 'core:window:allow-destroy',
  minimize: 'core:window:allow-minimize',
  toggleMaximize: 'core:window:allow-toggle-maximize',
  startDragging: 'core:window:allow-start-dragging',
  setFocus: 'core:window:allow-set-focus',
  show: 'core:window:allow-show',
  hide: 'core:window:allow-hide',
  setTitle: 'core:window:allow-set-title',
  setDecorations: 'core:window:allow-set-decorations',
  setSize: 'core:window:allow-set-size',
  setPosition: 'core:window:allow-set-position',
  center: 'core:window:allow-center',
};

/** Methods included in core:window:default (granted via `core:default`) —
 * verified against the Tauri 2 core-permissions reference. */
const DEFAULT_COVERED = new Set(['isMaximized', 'getByLabel', 'getAll']);

/** The method set used by files rendered in BOTH windows (the chrome). */
const SHARED_CHROME_FILES = new Set(['components/ui/window-controls.tsx']);

function walk(dir: string, out: string[] = []): string[] {
  for (const name of readdirSync(dir)) {
    const p = join(dir, name);
    if (statSync(p).isDirectory()) walk(p, out);
    else if (/\.(ts|tsx)$/.test(name)) out.push(p);
  }
  return out;
}

interface Usage {
  rel: string;
  methods: Set<string>;
  usesWebviewWindow: boolean;
}

function scanUsage(): Usage[] {
  const usages: Usage[] = [];
  for (const p of walk(uiRoot)) {
    const src = readFileSync(p, 'utf8');
    const importsWindowApi =
      /from '@tauri-apps\/api\/window'/.test(src) ||
      /from '@tauri-apps\/api\/webviewWindow'/.test(src) ||
      /from '@tauri-apps\/api\/webview'/.test(src) ||
      /from '@tauri-apps\/api\/event'/.test(src);
    if (!importsWindowApi) continue;
    const methods = new Set<string>();
    for (const m of Object.keys(METHOD_PERMISSIONS)) {
      if (new RegExp(`\\.${m}\\(`).test(src)) methods.add(m);
    }
    for (const m of DEFAULT_COVERED) {
      if (new RegExp(`\\.${m}\\(`).test(src)) methods.add(m);
    }
    const usesWebviewWindow =
      /new WebviewWindow\(/.test(src) || /WebviewWindow\.getByLabel\(/.test(src);
    usages.push({ rel: p.slice(uiRoot.length + 1), methods, usesWebviewWindow });
  }
  return usages;
}

function loadPermissions(id: string): Set<string> {
  const doc = JSON.parse(readFileSync(join(capabilitiesDir, `${id}.json`), 'utf8')) as {
    permissions: string[];
  };
  return new Set(doc.permissions);
}

describe('window capability gate (SB-B-02) — every window API call is permitted', () => {
  const mainPerms = loadPermissions('default');
  const healthPerms = loadPermissions('disk-health');
  const usages = scanUsage();

  it('the scan found real window-API usage (the gate is live, not vacuous)', () => {
    expect(usages.length).toBeGreaterThan(0);
    const destroyUsage = usages.find((u) => u.methods.has('destroy'));
    expect(destroyUsage?.rel).toBe('app/exit-guard.tsx');
  });

  it('every used method has its permission granted where the code runs', () => {
    const problems: string[] = [];
    for (const u of usages) {
      const caps = SHARED_CHROME_FILES.has(u.rel)
        ? [
            { window: 'main', perms: mainPerms },
            { window: 'disk-health', perms: healthPerms },
          ]
        : [{ window: 'main', perms: mainPerms }];
      for (const m of u.methods) {
        if (DEFAULT_COVERED.has(m)) continue;
        const perm = METHOD_PERMISSIONS[m];
        if (!perm) continue;
        for (const cap of caps) {
          if (!cap.perms.has(perm)) {
            problems.push(`${u.rel} uses .${m}() but ${cap.window} capability lacks ${perm}`);
          }
        }
      }
      if (u.usesWebviewWindow && !mainPerms.has('core:webview:allow-create-webview-window')) {
        problems.push(`${u.rel} constructs WebviewWindow but main capability lacks the grant`);
      }
    }
    expect(problems).toEqual([]);
  });

  it('the P0 pin: destroy() is used → core:window:allow-destroy must be granted (main window)', () => {
    // The exact regression of SB-B-02: allow-destroy missing while exit-guard
    // and the onCloseRequested wrapper both invoke plugin:window|destroy.
    expect(mainPerms.has('core:window:allow-destroy')).toBe(true);
  });
});
