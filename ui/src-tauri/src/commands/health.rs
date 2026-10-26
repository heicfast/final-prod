//! commands/health.rs — health_read (type-maps §7). One engine call fills
//! the report + attributes (two-call on attrs). available == 0 → the typed
//! health-unavailable refusal. The engine carries NO SMART threshold field —
//! threshold stays null (truth, never a fabricated vendor threshold).

use serde_json::{json, Value};

use crate::bridge::abi;
use crate::bridge::error::{code, check, BridgeError};
use crate::bridge::util::fixed_str;

#[tauri::command]
pub async fn health_read(device_id: String) -> Result<Value, BridgeError> {
    tauri::async_runtime::spawn_blocking(move || {
        let n: u32 = device_id
            .strip_prefix("disk-")
            .and_then(|r| r.parse().ok())
            .ok_or_else(|| {
                BridgeError::bridge(
                    code::DEVICE_GONE,
                    format!("'{device_id}' is not a device id (expected disk-N)"),
                )
            })?;

        let mut report: abi::CrUiSmartReport = unsafe { std::mem::zeroed() };
        let mut needed: u32 = 0;
        let rc = unsafe {
            abi::cr_ui_health_read(n, &mut report, std::ptr::null_mut(), 0, &mut needed)
        };
        if rc == abi::CR_UI_ERR_UNSUPPORTED {
            return Err(BridgeError::bridge(
                code::HEALTH_UNAVAILABLE,
                "SMART is unavailable on this system/device (engine platform layer refused)",
            ));
        }
        if rc == 0x5007_0001 {
            return Err(BridgeError::bridge(
                code::HEALTH_UNAVAILABLE,
                "the engine reported no SMART data for this device",
            ));
        }
        check(rc, "cr_ui_health_read(size)")?;
        if report.available == 0 {
            return Err(BridgeError::bridge(
                code::HEALTH_UNAVAILABLE,
                "the SMART table could not be decoded for this device",
            ));
        }

        let mut attrs = vec![unsafe { std::mem::zeroed::<abi::CrUiSmartAttr>() }; needed as usize];
        let mut cap = needed;
        let rc = unsafe { abi::cr_ui_health_read(n, &mut report, attrs.as_mut_ptr(), cap, &mut cap) };
        check(rc, "cr_ui_health_read")?;
        attrs.truncate(cap as usize);

        // Status grading (type-maps §7): failing-now/critical → bad; failing
        // attributes, high wear, or low spare → caution; else good.
        let wear = if report.nvme_available != 0 {
            Some(report.percentage_used)
        } else if report.ssd_wear_level > 0 {
            Some(report.ssd_wear_level)
        } else {
            None
        };
        let any_attr_failing = attrs.iter().any(|a| a.failing != 0);
        let low_spare = report.nvme_available != 0
            && report.available_spare_threshold != 0
            && report.available_spare <= report.available_spare_threshold;
        let status = if report.failing_now != 0 || report.critical_warning != 0 {
            "bad"
        } else if any_attr_failing
            || wear.map(|w| w >= 80).unwrap_or(false)
            || low_spare
        {
            "caution"
        } else {
            "good"
        };

        let temperature = if report.nvme_available != 0 {
            Some(report.temperature_c_x10 as f64 / 10.0)
        } else if report.temperature_c > 0 {
            Some(report.temperature_c as f64)
        } else {
            None
        };
        let power_on_hours = if report.nvme_available != 0 {
            Some(report.power_on_hours_nvme)
        } else if report.power_on_hours > 0 {
            Some(report.power_on_hours)
        } else {
            None
        };

        let rows: Vec<Value> = attrs
            .iter()
            .map(|a| {
                json!({
                    "id": a.id,
                    "name": fixed_str(&a.name),
                    "current": (if a.value != 0 { json!(a.value) } else { Value::Null }),
                    "worst": (if a.worst != 0 { json!(a.worst) } else { Value::Null }),
                    // The engine carries no threshold field: null is truth.
                    "threshold": Value::Null,
                    "raw": a.raw,
                    "status": (if a.failing != 0 { "failing" } else { "ok" }),
                })
            })
            .collect();

        Ok(json!({
            "deviceId": device_id,
            "status": status,
            "temperatureC": temperature,
            "powerOnHours": power_on_hours,
            "diskClass": (if report.ssd_like == 1 { "ssd" } else if report.ssd_like == 0 { "hdd" } else { "unknown" }),
            "wearPercent": wear,
            "trimEnabled": (if report.trim_known != 0 { json!(report.trim_enabled != 0) } else { Value::Null }),
            "reallocatedSectors": (if report.reallocated_sectors > 0 { json!(report.reallocated_sectors) } else { json!(0) }),
            "pendingSectors": (if report.pending_sectors > 0 { json!(report.pending_sectors) } else { json!(0) }),
            "attributes": rows,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
