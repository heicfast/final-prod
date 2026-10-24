/** Type surface for scripts/i18n-sweep.mjs (consumed by the sweep test). */
export interface SweepResult {
  defined: string[];
  used: string[];
  unused: string[];
  missing: string[];
}
export function definedKeys(enSource: string): Set<string>;
export function analyze(): SweepResult;
