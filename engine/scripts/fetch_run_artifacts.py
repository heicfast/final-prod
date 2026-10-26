#!/usr/bin/env python3
"""fetch_run_artifacts.py <run_id> <name_prefix> <destdir> [concurrency]
Download every artifact of a run whose name starts with <name_prefix>,
unpacking each into <destdir>/<artifact-name>/ .
"""
import concurrent.futures as cf
import io
import json
import os
import sys
import time
import urllib.request
import zipfile

BASE = 'https://api.github.com/repos/heicfast/core_recovery'
TOK = os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
if not TOK:
    sys.exit('GH_TOKEN must be set')


class NoAuthRedirect(urllib.request.HTTPRedirectHandler):
    """Blob storage (artifact zips) signs redirects with SAS tokens - the
    GitHub Authorization header forwarded on redirect makes the signature
    invalid (403). Strip auth headers on cross-host redirects."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        new = super().redirect_request(req, fp, code, msg, headers, newurl)
        if new is not None:
            host = urllib.request.urlsplit(newurl).hostname or ''
            api_host = urllib.request.urlsplit(BASE).hostname
            if host != api_host:
                new.unredirected_hdrs.pop('Authorization', None)
                for k in list(new.headers):
                    if k.lower() == 'authorization':
                        del new.headers[k]
                new.unredirected_hdrs.pop('authorization', None)
        return new


OPENER = urllib.request.build_opener(NoAuthRedirect())


def api(url):
    req = urllib.request.Request(url, headers={'Authorization': 'token ' + TOK,
                                               'Accept': 'application/vnd.github+json'})
    with OPENER.open(req) as r:
        return json.load(r)


def fetch_one(aid, name, dest):
    outdir = os.path.join(dest, name)
    if os.path.isdir(outdir) and os.listdir(outdir):
        return name, 'cached'
    url = f'{BASE}/actions/artifacts/{aid}/zip'
    for attempt in range(4):
        try:
            req = urllib.request.Request(url, headers={'Authorization': 'token ' + TOK})
            with OPENER.open(req, timeout=300) as r:
                data = r.read()
            os.makedirs(outdir, exist_ok=True)
            with zipfile.ZipFile(io.BytesIO(data)) as z:
                z.extractall(outdir)
            return name, 'ok'
        except Exception as e:
            if attempt == 3:
                return name, f'FAIL {e}'
            time.sleep(3 * (attempt + 1))


def main():
    run_id, prefix, dest = sys.argv[1], sys.argv[2], sys.argv[3]
    conc = int(sys.argv[4]) if len(sys.argv) > 4 else 4
    os.makedirs(dest, exist_ok=True)
    items, page = [], 1
    while True:
        d = api(f'{BASE}/actions/runs/{run_id}/artifacts?per_page=100&page={page}')
        batch = [a for a in d.get('artifacts', []) if a['name'].startswith(prefix)]
        items.extend(batch)
        if len(d.get('artifacts', [])) < 100:
            break
        page += 1
    print(f'{len(items)} artifacts match prefix {prefix!r}')
    ok = fail = cached = 0
    with cf.ThreadPoolExecutor(max_workers=conc) as ex:
        futs = [ex.submit(fetch_one, a['id'], a['name'], dest) for a in items]
        for f in cf.as_completed(futs):
            name, status = f.result()
            if status == 'ok':
                ok += 1
            elif status == 'cached':
                cached += 1
            else:
                fail += 1
                print(f'  {name}: {status}', file=sys.stderr)
    print(f'done: {ok} downloaded, {cached} cached, {fail} failed -> {dest}')
    sys.exit(1 if fail else 0)


if __name__ == '__main__':
    main()
