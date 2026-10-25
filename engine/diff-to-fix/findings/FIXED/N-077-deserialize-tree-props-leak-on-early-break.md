# MINOR N-077 — session loader leaks the caller's FileProperties reference on every early break between allocation and attach (LeakSanitizer red since `b7d4e22`)

**Severity:** MINOR (NEW-FINDING, session 22 baseline repair: the ASan CI lane was red on `main` when this session's first push was made) · **Module:** session (tree-record loader) · **Parameters affected:** Q67 (sanitizer cleanliness — the linux ASan lane fails), A35-adjacent (error-path resource hygiene on hostile input)
**Corpus references:** none — a quality defect, not an og-parity divergence; the defect was introduced by the N-058/M-050 hardening landing itself (`b7d4e22`, which added the bounds-gate break paths without releasing the just-allocated object)
**Core references:** `src/core/session/cr_records.cpp` `deserialize_tree()`: `props = new (std::nothrow) tree::FileProperties()` (refs_=1, caller-owned per `include/cr/tree/cr_node.h:123-131`'s refcount contract) with FOUR early `break` paths before the single `props->release()` at the loop tail — the N-058 name-length gate, the resident-data truncate, the sha128 truncate, the M-051 carve-note length gate, plus the N-058 role-length gate inside the marker==1 branch; audited at `275b7ac` (post-M-055 push)
**Status:** NEW-FINDING — filed and FIXED in the same session as baseline repair (a session does not push findings onto a red lane)

## Summary

The N-058/M-050 loader hardening (`b7d4e22`) added per-record bounds gates
that `break` out of the record loop the moment a hostile or truncated stream
lies about a length. Every one of those breaks exits AFTER the per-record
`new FileProperties()` but BEFORE the ownership handoff (`add_file` /
`set_props` + the loop-tail `props->release()`). The caller-held reference
(started at 1 by the constructor, per the `cr_node.h` refcount contract) is
never dropped — the object leaks. LeakSanitizer turns this into a hard CI
failure on the linux ASan lane: `Session.TruncatedTailFailsAfterFullCount`
and `Session.HostileNameAndRoleLengthBombs` both abort with `Direct leak of
528 byte(s) in 2 object(s) allocated from ... deserialize_tree ...
cr_records.cpp:253`.

## What the reference does

Not an og-parity finding. The og's loader (rsf-record4) is not in the leak's
causal chain; the defect is local to our hardening wave.

## What our core did (pre-fix)

As above — exactly one `FileProperties` (~528 bytes with the default
members) leaked per failed `deserialize_tree` call. Reachable only via
corrupt/hostile session files (the failure paths themselves are correct
N-058/M-051 behavior — the gates fire as designed); the leak is the
unreleased caller reference on the way out.

## Gap

Falsifiable and falsified in-session: the ASan lane at `275b7ac^` (pre-fix)
fails both tests with the leak report quoted above; post-fix the full ASan
suite is green (640/640). The failing tests already existed — they ARE the
regression tests; the fix makes the existing detectors pass.

## Impact

- CI: the linux ASan lane was red on `main` (every push after `b7d4e22`).
- Runtime: a slow, bounded leak on hostile session files (one object per
  failed load attempt); no wrong results, no crash.

## Reproduction / verification

Executed this session (the fail-before ran on the unmodified tree):

```
$ ctest --preset linux-gcc   # pre-fix, at 275b7ac^
Session.TruncatedTailFailsAfterFullCount  ... Failed
Session.HostileNameAndRoleLengthBombs     ... Failed
==11598==ERROR: LeakSanitizer: detected memory leaks
Direct leak of 528 byte(s) in 2 object(s) allocated from:
    #1 ... cr::session::deserialize_tree(...) src/core/session/cr_records.cpp:253
$ (fix applied)
$ ctest --preset linux-gcc
100% tests passed out of 640
```

## Fix plan (executed this session)

1. A scoped guard (`PropsRef`: releases the caller's reference on
   destruction unless dismissed) wrapping the per-record allocation.
2. The normal path keeps its explicit `props->release()` and then dismisses
   the guard (`owned = false`) — zero double-release, zero behavior change
   on any successful path.
3. Every early break now releases via the guard's destructor (the
   N-058/M-051 gates' semantics unchanged — they still break exactly where
   they did).

## Tests to add

None beyond the existing pair — `Session.TruncatedTailFailsAfterFullCount`
and `Session.HostileNameAndRoleLengthBombs` are the detectors; the fix is
proven by their red→green transition under LeakSanitizer.

## Related findings

- N-058 / M-050 (the hardening whose gates introduced the leak — the gates
  stay; the leak was the landing defect, same pattern as M-055: correct
  finding, broken landing).
- M-055 (the same session's other baseline repair: win-layer buildability).

## Open questions

None.

## Resolution

**Status:** FIXED
**Fixed in commit:** this session's N-077 commit (PropsRef guard in deserialize_tree)
**Verified by:** fail-before executed on the unmodified tree (both tests Failed with the LeakSanitizer report quoted in Reproduction; the ASan suite was red at the session's first push) → pass-after full linux-gcc/ASan suite **640/640** green; linux-release suite also re-verified green with the change.
**Resolution notes:** Scoped PropsRef guard releases the caller-held FileProperties reference on every early exit between the per-record allocation and the ownership handoff; the normal path dismisses the guard after its existing explicit release, so no successful-path behavior changes. The four N-058/M-051 bounds gates keep their exact semantics (break on hostile lengths); only the leaked reference on the way out is fixed. Root cause class recorded for the audit trail: the M-050/N-058 landing (`b7d4e22`) added the break paths without auditing the ownership lifecycle it was interrupting — and landed without running the ASan lane, which is precisely the lane that catches this defect class; the parallel-lane sloppiness pattern (visible-red CI absorbed by merges) is the same one M-055 documented.
