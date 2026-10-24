#!/usr/bin/env python3
"""vlm_extract_cli.py - batch VLM extraction of og screenshots via the z-ai CLI
(fixed template, confidence flags, model recorded per shot).

Usage: python vlm_extract_cli.py <screenshots-dir> <out.jsonl> <run-id>

Licensed under the MIT license. See LICENSE.
"""
import json
import os
import subprocess
import sys
import tempfile

TEMPLATE = ('You are reading a screenshot of the EaseUS Data Recovery Wizard Free UI '
            '(Windows, 1024x768). Extract structured facts. For EVERY value, also state '
            'your confidence: high (text clearly readable), medium (partially readable/'
            'inferred), low (uncertain). Answer ONLY with JSON of this exact shape: '
            '{"screen_kind": "landing|scanning|results_tree|destination_dialog|recovery_progress|'
            'completion|error_dialog|other", "file_count": {"value": <number|null>, '
            '"confidence": "high|medium|low"}, "total_size": {"value": "<string|null>", '
            '"confidence": "high|medium|low"}, "listed_filenames": {"value": ["<up to 10 names>"], '
            '"confidence": "high|medium|low"}, "selected_filenames": {"value": ["<up to 10 names>"], '
            '"confidence": "high|medium|low"}, "recovered_count": {"value": <number|null>, '
            '"confidence": "high|medium|low"}, "error_dialogs": {"value": ["<dialog title/text if any>"], '
            '"confidence": "high|medium|low"}, "drive_or_location_labels": {"value": ["<labels>"], '
            '"confidence": "high|medium|low"}, "notes": "<one line>"}')


def main():
    d, out, run_id = sys.argv[1], sys.argv[2], sys.argv[3]
    only = sys.argv[4].split(',') if len(sys.argv) > 4 and sys.argv[4] else None
    files = sorted(f for f in os.listdir(d) if f.lower().endswith('.png'))
    if only:
        files = [f for f in files if f in only]
    with open(out, 'w', encoding='utf-8') as o:
        for f in files:
            with tempfile.NamedTemporaryFile(suffix='.json', delete=False) as tf:
                tmp = tf.name
            try:
                # rate-limit aware: retry with backoff on transient 429s
                for attempt in range(6):
                    subprocess.run(['z-ai', 'vision', '-p', TEMPLATE, '-i', os.path.join(d, f), '-o', tmp],
                                   capture_output=True, timeout=240)
                    if os.path.getsize(tmp) > 0:
                        break
                    import time
                    time.sleep(30 * (attempt + 1))
                with open(tmp, encoding='utf-8') as r:
                    resp = json.load(r)
                model = resp.get('model', 'unknown')
                content = resp['choices'][0]['message']['content']
                content = content.strip()
                if content.startswith('```'):
                    content = content.strip('`')
                    if content.startswith('json'):
                        content = content[4:]
                try:
                    parsed = json.loads(content.strip())
                except Exception:
                    parsed = {'parse_error': True, 'raw': content[:1500]}
                rec = {'file': f, 'model': model, 'run_id': run_id,
                       'extracted_utc': __import__('datetime').datetime.now(
                           __import__('datetime').timezone.utc).isoformat(),
                       'extraction': parsed}
                o.write(json.dumps(rec) + '\n')
                print(f'{f}: {parsed.get("screen_kind", "?")} parse={"err" if parsed.get("parse_error") else "ok"}')
            finally:
                os.unlink(tmp)
    print(f'wrote {out} ({len(files)} shots)')


if __name__ == '__main__':
    main()
