/* cr_bootstrap.cpp - NTFS volume bootstrap (R2.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_bootstrap.h"
#include <algorithm>
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/ntfs/cr_fixup.h"
#include <cstring>

namespace cr {
namespace ntfs {

namespace {
bool power_of_two(u64 v) {
    return v != 0 && (v & (v - 1)) == 0;
}
} // namespace

u32 validate_boot_sector(const BootSector& bs, const Geometry& geom, VolumeParams& out) {
    if (!oem_is_ntfs(bs))
        return ERR_BAD_MAGIC;
    if (!power_of_two(bs.bytes_per_sector) || bs.bytes_per_sector < 256 ||
        bs.bytes_per_sector > 4096)
        return ERR_BAD_PARAMETER;
    if (bs.sectors_per_cluster == 0 || !power_of_two(bs.sectors_per_cluster))
        return ERR_BAD_PARAMETER;
    if (bs.bytes_per_sector != geom.logical_sector &&
        bs.bytes_per_sector != 512 /* 512e volumes report 512 in DBR */)
        return ERR_BAD_PARAMETER;

    const u64 cluster_bytes = u64(bs.sectors_per_cluster) * bs.bytes_per_sector;
    if (cluster_bytes == 0)
        return ERR_BAD_PARAMETER;
    const u64 total_clusters = bs.total_sectors / bs.sectors_per_cluster;
    if (total_clusters == 0)
        return ERR_BAD_PARAMETER;
    /* P1-3: the declared volume must fit the actual device (2x slack for
     * shrink-tolerant snapshots); unclamped, a bogus total_sectors feeds
     * multi-GiB allocations and billion-iteration walks downstream */
    if (geom.total_bytes != 0 && bs.total_sectors * u64(bs.bytes_per_sector) > geom.total_bytes * 2)
        return ERR_BAD_PARAMETER;
    if (bs.mft_lcn >= total_clusters)
        return ERR_BAD_PARAMETER;
    if (bs.mftmirr_lcn >= total_clusters)
        return ERR_BAD_PARAMETER;

    u64 record_size = record_size_from_dbr(bs, bs.bytes_per_sector);
    if (record_size == 0 || !power_of_two(record_size))
        return ERR_BAD_PARAMETER;
    if (record_size < 0x400)
        record_size = 0x400; /* corpus clamp */
    if (record_size % bs.bytes_per_sector != 0)
        return ERR_BAD_PARAMETER;

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
    if (rh.magic != RECORD_MAGIC_FILE)
        return ERR_BAD_MAGIC;
    u32 rc = fixup_apply(buf.data(), buf.size(), rh.update_sequence_offset, rh.update_sequence_size,
                         p_.sector_bytes);
    if (rc != OK)
        return rc;

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
        if (type == ATTR_TYPE_TERMINATOR)
            break;
        u32 len = u32_from_bytes_le(buf.data() + pos + 4);
        if (len < 16 || pos + len > buf.size())
            return ERR_BAD_PARAMETER;
        u8 nonresident = buf[pos + 8];
        u16 name_off = u16_from_bytes_le(buf.data() + pos + 12);

        if (type == ATTR_TYPE_DATA && !nonresident) {
            /* resident $DATA on $MFT: tiny MFT (all records in one run) */
            ResidentAttributeHeader ra;
            std::memcpy(&ra, buf.data() + pos, sizeof(ra));
            if (u64(ra.value_offset) + u64(ra.value_length) > u64(len))
                return ERR_BAD_PARAMETER;
            const u8* runs = buf.data() + pos + ra.value_offset;
            u32 runs_len = ra.value_length;
            RunList rl;
            if (RunList::decode(runs, runs_len, 0, p_.total_clusters, rl) != OK)
                return ERR_BAD_PARAMETER;
            mft_run_ = rl;
            have_data = true;
        } else if (type == ATTR_TYPE_DATA && nonresident) {
            if (pos + 16 + sizeof(NonresidentTail) > buf.size())
                return ERR_BAD_PARAMETER;
            NonresidentTail na;
            std::memcpy(&na, buf.data() + pos + 16, sizeof(na));
            if (na.runlist_offset < 0x40 || u32(na.runlist_offset) + 8 > len)
                return ERR_BAD_PARAMETER;
            const u8* runs = buf.data() + pos + na.runlist_offset;
            RunList rl;
            if (RunList::decode(runs, len - na.runlist_offset, na.start_vcn, p_.total_clusters,
                                rl) != OK)
                return ERR_BAD_PARAMETER;
            mft_run_ = rl;
            have_data = true;
        } else if (type == ATTR_TYPE_ATTRIBUTE_LIST) {
            /* corpus: "MFT file record has 0x20 attribute" path - keep the
             * content bytes; after the loop we reassemble from child records.
             * Nonresident lists are legal (huge, fragmented $MFTs) - the og
             * chases them (FUN_18001a540 fragmented-MFT handler); decode the
             * runlist and read the stream, bounded (attrlists stay small even
             * when nonresident - 1 MiB cap against corrupt size fields). */
            if (nonresident) {
                if (pos + 16 + sizeof(NonresidentTail) > buf.size())
                    return ERR_BAD_PARAMETER;
                NonresidentTail na;
                std::memcpy(&na, buf.data() + pos + 16, sizeof(na));
                if (na.runlist_offset < 0x40 || u32(na.runlist_offset) + 8 > len)
                    return ERR_BAD_PARAMETER;
                RunList rl;
                if (RunList::decode(buf.data() + pos + na.runlist_offset, len - na.runlist_offset,
                                    na.start_vcn, p_.total_clusters, rl) != OK)
                    return ERR_BAD_PARAMETER;
                u64 want = na.actual != 0 ? na.actual : na.allocated;
                if (want == 0 || want > (1ull << 20))
                    return ERR_BAD_PARAMETER;
                attrlist_extra.assign(size_t(want), 0);
                u64 got = 0;
                for (const DataRun& dr : rl.runs()) {
                    if (got >= want)
                        break;
                    u64 run_bytes = dr.length * p_.cluster_bytes;
                    if (run_bytes == 0)
                        continue;
                    if (!dr.sparse) {
                        const u64 take = run_bytes < want - got ? run_bytes : want - got;
                        if (dev_->read_at(dr.lcn * p_.cluster_bytes, attrlist_extra.data() + got,
                                          u32(take)) != OK)
                            return ERR_READ_FAILED;
                        got += take;
                    } else {
                        /* sparse region: zeros (the vector is zero-initialized) */
                        got += run_bytes < want - got ? run_bytes : want - got;
                    }
                }
                attrlist_extra.resize(size_t(got));
                if (attrlist_extra.empty())
                    return ERR_BAD_PARAMETER;
                have_attr_list = true;
            } else {
                ResidentAttributeHeader ra;
                std::memcpy(&ra, buf.data() + pos, sizeof(ra));
                if (u64(ra.value_offset) + u64(ra.value_length) > u64(len))
                    return ERR_BAD_PARAMETER;
                attrlist_extra.assign(buf.begin() + pos + ra.value_offset,
                                      buf.begin() + pos + ra.value_offset + ra.value_length);
                have_attr_list = true;
            }
        }
        pos += len;
        if (name_off > len)
            return ERR_BAD_PARAMETER; /* sanity on header */
    }
    if (depth >= MAX_ATTR_WALK)
        return ERR_DEPTH_LIMIT;
    if (!have_data)
        return ERR_BAD_PARAMETER;

    /* fragmented-MFT reassembly: if $ATTRIBUTE_LIST present, its entries point
     * to the records carrying the remaining $DATA fragments of $MFT (fix F6:
     * strict depth/cycle limit over record numbers). Entries point at
     * extension records of arbitrary number (real volumes put them well past
     * the reserved range) - reachability is bounded by read_record itself
     * plus the visited set; there is no record-number cutoff. */
    if (have_attr_list && !attrlist_extra.empty()) {
        constexpr u32 FRAG_ENTRY_LIMIT = 65536; /* buffer is <= 1 MiB anyway */
        std::vector<u64> visited;
        /* parse $ATTRIBUTE_LIST entries (NTFS spec layout, 26-byte min,
         * 8-byte aligned stride):
         *   +0x00 u32 type | +0x04 u16 reclen | +0x06 u8 namelen |
         *   +0x07 u8 nonresident | +0x08 u64 lowest_vcn |
         *   +0x10 u64 file_reference (u48 record | u16 seq) | +0x18 u16 id */
        struct Frag {
            u64 lowest_vcn = 0;
            std::vector<DataRun> runs;
        };
        std::vector<Frag> frags;
        u32 apos = 0;
        u64 steps = 0;
        while (apos + 26 <= attrlist_extra.size()) {
            if (++steps > FRAG_ENTRY_LIMIT)
                break; /* corrupt stream: keep what parsed, never fail open */
            u32 atype = u32_from_bytes_le(attrlist_extra.data() + apos);
            u16 areclen = u16_from_bytes_le(attrlist_extra.data() + apos + 4);
            if (areclen < 26 || apos + areclen > attrlist_extra.size())
                break;
            u8 anonres = attrlist_extra[apos + 7];
            u64 aref = u64_from_bytes_le(attrlist_extra.data() + apos + 0x10);
            u64 alowest = u64_from_bytes_le(attrlist_extra.data() + apos + 0x08);
            if (atype == ATTR_TYPE_DATA && anonres) {
                u64 child = mft_reference_record(aref);
                bool seen = false;
                for (u64 v : visited)
                    if (v == child) {
                        seen = true;
                        break;
                    }
                if (!seen) {
                    visited.push_back(child);
                    /* read child record, apply the USA fixup (the on-disk
                     * image carries sector-tail USNs; parsing raw corrupts
                     * runlists crossing the 510/1022 boundaries), extract
                     * its nonresident $DATA runlist fragment */
                    std::vector<u8> cr;
                    u32 rrc = read_record(child, cr);
                    if (rrc == OK && cr.size() >= sizeof(RecordHeader)) {
                        RecordHeader crh;
                        std::memcpy(&crh, cr.data(), sizeof(crh));
                        if (fixup_apply(cr.data(), u32(cr.size()), crh.update_sequence_offset,
                                        crh.update_sequence_size, p_.sector_bytes) == OK) {
                            u32 cpos = crh.attribute_offset;
                            int cdepth = 0;
                            while (cpos + 16 <= cr.size() && cdepth < 64) {
                                ++cdepth;
                                u32 ctype = u32_from_bytes_le(cr.data() + cpos);
                                if (ctype == ATTR_TYPE_TERMINATOR)
                                    break;
                                u32 clen = u32_from_bytes_le(cr.data() + cpos + 4);
                                if (clen < 16 || cpos + clen > cr.size())
                                    break;
                                if (ctype == ATTR_TYPE_DATA && cr[cpos + 8]) {
                                    if (cpos + 16 + sizeof(NonresidentTail) > cr.size())
                                        break;
                                    NonresidentTail cna;
                                    std::memcpy(&cna, cr.data() + cpos + 16, sizeof(cna));
                                    RunList crl;
                                    if (u32(cna.runlist_offset) + 8 <= clen &&
                                        RunList::decode(cr.data() + cpos + cna.runlist_offset,
                                                        clen - cna.runlist_offset, cna.start_vcn,
                                                        p_.total_clusters, crl) == OK) {
                                        Frag f;
                                        f.lowest_vcn = alowest;
                                        f.runs = crl.runs();
                                        frags.push_back(std::move(f));
                                    }
                                }
                                cpos += clen;
                            }
                        }
                    }
                }
            }
            apos += u32(areclen + 7) & ~u32(7); /* entries are 8-byte aligned */
        }
        /* merge in lowest_vcn order: each fragment's runlist is already
         * VCN-relative; ordering by the entry's lowest_vcn yields the true
         * $MFT extent view (discovery order is arbitrary) */
        std::sort(frags.begin(), frags.end(),
                  [](const Frag& a, const Frag& b) { return a.lowest_vcn < b.lowest_vcn; });
        for (const Frag& f : frags)
            for (const DataRun& dr : f.runs)
                mft_extents_.push_back(dr);
    }
    return OK;
}

u32 NtfsVolume::load_mftmirr_stream() {
    /* $MFTMirr (record 1) owns the mirror region; its copy sits at
     * mftmirr_lcn + record_size inside the mirror. Read THAT copy and take
     * its $DATA runlist as the mirror mapping. */
    std::vector<u8> buf;
    buf.resize(p_.record_size);
    u64 off = p_.mftmirr_lcn * p_.cluster_bytes + p_.record_size;
    if (off + p_.record_size > dev_->size())
        return ERR_RANGE_OUT_OF_BOUNDS;
    if (dev_->read_at(off, buf.data(), u32(buf.size())) != OK)
        return ERR_READ_FAILED;
    RecordHeader rh;
    std::memcpy(&rh, buf.data(), sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE)
        return ERR_BAD_MAGIC;
    u32 rc = fixup_apply(buf.data(), buf.size(), rh.update_sequence_offset, rh.update_sequence_size,
                         p_.sector_bytes);
    if (rc != OK)
        return rc;
    std::vector<u8> scratch;
    rc = enum_mirr_attrs(buf, scratch);
    if (rc != OK)
        return rc;
    return mirr_valid_ ? OK : ERR_BAD_PARAMETER;
}

u32 NtfsVolume::enum_mirr_attrs(const std::vector<u8>& buf, std::vector<u8>& scratch) {
    /* walk attributes of the $MFTMirr record copy; take its $DATA runs */
    u32 pos;
    {
        RecordHeader rh;
        std::memcpy(&rh, buf.data(), sizeof(rh));
        pos = rh.attribute_offset;
    }
    int depth = 0;
    while (pos + 16 <= buf.size() && depth < 64) {
        ++depth;
        u32 type = u32_from_bytes_le(buf.data() + pos);
        if (type == ATTR_TYPE_TERMINATOR)
            break;
        u32 len = u32_from_bytes_le(buf.data() + pos + 4);
        if (len < 16 || pos + len > buf.size())
            return ERR_BAD_PARAMETER;
        if (type == ATTR_TYPE_DATA) {
            if (buf[pos + 8]) { /* nonresident */
                if (pos + 64 > buf.size())
                    return ERR_BAD_PARAMETER;
                NonresidentTail na;
                std::memcpy(&na, buf.data() + pos + 16, sizeof(na));
                if (na.runlist_offset >= 0x40 && u32(na.runlist_offset) + 8 <= len) {
                    RunList rl;
                    if (RunList::decode(buf.data() + pos + na.runlist_offset,
                                        len - na.runlist_offset, na.start_vcn, p_.total_clusters,
                                        rl) == OK) {
                        mirr_run_ = rl;
                        mirr_valid_ = true;
                    }
                }
            } else {
                ResidentAttributeHeader ra;
                std::memcpy(&ra, buf.data() + pos, sizeof(ra));
                if (u64(ra.value_offset) + u64(ra.value_length) <= u64(len) &&
                    ra.value_length > 0) {
                    RunList rl;
                    if (RunList::decode(buf.data() + pos + ra.value_offset, ra.value_length, 0,
                                        p_.total_clusters, rl) == OK) {
                        mirr_run_ = rl;
                        mirr_valid_ = true;
                    }
                }
            }
        }
        pos += len;
    }
    (void)scratch;
    return OK;
}

std::unique_ptr<NtfsVolume> NtfsVolume::open(std::unique_ptr<IStorageDevice> device, u32* err) {
    if (!device) {
        if (err)
            *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    BootSector bs;
    if (device->size() < sizeof(bs)) {
        if (err)
            *err = ERR_BAD_PARAMETER;
        return nullptr;
    }
    if (device->read_at(0, &bs, sizeof(bs)) != OK) {
        if (err)
            *err = ERR_READ_FAILED;
        return nullptr;
    }
    if (!boot_signature_ok(reinterpret_cast<const u8*>(&bs))) {
        if (err)
            *err = ERR_BAD_MAGIC;
        return nullptr;
    }

    auto v = std::unique_ptr<NtfsVolume>(new NtfsVolume());
    v->dev_ = std::move(device);

    u32 rc = validate_boot_sector(bs, v->dev_->geometry(), v->p_);
    if (rc != OK) {
        if (err)
            *err = rc;
        return nullptr;
    }
    rc = v->load_mft_stream();
    if (rc != OK) {
        /* corpus: $MFTMirr cross-reference as repair/verification source when
         * primary fails. Try mirror's $MFT? The mirror holds records 0..3
         * copies - for bootstrap we only need record 0's runlist; if primary
         * is unreadable there is nothing to mirror FROM - surface the error. */
        if (err)
            *err = rc;
        return nullptr;
    }
    /* mirror stream is best-effort; its failure is not fatal */
    (void)v->load_mftmirr_stream();
    if (err)
        *err = OK;
    return v;
}

u32 NtfsVolume::read_record(u64 record_no, std::vector<u8>& out) {
    if (record_no >= max_record_no())
        return ERR_RANGE_OUT_OF_BOUNDS;
    out.resize(p_.record_size);
    /* Contiguous fast path first; fragmented-MFT extent view second (F6);
     * $MFTMirr as last resort for records 0..3 (corpus bootstrap). */
    const u64 vcn = record_no * p_.record_size / p_.cluster_bytes;
    const u64 rel = record_no * p_.record_size % p_.cluster_bytes;
    bool sparse = false;
    u64 lcn = 0;
    bool found = mft_run_.vcn_to_lcn(vcn, &lcn, &sparse);
    if (!found) {
        for (const DataRun& dr : mft_extents_) {
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
                return dev_->read_at(mlcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
        }
        return ERR_RANGE_OUT_OF_BOUNDS;
    }
    return dev_->read_at(lcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
}

u32 NtfsVolume::read_record_mirr(u64 record_no, std::vector<u8>& out) {
    if (!mirr_valid_ || record_no >= 4)
        return ERR_UNSUPPORTED;
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
