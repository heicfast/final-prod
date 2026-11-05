#!/bin/bash
# sync-push.sh — pull-rebase with conflict auto-resolution for worklog/trail files, then push with retries.
# Usage: sync-push.sh [max_attempts]
# Requires: commit already made (clean tree apart from staged). If conflicts arise in
# trail files, the stacked-history convention is applied manually by the caller.
MAX=${1:-8}
cd /home/z/my-project/work/core_recovery
for i in $(seq 1 $MAX); do
  git fetch origin --quiet
  LOCAL=$(git rev-parse HEAD)
  REMOTE=$(git rev-parse origin/main)
  if [ "$LOCAL" = "$REMOTE" ]; then
    echo "ALREADY_SYNCED $LOCAL"
    exit 0
  fi
  # try rebase
  if git pull --rebase origin main --quiet >/dev/null 2>&1; then
    if git push --quiet origin main 2>/dev/null; then
      echo "PUSHED $(git rev-parse --short HEAD)"
      exit 0
    fi
  else
    echo "REBASE_CONFLICT attempt=$i"
    git rebase --abort 2>/dev/null
    exit 2
  fi
  echo "RETRY attempt=$i (racing lane moved again)"
  sleep 3
done
echo "GAVE_UP_AFTER $MAX"
exit 1
