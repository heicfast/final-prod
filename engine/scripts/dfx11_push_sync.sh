#!/usr/bin/env bash
# DFX11 push-sync: rebase onto origin/main, union-resolving the shared audit files,
# then push. Retries up to 6 times (the remote moves fast with parallel legs).
# Usage: bash scripts/dfx11_push_sync.sh
set -u
cd "$(git rev-parse --show-toplevel)" || exit 9
for i in 1 2 3 4 5 6; do
  git fetch origin || { echo "FETCH-FAIL"; exit 1; }
  UP=$(git rev-parse origin/main); HERE=$(git rev-parse main)
  if [ "$UP" = "$HERE" ] || git merge-base --is-ancestor "$UP" "$HERE"; then
    git push origin main && { echo "PUSH-OK"; exit 0; }
    continue
  fi
  if ! git rebase origin/main; then
    for f in $(git diff --name-only --diff-filter=U); do
      case "$f" in
        worklog.md|diff-to-fix/SUMMARY.md|diff-to-fix/CROSS-REF-MATRIX.md|diff-to-fix/findings/FIXED/RESOLVED-INDEX.md|docs/PARITY-DELTAS.md)
          python3 - "$f" << 'PYEOF'
import re,sys
p=sys.argv[1]; s=open(p).read()
out=[]; i=0
while True:
    m=re.search(r'<<<<<<< [^\n]*\n(.*?)=======\n(.*?)>>>>>>> [^\n]*\n', s[i:], re.S)
    if not m: out.append(s[i:]); break
    out.append(s[i:i+m.start()]); out.append(m.group(1)); out.append(m.group(2))
    i+=m.end()
open(p,'w').write(''.join(out))
print('union-resolved', p)
PYEOF
          git add "$f" ;;
        worklog/session-diff-fix-prompt.md)
          git checkout --ours -- "$f" 2>/dev/null || true; git add "$f" ;;
        *)
          echo "MANUAL-CONFLICT: $f"; exit 8 ;;
      esac
    done
    GIT_EDITOR=true git rebase --continue || exit 7
  fi
done
echo "PUSH-RETRIES-EXHAUSTED"; exit 6
