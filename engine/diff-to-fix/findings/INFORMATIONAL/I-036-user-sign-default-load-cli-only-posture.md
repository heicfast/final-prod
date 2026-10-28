# INFORMATIONAL I-036 — the user-sign "default-on" posture is CLI-app-level only: the engine/orchestrator and the cr_ui ABI path load nothing by default (D-104's "loaded at engine init" phrasing is imprecise)

**Severity:** INFORMATIONAL
**Module:** carve (user-sign default posture)
**Parameters affected:** API contract truthfulness, configuration defaults
**Corpus references:** n/a (the og has no channel — D-104/E-A); D-104's own text: "loaded at engine init when no explicit --user-signs is given" (`docs/PARITY-DELTAS.md:3194-3196`)
**Core references:** the ONLY load site: `src/app/rrecov.cpp:1228-1264` (the CLI app's startup); no default load in the engine, the orchestrator, or cr_ui (an ABI host gets user signs only via `cr_ui_register_user_signs`, `src/ui/cr_ui.cpp:4509`); the precise doc: `docs/PREBUILT-ARTIFACTS.md:106` ("beside rrecov.exe"); audited at core `8d0d88b8
**Status:** READ-ONLY REGISTER — no fix applied

## Summary

"default-on user signs" is the CLI product's posture; the library posture is
default-off. D-104 and PARITY-OG-CORE-INVENTORY phrase it as an engine-init behavior,
which overstates the surface. Registered here so hosts and parity benches scope the
channel correctly (the S-055/S-056/M-128 defects apply to the CLI default path and to
any host that explicitly loads the set — not to bare library consumers).

## What the reference does

n/a.

## What our core does

The single load site (cite above) + the explicit registration API.

## Gap

Doc-precision only.

## Impact

Bench/parity scoping correctness; no behavior defect.

## Reproduction / verification

Load-site census (grep + the read of rrecov.cpp:1228-1264 and cr_ui.cpp:4509).

## Fix plan

1. One-line precision edit to D-104 / the inventory row (doc lane): "loaded by the CLI
   app at startup; the library default is off (cr_ui_register_user_signs)".
2. No code change.

## Tests to add

- None (optionally a doc-gate phrase).

## Related findings

- S-055 / S-056 / M-128 / M-129 / N-171 (the channel's defects — all CLI-default-path
  or explicit-load scoped by this register)
- N-170 (the doc-truth batch D-104 belongs to)

## Open questions

None.
