# PARITY-DELTAS — session 97 (performance parity) — the architectural parity record

> The mandate §4: "Where the core diverges architecturally, the divergence is
> either removed or documented as a deliberate, evidenced improvement with
> measured justification." This sheet disposition's session-97's parity work:
> the re-grading of the 64-item checklist (worklog/REFERENCE-ARCHITECTURE.md
> §5), the session's own deltas, and the pointer into the repo's standing
> ledger (docs/PARITY-DELTAS.md) for the pre-existing registers.

---

## §1 The 9 [U] checklist items — dispositioned (every one checked in source this session)

| # | Item (REFERENCE-ARCHITECTURE §5) | Disposition | Evidence |
|---|---|---|---|
| 7 | Layout-IOCTL growing-buffer retry (1000 B + 640/retry) in enumeration | **[D] deliberate** — the core never uses the layout IOCTL for enumeration: it parses partition tables from device bytes (cr_table_parse) with its own PhysicalDrive + cfgmgr32 merge. The og's IOCTL walk and the core's byte-parse produce the same tables from the same bytes; the growing-buffer retry is an artifact of the IOCTL's variable output, a problem the byte-parse does not have. | rg: zero DRIVE_LAYOUT in src/win/cr_enumerate.cpp; the DELETE/SET layout IOCTLs in cr_transport.cpp are the quiesce chain, not enumeration |
| 11 | Writability probe + ERROR_WRITE_PROTECT short-circuit (destination arming) | **[DD] deliberate** — the core's DestIo surfaces write failures at first write with the OS's own error (the honest shape); the og's early probe fails at arming instead. Fail-later-with-the-same-error vs fail-earlier: no masked failure either way. Not adopted this session: adding an early probe changes the observable failure timing (a behavior change), and no consumer metric depends on it. | rg: zero WRITE_PROTECT in cr_destio.cpp; corpus cite Device_decompiled.c:52986-52997 |
| 24 | $MFTMirr record-1 $DATA-runlist fallback (two-layer MFT resilience) | **[M] matches** — `load_mft_stream(at_lcn, from_mirror)` routes the mirror LCN with the og's fallback shape; the mirror-invalid demotion is the registered D-52 hardening (a torn mirror LCN skips the mirror, keeps the open). | cr_bootstrap.cpp:85-94, :123-133 |
| 29 | exFAT "1 MiB FAT windows" | **[DD] core-specific** — the corpus does NOT document the og's FAT windowing (flagged by Agent A; CORRECTIONS-adjacent). The core's 1 MiB aligned windows (cr_exfat_quick.cpp:27-39) are a core-side choice, not an og-parity claim. Registered as such. | corpus absence (REFERENCE-ARCHITECTURE §2); core cite cr_exfat_quick.cpp:27-39 |
| 32 | EXT4 backup-superblock positions | **[D] core-own, more rigorous** — the corpus marks the og's backup-SB positions UNVERIFIED (CORRECTIONS #4); the core's grid is verified against real mke2fs images (the S-027 bounds comment, cr_ext4_engine.cpp:70-85) and capped at a 4 TiB coverage grid. The core must not (and does not) cite og parity here — its own verification is the evidence. | cr_ext4_engine.cpp:70-85 |
| 33 | EXT4 journal gate = magic + blocktype {3,4} only | **[M] matches** — "the jsb gates the WHOLE harvest (magic + blocktype {3,4} at journal block 0)", the S-004 gates intact, ESCAPE-tagged blocks additionally parse with the magic restored. | cr_ext4_engine.cpp:900-903 |
| 42 | RAW per-extension runtime toggles (og RawConfig.ini, 34 toggles) | **[DD] deliberate** — the core's carve registry is compiled (cr_rawsigns/cr_signatures) with the user-signature file as the only runtime extension surface (userdata-defaults.json); the og's ini toggles are a configuration layer the core's clean-room registry does not carry. No consumer metric depends on runtime toggling. | include/cr/carve/cr_rawsigns.h:4; data/userdata-defaults.json |
| 44 | Memory-adaptive batch sizing (GlobalMemoryStatus-driven) | **[DD] deliberate, measured** — the core uses fixed bounded batches everywhere (the 8 MiB cache, the 4 MiB remnant block, the 64 MiB caps); its working-set peak is flat at 53.7 MiB on an 8 GiB deepcarve (vs the driver era's 52.0 — run 41140212296 vs 41042267586 samples). Bounded-by-construction beats adaptively-bounded for the memory metric; no adaptive path is needed. | rg: zero GlobalMemoryStatus in src/core; PERFORMANCE.md §2.1 rows 13-14 |
| 55 | Candidate scoring by recoverable-file count + AdjustPartitionInfoOrder | **[M] matches** — the M-017 primary structural score, the AdjustPartitionInfoOrder rule (a) leader-promotion, the table-matched leader's 1000-permille cap. | cr_candidates.cpp:705-712, :1093, :1255-1298 |

Checklist re-tally after disposition: **[M] 35 · [D] 17 · [DD] 12 · [U] 0** — no
unknowns remain from the Phase-0 pass.

## §2 The session's own deltas (performance-session scoped)

1. **REMOVED (a divergence, not registered): the F12 cross-disk false
   refusal.** The pre-session arming refused recovery to any destination
   whose volume span numerically intersected the source extents ACROSS
   DISKS — a defect wearing a safety rule's clothes (worklog/
   RECALL0-DIAGNOSIS.md; 10,248/10,405 scenario items refused in both
   transport eras). The same-disk precondition restores the documented
   intent (run-ours.ps1's own "stays quiet by construction" comment; the
   CLI's on_source computation). The REAL same-disk refusal is intact and
   unit-pinned (test_transport_win.cpp + the existing writer suites).
2. **The harness's stale 'rda' transport expectation** — removed (the
   gate now asserts the truth: 'direct'). Not a core delta; recorded here
   because the mandate's "no reference to a driver that no longer exists"
   applies to the harness surfaces too.
3. **Kept (registered before, re-evidenced this session): the cache
   divergence N-045/D-72** — og 4 MiB + prefetch thread vs core 8 MiB
   reactive LRU. This session's evidence: the direct path EXCEEDS the
   driver on every wall (deepcarve −32%, CPU/GiB −39%) WITHOUT a
   prefetch thread — the divergence is measured as superior at the
   runner's profile; the og's prefetch model remains available as the
   F10 lever if a profile shows tail latency the reactive cache loses.
4. **Kept: the pool fan-out divergence (checklist #45, Agent A's new
   item)** — the og serializes all reads QD1 through one per-volume
   device; the core fans out CPU work (carve/remnant/candidates, ≤8
   workers) over the shared handle under the cache mutex. Measured
   superior (the walls above); the single-flight ABI contract is the
   kept invariant.
5. **Kept: the 64 KiB envelope** (vs the og's by-construction chunking)
   — checklist #4/#5, registered OQ-S4-3, re-evidenced: the sweep legs
   (seq 64K/256K/4M) now capture the profile every dispatch.

## §3 The peripheral-surface comparison (mandate §4's driver-surface parity)

| Surface | Driver guarantee (DRIVER-BASELINE §4) | Direct path today | Verdict |
|---|---|---|---|
| Geometry | passthrough IOCTL, status+Information verbatim (CI-asserted 1.0 fidelity) | probe_geometry: GET_LENGTH_INFO + geometry-EX + storage-query, N-206 ceil | EQUAL (unit-pinned) |
| Capabilities/identity | GET_INFO/GET_VERSION wire structs, size-frozen | transport string + geometry in api-result; the ABI v4 surface | EQUAL (ABI-pinned) |
| Error mapping | verbatim NTSTATUS, Information=0 on failure | verbatim Win32→core-code mapping, no masking; the N-125 disconnect classification (a superset the driver lacked) | EQUAL OR BETTER |
| Cancellation | CancelIo/close aborts a stuck read, STATUS_CANCELLED | cooperative CancelToken at engine loop heads; a stuck in-flight ReadFile is NOT interruptible (F2/T11 lever — CancelSynchronousIo would close this; see PLATFORM-CAPABILITIES G3) | GAP (registered, T11's scope) |
| Lifecycle | CLEANUP releases everything; unlimited shared handles | RAII closes; share R|W|D opens; concurrent opens unchanged | EQUAL |
| Read-size independence | any 512 B–16 MiB forwards as one IRP | the envelope + bounce preserves any length; unit-pinned | EQUAL |
| Alignment tolerance | no precondition (byte offsets forward verbatim) | the 64 KiB envelope absorbs any alignment (GAP-S6-13 protection) | EQUAL |

The one peripheral GAP (cancellation of an in-flight read) is T11's scope:
the driver could abort a stuck read via CancelIo on its IRP; the direct
path's synchronous ReadFile cannot be interrupted except by
CancelSynchronousIo from another thread (the PLATFORM-CAPABILITIES G3
lever). The interactive cancellation latency (cancel_to_stop_ms, now
measured by the harness) decides whether this matters at real disk
latencies — the in-flight read of a 256 KiB block at ~2 ms is far below
perceptibility, but a dying disk's 30 s timeout is not.

## §4 The pre-existing registers (unchanged this session, pointer only)

docs/PARITY-DELTAS.md carries the standing ledger (the N-xxx/D-xxx
registers: N-045/D-72 cache, D-52 mirror hardening, OQ-S4-3 chunking,
D-121 retry window, the S-052 LDM basis, etc.). This session added no new
standing registers beyond §2 above; the checklist re-grades (§1) fold
into REFERENCE-ARCHITECTURE.md's tally on its next revision.
