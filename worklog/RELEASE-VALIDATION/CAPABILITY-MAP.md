# CAPABILITY-MAP — gate §7 evidence: every capability surfaced or truthfully disabled

Exported from INTEGRATION-PLAN §4.7 with per-row verification against the
P4 tree (d166147b). Legend: **SURFACE** = wired to a real UI surface;
**DISABLE** = truthful-disable with a documented reason (+ a
BEHAVIOR-FINDINGS entry). Spot-verification commands in the right column.

| # | Capability (v4 exports) | Disposition | Where / why (verified) |
|---|---|---|---|
| 1 | Lost-partition apply + undo | **SURFACE** | lost-partition feature ↔ commands/lost_partition.rs (consent-gated apply + undo + undo-file save) |
| 2 | Sessions: list/load/save | **SURFACE** | backup screen + results save spinner ↔ commands/sessions.rs |
| 3 | Session resume + identity verify | **SURFACE** | queries/sessions.ts + home resume ↔ sessions.rs (`grep -rln resumeLast src/queries` hits both) |
| 4 | Autosave (dir + CR_EVT_AUTOSAVED) | **SURFACE** | completion auto-save to the sessions dir (scan.rs W1c wiring) |
| 5 | Thumbnails | **SURFACE** | preview.rs — sessions: real generation; live scans: preview-read fallback (BF-13) |
| 6 | Archive browse-inside | **SURFACE** | preview pane 'archive' payload (M-116 caps in preview.rs) |
| 7 | Dictionary unlock | **SURFACE** | unlock-volume-dialog.tsx ↔ devices.rs (explicit consent, N-064) |
| 8 | FVEK unlock | **SURFACE** | same dialog, advanced input (open_volume_with_key) |
| 9 | Consent surface (repair + apply) | **SURFACE** | consent-modal.tsx + bridge/consent.rs (non-dismissable, re-hydration, force-deny) |
| 10 | Elevation posture + relaunch | **SURFACE** | elevation-banner.tsx + commands/system.rs (TokenElevation + ShellExecuteW "runas") |
| 11 | Query filters/sort/paging | **SURFACE** | results.rs G-7 port over NodeMeta; filefilter mirror 1,481 fresh (`node scripts/gen-filefilter.mjs --check`) |
| 12 | Recycle-bin / Desktop quick targets | **SURFACE** | home-page.tsx:54 `vol-recycle-bin` ↔ devices.rs:253 resolution (grep-verified both sides) |
| 13 | User-taught signatures | **SURFACE** | settings-page.tsx import + startup defaults load (register_user_signs) |
| 14 | Health (health_read base) | **SURFACE** | health-page.tsx ↔ commands/health.rs; threshold column truthful-disabled (BF-03) |
| 15 | Pre-scan repair consent (volume targets) | **SURFACE** | consent modal; whole-disk targets disabled by the NG-21 bridge guard (§4.6) |
| 16 | Carve format catalog | **DISABLE** | no reference screen; filefilter mirror carries classification truth; `rrecov list-carve-formats` covers (BF-08) |
| 17 | Live carve tally | **DISABLE** | type-tree counts from real rows cover the need (BF-08) |
| 18 | Disk map | **DISABLE** | no visualization screen (BF-09, scoped) |
| 19 | Image create | **DISABLE** | no screen; CLI `rrecov image --create` covers (BF-10) |
| 20 | fsck escalation | **DISABLE** | in-scan pre_scan_check audit covers the product flow (BF-11) |
| 21 | BitLocker off | **DISABLE** | destructive irreversible decrypt, unobservable progress; CLI covers (BF-11) |
| 22 | TRIM chip at enumerate | **DISABLE** | one fsutil spawn per refresh avoided; health carries trim truth |
| 23 | Image targets (open_image) | **DISABLE** | no image-file scan surface (BF-12 — would also enable unelevated scans) |
| 24 | Core-side query engine | **DISABLE** | bridge-side filter over real rows; T14 re-measure pending (BF-16) |
| 25 | Assistant stats + regrade | **DISABLE** | no reference surface (BF-17) |
| 26 | Transport diagnostics | **DISABLE** (internal) | error refinement only, by design |
| 27 | Checkpoint count | **DISABLE** (internal) | autosave surfaces instead |

**Counts: 15 SURFACE / 12 DISABLE** (27 rows; 26-27 are internal-use by
design). Every DISABLE row carries its reason in-product (reasonKey or a
docs row) and a BEHAVIOR-FINDINGS.md entry — no silent gaps, no fake
surfaces.
