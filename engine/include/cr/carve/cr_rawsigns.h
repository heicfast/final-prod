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
 * Matching semantics (documented divergences from the og, whose exact hit
 * handler was not RE'd to completion):
 *   - header signs are probed at 512-byte-stride file starts; a sign's
 *     offset is relative to that start (corpus offsets are 0..46)
 *   - tail signs are searched at the same stride within [start+min,
 *     start+max); a tail hit closes the file at the end of that sector
 *   - header-only hit: extent = min(max_size, device end - start); the
 *     sign confidence carries through
 *   - header+tail hit: confidence = (header + tail) / 2 + 100 bonus,
 *     capped at 1000
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

    u32 format_count() const { return kFormatCount; }
    const RawSignFormat& format(u32 i) const { return kFormats[i]; }

    static const SignIndex& get();

private:
    SignIndex();
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
