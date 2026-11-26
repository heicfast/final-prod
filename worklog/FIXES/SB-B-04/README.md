# SB-B-04 fix evidence — the dead session-load spinner on Windows (P2)

## Defect (before)

`session-load-progress:<path>` embedded the RAW session file path
(`C:\ProgramData\HeicFast\Recovery Wizard\sessions\auto_...crsf`) in the
Tauri event name. Tauri 2 event names are constrained to alphanumeric plus
`-`, `/`, `:`, `_` — the backslashes, spaces and dots of a real Windows path
violate the charset: the TS `listen()` rejects, the rejection is
void-swallowed, and the W1b load-progress spinner (`useSessionLoadFraction`)
never attaches on any real install (the mock routes through its own
registry with no Tauri names, so the browser-mode CI lane cannot see it —
the mock-divergence class again). The load itself still completes; only the
progress surface is dead. `let _ = app.emit(...)` also swallowed the emit
side (harmless for the fix — the name is now legal).

## Fix (after)

The event name is now keyed by the session path HEX-ENCODED (UTF-8 bytes →
`[0-9a-f]`), applied by the identical deterministic transform on both sides:
- Rust emit: `bridge::events::session_load_event_name` (events.rs, used by
  `session_load_sink`'s emit).
- TS listen: `sessionLoadEventName` (tauri-connector.ts, used by
  `onSessionLoadProgress`).
Charset-legal by construction, collision-free over the path's own bytes
(distinct sessions keep distinct channels). The event PAYLOAD still carries
the true path (JSON is unconstrained). Docs/contract updated:
contracts/event-mapping.md + contracts/command-surface.json (name + notes)
— `validate.mjs` green after the change.

## Regression tests (before-red / after-green)

- `ui/tests/connector-tauri.test.ts` — "session-load-progress keys a Windows
  path as a charset-legal hex name (SB-B-04)": registers a listener through
  the real connector for a Windows path, asserts the registered name
  matches the charset regex, equals the deterministic hex derivation, that
  the RAW-path name is NOT registered, that payloads cross verbatim, and
  that unsubscribe removes the listener. RED pre-fix, GREEN post-fix.
- `ui/src-tauri/src/bridge/events.rs` `sb_b_04_tests` (Rust unit test) —
  the emit-side name: charset-legal for the same Windows path, deterministic
  hex equality. (Rust tests cannot execute in this Linux sandbox — no Rust
  toolchain — the bridge-integration CI lane on the Windows runner runs
  cargo test on push; the evidence folder records this honestly.)

Captured runs (Linux sandbox, real vitest): BEFORE (connector reverted):
1 failed (the new pin). AFTER: connector-tauri 26/26 green; contracts
validate.mjs green; tsc/eslint/prettier green.
