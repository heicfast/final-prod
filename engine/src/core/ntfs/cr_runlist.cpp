/* cr_runlist.cpp - NTFS runlist codec.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_runlist.h"
#include <cstring>
#include <limits>

namespace cr {
namespace ntfs {

namespace {
/* signed little-endian value of n bytes (1..8), offset field semantics */
i64 read_signed_le(const u8* p, u32 n) {
    i64 v = 0;
    for (u32 i = 0; i < n; ++i)
        v |= static_cast<i64>(p[i]) << (8 * i);
    /* sign-extend from the top used byte */
    const u32 bits = 8 * n;
    if (bits < 64) {
        i64 sign_bit = static_cast<i64>(1) << (bits - 1);
        if (v & sign_bit)
            v |= ~(sign_bit * 2 - 1);
    }
    return v;
}
u64 read_unsigned_le(const u8* p, u32 n) {
    u64 v = 0;
    for (u32 i = 0; i < n; ++i)
        v |= static_cast<u64>(p[i]) << (8 * i);
    return v;
}
u32 width_of(u64 v) {
    u32 w = 1;
    while (w < 8 && (v >> (8 * w)) != 0) /* fuzzer: w==8 shifts by 64 */
        ++w;
    return w;
}
/* Width for a u64-space delta (two's-complement round-trip): the delta
 * mod 2^64 must fit in w bytes interpreted as signed. */
u32 signed_width_of_delta(u64 d) {
    for (u32 w = 1; w <= 8; ++w) {
        const u64 bits = 8 * u64(w);
        const u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
        const u64 masked = d & mask;
        /* sign-extend masked to i64 and compare against d's wrap */
        const u64 sign = (bits >= 64) ? 0 : (1ULL << (bits - 1));
        const u64 ext = (masked & sign) ? (masked | ~mask) : masked;
        if (ext == d)
            return w;
    }
    return 8;
}
} // namespace

u32 RunList::decode(const u8* data, u32 bytes, u64 first_vcn, u64 max_clusters, RunList& out) {
    out.runs_.clear();
    if (data == nullptr)
        return ERR_BAD_PARAMETER;
    u32 pos = 0;
    u64 vcn = first_vcn;
    u64 lcn = 0;
    u64 accumulated = 0;
    while (true) {
        if (pos >= bytes)
            return ERR_TRUNCATED;
        const u8 header = data[pos];
        if (header == 0)
            break; /* terminator */
        /* SPEC order (Linux fs/ntfs3 run.c: size_size = *run_buf & 0xF,
         * offset_size = *run_buf >> 4): LOW nibble = length-field width,
         * HIGH nibble = offset-field width. The corpus artifact's
         * "high = length" prose misread the order-agnostic size sum. */
        const u32 len_width = header & 0xF;
        const u32 off_width = header >> 4;
        if (len_width == 0 || len_width > 8)
            return ERR_BAD_PARAMETER;
        if (off_width > 8)
            return ERR_BAD_PARAMETER;
        if (pos + 1 + len_width + off_width > bytes)
            return ERR_TRUNCATED;

        const u64 run_len = read_unsigned_le(data + pos + 1, len_width);
        if (run_len == 0)
            return ERR_BAD_PARAMETER; /* zero-length run is invalid */
        accumulated += run_len;
        if (accumulated > max_clusters)
            return ERR_BAD_PARAMETER;
        if (out.runs_.size() >= RUNLIST_MAX_RUNS)
            return ERR_CYCLE_LIMIT;

        i64 delta = 0;
        bool sparse = (off_width == 0);
        if (!sparse)
            delta = read_signed_le(data + pos + 1 + len_width, off_width);

        if (!sparse)
            lcn = (delta < 0) ? lcn - static_cast<u64>(-delta) : lcn + static_cast<u64>(delta);
        DataRun r;
        r.vcn = vcn;
        r.length = run_len;
        r.offset = delta;
        r.lcn = sparse ? 0 : lcn;
        r.sparse = sparse;
        out.runs_.push_back(r);

        vcn += run_len;
        pos += 1 + len_width + off_width;
    }
    return OK;
}

std::vector<u8> RunList::encode() const {
    std::vector<u8> out;
    u64 prev_lcn = 0;
    for (const DataRun& r : runs_) {
        u64 lcn_val = r.sparse ? 0 : r.lcn;
        /* fuzzer: extreme lcns make the i64 delta overflow (UB); compute
         * in u64 space and emit only representable deltas, clamping the
         * rest into the 8-byte signed window */
        u64 delta_u = r.sparse ? 0 : lcn_val - prev_lcn;
        u64 len_width = width_of(r.length);
        u64 off_width = r.sparse ? 0 : signed_width_of_delta(delta_u);
        out.push_back(static_cast<u8>((off_width << 4) | len_width));
        /* length (unsigned) */
        for (u32 i = 0; i < len_width; ++i)
            out.push_back(static_cast<u8>(r.length >> (8 * i)));
        /* offset (two's complement of the u64-space delta) */
        if (!r.sparse) {
            u64 bits = 8 * off_width;
            u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
            u64 enc = delta_u & mask;
            for (u32 i = 0; i < off_width; ++i)
                out.push_back(static_cast<u8>(enc >> (8 * i)));
            prev_lcn = lcn_val;
        }
    }
    out.push_back(0); /* terminator */
    return out;
}

u64 RunList::total_clusters() const {
    u64 t = 0;
    for (const auto& r : runs_)
        t += r.length;
    return t;
}

bool RunList::vcn_to_lcn(u64 vcn, u64* lcn, bool* sparse_out) const {
    if (runs_.empty())
        return false;
    u64 lo = 0, hi = runs_.size() - 1;
    while (lo <= hi) {
        u64 mid = (lo + hi) / 2;
        const DataRun& r = runs_[mid];
        if (vcn < r.vcn) {
            if (mid == 0)
                return false;
            hi = mid - 1;
        } else if (vcn >= r.vcn + r.length) {
            lo = mid + 1;
        } else {
            if (r.sparse) {
                if (sparse_out)
                    *sparse_out = true;
                if (lcn)
                    *lcn = 0;
            } else {
                if (sparse_out)
                    *sparse_out = false;
                if (lcn)
                    *lcn = r.lcn + (vcn - r.vcn);
            }
            return true;
        }
    }
    return false;
}

void RunList::extents(std::vector<Extent>& out) const {
    out.clear();
    for (const auto& r : runs_) {
        if (r.sparse)
            continue;
        if (!out.empty() && out.back().lcn + out.back().clusters == r.lcn) {
            out.back().clusters += r.length;
        } else {
            out.push_back(Extent{r.lcn, r.length});
        }
    }
}

} // namespace ntfs
} // namespace cr
