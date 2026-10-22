/* cr_devmap.cpp - ReadErrorMap implementation (R1.6).
 *
 * Every failed transport read lands here and travels with the scan result
 * and the session record. Reference defect: none (reference had no persistent
 * error map at all - this is a clean-room addition required by R1.6).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_device.h"
#include <algorithm>

namespace cr {

void ReadErrorMap::add(u64 offset, u64 length, u32 attempts) {
    if (length == 0) return;
    bool merged = false;
    for (auto &r : ranges_) {
        if (offset <= r.offset + r.length && offset + length >= r.offset) {
            u64 new_start = std::min(r.offset, offset);
            u64 new_end = std::max(r.offset + r.length, offset + length);
            r.offset = new_start;
            r.length = new_end - new_start;
            r.attempts = std::max(r.attempts, attempts);
            merged = true;
            break;
        }
    }
    if (!merged) {
        ranges_.push_back(ReadErrorRange{offset, length, attempts});
    }
    /* normalize on EVERY add: sort + absorb overlapping/touching runs
     * (a merged range may now swallow later ranges entirely) */
    std::sort(ranges_.begin(), ranges_.end(),
              [](const ReadErrorRange &a, const ReadErrorRange &b) { return a.offset < b.offset; });
    for (size_t i = 0; i + 1 < ranges_.size();) {
        if (ranges_[i].offset + ranges_[i].length >= ranges_[i + 1].offset) {
            u64 end = std::max(ranges_[i].offset + ranges_[i].length,
                               ranges_[i + 1].offset + ranges_[i + 1].length);
            ranges_[i].length = end - ranges_[i].offset;
            ranges_[i].attempts = std::max(ranges_[i].attempts, ranges_[i + 1].attempts);
            ranges_.erase(ranges_.begin() + i + 1);
        } else {
            ++i;
        }
    }
}

u64 ReadErrorMap::total_bytes() const {
    u64 t = 0;
    for (const auto &r : ranges_) t += r.length;
    return t;
}

void ReadErrorMap::merge(const ReadErrorMap &other) {
    for (const auto &r : other.ranges_) add(r.offset, r.length, r.attempts);
}

void ReadErrorMap::serialize(std::vector<u8> &out) const {
    auto put32 = [&out](u32 v) {
        out.push_back(static_cast<u8>(v));
        out.push_back(static_cast<u8>(v >> 8));
        out.push_back(static_cast<u8>(v >> 16));
        out.push_back(static_cast<u8>(v >> 24));
    };
    put32(static_cast<u32>(ranges_.size()));
    for (const auto &r : ranges_) {
        put32(static_cast<u32>(r.offset));
        put32(static_cast<u32>(r.offset >> 32));
        put32(static_cast<u32>(r.length));
        put32(static_cast<u32>(r.length >> 32));
        put32(r.attempts);
    }
}

ReadErrorMap ReadErrorMap::deserialize(const u8 *data, size_t len, u32 *consumed) {
    ReadErrorMap m;
    u32 used = 0;
    auto get32 = [&data, &len, &used](u32 *out) -> bool {
        if (used + 4 > len) return false;
        *out = cr::u32_from_bytes_le(data + used);
        used += 4;
        return true;
    };
    u32 count = 0;
    if (!get32(&count)) {
        if (consumed) *consumed = used;
        return m;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 o_lo = 0, o_hi = 0, l_lo = 0, l_hi = 0, att = 0;
        if (!get32(&o_lo) || !get32(&o_hi) || !get32(&l_lo) || !get32(&l_hi) || !get32(&att))
            break;
        m.add((static_cast<u64>(o_hi) << 32) | o_lo, (static_cast<u64>(l_hi) << 32) | l_lo, att);
    }
    if (consumed) *consumed = used;
    return m;
}

} // namespace cr
