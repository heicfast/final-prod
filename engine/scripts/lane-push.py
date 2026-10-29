#!/usr/bin/env python3
"""lane-push.py — robust fetch/rebase/resolve/push loop for the racing-lane environment.

Resolves worklog.md conflicts by stacking both sides (racing entry first, our entry
after), per the stacked-history convention. Only handles worklog.md conflicts;
any other conflict aborts for manual resolution.
"""
import subprocess, sys, re, time

REPO = "/home/z/my-project/work/core_recovery"

def run(cmd, check=False):
    r = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True)
    if check and r.returncode != 0:
        print(f"CMD FAILED: {' '.join(cmd)}\nSTDOUT: {r.stdout}\nSTDERR: {r.stderr}")
    return r

def resolve_worklog_conflict():
    """Stack both sides of the worklog.md conflict: HEAD (racing) first, ours after."""
    with open(f"{REPO}/worklog.md", encoding="utf-8") as f:
        content = f.read()
    m = re.search(r"<<<<<<< HEAD\n(.*?)\n=======\n(.*?)\n>>>>>>> [^\n]*\n", content, re.DOTALL)
    if not m:
        return False
    head_side, our_side = m.group(1), m.group(2)
    # Dedup: if our side content already exists elsewhere (already stacked), skip it
    if our_side.strip() and our_side.strip()[:200] in content.replace(m.group(0), ""):
        resolution = head_side
    else:
        resolution = head_side + "\n\n---\n\n" + our_side
    content = content[:m.start()] + resolution + content[m.end():]
    with open(f"{REPO}/worklog.md", "w", encoding="utf-8") as f:
        f.write(content)
    return True

attempts = int(sys.argv[1]) if len(sys.argv) > 1 else 12
for i in range(1, attempts + 1):
    run(["git", "fetch", "origin"])
    remote = run(["git", "rev-parse", "origin/main"]).stdout.strip()
    local = run(["git", "rev-parse", "HEAD"]).stdout.strip()
    if remote == local:
        print(f"SYNCED {local[:8]}")
        sys.exit(0)
    # rebase onto remote
    r = run(["git", "rebase", "origin/main"])
    if r.returncode != 0:
        status = run(["git", "status", "--short"]).stdout
        conflicted = [l for l in status.splitlines() if l.startswith("UU")]
        non_worklog = [l for l in conflicted if "worklog.md" not in l]
        if conflicted and not non_worklog and resolve_worklog_conflict():
            run(["git", "add", "worklog.md"])
            rc = run(["git", "-c", "core.editor=true", "rebase", "--continue"])
            if rc.returncode != 0:
                run(["git", "rebase", "--abort"])
                print(f"REBASE_CONTINUE_FAILED attempt={i}")
                time.sleep(4)
                continue
        else:
            run(["git", "rebase", "--abort"])
            print(f"MANUAL_CONFLICT_NEEDED attempt={i}: {status}")
            sys.exit(2)
    # push
    p = run(["git", "push", "origin", "main"])
    if p.returncode == 0:
        print(f"PUSHED {run(['git','rev-parse','HEAD']).stdout.strip()[:8]}")
        sys.exit(0)
    print(f"RETRY attempt={i}: {p.stderr.strip().splitlines()[0] if p.stderr.strip() else ''}")
    time.sleep(2)
print("GAVE_UP")
sys.exit(1)
