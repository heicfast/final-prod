#!/usr/bin/env python3
"""make_baseline.py - generate a bench/baseline.json candidate from a completed
ours leg (the per-scenario metric directories as laid down by compare.py's
'Reorganize legs' step: ours/<scenario>/metrics-deepcarve.json).

The baseline is versioned and updated ONLY by human review: this tool emits the
CANDIDATE json to stdout (or --out); a human reviews the numbers, then commits
it as bench/baseline.json. Never wire this into CI auto-commit.

Baseline schema (regression_gate.py contract):
  scenarios.<scenario>.{recall, precision, perf.<mode>.scan_wall_seconds}

Scan-time baselines are only taken from UNcapped modes (a capped scan's wall
time is the cap, not the engine's speed - a baseline from it would be a lie).

Licensed under the MIT license. See LICENSE.
"""

import argparse
import glob
import json
import os
import sys


def load(path):
    try:
        with open(path, 'r', encoding='utf-8-sig') as f:
            return json.load(f)
    except Exception:
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--ours-root', required=True,
                    help='dir with per-scenario ours results (compare.py reorganized layout)')
    ap.add_argument('--out', default='-', help='output json path ("-" = stdout)')
    ap.add_argument('--run-id', default='', help='provenance: source run id')
    args = ap.parse_args()

    scenarios = {}
    notes = []
    for d in sorted(glob.glob(os.path.join(args.ours_root, '*'))):
        if not os.path.isdir(d):
            continue
        m = load(os.path.join(d, 'metrics-deepcarve.json'))
        if not m:
            notes.append(f'{os.path.basename(d)}: no metrics-deepcarve.json (leg failed or og dir) - EXCLUDED')
            continue
        scen = os.path.basename(d)
        if m.get('tool') != 'ours':
            notes.append(f'{scen}: metrics not ours leg - EXCLUDED')
            continue
        entry = {
            'recall': m.get('recall'),
            'precision': m.get('precision'),
            'perf': {},
        }
        # per-mode scan times from all metrics-<mode>.json files, uncapped only
        for mp in sorted(glob.glob(os.path.join(d, 'metrics-*.json'))):
            mm = load(mp)
            if not mm or mm.get('tool') != 'ours':
                continue
            mode = mm.get('mode') or os.path.basename(mp)[len('metrics-'):-len('.json')]
            perf = mm.get('perf') or {}
            pmode = perf.get(mode) or {}
            wall = pmode.get('scan_wall_seconds')
            if wall is None:
                continue
            if pmode.get('scan_capped') or mm.get('perf', {}).get(mode, {}).get('scan_capped'):
                notes.append(f'{scen}/{mode}: scan was CAPPED ({wall}s) - wall time excluded from baseline')
                entry['perf'][mode] = {'scan_wall_seconds': None, 'capped': True}
                continue
            entry['perf'][mode] = {'scan_wall_seconds': wall}
        scenarios[scen] = entry

    out = {
        '_comment': 'Versioned regression baseline for the ours leg. Updated ONLY by '
                    'human-reviewed commits. Format: scenarios.<scenario>.{recall,precision,'
                    'perf.<mode>.scan_wall_seconds} as produced by bench/metrics/compute_metrics.py. '
                    f'Source: run {args.run_id or "unspecified"} via bench/metrics/make_baseline.py.',
        'schema': 1,
        'source_run_id': args.run_id,
        'scenarios': scenarios,
    }
    if notes:
        out['exclusions'] = notes

    text = json.dumps(out, indent=1)
    if args.out == '-':
        print(text)
    else:
        with open(args.out, 'w', encoding='utf-8') as f:
            f.write(text + '\n')
        print(f'wrote {args.out}: {len(scenarios)} scenarios')
    for n in notes:
        print(f'NOTE {n}', file=sys.stderr)


if __name__ == '__main__':
    sys.exit(main())
