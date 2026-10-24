#!/usr/bin/env python3
"""compare.py - build docs/COMPARATIVE-BENCHMARK.md + benchmark-summary.json from
real per-scenario metrics of both legs.

Verdict policy (per metric, ours vs og):
  MATCH           both present, |delta| <= tolerance
  EXCEEDS         ours strictly better beyond tolerance
  BELOW           ours strictly worse beyond tolerance (root-cause placeholder + fix plan)
  NOT-COMPARABLE  a leg is missing/failed/capped/unsupported - reason recorded verbatim

Nothing is hidden: BELOW rows are findings to fix, NOT-COMPARABLE rows carry the
precise reason, and any scenario where a leg crashed/hung is listed with its
artifact reference.

Licensed under the MIT license. See LICENSE.
"""

import argparse
import glob
import json
import os
import sys

TOL = 0.02  # 2% absolute tolerance for MATCH on ratio metrics

# Evidence-based root-cause register (bench/root-causes.json) - forensics
# output, cited by run+artifact; merged into the BELOW register. Absent
# entries fall back to honest TODO placeholders.
ROOT_CAUSES = {}
try:
    _rcp = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'root-causes.json')
    with open(_rcp, 'r', encoding='utf-8-sig') as _f:
        ROOT_CAUSES = json.load(_f)
except Exception:
    pass


def load_json(path):
    if not path or not os.path.exists(path):
        return None
    try:
        with open(path, 'r', encoding='utf-8-sig') as f:
            return json.load(f)
    except Exception as e:
        return {'_error': str(e)}


def collect(root):
    """{scenario: {'metrics':..., 'run':..., 'extra_paths':[...]}} for a leg root."""
    out = {}
    if not root or not os.path.isdir(root):
        return out
    for d in sorted(glob.glob(os.path.join(root, '*'))):
        if not os.path.isdir(d):
            continue
        scen = os.path.basename(d)
        # ours legs carry per-scan-mode metrics files; the headline is deepcarve
        m = (load_json(os.path.join(d, 'metrics-deepcarve.json'))
             or load_json(os.path.join(d, 'metrics.json')))
        modes = {}
        for mp in sorted(glob.glob(os.path.join(d, 'metrics-*.json'))):
            mm = load_json(mp)
            if mm:
                modes[mm.get('mode', os.path.basename(mp))] = mm
        run = load_json(os.path.join(d, 'ours-run.json')) or load_json(os.path.join(d, 'og-run.json'))
        out[scen] = {
            'metrics': m,
            'modes': modes,
            'run': run,
            'state': load_json(os.path.join(d, 'scenario-state.json')),
            'dir': d,
        }
    return out


def verdict(ours_v, og_v, tol=TOL, higher_is_better=True):
    if ours_v is None or og_v is None:
        return 'NOT-COMPARABLE'
    if og_v == 0 and ours_v == 0:
        return 'MATCH'
    delta = ours_v - og_v
    if abs(delta) <= tol:
        return 'MATCH'
    better = delta > 0 if higher_is_better else delta < 0
    return 'EXCEEDS' if better else 'BELOW'


def fmt(v, pct=True):
    if v is None:
        return 'n/a'
    if pct:
        return f'{v * 100:.1f}%'
    return f'{v}'


def metric_rows(scen, o, g):
    """rows: (metric, ours, og, verdict)"""
    om, gm = (o or {}).get('metrics'), (g or {}).get('metrics')
    rows = []
    for key, hib in [('recall', True), ('precision', True), ('f1', True),
                     ('path_preservation', True), ('timestamp_preservation', True)]:
        ov = (om or {}).get(key)
        gv = (gm or {}).get(key)
        v = verdict(ov, gv, hib=hib)
        # cap normalization: og recall_attempted_set is the fair number when the cap hit
        cap_note = ''
        if key == 'recall' and gm and (gm.get('og_cap') or {}).get('cap_gate_hit'):
            gv = gm.get('recall_attempted_set', gv)
            cap_note = ' (og normalized to attempted set: cap gate hit)'
            v = verdict(ov, gv, hib=hib)
        rows.append((key, ov, gv, v, cap_note))
    return rows


def scan_time(leg):
    m = (leg or {}).get('metrics') or {}
    perf = m.get('perf') or {}
    if args_tool_is_ours(m):
        v = None
        for mode in ('deepcarve', 'deep', 'quick'):
            if mode in perf and perf[mode].get('scan_wall_seconds') is not None:
                v = perf[mode]['scan_wall_seconds']
                break
        return v
    p = perf.get('default_full') or {}
    return p.get('scan_wall_seconds')


def args_tool_is_ours(m):
    return (m or {}).get('tool') == 'ours'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--ours-root', required=True, help='dir with per-scenario ours results')
    ap.add_argument('--og-root', required=True, help='dir with per-scenario og results')
    ap.add_argument('--out-md', default='docs/COMPARATIVE-BENCHMARK.md')
    ap.add_argument('--out-json', default='docs/benchmark-summary.json')
    ap.add_argument('--run-id', default='')
    ap.add_argument('--ours-missing-reason', default='')
    ap.add_argument('--og-missing-reason', default='')
    global args
    args = ap.parse_args()

    ours = collect(args.ours_root)
    og = collect(args.og_root)
    scenarios = sorted(set(ours) | set(og))

    summary = {
        'run_id': args.run_id,
        'generated_utc': __import__('datetime').datetime.utcnow().isoformat() + 'Z',
        'scenarios': {},
        'aggregate': {},
        'decay': {},
        'cap_impact': {},
        'incidents': [],
        'missing_legs': {
            'ours': args.ours_missing_reason,
            'og': args.og_missing_reason,
        },
    }
    md = []
    md.append('# Comparative Benchmark: core_recovery (rrecov + rda-driver) vs EaseUS DRW free (og)')
    md.append('')
    md.append(f'> Generated from CI artifacts only (run `{args.run_id or 'unspecified'}`).')
    md.append('> Ground truth = seed manifest + recorded deletion method. Verdicts: '
              'MATCH / EXCEEDS / BELOW / NOT-COMPARABLE. BELOW rows are fix-priority findings.')
    md.append('')

    # ---- per-scenario table ----
    md.append('## Per-scenario results')
    md.append('')
    md.append('| scenario | tool | recall | precision | F1 | FP | found | recovered | keep-ctrl | verdict(recall) |')
    md.append('|---|---|---|---|---|---|---|---|---|---|')
    agg_ours = {'n': 0, 'rec': 0.0, 'prec': 0.0, 'bytes': 0, 'bytes_rec': 0}
    agg_og = {'n': 0, 'rec': 0.0, 'prec': 0.0, 'bytes': 0, 'bytes_rec': 0}
    below_findings = []
    for scen in scenarios:
        o, g = ours.get(scen), og.get(scen)
        om = (o or {}).get('metrics')
        gm = (g or {}).get('metrics')
        fs = (om or gm or {}).get('fs', '')
        entry = {'fs': fs}
        modes = (o or {}).get('modes') or {}
        if modes:
            entry['ours_per_mode_recall'] = {k: v.get('recall') for k, v in modes.items()}
        for name, leg, m in (('ours', o, om), ('og', g, gm)):
            if not leg:
                entry[name] = {'status': 'MISSING'}
                continue
            if not m:
                entry[name] = {'status': 'LEG-FAILED', 'artifact': leg['dir']}
                summary['incidents'].append({
                    'scenario': scen, 'tool': name, 'kind': 'leg-failed',
                    'artifact': leg['dir'],
                    'detail': 'metrics.json absent - the leg crashed or was aborted; see run logs in the artifact',
                })
                continue
            entry[name] = m
            if name == 'ours':
                agg_ours['n'] += m.get('ground_truth_files', 0) or 0
                agg_ours['rec'] += (m.get('recall', 0) or 0) * (m.get('ground_truth_files', 0) or 0)
                agg_ours['prec'] += (m.get('precision', 0) or 0) * (m.get('reported_recovered_files', 0) or 0)
                sw = m.get('size_weighted') or {}
                agg_ours['bytes'] += sw.get('total_bytes', 0) or 0
                agg_ours['bytes_rec'] += sw.get('recovered_bytes', 0) or 0
            else:
                gv = m.get('recall_attempted_set', m.get('recall', 0))
                agg_og['n'] += m.get('ground_truth_files', 0) or 0
                agg_og['rec'] += (gv or 0) * (m.get('ground_truth_files', 0) or 0)
                agg_og['prec'] += (m.get('precision', 0) or 0) * (m.get('reported_recovered_files', 0) or 0)
                sw = m.get('size_weighted') or {}
                agg_og['bytes'] += sw.get('total_bytes', 0) or 0
                agg_og['bytes_rec'] += sw.get('recovered_bytes', 0) or 0

            def kv(k, pct=True):
                v = m.get(k)
                return fmt(v, pct) if v is not None else 'n/a'
            keep = m.get('keep_control')
            keep_s = 'n/a' if not keep else ('OK' if keep.get('ok') else f"FAIL {keep.get('recovered_hash_ok')}/{keep.get('total')}")
            md.append(f"| {scen} ({fs}) | {name} | {kv('recall')} | {kv('precision')} | {kv('f1')} | "
                      f"{m.get('false_positives', 'n/a')} | {m.get('found_rows', 'n/a')} | "
                      f"{m.get('recovered_hash_ok', 'n/a')}/{m.get('ground_truth_files', 'n/a')} | {keep_s} | - |")
            # scan-time verdict
            ot = scan_time(o) if name == 'ours' else None
            gt_ = scan_time(g) if name == 'og' else None
            if name == 'og' and ot is None:
                ot = scan_time(ours.get(scen))
        # verdict line
        if om and gm:
            og_recall = gm.get('recall_attempted_set') if (gm.get('og_cap') or {}).get('cap_gate_hit') else gm.get('recall')
            v = verdict(om.get('recall'), og_recall)
            capn = ' *(og cap-normalized)*' if og_recall != gm.get('recall') else ''
            md.append(f"| {scen} ({fs}) | **verdict** | {fmt(om.get('recall'))} vs {fmt(og_recall)}{capn} | "
                      f"{fmt(om.get('precision'))} vs {fmt(gm.get('precision'))} | "
                      f"{fmt(om.get('f1'))} vs {fmt(gm.get('f1'))} | - | - | - | **{v}** |")
            if v == 'BELOW':
                rc = ROOT_CAUSES.get(scen) or {}
                below_findings.append({
                    'scenario': scen, 'metric': 'recall',
                    'ours': om.get('recall'), 'og': og_recall,
                    'root_cause_hypothesis': rc.get('root_cause',
                        'TODO - engine-level investigation next session'),
                    'fix_plan': rc.get('fix_plan',
                        'register in docs/CORE-GAP-LIST.md and fix in a follow-up session'),
                    'evidence': rc.get('evidence', ''),
                })
        summary['scenarios'][scen] = entry

    # ---- ours per-scan-mode recall table ----
    md.append('')
    md.append('## ours per-scan-mode recall (quick / deep / deep+carve)')
    md.append('')
    md.append('| scenario | quick | deep | deep+carve |')
    md.append('|---|---|---|---|')
    for scen in scenarios:
        modes = (ours.get(scen) or {}).get('modes') or {}
        if not modes:
            continue
        q = fmt(modes.get('quick', {}).get('recall'))
        dd = fmt(modes.get('deep', {}).get('recall'))
        dc = fmt(modes.get('deepcarve', {}).get('recall'))
        md.append(f'| {scen} | {q} | {dd} | {dc} |')
    md.append('')

    # ---- aggregate ----
    def agg(a):
        if not a['n']:
            return None
        return {
            'count_weighted_recall': round(a['rec'] / a['n'], 4),
            'count_weighted_precision': round(a['prec'] / max(1, a['n']), 4),
            'byte_weighted_recall': round(a['bytes_rec'] / max(1, a['bytes']), 4),
        }
    summary['aggregate']['ours'] = agg(agg_ours)
    summary['aggregate']['og'] = agg(agg_og)
    md.append('')
    md.append('## Aggregate')
    md.append('')
    md.append('| tool | recall (count-weighted) | precision (count-weighted) | recall (byte-weighted) |')
    md.append('|---|---|---|---|')
    for name, a in (('ours', summary['aggregate']['ours']), ('og', summary['aggregate']['og'])):
        if a:
            md.append(f"| {name} | {fmt(a['count_weighted_recall'])} | {fmt(a['count_weighted_precision'])} | {fmt(a['byte_weighted_recall'])} |")
        else:
            md.append(f'| {name} | n/a | n/a | n/a |')

    # ---- decay curve ----
    md.append('')
    md.append('## Real-world decay (recall vs volume overwrite)')
    md.append('')
    md.append('| tool | decay_50 | decay_80 | decay_100 |')
    md.append('|---|---|---|---|')
    decay = {'ours': {}, 'og': {}}
    for name, legmap in (('ours', ours), ('og', og)):
        vals = []
        for pct in ('50', '80', '100'):
            m = (legmap.get(f'decay_{pct}') or {}).get('metrics')
            v = fmt(m.get('recall')) if m else 'n/a'
            decay[name][f'decay_{pct}'] = m.get('recall') if m else None
            vals.append(v)
        md.append(f"| {name} | {vals[0]} | {vals[1]} | {vals[2]} |")
    summary['decay'] = decay

    # ---- cap impact ----
    md.append('')
    md.append('## og free-cap impact (500 MB)')
    md.append('')
    cap_rows = []
    for scen in scenarios:
        gm = (og.get(scen) or {}).get('metrics')
        om = (ours.get(scen) or {}).get('metrics')
        if gm and (gm.get('og_cap') or {}).get('cap_gate_hit'):
            cap_rows.append((scen, gm.get('recall'), gm.get('recall_attempted_set'),
                             (gm.get('og_cap') or {}).get('recovered_bytes_this_run'),
                             om.get('recall') if om else None))
    if cap_rows:
        md.append('| scenario | og recall (raw) | og recall (attempted set) | og recovered bytes this run | ours recall (uncapped) |')
        md.append('|---|---|---|---|---|')
        for r in cap_rows:
            md.append(f'| {r[0]} | {fmt(r[1])} | {fmt(r[2])} | {r[3]} | {fmt(r[4])} |')
        md.append('')
        md.append('Cap-loss = algorithmic-neutral subset: the gap between og raw recall and ours '
                  'recall on the same scenario decomposes into (a) files og never attempted '
                  '(free cap) and (b) algorithmic difference. The attempted-set column isolates (b).')
    else:
        md.append('_No scenario hit the og 500 MB cap in this run (or the og leg is missing)._')
    summary['cap_impact'] = {'scenarios_with_cap_gate': [r[0] for r in cap_rows]}

    # ---- incidents ----
    md.append('')
    md.append('## Crashes / hangs / capped runs')
    md.append('')
    if not summary['incidents']:
        md.append('_None recorded._')
    else:
        for inc in summary['incidents']:
            md.append(f"- **{inc['scenario']} / {inc['tool']}**: {inc['kind']} - {inc.get('detail', '')} "
                      f"(artifact: `{inc.get('artifact', '')}`)")
    # capped scans (ours)
    for scen, leg in ours.items():
        for mode_info in ((leg.get('metrics') or {}).get('perf') or {}).values() if leg.get('metrics') else []:
            if isinstance(mode_info, dict) and mode_info.get('scan_capped'):
                summary['incidents'].append({'scenario': scen, 'tool': 'ours', 'kind': 'scan-capped',
                                             'detail': f"mode hit the wall-clock cap ({mode_info.get('scan_wall_seconds')}s)"})
                md.append(f'- **{scen} / ours**: scan-capped at {mode_info.get("scan_wall_seconds")}s '
                          f'(partial state preserved, see session + log artifacts)')

    # ---- BELOW register ----
    md.append('')
    md.append('## BELOW findings (fix priorities for the next session)')
    md.append('')
    if below_findings:
        for b in below_findings:
            md.append(f"- `{b['scenario']}` recall: ours {fmt(b['ours'])} vs og {fmt(b['og'])} - "
                      f"root cause: {b['root_cause_hypothesis']} - plan: {b['fix_plan']}")
            if b.get('evidence'):
                md.append(f"  - evidence: {b['evidence']}")
    else:
        md.append('_No BELOW findings in this run._')
    summary['below_findings'] = below_findings

    # ---- forensic notes (root-causes entries for scenarios present here, even when not BELOW) ----
    md.append('')
    md.append('## Forensic notes (carried from bench/root-causes.json)')
    md.append('')
    notes_shown = 0
    for scen in scenarios:
        rc = ROOT_CAUSES.get(scen) or {}
        for note_key in ('root_cause', 'ours_precision_note', 'caveat'):
            if rc.get(note_key):
                md.append(f"- `{scen}` ({note_key}): {rc[note_key]}")
                if rc.get('evidence') and note_key != 'root_cause':
                    md.append(f"  - evidence: {rc['evidence']}")
                notes_shown += 1
    if not notes_shown:
        md.append('_None._')

    # ---- NOT-COMPARABLE register ----
    md.append('')
    md.append('## NOT-COMPARABLE register')
    md.append('')
    nc = []
    for scen in scenarios:
        om = (ours.get(scen) or {}).get('metrics')
        gm = (og.get(scen) or {}).get('metrics')
        for name, m in (('ours', om), ('og', gm)):
            if not m:
                nc.append(f'- `{scen}` / {name}: metrics missing (leg failed or not requested '
                          f'- see missing_legs reasons and incidents)')
        if om:
            for note in om.get('notes', []):
                if 'NOT-COMPARABLE' in note:
                    nc.append(f'- `{scen}` / ours: {note}')
        if gm:
            for note in (gm.get('notes', []) or []):
                if 'NOT-COMPARABLE' in note or 'NOT-COMPARABLE' in (gm.get('scan_mode_note') or ''):
                    nc.append(f'- `{scen}` / og: {note}')
    if nc:
        md.extend(nc)
    else:
        md.append('_None._')
    summary['not_comparable'] = nc

    os.makedirs(os.path.dirname(os.path.abspath(args.out_md)), exist_ok=True)
    with open(args.out_md, 'w', encoding='utf-8') as f:
        f.write('\n'.join(md) + '\n')
    os.makedirs(os.path.dirname(os.path.abspath(args.out_json)), exist_ok=True)
    with open(args.out_json, 'w', encoding='utf-8') as f:
        json.dump(summary, f, indent=1)
    print(f'wrote {args.out_md} and {args.out_json}')
    print(f"scenarios: {len(scenarios)}  BELOW findings: {len(below_findings)}  incidents: {len(summary['incidents'])}")


if __name__ == '__main__':
    sys.exit(main())
