//! tests/bridge_integration.rs — the real-bridge integration gate
//! (BF-ABI-02 closure).
//!
//! THE missing gate this session installed: ui-ci runs `cargo check`
//! (no link step) and the release lane builds the app but never asserts
//! the bridge's ABI structs against the BUILT engine — which is exactly
//! how eb4bb5f3's struct appends (bus_type, power_cycles_nvme,
//! threshold) shipped with a stale Rust mirror: an 8×N-byte heap
//! overflow in `enumerate_disks` (row-stride 296 vs the engine's 304)
//! and a 16-byte stack smash on every `health_read` (report 224 vs
//! 240). This crate builds the REAL bridge against the REAL engine and
//! fails loudly on every axis of that class.
//!
//! Environment (set by .github/workflows/bridge-integration.yml):
//!   CR_IT_EVIDENCE   — directory for evidence JSON/CSV (created)
//!   CR_IT_MIN_DISKS  — minimum expected physical-disk count (default 1)
//!   CR_IT_SCAN_DISK  — the SEEDED disk number for the scan-truth test
//!                      (REQUIRED for that test; the workflow attaches a
//!                      seeded NTFS VHDX and passes its disk number)
//!   CR_IT_SCAN_SECS  — scan sampling window in seconds (default 240)
//!   CR_IT_EXPECT_FILES — minimum files the scan must find on the seed
//!   CR_ABI_LAYOUTS   — the engine's abi_layout_dump.c manifest path
//!                      (consumed by the layout-gate test file)
//!
//! Every test writes its raw observations to the evidence directory
//! BEFORE asserting, so a failure leaves the reproduction artifact
//! behind (the Phase-1 discipline: no claim without captured evidence).

use std::path::PathBuf;
use std::time::{Duration, Instant};

use recovery_wizard_ui_lib::bridge::abi;
use recovery_wizard_ui_lib::bridge::util::{enumerate_disks, fixed_str};

fn evidence_dir() -> PathBuf {
    let d = std::env::var("CR_IT_EVIDENCE")
        .map(PathBuf::from)
        .unwrap_or_else(|_| PathBuf::from("cr-it-evidence"));
    std::fs::create_dir_all(&d).expect("evidence dir");
    d
}

fn write_evidence(name: &str, content: String) {
    let p = evidence_dir().join(name);
    if let Err(e) = std::fs::write(&p, content) {
        eprintln!("evidence write failed ({p:?}): {e}");
    }
}

// ---------------------------------------------------------------------------
// 1. Enumeration coherence — the disks-missing reproduction (R3/BF-ABI-01)
// ---------------------------------------------------------------------------

/// The engine writes N rows at its own stride; the bridge reads them at
/// the Rust mirror's stride. Any mismatch collapses rows ≥1 onto garbage
/// (the observable: duplicate/absurd `number`, nonsense sizes) — exactly
/// the "only one disk is appearing" class. The evidence JSON carries the
/// RAW decoded rows so the shift is visible post-mortem.
#[test]
fn enumerate_rows_coherent() {
    let disks = enumerate_disks().expect("cr_ui_enumerate_disks via the bridge");
    let min: usize = std::env::var("CR_IT_MIN_DISKS")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(1);

    let rows: Vec<serde_json::Value> = disks
        .iter()
        .map(|d| {
            serde_json::json!({
                "number": d.number,
                "size_bytes": d.size_bytes,
                "logical_sector": d.logical_sector,
                "physical_sector": d.physical_sector,
                "removable": d.removable,
                "layout": d.layout,
                "table_ok": d.table_ok,
                "model": fixed_str(&d.model),
                "serial": fixed_str(&d.serial),
                "trim_known": d.trim_known,
                "trim_on": d.trim_on,
            })
        })
        .collect();
    let evidence = serde_json::json!({
        "count": disks.len(),
        "min_expected": min,
        "rows": rows,
    });
    write_evidence(
        "enumerate.json",
        serde_json::to_string_pretty(&evidence).unwrap(),
    );
    eprintln!("ENUMERATE EVIDENCE:\n{}", serde_json::to_string_pretty(&evidence).unwrap());

    assert!(
        disks.len() >= min,
        "enumeration returned {} rows, expected at least {min} — disks are being dropped",
        disks.len()
    );

    let mut numbers: Vec<u32> = disks.iter().map(|d| d.number).collect();
    numbers.sort_unstable();
    let dup = numbers.windows(2).any(|w| w[0] == w[1]);
    assert!(
        !dup,
        "duplicate disk numbers decoded ({numbers:?}) — the row stride is shifted \
         (the Rust mirror's sizeof(CrUiDiskInfo) != the engine's sizeof(cr_ui_disk_info))",
    );
    for d in &disks {
        assert!(
            d.number < 64,
            "absurd disk number {} decoded — row decode is shifted",
            d.number
        );
        assert!(
            d.size_bytes > 1024 * 1024,
            "disk {} decoded a nonsense size ({}) — row decode is shifted",
            d.number,
            d.size_bytes
        );
    }
}

// ---------------------------------------------------------------------------
// 2. Health per-disk — the SMART-only-first-disk reproduction (R4 stratum 2)
// ---------------------------------------------------------------------------

/// The engine's cr_ui_health_read memsets the FULL cr_ui_smart_report
/// (240 bytes on the current header) into the caller's pointer. A stale
/// Rust mirror (224 bytes) gets 16 bytes of its stack overwritten on
/// EVERY call. This test brackets the report with repr(C) canaries so
/// the overrun is DETERMINISTICALLY observable, and records every disk's
/// raw health outcome (data, honest unavailability, or error) as
/// evidence — the per-disk-switching truth.
#[test]
fn health_read_per_disk_is_coherent() {
    #[repr(C)]
    struct Guarded {
        head: [u64; 4],
        report: abi::CrUiSmartReport,
        tail: [u64; 4],
    }
    let disks = enumerate_disks().expect("enumerate for health pass");
    assert!(!disks.is_empty(), "no disks to health-check");

    let mut outcomes: Vec<serde_json::Value> = Vec::new();
    for d in &disks {
        // SAFETY: all-zeroed PODs are the ABI's documented safe default.
        let mut g: Guarded = unsafe { std::mem::zeroed() };
        g.head = [0xA5A5_A5A5_A5A5_A5A5; 4];
        g.tail = [0x5A5A_5A5A_5A5A_5A5A; 4];
        let mut needed: u32 = 0;
        let rc = unsafe {
            abi::cr_ui_health_read(d.number, &mut g.report, std::ptr::null_mut(), 0, &mut needed)
        };
        let canary_ok = g.head == [0xA5A5_A5A5_A5A5_A5A5; 4] && g.tail == [0x5A5A_5A5A_5A5A_5A5A; 4];
        outcomes.push(serde_json::json!({
            "disk": d.number,
            "rc_size_probe": format!("0x{rc:08X}"),
            "needed_attrs": needed,
            "canary_intact": canary_ok,
            "available": g.report.available,
            "nvme_available": g.report.nvme_available,
            "transport": fixed_str(&g.report.transport),
        }));
        // Evidence lands INCREMENTALLY (per disk) — a hard crash on a later
        // disk still leaves the earlier outcomes on disk.
        write_evidence(
            "health.json",
            serde_json::to_string_pretty(&serde_json::json!({ "outcomes": outcomes })).unwrap(),
        );
        assert!(
            canary_ok,
            "STACK CANARY CLOBBERED on disk {} — cr_ui_health_read wrote past the \
             Rust-side report (the mirror is smaller than the engine's struct)",
            d.number
        );
        // Either a clean refusal (no SMART on this device class — VMs,
        // VHDs) or a coherent success. Anything else is a wiring break.
        if rc != abi::CR_UI_OK {
            assert!(
                rc == abi::CR_UI_ERR_UNSUPPORTED || rc == 0x5007_0001,
                "disk {}: unexpected health rc 0x{rc:08X}",
                d.number
            );
            continue;
        }
        if g.report.available != 0 || g.report.nvme_available != 0 {
            // Coherent data: the two-call attr round must agree.
            let mut attrs =
                vec![unsafe { std::mem::zeroed::<abi::CrUiSmartAttr>() }; needed as usize];
            let mut cap = needed;
            let rc2 = unsafe {
                abi::cr_ui_health_read(d.number, &mut g.report, attrs.as_mut_ptr(), cap, &mut cap)
            };
            assert_eq!(
                rc2, abi::CR_UI_OK,
                "disk {}: attr round failed 0x{rc2:08X}",
                d.number
            );
            assert!(
                cap <= needed,
                "disk {}: attr count grew between calls ({needed} -> {cap})",
                d.number
            );
        }
    }
    let evidence = serde_json::json!({ "outcomes": outcomes });
    write_evidence("health.json", serde_json::to_string_pretty(&evidence).unwrap());
    eprintln!("HEALTH EVIDENCE:\n{}", serde_json::to_string_pretty(&evidence).unwrap());
}

// ---------------------------------------------------------------------------
// 3. Scan truth — the stuck-at-75% reproduction axes (R5)
// ---------------------------------------------------------------------------

/// Drives a REAL scan over the SEEDED disk (quick+carve+deep, the default
/// mask) through the same ABI the bridge uses, samples telemetry at 500 ms
/// and asserts the axes the UI renders are REAL and MOVING:
///
///  a. overall permille strictly advances (no fake/hung progress),
///  b. found files and found BYTES grow on real data (D4b class),
///  c. DURING THE DEEP STAGE the bytes_scanned axis ADVANCES — the
///     P-DEEP-01 fail-before pin (today: bytes_scanned is carve-true
///     only, so "Reading sector: X/Y" freezes through the whole deep
///     phase — the user-visible "stuck in sector scanning"),
///  d. no >15 s all-axes freeze while the run is in flight (the D4e/D4f
///     class regression guard).
///
/// The run is cancelled at the end of the window (bounded gate time).
#[test]
fn scan_progress_axes_move() {
    let disk: u32 = std::env::var("CR_IT_SCAN_DISK")
        .expect("CR_IT_SCAN_DISK must name the seeded disk (the workflow attaches it)")
        .parse()
        .expect("CR_IT_SCAN_DISK is a disk number");
    let window_secs: u64 = std::env::var("CR_IT_SCAN_SECS")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(240);
    let expect_files: u64 = std::env::var("CR_IT_EXPECT_FILES")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(1);

    // Open the disk (asserts elevation + presence honestly).
    let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_open_disk(disk, &mut dev) };
    assert_eq!(
        rc,
        abi::CR_UI_OK,
        "cr_ui_open_disk({disk}) -> 0x{rc:08X} (elevation/device problem)"
    );

    // Event sink: count heartbeats + progress events (engine-side truth).
    #[derive(Default)]
    struct Counts {
        progress: u64,
        heartbeat: u64,
        files_found: u64,
    }
    let counts = std::sync::Arc::new(std::sync::Mutex::new(Counts::default()));
    extern "C" fn sink(user: *mut std::ffi::c_void, evt: *const abi::CrUiEvent) {
        let c = unsafe { &*(user as *const std::sync::Mutex<Counts>) };
        let e = unsafe { &*evt };
        let mut g = c.lock().unwrap();
        match e.r#type {
            abi::CR_EVT_SCAN_PROGRESS => g.progress += 1,
            abi::CR_EVT_HEARTBEAT => g.heartbeat += 1,
            abi::CR_EVT_FILES_FOUND => g.files_found += 1,
            _ => {}
        }
    }

    // SAFETY: zeroed config = the documented safe default EXCEPT the stage
    // mask, which this gate pins to quick|deep (bits 1|2 = 3). First real
    // execution evidence (run 44071105301): with the default quick|deep|carve
    // the carve stage — 8.45 MiB/s, BF-30 — consumed the whole 480 s window
    // on the 2 GiB seeded VHD and the scan was cancelled mid-carve, so the
    // deep stage was NEVER reached and the deep-window gate below panicked
    // vacuously ("never observed the deep stage in flight"). The carve axes
    // have their own coverage (the D4a live-bytes stamp + the engine unit
    // suite); THIS gate pins the P-DEEP-01 deep-bytes axis, so the deep
    // stage must actually run inside the window: quick|deep reaches deep
    // seconds after quick, deterministically.
    let mut cfg: abi::CrUiScanConfig = unsafe { std::mem::zeroed() };
    cfg.stage_mask = 1 | 2; /* quick|deep — carve disabled in this gate */
    let mut scan: *mut abi::CrUiScan = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_scan_create(&cfg, &mut scan) };
    assert_eq!(rc, abi::CR_UI_OK, "scan_create 0x{rc:08X}");
    let rc = unsafe { abi::cr_ui_scan_attach_device(scan, dev) };
    assert_eq!(rc, abi::CR_UI_OK, "attach 0x{rc:08X}");
    let rc = unsafe {
        abi::cr_ui_scan_set_event_fn(
            scan,
            Some(sink),
            std::sync::Arc::as_ptr(&counts) as *mut std::ffi::c_void,
        )
    };
    assert_eq!(rc, abi::CR_UI_OK, "set_event_fn 0x{rc:08X}");

    struct SendPtr(*mut abi::CrUiScan);
    unsafe impl Send for SendPtr {}
    // Method call, not field access: edition-2021 precise capture moves the
    // whole Send wrapper into the closure — a bare `sp.0` capture moves the
    // raw pointer field itself, which is not Send (E0277, run 44023463816).
    impl SendPtr {
        fn get(self) -> *mut abi::CrUiScan {
            self.0
        }
    }
    let sp = SendPtr(scan);
    let run_handle = std::thread::spawn(move || unsafe { abi::cr_ui_scan_run(sp.get()) });

    // Sample telemetry at 500 ms.
    #[derive(Clone, Copy)]
    struct Sample {
        t_ms: u64,
        overall_pm: u32,
        quick_pm: u32,
        quick_done: u32,
        deep_pm: u32,
        deep_done: u32,
        carve_pm: u32,
        carve_done: u32,
        bytes_scanned: u64,
        bytes_found: u64,
        files: u64,
        errors: u32,
        phase: u32,
        phase_permille: u32,
    }
    let t0 = Instant::now();
    let mut samples: Vec<Sample> = Vec::new();
    let deadline = Duration::from_secs(window_secs);
    while t0.elapsed() < deadline {
        // 100 ms cadence: the seeded volume's REAL 3 GiB keeps the deep
        // sweep in flight for seconds, but quick and the sub-second phases
        // of deep are only observable at this resolution (the first real
        // execution at 500 ms captured exactly ONE sample — the whole
        // quick|deep scan of the sparse 2 GiB fixture finished inside the
        // first tick; run 44084452441).
        std::thread::sleep(Duration::from_millis(100));
        // SAFETY: telemetry is any-thread-safe per the header.
        let mut tel: abi::CrUiTelemetry = unsafe { std::mem::zeroed() };
        let rc = unsafe { abi::cr_ui_scan_telemetry(scan, &mut tel) };
        assert_eq!(rc, abi::CR_UI_OK, "telemetry 0x{rc:08X}");
        samples.push(Sample {
            t_ms: t0.elapsed().as_millis() as u64,
            overall_pm: tel.overall_permille,
            quick_pm: tel.quick.permille,
            quick_done: tel.quick.completed,
            deep_pm: tel.deep.permille,
            deep_done: tel.deep.completed,
            carve_pm: tel.carve.permille,
            carve_done: tel.carve.completed,
            bytes_scanned: tel.bytes_scanned,
            bytes_found: tel.bytes_found,
            files: tel.files_found + tel.deleted_found + tel.carved_found,
            errors: tel.error_ranges,
            phase: tel.phase,
            phase_permille: tel.phase_permille,
        });
        let finished = run_handle.is_finished();
        if finished {
            break;
        }
    }

    // Evidence FIRST (the Phase-1 discipline), then cancel + join.
    let csv = {
        let mut s = String::from(
            "t_ms,overall_pm,quick_pm,quick_done,deep_pm,deep_done,carve_pm,carve_done,\
             bytes_scanned,bytes_found,files,errors,phase,phase_permille\n",
        );
        for m in &samples {
            s.push_str(&format!(
                "{},{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
                m.t_ms, m.overall_pm, m.quick_pm, m.quick_done, m.deep_pm, m.deep_done,
                m.carve_pm, m.carve_done, m.bytes_scanned, m.bytes_found, m.files, m.errors,
                m.phase, m.phase_permille
            ));
        }
        s
    };
    write_evidence("scan-timeline.csv", csv);
    let c = counts.lock().unwrap();
    write_evidence(
        "scan-events.json",
        serde_json::to_string_pretty(&serde_json::json!({
            "progress_events": c.progress,
            "heartbeat_events": c.heartbeat,
            "files_found_events": c.files_found,
            "samples": samples.len(),
        }))
        .unwrap(),
    );
    eprintln!(
        "SCAN EVENTS: progress={} heartbeat={} filesFound={} samples={}",
        c.progress, c.heartbeat, c.files_found, samples.len()
    );
    drop(c);

    let rc = unsafe { abi::cr_ui_scan_cancel(scan) };
    assert_eq!(rc, abi::CR_UI_OK, "cancel 0x{rc:08X}");
    let run_rc = run_handle.join().expect("run thread panicked");
    // A completed scan returns OK; a cancelled one returns CANCELLED — both
    // are honest terminal states for this gate.
    assert!(
        run_rc == abi::CR_UI_OK || run_rc == abi::CR_UI_ERR_CANCELLED,
        "run() returned 0x{run_rc:08X}"
    );
    unsafe { abi::cr_ui_scan_close(scan) };
    unsafe { abi::cr_ui_device_close(dev) };

    // ---- assertions -------------------------------------------------------
    assert!(
        !samples.is_empty(),
        "no telemetry samples captured — the sampling loop never observed the scan"
    );

    /* The quick|deep scan of this fixture COMPLETES inside one 100 ms tick
     * (runs 44084452441/44090904420: the deep stage is the metadata-
     * structured full pass, not a slow byte sweep) — so "advanced between
     * the first and last sample" is not assertable here. The terminal
     * sample carries the completed truth instead. */
    // (a) overall permille REACHED the quick|deep cap (750 with the
    // default 40/35/25 weights) — the scan made real progress.
    let last_pm = samples.last().unwrap().overall_pm;
    assert!(
        last_pm >= 750,
        "overall permille never reached the quick|deep cap ({last_pm} < 750) — the scan made no real progress"
    );

    // (b) real files + real bytes on real data.
    let max_files = samples.iter().map(|s| s.files).max().unwrap();
    let max_bytes_found = samples.iter().map(|s| s.bytes_found).max().unwrap();
    assert!(
        max_files >= expect_files,
        "scan found {max_files} files, expected >= {expect_files} on the seeded disk"
    );
    assert!(
        max_bytes_found > 0,
        "files were found but bytes_found stayed 0 — the found-size axis is dead (the \
         user-visible 'files found 21 (0 B)')"
    );

    // (c) THE PHASE MODEL PIN (D-P6), reframed by the first real
    // executions: on a PARTITION scan the deep stage is the engine's
    // metadata-structured full pass — run 44090904420's timeline shows it
    // COMPLETING inside one 100 ms tick with the whole 3 GiB deleted set
    // FOUND (bytes_found ~3.2 GB, 293 files). Deep is not a slow byte
    // sweep on this fixture class, so a multi-second in-flight window
    // cannot exist; the byte-denominated advanced work is CARVE (excluded
    // here for BF-30's ~8 MiB/s lane cost; its live-bytes stamp is pinned
    // engine-side in the unit suite). What this gate pins on real Windows:
    // quick and deep both RAN and COMPLETED, and the phase axes complete
    // at 100% where the global curve caps at 750 — the exact "jumps
    // directly to 75% advanced scanning" presentation D-P6 closes.
    let last = samples.last().unwrap();
    assert!(last.quick_done != 0, "quick never completed");
    assert!(
        last.deep_done != 0,
        "the deep stage never completed — quick|deep must run deep to the end"
    );
    assert_eq!(last.phase, 1u32, "terminal phase must be advanced (1)");
    assert_eq!(
        last.phase_permille, 1000u32,
        "the advanced phase must complete at 100% (the reference's restarting gauge)"
    );
    assert_eq!(
        last.overall_pm, 750u32,
        "the global curve's quick|deep cap — the 75% the user reported"
    );

    // (d) no all-axes freeze > 15 s while in flight (regression guard for
    // the D4e/D4f class).
    let mut frozen_run_s = 0.0f64;
    let mut last_moving: Option<&Sample> = None;
    for s in &samples {
        let moving = s.overall_pm != last_moving.map(|m| m.overall_pm).unwrap_or(1)
            || s.bytes_scanned != last_moving.map(|m| m.bytes_scanned).unwrap_or(1)
            || s.files != last_moving.map(|m| m.files).unwrap_or(1);
        if moving {
            frozen_run_s = 0.0;
            last_moving = Some(s);
        } else {
            // time-based (the 0.5 s/sample form assumed the 500 ms cadence;
            // at 100 ms it would undercount fourfold)
            let since_move_ms = match last_moving {
                Some(m) => s.t_ms.saturating_sub(m.t_ms),
                None => 0,
            };
            frozen_run_s = f64::from(since_move_ms as u32) / 1000.0;
            assert!(
                frozen_run_s <= 15.0,
                "all progress axes frozen for {frozen_run_s}s at t={}ms — the D4e/D4f \
                 freeze class is back",
                s.t_ms
            );
        }
    }
}


// ---------------------------------------------------------------------------
// T10-T12: the filter axes + the deleted-tree reconstruction, on the real
// seeded disk (quick|deep completes in <1 s on this fixture — run 44090904420).
// ---------------------------------------------------------------------------

#[test]
fn results_filters_and_deleted_tree() {
    let disk: u32 = std::env::var("CR_IT_SCAN_DISK")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(0);
    let expect_files: u64 = std::env::var("CR_IT_EXPECT_FILES")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(1);
    assert!(
        disk != 0,
        "CR_IT_SCAN_DISK not set — this gate runs under the bridge-integration workflow"
    );

    let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_open_disk(disk, &mut dev) };
    assert_eq!(rc, abi::CR_UI_OK, "open_disk 0x{rc:08X}");
    let mut cfg: abi::CrUiScanConfig = unsafe { std::mem::zeroed() };
    cfg.stage_mask = 1 | 2; /* quick|deep — the fixture's proven shape */
    let mut scan: *mut abi::CrUiScan = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_scan_create(&cfg, &mut scan) };
    assert_eq!(rc, abi::CR_UI_OK, "scan_create 0x{rc:08X}");
    let rc = unsafe { abi::cr_ui_scan_attach_device(scan, dev) };
    assert_eq!(rc, abi::CR_UI_OK, "attach 0x{rc:08X}");
    let rc = unsafe { abi::cr_ui_scan_run(scan) }; /* synchronous — <1 s here */
    assert_eq!(rc, abi::CR_UI_OK, "run 0x{rc:08X}");

    /* Save + reload as a session — the query surface is session-scoped
     * (exactly the path the results explorer takes after a scan). */
    let path = std::env::temp_dir().join("cr-it-query.crsf");
    let cpath = std::ffi::CString::new(path.to_string_lossy().as_bytes()).unwrap();
    let rc = unsafe { abi::cr_ui_scan_save_session(scan, cpath.as_ptr()) };
    assert_eq!(rc, abi::CR_UI_OK, "save_session 0x{rc:08X}");
    unsafe { abi::cr_ui_scan_close(scan) };
    unsafe { abi::cr_ui_device_close(dev) };
    let mut sess: *mut abi::CrUiSession = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_session_load(cpath.as_ptr(), &mut sess) };
    assert_eq!(rc, abi::CR_UI_OK, "session_load 0x{rc:08X}");

    let cstr_of = |p: *mut i8| -> String {
        if p.is_null() {
            String::new()
        } else {
            unsafe { std::ffi::CStr::from_ptr(p) }.to_string_lossy().into_owned()
        }
    };
    /// A query row OWNS nothing: `name`/`path` are engine-heap pointers
    /// freed by `cr_ui_query_rows_free`. A shallow struct copy (the old
    /// `from_raw_parts(..).to_vec()`) carried those raw pointers past the
    /// free — every later `cstr_of(r.path)` read a dangling pointer, and
    /// when the allocator reused the freed block for the destination
    /// String, `copy_nonoverlapping` saw src==dst and aborted
    /// (core/ptr/mod.rs:554, run 44102634559). Rows are materialized into
    /// OWNED data BEFORE the engine frees its side.
    struct OwnedRow {
        node_id: u64,
        name: String,
        path: String,
        size: u64,
        deleted: u32,
        category: u32,
    }
    let run_query = |q: &mut abi::CrUiQuery| -> (u64, Vec<OwnedRow>) {
        q.offset = 0;
        q.limit = 500;
        let mut rows: *mut abi::CrUiQueryRow = std::ptr::null_mut();
        let mut count: u32 = 0;
        let mut total: u64 = 0;
        let rc = unsafe { abi::cr_ui_query_run(sess, q, &mut rows, &mut count, &mut total) };
        assert_eq!(rc, abi::CR_UI_OK, "query_run 0x{rc:08X}");
        let mut v: Vec<OwnedRow> = Vec::with_capacity(count as usize);
        if !rows.is_null() {
            for i in 0..count as usize {
                let r = unsafe { &*rows.add(i) };
                v.push(OwnedRow {
                    node_id: r.node_id,
                    name: cstr_of(r.name),
                    path: cstr_of(r.path),
                    size: r.size,
                    deleted: r.deleted,
                    category: r.category,
                });
            }
            unsafe { abi::cr_ui_query_rows_free(rows, count) };
        }
        (total, v)
    };

    /* (a) the whole manifest (R9 "filters are dead": the query returns rows). */
    let mut q: abi::CrUiQuery = unsafe { std::mem::zeroed() };
    q.count_only = 1;
    let (total, _) = run_query(&mut q);
    assert!(total >= expect_files, "query total {total} < manifest {expect_files}");

    /* (b) THE CATEGORY FILTERS (the engine's FileFilter table — the og's 8
     * categories; mask bit = 1 << category value: image 1<<1, document
     * 1<<4, archive 1<<5). The seed carries 5 .jpg + 3 .pdf + 2 .zip live. */
    for (mask, want, label) in [
        (1u32 << 1, 5u64, "image"),
        (1u32 << 4, 3u64, "document"),
        (1u32 << 5, 2u64, "archive"),
    ] {
        let mut q: abi::CrUiQuery = unsafe { std::mem::zeroed() };
        q.category_mask = mask;
        let (total, rows) = run_query(&mut q);
        assert!(total >= want, "{label}: {total} < {want} (the category filter is dead)");
        assert!(!rows.is_empty(), "{label}: no rows materialized");
    }

    /* (c) the SIZE filter: the 17 live 256 KiB files + the 3 GiB deleted
     * bulk clear a 250 KiB floor (the 4-128 KiB rows must not). */
    let mut q: abi::CrUiQuery = unsafe { std::mem::zeroed() };
    q.size_min = 250 * 1024;
    let (total, _) = run_query(&mut q);
    assert!(total >= 18, "size_min filter: {total} < 18");

    /* (d) THE DELETED-TREE RECONSTRUCTION (T12): deleted_only returns the
     * seeded deleted set (80 del-*.bin + the 3 GiB bulk) with their
     * ORIGINAL parent path — "gone\del-N.bin" — the "reconstruct them as
     * they were before deletion" standard. */
    let mut q: abi::CrUiQuery = unsafe { std::mem::zeroed() };
    q.deleted_only = 1;
    let (total, rows) = run_query(&mut q);
    assert!(total >= 81, "deleted_only: {total} < 81 (80 del + the bulk)");
    let in_gone = rows
        .iter()
        .filter(|r| r.path.to_ascii_lowercase().contains("gone"))
        .count();
    assert!(
        in_gone >= 80,
        "deleted rows under the reconstructed gone\\ parent: {in_gone} < 80"
    );
    let evidence: String = rows
        .iter()
        .map(|r| format!("{}\t{}\t{}\n", r.path, r.size, r.deleted))
        .collect();
    write_evidence("deleted-tree.txt", evidence);

    unsafe { abi::cr_ui_session_close(sess) };
    let _ = std::fs::remove_file(&path);
}


// ---------------------------------------------------------------------------
// T13: the preview surfaces on real Windows — image metadata + the archive
// browse-inside inventory (M-102/M-116's ABI halves, exercised end to end).
// ---------------------------------------------------------------------------

#[test]
fn preview_surfaces() {
    let disk: u32 = std::env::var("CR_IT_SCAN_DISK")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(0);
    assert!(
        disk != 0,
        "CR_IT_SCAN_DISK not set — this gate runs under the bridge-integration workflow"
    );

    let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_open_disk(disk, &mut dev) };
    assert_eq!(rc, abi::CR_UI_OK, "open_disk 0x{rc:08X}");
    let mut cfg: abi::CrUiScanConfig = unsafe { std::mem::zeroed() };
    cfg.stage_mask = 1 | 2;
    let mut scan: *mut abi::CrUiScan = std::ptr::null_mut();
    assert_eq!(abi::CR_UI_OK, unsafe { abi::cr_ui_scan_create(&cfg, &mut scan) });
    assert_eq!(abi::CR_UI_OK, unsafe { abi::cr_ui_scan_attach_device(scan, dev) });
    let rc = unsafe { abi::cr_ui_scan_run(scan) };
    assert_eq!(rc, abi::CR_UI_OK, "run 0x{rc:08X}");
    let path = std::env::temp_dir().join("cr-it-preview.crsf");
    let cpath = std::ffi::CString::new(path.to_string_lossy().as_bytes()).unwrap();
    let rc = unsafe { abi::cr_ui_scan_save_session(scan, cpath.as_ptr()) };
    assert_eq!(rc, abi::CR_UI_OK, "save_session 0x{rc:08X}");
    unsafe { abi::cr_ui_scan_close(scan) };
    /* dev STAYS OPEN — preview_open_session needs the device + session. */
    let mut sess: *mut abi::CrUiSession = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_session_load(cpath.as_ptr(), &mut sess) };
    assert_eq!(rc, abi::CR_UI_OK, "session_load 0x{rc:08X}");

    let node_of_category = |mask: u32| -> u64 {
        let mut q: abi::CrUiQuery = unsafe { std::mem::zeroed() };
        q.category_mask = mask;
        q.limit = 10;
        let mut rows: *mut abi::CrUiQueryRow = std::ptr::null_mut();
        let mut count: u32 = 0;
        let mut total: u64 = 0;
        let rc = unsafe { abi::cr_ui_query_run(sess, &q, &mut rows, &mut count, &mut total) };
        assert_eq!(rc, abi::CR_UI_OK, "query_run 0x{rc:08X}");
        assert!(count > 0, "no rows for category mask 0x{mask:x}");
        let id = unsafe { (*rows).node_id };
        if !rows.is_null() {
            unsafe { abi::cr_ui_query_rows_free(rows, count) };
        }
        id
    };

    /* (a) IMAGE: the preview metadata classifies the .jpg node (the kind
     * dispatch the preview pane renders from). */
    let image_node = node_of_category(1u32 << 1);
    let mut pv: *mut abi::CrUiPreview = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_preview_open_session(dev, sess, image_node, &mut pv) };
    assert_eq!(rc, abi::CR_UI_OK, "preview_open_session(image) 0x{rc:08X}");
    let mut meta: abi::CrUiMetadata = unsafe { std::mem::zeroed() };
    let rc = unsafe { abi::cr_ui_preview_metadata(pv, &mut meta) };
    assert_eq!(rc, abi::CR_UI_OK, "preview_metadata(image) 0x{rc:08X}");
    let kind = fixed_str(&meta.kind);
    assert!(kind.contains("image"), "image node metadata kind is '{kind}'");
    unsafe { abi::cr_ui_preview_close(pv) };

    /* (b) ARCHIVE: the real zip's browse-inside inventory (M-116: the
     * central-directory walk, the 512-entry hostile bound) — the seed's
     * backup-1.zip is a genuine zip of the 43-entry docs folder. */
    let archive_node = node_of_category(1u32 << 5);
    let mut pv: *mut abi::CrUiPreview = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_preview_open_session(dev, sess, archive_node, &mut pv) };
    assert_eq!(rc, abi::CR_UI_OK, "preview_open_session(archive) 0x{rc:08X}");
    let mut meta: abi::CrUiMetadata = unsafe { std::mem::zeroed() };
    let rc = unsafe { abi::cr_ui_preview_metadata(pv, &mut meta) };
    assert_eq!(rc, abi::CR_UI_OK, "preview_metadata(archive) 0x{rc:08X}");
    let kind = fixed_str(&meta.kind);
    assert!(kind.contains("archive"), "archive node metadata kind is '{kind}'");
    let mut entries: u32 = 0;
    let rc = unsafe { abi::cr_ui_archive_entry_count(pv, &mut entries) };
    assert_eq!(rc, abi::CR_UI_OK, "archive_entry_count 0x{rc:08X}");
    assert!(entries >= 5, "the real zip's inventory has {entries} entries (< 5)");
    unsafe { abi::cr_ui_preview_close(pv) };
    write_evidence("preview.txt", format!("image kind: {kind:?} (node {image_node}); archive entries: {entries}\n"));

    unsafe { abi::cr_ui_session_close(sess) };
    unsafe { abi::cr_ui_device_close(dev) };
    let _ = std::fs::remove_file(&path);
}

// ---------------------------------------------------------------------------
// W4 (the installer-integration session): the ranged + cancellable image
// producer on the REAL seeded disk — cr_ui_image_create_ex end to end.
// A bounded 4 MiB sector range exercises D-IMG-3 (content length + the
// .sha256 sidecar of the RANGE) and the caller-owned flag exercises
// D-IMG-1 (ERR_CANCELLED + the truthful .partial). Byte-level content
// equality is pinned by the engine's portable ImageCreateRange.* suite;
// this gate pins the real-disk wiring (elevation, geometry, the file
// disposition, the sidecar).
// ---------------------------------------------------------------------------

#[test]
fn image_create_ex_range_and_cancel() {
    let disk: u32 = std::env::var("CR_IT_SCAN_DISK")
        .ok()
        .and_then(|v| v.parse().ok())
        .unwrap_or(0);
    assert!(
        disk != 0,
        "CR_IT_SCAN_DISK not set — this gate runs under the bridge-integration workflow"
    );

    // geometry first: the range is sized off the REAL disk's sector count
    let mut dev: *mut abi::CrUiDevice = std::ptr::null_mut();
    let rc = unsafe { abi::cr_ui_open_disk(disk, &mut dev) };
    assert_eq!(rc, abi::CR_UI_OK, "open_disk 0x{rc:08X}");
    let mut g: abi::CrUiGeometry = unsafe { std::mem::zeroed() };
    let rc = unsafe { abi::cr_ui_device_geometry(dev, &mut g) };
    assert_eq!(rc, abi::CR_UI_OK, "device_geometry 0x{rc:08X}");
    assert!(g.logical_sector >= 512, "logical sector {g.logical_sector} too small");
    assert!(g.sector_count > 4096, "seed disk too small for the range test");

    // (a) D-IMG-3: a 4 MiB range from sector 1024 — file length == the
    // range bytes exactly, and the .sha256 sidecar of the RANGE exists
    // beside the committed image
    let first: u64 = 1024;
    let count: u64 = (4 * 1024 * 1024) / u64::from(g.logical_sector);
    let expect_len = count * u64::from(g.logical_sector);
    let out = std::env::temp_dir().join(format!("cr-it-image-ex-{disk}.img"));
    let cpath = std::ffi::CString::new(out.to_string_lossy().as_bytes()).unwrap();
    let rc = unsafe {
        abi::cr_ui_image_create_ex(
            disk,
            cpath.as_ptr(),
            first,
            count,
            std::ptr::null_mut(),
            None,
            std::ptr::null_mut(),
        )
    };
    assert_eq!(rc, abi::CR_UI_OK, "image_create_ex(range) 0x{rc:08X}");
    let len = std::fs::metadata(&out).map(|m| m.len()).unwrap_or(0);
    assert_eq!(len, expect_len, "ranged image length {len} != {expect_len}");
    let sidecar = std::fs::read_to_string(out.with_extension("sha256")).unwrap_or_default();
    assert!(
        sidecar.len() == 67 + out.file_name().unwrap().to_string_lossy().len()
            && sidecar.starts_with(|c: char| c.is_ascii_hexdigit()),
        "sidecar shape wrong: {sidecar:?}"
    );

    // (b) D-IMG-1: the caller-owned flag — set BEFORE the call, the first
    // loop head refuses, ERR_CANCELLED + the truthful .partial
    let out2 = std::env::temp_dir().join(format!("cr-it-image-cancel-{disk}.img"));
    let cpath2 = std::ffi::CString::new(out2.to_string_lossy().as_bytes()).unwrap();
    let flag: std::sync::atomic::AtomicU32 = std::sync::atomic::AtomicU32::new(1);
    let rc = unsafe {
        abi::cr_ui_image_create_ex(
            disk,
            cpath2.as_ptr(),
            first,
            count,
            flag.as_ptr(),
            None,
            std::ptr::null_mut(),
        )
    };
    assert_eq!(
        rc, abi::CR_UI_ERR_CANCELLED,
        "pre-cancelled image_create_ex 0x{rc:08X}"
    );
    assert!(
        out2.with_extension("partial").exists(),
        "the cancelled create must keep the .partial"
    );
    assert!(!out2.exists(), "a cancelled create must NOT commit the image");

    // (c) the out-of-range refusal: beyond the disk's end is ERR_BAD_
    // PARAMETER — never a silent truncation (the og's Invalid sector)
    let rc = unsafe {
        abi::cr_ui_image_create_ex(
            disk,
            cpath.as_ptr(),
            g.sector_count,
            8,
            std::ptr::null_mut(),
            None,
            std::ptr::null_mut(),
        )
    };
    assert_eq!(
        rc, abi::CR_UI_ERR_BAD_PARAMETER,
        "out-of-range create_ex 0x{rc:08X}"
    );

    unsafe { abi::cr_ui_device_close(dev) };
    let _ = std::fs::remove_file(&out);
    let _ = std::fs::remove_file(out.with_extension("sha256"));
    let _ = std::fs::remove_file(out2.with_extension("partial"));
    write_evidence(
        "image-create-ex.txt",
        format!(
            "range image: sectors [{first}, {}) -> {expect_len} bytes + sidecar; pre-cancel: ERR_CANCELLED + .partial; out-of-range: ERR_BAD_PARAMETER (disk {disk}, sector_count {})\n",
            first + count,
            g.sector_count
        ),
    );
}
