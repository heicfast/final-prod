/* cr_bootstrap.cpp - NTFS volume bootstrap (R2.1).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/ntfs/cr_bootstrap.h"
#include "cr/cr_log.h"
#include <algorithm>
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_ntfs_wire.h"
#include "cr/ntfs/cr_fixup.h"
#include <cstring>

namespace cr {
namespace ntfs {

namespace {} // namespace

u32 validate_boot_sector(const BootSector& bs, const Geometry& geom, VolumeParams& out) {
    if (!oem_is_ntfs(bs))
        return ERR_BAD_MAGIC;
    /* N-011b (diff-to-fix, session-2 re-attribution of M-001's quote): the
     * og's VOLUME-OPEN ladder is NTFSLib CNtfsVolume::LoadNtfsVolume
     * :19292-19344 (verified first-hand this session):
     *   - bps must be a 512 MULTIPLE (:19299 `(bps & 0x1ff) == 0`) - no
     *     pow2 requirement (1536 passes); bps == 256 fails the og's mask.
     *     The 4096 cap is core hardening (spec max; the og has none) -
     *     registered in PARITY-DELTAS D-52.
     *   - spc is a SIGNED byte: < 0x81 -> direct sectors-per-cluster (any
     *     value, non-pow2 included); >= 0x81 -> 1 << (256 - spc) sectors
     *     (:19308-19312). spc == 0 -> reject. The pow2 requirement is
     *     dropped (the og has none).
     *   - cpr (clusters-per-record i8): 0 or a 0-result -> the volume is
     *     REJECTED (:19322/:19331 "It is not a NTFS volume!") - the M-001
     *     0x200-fallback/0x400-clamp ladder belongs to the WALK function
     *     (FUN_18001acc0), not volume open; no pow2 / %bps check, no
     *     0x400 clamp here.
     *   - index-block byte (+0x44): 0 -> reject (:19336-19343).
     *   - no 0xAA55 check (the og never looks at the signature here).
     * Core hardening kept and registered: the bps-vs-device-sector match
     * (512e escape) and the P1-3 total-size sanity + P1-4 shift-UB guard
     * (1 MiB record cap) - the og lacks all three. */
    if ((bs.bytes_per_sector & 0x1FF) != 0 || bs.bytes_per_sector < 512 ||
        bs.bytes_per_sector > 4096)
        return ERR_BAD_PARAMETER;
    if (bs.clusters_per_index_block == 0)
        return ERR_BAD_PARAMETER;
    const i8 spc_raw = static_cast<i8>(bs.sectors_per_cluster);
    if (spc_raw == 0)
        return ERR_BAD_PARAMETER;
    const u64 sectors_per_cluster =
        spc_raw > 0 ? u64(spc_raw) : (1ULL << (u64(-spc_raw) & 31)); /* og shift decode */
    if (bs.bytes_per_sector != geom.logical_sector &&
        bs.bytes_per_sector != 512 /* 512e volumes report 512 in DBR */)
        return ERR_BAD_PARAMETER;

    const u64 cluster_bytes = sectors_per_cluster * bs.bytes_per_sector;
    if (cluster_bytes == 0)
        return ERR_BAD_PARAMETER;
    const u64 total_clusters = bs.total_sectors / sectors_per_cluster;
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

    /* N-011b: the og's OPEN ladder decodes cpr directly - positive ->
     * cpr x sectors-per-cluster sectors, negative -> (2^(-cpr))/bps
     * sectors (:19321-19330) - and REJECTS on 0/0-result. No pow2 or
     * sector-divisibility check and no 0x400 clamp at open (the M-001
     * walk-time ladder still governs buffer capacity inside the walk,
     * whose evidence FUN_18001acc0 :21134-21142 stands). The P1-4
     * shift-UB guard (exponents > 20 decode 0 -> reject) stays as core
     * hardening (the og would compute a 1-GiB record size for -30). */
    u64 record_size = record_size_from_dbr(bs, bs.bytes_per_sector);
    if (record_size == 0)
        return ERR_BAD_PARAMETER; /* og: "It is not a NTFS volume!" */
    if (record_size < bs.bytes_per_sector)
        return ERR_BAD_PARAMETER; /* sub-sector record: the og's 0-result */

    out.record_size = record_size;
    out.cluster_bytes = static_cast<u32>(cluster_bytes);
    out.sector_bytes = bs.bytes_per_sector;
    out.total_clusters = total_clusters;
    out.mft_lcn = bs.mft_lcn;
    out.mftmirr_lcn = bs.mftmirr_lcn;
    out.flags = 0;
    return OK;
}

u32 NtfsVolume::load_mft_stream(u64 at_lcn, bool from_mirror) {
    /* read record 0 with the corpus minimum capacity, fixup, then decode the
     * $DATA runlist of $MFT itself; $ATTRIBUTE_LIST -> fragmented path (F6).
     * S-063: at_lcn parameterizes the READ POSITION — the primary call passes
     * p_.mft_lcn, the og's mirror fallback (FUN_1800272b0's LAB_180027467,
     * NTFSSearchDLL :31595-31609, the identical chunk reader FUN_18001a3c0
     * at the $MFTMirr LCN) passes p_.mftmirr_lcn. from_mirror only routes
     * the geometry gates the og's fallback re-checks (the odd/>16-sector
     * record-size class re-reads at 512-byte rounding there; our P1-4 cap
     * already governs both positions the same way — registered N-196). */
    (void)from_mirror;
    std::vector<u8> buf;
    buf.resize(p_.record_size);
    if (dev_->read_at(at_lcn * p_.cluster_bytes, buf.data(), u32(buf.size())) != OK)
        return ERR_READ_FAILED;

    RecordHeader rh;
    std::memcpy(&rh, buf.data(), sizeof(rh));
    if (rh.magic != RECORD_MAGIC_FILE)
        return ERR_BAD_MAGIC;
    u32 rc = fixup_apply(buf.data(), buf.size(), rh.update_sequence_offset, rh.update_sequence_size,
                         p_.sector_bytes);
    if (rc != OK)
        return rc;

    /* walk attributes of record 0 via the shared walker (W4 consolidation,
     * AUDIT-NTFS §4 row 21: the three hand-rolled loops had divergent bounds
     * checks - name_offset sanity only here, a 256-attr cap here vs 64 in the
     * other two - record 0 now gets the same walk every other record gets) */
    std::vector<AttrView> attrs;
    rc = enum_attributes(buf.data(), u32(buf.size()), attrs);
    if (rc != OK)
        return rc;
    bool have_data = false;
    bool have_attr_list = false;
    std::vector<u8> attrlist_extra; /* $ATTRIBUTE_LIST content of $MFT */

    for (const AttrView& a : attrs) {
        if (a.type == ATTR_TYPE_DATA) {
            if (!a.nonresident) {
                /* resident $DATA on $MFT: tiny MFT (all records in one run);
                 * the resident value IS the runlist bytes */
                RunList rl;
                if (RunList::decode(a.at + a.value_offset, a.value_length, 0, p_.total_clusters,
                                    rl) != OK)
                    return ERR_BAD_PARAMETER;
                mft_run_ = rl;
                have_data = true;
            } else {
                RunList rl;
                if (RunList::decode(a.at + a.runlist_offset, a.record_length - a.runlist_offset,
                                    a.start_vcn, p_.total_clusters, rl) != OK)
                    return ERR_BAD_PARAMETER;
                mft_run_ = rl;
                have_data = true;
            }
        } else if (a.type == ATTR_TYPE_ATTRIBUTE_LIST) {
            /* corpus: "MFT file record has 0x20 attribute" path - keep the
             * content bytes; after the loop we reassemble from child records.
             * Nonresident lists are legal (huge, fragmented $MFTs) - the og
             * chases them (FUN_18001a540 fragmented-MFT handler); decode the
             * runlist and read the stream, bounded (attrlists stay small even
             * when nonresident - 1 MiB cap against corrupt size fields). */
            if (a.nonresident) {
                RunList rl;
                if (RunList::decode(a.at + a.runlist_offset, a.record_length - a.runlist_offset,
                                    a.start_vcn, p_.total_clusters, rl) != OK)
                    return ERR_BAD_PARAMETER;
                u64 want = a.actual != 0 ? a.actual : a.allocated;
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
                attrlist_extra.assign(a.at + a.value_offset,
                                      a.at + a.value_offset + a.value_length);
                have_attr_list = true;
            }
        }
    }
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
                            /* shared walker (row 21); on a corrupt child the
                             * attributes pushed before the first invalid one
                             * are salvaged - the old loop broke mid-walk the
                             * same way */
                            std::vector<AttrView> cattrs;
                            (void)enum_attributes(cr.data(), u32(cr.size()), cattrs);
                            for (const AttrView& ca : cattrs) {
                                if (ca.type == ATTR_TYPE_DATA && ca.nonresident) {
                                    RunList crl;
                                    if (RunList::decode(ca.at + ca.runlist_offset,
                                                        ca.record_length - ca.runlist_offset,
                                                        ca.start_vcn, p_.total_clusters,
                                                        crl) == OK) {
                                        Frag f;
                                        f.lowest_vcn = alowest;
                                        f.runs = crl.runs();
                                        frags.push_back(std::move(f));
                                    }
                                }
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
    rc = enum_mirr_attrs(buf);
    if (rc != OK)
        return rc;
    return mirr_valid_ ? OK : ERR_BAD_PARAMETER;
}

u32 NtfsVolume::enum_mirr_attrs(const std::vector<u8>& buf) {
    /* walk attributes of the $MFTMirr record copy via the shared walker
     * (row 21); take the FIRST unnamed $DATA runlist as the mirror mapping.
     * N-001 (diff-to-fix) - og parity (NTFSLib InitMFTMirror,
     * NTFSLib_decompiled.c :17369-17414, read first-hand): the FIRST
     * unnamed $DATA decides - non-resident -> decode its runlist and STOP
     * (the og breaks at :17393); resident -> hard abort with the logged
     * "$MFTMirror::$DATA is resident!" string and the mirror is INVALID
     * (init returns 0). The previous code accepted a LATER $DATA and even
     * tried to decode resident content as a runlist - a corrupt mirror
     * could fabricate a mapping the og refuses. */
    std::vector<AttrView> attrs;
    u32 rc = enum_attributes(buf.data(), u32(buf.size()), attrs);
    if (rc != OK)
        return rc;
    for (const AttrView& a : attrs) {
        if (a.type != ATTR_TYPE_DATA || a.name_length != 0)
            continue; /* named streams are not the mirror's $DATA */
        if (!a.nonresident)
            return OK; /* resident: og abort - mirror stays invalid */
        RunList rl;
        if (RunList::decode(a.at + a.runlist_offset, a.record_length - a.runlist_offset,
                            a.start_vcn, p_.total_clusters, rl) == OK) {
            mirr_run_ = rl;
            mirr_valid_ = true;
        }
        break; /* first unnamed $DATA only (og :17393) */
    }
    /* the og serves mirror reads up to the runlist-derived span
     * (InitMFTMirror writes this+0x70 = (last_run.offset + last_run.length)
     * factors, and CNtfsMFT::ReadFileRecord gates on `record <= this+0x70`,
     * :17460) - NOT a hardcoded 4. Record capacity = runlist span bytes /
     * record size, sanity-capped against absurd corrupt runlists. */
    mirr_record_capacity_ = 0;
    if (mirr_valid_ && p_.record_size != 0) {
        const u64 span = mirr_run_.total_clusters() * u64(p_.cluster_bytes);
        mirr_record_capacity_ = span / p_.record_size;
        if (mirr_record_capacity_ > 1024)
            mirr_record_capacity_ = 1024; /* defensive cap (P1-family) */
    }
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

    auto v = std::unique_ptr<NtfsVolume>(new NtfsVolume());
    v->dev_ = std::move(device);

    /* M-043 (diff-to-fix): a torn/damaged primary DBR no longer ends the
     * scan — the og's analyser verifies the VOLUME-END BACKUP DBR in the
     * recognition path (CNTFSVolumeAnalyser::Analyze, backup sector via
     * FUN_1800048f0, the verified location stored at +0x510; corpus
     * NTFSFileSystemAnalyser_decompiled.c :4309-4327) and proceeds from
     * it, where the core's old scanner never read the backup at all (only
     * the consent-gated rawfixer did — a torn primary with an intact
     * backup required the user to know about --repair). Read-only
     * fallback: the backup's geometry feeds the MFT load and
     * boot_from_backup flags it; REPAIRING the primary stays consent-
     * gated exactly as before. The backup location is the volume's LAST
     * sector (rawfixer's corpus-decoded `total - sector`, S-008). */
    bool from_backup = false;
    u32 rc = OK;
    if (!boot_signature_ok(reinterpret_cast<const u8*>(&bs)))
        rc = ERR_BAD_MAGIC;
    if (rc == OK)
        rc = validate_boot_sector(bs, v->dev_->geometry(), v->p_);
    if (rc != OK) {
        BootSector backup{};
        const u64 total = v->dev_->size();
        const u64 sec = v->dev_->geometry().logical_sector != 0
                            ? u64(v->dev_->geometry().logical_sector)
                            : u64(512);
        if (total >= 2 * sec && v->dev_->read_at(total - sec, &backup, sizeof(backup)) == OK &&
            boot_signature_ok(reinterpret_cast<const u8*>(&backup))) {
            VolumeParams bp{};
            if (validate_boot_sector(backup, v->dev_->geometry(), bp) == OK) {
                bs = backup;
                v->p_ = bp;
                from_backup = true;
                rc = OK;
            }
        }
        if (rc != OK) {
            if (err)
                *err = rc;
            return nullptr;
        }
    }
    v->p_.boot_from_backup = from_backup;
    rc = v->load_mft_stream(v->p_.mft_lcn, false);
    if (rc != OK && v->p_.mftmirr_lcn != v->p_.mft_lcn) {
        /* S-063: the og's $MFTMirr bootstrap fallback — when record 0 is
         * unreadable at the primary $MFT LCN (or its geometry gates reject
         * it), the walk entry retries the IDENTICAL read at the $MFTMirr
         * LCN before refusing with 0x610016 (FUN_1800272b0, the
         * LAB_180027467 block at NTFSSearchDLL :31595-31609; the getters
         * FUN_18003f990/FUN_18003f970 = the boot struct's +0x30/+0x38
         * LCN pair). The mirror's record-0 slot is a copy of $MFT's own
         * record, whose $DATA runlist describes the entire primary MFT:
         * there is everything to bootstrap FROM — the pre-fix comment
         * reasoned the opposite and refused the whole volume. */
        const u32 mrc = v->load_mft_stream(v->p_.mftmirr_lcn, true);
        if (mrc == OK) {
            v->p_.boot_from_mftmirr = true; /* the open diagnostic */
            if (cr::log::enabled())
                cr::log::write(cr::log::Level::Info,
                               "ntfs: primary $MFT record 0 unreadable (rc=%08x); "
                               "bootstrapped from the $MFTMirr copy (LCN %llu)",
                               rc, static_cast<unsigned long long>(v->p_.mftmirr_lcn));
            rc = OK;
        }
    }
    if (rc != OK) {
        /* both the primary and the mirror failed — the og's 0x610016
         * refusal class: surface the primary's error. */
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
        /* N-001: the mirror serves whatever its $DATA runlist actually
         * spans (og ReadFileRecord gate `param_1 <= this+0x70`) - not a
         * hardcoded first-4-records window. Modern Windows mirrors span
         * exactly 4 records; NT 3.x/4.0 and some third-party formatters
         * mirror more. */
        if (record_no < mirr_record_capacity_ && mirr_valid_) {
            u64 mlcn = 0;
            bool msparse = false;
            if (mirr_run_.vcn_to_lcn(vcn, &mlcn, &msparse) && !msparse)
                return dev_->read_at(mlcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
        }
        return ERR_RANGE_OUT_OF_BOUNDS;
    }
    return dev_->read_at(lcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
}

u32 NtfsVolume::read_record_mirror(u64 record_no, std::vector<u8>& out) {
    /* S-012: mirror-first read for the size-0 rescue (the og's FUN_18001e120
     * iterates the MFT-copy list and reads the record's copy there). The
     * primary may hold the wiped/deleted state while the mirror still keeps
     * the older bytes with resident $DATA intact. */
    if (record_no >= max_record_no() || !mirr_valid_ || record_no >= mirr_record_capacity_)
        return ERR_RANGE_OUT_OF_BOUNDS;
    out.resize(p_.record_size);
    const u64 vcn = record_no * p_.record_size / p_.cluster_bytes;
    const u64 rel = record_no * p_.record_size % p_.cluster_bytes;
    u64 mlcn = 0;
    bool msparse = false;
    if (!mirr_run_.vcn_to_lcn(vcn, &mlcn, &msparse) || msparse)
        return ERR_RANGE_OUT_OF_BOUNDS;
    return dev_->read_at(mlcn * p_.cluster_bytes + rel, out.data(), u32(out.size()));
}

} // namespace ntfs
} // namespace cr
