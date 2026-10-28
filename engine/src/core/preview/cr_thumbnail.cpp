/* cr_thumbnail.cpp - engine-side thumbnail generation over the bounded
 * PreviewReader (see cr_thumbnail.h for the contract and the corpus
 * anchors; docs/PARITY-OG-CORE-INVENTORY.md Section C.2 for the phased
 * format matrix).
 *
 * Decode layering:
 *   - JPEG / PNG / BMP / GIF / PSD / PNM / HDR: the vendored public-domain
 *     decoder (third_party/stb/stb_image.h, STBI_NO_STDIO, fed through a
 *     budget-gated callback adapter over PreviewReader - the engine never
 *     opens files);
 *   - TIFF baseline: a native, bounded decoder (uncompressed / PackBits /
 *     Deflate strips, 8/16-bit, gray / palette / RGB / RGBA, both endians),
 *     following the repo's own-parser doctrine; predictor-2, tiled TIFF and
 *     JPEG-in-TIFF are honestly refused (documented phase-2);
 *   - JPEG EXIF embedded-thumbnail fast path: native APP1/TIFF walk
 *     (IFD0 -> IFD1, tags 0x201/0x202) for oversized inputs - the
 *     og-parity strategy (GDI+/LibRaw/libheif all prefer embedded thumbs).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/preview/cr_thumbnail.h"

#include <cstring>
#include <zlib.h>

/* ---------------------------------------------------------------- */
/* the vendored decoder (public domain; no vcpkg surface - the license
 * ships beside it in third_party/stb/LICENSE) */
/* ---------------------------------------------------------------- */
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100 4189 4244 4245 4267 4456 4457 4701 4702 4706)
#else
#pragma GCC diagnostic push
/* -Wmaybe-uninitialized is a GCC-only group: clang's equivalent lives under
 * -Wconditional-uninitialized, and an unknown group under -Werror fails the
 * whole fuzz build (-Werror,-Wunknown-warning-option, run 40680931476) */
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#define STBI_NO_STDIO 1
#define STBI_MAX_DIMENSIONS 32768 /* belt under the dims-first bomb guard */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

namespace cr {
namespace preview {

/* ---------------------------------------------------------------- */
/* status / capability surface */
/* ---------------------------------------------------------------- */

const char* thumbnail_status_name(ThumbStatus s) {
    switch (s) {
    case ThumbStatus::Ok:
        return "ok";
    case ThumbStatus::Unsupported:
        return "unsupported";
    case ThumbStatus::DecodeFailed:
        return "decode-failed";
    case ThumbStatus::Oversized:
        return "oversized";
    case ThumbStatus::BudgetExceeded:
        return "budget-exceeded";
    case ThumbStatus::UnreadableSpan:
        return "unreadable-span";
    case ThumbStatus::BadParameter:
        return "bad-parameter";
    }
    return "?";
}

bool thumbnail_format_supported(std::string_view f) {
    return f == "jpeg" || f == "png" || f == "bmp" || f == "gif" || f == "tiff" || f == "psd" ||
           f == "pnm" || f == "hdr";
}

/* ---------------------------------------------------------------- */
/* internal: shared budget + fetch (the M-028 cost doctrine applied to
 * thumbnail generation - every logical byte pulled from the reader is
 * accounted BEFORE it moves; work units bound the fetch count) */
/* ---------------------------------------------------------------- */
namespace {

struct Budget {
    u32 read_left;
    u32 work_left;
    bool over = false; /* read or work exhausted */
    void spend_read(u32 n) {
        if (read_left <= n) {
            read_left = 0;
            over = true;
        } else {
            read_left -= n;
        }
    }
    bool spend_work() {
        if (work_left == 0) {
            over = true;
            return false;
        }
        --work_left;
        return true;
    }
};

/* Random-access bounded fetch: returns bytes placed (0 on budget/EOF). */
u32 bfetch(PreviewReader& pr, Budget& b, u64 off, void* buf, u32 len) {
    if (len == 0)
        return 0;
    if (!b.spend_work() || b.read_left == 0) {
        b.over = true;
        return 0;
    }
    u32 want = len < b.read_left ? len : b.read_left;
    const u32 got = pr.read_at(off, buf, want);
    b.spend_read(got);
    return got;
}

/* Sequential adapter for the stb callbacks: stb reads forward-only, so the
 * position lives here; every callback is budget-gated. */
class StbioSource {
public:
    StbioSource(PreviewReader& pr, Budget& b) : pr_(pr), b_(b) {}

    stbi_io_callbacks callbacks() {
        stbi_io_callbacks cb;
        cb.read = &StbioSource::read_cb;
        cb.skip = &StbioSource::skip_cb;
        cb.eof = &StbioSource::eof_cb;
        return cb;
    }
    bool hit_eof() const { return eof_; }
    bool over_budget() const { return b_.over; }

private:
    static int read_cb(void* user, char* data, int size) {
        auto* s = static_cast<StbioSource*>(user);
        if (size <= 0)
            return 0;
        if (!s->b_.spend_work() || s->b_.read_left == 0) {
            s->b_.over = true;
            /* budget exhaustion IS an EOF for stb's stream protocol: every
             * stb loop (stbi__skip_jpeg_junk_at_end stb_image.h:3393 and
             * friends) keys on at_eof; a budget-stopped stream that keeps
             * reporting not-at-EOF while reads return 0 spins forever (the
             * fuzz-thumbnail timeout, run 40682713696) */
            s->eof_ = true;
            return 0;
        }
        u32 want = u32(size) < s->b_.read_left ? u32(size) : s->b_.read_left;
        const u32 got = s->pr_.read_at(s->pos_, data, want);
        s->b_.spend_read(got);
        if (got < want)
            s->eof_ = true;
        s->pos_ += got;
        return int(got);
    }
    static void skip_cb(void* user, int n) {
        auto* s = static_cast<StbioSource*>(user);
        if (n > 0) {
            s->pos_ += u64(n);
            if (s->pos_ > s->pr_.size())
                s->eof_ = true;
        }
    }
    static int eof_cb(void* user) {
        auto* s = static_cast<StbioSource*>(user);
        return s->eof_ || s->pos_ >= s->pr_.size() ? 1 : 0;
    }

    PreviewReader& pr_;
    Budget& b_;
    u64 pos_ = 0;
    bool eof_ = false;
};

/* ---------------------------------------------------------------- */
/* sniffing (first bytes; names every family we can see, including the
 * documented phase-2 refusals) */
/* ---------------------------------------------------------------- */

enum class Fmt { Jpeg, Png, Bmp, Gif, Tiff, Psd, Pnm, Hdr, Webp, Heic, Unknown };

struct Sniff {
    Fmt fmt;
    const char* name;
};

Sniff sniff(const u8* p, size_t n) {
    auto starts = [&](const char* s, size_t k) { return n >= k && std::memcmp(p, s, k) == 0; };
    if (n >= 3 && p[0] == 0xFF && p[1] == 0xD8 && p[2] == 0xFF)
        return {Fmt::Jpeg, "jpeg"};
    if (starts("\x89PNG\r\n\x1a\n", 8))
        return {Fmt::Png, "png"};
    if (n >= 2 && p[0] == 'B' && p[1] == 'M')
        return {Fmt::Bmp, "bmp"};
    if (starts("GIF87a", 6) || starts("GIF89a", 6))
        return {Fmt::Gif, "gif"};
    if ((n >= 4 && p[0] == 'I' && p[1] == 'I' && p[2] == 42 && p[3] == 0) ||
        (n >= 4 && p[0] == 'M' && p[1] == 'M' && p[2] == 0 && p[3] == 42))
        return {Fmt::Tiff, "tiff"};
    if (starts("8BPS", 4))
        return {Fmt::Psd, "psd"};
    if (n >= 2 && p[0] == 'P' && p[1] >= '1' && p[1] <= '7')
        return {Fmt::Pnm, "pnm"};
    if (starts("#?RADIANCE\n", 11) || starts("#?RGBE\n", 7))
        return {Fmt::Hdr, "hdr"};
    if (starts("RIFF", 4) && n >= 12 && std::memcmp(p + 8, "WEBP", 4) == 0)
        return {Fmt::Webp, "webp"};
    if (n >= 12 && std::memcmp(p + 4, "ftyp", 4) == 0 &&
        (std::memcmp(p + 8, "heic", 4) == 0 || std::memcmp(p + 8, "heix", 4) == 0 ||
         std::memcmp(p + 8, "hevc", 4) == 0 || std::memcmp(p + 8, "hevx", 4) == 0 ||
         std::memcmp(p + 8, "mif1", 4) == 0))
        return {Fmt::Heic, "heic"};
    return {Fmt::Unknown, "unknown"};
}

/* ---------------------------------------------------------------- */
/* downscale: premultiplied box filter, aspect-preserving, max_edge on
 * BOTH axes (the og's width-derived GDI+ scale quirk corrected), never
 * upscaled */
/* ---------------------------------------------------------------- */

void box_downscale(std::vector<u8>& rgba, u32& w, u32& h, u32 max_edge) {
    if (w == 0 || h == 0)
        return;
    if (w <= max_edge && h <= max_edge)
        return; /* never upscale */
    const u64 big = w > h ? u64(w) : u64(h);
    u32 tw = u32((u64(w) * max_edge + big - 1) / big);
    u32 th = u32((u64(h) * max_edge + big - 1) / big);
    if (tw == 0)
        tw = 1;
    if (th == 0)
        th = 1;
    std::vector<u8> out(size_t(tw) * th * 4);
    const u8* src = rgba.data();
    for (u32 ty = 0; ty < th; ++ty) {
        u64 y0 = (u64(ty) * h) / th;
        u64 y1 = ((u64(ty) + 1) * h) / th;
        if (y1 <= y0)
            y1 = y0 + 1;
        if (y1 > h)
            y1 = h;
        for (u32 tx = 0; tx < tw; ++tx) {
            u64 x0 = (u64(tx) * w) / tw;
            u64 x1 = ((u64(tx) + 1) * w) / tw;
            if (x1 <= x0)
                x1 = x0 + 1;
            if (x1 > w)
                x1 = w;
            u64 c[3] = {0, 0, 0};
            u64 a_sum = 0, area = 0;
            for (u64 y = y0; y < y1; ++y) {
                const u8* row = src + (size_t(y) * w + x0) * 4;
                for (u64 x = x0; x < x1; ++x, row += 4) {
                    const u64 a = row[3];
                    c[0] += u64(row[0]) * a;
                    c[1] += u64(row[1]) * a;
                    c[2] += u64(row[2]) * a;
                    a_sum += a;
                    ++area;
                }
            }
            u8* dst = out.data() + (size_t(ty) * tw + tx) * 4;
            const u64 a_avg = (a_sum + area / 2) / area;
            dst[3] = u8(a_avg);
            if (a_sum > 0) {
                dst[0] = u8((c[0] + a_sum / 2) / a_sum);
                dst[1] = u8((c[1] + a_sum / 2) / a_sum);
                dst[2] = u8((c[2] + a_sum / 2) / a_sum);
            } else {
                dst[0] = dst[1] = dst[2] = 0;
            }
        }
    }
    rgba.swap(out);
    w = tw;
    h = th;
}

} // namespace

/* ---------------------------------------------------------------- */
/* internal: JPEG EXIF embedded-thumbnail extraction (APP1 walk,
 * IFD0 -> IFD1, tags 0x201/0x202; offsets relative to the TIFF header -
 * the EXIF contract) */
/* ---------------------------------------------------------------- */
namespace {

class ExifWalker {
public:
    ExifWalker(PreviewReader& pr, Budget& b) : pr_(pr), b_(b) {}

    /* Returns the embedded JPEG thumbnail bytes, empty when absent. */
    std::vector<u8> embedded_thumbnail(u64 limit) {
        const u64 size = pr_.size();
        u64 off = 2; /* past SOI */
        for (u32 guard = 0; guard < 512 && off + 4 <= size && off < limit; ++guard) {
            u8 m4[4];
            if (bfetch(pr_, b_, off, m4, 2) != 2)
                return {};
            if (m4[0] != 0xFF)
                return {};
            u32 marker = m4[1];
            while (marker == 0xFF) { /* fill bytes */
                ++off;
                if (bfetch(pr_, b_, off, m4, 2) != 2 || m4[0] != 0xFF)
                    return {};
                marker = m4[1];
            }
            if (marker == 0xD9 || marker == 0xDA) /* EOI / SOS: metadata ends */
                return {};
            if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
                off += 2; /* standalone markers */
                continue;
            }
            if (bfetch(pr_, b_, off + 2, m4, 2) != 2)
                return {};
            const u32 seg_len = (u32(m4[0]) << 8) | m4[1];
            if (seg_len < 2)
                return {};
            if (marker == 0xE1) {
                u8 head[6];
                if (bfetch(pr_, b_, off + 4, head, 6) == 6 &&
                    std::memcmp(head, "Exif\0\0", 6) == 0) {
                    auto thumb = parse_tiff_thumbnail(off + 10, u64(seg_len) - 8);
                    if (!thumb.empty())
                        return thumb;
                }
            }
            off += 2 + seg_len;
        }
        return {};
    }

private:
    u32 rd16(const u8* p) const {
        return le_ ? u32(p[0]) | (u32(p[1]) << 8) : (u32(p[0]) << 8) | u32(p[1]);
    }
    u32 rd32(const u8* p) const {
        return le_ ? u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24)
                   : (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
    }
    bool fetch(u64 rel, void* buf, u32 len) {
        return bfetch(pr_, b_, tiff_ + rel, buf, len) == len;
    }

    std::vector<u8> parse_tiff_thumbnail(u64 tiff_abs, u64 seg_payload) {
        u8 hdr[8];
        if (bfetch(pr_, b_, tiff_abs, hdr, 8) != 8)
            return {};
        if (hdr[0] == 'I' && hdr[1] == 'I')
            le_ = true;
        else if (hdr[0] == 'M' && hdr[1] == 'M')
            le_ = false;
        else
            return {};
        if (rd16(hdr + 2) != 42)
            return {};
        const u32 ifd0 = rd32(hdr + 4);
        tiff_ = tiff_abs;
        /* IFD0: find the next-IFD link (IFD1 is the thumbnail IFD) */
        u8 c2[2];
        if (!fetch(ifd0, c2, 2))
            return {};
        const u32 n0 = rd16(c2);
        if (n0 > 256)
            return {};
        u8 link[4];
        if (!fetch(ifd0 + 2 + u64(n0) * 12, link, 4))
            return {};
        const u32 ifd1 = rd32(link);
        if (ifd1 == 0)
            return {};
        u8 c2b[2];
        if (!fetch(ifd1, c2b, 2))
            return {};
        const u32 n1 = rd16(c2b);
        if (n1 == 0 || n1 > 256)
            return {};
        std::vector<u8> ents(size_t(n1) * 12);
        if (!fetch(ifd1 + 2, ents.data(), u32(ents.size())))
            return {};
        u32 thumb_off = 0, thumb_len = 0;
        for (u32 i = 0; i < n1; ++i) {
            const u8* e = ents.data() + size_t(i) * 12;
            const u32 tag = rd16(e);
            const u32 type = rd16(e + 2);
            const u32 count = rd32(e + 4);
            if (tag == 0x0201 && type == 4 && count == 1)
                thumb_off = rd32(e + 8);
            else if (tag == 0x0202 && type == 4 && count == 1)
                thumb_len = rd32(e + 8);
        }
        if (thumb_off == 0 || thumb_len == 0)
            return {};
        /* bounds: inside the file, inside the APP1 payload, inside sanity */
        if (u64(thumb_len) > seg_payload || thumb_off + u64(thumb_len) > seg_payload)
            return {};
        if (tiff_ + thumb_off + u64(thumb_len) > pr_.size() || thumb_len > (64u << 20))
            return {};
        std::vector<u8> jpeg(thumb_len);
        if (!fetch(thumb_off, jpeg.data(), thumb_len))
            return {};
        if (jpeg.size() < 3 || jpeg[0] != 0xFF || jpeg[1] != 0xD8)
            return {}; /* must itself be a JPEG */
        return jpeg;
    }

    PreviewReader& pr_;
    Budget& b_;
    u64 tiff_ = 0;
    bool le_ = true;
};

} // namespace

/* ---------------------------------------------------------------- */
/* internal: native baseline-TIFF decoder (strips; uncompressed /
 * PackBits / Deflate; 8/16-bit; gray / palette / RGB / RGBA; both
 * endians; predictor-1 only) */
/* ---------------------------------------------------------------- */
namespace {

struct TiffTags {
    u32 width = 0, height = 0, spp = 1, bps = 1;
    u32 compression = 1, photometric = 1, planar = 1, predictor = 1;
    u64 rows_per_strip = 0xFFFFFFFF;
    std::vector<u64> strip_offsets, strip_counts;
    u64 colormap_off = 0;
    u32 colormap_count = 0;
};

class TiffDecoder {
public:
    TiffDecoder(PreviewReader& pr, Budget& b) : pr_(pr), b_(b) {}

    /* false = undecodable for v1 (status decided by the caller from
     * b_.over / eof evidence) */
    bool info(TiffTags& t) {
        u8 hdr[8];
        if (bfetch(pr_, b_, 0, hdr, 8) != 8)
            return false;
        if (hdr[0] == 'I' && hdr[1] == 'I')
            le_ = true;
        else if (hdr[0] == 'M' && hdr[1] == 'M')
            le_ = false;
        else
            return false;
        if (rd16(hdr + 2) != 42)
            return false;
        u32 ifd_off = rd32(hdr + 4);
        std::vector<u64> bps_vals;
        for (u32 ifd_n = 0; ifd_n < 4 && ifd_off != 0; ++ifd_n) {
            u8 c2[2];
            if (bfetch(pr_, b_, ifd_off, c2, 2) != 2)
                return false;
            const u32 n = rd16(c2);
            if (n == 0 || n > 1024)
                return false;
            if (ifd_n > 0)
                break; /* tags beyond IFD0 are not v1 inputs */
            std::vector<u8> ents(size_t(n) * 12);
            if (bfetch(pr_, b_, ifd_off + 2, ents.data(), u32(ents.size())) != ents.size())
                return false;
            for (u32 i = 0; i < n; ++i) {
                const u8* e = ents.data() + size_t(i) * 12;
                if (!entry(e, t, bps_vals))
                    return false;
            }
            u8 link[4];
            if (bfetch(pr_, b_, ifd_off + 2 + u64(n) * 12, link, 4) != 4)
                return false;
            ifd_off = rd32(link);
        }
        if (!bps_vals.empty()) {
            const u64 first = bps_vals[0];
            for (u64 v : bps_vals)
                if (v != first)
                    return false; /* mixed sample widths refused */
            /* TIFF bit depths are tiny by construction (8/16/32/64); the
             * 8/16 gate below refuses anything else, so the narrowing is
             * safe (MSVC C4244-as-error, run 40680931476) */
            t.bps = u32(first);
        }
        if (t.bps != 8 && t.bps != 16)
            return false;
        return t.width != 0 && t.height != 0;
    }

    /* Decode into 8-bit RGBA. Returns false on any refused/short strip. */
    bool decode(const TiffTags& t, std::vector<u8>& rgba) {
        if (t.planar != 1 || t.predictor != 1) /* planar config + predictor-2 are v1 refusals */
            return false;
        const u32 spp_eff = effective_spp(t);
        if (spp_eff == 0)
            return false;
        const u32 row_bytes = t.bps == 8 ? t.width * spp_eff : t.width * spp_eff * 2;
        const u64 rps = t.rows_per_strip == 0 ? t.height : t.rows_per_strip;
        const u32 nstrips = u32((t.height + rps - 1) / rps);
        if (t.strip_offsets.size() < nstrips || t.strip_counts.size() < nstrips)
            return false;
        std::vector<u8> samples(size_t(t.width) * t.height * spp_eff);
        std::vector<u8> strip_in, strip_out;
        for (u32 s = 0; s < nstrips; ++s) {
            const u32 rows = u32(rps * (s + 1) > t.height ? t.height - rps * s : rps);
            if (rows == 0)
                continue;
            const u64 want_out = u64(row_bytes) * rows;
            const u64 src_len = t.strip_counts[s];
            if (src_len == 0 || src_len > (256u << 20))
                return false;
            strip_in.resize(size_t(src_len));
            if (bfetch(pr_, b_, t.strip_offsets[s], strip_in.data(), u32(src_len)) != src_len)
                return false;
            if (!uncompress_strip(t, strip_in, strip_out, size_t(want_out)))
                return false;
            /* lay the rows down (16-bit narrows to 8 on the way) */
            const u32 sample_stride = t.bps == 8 ? 1 : 2;
            for (u32 r = 0; r < rows; ++r) {
                const u8* src = strip_out.data() + size_t(r) * row_bytes;
                u8* dst = samples.data() + (size_t(rps * s + r) * t.width) * spp_eff;
                if (t.bps == 8) {
                    std::memcpy(dst, src, size_t(t.width) * spp_eff);
                } else {
                    for (u32 i = 0; i < t.width * spp_eff; ++i) {
                        dst[i] = u8(rd16(src + size_t(i) * sample_stride) >> 8);
                    }
                }
            }
        }
        return to_rgba(t, samples, rgba);
    }

    bool failed_short() const { return b_.over; }

private:
    static u32 effective_spp(const TiffTags& t) {
        if (t.photometric == 3)
            return t.spp == 1 ? 1 : 0; /* palette must be single-band */
        if (t.photometric == 2) {
            if (t.spp < 3)
                return 0;
            return t.spp >= 4 ? 4 : 3; /* RGB(A) */
        }
        if (t.photometric == 0 || t.photometric == 1)
            return t.spp == 1 ? 1 : 0; /* grayscale */
        return 0;
    }

    bool entry(const u8* e, TiffTags& t, std::vector<u64>& bps_vals) {
        const u32 tag = rd16(e);
        const u32 type = rd16(e + 2);
        const u32 count = rd32(e + 4);
        /* value bytes for the classic types */
        u32 unit = 0;
        switch (type) {
        case 1:
            unit = 1;
            break; /* BYTE */
        case 3:
            unit = 2;
            break; /* SHORT */
        case 4:
            unit = 4;
            break; /* LONG */
        case 16:
            unit = 8;
            break; /* LONG8 - out-of-line only, ignored */
        default:
            unit = 0;
            break;
        }
        const auto val =
            [&](u32 idx) -> u64 { /* reads one value, inline or indirect */
                                  if (unit == 0)
                                      return 0;
                                  const u32 total = count * unit;
                                  u8 tmp[8] = {
                                      0}; /* zero-init: the LONG8 tail is never read inline */
                                  if (total <= 4) {
                                      std::memcpy(tmp, e + 8, 4);
                                  } else {
                                      const u64 off = rd32(e + 8) + u64(idx) * unit;
                                      if (bfetch(pr_, b_, off, tmp, unit) != unit)
                                          return 0;
                                  }
                                  if (unit == 1)
                                      return tmp[0];
                                  if (unit == 2)
                                      return rd16(tmp);
                                  if (unit == 4)
                                      return rd32(tmp);
                                  return rd32(tmp) | (u64(rd32(tmp + 4)) << 32);
        };
        switch (tag) {
        case 256:
            t.width = u32(val(0));
            break;
        case 257:
            t.height = u32(val(0));
            break;
        case 258:
            for (u32 i = 0; i < count && i < 8; ++i)
                bps_vals.push_back(val(i));
            break;
        case 259:
            t.compression = u32(val(0));
            break;
        case 262:
            t.photometric = u32(val(0));
            break;
        case 273:
            t.strip_offsets.clear();
            for (u32 i = 0; i < count && i < 65536; ++i)
                t.strip_offsets.push_back(val(i));
            break;
        case 277:
            t.spp = u32(val(0));
            break;
        case 278:
            t.rows_per_strip = val(0);
            break;
        case 279:
            t.strip_counts.clear();
            for (u32 i = 0; i < count && i < 65536; ++i)
                t.strip_counts.push_back(val(i));
            break;
        case 284:
            t.planar = u32(val(0));
            break;
        case 317:
            t.predictor = u32(val(0));
            break;
        case 320:
            t.colormap_off = rd32(e + 8);
            t.colormap_count = count;
            break;
        default:
            break;
        }
        return true;
    }

    bool uncompress_strip(const TiffTags& t, const std::vector<u8>& in, std::vector<u8>& out,
                          size_t want_out) {
        if (t.compression == 1) {
            if (in.size() < want_out)
                return false;
            out.assign(in.begin(), in.begin() + want_out);
            return true;
        }
        if (t.compression == 8 || t.compression == 32946) { /* Deflate (zlib-wrapped) */
            out.resize(want_out + 4096);
            uLongf got = uLongf(out.size());
            const int rc = uncompress(out.data(), &got, in.data(), uLong(in.size()));
            if (rc != Z_OK || got < want_out)
                return false;
            out.resize(size_t(got));
            return true;
        }
        if (t.compression == 32773) { /* PackBits */
            out.clear();
            out.reserve(want_out);
            size_t i = 0;
            while (i < in.size() && out.size() < want_out) {
                const int8_t n = int8_t(in[i++]);
                if (n >= 0) {
                    const size_t lit = size_t(n) + 1;
                    if (i + lit > in.size() || out.size() + lit > want_out)
                        return false;
                    out.insert(out.end(), in.begin() + i, in.begin() + i + lit);
                    i += lit;
                } else if (n != -128) {
                    /* TIFF6 spec: -127 <= n <= -1 -> repeat the next byte
                     * 1-n times (2..128); 257-n is the UNSIGNED-byte form */
                    const size_t rep = size_t(1 - int(n));
                    if (i >= in.size() || out.size() + rep > want_out)
                        return false;
                    out.insert(out.end(), rep, in[i++]);
                } /* -128: no-op */
            }
            return out.size() == want_out;
        }
        return false; /* CCITT/ JPEG-in-TIFF / others: honest v1 refusal */
    }

    bool to_rgba(const TiffTags& t, const std::vector<u8>& samples, std::vector<u8>& rgba) {
        const u32 spp = effective_spp(t);
        if (samples.size() < size_t(t.width) * t.height * spp)
            return false;
        rgba.resize(size_t(t.width) * t.height * 4);
        if (t.photometric == 3) { /* palette: colormap = 3 planes of u16 */
            if (t.colormap_off == 0 || t.colormap_count < 3u * (1u << t.bps))
                return false;
            const u32 ncolors = 1u << t.bps;
            std::vector<u8> cm(size_t(t.colormap_count) * 2);
            if (bfetch(pr_, b_, t.colormap_off, cm.data(), u32(cm.size())) != cm.size())
                return false;
            auto plane = [&](u32 p, u32 idx) -> u8 {
                const u8* q = cm.data() + (size_t(p) * ncolors + idx) * 2;
                return u8(rd16(q) >> 8);
            };
            for (u32 i = 0; i < t.width * t.height; ++i) {
                const u8 idx = samples[i];
                u8* d = rgba.data() + size_t(i) * 4;
                d[0] = plane(0, idx);
                d[1] = plane(1, idx);
                d[2] = plane(2, idx);
                d[3] = 255;
            }
            return true;
        }
        for (u32 i = 0; i < t.width * t.height; ++i) {
            const u8* s = samples.data() + size_t(i) * spp;
            u8* d = rgba.data() + size_t(i) * 4;
            if (spp == 1) {
                const u8 g = t.photometric == 0 ? u8(255 - s[0]) : s[0];
                d[0] = d[1] = d[2] = g;
                d[3] = 255;
            } else {
                d[0] = s[0];
                d[1] = s[1];
                d[2] = s[2];
                d[3] = spp >= 4 ? s[3] : 255;
            }
        }
        return true;
    }

    u32 rd16(const u8* p) const {
        return le_ ? u32(p[0]) | (u32(p[1]) << 8) : (u32(p[0]) << 8) | u32(p[1]);
    }
    u32 rd32(const u8* p) const {
        return le_ ? u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24)
                   : (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
    }

    PreviewReader& pr_;
    Budget& b_;
    bool le_ = true;
};

} // namespace

/* ---------------------------------------------------------------- */
/* the public entry: sniff -> dispatch -> dims-first guard -> bounded
 * decode -> downscale, with the unreadable-span truth gate */
/* ---------------------------------------------------------------- */

namespace {

/* ---- native dims-first header parsers (the bomb guard must not depend
 * on the decoder's own size policies - stb's header scan refuses
 * x*y*n ~ 2^30 shapes as "corrupt", so we read the geometry ourselves,
 * exactly like the preview module's own sniffers) ---- */

u32 be32(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}
u32 le32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

bool native_dims(Fmt fmt, PreviewReader& pr, Budget& b, u32& w, u32& h) {
    u8 buf[64];
    switch (fmt) {
    case Fmt::Png: {
        if (bfetch(pr, b, 0, buf, 26) != 26)
            return false;
        if (std::memcmp(buf + 12, "IHDR", 4) != 0)
            return false; /* IHDR must be the first chunk */
        w = be32(buf + 16);
        h = be32(buf + 20);
        return w != 0 && h != 0;
    }
    case Fmt::Jpeg: {
        u64 off = 2;
        u8 m[4];
        for (u32 guard = 0; guard < 512; ++guard) {
            if (bfetch(pr, b, off, m, 2) != 2)
                return false;
            if (m[0] != 0xFF)
                return false;
            u32 marker = m[1];
            while (marker == 0xFF) { /* fill bytes */
                ++off;
                if (bfetch(pr, b, off, m, 2) != 2 || m[0] != 0xFF)
                    return false;
                marker = m[1];
            }
            if (marker == 0xD9 || marker == 0xDA)
                return false; /* EOI / SOS before any SOF */
            if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
                off += 2;
                continue;
            }
            if (bfetch(pr, b, off + 2, m, 2) != 2)
                return false;
            const u32 len = (u32(m[0]) << 8) | m[1];
            if (len < 2)
                return false;
            const bool sof = marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 &&
                             marker != 0xCC;
            if (sof) {
                u8 dims[6];
                if (bfetch(pr, b, off + 4, dims, 6) != 6)
                    return false;
                h = (u32(dims[1]) << 8) | dims[2];
                w = (u32(dims[3]) << 8) | dims[4];
                return w != 0 && h != 0;
            }
            off += 2 + len;
            if (off > (256u << 10))
                return false; /* header walk bounded */
        }
        return false;
    }
    case Fmt::Bmp: {
        if (bfetch(pr, b, 0, buf, 26) != 26)
            return false;
        w = le32(buf + 18);
        const i32 raw_h = i32(le32(buf + 22));
        h = raw_h < 0 ? u32(-raw_h) : u32(raw_h); /* top-down DIBs */
        return w != 0 && h != 0;
    }
    case Fmt::Gif: {
        if (bfetch(pr, b, 0, buf, 10) != 10)
            return false;
        w = u32(buf[6]) | (u32(buf[7]) << 8);
        h = u32(buf[8]) | (u32(buf[9]) << 8);
        return w != 0 && h != 0;
    }
    case Fmt::Psd: {
        if (bfetch(pr, b, 0, buf, 26) != 26)
            return false;
        h = be32(buf + 14);
        w = be32(buf + 18);
        return w != 0 && h != 0;
    }
    case Fmt::Pnm: {
        if (bfetch(pr, b, 0, buf, 64) < 2)
            return false;
        /* header: magic, whitespace/comments, width, height */
        u32 i = 2, fields = 0;
        u64 vals[2] = {0, 0};
        while (i < 64 && fields < 2) {
            if (buf[i] == '#') {
                while (i < 64 && buf[i] != '\n')
                    ++i;
                continue;
            }
            if (buf[i] == ' ' || buf[i] == '\t' || buf[i] == '\r' || buf[i] == '\n') {
                ++i;
                continue;
            }
            if (buf[i] < '0' || buf[i] > '9')
                return false;
            u64 v = 0;
            while (i < 64 && buf[i] >= '0' && buf[i] <= '9') {
                v = v * 10 + u32(buf[i] - '0');
                ++i;
            }
            vals[fields++] = v;
        }
        if (fields != 2)
            return false;
        w = u32(vals[0]);
        h = u32(vals[1]);
        return w != 0 && h != 0;
    }
    case Fmt::Hdr: {
        if (bfetch(pr, b, 0, buf, 64) < 8)
            return false;
        /* the "-Y <h> +X <w>" resolution line */
        for (u32 i = 0; i + 6 < 64; ++i) {
            if (std::memcmp(buf + i, "-Y ", 3) == 0) {
                u32 j = i + 3;
                u64 vh = 0, vw = 0;
                while (j < 64 && buf[j] >= '0' && buf[j] <= '9')
                    vh = vh * 10 + u32(buf[j++] - '0');
                if (j + 3 <= 64 && std::memcmp(buf + j, " +X ", 4) == 0) {
                    j += 4;
                    while (j < 64 && buf[j] >= '0' && buf[j] <= '9')
                        vw = vw * 10 + u32(buf[j++] - '0');
                    w = u32(vw);
                    h = u32(vh);
                    return w != 0 && h != 0;
                }
            }
        }
        return false;
    }
    default:
        return false;
    }
}

} // namespace

namespace {

bool reader_gained_failures(const std::vector<ReadErrorRange>& before, const PreviewReader& pr) {
    const auto& after = pr.unreadable_spans().ranges();
    if (before.size() != after.size())
        return true;
    for (size_t i = 0; i < before.size(); ++i) {
        if (before[i].offset != after[i].offset || before[i].length != after[i].length ||
            before[i].attempts != after[i].attempts || before[i].module != after[i].module)
            return true;
    }
    return false;
}

ThumbStatus finish_ok(Thumbnail& out, std::vector<u8>& rgba, u32 w, u32 h, u32 max_edge,
                      const char* fmt, bool embedded) {
    box_downscale(rgba, w, h, max_edge);
    out.width = w;
    out.height = h;
    out.rgba.swap(rgba);
    out.source_format = fmt;
    out.from_embedded = embedded;
    out.truncated = false;
    return ThumbStatus::Ok;
}

ThumbStatus thumbnail_stb(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out, Fmt fmt,
                          Budget& b, const std::vector<ReadErrorRange>& fails_before) {
    const bool is_jpeg = fmt == Fmt::Jpeg;
    {
        /* dims-first (native header parse - the bomb guard runs BEFORE any
         * decoder allocation and never depends on decoder size policy) */
        u32 w = 0, h = 0;
        if (!native_dims(fmt, pr, b, w, h)) {
            out.truncated = !b.over;
            return b.over ? ThumbStatus::BudgetExceeded : ThumbStatus::DecodeFailed;
        }
        if (u64(w) * u64(h) > caps.max_pixels) {
            if (is_jpeg && caps.allow_embedded) {
                /* the og-parity escape: EXIF embedded thumbnails */
                ExifWalker wlk(pr, b);
                auto thumb = wlk.embedded_thumbnail(256u << 10);
                if (!thumb.empty()) {
                    int tw = 0, th = 0, tc = 0;
                    stbi_uc* px =
                        stbi_load_from_memory(thumb.data(), int(thumb.size()), &tw, &th, &tc, 4);
                    if (px) {
                        std::vector<u8> rgba(px, px + size_t(tw) * th * 4);
                        stbi_image_free(px);
                        if (reader_gained_failures(fails_before, pr))
                            return ThumbStatus::UnreadableSpan;
                        return finish_ok(out, rgba, u32(tw), u32(th), caps.max_edge, "jpeg", true);
                    }
                }
            }
            return ThumbStatus::Oversized;
        }
    }
    StbioSource src(pr, b);
    auto cb = src.callbacks();
    int w = 0, h = 0, comp = 0;
    stbi_uc* px = stbi_load_from_callbacks(&cb, &src, &w, &h, &comp, 4);
    if (!px) {
        out.truncated = src.hit_eof(); /* diagnostic only - no pixels */
        return src.over_budget() ? ThumbStatus::BudgetExceeded : ThumbStatus::DecodeFailed;
    }
    std::vector<u8> rgba(px, px + size_t(w) * h * 4);
    stbi_image_free(px);
    if (reader_gained_failures(fails_before, pr)) {
        out.truncated = false;
        return ThumbStatus::UnreadableSpan;
    }
    const char* name = fmt == Fmt::Jpeg  ? "jpeg"
                       : fmt == Fmt::Png ? "png"
                       : fmt == Fmt::Bmp ? "bmp"
                       : fmt == Fmt::Gif ? "gif"
                       : fmt == Fmt::Psd ? "psd"
                       : fmt == Fmt::Pnm ? "pnm"
                                         : "hdr";
    const ThumbStatus st = finish_ok(out, rgba, u32(w), u32(h), caps.max_edge, name, false);
    /* the decoder tolerated a short stream (stb's JPEG zero-fill leniency):
     * disclose it on the Ok result too - never a silent partial image */
    if (st == ThumbStatus::Ok)
        out.truncated = src.hit_eof();
    return st;
}

ThumbStatus thumbnail_tiff(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out, Budget& b,
                           const std::vector<ReadErrorRange>& fails_before) {
    TiffDecoder d(pr, b);
    TiffTags t;
    if (!d.info(t)) {
        out.truncated = !b.over;
        return b.over ? ThumbStatus::BudgetExceeded : ThumbStatus::DecodeFailed;
    }
    if (u64(t.width) * t.height > caps.max_pixels)
        return ThumbStatus::Oversized;
    std::vector<u8> rgba;
    if (!d.decode(t, rgba)) {
        out.truncated = !b.over;
        return b.over ? ThumbStatus::BudgetExceeded : ThumbStatus::DecodeFailed;
    }
    if (reader_gained_failures(fails_before, pr)) {
        out.truncated = false;
        return ThumbStatus::UnreadableSpan;
    }
    return finish_ok(out, rgba, t.width, t.height, caps.max_edge, "tiff", false);
}

} // namespace

ThumbStatus make_thumbnail(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out) {
    if (caps.max_edge == 0 || caps.max_edge > 8192 || caps.max_pixels == 0 || caps.max_read == 0 ||
        caps.max_work == 0)
        return ThumbStatus::BadParameter;
    if (pr.size() == 0)
        return ThumbStatus::Unsupported;

    const auto fails_before = pr.unreadable_spans().ranges();
    Budget b{caps.max_read, caps.max_work};

    u8 hdr[16];
    const u32 n = bfetch(pr, b, 0, hdr, pr.size() < 16 ? u32(pr.size()) : 16);
    const Sniff sn = sniff(hdr, n);

    switch (sn.fmt) {
    case Fmt::Webp:
    case Fmt::Heic:
    case Fmt::Unknown:
        out.source_format = sn.name; /* named, documented phase-2 refusals */
        return ThumbStatus::Unsupported;
    case Fmt::Tiff:
        return thumbnail_tiff(pr, caps, out, b, fails_before);
    case Fmt::Jpeg:
    case Fmt::Png:
    case Fmt::Bmp:
    case Fmt::Gif:
    case Fmt::Psd:
    case Fmt::Pnm:
    case Fmt::Hdr:
        return thumbnail_stb(pr, caps, out, sn.fmt, b, fails_before);
    }
    return ThumbStatus::Unsupported;
}

} // namespace preview
} // namespace cr
