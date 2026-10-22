/* cr_attributes.cpp - attribute enumeration and typed extraction (R2.2, R2.3).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_attributes.h"
#include <cstring>

namespace cr {
namespace ntfs {

u32 enum_attributes(const u8 *record, u32 record_size, std::vector<AttrView> &out) {
    out.clear();
    if (record == nullptr || record_size < 0x40) return ERR_BAD_PARAMETER;
    RecordHeader rh;
    std::memcpy(&rh, record, sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE) return ERR_BAD_MAGIC;
    u32 pos = rh.attribute_offset;
    if (pos < 0x30 || pos >= record_size) return ERR_BAD_PARAMETER;
    constexpr int MAX_ATTRS = 64; /* real records carry < 20 */
    for (int i = 0; i < MAX_ATTRS; ++i) {
        if (pos + 16 > record_size) return ERR_BAD_PARAMETER;
        u32 type = u32_from_bytes_le(record + pos);
        if (type == ATTR_TYPE_TERMINATOR) break;
        u32 len = u32_from_bytes_le(record + pos + 4);
        if (len < 16 || pos + len > record_size) return ERR_BAD_PARAMETER;

        AttrView a;
        a.type = type;
        a.record_length = len;
        a.nonresident = record[pos + 8] != 0;
        a.name_length = record[pos + 9];
        a.name_offset = u16_from_bytes_le(record + pos + 10);
        a.flags = u16_from_bytes_le(record + pos + 12);
        a.attribute_id = u16_from_bytes_le(record + pos + 14);
        a.at = record + pos;
        a.buf_size = record_size;
        if (!a.nonresident) {
            if (pos + 24 > record_size) return ERR_BAD_PARAMETER;
            a.value_length = u32_from_bytes_le(record + pos + 16);
            a.value_offset = u16_from_bytes_le(record + pos + 20);
            if (u32(a.value_offset) + a.value_length > len) return ERR_BAD_PARAMETER;
        } else {
            if (pos + 64 > record_size) return ERR_BAD_PARAMETER;
            a.start_vcn = u64_from_bytes_le(record + pos + 16);
            a.last_vcn = u64_from_bytes_le(record + pos + 24);
            a.runlist_offset = u16_from_bytes_le(record + pos + 32);
            a.allocated = u64_from_bytes_le(record + pos + 40);
            a.initialized = u64_from_bytes_le(record + pos + 48);
            a.actual = u64_from_bytes_le(record + pos + 56);
            if (a.runlist_offset < 0x40 || u32(a.runlist_offset) + 8 > len)
                return ERR_BAD_PARAMETER;
        }
        if (a.name_length > 0 && (u32(a.name_offset) < 24 || u32(a.name_offset) + a.name_length * 2 > len))
            return ERR_BAD_PARAMETER;
        out.push_back(a);
        if (len == 0) return ERR_BAD_PARAMETER;
        pos += len;
    }
    return OK;
}

u32 parse_attr_list_entries(const AttrView &list_attr, std::vector<AttrListEntry> &out) {
    out.clear();
    const u8 *d = nullptr;
    u32 dlen = 0;
    u32 rc = resident_span(list_attr, d, dlen);
    if (rc != OK) return rc;
    u32 pos = 0;
    constexpr int MAX_ENTRIES = 512;
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (pos + 26 > dlen) break;
        u32 type = u32_from_bytes_le(d + pos);
        u16 reclen = u16_from_bytes_le(d + pos + 4);
        if (reclen < 26 || pos + reclen > dlen) break;
        AttrListEntry e;
        e.type = type;
        e.lowest_vcn = u64_from_bytes_le(d + pos + 8);
        e.record_ref = u64_from_bytes_le(d + pos + 0x10);
        e.instance = u16_from_bytes_le(d + pos + 0x18);
        out.push_back(e);
        if (reclen == 0) break;
        pos += reclen;
    }
    return OK;
}

std::u16string attr_name(const AttrView &a) {
    std::u16string s;
    if (a.name_length == 0 || a.at == nullptr) return s;
    if (u32(a.name_offset) + u32(a.name_length) * 2 > a.record_length) return s;
    s.resize(a.name_length);
    for (u16 i = 0; i < a.name_length; ++i) {
        s[i] = u16_from_bytes_le(a.at + a.name_offset + u32(i) * 2);
    }
    return s;
}

u32 resident_span(const AttrView &a, const u8 *&data, u32 &len) {
    if (a.nonresident || a.at == nullptr) return ERR_BAD_PARAMETER;
    if (u32(a.value_offset) + a.value_length > a.record_length) return ERR_BAD_PARAMETER;
    data = a.at + a.value_offset;
    len = a.value_length;
    return OK;
}

u32 parse_file_name(const AttrView &a, FileNameInfo &out) {
    const u8 *d = nullptr;
    u32 dlen = 0;
    u32 rc = resident_span(a, d, dlen);
    if (rc != OK) return rc;
    if (dlen < 66) return ERR_TRUNCATED;
    FileNameInfo f;
    f.parent_ref = u64_from_bytes_le(d);
    f.parent_record = mft_reference_record(f.parent_ref);
    f.parent_sequence = u16(f.parent_ref >> 48);
    f.times.created = u64_from_bytes_le(d + 8);
    f.times.modified = u64_from_bytes_le(d + 16);
    f.times.changed = u64_from_bytes_le(d + 24);
    f.times.accessed = u64_from_bytes_le(d + 32);
    f.allocated_size = u64_from_bytes_le(d + 40);
    f.real_size = u64_from_bytes_le(d + 48);
    f.dos_flags = u32_from_bytes_le(d + 56);
    f.reparse = u32_from_bytes_le(d + 60);
    f.namespace_type = d[65];       /* +0x41 */
    u8 nlen = d[64];                /* +0x40 name length */
    if (u32(66) + u32(nlen) * 2 > dlen) return ERR_TRUNCATED;
    f.name.resize(nlen);
    for (u8 i = 0; i < nlen; ++i)
        f.name[i] = u16_from_bytes_le(d + 66 + u32(i) * 2);
    if (f.name.empty()) return ERR_BAD_PARAMETER;
    out = f;
    return OK;
}

u32 parse_standard_information(const AttrView &a, StandardInfo &out) {
    const u8 *d = nullptr;
    u32 dlen = 0;
    u32 rc = resident_span(a, d, dlen);
    if (rc != OK) return rc;
    if (dlen < 72) return ERR_TRUNCATED;
    StandardInfo s;
    s.times.created = u64_from_bytes_le(d);
    s.times.modified = u64_from_bytes_le(d + 8);
    s.times.changed = u64_from_bytes_le(d + 16);
    s.times.accessed = u64_from_bytes_le(d + 24);
    s.dos_flags = u32_from_bytes_le(d + 32);
    s.owner_id = u32_from_bytes_le(d + 48);
    s.security_id = u32_from_bytes_le(d + 52);
    out = s;
    return OK;
}

u32 parse_data_runs(const AttrView &a, u64 total_clusters, RunList &out) {
    if (!a.nonresident || a.at == nullptr) return ERR_BAD_PARAMETER;
    /* runlist_offset is from attribute start; decode the span */
    if (u32(a.runlist_offset) + 8 > a.record_length) return ERR_BAD_PARAMETER;
    return RunList::decode(a.at + a.runlist_offset, a.record_length - a.runlist_offset,
                           a.start_vcn, total_clusters, out);
}

u32 parse_index_root(const AttrView &a, u32 &entries_offset, u32 &index_length,
                     u32 &allocated, u32 &flags) {
    const u8 *d = nullptr;
    u32 dlen = 0;
    u32 rc = resident_span(a, d, dlen);
    if (rc != OK) return rc;
    if (dlen < 0x20) return ERR_TRUNCATED;
    /* $INDEX_ROOT value: type u32, collation u32, block_size u32, per_block u8,
     * unused[3], then IndexHeader at +0x10 */
    entries_offset = u32_from_bytes_le(d + 0x10);
    index_length = u32_from_bytes_le(d + 0x14);
    allocated = u32_from_bytes_le(d + 0x18);
    flags = u32_from_bytes_le(d + 0x1C);
    if (entries_offset < 0x20 || entries_offset > dlen) return ERR_BAD_PARAMETER;
    return OK;
}

} // namespace ntfs
} // namespace cr
