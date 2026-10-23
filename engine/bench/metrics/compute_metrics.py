#!/usr/bin/env python3
"""compute_metrics.py - compute per (tool, scenario) metrics from real artifacts.

Inputs (all from the run directories):
  --manifest    seed-manifest.jsonl          (ground truth)
  --scenario    scenario-state.json          (deletion method record)
  --inventory   recovered-inventory.json     (from inventory_recovered.py)
  --outdir      the tool run dir             (scan stdout/log/tree/report + ours-run.json
                                              or og-run.json + og-found.csv)

Output: metrics.json in --outdir with recall/precision/F1/FP/preservation/perf blocks
and honest not-comparable notes. og free-cap normalization: recall/precision are
computed BOTH over the full ground-truth set and over the attempted (capped) set.

Ground truth rules:
  - recall denominator: entries with keep=false ("deletable"), except for
    quick_format / full_format / delete_partition where the whole seed set
    (both partitions for delete_partition) is the denominator.
  - keep-control: keep entries must be reported by tools as ACTIVE on non-format
    scenarios (recorded, informational; hard failure policy lives in compare.py).

Licensed under the MIT license. See LICENSE.
"""

import argparse
import csv
import glob
import hashlib
import json
import os
import re
import sys

FORMAT_SCENARIOS = {'quick_format', 'full_format'}
WHOLE_DISK_SCENARIOS = {'delete_partition'}


def load_manifest(path):
    meta, entries = None, []
    with open(path, 'r', encoding='utf-8-sig') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            o = json.loads(line)
            if o.get('_meta'):
                meta = o
            else:
                entries.append(o)
    return meta, entries


def load_json(path):
    if not path or not os.path.exists(path):
        return None
    try:
        with open(path, 'r', encoding='utf-8-sig') as f:
            return json.load(f)
    except Exception:
        return None


def read_tree_csv(path):
    """rrecov tree --csv rows: path,size,origin,deleted,confidence,extent (no header)."""
    rows = []
    if not path or not os.path.exists(path):
        return rows
    with open(path, 'r', encoding='utf-8-sig', errors='replace') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if not line:
                continue
            try:
                rows.append(next(csv.reader([line])))
            except StopIteration:
                continue
    return rows


def read_og_csv(path):
    """og found CSV from compare_og_rsf.py: path,size,origin,deleted,confidence,extent."""
    rows = []
    if not path or not os.path.exists(path):
        return rows
    with open(path, 'r', encoding='utf-8-sig', errors='replace') as f:
        for line in f:
            line = line.rstrip('\r\n')
            if not line or line.startswith('path,'):
                continue
            try:
                rows.append(next(csv.reader([line])))
            except StopIteration:
                continue
    return rows


def norm(p):
    return p.replace('\\', '/').strip().lstrip('/').lower()


def manifest_match(row_path, entries_by_norm, entries_by_namesize):
    p = norm(row_path)
    # tree paths are prefixed by the volume root role, e.g. 'Windows (E:)/seed/...'
    if p in entries_by_norm:
        return entries_by_norm[p], 'path'
    for tok in ('/seed/', '/seedb/', '/keep/'):
        idx = p.find(tok)
        if idx >= 0:
            sub = p[idx + len(tok):]
            if sub in entries_by_norm:
                return entries_by_norm[sub], 'path'
    # basename+size fallback handled by caller via size field
    return None, None


def parse_samples(path):
    """samples CSV: t_ms,working_set_bytes,cpu_seconds -> peak mem + cpu seconds."""
    peak_mem = 0
    cpu = 0.0
    last_t = 0
    if not path or not os.path.exists(path):
        return peak_mem, cpu
    with open(path, 'r', encoding='utf-8-sig') as f:
        for line in f:
            parts = line.strip().split(',')
            if len(parts) != 3 or parts[0] == 't_ms':
                continue
            try:
                t = float(parts[0]); ws = int(parts[1]); c = float(parts[2])
            except ValueError:
                continue
            peak_mem = max(peak_mem, ws)
            cpu = max(cpu, c)
            last_t = max(last_t, t)
    return peak_mem, cpu


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--scenario', required=True)
    ap.add_argument('--inventory', required=True)
    ap.add_argument('--outdir', required=True)
    ap.add_argument('--tool', choices=['ours', 'og'], required=True)
    ap.add_argument('--fs', default='')
    ap.add_argument('--mode', default='deepcarve', help='scan mode for the ours leg (tree-<mode>.csv / inventory of recovered-<mode>)')
    ap.add_argument('--outname', default='metrics.json')
    args = ap.parse_args()

    meta, entries = load_manifest(args.manifest)
    scen = load_json(args.scenario) or {}
    inv = load_json(args.inventory) or {}
    mode = scen.get('mode', os.path.basename(os.path.dirname(args.scenario)))

    entries_by_norm = {norm(e['rel']): e for e in entries}
    entries_by_namesize = {}
    for e in entries:
        entries_by_namesize.setdefault((e['name'].lower(), int(e['size'])), []).append(e)

    scenario = mode
    if scenario in FORMAT_SCENARIOS or scenario in WHOLE_DISK_SCENARIOS:
        gt = entries          # everything was lost
    else:
        gt = [e for e in entries if not e.get('keep')]
    keep_gt = [e for e in entries if e.get('keep')]

    gt_by_norm = {norm(e['rel']): e for e in gt}

    # ---- recovered (hash-verified) accounting from the inventory ----
    rec_records = inv.get('records', [])
    recovered = {}   # norm rel -> record (hash match only)
    for r in rec_records:
        if r.get('match_level') and r.get('hash_match') and not r.get('keep'):
            recovered[norm(r['manifest_rel'])] = r
    recovered_keep = {}
    for r in rec_records:
        if r.get('match_level') and r.get('hash_match') and r.get('keep'):
            recovered_keep[norm(r['manifest_rel'])] = r

    # ---- found (tool-reported) accounting ----
    found_rows = []
    found_tool = None
    if args.tool == 'ours':
        p = os.path.join(args.outdir, f'tree-{args.mode}.csv')
        found_rows = read_tree_csv(p)
        found_tool = args.mode
    else:
        p = os.path.join(args.outdir, 'og-found.csv')
        found_rows = read_og_csv(p)
        found_tool = 'rsf'

    found_matched = 0
    found_deleted_matched = 0
    for row in found_rows:
        if len(row) < 2:
            continue
        path = row[0]
        m, lvl = manifest_match(path, entries_by_norm, entries_by_namesize)
        if m:
            found_matched += 1
            if not m.get('keep'):
                found_deleted_matched += 1

    # ---- og cap accounting ----
    cap = None
    if args.tool == 'og':
        ogrun = load_json(os.path.join(args.outdir, 'og-run.json')) or {}
        quota = load_json(os.path.join(args.outdir, 'og-quota.json')) or {}
        cap = {
            'quota_bytes': 524288000,
            'recovered_total_size_before': quota.get('recovered_total_size_before'),
            'recovered_total_size_after': quota.get('recovered_total_size_after'),
            'recovered_total_file_count_after': quota.get('recovered_total_file_count_after'),
            'cap_gate_hit': ogrun.get('cap_gate', {}).get('gate_seen', False),
            'recovered_bytes_this_run': None,
        }
        if quota.get('recovered_total_size_after') is not None and quota.get('recovered_total_size_before') is not None:
            cap['recovered_bytes_this_run'] = quota['recovered_total_size_after'] - quota['recovered_total_size_before']

    # ---- timing / perf ----
    perf = {}
    if args.tool == 'ours':
        runmeta = load_json(os.path.join(args.outdir, 'ours-run.json')) or {}
        for m in runmeta.get('scan_modes', []):
            samples = os.path.join(args.outdir, f'samples-{m.get("mode")}.csv')
            peak_mem, cpu = parse_samples(samples)
            rsamples = os.path.join(args.outdir, f'recover-samples-{m.get("mode")}.csv')
            rpeak, rcpu = parse_samples(rsamples)
            summ = m.get('summary') or {}
            perf[m.get('mode')] = {
                'scan_wall_seconds': m.get('wall_seconds'),
                'scan_capped': m.get('capped'),
                'scan_exit': m.get('exit_code'),
                'bytes_scanned': summ.get('bytes_scanned'),
                'files_found': summ.get('files'),
                'deleted_found': summ.get('deleted'),
                'carved_found': summ.get('carved'),
                'lost_partitions': summ.get('lost_partitions'),
                'peak_working_set_bytes': peak_mem,
                'cpu_seconds': cpu,
                'recover_wall_seconds': (m.get('recover') or {}).get('wall_seconds'),
                'recover_exit': (m.get('recover') or {}).get('exit_code'),
                'recover_peak_working_set_bytes': rpeak,
                'transport': m.get('transport_lines'),
            }
    else:
        ogrun = load_json(os.path.join(args.outdir, 'og-run.json')) or {}
        s = ogrun.get('scan_phase') or {}
        r = ogrun.get('recover_phase') or {}
        samples = os.path.join(args.outdir, 'og-samples.csv')
        peak_mem, cpu = parse_samples(samples)
        perf['default_full'] = {
            'scan_wall_seconds': s.get('wall_seconds'),
            'scan_timeout_hit': s.get('timeout'),
            'peak_working_set_bytes': peak_mem,
            'cpu_seconds': cpu,
            'recover_wall_seconds': r.get('wall_seconds'),
            'found_files': None,
        }

    # ---- preservation ----
    pres = load_json(os.path.join(args.outdir, 'preservation.json')) or ({} if args.tool == 'og' else {})
    if args.tool == 'og':
        pres = load_json(os.path.join(args.outdir, 'preservation.json')) or {}

    ts_pres = 0
    ts_tot = 0
    for r in rec_records:
        if r.get('match_level') == 'path' and r.get('hash_match'):
            m = entries_by_norm.get(norm(r['manifest_rel']))
            if m and m.get('ts_write'):
                ts_tot += 1
                try:
                    import datetime
                    want = datetime.datetime.fromisoformat(m['ts_write']).timestamp()
                    if abs(r['mtime'] - want) <= 2.0:
                        ts_pres += 1
                except Exception:
                    pass

    # ---- metric core ----
    n_gt = len(gt)
    n_rec = len([k for k in recovered if k in gt_by_norm])
    attempted = gt
    n_attempt = n_gt
    if args.tool == 'og' and cap and cap.get('cap_gate_hit'):
        # normalize over the attempted subset: whatever got recovered (hash ok)
        # plus the not-attempted remainder is excluded from the denominator
        attempted = [e for e in gt if norm(e['rel']) in recovered] or gt
        n_attempt = len(attempted)

    reported_recovered_files = inv.get('files_seen', 0)
    n_hash_ok = len([r for r in rec_records if r.get('match_level') and r.get('hash_match')])

    precision_den = max(1, reported_recovered_files)
    precision = n_hash_ok / precision_den
    if reported_recovered_files == 0 and not inv.get('false_positives', 0):
        precision = None  # nothing claimed, nothing wrong - not a 0% score
    recall = n_rec / max(1, n_gt)
    recall_attempted = n_rec / max(1, n_attempt) if n_attempt != n_gt else recall
    def _f1(p, r):
        if p is None:
            p = 1.0  # no-claims convention: neutral precision for F1
        return (2 * p * r / (p + r)) if (p + r) > 0 else 0.0
    f1 = _f1(precision, recall)
    f1a = _f1(precision, recall_attempted)

    # per-class breakdowns
    def class_recall(cls):
        sub = [e for e in gt if e.get('class') == cls]
        if not sub:
            return None
        hit = sum(1 for e in sub if norm(e['rel']) in recovered)
        return {'total': len(sub), 'recovered': hit, 'recall': round(hit / len(sub), 4)}

    classes = {}
    for e in gt:
        classes.setdefault(e.get('class'), 0)
        classes[e.get('class')] += 1
    per_class = {c: class_recall(c) for c in classes}

    size_weighted = None
    try:
        tot_bytes = sum(int(e['size']) for e in gt)
        rec_bytes = sum(int(e['size']) for e in gt if norm(e['rel']) in recovered)
        size_weighted = {'total_bytes': tot_bytes, 'recovered_bytes': rec_bytes,
                         'recall_bytes': round(rec_bytes / max(1, tot_bytes), 4)}
    except Exception:
        pass

    keep_control = None
    if keep_gt and scenario not in FORMAT_SCENARIOS:
        keep_ok = sum(1 for e in keep_gt if norm(e['rel']) in recovered_keep)
        keep_control = {'total': len(keep_gt), 'recovered_hash_ok': keep_ok,
                        'ok': keep_ok == len(keep_gt)}

    m = {
        'tool': args.tool, 'scenario': scenario, 'fs': args.fs or (scen.get('filesystem') or ''),
        'ground_truth_files': n_gt,
        'recovered_hash_ok': n_rec,
        'recall': round(recall, 4),
        'recall_attempted_set': round(recall_attempted, 4),
        'precision': None if precision is None else round(precision, 4),
        'f1': round(f1, 4),
        'f1_attempted_set': round(f1a, 4),
        'found_rows': len(found_rows),
        'found_matched_ground_truth': found_matched,
        'found_deleted_matched': found_deleted_matched,
        'reported_recovered_files': reported_recovered_files,
        'false_positives': inv.get('false_positives', 0),
        'fp_class': inv.get('fp_class', {}),
        'path_preservation': round((inv.get('matched_path', 0)) / max(1, inv.get('matched', 0)), 4) if inv.get('matched') else None,
        'timestamp_preservation': round(ts_pres / ts_tot, 4) if ts_tot else None,
        'timestamp_preservation_n': [ts_pres, ts_tot],
        'keep_control': keep_control,
        'per_class': per_class,
        'size_weighted': size_weighted,
        'og_cap': cap,
        'perf': perf,
        'preservation': pres,
        'notes': [],
    }

    if scenario == 'mft_corrupt' and args.fs and args.fs.upper() != 'NTFS':
        m['notes'].append('NOT-COMPARABLE: mft_corrupt is NTFS-only')
    if args.tool == 'og':
        ogrun = load_json(os.path.join(args.outdir, 'og-run.json')) or {}
        m['og_run_notes'] = ogrun.get('notes', [])
        m['attempts'] = ogrun.get('attempts', [])
        m['scan_mode_note'] = 'og free exposes a single scan mode (quick pass auto-continues to deep); ' \
                              'per-mode granularity is NOT-COMPARABLE'

    m['mode'] = args.mode if args.tool == 'ours' else 'default_full'
    outp = os.path.join(args.outdir, args.outname)
    with open(outp, 'w', encoding='utf-8') as f:
        json.dump(m, f, indent=1)
    print(f"metrics[{args.tool}/{scenario}]: recall={m['recall']} precision={m['precision']} "
          f"f1={m['f1']} fp={m['false_positives']} -> {outp}")


if __name__ == '__main__':
    sys.exit(main())
