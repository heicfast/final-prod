#!/usr/bin/env python3
"""regression_gate.py - compare this run's metrics against the versioned baseline.

Baseline policy: bench/baseline.json is updated ONLY by a human-reviewed commit.
The gate FAILS the run when ours regresses beyond thresholds:
  - recall  drops more than 2% absolute
  - precision drops more than 2% absolute
  - scan wall time grows more than 25%
Missing scenarios in the new run are failures (no silent drops). New scenarios
are reported (info only) so the baseline can be extended deliberately.

Licensed under the MIT license. See LICENSE.
"""

import argparse
import glob
import json
import os
import sys

RECALL_TOL = 0.02
PRECISION_TOL = 0.02
TIME_TOL = 0.25


def load(path):
    if not os.path.exists(path):
        return None
    with open(path, 'r', encoding='utf-8-sig') as f:
        return json.load(f)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--baseline', required=True)
    ap.add_argument('--ours-root', required=True)
    ap.add_argument('--out', default='regression-gate.json')
    args = ap.parse_args()

    baseline = load(args.baseline) or {}
    base_scen = baseline.get('scenarios', {})

    current = {}
    for d in sorted(glob.glob(os.path.join(args.ours_root, '*'))):
        m = load(os.path.join(d, 'metrics.json'))
        if m:
            current[os.path.basename(d)] = m

    failures = []
    regressions = []
    news = []
    for scen, m in sorted(current.items()):
        b = base_scen.get(scen)
        if not b:
            news.append(scen)
            continue
        for key, tol in (('recall', RECALL_TOL), ('precision', PRECISION_TOL)):
            cv, bv = m.get(key), b.get(key)
            if cv is None or bv is None:
                continue
            if cv < bv - tol:
                failures.append(f'{scen}: {key} regressed {fmt(bv)} -> {fmt(cv)} (tol {tol * 100:.0f}%)')
            elif cv < bv:
                regressions.append(f'{scen}: {key} dipped {fmt(bv)} -> {fmt(cv)} (within tolerance)')
        perf = m.get('perf') or {}
        bperf = b.get('perf') or {}
        for mode in perf:
            ct = (perf.get(mode) or {}).get('scan_wall_seconds')
            bt = (bperf.get(mode) or {}).get('scan_wall_seconds')
            if ct and bt and ct > bt * (1 + TIME_TOL):
                failures.append(f'{scen}/{mode}: scan time regressed {bt}s -> {ct}s (tol {TIME_TOL * 100:.0f}%)')

    dropped = [s for s in base_scen if s not in current and not base_scen[s].get('optional')]
    for s in dropped:
        failures.append(f'{s}: scenario missing from this run (no silent scenario drops)')

    out = {
        'baseline': os.path.abspath(args.baseline),
        'scenarios_current': sorted(current),
        'scenarios_new': news,
        'regressions_within_tolerance': regressions,
        'failures': failures,
        'passed': not failures,
    }
    with open(args.out, 'w', encoding='utf-8') as f:
        json.dump(out, f, indent=1)
    for r in regressions:
        print(f'DIP  {r}')
    for fl in failures:
        print(f'FAIL {fl}')
    print(f"regression gate: {'PASS' if not failures else 'FAIL'} "
          f"({len(current)} scenarios, {len(news)} new, {len(failures)} failures)")
    return 0 if not failures else 1


def fmt(v):
    return f'{v * 100:.1f}%'


if __name__ == '__main__':
    sys.exit(main())
