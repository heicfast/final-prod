//! commands/health.rs — health_read (type-maps §7). The engine call fills
//! the report + attributes (two-call on attrs) through the threshold-
//! carrying ex2 ABI (NG-9). Availability truth (NG-6/NG-9, breaking-change
//! fix): `available` is the ATA-SMART-table flag ONLY — a report with
//! `nvme_available == 1` is VALID health data (the engine returns OK with
//! no ATA table on NVMe drives); a readable-but-EMPTY ATA table (zero
//! attribute rows) grades "unknown" with a truthful empty table, never a
//! fabricated "good". No percentage is invented anywhere in this file.

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

        // Size probe: attrs == null returns the needed row count.
        let mut report: abi::CrUiSmartReport = unsafe { std::mem::zeroed() };
        let mut needed: u32 = 0;
        let rc = unsafe {
            abi::cr_ui_health_read_ex2(n, &mut report, std::ptr::null_mut(), 0, &mut needed)
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
                "the engine reported no health source for this device (no SMART table, no NVMe health log)",
            ));
        }
        check(rc, "cr_ui_health_read_ex2(size)")?;

        // NG-6: `available` = ATA table decoded; NVMe-only drives carry
        // nvme_available. Refuse ONLY when both sources are absent (the
        // engine already refuses that case; this is the defensive mirror).
        if report.available == 0 && report.nvme_available == 0 {
            return Err(BridgeError::bridge(
                code::HEALTH_UNAVAILABLE,
                "no health data for this device (no ATA SMART table and no NVMe health log)",
            ));
        }

        let mut attrs = vec![unsafe { std::mem::zeroed::<abi::CrUiSmartAttrEx>() }; needed as usize];
        let mut cap = needed;
        let rc = unsafe {
            abi::cr_ui_health_read_ex2(n, &mut report, attrs.as_mut_ptr(), cap, &mut cap)
        };
        check(rc, "cr_ui_health_read_ex2")?;
        attrs.truncate(cap as usize);

        let has_ata_table = report.available != 0 && !attrs.is_empty();
        let has_nvme = report.nvme_available != 0;
        // NG-9: a readable but EMPTY ATA table is "no data" — grade unknown,
        // never "good". Counters derived from attribute 5/0xC5/0xC6 rows are
        // null when the table is empty (0 would be a fabrication).
        let table_empty = report.available != 0 && attrs.is_empty();

        // Status grading (type-maps §7, reference grading list
        // Unknown/Good/Caution/Bad): failing-now/critical → bad; failing
        // attributes, high wear, or low spare → caution; data present and
        // clean → good; NO data basis → unknown.
        let wear = if has_nvme {
            Some(report.percentage_used)
        } else if report.ssd_wear_level > 0 {
            Some(report.ssd_wear_level)
        } else {
            None
        };
        let any_attr_failing = attrs.iter().any(|a| a.base.failing != 0);
        let low_spare = has_nvme
            && report.available_spare_threshold != 0
            && report.available_spare <= report.available_spare_threshold;
        let status = if report.failing_now != 0 || report.critical_warning != 0 {
            "bad"
        } else if any_attr_failing
            || wear.map(|w| w >= 80).unwrap_or(false)
            || low_spare
        {
            "caution"
        } else if has_ata_table || has_nvme {
            "good"
        } else {
            "unknown" // empty table: honest absence of data
        };

        let temperature = if has_nvme {
            Some(report.temperature_c_x10 as f64 / 10.0)
        } else if report.temperature_c > 0 {
            Some(report.temperature_c as f64)
        } else {
            None
        };
        let power_on_hours = if has_nvme {
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
                    "id": a.base.id,
                    "name": fixed_str(&a.base.name),
                    "current": (if a.base.value != 0 { json!(a.base.value) } else { Value::Null }),
                    "worst": (if a.base.worst != 0 { json!(a.base.worst) } else { Value::Null }),
                    // NG-9: the real vendor threshold (0 = the drive does not
                    // report one) — null only when genuinely unreported.
                    "threshold": (if a.threshold != 0 { json!(a.threshold) } else { Value::Null }),
                    "raw": a.base.raw,
                    "status": (if a.base.failing != 0 { "failing" } else { "ok" }),
                })
            })
            .collect();

        let transport = fixed_str(&report.transport);
        Ok(json!({
            "deviceId": device_id,
            "status": status,
            // Real wear-derived life remains available to the UI as
            // wearPercent; NO aggregate percentage is synthesized here.
            "temperatureC": temperature,
            "powerOnHours": power_on_hours,
            "diskClass": (if report.ssd_like == 1 { "ssd" } else if report.ssd_like == 0 { "hdd" } else { "unknown" }),
            "wearPercent": wear,
            "trimEnabled": (if report.trim_known != 0 { json!(report.trim_enabled != 0) } else { Value::Null }),
            "reallocatedSectors": (if table_empty || (has_nvme && !has_ata_table && report.reallocated_sectors == 0) { Value::Null } else { json!(report.reallocated_sectors) }),
            "pendingSectors": (if table_empty || (has_nvme && !has_ata_table && report.pending_sectors == 0) { Value::Null } else { json!(report.pending_sectors) }),
            "attributes": rows,
            "transport": (if transport.is_empty() { Value::Null } else { json!(transport) }),
            "tableEmpty": table_empty,
        }))
    })
    .await
    .map_err(|e| BridgeError::bridge(code::UNKNOWN, format!("blocking join: {e}")))?
}
