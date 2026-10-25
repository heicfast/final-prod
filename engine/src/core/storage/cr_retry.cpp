/* cr_retry.cpp - retrying, bad-sector-isolating transport wrapper.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_retry.h"
#include <cstring>
#include <vector>

namespace cr {

namespace {

/* Bisection depth budget: from a 1 MiB request down to 512-byte sectors is
 * 11 halvings; a small margin covers odd sizes. Bounded so hostile geometry
 * cannot force unbounded recursion. */
constexpr u32 kMaxBisectDepth = 24;

} // namespace

std::unique_ptr<RetryingDevice> RetryingDevice::create(std::unique_ptr<IStorageDevice> inner,
                                                       const RetryPolicy& policy) {
    if (!inner)
        return nullptr;
    return std::unique_ptr<RetryingDevice>(new RetryingDevice(std::move(inner), policy));
}

RetryingDevice::RetryingDevice(std::unique_ptr<IStorageDevice> inner, const RetryPolicy& policy)
    : inner_(std::move(inner)), policy_(policy) {}

const ReadErrorMap& RetryingDevice::error_map() const {
    std::lock_guard<std::mutex> lock(map_mutex_);
    return map_;
}

ReadErrorMap RetryingDevice::error_map_copy() const {
    std::lock_guard<std::mutex> lock(map_mutex_);
    return map_; /* value copy under the lock: race-free snapshot (G-06) */
}

u32 RetryingDevice::try_read(u64 offset, void* buffer, u32 length) {
    for (u32 attempt = 1;; ++attempt) {
        const u32 rc = inner_->read_at(offset, buffer, length);
        if (rc == OK)
            return OK;
        /* Out-of-bounds / bad-parameter failures are deterministic - retrying
         * cannot help; only IO-class failures are retried. */
        if (rc != ERR_READ_FAILED && rc != ERR_RETRY_EXHAUSTED)
            return rc;
        if (attempt >= policy_.max_attempts || attempt == 0xFFFFFFFFu)
            return rc;
    }
}

u32 RetryingDevice::read_at(u64 offset, void* buffer, u32 length) {
    if (!buffer || length == 0)
        return ERR_BAD_PARAMETER;
    if (offset > size() || length > size() - offset)
        return ERR_RANGE_OUT_OF_BOUNDS;
    return read_isolated(offset, static_cast<u8*>(buffer), length, kMaxBisectDepth);
}

u32 RetryingDevice::read_isolated(u64 offset, u8* buffer, u32 length, u32 depth_budget) {
    const u32 rc = try_read(offset, buffer, length);
    if (rc == OK)
        return OK;

    /* The whole span failed persistently. Distinguish hard errors (never
     * retried further) from IO failures that warrant isolation. */
    if (rc != ERR_READ_FAILED && rc != ERR_RETRY_EXHAUSTED)
        return rc;

    const u32 sector = inner_->geometry().logical_sector ? inner_->geometry().logical_sector : 512;
    /* N-004: the og's CatchBuffer salvage - one shifted-window second
     * chance before the span is bisected/marked bad. The og (ext4 engine,
     * CEXT4CatchBufferDeviceProxy, corpus ext4-engine--deep-dive.md §3)
     * re-reads [off-0x400, off+len) into a fresh buffer when the primary
     * window fails; that catches alignment-dependent transport failures
     * the plain retry (same window) cannot. Step = max(0x400, sector):
     * og-exact at 512n, still aligned at 4Kn (deliberate improvement - the
     * og's hardcoded 0x400 is misaligned on 4Kn). */
    if (policy_.backtrack) {
        const u64 back = sector > 0x400 ? sector : 0x400;
        /* Window must stay sector-aligned at BOTH ends and inside the
         * device; length rounded up to a whole sector so the shifted
         * request cannot end mid-sector (the tail beyond `length` is
         * dropped, the overlap [offset, offset+length) is what we keep). */
        const u32 win_len = u32(back + u64(length + sector - 1) / sector * sector);
        if (offset >= back && offset - back + win_len <= size()) {
            /* fresh buffer per catch, like the og's 0xE00 CatchBuffer
             * allocation (heap: no 12 KiB+ stack risk on worker threads) */
            std::vector<u8> win(win_len);
            if (inner_->read_at(offset - back, win.data(), win_len) == OK) {
                memcpy(buffer, win.data() + back, length);
                return OK; /* salvaged - range never recorded bad */
            }
        }
    }
    /* Bisect while a half would still be at least one full sector. */
    if (depth_budget == 0 || length <= sector || length / 2 < sector) {
        /* Cannot or must not bisect further: this range is bad. */
        {
            std::lock_guard<std::mutex> lock(map_mutex_);
            map_.add(offset, length, policy_.max_attempts);
        }
        if (policy_.mode == RetryMode::Skip) {
            memset(buffer, 0, length);
            return OK;
        }
        return ERR_RETRY_EXHAUSTED;
    }

    /* Bisect: salvage the good half, isolate the bad half. */
    const u32 half = length / 2;
    const u32 rc_lo = read_isolated(offset, buffer, half, depth_budget - 1);
    const u32 rc_hi = read_isolated(offset + half, buffer + half, length - half, depth_budget - 1);
    if (policy_.mode == RetryMode::Skip)
        return OK; /* bad halves were zero-filled (or are hard errors) */
    if (rc_lo != OK)
        return rc_lo;
    if (rc_hi != OK)
        return rc_hi;
    return OK;
}

} // namespace cr
