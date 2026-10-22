/*
 * cr_attributes.h - MFT record attribute enumeration and typed extraction.
 *
 * Corpus mapping (ntfs deep-dive section 6): attribute types 0x10/0x20/0x30/
 * 0x40/0x80/0x90/0xA0; resident vs non-resident; $ATTRIBUTE_LIST chase with
 * strict depth/cycle limit (fix F6); $FILE_NAME parent refs + namespaces +
 * exact 100ns times; $I30 INDEX_ROOT/ALLOCATION for directory content.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_ATTRIBUTES_H_
#define CR_ATTRIBUTES_H_

#include "cr_bootstrap.h"
#include "cr_runlist.h"
#include "../tree/cr_node.h"
#include <memory>
#include <vector>

namespace cr {
namespace ntfs {

/* One parsed attribute view. Cheap: no copies of values except where noted. */
struct AttrView {
    u32 type = 0;
    u32 record_length = 0;
    bool nonresident = false;
    u16 name_length = 0;       /* u16 chars */
    u16 name_offset = 0;       /* from attribute start */
    u16 flags = 0;             /* compressed/encrypted/sparse */
    u16 attribute_id = 0;
    /* resident */
    u32 value_length = 0;
    u16 value_offset = 0;
    /* nonresident */
    u64 start_vcn = 0;
    u64 last_vcn = 0;
    u16 runlist_offset = 0;
    u64 allocated = 0;
    u64 initialized = 0;
    u64 actual = 0;
    /* resolved location of the attribute header inside the record buffer */
    const u8 *at = nullptr;
    u32 buf_size = 0;          /* record buffer size for bounds checks */
    int buffer_index = 0;     /* which record buffer (0 = primary, >0 = attrlist child) */
};

/* Iterates attributes of a record buffer. Returns count via out vector. */
u32 enum_attributes(const u8 *record, u32 record_size, std::vector<AttrView> &out);

/* $ATTRIBUTE_LIST content: entries pointing at the records that carry the
 * attribute fragments. The ENGINE reads those child records (bounded by
 * depth limit + cycle guard, fix F6) - this parser is pure. */
struct AttrListEntry {
    u32 type = 0;
    u64 lowest_vcn = 0;
    u64 record_ref = 0;      /* raw reference (record | seq<<48) */
    u16 instance = 0;
};
u32 parse_attr_list_entries(const AttrView &list_attr, std::vector<AttrListEntry> &out);

/* Typed extraction from an AttrView. All validate bounds first. */

struct FileNameInfo {
    u64 parent_ref = 0;        /* raw NTFS reference */
    u64 parent_record = 0;
    u16 parent_sequence = 0;
    FileTimes times;           /* exact 100ns (R2.2) */
    u64 allocated_size = 0;
    u64 real_size = 0;
    u32 dos_flags = 0;         /* FILE_NAME flags: hidden/system/directory bits */
    u32 reparse = 0;
    u8 namespace_type = 0;
    std::u16string name;
};
u32 parse_file_name(const AttrView &a, FileNameInfo &out);

struct StandardInfo {
    FileTimes times;
    u32 dos_flags = 0;
    u32 owner_id = 0;
    u32 security_id = 0;
};
u32 parse_standard_information(const AttrView &a, StandardInfo &out);

/* $DATA / $I30 runlist decode (nonresident) or resident payload location. */
u32 parse_data_runs(const AttrView &a, u64 total_clusters, RunList &out);

/* Resident payload span. */
u32 resident_span(const AttrView &a, const u8 *&data, u32 &len);

/* Attribute name (u16 chars) as u16string. */
std::u16string attr_name(const AttrView &a);

/* $INDEX_ROOT: first-entries offset + flags (has $INDEX_ALLOCATION). */
u32 parse_index_root(const AttrView &a, u32 &entries_offset, u32 &index_length,
                     u32 &allocated, u32 &flags);

} // namespace ntfs
} // namespace cr

#endif // CR_ATTRIBUTES_H_
