#!/usr/bin/env python3
"""merge_runs.py - build the FINAL comparative report from two runs' artifacts:
the ours legs of run A (full matrix, ours-green) + the og legs of run B
(the og catch-up run on the fully-fixed head).

Usage:
  python bench/metrics/merge_runs.py \
      --ours-src dl-A --og-src dl-B \
      --out-md docs/COMPARATIVE-BENCHMARK.md \
      --out-json docs/benchmark-summary.json \
      --run-id "<runA>+<runB>" [--ours-missing-reason ...] [--og-missing-reason ...]

Lays both sources out into merged/ours/<scenario>/ + merged/og/<scenario>/
(using each artifact dir's metrics files), then delegates to compare.py's
collector logic by importing it (single source of truth for verdicts).
"""

import argparse
import glob
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import compare as cmp_mod  # noqa: E402


def collect_leg(src, tool, out_root):
    """Copy <src>/bench-<tool>-<fs>-<scenario>/benchmark-results -> out_root/<scenario>/."""
    n = 0
    for d in sorted(glob.glob(os.path.join(src, '*'))):
        base = os.path.basename(d)
        prefix = f'bench-{tool}-'
        if not base.startswith(prefix):
            continue
        scen = base[len(prefix):]
        # strip the fs prefix: <fs>-<scenario>
        scen = scen.split('-', 1)[1] if '-' in scen else scen
        br = os.path.join(d, 'benchmark-results')
        if not os.path.isdir(br):
            continue
        dst = os.path.join(out_root, scen)
        os.makedirs(dst, exist_ok=True)
        for f in glob.glob(os.path.join(br, '*')):
            shutil.copy2(f, os.path.join(dst, os.path.basename(f)))
        # provenance files at the artifact root
        for f in glob.glob(os.path.join(d, '*')):
            b = os.path.basename(f)
            if os.path.isfile(f) and (b.startswith('driver-') or b in ('og-version.txt', 'og-pe.json', 'rrecov-sha.txt')):
                shutil.copy2(f, os.path.join(dst, b))
        n += 1
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--ours-src', required=True, help='dir with bench-ours-* artifacts (run A)')
    ap.add_argument('--og-src', required=True, help='dir with bench-og-* artifacts (run B)')
    ap.add_argument('--workdir', default='merged-runs')
    ap.add_argument('--out-md', default='docs/COMPARATIVE-BENCHMARK.md')
    ap.add_argument('--out-json', default='docs/benchmark-summary.json')
    ap.add_argument('--run-id', default='')
    ap.add_argument('--ours-missing-reason', default='')
    ap.add_argument('--og-missing-reason', default='')
    args = ap.parse_args()

    ours_root = os.path.join(args.workdir, 'ours')
    og_root = os.path.join(args.workdir, 'og')
    n_ours = collect_leg(args.ours_src, 'ours', ours_root)
    n_og = collect_leg(args.og_src, 'og', og_root)
    print(f'collected: ours={n_ours} scenarios, og={n_og} scenarios')

    # delegate to compare.py's main with argv
    sys.argv = [
        'compare.py',
        '--ours-root', ours_root,
        '--og-root', og_root,
        '--out-md', args.out_md,
        '--out-json', args.out_json,
        '--run-id', args.run_id,
        '--ours-missing-reason', args.ours_missing_reason,
        '--og-missing-reason', args.og_missing_reason,
    ]
    cmp_mod.main()


if __name__ == '__main__':
    sys.exit(main())
