/* cr_reghive.cpp - read-only regf parser + FVEAutoUnlock harvest (S-014).
 *
 * Every structural offset below is decoded from the og's own RegLib.mo
 * decompile (re-workspace/reversed/ghidra-mo/RegLib_decompiled.c), not from
 * third-party docs:
 *   - regf header: magic 'regf' @+0 (FUN_1800013xx open path :2376), XOR
 *     checksum of the first 0x1FC bytes compared against header+0x1FC
 *     (FUN_180007670 :4814-4853, 8-way-unrolled XOR, canonicalized:
 *     0xFFFFFFFF -> 0xFFFFFFFE, 0 -> 1), root cell offset @+0x24
 *     (FUN_180007620 :4798 reads object+0x44 == header+0x24), total hbin
 *     size @+0x28 (bounds gate FUN_180007700 :4884-4887).
 *   - hbin: 'hbin' magic, offsets relative to the first hbin (file offset
 *     0x1000 + rel), cell offsets 8-aligned and >= 0x20 (FUN_180007700
 *     :4885-4887: offset-0x20 < 0xFFFFFFDF, offset < bins-size,
 *     (offset & 7) == 0).
 *   - cell: [i32 size][record]; record starts at cell+4 (cell+8 when the
 *     header dword @+0x18 == 1 - the old-format stride switch,
 *     FUN_180005ff0 :3767-3770); allocated cells have negative size
 *     (FUN_180007a60 :5048-5050 requires size < 0).
 *   - nk record (offsets from the 'nk' signature; all corpus-decoded):
 *       +0x02 u16 flags (bit 0x20 = ASCII name - name accessor :5001-5021)
 *       +0x14 u32 subkeys count  (FUN_180007b80 :5100 guard; write path
 *                                 FUN_180008030 :5310-5313 increments it)
 *       +0x1C u32 subkeys-list offset (FUN_180007a60 :5046)
 *       +0x24 u32 values count   (FUN_180007c10 :5130 guard)
 *       +0x28 u32 values-list offset (FUN_180007af0 :5066)
 *       +0x48 u16 name length in BYTES; +0x4C name (ASCII or UTF-16LE -
 *           the accessor halves the stored length for unicode names)
 *       minimum cell size 0x50 (:3810)
 *   - vk record: +0x02 u16 name length, +0x04 u32 data length (bit 31 =
 *     inline marker - masked the same way the og does at :997-1005),
 *     +0x08 u32 data offset (data inline in that dword when the real
 *     length <= 4), +0x0C u32 type (RegEnumValueA :1047 reads type there),
 *     +0x10 u16 flags (bit 1 = ASCII name), +0x12 name; minimum cell 0x18
 *     (:3827).
 *   - subkey lists: 'lf' (FastLeaf) / 'lh' (HashLeaf): count u16 @+0x02,
 *     entries {u32 offset, u32 hash} from +0x04; 'li' (IndxLeaf): count
 *     u16 @+0x02, u32 offsets from +0x04; 'ri' (Indirect): same shape as
 *     'li', entries point at further lists (FUN_180005ff0 :3856-3895).
 *   - values list: cell with count u32 offsets to vk records (count from
 *     the nk; SubValList::FUN_180009d30).
 *   - FVE walk: DRWService_decompiled.c :104-140 (live registry: per
 *     volume-GUID subkeys, "Data" REG_BINARY, 0x200 read buffer, _strupr_s
 *     GUID names) and bitlockerhelp_decompiled.c :3128-3160 (offline hive:
 *     "%s\\Windows\\System32\\config\\SYSTEM", then the LITERAL
 *     "\\ControlSet001\\Control\\FVEAutoUnlock\\" path - ControlSet001
 *     hardcoded, no Select consultation - then per-subkey "%s%s\\Data").
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/reghive/cr_reghive.h"
#include <cstring>

namespace cr {
namespace reghive {

namespace {

constexpr u32 SIG_REGF = 0x66676572u; /* 'regf' LE */
constexpr u16 SIG_NK = 0x6b6e;        /* 'nk' */
constexpr u16 SIG_VK = 0x6b76;        /* 'vk' */
constexpr u16 SIG_LF = 0x666c;        /* 'lf' */
constexpr u16 SIG_LH = 0x686c;        /* 'lh' */
constexpr u16 SIG_LI = 0x696c;        /* 'li' */
constexpr u16 SIG_RI = 0x6972;        /* 'ri' */

constexpr u32 REG_TYPE_BINARY = 3;

u16 rd16(const u8* p) {
    return u16(u16(p[0]) | (u16(p[1]) << 8));
}
u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}

/* og FUN_180007670: XOR of the first 0x1FC bytes' dwords, canonicalized. */
u32 header_checksum(const u8* base) {
    u32 acc = 0;
    for (u32 off = 0; off < REGF_CHECKSUM_OFF; off += 4)
        acc ^= rd32(base + off);
    if (acc == 0xFFFFFFFFu)
        return 0xFFFFFFFEu;
    if (acc == 0)
        return 1;
    return acc;
}

char fold(char c) {
    if (c >= 'A' && c <= 'Z')
        return char(c - 'A' + 'a');
    return c;
}

bool name_equals(const std::string& a, const char* b) {
    if (b == nullptr)
        return false;
    const size_t n = std::strlen(b);
    if (a.size() != n)
        return false;
    for (size_t i = 0; i < n; ++i)
        if (fold(a[i]) != fold(b[i]))
            return false;
    return true;
}

} // namespace

/* ------------------------------------------------------------- hive open */

bool RegistryHive::open(const u8* data, size_t size) {
    if (data == nullptr || size < REGF_HEADER_SIZE)
        return false;
    if (rd32(data) != SIG_REGF)
        return false;
    /* og: FUN_180007670(header) == header[+0x1FC] (RegLib :2376-2377) */
    if (header_checksum(data) != rd32(data + REGF_CHECKSUM_OFF))
        return false;
    const u32 bins = rd32(data + REGF_BINS_SIZE_OFF);
    const u64 want = u64(REGF_HEADER_SIZE) + u64(bins);
    if (bins == 0 || want > size)
        return false;
    const u32 root = rd32(data + REGF_ROOT_CELL_OFF);
    if (root == 0xFFFFFFFFu)
        return false;
    /* the og's cell gate (FUN_180007700): >= 0x20, < bins-size, 8-aligned */
    if (root < HBIN_HEADER_SIZE || root >= bins || (root & (REGF_CELL_ALIGN - 1)) != 0)
        return false;
    /* og stride switch (FUN_180005ff0 :3767): 8 when header+0x18 == 1 */
    base_ = data;
    size_ = size;
    bins_end_ = bins;
    root_cell_ = root;
    cell_stride_ = rd32(data + 0x18) == 1 ? 8 : 4;
    const u8* rec = nullptr;
    i32 csize = 0;
    if (!cell_at(root, rec, csize))
        return false;
    if (rd16(rec) != SIG_NK)
        return false;
    return true;
}

/* ---------------------------------------------------------------- cells */

bool RegistryHive::cell_at(u32 offset, const u8*& rec, i32& cell_size) const {
    if (offset < HBIN_HEADER_SIZE)
        return false;
    if (offset >= bins_end_)
        return false;
    if ((offset & (REGF_CELL_ALIGN - 1)) != 0)
        return false;
    const u64 pos = u64(REGF_HEADER_SIZE) + u64(offset);
    if (pos + 4 > size_)
        return false;
    const i32 sz = i32(rd32(base_ + pos));
    if (sz >= 0) /* free cell - the og requires size < 0 (allocated) */
        return false;
    const u32 abs_size = u32(-i64(sz));
    if (abs_size < cell_stride_)
        return false;
    if (pos + cell_stride_ > size_)
        return false;
    /* record length available within the buffer (may be shorter than the
     * declared cell when the hive is truncated - every reader below
     * bounds-checks against size_, never against abs_size) */
    (void)abs_size;
    rec = base_ + pos + cell_stride_;
    cell_size = sz;
    return true;
}

/* ---------------------------------------------------------------- names */

std::string RegistryHive::nk_name(const u8* nk) const {
    const u16 flags = rd16(nk + 0x02);
    const u16 len_bytes = rd16(nk + 0x48);
    const bool ascii = (flags & 0x20) != 0;
    /* name bytes live at nk+0x4C; the buffer bound is size_, checked by the
     * caller via the cell window - clamp to what the hive actually holds */
    const u64 name_pos = u64(nk - base_) + 0x4C;
    if (name_pos + len_bytes > size_)
        return {};
    std::string out;
    if (ascii) {
        out.assign(reinterpret_cast<const char*>(nk + 0x4C), len_bytes);
    } else {
        const size_t chars = len_bytes / 2;
        out.reserve(chars);
        for (size_t i = 0; i < chars; ++i) {
            const u16 wc = rd16(nk + 0x4C + i * 2);
            /* registry key names in the SYSTEM hive family are ASCII-range;
             * fold anything wider to '?' rather than failing the walk (the
             * og compares via its own unicode compare - same acceptance) */
            out.push_back(wc < 0x80 ? char(wc) : '?');
        }
    }
    return out;
}

/* -------------------------------------------------------- list plumbing */

bool RegistryHive::nk_subkeys_list(u32 nk_off, u32& list_off) const {
    const u8* rec = nullptr;
    i32 cs = 0;
    if (!cell_at(nk_off, rec, cs))
        return false;
    if (rd16(rec) != SIG_NK) {
        return false;
    }
    list_off = rd32(rec + 0x1C);
    return list_off != 0xFFFFFFFFu && list_off != 0;
}

bool RegistryHive::nk_values_list(u32 nk_off, u32& list_off, u32& count) const {
    const u8* rec = nullptr;
    i32 cs = 0;
    if (!cell_at(nk_off, rec, cs))
        return false;
    if (rd16(rec) != SIG_NK) {
        return false;
    }
    count = rd32(rec + 0x24);
    list_off = rd32(rec + 0x28);
    return true;
}

bool RegistryHive::collect_list_offsets(u32 list_off, std::vector<u32>& out, u32 depth) const {
    if (depth > 8 || out.size() >= 4096)
        return false; /* cyclic/absurd lists: refuse, stay read-only */
    const u8* rec = nullptr;
    i32 cs = 0;
    if (!cell_at(list_off, rec, cs))
        return false;
    const u16 sig = rd16(rec);
    if (sig != SIG_LF && sig != SIG_LH && sig != SIG_LI && sig != SIG_RI)
        return false;
    const u16 count = rd16(rec + 0x02);
    /* entries start at +0x04; lf/lh stride 8 (offset+hash), li/ri stride 4 */
    const u32 stride = (sig == SIG_LF || sig == SIG_LH) ? 8 : 4;
    const u64 need = u64(4) + u64(count) * stride;
    if (need > 0x100000) /* cap list reads at 1 MiB of entries */
        return false;
    for (u16 i = 0; i < count; ++i) {
        const u64 entry_pos = u64(rec - base_) + 4 + u64(i) * stride;
        if (entry_pos + 4 > size_)
            return false;
        const u32 off = rd32(base_ + entry_pos);
        if (off == 0xFFFFFFFFu || off == 0)
            continue;
        if (sig == SIG_RI) {
            if (!collect_list_offsets(off, out, depth + 1))
                continue;
        } else {
            out.push_back(off);
        }
    }
    return true;
}

/* ----------------------------------------------------------------- walk */

u64 RegistryHive::find_subkey(u64 parent, const char* name) const {
    if (base_ == nullptr || parent == 0 || name == nullptr || name[0] == '\0')
        return 0;
    const u32 cur = u32(parent - 1);
    u32 list_off = 0;
    if (!nk_subkeys_list(cur, list_off))
        return 0;
    std::vector<u32> children;
    if (!collect_list_offsets(list_off, children, 0))
        return 0;
    for (u32 child : children) {
        const u8* rec = nullptr;
        i32 cs = 0;
        if (!cell_at(child, rec, cs))
            continue;
        if (rd16(rec) != SIG_NK)
            continue;
        if (name_equals(nk_name(rec), name))
            return u64(child) + 1;
    }
    return 0;
}

u64 RegistryHive::find_key(const char* path) const {
    if (base_ == nullptr || path == nullptr)
        return 0;
    u64 cur = u64(root_cell_) + 1;
    const char* p = path;
    while (*p == '\\' || *p == '/')
        ++p;
    while (*p != '\0') {
        const char* sep = p;
        while (*sep != '\0' && *sep != '\\' && *sep != '/')
            ++sep;
        const std::string want(p, size_t(sep - p));
        const u64 next = find_subkey(cur, want.c_str());
        if (next == 0)
            return 0;
        cur = next;
        p = (*sep == '\0') ? sep : sep + 1;
    }
    return cur;
}

bool RegistryHive::enum_subkeys(u64 key, std::vector<std::string>& out) const {
    if (base_ == nullptr || key == 0)
        return false;
    const u32 nk_off = u32(key - 1);
    u32 list_off = 0;
    if (!nk_subkeys_list(nk_off, list_off))
        return true; /* no subkeys: valid, empty */
    std::vector<u32> children;
    if (!collect_list_offsets(list_off, children, 0))
        return false;
    for (u32 child : children) {
        const u8* rec = nullptr;
        i32 cs = 0;
        if (!cell_at(child, rec, cs))
            continue;
        if (rd16(rec) != SIG_NK)
            continue;
        out.push_back(nk_name(rec));
    }
    return true;
}

bool RegistryHive::read_value(u64 key, const char* name, u32& out_type,
                              std::vector<u8>& out_data) const {
    if (base_ == nullptr || key == 0 || name == nullptr)
        return false;
    const u32 nk_off = u32(key - 1);
    u32 list_off = 0;
    u32 count = 0;
    if (!nk_values_list(nk_off, list_off, count))
        return false;
    if (count == 0 || list_off == 0 || list_off == 0xFFFFFFFFu)
        return false;
    if (count > 0x10000)
        return false; /* absurd; refuse */
    /* values list: cell with `count` u32 vk offsets after the size dword */
    const u64 pos = u64(REGF_HEADER_SIZE) + u64(list_off) + 4;
    if (pos + u64(count) * 4 > size_)
        return false;
    for (u32 i = 0; i < count; ++i) {
        const u32 vk_off = rd32(base_ + pos + u64(i) * 4);
        if (vk_off == 0 || vk_off == 0xFFFFFFFFu)
            continue;
        const u8* vk = nullptr;
        i32 cs = 0;
        if (!cell_at(vk_off, vk, cs))
            continue;
        if (rd16(vk) != SIG_VK)
            continue;
        /* vk name: +0x02 u16 len, +0x10 u16 flags (bit 1 = ASCII), +0x12 */
        const u16 nlen = rd16(vk + 0x02);
        const u16 vflags = rd16(vk + 0x10);
        const bool ascii_name = (vflags & 0x01) != 0;
        std::string vn;
        const u64 npos = u64(vk - base_) + 0x12;
        if (npos + nlen > size_)
            continue;
        if (ascii_name) {
            vn.assign(reinterpret_cast<const char*>(vk + 0x12), nlen);
        } else {
            const size_t chars = nlen / 2;
            vn.reserve(chars);
            for (size_t c = 0; c < chars; ++c)
                vn.push_back(char(rd16(vk + 0x12 + c * 2) & 0x7F));
        }
        if (!name_equals(vn, name))
            continue;

        out_type = rd32(vk + 0x0C);
        u32 dlen = rd32(vk + 0x04) & 0x7FFFFFFFu; /* og inline-bit mask */
        if (dlen == 0) {
            out_data.clear();
            return true;
        }
        if (dlen > kFveMaxHiveValueBytes)
            return false; /* big-data class: never an FVE key; refuse */
        if (dlen <= 4) {
            /* data inline in the offset dword (the og reads it the same way
             * through its Val accessor) */
            out_data.assign(vk + 0x08, vk + 0x08 + dlen);
            return true;
        }
        const u32 doff = rd32(vk + 0x08);
        if (doff == 0 || doff == 0xFFFFFFFFu)
            return false;
        const u8* cell = nullptr;
        i32 dcs = 0;
        if (!cell_at(doff, cell, dcs))
            return false;
        const u64 dpos = u64(cell - base_);
        if (dpos + dlen > size_)
            return false;
        out_data.assign(cell, cell + dlen);
        return true;
    }
    return false;
}

/* ------------------------------------------------------------ FVE harvest */

size_t harvest_fve_auto_unlock(const RegistryHive& hive, std::vector<AutoUnlockKey>& out) {
    /* bitlockerhelp_decompiled.c :3140-3146: the LITERAL
     * "\\ControlSet001\\Control\\FVEAutoUnlock\\" - ControlSet001 is
     * hardcoded in the og (no Select consultation). */
    const u64 fve = hive.find_key("\\ControlSet001\\Control\\FVEAutoUnlock");
    if (fve == 0)
        return 0;
    std::vector<std::string> guids;
    if (!hive.enum_subkeys(fve, guids))
        return 0;
    size_t harvested = 0;
    for (const std::string& guid : guids) {
        if (out.size() >= kFveMaxRecords)
            break; /* corpus cap: <= 100 records */
        if (guid.empty() || guid.size() > kFveGuidMaxChars)
            continue; /* corpus cap: 50-char GUID field */
        /* per-subkey "Data" value (bitlockerhelp :3160 "%s%s\\Data") */
        const u64 sub = hive.find_subkey(fve, guid.c_str());
        if (sub == 0)
            continue;
        u32 type = 0;
        std::vector<u8> data;
        if (!hive.read_value(sub, "Data", type, data))
            continue;
        if (type != REG_TYPE_BINARY)
            continue; /* the og reads REG_BINARY blobs only */
        AutoUnlockKey k;
        k.volume_guid = guid;
        if (data.size() > kFveMaxBlobBytes)
            data.resize(kFveMaxBlobBytes); /* corpus cap: memcpy(Data, min(size, 0x100)) */
        if (data.size() < 32)
            continue; /* the old core's own >=32B floor was right: a BEK-class
                         blob is at least this big; protects against decoys */
        k.blob = std::move(data);
        out.push_back(std::move(k));
        ++harvested;
    }
    return harvested;
}

bool guid_equals_ci(const std::string& a, const char* b) {
    return name_equals(a, b);
}

std::vector<const AutoUnlockKey*>
select_auto_unlock_attempts(const std::vector<AutoUnlockKey>& keys, const char* target_guid) {
    std::vector<const AutoUnlockKey*> out;
    out.reserve(keys.size());
    const AutoUnlockKey* match = nullptr;
    for (const AutoUnlockKey& k : keys) {
        if (target_guid != nullptr && target_guid[0] != '\0' &&
            guid_equals_ci(k.volume_guid, target_guid)) {
            match = &k;
            break;
        }
    }
    if (match != nullptr)
        out.push_back(match);
    for (const AutoUnlockKey& k : keys) {
        if (&k == match)
            continue;
        out.push_back(&k);
    }
    return out;
}

} // namespace reghive
} // namespace cr
