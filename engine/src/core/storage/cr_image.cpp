/*
 * cr_image.cpp - disk-image scan targets (S-030). See cr_image.h.
 *
 * Format references (re-implemented from format understanding):
 *  - VMDK hosted sparse: SparseExtentHeader (magic 0x564D444B, version
 *    1/3), grain directory -> grain tables -> grains, embedded or
 *    standalone text descriptor with RW <sectors> <FLAT|SPARSE|ZERO>
 *    extent lines.
 *  - VHD: 512-byte "conectix" footer (disk type 2 = fixed, 3 = dynamic,
 *    4 = differencing), "cxsparse" dynamic header, u32 BAT (0xFFFFFFFF =
 *    unallocated), block = sector bitmap + data.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_image.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace cr {
namespace storage {

namespace {

/* little-endian scalar loads (the image formats are LE on disk) */
u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
u64 rd64(const u8* p) {
    return u64(rd32(p)) | (u64(rd32(p + 4)) << 32);
}

} // namespace

namespace {

void fill_geometry(Geometry& g, u64 total_bytes) {
    g.logical_sector = 512;
    g.physical_sector = 512;
    g.sector_count = total_bytes / 512;
    g.total_bytes = total_bytes;
}

/* =========================================================================
 * flat device
 * ========================================================================= */

class FlatImageDevice final : public IStorageDevice {
public:
    FlatImageDevice(std::unique_ptr<ImageByteSource> src, u64 offset, u64 bytes)
        : src_(std::move(src)) {
        const u64 fsize = src_->size();
        if (bytes == 0)
            bytes = fsize > offset ? fsize - offset : 0;
        total_ = bytes & ~u64(511); /* sector-granular bound */
        base_ = offset;
        fill_geometry(geom_, total_);
    }
    bool valid() const { return total_ != 0 && base_ + total_ <= src_->size(); }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > total_ || u64(len) > total_ - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        return src_->read(base_ + off, buf, len);
    }
    const Geometry& geometry() const override { return geom_; }
    u64 absolute_base() const override { return base_; }

private:
    std::unique_ptr<ImageByteSource> src_;
    u64 base_ = 0, total_ = 0;
    Geometry geom_;
};

/* =========================================================================
 * VMDK (hosted sparse, v1/v3 geometry; no parent chains, no compression)
 * ========================================================================= */

constexpr u32 kVmdkMagic = 0x564D444B; /* "KDMV" LE */

struct VmdkExtent {
    u64 start_sector = 0; /* logical sector in the virtual disk */
    u64 num_sectors = 0;
    enum Kind : u8 { KindFlat, KindSparse, KindZero } kind = KindZero;
    std::shared_ptr<ImageByteSource> src; /* null for ZERO */
    u64 file_base_sector = 0;             /* FLAT: base offset (sectors) */
    /* sparse geometry (from the extent header) */
    u64 grain_sectors = 0;
    u32 gtes_per_gt = 0;
    std::vector<u32> gd; /* eagerly-loaded grain directory (GDEs, sectors) */

    u64 end_sector() const { return start_sector + num_sectors; }
};

class VmdkDevice final : public IStorageDevice {
public:
    VmdkDevice(std::vector<VmdkExtent> exts, u64 total_sectors)
        : exts_(std::move(exts)), total_sectors_(total_sectors) {
        fill_geometry(geom_, total_sectors_ * 512);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        const u64 total = total_sectors_ * 512;
        if (off > total || u64(len) > total - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        u8* dst = static_cast<u8*>(buf);
        u64 done = 0;
        while (done < len) {
            const u64 abs = off + done;
            const u64 sec = abs / 512;
            const VmdkExtent* ext = find(sec);
            if (ext == nullptr) {
                /* outside every extent: zeros (the og's sparse-model
                 * semantics - a hole beyond the declared extents) */
                const u64 next = next_extent_start(sec);
                u64 n = (next == 0 ? total_sectors_ : next) * 512 - abs;
                if (n > len - done)
                    n = len - done;
                std::memset(dst + done, 0, size_t(n));
                done += n;
                continue;
            }
            /* byte position inside the extent (extent-local, NOT the
             * file position - FLAT adds its base, SPARSE maps grains) */
            const u64 eb = (sec - ext->start_sector) * 512 + abs % 512;
            /* bytes remaining inside this extent */
            u64 cap = ext->num_sectors * 512 - eb;
            u64 n = len - done < cap ? len - done : cap;
            switch (ext->kind) {
            case VmdkExtent::KindZero:
                std::memset(dst + done, 0, size_t(n));
                break;
            case VmdkExtent::KindFlat: {
                const u64 file_off = ext->file_base_sector * 512 + eb;
                if (u32 rc = ext->src->read(file_off, dst + done, u32(n)))
                    return rc;
                break;
            }
            case VmdkExtent::KindSparse: {
                /* grain-segment batching: one GDE/GTE lookup covers the
                 * whole run to the next grain boundary (grains are
                 * power-of-two sector counts >= 512 bytes) */
                const u64 grain = ext->grain_sectors;
                const u64 g = eb / 512 / grain;
                const u64 in_grain = eb - (g * grain) * 512;
                u64 seg = grain * 512 - in_grain;
                if (seg < n)
                    n = seg;
                u32 gte = 0;
                if (g < u64(ext->gd.size()) * ext->gtes_per_gt &&
                    ext->gd[size_t(g / ext->gtes_per_gt)] != 0) {
                    u8 gt[4];
                    const u64 gde = ext->gd[size_t(g / ext->gtes_per_gt)];
                    if (u32 rc =
                            ext->src->read(gde * 512 + (g % ext->gtes_per_gt) * 4, gt, 4))
                        return rc;
                    gte = rd32(gt);
                }
                if (gte == 0) {
                    std::memset(dst + done, 0, size_t(n));
                } else if (u32 rc = ext->src->read(u64(gte) * 512 + in_grain, dst + done,
                                                   u32(n))) {
                    return rc;
                }
                break;
            }
            }
            done += n;
        }
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    const VmdkExtent* find(u64 sector) const {
        for (const auto& e : exts_)
            if (sector >= e.start_sector && sector < e.end_sector())
                return &e;
        return nullptr;
    }
    u64 next_extent_start(u64 sector) const {
        u64 best = 0;
        for (const auto& e : exts_)
            if (e.start_sector > sector && (best == 0 || e.start_sector < best))
                best = e.start_sector;
        return best;
    }

    std::vector<VmdkExtent> exts_;
    u64 total_sectors_ = 0;
    Geometry geom_;
};

/* parse one extent line: <access> <sectors> <TYPE> ["<file>" [<offset>]]
 * (access is RW/RO - we scan read-only either way). */
bool parse_extent_line(const std::string& line, u64& sectors, VmdkExtent::Kind& kind,
                       std::string& file, u64& flat_offset) {
    size_t i = 0;
    auto skip_ws = [&] {
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
            ++i;
    };
    auto token = [&](std::string& t) {
        skip_ws();
        if (i < line.size() && line[i] == '"') {
            const size_t j = line.find('"', i + 1);
            if (j == std::string::npos)
                return false;
            t = line.substr(i + 1, j - i - 1);
            i = j + 1;
            return true;
        }
        const size_t j = line.find_first_of(" \t", i);
        t = line.substr(i, j == std::string::npos ? std::string::npos : j - i);
        i = j;
        return !t.empty();
    };
    std::string access;
    if (!token(access) || (access != "RW" && access != "RO"))
        return false;
    std::string nsec, type;
    if (!token(nsec) || !token(type))
        return false;
    sectors = 0;
    for (char c : nsec) {
        if (c < '0' || c > '9')
            return false;
        sectors = sectors * 10 + u64(c - '0');
        if (sectors > (1ull << 40))
            return false; /* hostile size: > 512 TiB extent */
    }
    if (type == "ZERO") {
        kind = VmdkExtent::KindZero;
        return true;
    }
    if (type == "FLAT") {
        kind = VmdkExtent::KindFlat;
    } else if (type == "SPARSE") {
        kind = VmdkExtent::KindSparse;
    } else {
        return false; /* VMFS/differencing families: ERR_UNSUPPORTED by
                         the caller (parse fails, type preserved via kind
                         sentinel below) */
    }
    if (!token(file))
        return false;
    flat_offset = 0;
    std::string off;
    if (token(off)) {
        for (char c : off) {
            if (c < '0' || c > '9')
                return false;
            flat_offset = flat_offset * 10 + u64(c - '0');
        }
    }
    return true;
}

u32 load_sparse_geometry(ImageByteSource& src, VmdkExtent& e) {
    u8 hdr[512];
    if (u32 rc = src.read(0, hdr, sizeof(hdr)))
        return rc;
    if (rd32(hdr) != kVmdkMagic)
        return ERR_BAD_MAGIC;
    const u32 version = rd32(hdr + 4);
    if (version != 1 && version != 3)
        return ERR_UNSUPPORTED;
    const u64 capacity = rd64(hdr + 8);
    const u64 grain = rd64(hdr + 16);
    const u32 gtes = rd32(hdr + 40);
    const u64 gd_off = rd64(hdr + 52);
    if (capacity == 0 || grain == 0 || gtes == 0)
        return ERR_BAD_MAGIC;
    if (capacity > (1ull << 42))
        return ERR_BAD_MAGIC; /* hostile capacity: > 4 PiB extents */
    if ((grain & (grain - 1)) != 0 || grain < 1)
        return ERR_BAD_MAGIC; /* grains are power-of-two sector counts */
    e.grain_sectors = grain;
    e.gtes_per_gt = gtes;
    if (gd_off == 0) {
        /* v3 allows gdOffset==0 with an embedded L1 - out of v1 scope */
        return ERR_UNSUPPORTED;
    }
    /* grain directory: one GDE per grain table; tables cover gtes grains */
    const u64 grains = (capacity + grain - 1) / grain;
    const u64 gdes = (grains + gtes - 1) / gtes;
    if (gdes == 0 || gdes > (1u << 22)) /* hostile directory: > 4M tables */
        return ERR_BAD_MAGIC;
    std::vector<u8> raw(size_t(gdes * 4));
    if (u32 rc = src.read(gd_off * 512, raw.data(), u32(raw.size())))
        return rc;
    e.gd.resize(size_t(gdes));
    for (u64 k = 0; k < gdes; ++k)
        e.gd[size_t(k)] = rd32(raw.data() + size_t(k * 4));
    if (e.num_sectors == 0)
        e.num_sectors = capacity;
    return OK;
}

u32 build_vmdk(std::unique_ptr<ImageByteSource> src, const ImageFileOps& ops,
               const char* base_dir, std::unique_ptr<IStorageDevice>& out) {
    out.reset();
    if (!src)
        return ERR_BAD_PARAMETER;
    u8 first[512];
    const u64 head_n = src->size() < 512 ? src->size() : 512;
    const u32 sniff_rc = src->read(0, first, u32(head_n));
    if (sniff_rc != OK)
        return sniff_rc;

    std::string descriptor;
    /* monolithic images: the source itself is the (single) extent */
    std::shared_ptr<ImageByteSource> mono;
    u64 mono_capacity = 0;

    if (rd32(first) == kVmdkMagic) {
        /* monolithicSparse: the descriptor is embedded at
         * descriptorOffset/Size; the source itself is the extent. */
        const u64 desc_off = rd64(first + 24) * 512;
        const u64 desc_len = rd64(first + 32) * 512;
        if (desc_len == 0 || desc_len > (4u << 20))
            return ERR_BAD_MAGIC;
        descriptor.resize(size_t(desc_len));
        if (u32 rc = src->read(desc_off, descriptor.data(), u32(desc_len)))
            return rc;
        mono_capacity = rd64(first + 8);
        mono = std::move(src);
    } else if (std::memcmp(first, "# Disk DescriptorFile", 21) == 0) {
        const u64 fsize = src->size();
        const u64 n = fsize < (1u << 20) ? fsize : (1u << 20);
        descriptor.resize(size_t(n));
        if (u32 rc = src->read(0, descriptor.data(), u32(n)))
            return rc;
        src.reset(); /* the descriptor file is not extent data */
    } else {
        return ERR_BAD_MAGIC;
    }

    /* parse the descriptor: extent lines only (v1; createType/CID lines
     * are informational for reading) */
    std::vector<VmdkExtent> exts;
    u64 logical = 0;
    size_t pos = 0;
    while (pos < descriptor.size()) {
        size_t eol = descriptor.find('\n', pos);
        if (eol == std::string::npos)
            eol = descriptor.size();
        std::string line = descriptor.substr(pos, eol - pos);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        pos = eol + 1;
        if (line.empty() || line[0] == '#')
            continue;
        if (line.find('=') != std::string::npos)
            continue; /* key=value header or an unknown key: not an extent */
        u64 sectors = 0, flat_off = 0;
        VmdkExtent::Kind kind;
        std::string file;
        if (!parse_extent_line(line, sectors, kind, file, flat_off))
            return ERR_BAD_MAGIC;
        VmdkExtent e;
        e.start_sector = logical;
        e.num_sectors = sectors;
        e.kind = kind;
        e.file_base_sector = flat_off; /* FLAT only; ignored elsewhere */
        if (kind == VmdkExtent::KindZero) {
            /* no source */
        } else if (mono != nullptr) {
            /* monolithic: the single extent IS this source */
            e.src = mono;
            if (kind == VmdkExtent::KindSparse) {
                e.num_sectors = mono_capacity;
                if (u32 rc = load_sparse_geometry(*mono, e))
                    return rc;
            }
        } else if (ops.valid()) {
            const std::string path = std::string(base_dir ? base_dir : "") + file;
            std::unique_ptr<ImageByteSource> fs;
            if (u32 rc = ops.open(ops.user, path.c_str(), fs))
                return rc;
            if (kind == VmdkExtent::KindSparse) {
                if (u32 rc = load_sparse_geometry(*fs, e))
                    return rc;
            }
            e.src = std::move(fs);
        } else {
            return ERR_BAD_PARAMETER; /* extent file needed, no opener */
        }
        logical += e.num_sectors;
        exts.push_back(std::move(e));
    }
    if (exts.empty())
        return ERR_BAD_MAGIC; /* a descriptor with no extents */

    out.reset(new VmdkDevice(std::move(exts), logical));
    return OK;
}

/* =========================================================================
 * VHD (fixed + dynamic; differencing is out of v1 scope)
 * ========================================================================= */

constexpr u32 kVhdFixed = 2, kVhdDynamic = 3, kVhdDifferencing = 4;

class VhdDynamicDevice final : public IStorageDevice {
public:
    VhdDynamicDevice(std::unique_ptr<ImageByteSource> src, u64 total_bytes, u32 block_size,
                     std::vector<u32> bat)
        : src_(std::move(src)), block_(block_size), bat_(std::move(bat)) {
        fill_geometry(geom_, total_bytes);
        bitmap_bytes_ = (block_ / 512 + 7) / 8;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        const u64 total = geom_.total_bytes;
        if (off > total || u64(len) > total - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        u8* dst = static_cast<u8*>(buf);
        u64 done = 0;
        while (done < len) {
            const u64 abs = off + done;
            const u64 block = abs / block_;
            if (block >= bat_.size()) {
                std::memset(dst + done, 0, size_t(len - done));
                break;
            }
            const u32 bat = bat_[size_t(block)];
            const u64 in_block = abs % block_;
            u64 n = std::min<u64>(len - done, block_ - in_block);
            if (bat == 0xFFFFFFFFu) {
                std::memset(dst + done, 0, size_t(n));
            } else {
                /* sector bitmap: 1 = the sector's data is present */
                const u64 sec_in_block = in_block / 512;
                const u64 base = u64(bat) * 512;
                /* split the run at bitmap state changes */
                u64 run = 0;
                bool alloc = sector_allocated(base, sec_in_block);
                while (run < n) {
                    const u64 s = sec_in_block + (run + 512 - 1) / 512;
                    if (s * 512 >= block_)
                        break;
                    const bool a = sector_allocated(base, s);
                    if (a != alloc)
                        break;
                    run += 512;
                }
                if (run == 0)
                    run = 512;
                if (run > n)
                    run = n;
                if (alloc) {
                    if (u32 rc = src_->read(base + bitmap_bytes_ + in_block, dst + done,
                                            u32(run)))
                        return rc;
                } else {
                    std::memset(dst + done, 0, size_t(run));
                }
                n = run;
            }
            done += n;
        }
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    bool sector_allocated(u64 bat_base, u64 sec_in_block) {
        if (sec_in_block >= block_ / 512)
            return false;
        u8 b = 0;
        if (src_->read(bat_base + sec_in_block / 8, &b, 1) != OK)
            return false;
        return (b & (1u << (sec_in_block % 8))) != 0;
    }
    std::unique_ptr<ImageByteSource> src_;
    u64 block_ = 0, bitmap_bytes_ = 0;
    std::vector<u32> bat_;
    Geometry geom_;
};

u32 build_vhd(std::unique_ptr<ImageByteSource> src, std::unique_ptr<IStorageDevice>& out) {
    out.reset();
    if (!src)
        return ERR_BAD_PARAMETER;
    const u64 fsize = src->size();
    if (fsize < 1024)
        return ERR_BAD_PARAMETER; /* footer + at least one sector */
    u8 foot[512];
    /* the trailing footer is authoritative (the offset-0 copy may be a
     * stale mirror after live converts) */
    if (u32 rc = src->read(fsize - 512, foot, sizeof(foot)))
        return rc;
    if (std::memcmp(foot, "conectix", 8) != 0)
        return ERR_BAD_MAGIC;
    const u64 data_offset = rd64(foot + 16);
    const u64 current_size = rd64(foot + 48);
    const u32 disk_type = rd32(foot + 60);
    if (current_size == 0 || (current_size & 511) != 0)
        return ERR_BAD_MAGIC;
    if (disk_type == kVhdFixed) {
        if (current_size + 512 > fsize)
            return ERR_BAD_MAGIC;
        out.reset(new FlatImageDevice(std::move(src), 0, current_size));
        return OK;
    }
    if (disk_type == kVhdDifferencing)
        return ERR_UNSUPPORTED; /* parent chains: tracked remainder */
    if (disk_type != kVhdDynamic)
        return ERR_UNSUPPORTED;
    /* dynamic header */
    u8 dyn[1024];
    if (u32 rc = src->read(data_offset, dyn, sizeof(dyn)))
        return rc;
    if (std::memcmp(dyn, "cxsparse", 8) != 0)
        return ERR_BAD_MAGIC;
    const u64 table_offset = rd64(dyn + 16);
    const u32 max_entries = rd32(dyn + 28);
    const u32 block_size = rd32(dyn + 32);
    if (table_offset == 0 || max_entries == 0 || block_size == 0 ||
        (block_size & 511) != 0 || (block_size & (block_size - 1)) != 0)
        return ERR_BAD_MAGIC;
    if (u64(max_entries) * 4 + table_offset > fsize)
        return ERR_BAD_MAGIC;
    if ((current_size + block_size - 1) / block_size > max_entries)
        return ERR_BAD_MAGIC; /* geometry cannot cover the disk */
    std::vector<u8> raw(size_t(max_entries) * 4);
    if (u32 rc = src->read(table_offset, raw.data(), u32(raw.size())))
        return rc;
    std::vector<u32> bat(max_entries);
    for (u32 k = 0; k < max_entries; ++k)
        bat[k] = rd32(raw.data() + size_t(k * 4));
    out.reset(new VhdDynamicDevice(std::move(src), current_size, block_size, std::move(bat)));
    return OK;
}

} // namespace

/* =========================================================================
 * public dispatch
 * ========================================================================= */

ImageFormat sniff_image_format(const u8* first_bytes, size_t n) {
    if (first_bytes == nullptr)
        return ImageFormat::Unknown;
    if (n >= 4 && rd32(first_bytes) == kVmdkMagic)
        return ImageFormat::Vmdk;
    if (n >= 21 && std::memcmp(first_bytes, "# Disk DescriptorFile", 21) == 0)
        return ImageFormat::Vmdk;
    /* VHDX files open with an 8-byte "vhdxfile" signature; VHD's identity
     * cookie ("conectix") lives in the TRAILING footer - checked by the
     * caller, not by this head-only sniff */
    if (n >= 8 && std::memcmp(first_bytes, "vhdxfile", 8) == 0)
        return ImageFormat::Vhdx;
    return ImageFormat::Flat;
}

u32 open_image(std::unique_ptr<ImageByteSource> src, const ImageFileOps& ops,
               const char* utf8_base_dir, std::unique_ptr<IStorageDevice>& out) {
    out.reset();
    if (!src)
        return ERR_BAD_PARAMETER;
    const u64 ssize = src->size();
    /* the smallest recognizable input is a VMDK text descriptor (tens of
     * bytes); every other scan target needs at least one full sector */
    if (ssize < 21)
        return ERR_BAD_PARAMETER;
    u8 first[512];
    const u64 want = ssize < 512 ? ssize : 512;
    if (u32 rc = src->read(0, first, u32(want)))
        return rc;
    const ImageFormat head = sniff_image_format(first, size_t(want));
    if (head == ImageFormat::Unknown ||
        (ssize < 512 && head != ImageFormat::Vmdk))
        return ERR_BAD_PARAMETER; /* too small to be a sector-aligned target */
    switch (head) {
    case ImageFormat::Vmdk:
        return build_vmdk(std::move(src), ops, utf8_base_dir, out);
    case ImageFormat::Vhdx:
    case ImageFormat::Edisk:
        return ERR_UNSUPPORTED; /* tracked remainder / deliberate exclusion */
    case ImageFormat::Vhd:
    case ImageFormat::Flat:
    case ImageFormat::Unknown:
        break;
    }
    /* VHD identity: the authoritative "conectix" footer TRAILS the file
     * (fixed disks carry raw data at offset 0 - the head cannot identify
     * them; dynamic disks also trail). */
    if (ssize >= 1024) {
        u8 tail[8];
        if (src->read(ssize - 512, tail, 8) == OK && std::memcmp(tail, "conectix", 8) == 0)
            return build_vhd(std::move(src), out);
    }
    auto flat = std::unique_ptr<FlatImageDevice>(new FlatImageDevice(std::move(src), 0, 0));
    if (!flat->valid())
        return ERR_BAD_PARAMETER;
    out = std::move(flat);
    return OK;
}


BootLayout classify_boot_layout(IStorageDevice* dev) {
    if (dev == nullptr)
        return BootLayout::Unknown;
    u8 lba0[512], lba1[512];
    if (dev->read_at(0, lba0, sizeof(lba0)) != OK)
        return BootLayout::Unknown;
    /* GPT: the protective MBR entry (type 0xEE) wins; fall back to the
     * LBA1 "EFI PART" signature when the protective entry was torn */
    if (lba0[510] == 0x55 && lba0[511] == 0xAA) {
        for (u32 e = 0; e < 4; ++e) {
            const u8* entry = lba0 + 446 + e * 16;
            if (entry[4] == 0xEE)
                return BootLayout::Gpt;
        }
        if (dev->read_at(512, lba1, sizeof(lba1)) == OK &&
            std::memcmp(lba1, "EFI PART", 8) == 0)
            return BootLayout::Gpt;
        return BootLayout::Mbr;
    }
    if (dev->read_at(512, lba1, sizeof(lba1)) == OK && std::memcmp(lba1, "EFI PART", 8) == 0)
        return BootLayout::Gpt; /* torn MBR, intact GPT header */
    return BootLayout::Unknown;
}

} // namespace storage
} // namespace cr
