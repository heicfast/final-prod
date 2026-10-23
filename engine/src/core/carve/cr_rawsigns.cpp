/* cr_rawsigns.cpp - the raw-signature layer runtime (index + probes).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/carve/cr_rawsigns.h"
#include <mutex>

namespace cr {
namespace carve {
namespace rawsigns {

namespace {
std::once_flag g_once;
SignIndex* g_index = nullptr;
} // namespace

SignIndex::SignIndex() {
    /* buckets over the sorted header-sign array: key = offset*256 + first */
    for (u32 i = 0; i < kHeaderSignCount; ++i) {
        const RawSign& s = kHeaderSigns[i];
        const u32 key = s.offset * 256 + s.bytes[0];
        if (buckets_[key].begin == 0 && buckets_[key].end == 0)
            buckets_[key].begin = i;
        buckets_[key].end = i + 1;
    }
}

const SignIndex& SignIndex::get() {
    std::call_once(g_once, [] { g_index = new SignIndex(); });
    return *g_index;
}

void SignIndex::probe_header(const u8* p, u64 avail, std::vector<const RawSign*>& out) const {
    out.clear();
    for (u32 off = 0; off < 47; ++off) {
        if (u64(off) >= avail)
            break;
        const u32 key = off * 256 + p[off];
        const Range& r = buckets_[key];
        for (u32 i = r.begin; i < r.end; ++i) {
            const RawSign& s = kHeaderSigns[i];
            if (u64(off) + s.len > avail)
                continue;
            if (std::memcmp(p + off, s.bytes, s.len) == 0)
                out.push_back(&s);
        }
    }
}

bool SignIndex::scan_tail(const u8* buf, u64 len, TailHit& out) const {
    /* stride the buffer at sector boundaries; test every tail sign whose
     * (offset, first byte) matches. The tail sign array is small (62);
     * a per-stride linear scan over a first-byte bucket is fine. */
    for (u64 pos = 0; pos + 512 <= len; pos += 512) {
        for (u32 i = 0; i < kTailSignCount; ++i) {
            const RawSign& s = kTailSigns[i];
            if (u64(s.offset) + s.len > 512)
                continue; /* tail sign must fit in one stride head */
            if (std::memcmp(buf + pos + s.offset, s.bytes, s.len) != 0)
                continue;
            out.sign = &s;
            out.pos = pos;
            return true;
        }
    }
    return false;
}

} // namespace rawsigns
} // namespace carve
} // namespace cr
