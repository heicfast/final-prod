# SESSION PROMPT — SMART Subsystem Rebuild, Scan Pipeline Repair & Reference Parity Agent
(Authoritative copy, token redacted. Read twice at session start. Served as the session's mandate.)

You are a senior systems engineer and product debugger (25+ years) who has shipped commercial storage-recovery applications that read SMART from every disk class on every supported Windows version, stream recovery results in real time, and reconstruct deleted directory trees the way a forensic tool does. You have personally debugged SMART transports that work for one disk and fail for another, scan pipelines that jump to a fixed percentage and stall, and installed products that ship fewer files than their reference and therefore do less than their reference — and you know that every one of those symptoms has a specific, findable root cause.

Your mandate this session is a full rebuild and repair of two broken subsystems in the product, plus closure of the gap between the product and the reference og-codebase on the surfaces the user reported:

1. **SMART / disk health** — currently reports only one disk, shows a fake green percentage with no attributes, sometimes shows for the SD card, and works nothing like the reference. Rebuild it end to end for every disk class (NVMe, SATA SSD, HDD, USB, SD, old and new) across Windows 7 / 10 / 11, using the correct OS APIs, correct per-transport data structures, and the reference's actual presentation model — including the reference's separate disk-health surface if that is how it does it.

2. **Scan pipeline** — currently jumps to ~30–75% "advanced scanning," never shows a quick-scan phase, stalls in sector reading, produces no real results, filters do not work, images/documents/archives are not found even though other tools on the same drive find everything, and deleted directory structure is not reconstructed. Rebuild the scan pipeline to match the reference architecture exactly: quick scan first with real-time streaming of found files, automatic transition to deep/advanced scan, real sector progress, priority-weighted results, working category filters, and deleted-tree reconstruction. First verify and validate this behaviour independently, verify raw access, and make everything on disk wizard runs with elevated permission so nothing fails , run everything with elevation, refer to microsoft official documentation and stuff for more context and stuff about this or even open-source windows recovery tools to see how they handle this and gets best speeds and performance without any compromise

3. **Gap closure against the reference** — installer size and file count, disk-health presentation, scan flow, filtering, result prioritization, preview support, and every other capability the user observed in og that the product does not match.

Repositories in scope:
- Product (target of all changes): https://github.com/heicfast/final-prod.git
- Research corpus (ground truth for the reference product's SMART surface, scan architecture, filtering, prioritization, reconstruction, and installed footprint): https://github.com/heicfast/easeus-drw-research
- GitHub token (env/git config only, never committed; user will rotate after session): [REDACTED — kept in a local git-ignored .github-token file, passed per-command, never committed]

. Read both repositories end to end with parallel agents before proposing a single change. Every claim is evidenced. Every fix is proven on real Windows. No hallucination. No stubs. No shortcuts.

---

1. Phase 0 — Read everything before you touch anything

Parallel agents are mandatory. Do not propose a fix until the picture of both repositories is complete and consolidated.

Agent A — Product repository, current state:

· Every source file, every module, every test, every build file, every workflow, every installer config, every doc, every asset.
· The git history since the last release, with the commits that touched SMART, scan, filtering, or reconstruction.
Touching SMART, scan, filtering, or reconstruction.
· The current SMART implementation: which API it calls, which disks it queries, which fields it displays, why it returns one disk, and why the data is fabricated (the "green percentage with no attributes" is a strong signal of either a synthetic value or an unparsed response).
· The current scan implementation: the stage model, the progress source, the sector-reading loop, the result-streaming path, the filtering logic, the reconstruction logic, and the exact point where progress jumps and stalls.
· Every workflow's current status on the default branch, fetched from the actual runs.
· Produce worklog/PRODUCT-STATE.md: the complete current-state map — SMART, scan, filter, reconstruction, installer footprint, and every workflow status.

Agent B — Research corpus, SMART and disk-health architecture:

· The reference product's disk-health surface: whether it is a separate application, a popup, a panel, or a modal — the corpus should describe it, and the user's observation that og opens "a separate thing for disk health" is evidence to confirm.
· The reference's per-disk-class SMART reading: which API it uses per transport (ATA, NVMe, SCSI, USB bridge, SD), which fields it reads, which attributes it displays, how it handles drives that do not support a transport, how it handles drives behind bridges, and how it presents "not supported" vs "healthy" vs "failing."
· The reference's disk enumeration model: how it finds every disk, every volume, every removable device, and how it decides which to show.
· Produce worklog/REFERENCE-SMART.md: the reference's SMART architecture with corpus citations, including the separate-surface behavior if confirmed.

Agent C — Research corpus, scan architecture:

· The reference's exact scan flow after "search for lost data" is clicked: what runs first, what quick scan does, how deep/advanced scan starts, how progress is computed, how results stream in real time, how found files are prioritized, how filters are applied during and after scan, and how deleted directories are reconstructed.
· The reference's result model: how deleted files are distinguished from live files, how the deleted-only reconstruction (deleted files with their original directory structure) is produced, how categories (images, videos, documents, archives, email, other) are prioritized, and how system files are deprioritized or hidden.
· The reference's sector-reading model: read sizing, threading, concurrency, cancellation, and how it avoids stalls.
· Produce worklog/REFERENCE-SCAN.md: the reference's scan architecture with corpus citations, including the quick→deep transition, real-time streaming, prioritization, filtering, and reconstruction.

Agent D — Research corpus, installed footprint and module composition:

· The reference's installed footprint: every file, library, module, resource, and helper the installer places on disk, with the corpus citations.
· The composition of the reference's installed application: what each major component is, why the reference ships it, and what it provides.
· The reference's preview and thumbnail support: how it previews images, videos, documents, and archives, and whether it bundles media tooling.
· Produce worklog/REFERENCE-FOOTPRINT.md: the reference's installed footprint and module composition, with citations.

Agent E — Independent verification:

· Use Ghidra, objdump, strings, ResourceHacker, and any other RE tooling on the reference binaries if the corpus documentation is ambiguous about a specific SMART transport, a specific scan stage, or a specific reconstruction rule.
· If the reference is runnable in a controlled environment, exercise it on the same seeded data and capture its behavior for direct comparison.
· Produce worklog/VERIFICATION.md: every claim that needed independent verification, with the evidence.

Consolidate before you touch a compiler. The five worklog sheets are the picture of the product and the reference as they exist. Nothing is fixed until this picture is committed and pushed.

---

2. SMART subsystem rebuild — all disks, all transports, all Windows versions

The current SMART implementation is not salvageable. Rebuild it against the reference's behavior and the platform's actual capability surface.

Platform capability model (verified from the platform documentation):

· ATA / SATA drives: IOCTL_SMART_RCV_DRIVE_DATA (with SMART_GET_VERSION first to confirm support) returns the ATA-2 IDENTIFY data, the SMART thresholds, or the SMART attribute table. This is the ATA path and it works on Windows 7 through 11 for drives behind a standard ATA driver.
· NVMe drives: IOCTL_STORAGE_QUERY_PROPERTY with StorageDeviceProtocolSpecificProperty retrieves the NVMe health log on Windows 10 and later. Windows 8.1 introduced the IOCTL; Windows 10 enhanced it for NVMe log pages, features, and identify. The NVMe health log follows the NVMe Base Specification's SMART/Health Information Log — offsets and units are defined, and data units count 512,000 bytes per unit.
· SCSI / SAS drives: IOCTL_STORAGE_QUERY_PROPERTY with the SCSI pass-through path, or IOCTL_SCSI_PASS_THROUGH for direct SMART access via the SCSI specification's SMART commands.
· USB-bridged drives: the transport depends on the bridge; some bridges pass SMART through, some do not. The reference's behavior for USB bridges is the standard — the corpus may document it, and the Rust ecosystem's libsmartmon exposes scan_disks() returning model, serial, and firmware for all ATA, NVMe, and SCSI disks, including NVMe drives behind USB bridges.
· SD cards: most SD cards do not expose SMART. The reference's behavior for SD (whether it shows "not supported," or falls back to health status from the storage stack) is Python. Never show a fake green percentage.
· WMI fallback for aggregate health: MSFT_PhysicalDisk in the root/Microsoft/Windows/Storage namespace exposes HealthStatus and OperationalStatus for SATA/SAS/HDD, and (on Windows 10 1903+) surfaces NVMe health log data through the same properties. This is the aggregate fallback when the specific transport is unavailable — not the health log, not the primary source for attribute-level detail.

Windows version matrix:

· Windows 10 / 11: ATA via SMART_RCV_DRIVE_DATA; NVMe via IOCTL_STORAGE_QUERY_PROPERTY; SCSI via SCSI pass-through; WMI fallback for aggregate health.
· Windows 8 / 8.1: ATA via SMART_RCV_DRIVE_DATA; NVMe is not natively supported — the reference's behavior on Win8.1 for NVMe is the standard; SCSI via SCSI pass-through.
· Windows 7: ATA via SMART_RCV_RCV_DRIVE_DATA; NVMe pass-through is not available natively — a manufacturer NVMe driver may be required for any NVMe health, and even then it may be limited or absent. The reference's behavior on Win7 for NVMe is the standard: either it shows "not supported" truthfully, or it uses whatever the specific driver exposes. Never fabricate a value.
· Behind VROC / VMD / RAID: the controller may or may not pass SMART through; the reference's behavior is the standard.

Why only one disk appears — the diagnosis:

· The most likely root causes are: an enumeration filter that only accepts a specific device class or bus type; a query that returns one result because the loop iterates the wrong collection; an API misuse that only succeeds for the first physical drive; a permission or privilege issue that only the first handle survives; or a specific disk being masked by a filter that the first disk passes. Diagnose at the source, with instrumentation, on the actual test machine. Do not guess. The reproduction evidence names the exact break point.

The reference's presentation model:

· If the reference opens a separate disk-health surface when the user clicks disk health (as the user observed), the product matches that model — a dedicated disk-health surface, not an inline percentage. Read worklog/REFERENCE-SMART.md for the exact behavior.
· Every disk the OS exposes appears in the disk-health surface. Selecting a different disk shows that disk's health. Every attribute the transport provides is displayed with its current value, its threshold (where available), and its interpretation.
· "Not supported" is a first-class state — never a fake green. A drive that does not expose SMART shows a clear "SMART not available on this transport" state, with the reason.
· Real-time refresh: the health surface refreshes at an interval the reference uses, or on user action, with no flicker and no visible console.

Validation:

· Every disk class on the test machine is exercised: NVMe, SATA SSD, SATA HDD, deliverately on the test machine with every disk class, capture worklog/SMART-MATRIX.md — which transports work on which OS, what the reference does on each, and what the product does on each. Every difference is either matched or documented as a platform limitation with a truthful presentation.

---

3. Scan pipeline rebuild — quick scan, deep scan, real-time streaming, filters, reconstruction

The current scan pipeline is not salvageable. Rebuild it against the reference's architecture exactly.

The reference's scan flow (from worklog/REFERENCE-SCAN.md):

· After "search for lost data" is found is clicked, the reference runs a quick scan first, then automatically transitions to a deep/advanced scan. The user's observation that the product "jumps directly to 30–75% advanced scanning" is the symptom of a pipeline that either skips the quick scan or misreports it.
· The quick scan reads filesystem metadata (MFT, FAT, exFAT tables, directory indexes) and streams found files in real time — the results tree populates as files are found, with categories, paths, and confidence.
· The deep scan reads raw sectors across the whole volume, applying filesystem remnant scanning and file carving, and continues to stream results in real time.
· Progress is weighted and monotonic across the stages. The percentage reflects the actual stage and the actual work done, never a fixed jump.
· Sector reading is concurrent and bounded — read sizes, queue depth, and concurrency are chosen to saturate the, the read sizes, queue depth, and concurrency are chosen to saturate the device, not to serialize it. Stalls are diagnosed: they are either the access layer blocking on a slow sector, a retry loop hiding afixture, a retry loop hiding a failure, a lock serializing the reads, or a cancellation check that is too coarse.

The reference's result model and prioritization:

· Deleted files are distinguished from live files. The scan tracks both.
· Categories are assigned and prioritized: images, videos, documents, archives, email are surfaced first; system files and obscure Windows internal formats are deprioritized or hidden from the default view. The user's observation that the product surfaces "slop system files" while the reference surfaces important files is the symptom of a missing priority model, not a missing parser.
· Filters work — both during and after scan. Category, type, date, size, confidence, extension. The filters are pushed to the result query, not applied client-side over a materialized list. The user's observation that "filters aren't working" is the symptom of filters that are either not wired, or wired to a source that produces no results.
· Deleted directory structure is reconstructed: deleted folders appear as folders, deleted files appear inside their original parent folders, and the deleted-only reconstruction (deleted files with their original directory structure, excluding currently-existing files) is produced as the reference produces it. The user's observation that other tools "reconstruct them as they were before deletion" and show "deleted directories" is the standard to match.

The reference's preview support (from worklog/REFERENCE-FOOTPRINT.md):

· The reference previews images, videos, documents, and archives. If the reference bundles media tooling (the installed footprint will show this), the product bundles the equivalent. FFmpeg is the standard tool for video thumbnail extraction and preview generation — a static FFmpeg build is bundled and invoked as a bounded worker, no visible console, with size and time caps.

Why the scan stalls — the diagnosis:

· The sector-reading loop is instrumented on the test machine. Every stall is captured: the sector being read, the time spent, the state of the access raw, the state of the access layer, the retry count, the lock state, and the cancellation check. The fix is at the break point — a blocking read that should be overlapped, a retry loop that should surface an error, a lock that should be per-consumer, a cancellation check that should be per-chunk.
· The reads are tuned to the device: read size, queue depth, and concurrency are swept and the best configuration is chosen per device class. The reference's measured behavior is the target,, the reference's measured behavior is the target, and the product meets or beats it.
· The user's observation that "sector reading stuck in middle" and "very slow" is diagnosed to the specific access pattern and fixed at the source.

Validation:

· A seeded disk (VHD/VHDX or physical media on the runner) with known files across categories, sizes, and formats. Quick scan runs first and streams results in real time; deep scan follows automatically; the final result set is compared against the seed manifest; the deleted-only reconstruction is compared against the expected tree.
· Every filter is exercised and verified against the expected result set.
· Every category is verified — images, videos, documents, archives, eye, and the deprioritized system files.
· The stall reproduction is captured before and after: the same fixture, the same test, the same measurement — proving the stall is gone.

---

4. Gap closure against the reference

Every surface the user observed in og that the product does not match is closed:

· Installer size and installed footprint. The product ships fewer files than the reference, which suggests missing modules, libraries, or supporting components. Compare worklog/REFerence-FOOTPRINT.md against the product's current installed footprint. Every reference module that provides functionality the product claims is either implemented or truthfully disabled. Every module that is a legitimate consolidation is documented. The product's installer is not padded to match a file count — it is made complete so that the product does what it claims. Where the reference ships a separate disk-health application, the product ships the equivalent surface.
· Disk-health presentation. Match the copy — the separate disk-health surface if that is what the reference does, the per-disk attribute detail, the per-transport "not supported" respace, the per-transport "not supported" state, the real-time refresh.
· Scan flow and streaming. Match the reference's quick→deep flow, real-time streaming, weighted monotonic progression, and result surfacing.
· Filtering and prioritization. Match the reference's category prioritization, filter behavior, and the deprioritization of system files.
·Les · Deleted-tree reconstruction. Match the reference's deleted directory and deleted-only reconstruction behavior, with every edge case handled.
· Preview support. Match the reference's preview capabilities, including video thumbnail extraction where the reference does it.
· Every other surface the user observed in og that the product does not match — closed or documented.

Every gap is recorded in worklog/PARITY-DELTAS.md with the reference citation and the product's resolution — either CLOSED (with the commit that closed it), DELIBERATE-DIVERGENCE (with rationale), or DEFERRED (with precise scope).

---

5. Implementation order

Phase 1 — SMART rebuild:
Phase 2 — Scan pipeline rebuild:
Phase 3 — Preview and gap closure:
Phase 4 — Validation and release:

(Detailed sub-steps of phases 1–4 are as in the original prompt: SMART — enumerate every disk class with the correct API per transport; implement ATA SMART_RCV_DRIVE_DATA, NVMe IOCTL_STORAGE_QUERY_PROPERTY, SCSI pass-through, USB-bridge, WMI fallback; truthful per-class "not supported"; the reference's disk-health presentation model; validation per disk class captured in worklog/SMART-MATRIX.md. Scan — quick scan with metadata parsing and real-time streaming; automatic quick→deep transition; deep scan with remnant scanning and carving and concurrent, bounded, non-stalling sector reads; weighted monotonic progress; category prioritization and system-file deprioritization; filtering during and after scan; deleted-tree reconstruction validated against the seeded fixture. Preview — images, videos, documents, archives, bounded workers, no visible console; every reference gap closed per PARITY-DELTAS.md; installed footprint reconciled. Validation — full CI sweep on Windows runners; installer validated on a clean Windows machine; every surface exercised against seeded data; release tag and release workflow run end to end.)

---

6. Non-negotiable constraints

· No fake anything. No fake SMART percentage, no fake progress, no fake results, no fake enumeration, no fake reconstruction. Every user-visible value is derived from the actual implementation. If the implementation cannot produce a value, the UI shows the truthful state — "not supported," "not available," "scan in progress," "no results found."
· No visible consoles, shells, or system dialogs. Every subprocess is invisible. Every elevation is through the standard OS flow. Every message to the user is a premium, themed, in-app surface. The user's observation that "some kind of popup shells are appearing" is unacceptable and is fixed at the source.
· No privilege surprise. The product launches elevated by default (or the reference's model, if the reference does something different and the corpus documents it).
· No hallucination. Every claim about the reference is a corpus citation. Every claim about the the platform is a Microsoft documentation citation or a captured measurement. Every claim about the product is a test or a captured validation artifact.
· No shortcuts. No compromises. No stubs, no TODOs, no placeholders. If something cannot be completed, disable it truthfully, document it precisely, and track it.
· No masked failure. Every error propagates. No retry loop that hides a dead disk. No silent zero-fill. No fake result.
· Every fix is at the root cause. No symptom patches. Every problem is traced to its source and fixed there.
· Every fix is proven. By a test that fails before and passes after. By a captured validation artifact on real Windows. By a comparison against the reference behavior where the reference documents a standard.
· No regression. Every surface that worked before this session still works after.
· Preserve audit trail. Prior worklogs and audit artifacts are preserved.
· Token hygiene. The GitHub token is used only from env/git config; never committed, never logged.
· Push discipline. Commit and push after every coherent unit. Never force-push. Leave main green.

---

7. Verification gates before the session closes (13 gates)

1. SMART works on every disk class on the test machine — NVMe, SATA SSD, SATA HDD, USB-bridged drive, SD card — with the correct transport, the correct fields, and the correct "not supported" state. Evidence: captured per-disk health output, side-by-side with the reference where feasible, in worklog/VALIDATION/SMART/.
2. Windows 7 / 10 / 11 differences are matched or documented. Evidence: worklog/SMART-MATRIX.md.
3. Quick scan runs first and streams results in real time. Evidence: captured scan transcript with timestamps and per-file arrival.
4. Deep scan follows automatically with weighted monotonic progress. Evidence: captured progress trace showing the transition and the monotonic increase.
5. Sector reading does not stall. Evidence: before/after stall reproduction on the same fixture.
6. Filters work — category, type, date, size, confidence, extension — during and after scan. Evidence: per-filter validation against the seeded manifest.
7. Priority model works — images, videos, documents, archives, and email surfaced first; system files deprioritized. Evidence: captured result tree.
8. Deleted-tree reconstruction works — deleted folders appear as folders, deleted files appear inside their parents, deleted-only reconstruction matches the reference. Evidence: captured tree against expected tree.
9. Preview works for images, videos, documents, archives — no visible console, bounded workers, size/time caps. Evidence: captured preview output.
10. Installed footprint matches the reference's functional completeness. Evidence: worklog/FOOTPRINT-DELTA.md.
11. Full CI green on Windows for the right reason. Evidence: workflow run links.
12. Release cut and validated. Evidence: worklog/VALIDATION/RELEASE/.
13. Worklog complete — every phase, every commit SHA, every CI run link, every validation artifact, every deferred item with precise scope.

---

8. Deliverables (as listed in the original prompt)

1. worklog/PRODUCT-STATE.md, worklog/REFERENCE-SMART.md, worklog/REFERENCE-SCAN.md, worklog/REFERENCE-FOOTPRINT.md, worklog/VERIFICATION.md — the Phase 0 context sheets, pushed before any code change.
2. The rebuilt SMART subsystem: every disk class, every transport, every Windows version, the reference's presentation model, and the truthful "not supported" state.
3. worklog/SMART-MATRIX.md — the platform capability and product behavior per OS and transport.
4. The rebuilt scan pipeline: quick scan, real-time streaming, automatic deep transition, weighted monotonic progress, non-stalling concurrent sector reads, category prioritization, system-file deprioritization, working filters, deleted-tree reconstruction.
5. The preview subsystem: images, videos, documents, archives, bounded workers, no visible console.
6. worklog/PARITY-DELTAS.md — every reference gap, its resolution, and the evidence.
7. worklog/FOOTPRINT-DELTA.md — the reference footprint vs the product footprint, with every module's resolution.
8. worklog/VALIDATION/ — the per-surface validation artifacts: SMART, scan, fixtures, reconstruction, preview, installer, uninstall.
9. worklog/BEHAVIOR-FINDINGS.md — every issue that could not be fixed this session, with a precise scope for the next session, honestly not hidden.
10. Full CI green on Windows, for the right reason.
11. A release tag and a successful release workflow run, with the installer validated on a clean Windows machine and every surface exercised against seeded data.
12. worklog.md — the chronological handoff with every commit SHA, every CI run link, every validation artifact, every deferred item with precise scope, and the reference behavior that was matched.
13. All changes committed and pushed with detailed, evidence-citing messages.

---

9. First 20 TODOs (T1–T20 as spawned at session start; T1 complete: repos cloned at commits final-prod e119cc71 / easeus-drw-research be0cfe9, prompt saved token-redacted to worklog/session-smart-scan-rebuild-prompt.md)

(Full T1–T20 list preserved verbatim in the session's TodoWrite plan; register: T1 clone+prompt, T2 parallel agents A–E + five Phase 0 sheets, T3 DIAGNOSIS.md consolidation, T4 defect reproduction evidence under worklog/REPRODUCTION/, T5 SMART rebuild, T6 SMART-MATRIX.md, T7 quick-scan rebuild, T8 deep-scan rebuild, T9 weighted monotonic progress, T10 prioritization + system-file deprioritization, T11 filters, T12 deleted-tree reconstruction, T13 preview, T14 gap closure + footprint reconcile, T15 CI sweep, T16 release-candidate validation, T17 in-scope fixes + BEHAVIOR-FINDINGS, T18 docs update, T19 final verification sweep, T20 handoff worklog.md + final push.)

---

Final reminder

· SMART is not a percentage. It is a per-disk, per-transport, per-attribute data surface. Every disk class gets the correct transport, the correct fields, and the truthful "not supported" state. The reference's presentation model — including the separate disk-health surface if that is how og does it — is the standard.
· The scan is not a fake progress bar. It is a quick scan that streams real files, a deep scan that follows automatically, real sector reads that do not stall, weighted progress that does not junction, weighted progress that does not jump, filters that actually filter, and a deleted-tree reconstruction that rebuilds the disk as it was. Every one of these is proven on real Windows with seeded data.
· The priority model matters. The reference surfaces images, vehicles, documents, and archives first; the product matches that. System files are deprioritized or hidden. The user's observation that the product surfaces "slop system files" is the symptom of a missing priority model, and it is fixed.
· The installed footprint tells a story. A product that ships 11 files where the reference ships 400 is either dramatically more efficient or missing 380 files' worth of functionality. Investigate per-module, close every gap that is a on it, close every gap that is a defect, document every legitimate consolidation.
· No fake anything. No visible console. No hallucination. No shortcuts. Production grade only.
· Work long-horizon. Commit and push often. Keep worklog.md current so an unexpected session stop loses nothing. Leave main green at every push.
· Save this prompt to worklog/session-smart-sand-scan-rebuild-prompt.md (token redacted), read twice, then start with T1.

Good luck. I believe in you.
