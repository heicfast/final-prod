# INFORMATIONAL I-023 — DRWCfg.bin persistent config channel (AES config cache, remembered scan mode) is out of the engine's scope; registered deliberately

**Severity:** INFORMATIONAL
**Module:** orchestration (config), cli
**Parameters affected:** P42 (public surface), A37 (stage model — scan-mode selection), P46 (documentation)
**Corpus references:** `re-workspace/artifacts/config-files/drwcfg-bin--aes-config-cache.md:32-54` (DRWCfg.bin: AES-256-CBC with a hardcoded key, per-scan-state/MRU/flags persisted between runs), `re-workspace/artifacts/UIUX/13-ui-core-wiring-state.md:28` (the remembered scan mode is re-applied at scan start)
**Core references:** `include/cr/cr_ui.h:296-320` (`cr_ui_scan_config` — per-call only; no persistent store), `src/app/rrecov.cpp` (stateless CLI: every run re-specifies mode/flags)
**Status:** READ-ONLY FINDING — deliberate difference registered (no defect)

## Summary

The og persists UI/session state (including the last-used scan mode, re-applied at the next scan start) in an AES-encrypted DRWCfg.bin cache. The core is a library + stateless CLI: scan configuration is per-call, with no settings persistence anywhere. This is a scope boundary (host-owned settings vs engine), registered here so the roadmap records it rather than rediscovering it.

## What the reference does

The drwcfg decode (first-cited this session) documents the AES-256-CBC config cache (hardcoded key), what it persists (per-scan state, MRU lists, flags), and the wiring artifact shows the remembered scan mode being re-applied at scan start.

## What our core does

`cr_ui_scan_config` is a zero-init struct per call; the CLI re-parses flags per run; nothing writes a settings file. The session/checkpoint files are the only persistent artifacts, and they persist SCAN STATE, not settings.

## Gap / Disposition

Deliberate: settings persistence is a host responsibility (a GUI host persists its own config and feeds cr_ui per call — the information content is identical). The og's channel is UI-layer state riding in the product binary. Registered as a scope exclusion; no action required unless a future host asks the core to own settings (in which case: do NOT copy the hardcoded-key AES — modern DPAPI/OS keychain is the honest equivalent).

## Reproduction / verification

Corpus-derived (two artifacts first-cited this session, cited above). Core side verified in-source at the audited commit.

## Fix plan

None (register). If ever needed: a host-side settings story belongs in the docs (UI-READINESS-PLAN), not the engine.

## Tests to add

None.

## Related findings

M-024 (runtime format-filter surface — the SCAN-AFFECTING config subset, which IS in scope and remains an active finding); I-010 (scope exclusions register — this joins that ledger); N-074 (doc-truth family — if CAPABILITY-MATRIX ever claims parity here, that would be a drift item).

## Open questions

None.

---

## Resolution

**Status:** ACKNOWLEDGED (verified this session, closing session / df5 lineage)
**Fixed in commit:** none — deliberate scope exclusion, code unchanged (the register's instruction)
**Verified by:** corpus artifact re-read first-hand (drwcfg-bin--aes-config-cache.md :30-40 — the AES-256-CBC hardcoded-key writer flow is real and UI-layer); core side re-verified (cr_ui.h:296 per-call struct; rrecov stateless CLI; no settings store anywhere in src/).
**Resolution notes:** The deliberate-difference rationale is CONFIRMED and preserved: settings persistence is a host responsibility feeding the per-call config surface — the information content is identical, and the og's channel is UI state riding in the product binary. The register's do-not-copy note (hardcoded-key AES → DPAPI/keychain for any future host story) is the correct posture and stands. No code change, per the INFORMATIONAL safety-hardening rule.
