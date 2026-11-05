/*
 * cr_reghive_edit.cpp - the offline hive value editor (M-113).
 * The header carries the corpus contract; this file is the editor.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/reghive/cr_reghive_edit.h"
#include "cr/cr_error.h"

#include <cstring>

namespace cr {
namespace reghive {

namespace {

inline u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
inline u16 rd16(const u8* p) {
    return u16(u32(p[0]) | (u32(p[1]) << 8));
}
inline void wr32(u8* p, u32 x) {
    for (int i = 0; i < 4; ++i)
        p[i] = u8(x >> (8 * i));
}

/* the hbin grid: hbin headers live every N bytes from file offset 0x1000,
 * each 0x20 bytes: 'hbin' + offset-of-this-hbin (bins-relative) + size */
bool hbin_size_at(const std::vector<u8>& buf, u32 bins_end, u32& hbin_off, u32& hbin_size) {
    if (hbin_off + HBIN_HEADER_SIZE > bins_end)
        return false;
    const u8* h = buf.data() + REGF_HEADER_SIZE + hbin_off;
    if (std::memcmp(h, "hbin", 4) != 0)
        return false;
    hbin_size = rd32(h + 8);
    return hbin_size != 0 && hbin_off + hbin_size <= bins_end;
}

} // namespace

bool HiveEditor::open(const u8* data, size_t size) {
    buf_.assign(data, data + size);
    if (!view_.open(buf_.data(), buf_.size()))
        return false;
    seq_ = rd32(buf_.data() + 4);
    return true;
}

u32 HiveEditor::find_vk_(const char* key_path, const char* value_name) const {
    const u64 key = view_.find_key(key_path);
    if (key == 0)
        return 0;
    const u32 nk_off = u32(key - 1);
    u32 list_off = 0;
    u32 count = 0;
    /* mirror the parser's values-list discovery (nk +0x24 count / +0x28
     * list - cr_reghive.cpp's own layout comments) */
    const u8* nk = buf_.data() + REGF_HEADER_SIZE + nk_off + 4; /* past cell size */
    if (nk + 0x2C > buf_.data() + buf_.size())
        return 0;
    count = rd32(nk + 0x24);
    list_off = rd32(nk + 0x28);
    if (count == 0 || list_off == 0 || list_off == 0xFFFFFFFFu || count > 0x10000)
        return 0;
    const u64 pos = u64(REGF_HEADER_SIZE) + u64(list_off) + 4;
    if (pos + u64(count) * 4 > buf_.size())
        return 0;
    for (u32 i = 0; i < count; ++i) {
        const u32 vk_off = rd32(buf_.data() + pos + u64(i) * 4);
        if (vk_off == 0 || vk_off == 0xFFFFFFFFu)
            continue;
        const u64 vpos = u64(REGF_HEADER_SIZE) + u64(vk_off);
        if (vpos + 4 + 0x18 > buf_.size())
            continue;
        const u8* vk = buf_.data() + vpos + 4; /* past cell size */
        if (rd16(vk) != 0x6b76)                /* 'vk' */
            continue;
        const u16 nlen = rd16(vk + 0x02);
        const u16 vflags = rd16(vk + 0x10);
        const bool ascii = (vflags & 0x01) != 0;
        std::string vn;
        if (ascii) {
            if (u64(vk - buf_.data()) + 0x12 + nlen > buf_.size())
                continue;
            vn.assign(reinterpret_cast<const char*>(vk + 0x12), nlen);
        } else {
            const size_t chars = nlen / 2;
            vn.reserve(chars);
            for (size_t c = 0; c < chars; ++c)
                vn.push_back(char(rd16(vk + 0x12 + c * 2) & 0x7F));
        }
        if (vn == value_name) /* exact-case: the caller derives names from
                               * decoded constants; the parser's fold lives
                               * in read_value - the surgeries' names are
                               * the corpus literals */
            return vk_off;
    }
    return 0;
}

bool HiveEditor::has_value(const char* key_path, const char* value_name) const {
    return find_vk_(key_path, value_name) != 0;
}

bool HiveEditor::read_value(const char* key_path, const char* value_name, u32& out_type,
                            std::vector<u8>& out_data) const {
    return view_.read_value(view_.find_key(key_path), value_name, out_type, out_data);
}

u32 HiveEditor::allocate_cell_(size_t need_bytes) {
    /* the allocation unit: 4 (size dword) + need, 8-aligned */
    const size_t need = (4 + need_bytes + 7) & ~size_t(7);
    const u32 bins_end = rd32(buf_.data() + REGF_BINS_SIZE_OFF);

    /* first-fit over free cells: walk every hbin, cell by cell */
    u32 hbin = 0;
    while (hbin < bins_end) {
        u32 hsize = 0;
        if (!hbin_size_at(buf_, bins_end, hbin, hsize))
            break;
        u32 off = hbin + HBIN_HEADER_SIZE;
        while (off + 8 <= hbin + hsize) {
            const i32 cs = i32(rd32(buf_.data() + REGF_HEADER_SIZE + off));
            if (cs > 0 && size_t(cs) >= need && size_t(cs) <= 0x100000) {
                /* a free cell big enough: split when the remainder can
                 * stand alone (>= 8 bytes) - the used span becomes an
                 * ALLOCATED (negative) cell, the tail stays free */
                if (size_t(cs) - need >= 8) {
                    wr32(buf_.data() + REGF_HEADER_SIZE + off, u32(-i32(need)));
                    wr32(buf_.data() + REGF_HEADER_SIZE + off + need, u32(cs) - u32(need));
                } else {
                    /* keep the cell's full size as the allocation */
                    wr32(buf_.data() + REGF_HEADER_SIZE + off, u32(-i32(cs)));
                }
                return off;
            }
            const size_t step = size_t(cs < 0 ? -cs : cs);
            if (step < 8)
                break; /* corrupt stride: stop this hbin */
            off += u32(step);
        }
        hbin += hsize;
    }

    /* no free cell: append a NEW hbin of whole 0x1000 blocks (the
     * format's own growth pattern; cells never span hbins) */
    const u32 new_hbin = bins_end;
    size_t blocks = (HBIN_HEADER_SIZE + 4 + need_bytes + 0xFFF) / 0x1000;
    if (blocks == 0)
        blocks = 1;
    const u32 add = u32(blocks * 0x1000);
    const size_t new_file_size = size_t(REGF_HEADER_SIZE) + size_t(bins_end) + add;
    buf_.resize(new_file_size, 0);
    u8* h = buf_.data() + REGF_HEADER_SIZE + new_hbin;
    std::memcpy(h, "hbin", 4);
    wr32(h + 4, new_hbin); /* this hbin's bins-relative offset */
    wr32(h + 8, add);      /* this hbin's size */
    /* the new cell starts right after the hbin header; the cell's size
     * dword is the ALLOCATED (negative) size of the used span; the tail
     * within the hbin stays one free cell */
    u8* c = h + HBIN_HEADER_SIZE;
    const u32 used = u32((4 + need_bytes + 7) & ~size_t(7));
    wr32(c, u32(-i32(used)));
    const u32 tail_off = new_hbin + HBIN_HEADER_SIZE + used;
    if (add > HBIN_HEADER_SIZE + used)
        wr32(buf_.data() + REGF_HEADER_SIZE + tail_off, add - HBIN_HEADER_SIZE - used);
    /* the header's total-bins field grows with it */
    wr32(buf_.data() + REGF_BINS_SIZE_OFF, bins_end + add);
    return new_hbin + HBIN_HEADER_SIZE;
}

void HiveEditor::free_cell_(u32 off) {
    if (off == 0)
        return;
    const u64 pos = u64(REGF_HEADER_SIZE) + u64(off);
    if (pos + 4 > buf_.size())
        return;
    const i32 cs = i32(rd32(buf_.data() + pos));
    if (cs < 0)
        wr32(buf_.data() + pos, u32(-cs)); /* positive = free */
}

void HiveEditor::refresh_header_() {
    /* sequence numbers: bump both (a clean hive carries seq1 == seq2) */
    seq_ = seq_ + 1;
    wr32(buf_.data() + 4, seq_);
    wr32(buf_.data() + 8, seq_);
    /* the XOR checksum over the first 0x1FC dwords, stored at 0x1FC (the
     * parser validates exactly this; Windows' loader does too) */
    u32 acc = 0;
    for (u32 off = 0; off < REGF_CHECKSUM_OFF; off += 4)
        acc ^= rd32(buf_.data() + off);
    wr32(buf_.data() + REGF_CHECKSUM_OFF, acc);
    view_.open(buf_.data(), buf_.size());
}

u32 HiveEditor::set_value(const char* key_path, const char* value_name, u32 type, const u8* data,
                          size_t len) {
    if (len > RegistryHive::kFveMaxHiveValueBytes)
        return ERR_RANGE_OUT_OF_BOUNDS;
    const u32 vk_off = find_vk_(key_path, value_name);
    if (vk_off == 0)
        return ERR_BAD_PARAMETER;                         /* the surgeries rewrite existing values
                                                           * only - an absent target is an honest
                                                           * error, never a silent create */
    u8* vk = buf_.data() + REGF_HEADER_SIZE + vk_off + 4; /* past cell size */
    const u32 old_raw = rd32(vk + 0x04);
    const u32 old_dlen = old_raw & 0x7FFFFFFFu;
    const bool old_inline = old_dlen <= 4;
    const u32 old_doff = rd32(vk + 0x08);

    if (len <= 4) {
        /* inline in the offset dword (bit 31 - the form both the core
         * parser and Windows read) */
        if (!old_inline)
            free_cell_(old_doff);
        u32 inline_word = 0;
        for (size_t i = 0; i < len; ++i)
            inline_word |= u32(data[i]) << (8 * i);
        wr32(vk + 0x04, u32(len) | 0x80000000u);
        wr32(vk + 0x08, inline_word);
        wr32(vk + 0x0C, type);
        refresh_header_();
        return OK;
    }

    /* separate data cell */
    if (!old_inline) {
        /* in-place when the new length fits the old cell's capacity */
        const u64 cpos = u64(REGF_HEADER_SIZE) + u64(old_doff);
        if (cpos + 4 <= buf_.size()) {
            const i32 cs = i32(rd32(buf_.data() + cpos));
            if (cs < 0 && size_t(-cs) >= 4 + len) {
                std::memcpy(buf_.data() + cpos + 4, data, len);
                wr32(vk + 0x04, u32(len));
                wr32(vk + 0x0C, type);
                refresh_header_();
                return OK;
            }
        }
    }
    const u32 new_doff = allocate_cell_(len);
    if (new_doff == 0)
        return ERR_NO_MEMORY;
    /* allocate_cell_ may have GROWN buf_ (a fresh hbin) - the vk pointer
     * must be re-derived from the (possibly reallocated) buffer before
     * any field write */
    vk = buf_.data() + REGF_HEADER_SIZE + vk_off + 4;
    std::memcpy(buf_.data() + REGF_HEADER_SIZE + new_doff + 4, data, len);
    if (!old_inline)
        free_cell_(old_doff);
    wr32(vk + 0x04, u32(len));
    wr32(vk + 0x08, new_doff);
    wr32(vk + 0x0C, type);
    refresh_header_();
    return OK;
}

} // namespace reghive
} // namespace cr
