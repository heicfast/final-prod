#!/usr/bin/env python3
"""inventory_recovered.py - hash-walk recovered output trees and match to the seed manifest.

Inputs: one or more recovered roots, the seed manifest (jsonl), optional tool report.
Output: recovered-inventory.json with per-file records, match levels, false positives.

Match levels (strictest first):
  path    - recovered relative path equals the manifest rel path (after stripping
            common recovery-junk prefixes like 'seed/' or a tool-added root)
  namesize- basename + exact size
  name    - basename only (flagged; counted separately, never as path-preserved)

Ground truth is ONLY the manifest. Anything not matched is a false positive and is
classified (carveable-extension plausible fragment vs noise vs tool-internal file).

Licensed under the MIT license. See LICENSE.
"""

import argparse
import hashlib
import json
import os
import sys

CARVEABLE_EXT = {
    '.jpg', '.jpeg', '.png', '.heic', '.tif', '.tiff', '.cr2', '.nef', '.arw', '.dng',
    '.mp4', '.mov', '.avi', '.mkv', '.pdf', '.docx', '.xlsx', '.pptx', '.zip', '.7z',
    '.rar', '.sqlite', '.db', '.pst', '.txt',
}
TOOL_INTERNAL = {
    'recoverylog.txt', 'recovery log.txt', 'log.txt', 'readme.txt',
}


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def load_manifest(path):
    meta = None
    entries = []
    with open(path, 'r', encoding='utf-8-sig') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            obj = json.loads(line)
            if obj.get('_meta'):
                meta = obj
            else:
                entries.append(obj)
    return meta, entries


def normalize_path(p):
    p = p.replace('\\', '/').strip()
    while p.startswith('/'):
        p = p[1:]
    return p.lower()


def strip_junk_prefixes(p):
    """Strip common tool-added path prefixes so 'seed/...' can match."""
    parts = p.split('/')
    # drop leading components until we find one that matches a manifest top dir
    tops = ('seed', 'seedb', 'keep', 'small', 'tiny', 'media', 'big', 'manysmall', 'nested',
            'longpath', 'unicode', 'frag', 'fraghigh', 'ads', 'hardlinks', 'sparse',
            'doomed', 'reconstructed', 'file path lost', 'lost', 'others')
    for i, seg in enumerate(parts):
        if seg.lower() in tops:
            return '/'.join(parts[i:])
    return '/'.join(parts)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', action='append', required=True, help='recovered output root (repeatable)')
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--label', default='recovered')
    ap.add_argument('--tool', default='ours')
    args = ap.parse_args()

    meta, entries = load_manifest(args.manifest)
    by_path = {}
    by_namesize = {}
    by_name = {}
    by_hash = {}
    for e in entries:
        p = normalize_path(e['rel'])
        by_path[p] = e
        by_namesize.setdefault((e['name'].lower(), int(e['size'])), []).append(e)
        by_name.setdefault(e['name'].lower(), []).append(e)
        by_hash.setdefault(e['sha256'], []).append(e)

    records = []
    total_files = 0
    total_bytes = 0
    for root in args.root:
        for dirpath, dirnames, filenames in os.walk(root):
            for fn in filenames:
                fp = os.path.join(dirpath, fn)
                try:
                    st = os.stat(fp)
                except OSError:
                    continue
                total_files += 1
                total_bytes += st.st_size
                rel = normalize_path(os.path.relpath(fp, root))
                # recover sessions may nest under a per-mode dir; strip known roots
                rel_stripped = strip_junk_prefixes(rel)
                m = None
                level = None
                if rel in by_path:
                    m, level = by_path[rel], 'path'
                elif rel_stripped in by_path:
                    m, level = by_path[rel_stripped], 'path'
                else:
                    key = (fn.lower(), st.st_size)
                    if key in by_namesize:
                        m, level = by_namesize[key][0], 'namesize'
                    elif fn.lower() in by_name:
                        m, level = by_name[fn.lower()][0], 'name'
                h = None
                if m is None:
                    # hash-only match (FAT32 8.3-name class, run 40079065019:
                    # the engine recovered the CONTENT but lost the LFN -
                    # recovered files carry _000.JPG-style short names that no
                    # path/name match can find. A SHA-256 match proves the
                    # content was recovered; the NAME loss stays visible in
                    # the path-preservation metrics and the match_level.)
                    h = sha256_file(fp)
                    if h in by_hash:
                        m, level = by_hash[h][0], 'hash'
                if m:
                    if h is None:
                        h = sha256_file(fp)
                    records.append({
                        'file': rel, 'match_level': level, 'manifest_rel': m['rel'],
                        'size': st.st_size, 'expected_size': int(m['size']),
                        'sha256': h, 'hash_match': h == m['sha256'],
                        'mtime': st.st_mtime, 'keep': bool(m.get('keep')),
                    })
                else:
                    ext = os.path.splitext(fn)[1].lower()
                    if fn.lower() in TOOL_INTERNAL or fn.lower().startswith('$'):
                        cls = 'tool-internal'
                    elif ext in CARVEABLE_EXT:
                        cls = 'unmatched-carveable-plausible'
                    else:
                        cls = 'noise'
                    records.append({
                        'file': rel, 'match_level': None, 'manifest_rel': None,
                        'size': st.st_size, 'sha256': h if h is not None else sha256_file(fp),
                        'hash_match': False, 'mtime': st.st_mtime, 'fp_class': cls,
                    })

    matched = [r for r in records if r['match_level']]
    hash_ok = [r for r in matched if r['hash_match']]
    fps = [r for r in records if not r['match_level']]
    out = {
        'label': args.label, 'tool': args.tool,
        'roots': args.root,
        'files_seen': total_files, 'bytes_seen': total_bytes,
        'matched': len(matched),
        'matched_path': sum(1 for r in matched if r['match_level'] == 'path'),
        'matched_namesize': sum(1 for r in matched if r['match_level'] == 'namesize'),
        'matched_name_only': sum(1 for r in matched if r['match_level'] == 'name'),
        'matched_hash_only': sum(1 for r in matched if r['match_level'] == 'hash'),
        'hash_match': len(hash_ok),
        'false_positives': len(fps),
        'fp_class': {
            'tool-internal': sum(1 for r in fps if r.get('fp_class') == 'tool-internal'),
            'unmatched-carveable-plausible': sum(1 for r in fps if r.get('fp_class') == 'unmatched-carveable-plausible'),
            'noise': sum(1 for r in fps if r.get('fp_class') == 'noise'),
        },
        'records': records,
    }
    with open(args.out, 'w', encoding='utf-8') as f:
        json.dump(out, f, indent=1)
    print(f"inventory[{args.label}]: files={total_files} matched={len(matched)} "
          f"(path={out['matched_path']} namesize={out['matched_namesize']} name={out['matched_name_only']} "
          f"hash-only={out['matched_hash_only']}) "
          f"hash_ok={len(hash_ok)} fp={len(fps)}")


if __name__ == '__main__':
    sys.exit(main())
