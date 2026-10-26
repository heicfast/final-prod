//! bridge/consent.rs — the pre-scan repair consent flow (S-008, plan §4.5).
//!
//! Engine contract (engine/include/cr/cr_ui.h, pre-scan repair section):
//! when `cr_ui_scan_config.pre_scan_repair` is 1, the shim runs the
//! read-only repair analysis before the stages and — when a repairable
//! plan exists — calls `repair_consent` ONCE, ON THE RUN THREAD, before
//! any engine read. Nonzero return = execute the transactional repair
//! through the shim-opened write view; zero = the plan is surfaced via
//! CR_EVT_REPAIR events and stats and NEVER executed (default-deny).
//!
//! Bridge wiring (the G-1 follow-up design, INTEGRATION-GAPS.md G-1 (a)):
//! `scan_start` arms the callback with a `ConsentPipe` (scan id + app
//! handle + state Arc) as the user pointer. The sink:
//! 1. mints a `requestId`, registers a pending `ConsentGate` in the state
//!    registry (the registry lock is taken and RELEASED before blocking —
//!    the run thread never sleeps holding a lock, plan §4.5 deadlock rule),
//! 2. emits `consent-request:<scanId>` with the plan POD translated to
//!    JSON (the event is scan-scoped so the page can subscribe in advance;
//!    the minted requestId rides the payload and is the ONLY key
//!    `consent_respond` accepts — an echo token, never a guessable
//!    authority),
//! 3. blocks the run thread on the gate (NO user-facing timeout: a
//!    non-answer is a user decision, plan §4.5 — the prompt stays open
//!    until answered, the scan is cancelled, or the process exits).
//!
//! Default-deny holds on every teardown path: `scan_cancel` force-resolves
//! the scan's pending gates (a blocked run thread must unblock before the
//! engine can observe the cancel token), and the run tail closes any
//! survivor after `cr_ui_scan_run` returns (hygiene; the callback always
//! returns before run does — it blocks run).
//!
//! Pipe lifetime: the consent callback can only fire INSIDE `run()`, so
//! the pipe is the run thread's exclusive property — `Box::into_raw` at
//! scan_start, `Box::from_raw` in the run tail after run() returns. The
//! callback holds `&ConsentPipe` only across a single invocation; the
//! gate it blocks on is an `Arc` clone, so a concurrent close
//! (cancel) can never dangle.

use std::sync::{Arc, Condvar, Mutex};

use serde_json::json;
use tauri::{AppHandle, Emitter};

use super::abi;
use super::error::{code, BridgeError};
use super::state::{BridgeState, PendingConsent};
use super::util::fixed_str;

/// One pending consent decision: `None` until answered/force-denied.
/// `Condvar` wakeups: respond(), close().
#[derive(Default)]
pub struct ConsentGate {
    resolved: Mutex<Option<bool>>,
    cv: Condvar,
}

impl ConsentGate {
    /// Set the decision. Returns false when already resolved (a double
    /// respond, or a cancel that force-denied first) — the caller turns
    /// that into the typed consent-not-found refusal.
    pub fn respond(&self, allow: bool) -> bool {
        let mut g = lock(&self.resolved);
        if g.is_some() {
            return false;
        }
        *g = Some(allow);
        drop(g);
        self.cv.notify_all();
        true
    }

    /// Force-deny (cancel / teardown). Idempotent: a gate that was already
    /// answered keeps its answer; only a still-pending gate is denied.
    pub fn close(&self) {
        let mut g = lock(&self.resolved);
        if g.is_none() {
            *g = Some(false);
        }
        drop(g);
        self.cv.notify_all();
    }

    /// Block the run thread until a decision exists (no timeout — plan
    /// §4.5: a non-answer is a user decision; cancel/close are the
    /// deny paths, and they call close() which wakes this wait).
    pub fn wait_for_decision(&self) -> bool {
        let mut g = lock(&self.resolved);
        while g.is_none() {
            g = lock_wait(&self.cv, g);
        }
        (*g).unwrap_or(false)
    }
}

fn lock(m: &Mutex<Option<bool>>) -> std::sync::MutexGuard<'_, Option<bool>> {
    m.lock().unwrap_or_else(std::sync::PoisonError::into_inner)
}

fn lock_wait<'a>(
    cv: &'a Condvar,
    g: std::sync::MutexGuard<'a, Option<bool>>,
) -> std::sync::MutexGuard<'a, Option<bool>> {
    cv.wait(g).unwrap_or_else(std::sync::PoisonError::into_inner)
}

/// The `repair_consent_user` payload. Boxed + leaked at scan_start; freed
/// by the run tail after `cr_ui_scan_run` returns (consent fires only
/// inside run — cr_ui.h), so the single reader is gone before the free.
pub struct ConsentPipe {
    pub scan_id: String,
    pub app: AppHandle,
    pub state: Arc<BridgeState>,
}

/// The consent callback — engine run thread, at most once per scan.
///
/// Default-deny on every failure shape: null user/plan, a poisoned
/// registry, or an emit failure all still register + block + return the
/// gate's answer; when registration itself fails the only safe answer is
/// deny (the request would be unanswerable).
pub unsafe extern "C" fn consent_sink(
    user: *mut std::ffi::c_void,
    plan: *const abi::CrUiRepairPlan,
) -> u32 {
    if user.is_null() || plan.is_null() {
        return 0; // default-deny (cr_ui.h S-008)
    }
    let pipe = unsafe { &*(user as *const ConsentPipe) };
    let plan = unsafe { &*plan };
    let request_id = pipe.state.mint_consent_id();
    let payload = json!({
        "scanId": pipe.scan_id,
        "requestId": request_id,
        "plan": {
            "detected": plan.detected,
            "fsFamily": fixed_str(&plan.fs_family),
            "actionCount": plan.action_count,
            "refused": plan.refused,
            "reason": fixed_str(&plan.reason),
            "advisoryCount": plan.advisory_count,
        },
    });
    let gate = Arc::new(ConsentGate::default());
    pipe.state.register_consent(
        &request_id,
        PendingConsent {
            scan_id: pipe.scan_id.clone(),
            payload: payload.clone(),
            gate: gate.clone(),
        },
    );
    let _ = pipe
        .app
        .emit(&format!("consent-request:{}", pipe.scan_id), payload);
    // Block the RUN thread (no registry lock held — plan §4.5).
    let approved = gate.wait_for_decision();
    pipe.state.unregister_consent(&request_id);
    u32::from(approved)
}

/// The re-added `consent_respond` decision path (command-surface.json; the
/// manifest's removed[] entry retired when this landed — G-1 (a)).
/// Errors are the typed consent-not-found: unknown requestId, a
/// double-respond, or a cancel that force-denied first. Registry-only —
/// no engine call, so no spawn_blocking (the lock is held for one map
/// lookup + a condvar notify).
pub fn consent_respond_impl(
    state: &Arc<BridgeState>,
    request_id: &str,
    allow: bool,
) -> Result<(), BridgeError> {
    let gate = state
        .consents
        .lock()
        .map_err(|_| BridgeError::bridge(code::UNKNOWN, "consent registry lock"))?
        .get(request_id)
        .map(|p| p.gate.clone());
    match gate {
        Some(g) if g.respond(allow) => Ok(()),
        _ => Err(BridgeError::bridge(
            code::CONSENT_NOT_FOUND,
            format!(
                "consent request '{request_id}' is unknown or already resolved \
                 (responded twice, cancelled, or the scan ended)"
            ),
        )),
    }
}
