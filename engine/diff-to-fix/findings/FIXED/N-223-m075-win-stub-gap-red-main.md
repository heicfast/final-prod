# MINOR N-223 — the M-075 lane's push skipped the win_syntax_check gate's own rule: cr_fsck.cpp's new SDK symbols (Wow64DisableWow64FsRedirection / Wow64RevertWow64FsRedirection / ULONGLONG) landed without the tests/win-stub/windows.h declarations — main's Linux suite red at the pull

**Severity:** MINOR (the class convention; the repair landed in the same session — a NEW shape for the pushed-red class: not markers, not a semantic clobber, an incomplete stub header that the gate's own header comment explicitly names: "A new SDK symbol in src/win grows the stub here — that is the gate working")
**Module:** audit-trail integrity + CI (the win_syntax_check gate; the M-075 surface)
**Parameters affected:** Q68 (CI truth), the win-layer parse gate
**Corpus references:** none for the stub defect (a core-repo gate-integrity defect); the M-075 surface itself is the racing lane's landing (d10ae333)
**Core references:** found at origin/main after `d10ae333` (the M-075 landing): `src/win/cr_fsck.cpp:40/:47/:54` use `Wow64DisableWow64FsRedirection`/`Wow64RevertWow64FsRedirection` and `:58` `ULONGLONG` — none declared in `tests/win-stub/windows.h`. Verified first-hand: `ctest -R win_syntax_check` at the pulled tree → FAIL with the five undeclared-symbol errors; STILL FAILING at the post-M-078 head (`ddab68b4`-lineage) — the racing M-078 window's file set does not include the stub.
**Status:** FIXED (found and fixed in the same session, per §8's discovered-divergence rule)

## Summary

The M-075 racing lane landed the chkdsk endpoint with three new Windows SDK symbols but did not grow the win-stub header the parse gate compiles against — exactly the step the gate's own header comment prescribes. Main's Linux suite was red at the pull (`win_syntax_check` FAIL) and stayed red through the M-078 window — two racing lanes pushed over a red gate.

## The repair (executed this session)

1. `tests/win-stub/windows.h`: the three declarations added (ULONGLONG typedef + the two Wow64 redirection stubs, SDK-faithful shapes, declaration-only as the stub contract requires).
2. The gate re-run: 13/13 files parse OK; the full Linux suite green at the repaired tree.

## Resolution

**Status:** FIXED
**Fixed in commit:** (this push; the S85 parity fix lane, 2026-10-30 — rides the M-078 convergence record)
**Verified by:** `bash tools/win_syntax_check.sh` → 13 ok / 0 fail; the full ctest suite 100% at the repaired tree.
**Resolution notes:** The repair is the gate's own prescribed form (the stub grows with the SDK surface). The lesson extends the N-220 one: the pulled-red protocol must run the FULL ctest suite (which includes win_syntax_check) before any fix work — this session's own open did exactly that and caught the red before it could compound. Two racing windows (M-075's own and the M-078 window after it) pushed over the red gate; recorded here so the next audit's liveness battery can price the pattern.
