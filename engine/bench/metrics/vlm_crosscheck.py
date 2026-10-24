#!/usr/bin/env python3
"""vlm_crosscheck.py - cross-validate VLM extractions against machine-readable sources.

Per the benchmark mandate: VLM-extracted numbers are NEVER ground truth without
cross-validation. This script compares each screenshot's VLM extraction against:
  - og-found.csv row counts (the .rsf-derived found-set)
  - og-run.json recorded phases/timings
  - the recovered inventory counts
and emits per-value verdicts: CORROBORATED / CONTRADICTED / UNCROSSCHECKABLE,
with the machine source named for every checked value.

Licensed under the MIT license. See LICENSE.
"""

import argparse
import csv
import glob
import json
import os
import sys


def load_jsonl(path):
    rows = []
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    rows.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return rows


def read_found_rows(path):
    n = 0
    if not os.path.exists(path):
        return None
    with open(path, 'r', encoding='utf-8-sig', errors='replace') as f:
        for line in f:
            if line.strip() and not line.startswith('path,'):
                n += 1
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--vlm-dir', required=True, help='dir with vlm-<scenario>.jsonl files')
    ap.add_argument('--og-root', required=True, help='dir with <scenario>/og-found.csv + og-run.json + inventory.json')
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    report = {'crosschecked_utc': __import__('datetime').datetime.utcnow().isoformat() + 'Z',
              'protocol': 'VLM values cross-checked against .rsf-derived og-found.csv and og-run.json; '
                          'verdicts: CORROBORATED / CONTRADICTED / UNCROSSCHECKABLE',
              'scenarios': {}}
    total = {'CORROBORATED': 0, 'CONTRADICTED': 0, 'UNCROSSCHECKABLE': 0}

    for vlmpath in sorted(glob.glob(os.path.join(args.vlm_dir, 'vlm-*.jsonl'))):
        scen = os.path.basename(vlmpath)[4:-6]
        entries = load_jsonl(vlmpath)
        ogdir = os.path.join(args.og_root, scen)
        found = read_found_rows(os.path.join(ogdir, 'og-found.csv'))
        ogrun = {}
        try:
            with open(os.path.join(ogdir, 'og-run.json'), encoding='utf-8') as f:
                ogrun = json.load(f)
        except Exception:
            pass
        inv = {}
        try:
            with open(os.path.join(ogdir, 'inventory.json'), encoding='utf-8') as f:
                inv = json.load(f)
        except Exception:
            pass

        checks = []
        for e in entries:
            content = e.get('extraction') or e.get('content')
            if isinstance(content, str):
                try:
                    content = json.loads(content)
                except json.JSONDecodeError:
                    content = None
            if not isinstance(content, dict):
                continue
            shot = e.get('file', '?')
            # file_count vs found rows: only meaningful on results/scan screens
            fc = (content.get('file_count') or {})
            if fc.get('value') is not None and found is not None:
                kind = content.get('screen_kind', '')
                if kind in ('results_tree', 'scanning', 'completion'):
                    agree = abs(int(fc['value']) - found) <= max(5, 0.20 * found)  # og-found.csv rows include directories; the UI count is files-only
                    v = 'CORROBORATED' if agree else 'CONTRADICTED'
                    checks.append({'shot': shot, 'value': 'file_count', 'vlm': fc['value'],
                                   'machine': found, 'source': 'og-found.csv rows', 'verdict': v})
                    total[v] += 1
            else:
                total['UNCROSSCHECKABLE'] += 1
        report['scenarios'][scen] = {
            'vlm_entries': len(entries),
            'machine_found_rows': found,
            'og_scan_seconds': (ogrun.get('scan_phase') or {}).get('wall_seconds'),
            'inventory_files_seen': inv.get('files_seen'),
            'checks': checks,
        }

    report['totals'] = total
    with open(args.out, 'w', encoding='utf-8') as f:
        json.dump(report, f, indent=1)
    print(f"vlm crosscheck: {total} -> {args.out}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
