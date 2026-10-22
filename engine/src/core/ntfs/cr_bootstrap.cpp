/* cr_bootstrap.cpp - NTFS volume bootstrap (R2.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/ntfs/cr_fixup.h"
#include <cstring>

namespace cr {
namespace ntfs {

namespace {
bool power_of_two(u64 v) { return v != 0 && (v & (v - 1)) == 0; }
} // namespace

u32 validate_boot_sector(const BootSector &bs, const Geometry &geom, VolumeParams &out) {
    if (!oem_is_ntfs(bs)) return ERR_BAD_MAGIC;
    if (!power_of_two(bs.bytes_per_sector) || bs.bytes_per_sector < 256 ||
        bs.bytes_per_sector > 4096)
        return ERR_BAD_PARAMETER;
    if (bs.sectors_per_cluster == 0 || !power_of_two(bs.sectors_per_cluster))
        return ERR_BAD_PARAMETER;
    if (bs.bytes_per_sector != geom.logical_sector &&
        bs.bytes_per_sector != 512 /* 512e volumes report 512 in DBR */)
        return ERR_BAD_PARAMETER;

    const u64 cluster_bytes = u64(bs.sectors_per_cluster) * bs.bytes_per_sector;
    if (cluster_bytes == 0) return ERR_BAD_PARAMETER;
    const u64 total_clusters = bs.total_sectors / bs.sectors_per_cluster;
    if (total_clusters == 0) return ERR_BAD_PARAMETER;
    if (bs.mft_lcn >= total_clusters) return ERR_BAD_PARAMETER;
    if (bs.mftmirr_lcn >= total_clusters) return ERR_BAD_PARAMETER;

    u64 record_size = record_size_from_dbr(bs, bs.bytes_per_sector);
    if (record_size == 0 || !power_of_two(record_size)) return ERR_BAD_PARAMETER;
    if (record_size < 0x400) record_size = 0x400; /* corpus clamp */
    if (record_size % bs.bytes_per_sector != 0) return ERR_BAD_PARAMETER;

    out.record_size = record_size;
    out.cluster_bytes = static_cast<u32>(cluster_bytes);
    out.sector_bytes = bs.bytes_per_sector;
    out.total_clusters = total_clusters;
    out.mft_lcn = bs.mft_lcn;
    out.mftmirr_lcn = bs.mftmirr_lcn;
    out.flags = 0;
    return OK;
}

u32 NtfsVolume::load_mft_stream() {
    /* read record 0 with the corpus minimum capacity, fixup, then decode the
     * $DATA runlist of $MFT itself; $ATTRIBUTE_LIST -> fragmented path (F6) */
    std::vector<u8> buf;
    buf.resize(p_.record_size);
    if (dev_->read_at(p_.mft_lcn * p_.cluster_bytes, buf.data(), u32(buf.size())) != OK)
        return ERR_READ_FAILED;

    RecordHeader rh;
    std::memcpy(&rh, buf.data(), sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE) return ERR_BAD_MAGIC;
    u32 rc = fixup_apply(buf.data(), buf.size(), rh.update_sequence_offset, rh.update_sequence_size,
                         p_.sector_bytes);
    if (rc != OK) return rc;

    /* walk attributes of record 0 */
    u32 pos = rh.attribute_offset;
    bool have_data = false;
    bool have_attr_list = false;
    int depth = 0;
    std::vector<u8> attrlist_extra; /* $ATTRIBUTE_LIST content of $MFT */
    const int MAX_ATTR_WALK = 256;

    while (pos + 16 <= buf.size() && depth < MAX_ATTR_WALK) {
        ++depth;
        u32 type = u32_from_bytes_le(buf.data() + pos);
        if (type == ATTR_TYPE_TERMINATOR) break;
        u32 len = u32_from_bytes_le(buf.data() + pos + 4);
        if (len < 16 || pos + len > buf.size()) return ERR_BAD_PARAMETER;
        u8 nonresident = buf[pos + 8];
        u16 name_off = u16_from_bytes_le(buf.data() + pos + 12);

        if (type == ATTR_TYPE_DATA && !nonresident) {
            /* resident $DATA on $MFT: tiny MFT (all records in one run) */
            ResidentAttributeHeader ra;
            std::memcpy(&ra, buf.data() + pos, sizeof(ra));
            if (ra.value_offset + ra.value_length > len) return ERR_BAD_PARAMETER;
            const u8 *runs = buf.data() + pos + ra.value_offset;
            u32 runs_len = ra.value_length;
            RunList rl;
            if (RunList::decode(runs, runs_len, 0, p_.total_clusters, rl) != OK)
                return ERR_BAD_PARAMETER;
            mft_run_ = rl;
            have_data = true;
        } else if (type == ATTR_TYPE_DATA && nonresident) {
            if (pos + 16 + sizeof(NonresidentTail) > buf.size()) return ERR_BAD_PARAMETER;
            NonresidentTail na;
            std::memcpy(&na, buf.data() + pos + 16, sizeof(na));
            if (na.runlist_offset < 0x40 || u32(na.runlist_offset) + 8 > len) return ERR_BAD_PARAMETER;
            const u8 *runs = buf.data() + pos + na.runlist_offset;
            RunList rl;
            if (RunList::decode(runs, len - na.runlist_offset, na.start_vcn,
                                p_.total_clusters, rl) != OK)
                return ERR_BAD_PARAMETER;
            mft_run_ = rl;
            have_data = true;
        } else if (type == ATTR_TYPE_ATTRIBUTE_LIST) {
            /* corpus: "MFT file record has 0x20 attribute" path - keep the
             * content bytes; after the loop we reassemble from child records */
            if (nonresident) {
                return ERR_UNSUPPORTED; /* $MFT's own attr list is resident in
                                           practice; guard rather than hang */
            }
            ResidentAttributeHeader ra;
            std::memcpy(&ra, buf.data() + pos, sizeof(ra));
            if (ra.value_offset + ra.value_length > len) return ERR_BAD_PARAMETER;
            attrlist_extra.assign(buf.begin() + pos + ra.value_offset,
                                  buf.begin() + pos + ra.value_offset + ra.value_length);
            have_attr_list = true;
        }
        pos += len;
        if (name_off > len) return ERR_BAD_PARAMETER; /* sanity on header */
    }
    if (depth >= MAX_ATTR_WALK) return ERR_DEPTH_LIMIT;
    if (!have_data) return ERR_BAD_PARAMETER;

    /* fragmented-MFT reassembly: if $ATTRIBUTE_LIST present, its entries point
     * to the records carrying the remaining $DATA fragments of $MFT (fix F6:
     * strict depth/cycle limit over record numbers). */
    if (have_attr_list && !attrlist_extra.empty()) {
        constexpr u32 FRAG_DEPTH_LIMIT = 64;
        std::vector<u64> visited;
        /* parse $ATTRIBUTE_LIST entries (NTFS spec layout, 26-byte min):
         *   +0x00 u32 type | +0x04 u16 reclen | +0x06 u8 namelen |
         *   +0x07 u8 nonresident | +0x08 u64 lowest_vcn |
         *   +0x10 u64 file_reference (u48 record | u16 seq) | +0x18 u16 id */
        u32 apos = 0;
        int steps = 0;
        while (apos + 26 <= attrlist_extra.size()) {
            if (++steps > int(FRAG_DEPTH_LIMIT)) return ERR_DEPTH_LIMIT;
            u32 atype = u32_from_bytes_le(attrlist_extra.data() + apos);
            u16 areclen = u16_from_bytes_le(attrlist_extra.data() + apos + 4);
            if (areclen < 26 || apos + areclen > attrlist_extra.size()) break;
            u8 anonres = attrlist_extra[apos + 7];
            u64 aref = u64_from_bytes_le(attrlist_extra.data() + apos + 0x10);
            if (atype == ATTR_TYPE_DATA && anonres) {
                u64 child = mft_reference_record(aref);
                for (u64 v : visited)
                    if (v == child) return ERR_CYCLE_LIMIT; /* fix F6/F7 */
                visited.push_back(child);
                if (child < MFT_RESERVED_RECORDS) {
                    /* read child record, extract its nonresident $DATA header
                     * runlist fragment, and append to mft_extents_ view */
                    std::vector<u8> cr;
                    u32 rrc = read_record(child, cr);
                    if (rrc == OK && cr.size() >= sizeof(RecordHeader)) {
                        RecordHeader crh;
                        std::memcpy(&crh, cr.data(), sizeof(crh));
                        u32 cpos = crh.attribute_offset;
                        int cdepth = 0;
                        while (cpos + 16 <= cr.size() && cdepth < 64) {
                            ++cdepth;
                            u32 ctype = u32_from_bytes_le(cr.data() + cpos);
                            if (ctype == ATTR_TYPE_TERMINATOR) break;
                            u32 clen = u32_from_bytes_le(cr.data() + cpos + 4);
                            if (clen < 16 || cpos + clen > cr.size()) break;
                            if (ctype == ATTR_TYPE_DATA && cr[cpos + 8]) {
                                if (cpos + 16 + sizeof(NonresidentTail) > cr.size()) break;
                                NonresidentTail cna;
                                std::memcpy(&cna, cr.data() + cpos + 16, sizeof(cna));
                                RunList crl;
                                if (u32(cna.runlist_offset) + 8 <= clen &&
                                    RunList::decode(cr.data() + cpos + cna.runlist_offset,
                                                    clen - cna.runlist_offset, cna.start_vcn,
                                                    p_.total_clusters, crl) == OK) {
                                    for (const DataRun &dr : crl.runs())
                                        mft_extents_.push_back(dr);
                                }
                            }
                            cpos += clen;
                        }
                    }
                }
            }
            apos += areclen;
        }
    }
    return OK;
}

u32 NtfsVolume::load_mftmirr_stream() {
    std::vector<u8> buf;
    buf.resize(p_.record_size);
    if (dev_->read_at(p_.mftmirr_lcn * p_.cluster_bytes, buf.data(), u32(buf.size())) != OK)
        return ERR_READ_FAILED;
    RecordHeader rh;
    std::memcpy(&rh, buf.data(), sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE) return ERR_BAD_MAGIC;
    u32 rc = fixup_apply(buf.data(), buf.size(), rh.update_sequence_offset, rh.update_sequence_size,
                         p_.sector_bytes);
    if (rc != OK) return rc;
    u32 pos = rh.attribute_offset;
    int depth = 0;
    while (pos + 16 <= buf.size() && depth < 64) {
        ++depth;
        u32 type = u32_from_bytes_le(buf.data() + pos);
        if (type == ATTR_TYPE_TERMINATOR) break;
        u32 len = u32_from_bytes_le(buf.data() + pos + 4);
        if (len < 16 || pos + len > buf.size()) return ERR_BAD_PARAMETER;
        if (type == ATTR_TYPE_DATA && !buf[pos + 8]) {
            ResidentAttributeHeader ra;
            std::memcpy(&ra, buf.data() + pos, sizeof(ra));
            if (ra.value_offset + ra.value_length > len) return ERR_BAD_PARAMETER;
            RunList rl;
            if (RunList::decode(buf.data() + pos + ra.value_offset, ra.value_length, 0,
                                p_.total_clusters, rl) == OK) {
                mirr_run_ = rl;
                mirr_valid_ = true;
            }
        } else if (type == ATTR_TYPE_DATA) {
            if (pos + 16 + sizeof(NonresidentTail) > buf.size()) return ERR_BAD_PARAMETER;
            NonresidentTail na;
            std::memcpy(&na, buf.data() + pos + 16, sizeof(na));
            if (na.runlist_offset < 0x40 && u32(na.runlist_offset) + 8 <= len) {
                RunList rl;
                if (RunList::decode(buf.data() + pos + na.runlist_offset, len - na.runlist_offset,
                                    na.start_vcn, p_.total_clusters, rl) == OK) {
                    mirr_run_ = rl;
                    mirr_valid_ = true;
                }
            }
        }
        pos += len;
    }
    return mirr_valid_ ? OK : ERR_BAD_PARAMETER;
}

std::unique_ptr<NtfsVolume> NtfsVolume::open(std::unique_ptr<IStorageDevice> device, u32 *err) {
    if (!device) {
        if (err) *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    BootSector bs;
    if (device->size() < sizeof(bs)) {
        if (err) *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    if (device->read_at(0, &bs, sizeof(bs)) != OK) {
        if (err) *err = ERR_READ_FAILED;
        return nullptr;
    }
    if (!boot_signature_ok(reinterpret_cast<const u8 *>(&bs))) {
        if (err) *err = ERR_BAD_MAGIC;
        return nullptr;
    }

    auto v = std::unique_ptr<NtfsVolume>(new NtfsVolume());
    v->dev_ = std::move(device);

    u32 rc = validate_boot_sector(bs, v->dev_->geometry(), v->p_);
    if (rc != OK) {
        if (err) *err = rc;
        return nullptr;
    }
    rc = v->load_mft_stream();
    if (rc != OK) {
        /* corpus: $MFTMirr cross-reference as repair/verification source when
         * primary fails. Try mirror's $MFT? The mirror holds records 0..3
         * copies - for bootstrap we only need record 0's runlist; if primary
         * is unreadable there is nothing to mirror FROM - surface the error. */
        if (err) *err = rc;
        return nullptr;
    }
    /* mirror stream is best-effort; its failure is not fatal */
    (void)v->load_mftmirr_stream();
    if (err) *err = OK;
    return v;
}

u32 NtfsVolume::read_record(u64 record_no, std::vector<u8> &out) {
    if (record_no >= max_record_no()) return ERR_RANGE_OUT_OF_BOUNDS;
    out.resize(p_.record_size);
    /* Contiguous fast path first; fragmented-MFT extent view second (F6);
     * $MFTMirr as last resort for records 0..3 (corpus bootstrap). */
    const u64 vcn = record_no * p_.record_size / p_.cluster_bytes;
    const u64 rel = record_no * p_.record_size % p_.cluster_bytes;
    bool sparse = false;
    u64 lcn = 0;
    bool found = mft_run_.vcn_to_lcn(vcn, &lcn, &sparse);
    if (!found) {
        for (const DataRun &dr : mft_extents_) {
            if (vcn >= dr.vcn && vcn < dr.vcn + dr.length) {
                lcn = dr.lcn + (vcn - dr.vcn);
                sparse = dr.sparse;
                found = true;
                break;
            }
        }
    }
    if (!found || sparse) {
        if (record_no < 4 && mirr_valid_) {
            u64 mlcn = 0;
            bool msparse = false;
            if (mirr_run_.vcn_to_lcn(vcn, &mlcn, &msparse) && !msparse)
                return dev_->read_at(mlcn * p_.cluster_bytes + rel, out.data(),
                                     u32(out.size()));
        }
        return ERR_RANGE_OUT_OF_BOUNDS;
    }
    return dev_->read_at(lcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
}

u32 NtfsVolume::read_record_mirr(u64 record_no, std::vector<u8> &out) {
    if (!mirr_valid_ || record_no >= 4) return ERR_UNSUPPORTED;
    out.resize(p_.record_size);
    u64 vcn = record_no * p_.record_size / p_.cluster_bytes;
    u64 lcn = 0;
    bool sparse = false;
    if (!mirr_run_.vcn_to_lcn(vcn, &lcn, &sparse) || sparse)
        return ERR_RANGE_OUT_OF_BOUNDS;
    return dev_->read_at(lcn * p_.cluster_bytes + (record_no * p_.record_size % p_.cluster_bytes),
                         out.data(), u32(out.size()));
}

} // namespace ntfs
} // namespace cr
