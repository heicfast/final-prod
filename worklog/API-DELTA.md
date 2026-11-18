# API-DELTA — the reference API surface vs the product, per capability

> Deliverable of Phase 2 (T10), Installer Integration & API Surface Expansion
> session, 2026-11-18. Sources: `worklog/INTEGRATION-MATRIX.md` §3 (the
> 32-row gap register M01–M32, resolutions + corpus citations — the
> execution contract), `worklog/REFERENCE-API.md` (§10, the ranked
> capability gaps), `worklog/PRODUCT-API.md` (the counted-from-source
> product truth + §7 documented-truth gap list), and this session's wave
> landings. Row semantics: CLOSED = shipped + pinned this session (the
> wave's SHAs in worklog.md); OPEN-scheduled = a matrix wave owns it
> (W5..W10); DEFER/DISPOSITIONED = the recorded scope stands (never
> silently dropped). Every corpus citation lives in the matrix row;
> this sheet is the per-capability resolution ledger.

**INTEGRATION BOUNDARY (binding, unchanged):** capability parity in the
product's own original code only — no og-codebase file, code, or binary
is ever copied, shipped, or derived from. Reference rows cite corpus
BEHAVIOR (strings/flows/grammar), not bytes.

## 1. Imaging — CLOSED this session (W4-A b8740541 + W4-B 07eb9ed4 + style b43ce273)

| Row | Capability (corpus citation) | Resolution |
|---|---|---|
| M01 / D-IMG-1 | Image-create CANCEL — stop-confirm + "You need to re-create the disk image if you stop it." (rev/strings:11672-11673) | **CLOSED**: `cr_ui_image_create_ex`'s caller-owned volatile flag (the session_load_with_events idiom), threaded through the win-layer CancelToken; the CLI's Ctrl-C token; the bridge's `image_create_cancel` command + the entry flag; `cancelSupported: true` + `cancelRequested` on the state surface; the event-map's cancel acknowledgment (the terminal image-finished payload). On cancel the partial is KEPT (S-040 disposition) — the reference's re-create truth. |
| M02 / D-IMG-3 | Sector-range imaging — "More Option" from/to, [0-9]{1,15} validation, "Invalid sector" (rev/strings:11640-11760) | **CLOSED**: `cr_ui_image_create_ex(disk, path, first_sector, sector_count, ...)` — first 0-based, count 0 = to the end, 0/0 = whole disk = BYTE-IDENTICAL legacy (the pinned no-regression test); CLI `--from-sector/--to-sector` (both-or-neither, the digits grammar, inclusive [from,to], the literal legacy call when un-ranged); the bridge's optional firstSector/sectorCount + the ranged space gate; the it-gate's real-disk range run. Out-of-range refuses ERR_BAD_PARAMETER — never a silent truncation. |
| M01/M02 residual | The TS connector surface (contract.ts `cancelImageCreation` + range params on `startImageCreation`) + the stop-confirm dialog + More-Option fields | **OPEN — W9's recorded scope** (the UI wave): the engine + CLI + bridge halves are the shipped parts today; the connector method, the mock, and the dialog land with the imaging UI. |
| M03 / D-IMG-2 | `.edisk` proprietary container | **DEFER (scope stands)**: the og's container internals are corpus-UNKNOWN — a writer cannot be specified without invention, and copying the container is IP-barred; the flat family + D-147's VHD/VHDX exclusion stand. |
| M04 / BF-12 | Image-file scan targets (`.vmdk .vhd .vhdx .edisk .dmg .img` filter) | **OPEN — W8** (UI wiring: the import dialog over the shipped `cr_ui_open_image`/CLI surface; `.edisk` excluded by M03). |

## 2. Preview / media / archive — OPEN (W5) or DEFER, unchanged this session

| Row | Resolution |
|---|---|
| M05 / D-P8 rendered video/document previews | DEFER — scope verbatim (renderer-set provenance decision; the truthful metadata + not-rendered hint ships meanwhile). |
| M06 archive browse-inside tree grouping | OPEN — W5 (bridge/UI tree over the shipped flat inventory; the 7z/rar/tar families stay deferred). |
| M07 thumbnail live-scan addressing | OPEN — W5 (`cr_ui_thumbnail_generate_snapshot`). |
| M13 CR_EVT 13 CURRENT_PATH | OPEN — W5 (the per-item path event + events.rs). |

## 3. Repair — the big open family (W7/W9)

| Row | Resolution |
|---|---|
| M08 / G-1(b) standalone repair workbench | OPEN — W7 (ABI trio `cr_ui_repair_analyze/_apply/_undo` with `rrecov fix` semantics) + W9 (the workbench page over the four real Repair methods). NG-12's truthful interim page is W8. |
| M09 per-format file-CONTENT repair | DISPOSITIONED (S-050/D-137): a separate product line, recorded, unscheduled. |
| M10 repair-during-recovery | DISPOSITIONED (BF-14's truthful-disable stands until M09's engines exist). |

## 4. SMART / health — closed axes stay closed; the vendor tail deferred

| Row | Resolution |
|---|---|
| M11 / D-P4c vendor bridge dialects | DEFER — scope stands (decode the corpus decompile FIRST; no fabricated dialects). |
| M12 vendor attribute display-name families | OPEN — W5 (own-code data tables from the decoded research markdown). |

## 5. Scan / results / query; Recovery; robustness — OPEN waves

| Row | Resolution |
|---|---|
| M14 query `existing` knob + parity harness | OPEN — W6 (D-B3's one-path rationale stands until the measured migration). |
| M20 / R-48 ACL preservation | OPEN — W6 (opt-in; $SDH/$SII + SetNamedSecurityInfoW; never blocks). |
| M15 filter toggles; M16 BF-21 Versions/Tags; M24 BF-07 reattach-resume; M22 reasonKeys; M28 elevation ordering; M31 NG-12 interim; M32 BF-11 maintenance screens; M21 R-47 Auto-Recover | OPEN — W8 (the no-new-engine-deps UI wiring batch; Auto-Recover's email leg deferred). |
| M29 BF-25/30 carve wall + checkpoint; M30 BF-32 teardown race | OPEN — W10 (measured-only; the concurrent performance lane owns this territory — coordinated, not duplicated). |

## 6. The session's API-count truth

93 → **94 exports** (the append landed with every same-commit invariant:
gen_abi_layouts re-run — zero struct drift; abi.rs re-pinned 94/94; the
doc_truth_gate caught and re-trued all six stale count sites; the W3
contracts-README guard now enforces the class forever). The CLI gains two
flags (`image --from-sector/--to-sector`); the Tauri command surface
gains one command (`image_create_cancel`, 54 → 55 rows, serves-empty
until W9 wires the connector method); the event map's imaging-cancel truth
landed. No existing export, signature, error code, event, or CLI verb
changed — the append-only policy held everywhere.
