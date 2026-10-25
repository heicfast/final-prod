/* cr_nvme.cpp - NVMe health log page parser (pure core).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/health/cr_nvme.h"
#include "cr/cr_error.h"

namespace cr {
namespace health {

namespace {
u64 le128(const u8* p) {
    /* 128-bit counter truncated to u64 (low 8 bytes, LE) — values above
     * 2^64 units are not meaningful for health reporting */
    u64 v = 0;
    for (int i = 7; i >= 0; --i)
        v = (v << 8) | u64(p[i]);
    return v;
}
} // namespace

u32 parse_nvme_health_page(const u8* page, u32 bytes, NvmeHealthInfo& out) {
    if (page == nullptr || bytes < 192)
        return ERR_TRUNCATED;
    out.available = true;
    out.critical_warning = page[0];
    out.temperature_k = u16(page[1]) | (u16(page[2]) << 8);
    out.available_spare = page[3];
    out.available_spare_threshold = page[4];
    /* N-009a (diff-to-fix): NVMe Identify Controller/Health log defines
     * percentage_used as a single u8 (values > 100 are impossible per
     * spec; the old u16 read consumed the reserved byte at +6). */
    out.percentage_used = page[5];
    out.data_units_read = le128(page + 32);
    out.data_units_written = le128(page + 48);
    /* M-036: host r/w, controller-busy and the error-log count were
     * dropped by the original parser; data units are converted to bytes
     * exactly as the og DiskHealth does (x 512,000, :5589-5614). */
    out.host_reads = le128(page + 64);
    out.host_writes = le128(page + 80);
    out.controller_busy_time = le128(page + 96);
    out.power_cycles = le128(page + 112);
    out.power_on_hours = le128(page + 128);
    out.unsafe_shutdowns = le128(page + 144);
    out.media_errors = le128(page + 160);
    out.num_error_log_entries = le128(page + 176);
    out.data_bytes_read = out.data_units_read * 512000u;
    out.data_bytes_written = out.data_units_written * 512000u;
    return OK;
}

} // namespace health
} // namespace cr
