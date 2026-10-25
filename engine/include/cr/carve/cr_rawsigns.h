/*
 * cr_rawsigns.h - the raw-signature carving layer (R4 layer 1).
 *
 * The og's first carving pass: a table of per-extension header/tail byte
 * patterns (raw_signs.json, 139 formats) with (offset, confidence, min/max
 * size) semantics - a cheap sector-granular prefilter that covers formats
 * without full grammar validators. Our grammar validators (30 formats)
 * remain the precision layer; this is the breadth layer, deduped against
 * both.
 *
 * Matching semantics (documented divergences from the og):
 *   - header signs are probed at 512-byte-stride file starts; a sign's
 *     offset is relative to that start (corpus offsets are 0..46)
 *   - tail signs are searched at the same stride within [start+min,
 *     start+max), walking forward from start+min; a tail hit closes the
 *     file at the end of that sector
 *   - header-only hit: extent = min(max_size, device end - start); the
 *     sign confidence carries through
 *   - header+tail hit: confidence = (header + tail) / 2 + 100 bonus,
 *     capped at 1000
 *   - tail-walk bound: the og streams the ENTIRE KiB-scaled window
 *     (FUN_1800205b0 returns 2 "need more data" until the span reaches
 *     max*0x400 KiB-bytes - up to 200 MiB for the media entries). We
 *     forward-read at most kRawSignTailScanLimit (8 MiB) per candidate
 *     and fall back to header-only emission for tails beyond it
 *     (S-015 fix: named bound, og's true range documented here)
 *
 * Size units (S-015, decoded): the corpus raw_signs.json min/max fields
 * are KiB counts; the compiled table carries BYTES (converted once in
 * scripts/gen_rawsigns.py). The og's own gates: span < max*0x400 needs
 * more data; emitted size = max<<10; tail acceptance requires
 * size >= min<<10; absent/zero max defaults to 500 MiB in the og
 * (0x7d000 KiB) - the generator REFUSES zero-max entries instead of
 * shipping a silent default divergence.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_RAWSIGNS_H_
#define CR_RAWSIGNS_H_

#include "../cr_types.h"
#include <cstring>
#include <vector>

namespace cr {
namespace carve {
namespace rawsigns {

constexpr u32 kMaxSignBytes = 40;

/* S-015: per-candidate bound on the forward-read tail walk. The og streams
 * the whole [min<<10, max<<10) window (up to 200 MiB); we read at most
 * this many bytes past start+min looking for a tail sign, then emit
 * header-only. 8 MiB keeps the worst case (a raw-sign-heavy unallocated
 * region) bounded while covering the og's common tail distances. */
constexpr u64 kRawSignTailScanLimit = 8ull << 20;

struct RawSignFormat {
    const char* ext;
    u64 min_size;
    u64 max_size;
};

struct RawSign {
    u32 offset; /* pattern position within the file start */
    u16 len;
    u8 bytes[kMaxSignBytes];
    u16 conf; /* 0..1000 */
    u32 fmt;  /* index into kFormats */
};

/* generated in cr_rawsigns_data.cpp */
extern const RawSignFormat kFormats[];
extern const RawSign kHeaderSigns[];
extern const RawSign kTailSigns[];
extern const u32 kFormatCount;
extern const u32 kHeaderSignCount;
extern const u32 kTailSignCount;

/* W4 (AUDIT-PIPELINE register row 5): the SignBucket struct was never
 * referenced - SignIndex dispatches over its private Range buckets. The
 * dead public type was deleted. */

class SignIndex {
public:
    /* sector-stride probe at `p` (points at the candidate file start):
     * fills out with every header sign matching at p. */
    void probe_header(const u8* p, u64 avail, std::vector<const RawSign*>& out) const;

    /* stride-scan [buf, buf+len) for tail signs; returns the first hit
     * (sign + absolute position within the buffer). */
    struct TailHit {
        const RawSign* sign;
        u64 pos; /* buffer-relative position of the matching stride */
    };
    bool scan_tail(const u8* buf, u64 len, TailHit& out) const;

    /* S-015 companion (og-parity tail scoping): the og's tail walk
     * (FUN_1800205b0) iterates the CANDIDATE FORMAT'S OWN tail-sign list
     * - the record at param_1+0x118 owns min/max and the tail list that
     * the matching loop walks. Cross-format tail pairing fabricated
     * close positions (a .sample candidate closed by flp's tail, etc.).
     * This overload restricts the scan to the candidate's format. */
    bool scan_tail_for_format(const u8* buf, u64 len, u32 fmt, TailHit& out) const;

    /* S-015: the documented tail-walk bound (kRawSignTailScanLimit). */
    static constexpr u64 tail_scan_limit() { return kRawSignTailScanLimit; }

    u32 format_count() const { return kFormatCount; }
    const RawSignFormat& format(u32 i) const { return kFormats[i]; }

    static const SignIndex& get();

private:
    SignIndex();
    bool scan_tail_impl(const u8* buf, u64 len, i64 fmt_filter, TailHit& out) const;
    /* bucket key: offset * 256 + first byte -> index range in kHeaderSigns */
    struct Range {
        u32 begin;
        u32 end;
    };
    Range buckets_[47 * 256] = {};
};

} // namespace rawsigns
} // namespace carve
} // namespace cr

#endif // CR_RAWSIGNS_H_
