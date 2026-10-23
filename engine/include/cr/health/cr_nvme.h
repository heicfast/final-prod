/*
 * cr_nvme.h - NVMe health log page (page 2) pure parser.
 *
 * Layout per NVMe base specification (1.x), Health Information log page —
 * offsets in bytes within the 512-byte page:
 *   +0        critical_warning
 *   +1..2     temperature (Kelvin, LE)
 *   +3        available_spare (%)
 *   +4        available_spare_threshold (%)
 *   +5..6     percentage_used
 *   +7..31    reserved
 *   +32..47   data_units_read      (128-bit LE)
 *   +48..63   data_units_written   (128-bit LE)
 *   +64..79   host_reads           (128-bit)
 *   +80..95   host_writes          (128-bit)
 *   +96..111  controller_busy_time (128-bit)
 *   +112..127 power_cycles         (128-bit)
 *   +128..143 power_on_hours       (128-bit)
 *   +144..159 unsafe_shutdowns     (128-bit)
 *   +160..175 media_errors         (128-bit)
 *   +176..191 num_error_log_entries(128-bit)
 *
 * PARITY-AUDIT F-A4-17: the previous inline parse in src/win/cr_health.cpp
 * read power_cycles/hours/unsafe/media at 64/80/96/112 — the Host Reads /
 * Host Writes / Controller Busy Time / Power Cycles positions. The parser
 * lives in pure core so the mapping is unit-testable on Linux CI (the win
 * layer is compile-verified only, G-17).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_NVME_HEALTH_H_
#define CR_NVME_HEALTH_H_

#include "../cr_types.h"

namespace cr {
namespace health {

struct NvmeHealthInfo {
    bool available = false;
    u8 critical_warning = 0;
    u16 temperature_k = 0;
    u8 available_spare = 0;
    u8 available_spare_threshold = 0;
    u64 percentage_used = 0;
    u64 data_units_read = 0;
    u64 data_units_written = 0;
    u64 power_cycles = 0;
    u64 power_on_hours = 0;
    u64 unsafe_shutdowns = 0;
    u64 media_errors = 0;
};

/* Parse a health log page buffer (bytes >= 176; the 512-byte page always
 * satisfies this). Returns ERR_TRUNCATED on short buffers, OK otherwise.
 * Sets out.available = true on success. */
u32 parse_nvme_health_page(const u8* page, u32 bytes, NvmeHealthInfo& out);

} // namespace health
} // namespace cr

#endif // CR_NVME_HEALTH_H_
