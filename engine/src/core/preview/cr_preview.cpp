/* cr_preview.cpp - bounded preview reader + metadata extractors (G-10).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/preview/cr_preview.h"
#include "cr/compress/cr_decomp.h"
#include <cstring>
#include <zlib.h>

namespace cr {
namespace preview {

/* ---- the reader ---- */

PreviewReader::PreviewReader(IStorageDevice* source, const tree::FileProperties* props)
    : source_(source), props_(props), size_(props ? props->logical_size : 0) {}

u32 PreviewReader::read_at(u64 logical_offset, void* buf, u32 len) {
    if (buf == nullptr || props_ == nullptr || source_ == nullptr)
        return 0;
    if (logical_offset >= size_)
        return 0; /* EOF */
    if (len > kMaxRead)
        len = kMaxRead;
    if (u64(len) > size_ - logical_offset)
        len = u32(size_ - logical_offset); /* short read at logical EOF */
    u8* dst = static_cast<u8*>(buf);
    std::memset(dst, 0, len); /* gaps + failures default to zero */

    u64 done = 0;
    /* 1) resident prefix */
    const u64 res = props_->resident_data.size();
    if (logical_offset < res) {
        const u64 take = u64(len) < res - logical_offset ? u64(len) : res - logical_offset;
        std::memcpy(dst, props_->resident_data.data() + logical_offset, size_t(take));
        done = take;
    }

    /* 2) disk extents (absolute offsets; logical stream continues after
     * the resident prefix), then 3) sparse tail stays zero */
    u64 ext_base = res; /* logical start of the extent region */
    for (const auto& e : props_->extents) {
        if (done >= len)
            break;
        const u64 ext_end = ext_base + e.length;
        if (logical_offset + done < ext_end && logical_offset + len > ext_base) {
            const u64 in_ext_off =
                logical_offset + done > ext_base ? logical_offset + done - ext_base : 0;
            u64 want = e.length - in_ext_off;
            if (want > u64(len) - done)
                want = u64(len) - done;
            if (want > 0) {
                if (e.codec != 0) {
                    /* compressed extent: decode the whole block (bounded
                     * by the same 256 MiB guard the writer uses), then
                     * slice the requested window out of it */
                    const u64 psize = e.psize ? e.psize : e.length;
                    if (psize == 0 || e.length == 0 || psize > kMaxDecode ||
                        e.length > kMaxDecode) {
                        unreadable_.add(e.offset, psize, 1); /* malformed caps */
                        done += want;                        /* zero-filled; keep positions aligned
                                                  (same advance as the other failure branches:
                                                  without it the NEXT extent's bytes land at this
                                                  extent's logical offset) */
                    } else {
                        std::vector<u8> packed(size_t(psize), 0);
                        ++device_reads_;
                        device_bytes_ += psize;
                        if (source_->read_at(e.offset, packed.data(), u32(psize)) != OK) {
                            unreadable_.add(e.offset, psize, 1);
                        } else {
                            std::vector<u8> plain(size_t(e.length), 0);
                            u64 take = 0;
                            if (!compress::decompress_block(e.codec, packed.data(), size_t(psize),
                                                            plain.data(), size_t(e.length))) {
                                unreadable_.add(e.offset, psize, 1); /* undecodable */
                            } else if (in_ext_off < plain.size()) {
                                const u64 have = plain.size() - in_ext_off;
                                take = want < have ? want : have;
                                std::memcpy(dst + done, plain.data() + size_t(in_ext_off),
                                            size_t(take));
                            }
                            /* advance by this extent's contribution
                             * regardless: failed/undecodable bytes are
                             * already zero in dst - positions stay aligned */
                            done += take ? take : want;
                        }
                    }
                } else {
                    ++device_reads_;
                    device_bytes_ += want;
                    const u32 got = u32(want);
                    if (source_->read_at(e.offset + in_ext_off, dst + done, got) != OK)
                        unreadable_.add(e.offset + in_ext_off, want, 1); /* zero-filled
                            (memset above); advance keeps alignment */
                    done += want;
                }
            }
        }
        ext_base = ext_end;
    }
    /* bytes beyond the last extent stay zero (sparse/sparse-tail) */
    return len; /* short only via the EOF clamp above */
}

/* M-028: the pre-read cost of read_at(off, len) — mirrors read_at's
 * arithmetic exactly (the same clamps, the same extent-overlap test,
 * the same psize fallback) so a Caps consumer can gate its budget
 * BEFORE any device byte moves. Plain extents cost their overlapped
 * window; compressed extents cost the whole packed block (the decode
 * needs it all); resident/sparse/malformed spans cost nothing. */
u64 PreviewReader::read_cost(u64 logical_offset, u32 len) const {
    if (props_ == nullptr || logical_offset >= size_ || len == 0)
        return 0;
    if (len > kMaxRead)
        len = kMaxRead;
    if (u64(len) > size_ - logical_offset)
        len = u32(size_ - logical_offset);
    u64 cost = 0;
    const u64 res = props_->resident_data.size();
    /* only the part of the request BEYOND the resident prefix can touch
     * extents (read_at serves the prefix from memory) */
    const u64 lo = logical_offset > res ? logical_offset : res;
    const u64 hi = logical_offset + len; /* request end, EOF-clamped above */
    u64 ext_base = res;
    for (const auto& e : props_->extents) {
        const u64 ext_end = ext_base + e.length;
        const u64 ov_lo = lo > ext_base ? lo : ext_base;
        const u64 ov_hi = hi < ext_end ? hi : ext_end;
        if (ov_hi > ov_lo) {
            if (e.codec != 0) {
                const u64 psize = e.psize ? e.psize : e.length;
                if (psize == 0 || e.length == 0 || psize > kMaxDecode || e.length > kMaxDecode) {
                    /* malformed caps: read_at refuses without a device access */
                } else {
                    cost += psize; /* the whole packed block comes off the device */
                }
            } else {
                cost += ov_hi - ov_lo; /* the overlapped window, exactly */
            }
        }
        ext_base = ext_end;
    }
    return cost;
}

/* ---- bounded metadata extractors (G-10) ----
 *
 * All parsers obey Caps: they pull at most max_read bytes through the
 * reader (plus one bounded tail window for MP4 moov-at-end), track work
 * units, never recurse, and never allocate proportional to CLAIMED sizes
 * (only to bytes actually read). */

namespace {

/* bounded byte cursor over the reader (owns a window) */
struct Window {
    PreviewReader* pr;
    std::vector<u8> buf;
    u64 base = 0; /* logical offset of buf[0] */
    Caps caps;
    u64 pulled = 0;
    u64 work = 0;
    bool truncated = false;
    int64_t deadline = 0; /* M-077: steady_clock ns at the time-guard cutoff */
    u32 last_got = 0;     /* bytes available in the buffer returned by the
                             last fetch (short windows are the NORM for a
                             recovery product: every fetch consumer must
                             either demand the full window or consult this) */

    /* fetch at least `want` bytes at [off, off+want) - LENIENT: returns
     * whatever is available (last_got bytes, possibly fewer than `want`
     * at EOF). Returned pointer is valid only until the NEXT fetch.
     * nullptr only when zero bytes are available / caps exceeded.
     * M-028: the budget gate is COST-based (read_cost), so a fetch whose
     * underlying extent is compressed is charged the whole packed block
     * BEFORE the device is touched - the pre-fix byte-count gate let a
     * 4 KiB window over a 64 KiB block pull 64 KiB silently. */
    const u8* fetch_any(u64 off, u32 want) {
        if (work++ > caps.max_work) {
            truncated = true;
            return nullptr;
        }
        /* M-077: the decode-time guard (the og's 15 s product cap,
         * UIUX/10:24) - checked on the extraction's read heartbeat
         * (every fetch), bounded by the work gate's granularity; a host
         * overrides via Caps::max_decode_ms (UINT32_MAX disables). */
        if (deadline != 0 &&
            std::chrono::steady_clock::now().time_since_epoch().count() > int64_t(deadline)) {
            truncated = true;
            return nullptr;
        }
        const u32 w = want > 4096 ? u32(4096) : want;
        if (buf.size() >= w && base <= off && base + (buf.size() - w) >= off) {
            /* the window provably covers [off, off + w): every cached byte
             * is a real read byte (buffers never extend past EOF) */
            last_got = w;
            return buf.data() + size_t(off - base);
        }
        const u64 remaining = pr->size() > off ? pr->size() - off : 0;
        if (remaining == 0)
            return nullptr;
        u32 n = w;
        if (u64(n) > remaining)
            n = u32(remaining);
        const u64 cost = pr->read_cost(off, n);
        if (pulled + cost > caps.max_read) {
            truncated = true;
            return nullptr;
        }
        buf.resize(n);
        const u32 got = pr->read_at(off, buf.data(), n);
        if (got == 0)
            return nullptr;
        buf.resize(got);
        base = off;
        last_got = got;
        pulled += cost; /* device truth (compressed windows cost psize) */
        return buf.data();
    }

    /* M-028: cap-enforcing direct read for the parsers that need a
     * contiguous buffer outside the reusable window (the docx/zip tail,
     * central-directory, LFH and deflate windows). Refuses (0) when the
     * request's DEVICE cost would exceed the remaining budget - the
     * caller degrades honestly (truncated is set here). On success the
     * cost is accounted, so `pulled` always mirrors device bytes. */
    u32 checked_read(u64 off, void* dst, u32 len) {
        if (len == 0)
            return 0;
        if (off >= pr->size())
            return 0;
        if (u64(len) > pr->size() - off)
            len = u32(pr->size() - off);
        const u64 cost = pr->read_cost(off, len);
        if (pulled + cost > caps.max_read) {
            truncated = true;
            return 0;
        }
        const u32 got = pr->read_at(off, dst, len);
        if (got == 0)
            return 0;
        pulled += cost;
        return got;
    }

    /* STRICT fetch: the full `want` bytes or nothing. Truncated files
     * fail the parsers that need the bytes instead of over-reading the
     * window buffer (the pre-fix short-window class read `p[N]` past a
     * EOF-clamped buffer - caught by the CI fuzzer, cr_preview.cpp:648). */
    const u8* fetch(u64 off, u32 want) {
        const u8* p = fetch_any(off, want);
        if (p && last_got < want)
            return nullptr;
        return p;
    }
};

u64 pr_size(Window& w) {
    return w.pr->size();
}

u32 be16(const u8* p) {
    return u32(p[0]) << 8 | u32(p[1]);
}
u32 be32(const u8* p) {
    return u32(p[0]) << 24 | u32(p[1]) << 16 | u32(p[2]) << 8 | u32(p[3]);
}
u32 le16(const u8* p) {
    return u32(p[1]) << 8 | u32(p[0]);
}
u32 le32(const u8* p) {
    return u32(p[3]) << 24 | u32(p[2]) << 16 | u32(p[1]) << 8 | u32(p[0]);
}

/* --- image dimensions (bounded marker walks) --- */

bool jpeg_dims(Window& w, Metadata& m) {
    u64 off = 2;
    for (u32 guard = 0; guard < 256; ++guard) {
        const u8* p = w.fetch(off, 4);
        if (!p || p[0] != 0xFF)
            return false;
        const u8 marker = p[1];
        if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7) || marker == 0x01) {
            off += 2;
            continue;
        }
        if (marker == 0xDA)
            return false; /* no SOF before scan data */
        const u32 seg = be16(p + 2);
        if (seg < 2)
            return false;
        if ((marker >= 0xC0 && marker <= 0xC3) || (marker >= 0xC5 && marker <= 0xC7) ||
            (marker >= 0xC9 && marker <= 0xCB) || (marker >= 0xCD && marker <= 0xCF)) {
            const u8* q = w.fetch(off + 4, 8);
            if (!q || seg < 7)
                return false;
            m.height = be16(q + 1);
            m.width = be16(q + 3);
            return true;
        }
        off += 2 + seg;
    }
    return false;
}

bool jpeg_exif(Window& w, Metadata& m) {
    /* APP1/Exif: walk the marker stream to the first APP1 (bounded),
     * then a bounded TIFF IFD0 walk for Orientation(274) + DateTime(306) */
    u64 off = 2;
    u32 seg = 0;
    bool found = false;
    for (u32 guard = 0; guard < 64 && !found; ++guard) {
        const u8* m4 = w.fetch(off, 4);
        if (!m4 || m4[0] != 0xFF)
            return false;
        const u8 marker = m4[1];
        if (marker == 0xD8 || (marker >= 0xD0 && marker <= 0xD7) || marker == 0x01) {
            off += 2;
            continue;
        }
        if (marker == 0xDA)
            return false; /* SOS without APP1: no Exif */
        const u32 sz = be16(m4 + 2);
        if (sz < 2)
            return false;
        if (marker == 0xE1) {
            seg = sz;
            found = true;
            break;
        }
        off += 2 + sz;
    }
    if (!found || seg < 8)
        return false;
    const u8* hdr = w.fetch(off + 4, 6); /* "Exif\0\0" then TIFF header */
    if (!hdr || std::memcmp(hdr, "Exif", 4) != 0)
        return false;
    const u64 tiff = off + 10; /* TIFF header logical offset */
    const u8* t = w.fetch(tiff, 8);
    if (!t)
        return false;
    const bool le = t[0] == 'I' && t[1] == 'I';
    const auto rd16 = [&le](const u8* q) { return le ? le16(q) : be16(q); };
    const auto rd32 = [&le](const u8* q) { return le ? le32(q) : be32(q); };
    const u64 ifd0 = tiff + rd32(t + 4);
    const u8* cnt = w.fetch(ifd0, 2);
    if (!cnt)
        return false;
    const u32 n = rd16(cnt);
    if (n > 512)
        return false; /* implausible: hostile */
    for (u32 i = 0; i < n; ++i) {
        const u8* e = w.fetch(ifd0 + 2 + u64(i) * 12, 12);
        if (!e)
            return false;
        const u32 tag = rd16(e);
        const u32 type = rd16(e + 2);
        const u32 count = rd32(e + 4);
        if (tag == 274 && type == 3 && count == 1) {
            const u32 v = rd16(e + 8);
            if (v >= 1 && v <= 8)
                m.orientation = std::to_string(v);
        } else if (tag == 306 && type == 2 && count >= 19 && count <= 32) {
            /* value inline when <= 4 bytes, else the TIFF-relative offset
             * stored in the value field (e+8) */
            const u8* d = count > 4 ? w.fetch(tiff + rd32(e + 8), 20) : e + 8;
            if (d)
                m.datetime.assign(reinterpret_cast<const char*>(d), 19);
        }
    }
    return true;
}

bool png_dims(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 33);
    if (!p || std::memcmp(p + 1, "PNG", 3) != 0)
        return false;
    if (std::memcmp(p + 12, "IHDR", 4) != 0)
        return false;
    m.width = be32(p + 16);
    m.height = be32(p + 20);
    return true;
}

bool gif_dims(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 10);
    if (!p || std::memcmp(p, "GIF8", 4) != 0)
        return false;
    m.width = le16(p + 6);
    m.height = le16(p + 8);
    return true;
}

bool bmp_dims(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 26);
    if (!p || p[0] != 'B' || p[1] != 'M')
        return false;
    m.width = le32(p + 18);
    m.height = le32(p + 22) & 0x7FFFFFFFu;
    return true;
}

bool tiff_dims(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 8);
    if (!p)
        return false;
    const bool le = p[0] == 'I' && p[1] == 'I';
    const bool be = p[0] == 'M' && p[1] == 'M';
    if (!le && !be)
        return false;
    const auto rd16 = [&le](const u8* q) { return le ? le16(q) : be16(q); };
    const auto rd32 = [&le](const u8* q) { return le ? le32(q) : be32(q); };
    const u64 ifd = rd32(p + 4);
    const u8* c = w.fetch(ifd, 2);
    if (!c)
        return false;
    const u32 n = rd16(c);
    if (n > 512)
        return false;
    u32 w256 = 0, h257 = 0;
    for (u32 i = 0; i < n; ++i) {
        const u8* e = w.fetch(ifd + 2 + u64(i) * 12, 12);
        if (!e)
            break;
        const u32 tag = rd16(e);
        const u32 type = rd16(e + 2);
        if (tag == 256 && type == 3)
            w256 = rd16(e + 8);
        else if (tag == 257 && type == 3)
            h257 = rd16(e + 8);
        else if (tag == 256 && type == 4)
            w256 = rd32(e + 8);
        else if (tag == 257 && type == 4)
            h257 = rd32(e + 8);
    }
    m.width = w256;
    m.height = h257;
    return true;
}

bool webp_dims(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 30);
    if (!p || std::memcmp(p, "RIFF", 4) != 0 || std::memcmp(p + 8, "WEBP", 4) != 0)
        return false;
    if (std::memcmp(p + 12, "VP8X", 4) == 0) {
        const u32 cw = u32(p[24]) | u32(p[25]) << 8 | u32(p[26]) << 16;
        const u32 ch = u32(p[27]) | u32(p[28]) << 8 | u32(p[29]) << 16;
        m.width = cw + 1;
        m.height = ch + 1;
        return true;
    }
    if (std::memcmp(p + 12, "VP8 ", 4) == 0 && p[23] == 0x9D && p[24] == 0x01 && p[25] == 0x2A) {
        m.width = le16(p + 26) & 0x3FFF;
        m.height = le16(p + 28) & 0x3FFF;
        return true;
    }
    if (std::memcmp(p + 12, "VP8L", 4) == 0 && p[20] == 0x2F) {
        const u32 bits = u32(p[21]) | u32(p[22]) << 8 | u32(p[23]) << 16 | u32(p[24]) << 24;
        m.width = (bits & 0x3FFF) + 1;
        m.height = ((bits >> 14) & 0x3FFF) + 1;
        return true;
    }
    return false;
}

/* --- media --- */

bool mp4_info(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 16);
    if (!p || be32(p) < 8 || std::memcmp(p + 4, "ftyp", 4) != 0)
        return false;
    const u32 brand = be32(p + 8);
    char b[5] = {char(brand >> 24), char(brand >> 16), char(brand >> 8), char(brand), 0};
    m.codec = b;

    auto find_mvhd = [&](u64 from, u64 to) -> bool {
        u64 off = from;
        while (off + 8 <= to) {
            const u8* q = w.fetch(off, 16);
            if (!q)
                return false;
            const u32 sz = be32(q);
            if (sz < 8)
                return false;
            if (std::memcmp(q + 4, "moov", 4) == 0) {
                u64 co = off + 8;
                u64 cend = off + sz;
                if (cend > pr_size(w))
                    cend = pr_size(w);
                while (co + 8 <= cend) {
                    const u8* r = w.fetch(co, 32);
                    if (!r)
                        return false;
                    const u32 csz = be32(r);
                    if (csz < 8)
                        return false;
                    if (std::memcmp(r + 4, "mvhd", 4) == 0 && csz >= 28) {
                        const u32 ver = r[8];
                        if (ver == 0) {
                            const u32 timescale = be32(r + 20);
                            const u32 dur = be32(r + 24);
                            if (timescale) {
                                m.duration_ms = u64(dur) * 1000 / timescale;
                                return true;
                            }
                        } else if (ver == 1 && csz >= 40) {
                            const u32 timescale = be32(r + 28);
                            const u8* d = w.fetch(co + 32, 8);
                            if (d && timescale) {
                                const u64 dur = (u64(be32(d)) << 32) | be32(d + 4);
                                m.duration_ms = dur * 1000 / timescale;
                                return true;
                            }
                        }
                        return false;
                    }
                    co += csz;
                }
                return false;
            }
            if (sz == 0 || off + sz > to)
                return false;
            off += sz;
        }
        return false;
    };
    const u64 sz = pr_size(w);
    if (find_mvhd(0, sz < 65536 ? sz : 65536))
        return true;
    if (sz > 65536)
        return find_mvhd(sz - 65536, sz); /* moov-at-end tail window */
    return false;
}

bool wav_info(Window& w, Metadata& m) {
    const u8* p = w.fetch(0, 44);
    if (!p || std::memcmp(p, "RIFF", 4) != 0 || std::memcmp(p + 8, "WAVE", 4) != 0)
        return false;
    u64 off = 12;
    u32 rate = 0, bps = 0;
    u64 data = 0;
    for (u32 guard = 0; guard < 32; ++guard) {
        const u8* c = w.fetch(off, 8); /* chunk id + size */
        if (!c)
            break;
        const u32 csz = le32(c + 4);
        if (std::memcmp(c, "fmt ", 4) == 0 && csz >= 16) {
            /* fmt body: wFormatTag@+8, rate@+12, bits/sample@+22 (the
             * pre-fix code read bits/sample out of a 16-byte window:
             * bytes 22-23 were past the fetch and read stale window
             * memory whenever the fmt chunk sat past the head cache) */
            const u8* f = w.fetch(off + 8, 16);
            if (!f)
                break;
            const u32 fmt = le16(f);
            if (fmt == 1)
                m.codec = "pcm";
            else if (fmt == 3)
                m.codec = "float";
            else if (fmt == 6)
                m.codec = "alaw";
            else if (fmt == 7)
                m.codec = "mulaw";
            else if (fmt == 0xFFFE)
                m.codec = "ext";
            rate = le32(f + 4); /* +12 chunk-relative */
            bps = le16(f + 14); /* +22 chunk-relative */
        } else if (std::memcmp(c, "data", 4) == 0) {
            data = csz;
            break;
        }
        if (csz == 0)
            break;
        off += 8 + csz + (csz & 1);
    }
    if (rate && bps >= 8) {
        const u64 bytes_per_sec = u64(rate) * (bps / 8);
        if (bytes_per_sec)
            m.duration_ms = data * 1000 / bytes_per_sec;
    }
    return true;
}

/* --- documents --- */

void text_sniff(Window& w, Metadata& m) {
    u64 off = 0;
    std::string acc;
    while (acc.size() < w.caps.text_snippet && off < pr_size(w)) {
        const u8* p = w.fetch_any(off, 512);
        if (!p)
            break;
        bool nul = false;
        for (size_t i = 0; i < w.last_got && acc.size() < w.caps.text_snippet; ++i) {
            const u8 c = w.buf[i];
            if (c == 0) {
                nul = true;
                break;
            }
            if (c >= 0x20 && c < 0x7F)
                acc.push_back(char(c));
            else if (c == '\n' || c == '\t')
                acc.push_back(' ');
        }
        if (nul)
            break; /* binary: keep what we have */
        off += w.last_got;
        if (w.last_got < 512)
            break;
    }
    m.text_snippet = acc;
    m.kind = "text";
    m.format = "txt";
}

void pdf_sniff(Window& w, Metadata& m) {
    const u8* p = w.fetch_any(0, 1024);
    if (!p || std::memcmp(p, "%PDF-", 5) != 0)
        return;
    m.kind = "document";
    m.format = "pdf";
    std::string acc;
    for (size_t i = 0; i < w.last_got && acc.size() < w.caps.text_snippet; ++i) {
        const u8 c = w.buf[i];
        if (c >= 0x20 && c < 0x7F)
            acc.push_back(char(c));
    }
    m.text_snippet = acc;
}

/* DOCX/XLSX/PPTX: [local file header <PK\x03\x04 ...>] - bounded zip
 * central-directory scan from the END of the stream (the corpus caps
 * previews, so we only read the tail window), then a bounded inflate of
 * word/document.xml's first bytes. Uses zlib (already a core dependency).
 * ZIP entry bodies are RAW deflate (windowBits -15) - the pre-fix code
 * used 15 (zlib-header mode), which rejects ~every real DOCX stream.
 * Labels stay honest: a zip is "archive"/"zip" until a known OOXML part
 * is found; word/document.xml upgrades it to "document"/"docx",
 * xl/workbook.xml to "docx"-shaped spreadsheets stays "zip" unless
 * found. */
struct ZStreamGuard {
    z_stream zs{};
    bool ok = false;
    ZStreamGuard() { ok = inflateInit2(&zs, -15 /* raw deflate, ZIP body */) == Z_OK; }
    ~ZStreamGuard() {
        if (ok)
            inflateEnd(&zs);
    }
};

void docx_sniff(Window& w, Metadata& m) {
    m.kind = "archive"; /* any PK-prefixed stream: honest, no docx claim */
    m.format = "zip";
    const u64 sz = pr_size(w);
    if (sz < 22)
        return;
    /* End of Central Directory: last 64 KiB window, search backwards
     * (covers eocd at offset 0 of the window; strict bounds) */
    const u64 tail_off = sz > 65536 ? sz - 65536 : 0;
    const u32 tail_len = u32(sz - tail_off);
    std::vector<u8> tail(tail_len, 0);
    const u32 got = w.checked_read(tail_off, tail.data(), tail_len);
    if (got == 0)
        return; /* M-028: over-budget tail refused - honest archive/zip
                 * degrade with truncated set (pre-fix: up to 64 KiB
                 * pulled straight through the reader, past the cap) */
    tail.resize(got);
    u32 eocd = 0;
    bool have_eocd = false;
    if (tail.size() >= 22) {
        for (size_t i = tail.size() - 22;; --i) {
            if (tail[i] == 'P' && tail[i + 1] == 'K' && tail[i + 2] == 0x05 &&
                tail[i + 3] == 0x06) {
                eocd = u32(i);
                have_eocd = true;
                break;
            }
            if (i == 0)
                break;
        }
    }
    if (!have_eocd)
        return; /* no central directory reachable: still an honest zip
                 fragment label (archive/zip set at entry) */
    /* central directory offset/size */
    const u32 cd_size = le32(tail.data() + eocd + 12);
    const u32 cd_off = le32(tail.data() + eocd + 16);
    if (u64(cd_off) + cd_size > sz)
        return;
    /* bounded CD read (M-028: through the cap gate) */
    const u32 cd_len = cd_size > 65536 ? 65536 : cd_size;
    std::vector<u8> cd(cd_len, 0);
    const u32 cd_got = w.checked_read(cd_off, cd.data(), cd_len);
    if (cd_got == 0) {
        return; /* budget exhausted: honest zip label + truncated */
    }
    if (cd_got < cd_len)
        m.truncated = true; /* the CD is partially read: the inventory is
                             * partial - never a silent loss */
    cd.resize(cd_got);
    /* find word/document.xml entry (local header offset) AND collect the
     * content inventory (M-066: the og's ArchivePreviewModel browse-
     * inside-archive contract - every walkable CD entry, damaged zips
     * included, carved or FS-origin alike; bounded by
     * kMaxPreviewArchiveEntries with the truncated flag) */
    static const char want[] = "word/document.xml";
    const size_t want_len = sizeof(want) - 1;
    u64 lho = 0;
    bool found = false;
    bool entries_capped = false;
    for (size_t o = 0; o + 46 <= cd.size();) {
        if (cd[o] == 'P' && cd[o + 1] == 'K' && cd[o + 2] == 0x01 && cd[o + 3] == 0x02) {
            const u32 nlen = le16(cd.data() + o + 28);
            const u32 elen = le16(cd.data() + o + 30);
            const u32 clen = le16(cd.data() + o + 32);
            if (o + 46 + nlen <= cd.size()) {
                if (!entries_capped && m.entries.size() < kMaxPreviewArchiveEntries) {
                    ArchiveEntry e;
                    e.name.assign(reinterpret_cast<const char*>(cd.data() + o + 46),
                                  reinterpret_cast<const char*>(cd.data() + o + 46) + nlen);
                    e.size = le32(cd.data() + o + 24);
                    e.compressed = le32(cd.data() + o + 20);
                    e.crc_present = le32(cd.data() + o + 16) != 0;
                    m.entries.push_back(std::move(e));
                } else if (m.entries.size() >= kMaxPreviewArchiveEntries) {
                    entries_capped = true; /* hostile archive: cap + flag */
                }
                if (nlen == want_len && std::memcmp(cd.data() + o + 46, want, want_len) == 0) {
                    lho = le32(cd.data() + o + 42);
                    found = true;
                    /* the hunt continues: the upgrade needs the walk, the
                     * inventory needs every entry - no break */
                }
            }
            o += 46 + u64(nlen) + elen + clen;
        } else {
            ++o;
        }
    }
    if (entries_capped)
        m.truncated = true; /* the inventory is partial - never a silent loss */
    if (!found)
        return; /* xlsx/pptx/plain zip: no docx upgrade - the label stays,
                 * but the M-066 inventory is already in m.entries */
    /* local file header: name len + extra len, then the deflate stream */
    std::vector<u8> lfh(30, 0);
    if (w.checked_read(lho, lfh.data(), 30) != 30 || lfh[0] != 'P' || lfh[1] != 'K')
        return;
    const u32 nlen = le16(lfh.data() + 26);
    const u32 elen = le16(lfh.data() + 28);
    const u64 data_off = lho + 30 + nlen + elen;
    /* bounded inflate (M-028: through the cap gate) */
    std::vector<u8> comp(2048, 0);
    const u32 cr = w.checked_read(data_off, comp.data(), 2048);
    if (cr == 0)
        return;
    comp.resize(cr);
    std::vector<u8> out(4096, 0);
    ZStreamGuard zg;
    if (!zg.ok)
        return;
    zg.zs.next_in = comp.data();
    zg.zs.avail_in = uInt(comp.size());
    zg.zs.next_out = out.data();
    zg.zs.avail_out = uInt(out.size());
    (void)inflate(&zg.zs, Z_SYNC_FLUSH);
    /* strip tags, keep text */
    std::string acc;
    bool in_tag = false;
    for (size_t i = 0; i < out.size() - zg.zs.avail_out && acc.size() < w.caps.text_snippet; ++i) {
        const char c = char(out[i]);
        if (c == '<')
            in_tag = true;
        else if (c == '>')
            in_tag = false;
        else if (!in_tag && c >= 0x20 && c < 0x7F)
            acc.push_back(c);
    }
    m.text_snippet = acc;
    m.kind = "document";
    m.format = "docx";
}

} // namespace

Metadata extract_metadata(PreviewReader& pr, const Caps& caps) {
    /* M-077: the og's shipped product caps (UIUX/10 §3, QM-verified copy):
     * the 100 MB eligibility gate refuses BEFORE any read ("Files larger
     * than 100MB are not available for preview") - never silent (the
     * truncated flag carries the gate). */
    if (pr.size() > u64(caps.max_file_bytes)) {
        Metadata early;
        early.kind = "oversize";
        early.format = "eligibility";
        early.truncated = true;
        return early;
    }
    Metadata m;
    Window w;
    w.pr = &pr;
    w.caps = caps;
    /* M-077: the 15 s decode-time guard - the steady-clock deadline the
     * fetch heartbeat checks (a host overrides via Caps::max_decode_ms;
     * UINT32_MAX disables the gate). */
    if (caps.max_decode_ms != 0xFFFFFFFFu) {
        w.deadline =
            (std::chrono::steady_clock::now() + std::chrono::milliseconds(caps.max_decode_ms))
                .time_since_epoch()
                .count();
    }
    /* M-028: per-call device-byte baseline (the closing belt compares
     * the delta against caps.max_read) */
    const u64 dev_bytes_at_entry = pr.device_bytes();

    /* one head fetch; scalarize every magic BEFORE further fetches (the
     * Window buffer is reused - pointers do not survive a re-fetch).
     * LENIENT head fetch: short headers are the recovery norm, so every
     * magic probe is gated on the bytes actually present (the strict
     * fetch(0,32) of the pre-fix code over-read p[N] past EOF-clamped
     * windows on every truncated file - CI-fuzzer caught). */
    const u8* head = w.fetch_any(0, 32);
    if (!head) {
        /* zero-length, unreadable, or over-budget head (M-028: an
         * over-budget head must still report truncated truthfully -
         * the pre-fix early return dropped the flag on exactly the
         * path a compressed-extent head busts its budget on) */
        m.truncated = w.truncated;
        if (pr.device_bytes() - dev_bytes_at_entry > caps.max_read)
            m.truncated = true;
        return m;
    }
    const size_t hs = w.last_got;
    const u64 sz = pr.size();
    /* scalarize only bytes the window provably holds (a 1-byte file must
     * not read head[1] - caught by the truncated-magic regression) */
    const u8 h0 = hs >= 1 ? head[0] : 0;
    const u8 h1 = hs >= 2 ? head[1] : 0;
    const bool is_jpeg = hs >= 2 && h0 == 0xFF && h1 == 0xD8;
    const bool is_png = hs >= 4 && std::memcmp(head + 1, "PNG", 3) == 0;
    const bool is_gif = hs >= 4 && std::memcmp(head, "GIF8", 4) == 0;
    const bool is_bmp = hs >= 2 && h0 == 'B' && h1 == 'M';
    const bool is_tiff = hs >= 4 && ((h0 == 'I' && h1 == 'I' && head[2] == 42 && head[3] == 0) ||
                                     (h0 == 'M' && h1 == 'M' && head[2] == 0 && head[3] == 42));
    const bool riff = hs >= 4 && std::memcmp(head, "RIFF", 4) == 0;
    bool is_webp = false, is_wav = false;
    if (riff && sz >= 12) {
        const u8* w8 = w.fetch(8, 4);
        if (w8) {
            is_wav = std::memcmp(w8, "WAVE", 4) == 0;
            is_webp = std::memcmp(w8, "WEBP", 4) == 0;
        }
    }
    const bool is_mp4 = hs >= 12 && be32(head + 4) >= 8 && std::memcmp(head + 4, "ftyp", 4) == 0;
    const bool is_pdf = hs >= 5 && std::memcmp(head, "%PDF-", 5) == 0;
    const bool is_zip = hs >= 2 && h0 == 'P' && h1 == 'K';

    if (is_jpeg) {
        m.kind = "image";
        m.format = "jpeg";
        (void)jpeg_dims(w, m);
        (void)jpeg_exif(w, m);
    } else if (is_png) {
        m.kind = "image";
        m.format = "png";
        (void)png_dims(w, m);
    } else if (is_gif) {
        m.kind = "image";
        m.format = "gif";
        (void)gif_dims(w, m);
    } else if (is_bmp) {
        m.kind = "image";
        m.format = "bmp";
        (void)bmp_dims(w, m);
    } else if (is_tiff) {
        m.kind = "image";
        m.format = "tiff";
        (void)tiff_dims(w, m);
    } else if (is_webp) {
        m.kind = "image";
        m.format = "webp";
        (void)webp_dims(w, m);
    } else if (is_wav) {
        m.kind = "audio";
        m.format = "wav";
        (void)wav_info(w, m);
    } else if (is_mp4) {
        m.kind = "video";
        m.format = "mp4"; /* brand string refined in mp4_info */
        (void)mp4_info(w, m);
    } else if (is_pdf) {
        pdf_sniff(w, m);
    } else if (is_zip) {
        docx_sniff(w, m);
    } else {
        bool printable = true;
        for (size_t i = 0; i < 16 && i < hs; ++i)
            if (head[i] >= 0x7F)
                printable = false;
        if (printable && sz > 0)
            text_sniff(w, m);
        else
            m.kind = "unsupported";
    }
    /* M-066: OR, not assign - the read-budget flag (w.truncated) and the
     * inventory flag (the archive-entry cap inside docx_sniff, plus the
     * CD partial-read flag) are independent truncation causes */
    m.truncated = m.truncated || w.truncated;
    /* M-028 belt: if ANY path still moved more device bytes than the
     * cap allows, the flag tells the truth (per-call delta, so a reused
     * reader's earlier reads do not false-positive). With the cost-gated
     * fetch_any/checked_read this never fires today - it exists so a
     * future parser cannot reintroduce the bypass silently. */
    if (pr.device_bytes() - dev_bytes_at_entry > caps.max_read)
        m.truncated = true;
    return m;
}

} // namespace preview
} // namespace cr
