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
    while ((v >> (8 * w)) != 0)
        ++w;
    return w;
}
/* Minimal byte width holding v as a SIGNED little-endian field that
 * round-trips through read_signed_le: need -2^(8w-1) <= v < 2^(8w-1).
 * This is stricter than width_of: +128 needs 2 bytes (1 byte would decode
 * as -128), and -129 needs 2 bytes (1 byte cannot hold it at all). */
u32 signed_width_of(i64 v) {
    for (u32 w = 1; w <= 8; ++w) {
        const i64 bound =
            (w == 8) ? (std::numeric_limits<i64>::max)() : ((i64(1) << (8 * w - 1)) - 1);
        const i64 nbound = (w == 8) ? (std::numeric_limits<i64>::min)() : -(i64(1) << (8 * w - 1));
        if (v <= bound && v >= nbound)
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
        const u32 len_width = header >> 4;
        const u32 off_width = header & 0xF;
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
        i64 delta = 0;
        if (!r.sparse)
            delta = static_cast<i64>(lcn_val) - static_cast<i64>(prev_lcn);
        u32 len_width = width_of(r.length);
        u32 off_width = r.sparse ? 0 : signed_width_of(delta);
        out.push_back(static_cast<u8>((len_width << 4) | off_width));
        /* length (unsigned) */
        for (u32 i = 0; i < len_width; ++i)
            out.push_back(static_cast<u8>(r.length >> (8 * i)));
        /* offset (two's complement of signed value) */
        if (!r.sparse) {
            u64 bits = 8 * off_width;
            u64 mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1);
            u64 enc = static_cast<u64>(delta) & mask;
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
