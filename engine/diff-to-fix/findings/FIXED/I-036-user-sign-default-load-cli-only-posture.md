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

## Resolution

**Status:** ACKNOWLEDGED (the register confirmed first-hand; the D-104 + inventory precision edits applied)
**Fixed in commit:** (this session's I-036 landing — S47 lane; the doc edits in PARITY-DELTAS.md D-104 and PARITY-OG-CORE-INVENTORY.md:84)
**Verified by:** load-site census re-executed at core `d872452c`: `src/app/rrecov.cpp:1269-1310` is the ONLY default load site (the executable-adjacent `userdata-defaults.json` at CLI startup, line-drifted from the audited :1228-1264); `cr_ui_register_user_signs` at `src/ui/cr_ui.cpp:4615` (drifted from :4509) is the only ABI path; grep-census-zero for any engine/orchestrator-side default load (`src/core/`, `src/ui/` carry no install of the default set); `docs/PREBUILT-ARTIFACTS.md:106` already precise ("beside rrecov.exe").
**Resolution notes:** The register's claim is confirmed exactly: "default-on user signs" is the CLI product's posture; the library posture is default-off. The finding's demanded one-line precision edit applied at both stale sites — D-104's "loaded at engine init" (PARITY-DELTAS.md D-104) and the inventory row's "engine-init load" (PARITY-OG-CORE-INVENTORY.md:84) now read "CLI-app startup load; the library default is off (cr_ui_register_user_signs)". No code change — the divergence is the documented product posture and stays. Future sessions scoping S-055/S-056/M-128/N-171 (the channel's defects) should cite this register for the affected surface: the CLI default path + any explicit-loading host, never a bare library consumer.
