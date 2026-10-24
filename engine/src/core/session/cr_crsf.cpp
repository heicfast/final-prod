/* cr_crsf.cpp - the CRSF container (chunked zlib, CRC-guarded).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/session/cr_crsf.h"
#include "cr/table/cr_table_wire.h" /* crc32 */
#include <zlib.h>
#include <cstring>

namespace cr {
namespace session {

namespace {
constexpr u8 REC_MAGIC[4] = {'C', 'R', 'S', 'R'};
}

/* ---- chunk framing (reference-proven scheme + our CRC accounting) ---- */

u32 crsf_chunk_frame(const u8* payload, u32 len, bool compress, std::vector<u8>& out) {
    if (len == 0 || len > CRSF_CHUNK_MAX)
        return 0x50040001; /* ERR_SESSION_FORMAT */
    if (compress) {
        uLongf comp_len = compressBound(len);
        std::vector<u8> comp(comp_len);
        if (compress2(comp.data(), &comp_len, payload, len, 5) != Z_OK)
            return 0x50040002;
        if (comp_len < len && comp_len < CRSF_CHUNK_MAX) {
            /* deflate only when strictly smaller (reference comparison rule) */
            out.push_back(u8(((comp_len - 1) & 0xFFF) | 0x8000));
            out.push_back(u8((((comp_len - 1) & 0xFFF) | 0x8000) >> 8));
            out.insert(out.end(), comp.begin(), comp.begin() + comp_len);
            return OK;
        }
    }
    /* stored */
    out.push_back(u8(((len - 1) & 0xFFF)));
    out.push_back(u8((((len - 1) & 0xFFF)) >> 8));
    out.insert(out.end(), payload, payload + len);
    return OK;
}

u32 crsf_chunk_parse(const u8* data, u32 len, std::vector<u8>& out) {
    if (len < 2)
        return 0x50040001;
    /* header: bits0-11 = payload-1; bit15 = deflate */
    const u16 hdr = u16(data[0]) | (u16(data[1]) << 8);
    const u32 payload_len = (hdr & 0x0FFF) + 1;
    const bool deflated = (hdr & 0x8000) != 0;
    if (2 + payload_len > len)
        return 0x50040001;
    if (deflated) {
        uLongf out_len = CRSF_CHUNK_MAX * 4;
        std::vector<u8> buf(out_len);
        if (uncompress(buf.data(), &out_len, data + 2, payload_len) != Z_OK)
            return 0x50040002; /* caller decides: fallback to raw (F10) */
        out.insert(out.end(), buf.begin(), buf.begin() + out_len);
    } else {
        out.insert(out.end(), data + 2, data + 2 + payload_len);
    }
    return OK;
}

/* ---- Writer ---- */

struct Writer::Impl {
    WriteFn write = nullptr;
    TellFn tell = nullptr;
    SeekFn seek = nullptr;
    void* user = nullptr;
    u64 session_id = 0;
    u32 record_count = 0;
    struct Pending {
        u64 header_at = 0; /* tell() when the record header was written */
        RecordType type;
        u32 chain = 0; /* on-disk bytes after the header */
        u32 crc = 0;   /* CRC32 of the DECOMPRESSED data */
    };
    std::vector<Pending> pending;
};

Writer::Writer(WriteFn write, TellFn tell, SeekFn seek, void* user)
    : impl_(new Impl{write, tell, seek, user, 0, 0, {}}) {}
Writer::~Writer() = default;

u32 Writer::open(u64 session_id) {
    impl_->session_id = session_id;
    u8 hdr[CRSF_HEADER_BYTES];
    std::memset(hdr, 0, sizeof(hdr));
    std::memcpy(hdr, CRSF_MAGIC, 8);
    u32 v = CRSF_VERSION;
    hdr[8] = u8(v);
    hdr[9] = u8(v >> 8);
    hdr[10] = u8(v >> 16);
    hdr[11] = u8(v >> 24);
    /* record_count at +0x10 patched in finalize */
    u32 u = CRSF_UNIT;
    hdr[0x14] = u8(u);
    hdr[0x15] = u8(u >> 8);
    hdr[0x16] = u8(u >> 16);
    hdr[0x17] = u8(u >> 24);
    u32 sid_lo = u32(session_id);
    hdr[0x18] = u8(sid_lo);
    hdr[0x19] = u8(sid_lo >> 8);
    hdr[0x1A] = u8(sid_lo >> 16);
    hdr[0x1B] = u8(sid_lo >> 24);
    u32 sid_hi = u32(session_id >> 32);
    hdr[0x1C] = u8(sid_hi);
    hdr[0x1D] = u8(sid_hi >> 8);
    hdr[0x1E] = u8(sid_hi >> 16);
    hdr[0x1F] = u8(sid_hi >> 24); /* audit row 33: hi dword was never
                                     written - 64-bit session ids were
                                     silently truncated */
    return impl_->write(impl_->user, hdr, sizeof(hdr)) != 0 ? u32(0x50040002) : u32(OK);
}

u32 Writer::add_record(RecordType type, const void* data, u32 len) {
    /* emit the 24-byte header with chain=0 (patched in finalize) */
    u8 rh[CRSF_RECORD_HEADER_BYTES];
    std::memset(rh, 0, sizeof(rh));
    std::memcpy(rh, REC_MAGIC, 4);
    rh[4] = u8(type);
    rh[5] = u8(u32(type) >> 8);
    rh[6] = u8(u32(type) >> 16);
    rh[7] = u8(u32(type) >> 24);
    const u64 sid = impl_->session_id;
    for (int i = 0; i < 8; ++i)
        rh[8 + i] = u8(sid >> (8 * i));
    /* chain/crc zeroed; written at +0x10/+0x14 */
    const u64 at = impl_->tell(impl_->user);
    if (u32 rc = impl_->write(impl_->user, rh, sizeof(rh)))
        return rc;

    /* frame the payload in <=4KiB chunks */
    std::vector<u8> framed;
    const u8* p = static_cast<const u8*>(data);
    u32 remaining = len;
    while (remaining > 0) {
        u32 take = remaining < CRSF_CHUNK_MAX ? remaining : CRSF_CHUNK_MAX;
        /* SESS-4 (GAP-W3-9b): the reference compresses EVERY chunk and
         * stores when deflate does not shrink it (the strictly-smaller rule
         * lives in crsf_chunk_frame). The old `take >= 64` carve-out kept
         * tiny tail chunks stored unconditionally - a divergence from the
         * reference-proven framing scheme. */
        bool try_deflate = true;
        std::vector<u8> chunk;
        if (u32 rc = crsf_chunk_frame(p, take, try_deflate, chunk))
            return rc;
        framed.insert(framed.end(), chunk.begin(), chunk.end());
        p += take;
        remaining -= take;
    }
    if (!framed.empty()) {
        if (u32 rc = impl_->write(impl_->user, framed.data(), u32(framed.size())))
            return rc;
    }
    Impl::Pending pd;
    pd.header_at = at;
    pd.type = type;
    pd.chain = u32(framed.size());
    pd.crc = table::crc32(data, len);
    impl_->pending.push_back(pd);
    ++impl_->record_count;
    return OK;
}

u32 Writer::finalize() {
    /* patch record_count in the container header */
    if (u32 rc = impl_->seek(impl_->user, 0x10))
        return rc;
    u8 cnt[4];
    cnt[0] = u8(impl_->record_count);
    cnt[1] = u8(impl_->record_count >> 8);
    cnt[2] = u8(impl_->record_count >> 16);
    cnt[3] = u8(impl_->record_count >> 24);
    if (u32 rc = impl_->write(impl_->user, cnt, 4))
        return rc;
    /* patch each record header's chain + crc */
    for (const auto& pd : impl_->pending) {
        if (u32 rc = impl_->seek(impl_->user, pd.header_at + 0x10))
            return rc;
        u8 f[8];
        f[0] = u8(pd.chain);
        f[1] = u8(pd.chain >> 8);
        f[2] = u8(pd.chain >> 16);
        f[3] = u8(pd.chain >> 24);
        f[4] = u8(pd.crc);
        f[5] = u8(pd.crc >> 8);
        f[6] = u8(pd.crc >> 16);
        f[7] = u8(pd.crc >> 24);
        if (u32 rc = impl_->write(impl_->user, f, 8))
            return rc;
    }
    return OK;
}

/* ---- Reader ---- */

struct Reader::Impl {
    ReadFn read = nullptr;
    SeekFn seek = nullptr;
    SizeFn size = nullptr;
    void* user = nullptr;
    std::vector<u64> record_offsets; /* header positions */
};

Reader::Reader(ReadFn read, SeekFn seek, SizeFn size, void* user)
    : impl_(new Impl{read, seek, size, user, {}}) {}
Reader::~Reader() = default;

u32 Reader::open(u32* record_count_out) {
    const u64 total = impl_->size(impl_->user);
    if (total < CRSF_HEADER_BYTES)
        return 0x50040001;
    std::vector<u8> buf(CRSF_HEADER_BYTES);
    if (impl_->seek(impl_->user, 0))
        return 0x50040002;
    if (impl_->read(impl_->user, buf.data(), CRSF_HEADER_BYTES))
        return 0x50040002;
    if (std::memcmp(buf.data(), CRSF_MAGIC, 8) != 0)
        return 0x50030001; /* BAD_MAGIC */
    const u32 version = u32_from_bytes_le(buf.data() + 8);
    if (version != CRSF_VERSION)
        return 0x50040003; /* SESSION_VERSION */
    count_ = u32_from_bytes_le(buf.data() + 0x10);
    if (record_count_out)
        *record_count_out = count_;

    /* index records by walking the chain; on a broken header skip forward
     * in UNIT steps until a well-formed header or EOF (F10 resumability) */
    impl_->record_offsets.clear();
    u64 pos = CRSF_HEADER_BYTES;
    u32 guard = 0;
    while (pos + CRSF_RECORD_HEADER_BYTES <= total && guard++ < 65536) {
        if (impl_->record_offsets.size() >= count_ && count_ != 0)
            break;
        u8 rh[CRSF_RECORD_HEADER_BYTES];
        if (impl_->seek(impl_->user, pos))
            break;
        if (impl_->read(impl_->user, rh, CRSF_RECORD_HEADER_BYTES))
            break;
        if (std::memcmp(rh, REC_MAGIC, 4) != 0) {
            pos += CRSF_UNIT; /* skip forward (F10) */
            continue;
        }
        const u32 chain = u32_from_bytes_le(rh + 0x10);
        if (chain == 0 && u32_from_bytes_le(rh + 0x14) == 0) {
            /* unfinalized tail record: stop indexing (truncated save) */
            break;
        }
        if (pos + CRSF_RECORD_HEADER_BYTES + chain > total)
            break;
        impl_->record_offsets.push_back(pos);
        pos += CRSF_RECORD_HEADER_BYTES + chain;
    }
    if (count_ == 0)
        count_ = u32(impl_->record_offsets.size());
    /* Torn-tail salvage (F10, BENCH-F1): the header's record_count is patched
     * only on finalize. A writer killed mid-checkpoint (hard-kill cancel probe)
     * leaves a header that claims N records while the walk indexes fewer -
     * the unfinalized tail record stops it by construction. Exposing the
     * header's N to the caller made read_record(N-1) fail with
     * ERR_SESSION_FORMAT and defeated the documented "resumable by
     * construction" contract (a torn checkpoint is no worse than none).
     * The indexed prefix is the salvageable truth: clamp. */
    else if (u32(impl_->record_offsets.size()) < count_)
        count_ = u32(impl_->record_offsets.size());
    if (record_count_out)
        *record_count_out = count_;
    return OK;
}

u32 Reader::read_record(u32 index, Record& out) {
    if (index >= impl_->record_offsets.size())
        return 0x50040001;
    const u64 at = impl_->record_offsets[index];
    if (impl_->seek(impl_->user, at))
        return 0x50040002;
    u8 rh[CRSF_RECORD_HEADER_BYTES];
    if (impl_->read(impl_->user, rh, CRSF_RECORD_HEADER_BYTES))
        return 0x50040002;
    if (std::memcmp(rh, REC_MAGIC, 4) != 0)
        return 0x50030001;
    out.type = RecordType(u32_from_bytes_le(rh + 4));
    out.session_id = u64_from_bytes_le(rh + 8);
    const u32 chain = u32_from_bytes_le(rh + 0x10);
    const u32 want_crc = u32_from_bytes_le(rh + 0x14);
    std::vector<u8> framed(chain);
    if (chain) {
        if (impl_->read(impl_->user, framed.data(), chain))
            return 0x50040002;
    }
    /* inflate chunk-by-chunk; per-chunk fallback to the raw payload (F10).
     * P1-7: record-level budget caps the 4 KiB-chunk x 16 KiB-inflate
     * amplification (a crafted session could otherwise demand ~8192x its
     * size in heap) */
    constexpr size_t CRSF_RECORD_MAX = 64u << 20; /* 64 MiB */
    out.data.clear();
    out.data.reserve(std::min<size_t>(framed.size() * 4, CRSF_RECORD_MAX));
    u32 pos = 0;
    while (pos + 2 <= framed.size()) {
        const u16 hdr = u16(framed[pos]) | (u16(framed[pos + 1]) << 8);
        const u32 payload_len = (hdr & 0x0FFF) + 1;
        if (pos + 2 + payload_len > framed.size())
            break;
        std::vector<u8> chunk;
        if (crsf_chunk_parse(framed.data() + pos, payload_len + 2, chunk) != OK) {
            chunk.assign(framed.begin() + pos + 2, framed.begin() + pos + 2 + payload_len);
        }
        if (out.data.size() + chunk.size() > CRSF_RECORD_MAX) {
            out.data.clear();
            return ERR_NO_MEMORY; /* decompression bomb */
        }
        out.data.insert(out.data.end(), chunk.begin(), chunk.end());
        pos += 2 + payload_len;
    }
    const u32 have_crc = table::crc32(out.data.data(), out.data.size());
    if (have_crc != want_crc) {
        out.data.clear();
        return 0x50030002; /* BAD_CHECKSUM: caller may continue with later records */
    }
    return OK;
}

} // namespace session
} // namespace cr
