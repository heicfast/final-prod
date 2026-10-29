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
 *     Deflate / LZW strips, 8/16-bit, gray / palette / RGB / RGBA, both
 *     endians — the matrix kept in step with the header's (N-135 item 7:
 *     the .cpp's copy had missed M-085's LZW)),
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
    return f == "jpeg" || f == "png" || f == "bmp" || f == "gif" || f == "tiff" ||
           f == "raw" /* S-045: the TIFF-RAW half */ || f == "psd" || f == "pnm" || f == "hdr" ||
           f == "txt" /* N-144: the text-snapshot family (the og's TxtViewFiles
                       * thumbnails - eligibility itself is the caller's
                       * textexts::eligible gate, exactly the og's
                       * extension-membership dispatch) */
        ;
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
        /* N-112 (discovered during M-109's band test): LOOP the read -
         * PreviewReader::read_at clamps every call to kMaxRead (4 MiB),
         * so a single-shot read satisfied only the first 4 MiB of stb's
         * IDAT request and every PNG with an IDAT larger than that (any
         * real-world photo-scale PNG) failed with outofdata. The loop
         * assembles the full request (each iteration budget-spent) until
         * true EOF. */
        u32 got = 0;
        while (got < want) {
            const u32 n = s->pr_.read_at(s->pos_ + got, data + got, want - got);
            if (n == 0)
                break; /* true EOF (or unreadable - the span map carries it) */
            s->b_.spend_read(n);
            got += n;
        }
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

enum class Fmt { Jpeg, Png, Bmp, Gif, Tiff, Psd, Pnm, Hdr, Webp, Heic, Wmf, Emf, Ico, Unknown };

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
    /* the metafile/icon family: GDI+ rasterizes all three natively (the og's
     * own thumbnail path); the core's phase-1 matrix does not - they are
     * NAMED refusals, never "unknown" (N-092). Placeable WMF: the 0xD7CDC69A
     * key; standard WMF: u16 type 1/2 + u16 header size 9. */
    if (n >= 4 && p[0] == 0x9A && p[1] == 0xC6 && p[2] == 0xCD && p[3] == 0xD7)
        return {Fmt::Wmf, "wmf"};
    if (n >= 4 && (p[0] == 0x01 || p[0] == 0x02) && p[1] == 0x00 && p[2] == 0x09 && p[3] == 0x00)
        return {Fmt::Wmf, "wmf"};
    /* EMF: the EMR_HEADER signature dword " EMF" sits at +40 */
    if (n >= 44 && std::memcmp(p + 40, " EMF", 4) == 0)
        return {Fmt::Emf, "emf"};
    /* ICO/CUR: reserved u16 0 + type u16 1 (icon) / 2 (cursor) */
    if (n >= 4 && p[0] == 0x00 && p[1] == 0x00 && (p[2] == 0x01 || p[2] == 0x02) && p[3] == 0x00)
        return {Fmt::Ico, "ico"};
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

    /* S-043: the EXIF Orientation tag (0x112) from IFD0 - the og reads
     * GDI+ property 0x112 and applies the matching RotateFlip BEFORE the
     * 256-px pass (FUN_1402fcc30 :521694-725; call order :652307-309).
     * Returns 1..8, or 0 when absent/invalid (no transform). */
    u32 orientation() {
        u64 tiff_abs = 0, seg_payload = 0;
        if (!find_exif_tiff(tiff_abs, seg_payload))
            return 0;
        tiff_ = tiff_abs;
        u8 hdr[8];
        if (!bfetch(pr_, b_, tiff_, hdr, 8))
            return 0;
        if (hdr[0] == 'I' && hdr[1] == 'I')
            le_ = true;
        else if (hdr[0] == 'M' && hdr[1] == 'M')
            le_ = false;
        else
            return 0;
        if (rd16(hdr + 2) != 42)
            return 0;
        const u32 ifd0 = rd32(hdr + 4);
        u8 c2[2];
        if (!fetch(ifd0, c2, 2))
            return 0;
        const u32 n0 = rd16(c2);
        if (n0 == 0 || n0 > 256)
            return 0;
        std::vector<u8> ents(size_t(n0) * 12);
        if (!fetch(ifd0 + 2, ents.data(), u32(ents.size())))
            return 0;
        for (u32 i = 0; i < n0; ++i) {
            const u8* e = ents.data() + size_t(i) * 12;
            if (rd16(e) == 0x0112 && rd16(e + 2) == 3 && rd32(e + 4) == 1) {
                const u32 v = rd16(e + 8);
                if (v >= 1 && v <= 8)
                    return v;
                return 0; /* an invalid orientation byte: no transform */
            }
        }
        return 0;
    }

    /* Returns the embedded JPEG thumbnail bytes, empty when absent.
     * S-043 rider: ifd1_orientation() is valid after a non-empty return -
     * IFD1 may declare its own 0x112 (the marker of a NOT-already-oriented
     * embedded thumb; the CIPA DC-008 convention stores thumbnails
     * upright, so the tag is normally absent and the bytes serve as
     * stored - the closing-fix lane's D-105 rule, made precise). */
    std::vector<u8> embedded_thumbnail(u64 limit) {
        u64 tiff_abs = 0, seg_payload = 0;
        if (!find_exif_tiff(tiff_abs, seg_payload, limit))
            return {};
        auto thumb = parse_tiff_thumbnail(tiff_abs, seg_payload);
        return thumb;
    }

    /* S-043 rider: the orientation IFD1 itself declared (0 = none) - see
     * embedded_thumbnail(). */
    u32 ifd1_orientation() const { return ifd1_orientation_; }

    /* The APP1/Exif segment's TIFF header (absolute) + payload length.
     * Shared by the embedded-thumbnail walk and the orientation read. */
    bool find_exif_tiff(u64& tiff_abs, u64& seg_payload, u64 limit = 256u << 10) {
        const u64 size = pr_.size();
        u64 off = 2; /* past SOI */
        for (u32 guard = 0; guard < 512 && off + 4 <= size && off < limit; ++guard) {
            u8 m4[4];
            if (bfetch(pr_, b_, off, m4, 2) != 2)
                return false;
            if (m4[0] != 0xFF)
                return false;
            u32 marker = m4[1];
            while (marker == 0xFF) { /* fill bytes */
                ++off;
                if (bfetch(pr_, b_, off, m4, 2) != 2 || m4[0] != 0xFF)
                    return false;
                marker = m4[1];
            }
            if (marker == 0xD9 || marker == 0xDA) /* EOI / SOS: metadata ends */
                return false;
            if (marker == 0x01 || (marker >= 0xD0 && marker <= 0xD7)) {
                off += 2; /* standalone markers */
                continue;
            }
            if (bfetch(pr_, b_, off + 2, m4, 2) != 2)
                return false;
            const u32 seg_len = (u32(m4[0]) << 8) | m4[1];
            if (seg_len < 2)
                return false;
            if (marker == 0xE1) {
                u8 head[6];
                if (bfetch(pr_, b_, off + 4, head, 6) == 6 &&
                    std::memcmp(head, "Exif\0\0", 6) == 0) {
                    tiff_abs = off + 10;
                    seg_payload = u64(seg_len) - 8;
                    return true;
                }
            }
            off += 2 + seg_len;
        }
        return false;
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
        ifd1_orientation_ = 0;
        for (u32 i = 0; i < n1; ++i) {
            const u8* e = ents.data() + size_t(i) * 12;
            const u32 tag = rd16(e);
            const u32 type = rd16(e + 2);
            const u32 count = rd32(e + 4);
            if (tag == 0x0201 && type == 4 && count == 1)
                thumb_off = rd32(e + 8);
            else if (tag == 0x0202 && type == 4 && count == 1)
                thumb_len = rd32(e + 8);
            else if (tag == 0x0112 && type == 3 && count == 1) {
                const u32 v = rd16(e + 8);
                ifd1_orientation_ = (v >= 1 && v <= 8) ? v : 0;
            }
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
    u32 ifd1_orientation_ = 0;
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
    u32 orientation = 1; /* tag 274 - S-043 rider: GDI+ surfaces this as
                             property 0x112 for TIFF inputs */
    u64 rows_per_strip = 0xFFFFFFFF;
    std::vector<u64> strip_offsets, strip_counts;
    u64 colormap_off = 0;
    u32 colormap_count = 0;
    /* S-045 (the TIFF-RAW half): the JPEGInterchangeFormat pair (513/514)
     * - the full-size JPEG preview every TIFF-based camera RAW (CR2/DNG/
     * NEF/ARW...) carries - plus the SubIFD chain (330) NEF/ARW park it
     * in. Zero when absent: plain baseline TIFFs never carry 513/514. */
    u64 jpeg_if_off = 0, jpeg_if_len = 0;
    std::vector<u64> subifd_offsets;
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
        case 274: /* Orientation - S-043 rider: GDI+ surfaces this as
                   * property 0x112 for TIFF inputs; the value space is
                   * 1..8, junk keeps the identity (the default) */
            if (count == 1) {
                const u32 v = u32(val(0));
                if (v >= 1 && v <= 8)
                    t.orientation = v;
            }
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
        case 330: /* SubIFD - NEF/ARW park the JPEG preview one level down */
            for (u32 i = 0; i < count && i < 8; ++i)
                t.subifd_offsets.push_back(val(i));
            break;
        case 513:
            t.jpeg_if_off = val(0);
            break;
        case 514:
            t.jpeg_if_len = val(0);
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
        if (t.compression == 5) { /* LZW (M-085: the classic TIFF LZW -
                                   * MSB-first packing, 9..12-bit codes, Clear=256/EOI=257, the
                                   * early-change width step at 510/1022/2046 - the scanner and
                                   * Photoshop "LZW" TIFF export GDI+ decodes) */
            if (t.bps == 0 || t.bps > 8)
                return false; /* 16-bit LZW is nonstandard: honest refusal */
            const u32 n_literals = 1u << t.bps;
            std::vector<u16> prefix(4096, 0xFFFF);
            std::vector<u8> suffix(4096, 0);
            u32 next_free = n_literals + 2;
            u32 width = 9;
            u32 maxcode = (1u << width) - 2; /* early change: one code early */
            u32 bitpos = 0;
            u16 prev = 0xFFFF;
            out.clear();
            out.reserve(want_out);
            auto next_code = [&]() -> u32 {
                if (u64(bitpos) + width > u64(in.size()) * 8)
                    return 0xFFFFFFFF;
                u32 code = 0;
                for (u32 k = 0; k < width; ++k) {
                    const u64 p = bitpos + k;
                    code = (code << 1) | u32((in[size_t(p >> 3)] >> (7 - (p & 7))) & 1);
                }
                bitpos += width;
                return code;
            };
            auto emit_chain = [&](u16 code) -> bool { /* code -> byte run */
                                                      u8 tmp[4096];
                                                      u32 n = 0;
                                                      u16 c = code;
                                                      while (c >= n_literals + 2) {
                                                          if (n >= 4096)
                                                              return false; /* hostile cycle */
                                                          tmp[n++] = suffix[c];
                                                          c = prefix[c];
                                                          if (c == 0xFFFF)
                                                              return false;
                                                      }
                                                      if (c >= n_literals)
                                                          return false;
                                                      tmp[n++] = u8(c);
                                                      if (out.size() + n > want_out)
                                                          return false;
                                                      for (u32 i = 0; i < n; ++i)
                                                          out.push_back(tmp[n - 1 - i]);
                                                      return true;
            };
            auto first_of = [&](u16 code) -> u8 {
                u16 c = code;
                while (c >= n_literals + 2)
                    c = prefix[c];
                return u8(c);
            };
            while (out.size() < want_out) {
                const u32 code = next_code();
                if (code == 0xFFFFFFFF)
                    return false; /* short stream */
                if (code == 257)
                    break;         /* EOI */
                if (code == 256) { /* Clear: reset the table */
                    next_free = n_literals + 2;
                    width = 9;
                    maxcode = (1u << width) - 2;
                    prev = 0xFFFF;
                    continue;
                }
                if (code > next_free)
                    return false; /* not yet defined: hostile */
                /* the first code after a Clear consumes no table slot (its
                 * chain has no predecessor to extend) - the slot count only
                 * advances when an entry is actually added */
                auto bump = [&]() {
                    prev = u16(code);
                    ++next_free;
                    if (next_free > maxcode && width < 12) {
                        ++width;
                        maxcode = (width == 12) ? 4096 : (1u << width) - 2;
                    }
                };
                if (code == next_free) { /* KwKwK: prev + first(prev) */
                    if (prev == 0xFFFF || next_free >= 4096)
                        return false;
                    prefix[next_free] = prev;
                    suffix[next_free] = first_of(prev);
                    if (!emit_chain(u16(next_free)))
                        return false;
                    bump();
                } else {
                    if (!emit_chain(u16(code)))
                        return false;
                    if (prev == 0xFFFF) {
                        prev = u16(code); /* first after Clear: no slot */
                        continue;
                    }
                    if (next_free >= 4096) {
                        prev = u16(code); /* table full: no more adds */
                        continue;
                    }
                    prefix[next_free] = prev;
                    suffix[next_free] = first_of(u16(code));
                    bump();
                }
            }
            return out.size() == want_out;
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

/* S-043: apply the EXIF orientation to decoded RGBA before the downscale -
 * the og's 8-case RotateFlip map (FUN_1402fcc30 :521694-725: 2->4 flip-X,
 * 3->2 rotate-180, 4->6 flip-Y, 5->5 transpose, 6->1 rotate-90-CW,
 * 7->7 transverse, 8->3 rotate-270-CW), applied BEFORE the 256-px pass
 * (call order :652307-309). Straight RGBA8, row-major, w*4 stride. */
void apply_exif_orientation(std::vector<u8>& rgba, u32& w, u32& h, u32 orient) {
    if (orient < 2 || orient > 8 || w == 0 || h == 0)
        return; /* 1 = identity; 0/invalid = no transform (the og's switch
                 * has no default arm - unknown bytes never rotate) */
    std::vector<u8> out(rgba.size());
    const u32 W = w, H = h;
    const bool swap = orient >= 5; /* 5..8 transpose the axes */
    for (u32 y = 0; y < H; ++y) {
        for (u32 x = 0; x < W; ++x) {
            u32 nx = 0, ny = 0;
            switch (orient) {
            case 2:
                nx = W - 1 - x;
                ny = y;
                break; /* flip X */
            case 3:
                nx = W - 1 - x;
                ny = H - 1 - y;
                break; /* rotate 180 */
            case 4:
                nx = x;
                ny = H - 1 - y;
                break; /* flip Y */
            case 5:
                nx = y;
                ny = x;
                break; /* transpose */
            case 6:
                nx = H - 1 - y;
                ny = x;
                break; /* rotate 90 CW */
            case 7:
                nx = H - 1 - y;
                ny = W - 1 - x;
                break; /* transverse */
            case 8:
                nx = y;
                ny = W - 1 - x;
                break; /* rotate 270 CW */
            }
            std::memcpy(out.data() + (size_t(ny) * (swap ? H : W) + nx) * 4,
                        rgba.data() + (size_t(y) * W + x) * 4, 4);
        }
    }
    if (swap)
        std::swap(w, h);
    rgba.swap(out);
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
    out.read_ok = true; /* M-115: every finish_ok caller passes clean reads */
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
            if (b.over)
                return ThumbStatus::BudgetExceeded;
            /* M-115: the header fetched clean but no decoder accepted it -
             * the read verdict is still clean (the og's local_160==0 leg) */
            out.read_ok = !reader_gained_failures(fails_before, pr);
            return ThumbStatus::DecodeFailed;
        }
        /* M-080/M-109: the byte-oversized middle class - a byte-heavy
         * but pixel-benign photo (the common case: a 6 MiB 12-MP JPEG) -
         * also takes the embedded fast path: it reads only the embedded
         * thumb (a few KiB), never the max_read body. The input bound is
         * the og's still-image eligibility gate - 200 MB (0xC800000,
         * DRWUI FUN_1402636e0 :422425-429, M-109's re-pin of M-080's
         * refuted no-cap doctrine); the pixel guard (64 MP) bounds decode
         * memory, so the raised I/O bound cannot OOM the decode path. */
        const bool pixel_oversized = u64(w) * u64(h) > caps.max_pixels;
        const bool byte_oversized = pr.size() > caps.max_read;
        if (pixel_oversized || byte_oversized) {
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
                        /* S-043 rider (D-105): IFD1 may declare its own
                         * 0x112 (a NOT-already-oriented embedded thumb -
                         * the rare non-conformant writer); the CIPA DC-008
                         * norm stores thumbnails upright and carries no
                         * tag, in which case the bytes serve as stored
                         * (no double rotation). */
                        u32 ew = u32(tw), eh = u32(th);
                        if (wlk.ifd1_orientation() >= 2)
                            apply_exif_orientation(rgba, ew, eh, wlk.ifd1_orientation());
                        return finish_ok(out, rgba, ew, eh, caps.max_edge, "jpeg", true);
                    }
                }
            }
            if (byte_oversized)
                return ThumbStatus::BudgetExceeded; /* named, honest refusal */
            return ThumbStatus::Oversized;
        }
    }
    /* S-043: read the orientation BEFORE the decode - the full decode
     * consumes the read budget the walker needs (the og's GDI+ path reads
     * property 0x112 before scaling, :652307-309). */
    u32 orient = 0;
    if (is_jpeg) {
        ExifWalker wlk(pr, b);
        orient = wlk.orientation();
    }
    StbioSource src(pr, b);
    auto cb = src.callbacks();
    int w = 0, h = 0, comp = 0;
    stbi_uc* px = stbi_load_from_callbacks(&cb, &src, &w, &h, &comp, 4);
    if (!px) {
        out.truncated = src.hit_eof(); /* diagnostic only - no pixels */
        if (src.over_budget())
            return ThumbStatus::BudgetExceeded;
        /* M-115: the payload fetched clean, the DECODER refused the stream -
         * the damaged-but-readable population the og promotes on read
         * evidence (readVerdict==0 && payload; the decode's failure never
         * retracts FUN_140089630(node, 2)) */
        out.read_ok = !reader_gained_failures(fails_before, pr);
        return ThumbStatus::DecodeFailed;
    }
    std::vector<u8> rgba(px, px + size_t(w) * h * 4);
    stbi_image_free(px);
    if (reader_gained_failures(fails_before, pr)) {
        out.truncated = false;
        return ThumbStatus::UnreadableSpan;
    }
    /* S-043: apply the RotateFlip BEFORE the 256-px pass - the og's call
     * order (:652307 orientation -> :652309 the 256-px thumbnail). The
     * embedded fast path (the budget escape above) is excluded: the
     * camera-pre-rotated embedded thumbnail is already display-oriented
     * (the libheif embedded-first precedent the fast path follows), so
     * both engines produce the correctly-oriented thumbnail. */
    if (orient != 0) {
        u32 tw = u32(w), th = u32(h);
        apply_exif_orientation(rgba, tw, th, orient);
        w = int(tw);
        h = int(th);
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
     * disclose it on the Ok result too - never a silent partial image.
     * JPEG is the ONE lenient family: the strict decoders (png/bmp/gif/
     * psd/pnm/hdr) refuse short streams outright, so their Ok proves the
     * data complete - their post-parse EOF probes (reading past IEND to
     * detect stream end, the stb API shape) are parse noise, never a data
     * truncation. Reporting those as truncated would poison the
     * disclosure signal (a complete PNG claiming truncation - the defect
     * the M-102 ABI test's fail-before caught). */
    if (st == ThumbStatus::Ok)
        out.truncated = is_jpeg && src.hit_eof();
    return st;
}

/* S-045 (the TIFF-RAW half): the SOF dims of an in-memory JPEG - the
 * dims-first bomb guard runs BEFORE any stb allocation over RAW preview
 * bytes (the engine doctrine: no decoder-size-policy dependence). */
bool jpeg_dims_mem(const std::vector<u8>& j, u32& w, u32& h) {
    if (j.size() < 4 || j[0] != 0xFF || j[1] != 0xD8)
        return false;
    size_t off = 2;
    while (off + 4 <= j.size()) {
        if (j[off] != 0xFF)
            return false;
        const u8 m = j[off + 1];
        if (m == 0xD9 || m == 0xDA)
            return false;
        if (m == 0x01 || (m >= 0xD0 && m <= 0xD7)) {
            off += 2;
            continue;
        }
        if (off + 4 > j.size())
            return false;
        const u32 len = (u32(j[off + 2]) << 8) | j[off + 3];
        if (len < 2)
            return false;
        if (m == 0xC0 || m == 0xC1 || m == 0xC2 || m == 0xC3) { /* SOF0/1/2/3 */
            if (off + 10 > j.size() || len < 8)
                return false;
            h = (u32(j[off + 5]) << 8) | j[off + 6];
            w = (u32(j[off + 7]) << 8) | j[off + 8];
            return w != 0 && h != 0;
        }
        off += 2 + len;
    }
    return false;
}

ThumbStatus thumbnail_tiff(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out, Budget& b,
                           const std::vector<ReadErrorRange>& fails_before) {
    TiffDecoder d(pr, b);
    TiffTags t;
    if (!d.info(t)) {
        out.truncated = !b.over;
        if (b.over)
            return ThumbStatus::BudgetExceeded;
        /* M-115: clean read, refused IFD - the read verdict stays clean */
        out.read_ok = !reader_gained_failures(fails_before, pr);
        return ThumbStatus::DecodeFailed;
    }
    /* S-045 (the TIFF-RAW half): a TIFF-RAW shape - the JPEGInterchange
     * Format pair (513/514) outside JPEG-compressed TIFFs (compression 6/7
     * stay the named JPEG-in-TIFF refusal) - is the camera-RAW family
     * (CR2/DNG/NEF/ARW...): the og routes RAW through its EURawImg/LibRaw
     * pipeline whose handoff is the preview JPEG; the core's equivalent
     * extracts the full-size JPEG preview and decodes it through the same
     * JPEG path (dims-first, budget-bounded, truth-gated). */
    if (t.jpeg_if_off != 0 && t.jpeg_if_len != 0 && t.jpeg_if_len < (256u << 20) &&
        t.compression != 6 && t.compression != 7) {
        std::vector<u8> jpeg(size_t(t.jpeg_if_len));
        if (bfetch(pr, b, t.jpeg_if_off, jpeg.data(), u32(t.jpeg_if_len)) == t.jpeg_if_len &&
            jpeg.size() > 3 && jpeg[0] == 0xFF && jpeg[1] == 0xD8) {
            u32 jw = 0, jh = 0;
            if (jpeg_dims_mem(jpeg, jw, jh) && u64(jw) * u64(jh) <= caps.max_pixels) {
                int dw = 0, dh = 0, dc = 0;
                stbi_uc* px =
                    stbi_load_from_memory(jpeg.data(), int(jpeg.size()), &dw, &dh, &dc, 4);
                if (px) {
                    std::vector<u8> rgba(px, px + size_t(dw) * dh * 4);
                    stbi_image_free(px);
                    if (reader_gained_failures(fails_before, pr)) {
                        out.truncated = false;
                        return ThumbStatus::UnreadableSpan;
                    }
                    return finish_ok(out, rgba, u32(dw), u32(dh), caps.max_edge, "raw", false);
                }
            }
        }
        /* the RAW shape without a decodable preview: fall through to the
         * baseline attempt (a layered second decoder, never a masked
         * failure - the final status reflects the real outcome) */
    }
    if (u64(t.width) * t.height > caps.max_pixels)
        return ThumbStatus::Oversized;
    std::vector<u8> rgba;
    if (!d.decode(t, rgba)) {
        out.truncated = !b.over;
        if (b.over)
            return ThumbStatus::BudgetExceeded;
        /* M-115: clean read, refused strips - the og's promote-on-read leg */
        out.read_ok = !reader_gained_failures(fails_before, pr);
        return ThumbStatus::DecodeFailed;
    }
    if (reader_gained_failures(fails_before, pr)) {
        out.truncated = false;
        return ThumbStatus::UnreadableSpan;
    }
    /* S-043 rider: TIFF tag 274 is the same orientation datum GDI+
     * surfaces as property 0x112 for TIFF inputs - the og RotateFlips
     * those too */
    u32 tw = t.width, th = t.height;
    if (t.orientation >= 2)
        apply_exif_orientation(rgba, tw, th, t.orientation);
    return finish_ok(out, rgba, tw, th, caps.max_edge, "tiff", false);
}

} // namespace

/* M-115 (defined below make_thumbnail): the read-verdict disclosure. */
ThumbStatus make_thumbnail_status(PreviewReader& pr, Thumbnail& out,
                                  const std::vector<ReadErrorRange>& fails_before, ThumbStatus st);

ThumbStatus make_thumbnail(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out) {
    if (caps.max_edge == 0 || caps.max_edge > 8192 || caps.max_pixels == 0 || caps.max_read == 0 ||
        caps.max_work == 0)
        return ThumbStatus::BadParameter;
    if (pr.size() == 0)
        return ThumbStatus::Unsupported;

    const auto fails_before = pr.unreadable_spans().ranges();
    Budget b{caps.max_read, caps.max_work};

    u8 hdr[48]; /* 48: the EMF signature lives at +40 (16 was enough for every
                 * other family; the metafile half of N-092 needs the tail) */
    const u32 n = bfetch(pr, b, 0, hdr, pr.size() < 48 ? u32(pr.size()) : 48);
    const Sniff sn = sniff(hdr, n);

    switch (sn.fmt) {
    case Fmt::Webp:
    case Fmt::Heic:
    case Fmt::Wmf:
    case Fmt::Emf:
    case Fmt::Ico:
    case Fmt::Unknown:
        out.source_format = sn.name; /* named, documented phase-2 refusals */
        return ThumbStatus::Unsupported;
    case Fmt::Tiff:
        return make_thumbnail_status(pr, out, fails_before,
                                     thumbnail_tiff(pr, caps, out, b, fails_before));
    case Fmt::Jpeg:
    case Fmt::Png:
    case Fmt::Bmp:
    case Fmt::Gif:
    case Fmt::Psd:
    case Fmt::Pnm:
    case Fmt::Hdr:
        return make_thumbnail_status(pr, out, fails_before,
                                     thumbnail_stb(pr, caps, out, sn.fmt, b, fails_before));
    }
    return ThumbStatus::Unsupported;
}

/* M-115: disclose the READ verdict separately from the decode verdict.
 * The og's promotion guard (DRWUI :422507-514) is fed by the recovery-
 * read verdict (FUN_1403eba50, :422442) and carries no decode input - a
 * payload fetched cleanly but refused by the decoder still promotes.
 * read_ok = this generation gained NO unreadable span (the og's
 * verdict-0 'clean read' equivalent; a read over an unfetchable span is
 * the og's -3 data-but-failed class, which does NOT promote). Only the
 * statuses whose paths actually fetched payload bytes can carry it. */
ThumbStatus make_thumbnail_status(PreviewReader& pr, Thumbnail& out,
                                  const std::vector<ReadErrorRange>& fails_before, ThumbStatus st) {
    if (st == ThumbStatus::Ok || st == ThumbStatus::DecodeFailed)
        out.read_ok = !reader_gained_failures(fails_before, pr);
    return st;
}

/* ---------------------------------------------------------------- */
/* N-144: the text-snapshot thumbnail (the og's TxtViewFiles render) */
/* ---------------------------------------------------------------- */
ThumbStatus make_text_snapshot(PreviewReader& pr, const ThumbnailCaps& caps, Thumbnail& out) {
    if (caps.max_edge == 0 || caps.max_edge > 8192 || caps.max_read == 0 || caps.max_work == 0)
        return ThumbStatus::BadParameter;
    if (pr.size() == 0)
        return ThumbStatus::Unsupported;

    const auto fails_before = pr.unreadable_spans().ranges();
    Budget b{caps.max_read, caps.max_work};

    /* the og's cap: the first min(size, 100) bytes, fetched verbatim
     * (FUN_1403d7ce0's plain copy into the zeroed 110-byte buffer) */
    u8 head[100];
    const u32 want = pr.size() < 100 ? u32(pr.size()) : 100;
    const u32 n = bfetch(pr, b, 0, head, want);
    if (n == 0)
        return ThumbStatus::Unsupported;

    /* the og's UTF-16 gate (FUN_1403d8840: len > 4 && the first two
     * bytes are FF FE or FE FF -> FUN_1403d7e20's wide->narrow pass);
     * every other head renders as its raw bytes (the ANSI string path) */
    std::vector<unsigned> text;
    if (n > 4 && ((head[0] == 0xFF && head[1] == 0xFE) || (head[0] == 0xFE && head[1] == 0xFF))) {
        const bool be = head[0] == 0xFE;
        for (u32 i = 2; i + 1 < n; i += 2) {
            const unsigned cp = be ? (unsigned(head[i]) << 8) | head[i + 1]
                                   : (unsigned(head[i + 1]) << 8) | head[i];
            /* surrogate pairs: the og's conversion maps a lone/paired
             * surrogate through the ANSI lossy path; the honest bounded
             * projection takes the pair when well-formed, '?' otherwise */
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 3 < n) {
                const unsigned lo = be ? (unsigned(head[i + 2]) << 8) | head[i + 3]
                                       : (unsigned(head[i + 3]) << 8) | head[i + 2];
                if (lo >= 0xDC00 && lo <= 0xDFFF) {
                    text.push_back(0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00));
                    i += 2;
                    continue;
                }
            }
            text.push_back(cp >= 0x80 && cp <= 0x10FFFF ? '?' : cp);
        }
    } else {
        for (u32 i = 0; i < n; ++i)
            text.push_back(head[i]);
    }

    /* the og's EMPTY check (FUN_1400a6200 on the draw string: empty ->
     * no thumbnail, the +0x28 promotion never fires) - the decoded text
     * with every code point stripped of renderables (all controls / an
     * all-BOM head) reads as empty exactly like the og's empty string */
    bool any_renderable = false;
    for (unsigned cp : text)
        if (cp >= 0x20 && cp != 0x7F) {
            any_renderable = true;
            break;
        }
    if (!any_renderable) {
        out.source_format = "txt";
        return make_thumbnail_status(pr, out, fails_before, ThumbStatus::Unsupported);
    }

    /* the render: white canvas, black glyph cells, the embedded 6x12
     * font at the thumbnail edge (the og draws Arial 13 on its fixed
     * 200x200; the core's max_edge default is the configured-era shape
     * the finding names as not-a-contract) */
    const unsigned cw = thumbnail_font_cell_w();
    const unsigned ch = thumbnail_font_cell_h();
    const unsigned margin = 2;
    const unsigned cols = caps.max_edge >= 2 * margin + cw ? (caps.max_edge - 2 * margin) / cw : 1;
    const unsigned rows = caps.max_edge >= 2 * margin + ch ? (caps.max_edge - 2 * margin) / ch : 1;
    out.width = caps.max_edge;
    out.height = caps.max_edge;
    out.rgba.assign(size_t(out.width) * size_t(out.height) * 4, 0);
    for (size_t i = 0; i + 3 < out.rgba.size(); i += 4) {
        out.rgba[i] = 0xFF;
        out.rgba[i + 1] = 0xFF;
        out.rgba[i + 2] = 0xFF;
        out.rgba[i + 3] = 0xFF; /* opaque white */
    }
    unsigned cx = 0, cy = 0;
    for (unsigned cp : text) {
        if (cp == 0x0A || cp == 0x0D) { /* LF/CR: line break (DrawText) */
            cx = 0;
            ++cy;
            continue;
        }
        if (cp == 0x09) { /* TAB: next 8-column stop */
            cx = (cx / 8 + 1) * 8;
            if (cx >= cols) {
                cx = 0;
                ++cy;
            }
            continue;
        }
        if (cx >= cols) { /* wrap at the canvas edge (DrawText's DT_WORDBREAK class) */
            cx = 0;
            ++cy;
        }
        if (cy >= rows)
            break; /* the canvas is full - the 100-byte budget bounds the loss */
        if (cp < 0x20 || cp == 0x7F) {
            /* the C0 controls + DEL: one blank advance, no glyph (GDI's
             * control-glyph class renders nothing visible) */
            ++cx;
            continue;
        }
        if (cp > 0x7E)
            cp = '?'; /* non-ASCII: the ANSI/CP_ACP lossy class */
        const unsigned char* glyph = thumbnail_font_glyph(cp);
        if (glyph != nullptr) {
            const size_t px = size_t(margin) + size_t(cx) * cw;
            const size_t py = size_t(margin) + size_t(cy) * ch;
            for (unsigned gy = 0; gy < ch && py + gy < out.height; ++gy) {
                const unsigned char row = glyph[gy];
                for (unsigned gx = 0; gx < cw && px + gx < out.width; ++gx)
                    if (row & (1u << (cw - 1 - gx))) {
                        const size_t o = (py + gy) * size_t(out.width) * 4 + (px + gx) * 4;
                        out.rgba[o] = 0;
                        out.rgba[o + 1] = 0;
                        out.rgba[o + 2] = 0;
                    }
            }
        }
        ++cx;
    }
    out.source_format = "txt";
    out.truncated = n < want; /* the head fetch came up short (honesty) */
    return make_thumbnail_status(pr, out, fails_before, ThumbStatus::Ok);
}

} // namespace preview
} // namespace cr
