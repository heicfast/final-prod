#!/usr/bin/env python3
"""rematch_inventory.py - post-process an inventory JSON with the hash-only
match level (added after run 40079065019 revealed the FAT32 8.3-name class:
the engine recovered CONTENT but emitted short names, and the in-CI inventory
could not path/name-match them - the payloads are deleted after CI inventory,
but the records carry each file's SHA-256, so the re-match is exact).

For every unmatched record whose sha256 matches a manifest entry, the record
gains match_level='hash', manifest_rel, keep; a re-run over an already
hash-matched inventory is a no-op. Idempotent; never weakens an existing
path/name match.
"""

import argparse
import json
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--inventory', required=True)
    ap.add_argument('--manifest', required=True)
    args = ap.parse_args()

    with open(args.inventory, 'r', encoding='utf-8-sig') as f:
        inv = json.load(f)
    by_hash = {}
    with open(args.manifest, 'r', encoding='utf-8-sig') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            o = json.loads(line)
            if o.get('_meta'):
                continue
            by_hash.setdefault(o['sha256'], []).append(o)

    rematched = 0
    for r in inv.get('records', []):
        if r.get('match_level'):
            continue
        h = r.get('sha256')
        if h and h in by_hash:
            m = by_hash[h][0]
            r['match_level'] = 'hash'
            r['manifest_rel'] = m['rel']
            r['hash_match'] = True
            r['keep'] = bool(m.get('keep'))
            r['expected_size'] = int(m['size'])
            rematched += 1

    # refresh the aggregate counters
    matched = [r for r in inv['records'] if r.get('match_level')]
    hash_ok = [r for r in matched if r.get('hash_match')]
    fps = [r for r in inv['records'] if not r.get('match_level')]
    inv['matched'] = len(matched)
    inv['matched_path'] = sum(1 for r in matched if r['match_level'] == 'path')
    inv['matched_namesize'] = sum(1 for r in matched if r['match_level'] == 'namesize')
    inv['matched_name_only'] = sum(1 for r in matched if r['match_level'] == 'name')
    inv['matched_hash_only'] = sum(1 for r in matched if r['match_level'] == 'hash')
    inv['hash_match'] = len(hash_ok)
    inv['false_positives'] = len(fps)
    inv['rematched_offline'] = rematched

    with open(args.inventory, 'w', encoding='utf-8') as f:
        json.dump(inv, f, indent=1)
    print(f"rematch[{args.inventory}]: {rematched} hash-only matches added; "
          f"matched={inv['matched']} (hash-only={inv['matched_hash_only']}) "
          f"hash_ok={inv['hash_match']} fp={inv['false_positives']}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
