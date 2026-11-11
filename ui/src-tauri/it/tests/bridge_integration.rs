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

    // SAFETY: zeroed config = the documented safe default (quick|deep|carve).
    let cfg: abi::CrUiScanConfig = unsafe { std::mem::zeroed() };
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
    let sp = SendPtr(scan);
    let run_handle = std::thread::spawn(move || unsafe { abi::cr_ui_scan_run(sp.0) });

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
    }
    let t0 = Instant::now();
    let mut samples: Vec<Sample> = Vec::new();
    let deadline = Duration::from_secs(window_secs);
    while t0.elapsed() < deadline {
        std::thread::sleep(Duration::from_millis(500));
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
             bytes_scanned,bytes_found,files,errors\n",
        );
        for m in &samples {
            s.push_str(&format!(
                "{},{},{},{},{},{},{},{},{},{},{},{}\n",
                m.t_ms, m.overall_pm, m.quick_pm, m.quick_done, m.deep_pm, m.deep_done,
                m.carve_pm, m.carve_done, m.bytes_scanned, m.bytes_found, m.files, m.errors
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
        samples.len() >= 4,
        "too few telemetry samples captured ({})",
        samples.len()
    );

    // (a) overall permille advances.
    let first_pm = samples.first().unwrap().overall_pm;
    let max_pm = samples.iter().map(|s| s.overall_pm).max().unwrap();
    assert!(
        max_pm > first_pm,
        "overall permille never advanced ({first_pm} -> {max_pm}) — the scan made no real progress"
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

    // (c) THE DEEP-BYTES PIN (P-DEEP-01): during the deep stage the
    // bytes_scanned axis must advance. Identify the deep window as
    // samples where deep is in flight (permille in (0,1000), not yet
    // completed) while quick AND carve are both completed (the standard
    // quick→carve→deep order).
    let deep_window: Vec<&Sample> = samples
        .iter()
        .filter(|s| {
            s.quick_done != 0
                && s.carve_done != 0
                && s.deep_done == 0
                && s.deep_pm > 0
                && s.deep_pm < 1000
        })
        .collect();
    eprintln!("deep-window samples: {}", deep_window.len());
    if deep_window.len() >= 4 {
        let b_first = deep_window.first().unwrap().bytes_scanned;
        let b_last = deep_window.last().unwrap().bytes_scanned;
        assert!(
            b_last > b_first,
            "bytes_scanned FROZEN through the deep stage ({b_first} -> {b_last} over {} \
             samples) — 'Reading sector: X/Y' is dead through the whole deep phase \
             (P-DEEP-01, the stuck-in-sector-scanning presentation)",
            deep_window.len()
        );
    } else {
        // The window didn't capture deep in flight (scan finished early or
        // the seed is too small). The gate needs a deep window to mean
        // anything — fail loudly rather than pass vacuously.
        panic!(
            "the sampling window never observed the deep stage in flight ({} qualifying \
             samples) — enlarge the seed or CR_IT_SCAN_SECS",
            deep_window.len()
        );
    }

    // (d) no all-axes freeze > 15 s while in flight (regression guard for
    // the D4e/D4f class).
    let mut frozen_run_s = 0.0f64;
    let mut last_moving: Option<&Sample> = None;
    for s in &samples {
        let moving = s.overall_pm != last_moving.map(|m| m.overall_pm).unwrap_or(1)
            || s.bytes_scanned != last_moving.map(|m| m.bytes_scanned).unwrap_or(1)
            || s.files != last_moving.map(|m| m.files).unwrap_or(1);
        if moving {
            frozen_run_s = 0;
            last_moving = Some(s);
        } else {
            frozen_run_s += 0.5;
            assert!(
                frozen_run_s <= 15.0,
                "all progress axes frozen for {frozen_run_s}s at t={}ms — the D4e/D4f \
                 freeze class is back",
                s.t_ms
            );
        }
    }
}
