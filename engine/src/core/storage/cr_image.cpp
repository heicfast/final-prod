/*
 * cr_image.cpp - disk-image scan targets (S-030): flat / VMDK / VHD / VHDX.
 *
 * Clean-room implementation against the public on-disk formats, empirically
 * verified structure-by-structure against qemu-img 10.0.13-produced images
 * (VMDK sparse v1 header/grain directory/grain table/grain data, VHD dynamic
 * footer/dynamic-header/BAT/block-bitmap, VHDX header pair/region table/
 * metadata/BAT/log), matching the og's dispatch semantics (corpus:
 * DRWUI_decompiled.c :948237-948409, :275877-275880; Device.mo's libvmdk
 * block device :49106-49124 with extent files + parent chains :48965-49100).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_image.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace cr {
namespace storage {

/* ------------------------------------------------------------------ */
/* helpers                                                             */
/* ------------------------------------------------------------------ */

namespace {

inline u16 rd16(const u8* p) {
    return u16(p[0]) | (u16(p[1]) << 8);
}
inline u32 rd32(const u8* p) {
    return u32(p[0]) | (u32(p[1]) << 8) | (u32(p[2]) << 16) | (u32(p[3]) << 24);
}
inline u64 rd64(const u8* p) {
    return u64(rd32(p)) | (u64(rd32(p + 4)) << 32);
}
inline u32 be32(const u8* p) {
    return (u32(p[0]) << 24) | (u32(p[1]) << 16) | (u32(p[2]) << 8) | u32(p[3]);
}
inline u64 be64(const u8* p) {
    return (u64(be32(p)) << 32) | u64(be32(p + 4));
}
inline u16 be16(const u8* p) {
    return (u16(p[0]) << 8) | u16(p[1]);
}

/* UTF-16 (host-endian std::u16string) <-> UTF-8, surrogate-aware;
 * image paths are overwhelmingly ASCII, non-ASCII round-trips correctly. */
std::u16string utf8_to_u16(const char* s, size_t len) {
    std::u16string out;
    out.reserve(len);
    for (size_t i = 0; i < len;) {
        const auto b0 = u8(s[i]);
        if (b0 < 0x80) {
            out.push_back(char16_t(b0));
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < len) {
            out.push_back(char16_t(((b0 & 0x1F) << 6) | (u8(s[i + 1]) & 0x3F)));
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < len) {
            out.push_back(char16_t(((b0 & 0x0F) << 12) | ((u8(s[i + 1]) & 0x3F) << 6) |
                                   (u8(s[i + 2]) & 0x3F)));
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0 && i + 3 < len) {
            const u32 cp = ((u32(b0 & 0x07) << 18) | (u32(u8(s[i + 1]) & 0x3F) << 12) |
                            (u32(u8(s[i + 2]) & 0x3F) << 6) | u32(u8(s[i + 3]) & 0x3F));
            out.push_back(char16_t(0xD800 + ((cp - 0x10000) >> 10)));
            out.push_back(char16_t(0xDC00 + ((cp - 0x10000) & 0x3FF)));
            i += 4;
        } else {
            out.push_back(u'?');
            i += 1;
        }
    }
    return out;
}

/* UTF-16BE (VHD parent-locator strings) -> host u16string. */
std::u16string be16_to_u16(const u8* p, u32 bytes) {
    std::u16string out;
    out.reserve(bytes / 2);
    for (u32 i = 0; i + 1 < bytes; i += 2)
        out.push_back(char16_t(be16(p + i)));
    return out;
}

bool path_is_absolute(const std::u16string& p) {
    if (p.size() >= 2 && p[1] == u':')
        return true;
    if (!p.empty() && (p[0] == u'\\' || p[0] == u'/'))
        return true;
    return false;
}

/* Directory prefix (including the final separator; empty when none). */
std::u16string dir_of(const std::u16string& p) {
    const size_t slash = p.find_last_of(u"/\\");
    if (slash == std::u16string::npos)
        return std::u16string();
    return p.substr(0, slash + 1);
}
std::u16string join_path(const std::u16string& dir, const std::u16string& name) {
    if (dir.empty() || path_is_absolute(name))
        return name;
    return dir + name;
}

constexpr u32 SECTOR = 512;

/* A file held open for the device's lifetime (RAII over the IO seam). */
struct HeldFile {
    ImageFileIo* io = nullptr;
    u64 handle = 0;
    u64 bytes = 0;
    HeldFile() = default;
    HeldFile(const HeldFile&) = delete;
    HeldFile& operator=(const HeldFile&) = delete;
    HeldFile(HeldFile&& o) noexcept : io(o.io), handle(o.handle), bytes(o.bytes) {
        o.io = nullptr;
        o.handle = 0;
    }
    HeldFile& operator=(HeldFile&& o) noexcept {
        if (this != &o) {
            if (io && handle)
                io->close(handle);
            io = o.io;
            handle = o.handle;
            bytes = o.bytes;
            o.io = nullptr;
            o.handle = 0;
        }
        return *this;
    }
    ~HeldFile() {
        if (io && handle)
            io->close(handle);
    }
};

u32 io_read_exact(ImageFileIo& io, u64 h, u64 offset, void* buf, u32 len) {
    return len == 0 ? OK : io.read(h, offset, buf, len);
}

std::unique_ptr<IStorageDevice> open_image_impl(ImageFileIo& io, const std::u16string& path,
                                                u32 depth, u32* err);

/* ------------------------------------------------------------------ */
/* FlatImageDevice - the og's flat fallback (CreateFileW + GetFileSizeEx)*/
/* ------------------------------------------------------------------ */

class FlatImageDevice final : public IStorageDevice {
public:
    /* N-072: image files are static media - a torn label cannot heal */
    bool is_static_media() const override { return true; }

    FlatImageDevice(HeldFile file, u32 logical_sector) : file_(std::move(file)) {
        geom_.logical_sector = logical_sector;
        geom_.physical_sector = logical_sector;
        geom_.total_bytes = file_.bytes;
        geom_.sector_count = logical_sector ? file_.bytes / logical_sector : 0;
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > file_.bytes || len > file_.bytes - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        return io_read_exact(*file_.io, file_.handle, off, buf, len);
    }
    const Geometry& geometry() const override { return geom_; }

private:
    HeldFile file_;
    Geometry geom_;
};

/* ------------------------------------------------------------------ */
/* VMDK                                                                */
/* ------------------------------------------------------------------ */

constexpr u32 VMDK_MAGIC = 0x564D444B; /* "KDMV" bytes, LE u32 */

struct VmdkSparseHeader {
    u32 version = 0;
    u64 capacity = 0;        /* sectors */
    u64 grain_size = 0;      /* sectors, power of two */
    u64 descriptor_off = 0;  /* sectors */
    u64 descriptor_size = 0; /* sectors */
    u32 num_gtes = 0;        /* entries per grain table */
    u64 gd_offset = 0;       /* sectors */
    bool valid = false;
};

/* Packed little-endian layout (empirically pinned against qemu output):
 * magic@0, version@4, flags@8, capacity@0xC, grainSize@0x14,
 * descriptorOffset@0x1C, descriptorSize@0x24, numGTEsPerGT@0x2C,
 * rgdOffset@0x30, gdOffset@0x38, overhead@0x40; 512 bytes total. */
VmdkSparseHeader parse_vmdk_sparse_header(const u8* p, size_t len) {
    VmdkSparseHeader h;
    if (len < SECTOR || rd32(p) != VMDK_MAGIC)
        return h;
    h.version = rd32(p + 4);
    if (h.version < 1 || h.version > 3)
        return h;
    h.capacity = rd64(p + 0x0C);
    h.grain_size = rd64(p + 0x14);
    h.descriptor_off = rd64(p + 0x1C);
    h.descriptor_size = rd64(p + 0x24);
    h.num_gtes = rd32(p + 0x2C);
    h.gd_offset = rd64(p + 0x38);
    if (h.capacity == 0 || h.grain_size == 0 || h.num_gtes == 0)
        return h;
    if ((h.grain_size & (h.grain_size - 1)) != 0)
        return h; /* spec: power of two */
    h.valid = true;
    return h;
}

enum class VmdkExtentKind : u8 { Flat, Sparse, Zero };

struct VmdkExtent {
    HeldFile file;
    VmdkExtentKind kind = VmdkExtentKind::Flat;
    u64 start_sector = 0; /* virtual first sector */
    u64 sectors = 0;      /* extent capacity in sectors */
    u64 flat_offset = 0;  /* bytes into the file (FLAT with offset) */
    VmdkSparseHeader sparse;
    std::vector<u32> gd; /* grain directory (sector values), loaded at open */
    /* grain-table cache (2 KB per GT, bounded - images stay scan-sized).
     * Heap-held: std::mutex is not movable and extents live in vectors. */
    struct GtCache {
        mutable std::mutex mu;
        mutable std::unordered_map<u32, std::vector<u32>> map;
    };
    std::unique_ptr<GtCache> gt_cache = std::make_unique<GtCache>();

    const std::vector<u32>* grain_table(u32 gde) const {
        if (gde >= gd.size() || gd[gde] == 0)
            return nullptr;
        {
            std::lock_guard<std::mutex> lock(gt_cache->mu);
            const auto it = gt_cache->map.find(gde);
            if (it != gt_cache->map.end())
                return &it->second;
        }
        std::vector<u32> gt(sparse.num_gtes, 0);
        const u64 off = u64(gd[gde]) * SECTOR;
        const u32 want = sparse.num_gtes * 4;
        if (io_read_exact(*file.io, file.handle, off, gt.data(), want) != OK)
            return nullptr;
        std::lock_guard<std::mutex> lock(gt_cache->mu);
        if (gt_cache->map.size() >= 256) /* 512 KB bound */
            gt_cache->map.clear();
        const auto res = gt_cache->map.emplace(gde, std::move(gt));
        return &res.first->second;
    }
};

struct VmdkDescriptor {
    bool has_parent = false;    /* parentFileNameHint (differencing) */
    std::u16string parent_hint; /* resolved path */
    std::vector<VmdkExtent> extents;
    u64 total_sectors = 0;
};

/* Parses one extent line: `RW 8192 SPARSE "disk-s001.vmdk" [offset]`.
 * Returns false on a hard error (missing file / bad magic / corruption /
 * unsupported family); true when the line was skipped or parsed. */
bool parse_extent_line(const std::string& line, const std::u16string& base_dir, ImageFileIo& io,
                       std::vector<VmdkExtent>& out, u32* err) {
    std::vector<std::string> toks;
    std::string cur;
    bool in_quote = false;
    for (char c : line) {
        if (c == '"') {
            in_quote = !in_quote;
            continue;
        }
        if (!in_quote && (c == ' ' || c == '\t')) {
            if (!cur.empty())
                toks.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty())
        toks.push_back(cur);
    if (toks.size() < 3)
        return true; /* key=value or comment - not an extent line */

    const std::string& access = toks[0];
    const std::string& type = toks[2];
    if (access != "RW" && access != "RDONLY" && access != "NOACCESS")
        return true;
    if (type != "FLAT" && type != "SPARSE" && type != "ZERO" && type != "VMFS" &&
        type != "VMFSSPARSE" && type != "VMFSRDM" && type != "VMFSRAW")
        return true;

    if (type == "VMFS" || type == "VMFSSPARSE" || type == "VMFSRDM" || type == "VMFSRAW") {
        *err = ERR_UNSUPPORTED; /* ESX family: registered exclusion */
        return false;
    }

    char* endp = nullptr;
    const unsigned long long sectors = std::strtoull(toks[1].c_str(), &endp, 10);
    if (endp == toks[1].c_str())
        return true;

    VmdkExtent ext;
    if (type == "ZERO") {
        ext.kind = VmdkExtentKind::Zero;
    } else {
        if (toks.size() < 4) {
            *err = ERR_IMAGE_CORRUPT;
            return false;
        }
        const std::u16string path =
            join_path(base_dir, utf8_to_u16(toks[3].data(), toks[3].size()));
        u32 e = 0;
        const u64 h = io.open(path, &e);
        if (h == 0) {
            *err = ERR_OPEN_FAILED; /* missing extent data file */
            return false;
        }
        ext.file.io = &io;
        ext.file.handle = h;
        ext.file.bytes = io.size(h);
        if (type == "FLAT") {
            ext.kind = VmdkExtentKind::Flat;
            if (toks.size() >= 5) {
                char* end2 = nullptr;
                const unsigned long long off_sectors = std::strtoull(toks[4].c_str(), &end2, 10);
                ext.flat_offset = (end2 == toks[4].c_str()) ? 0 : off_sectors * SECTOR;
            }
        } else { /* SPARSE */
            u8 hdr[SECTOR];
            if (ext.file.bytes < SECTOR || io_read_exact(io, h, 0, hdr, SECTOR) != OK) {
                *err = ERR_IMAGE_CORRUPT;
                return false;
            }
            ext.sparse = parse_vmdk_sparse_header(hdr, SECTOR);
            if (!ext.sparse.valid) {
                *err = ERR_BAD_MAGIC; /* the og: "Unsupported or corrupt VMDK" */
                return false;
            }
            const u64 grains =
                (ext.sparse.capacity + ext.sparse.grain_size - 1) / ext.sparse.grain_size;
            const u64 gdes = (grains + ext.sparse.num_gtes - 1) / ext.sparse.num_gtes;
            if (gdes > (1u << 24) || gdes * 4 + ext.sparse.gd_offset * SECTOR > ext.file.bytes) {
                *err = ERR_IMAGE_CORRUPT;
                return false;
            }
            ext.gd.resize(size_t(gdes), 0);
            if (gdes && io_read_exact(io, h, ext.sparse.gd_offset * SECTOR, ext.gd.data(),
                                      u32(gdes * 4)) != OK) {
                *err = ERR_IMAGE_CORRUPT;
                return false;
            }
            ext.kind = VmdkExtentKind::Sparse;
        }
    }
    ext.sectors = sectors;
    out.push_back(std::move(ext));
    return true;
}

VmdkDescriptor parse_vmdk_descriptor(const std::string& text, const std::u16string& base_dir,
                                     ImageFileIo& io, u32* err, bool parse_extents = true) {
    VmdkDescriptor d;
    std::string create_type;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t eol = text.find('\n', pos);
        if (eol == std::string::npos)
            eol = text.size();
        std::string line = text.substr(pos, eol - pos);
        pos = eol + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        if (line.empty() || line[0] == '#')
            continue;
        if (line.rfind("createType", 0) == 0 && line.size() > 11 && line[10] == '=') {
            create_type = line.substr(11);
            /* the value is quoted: createType="monolithicSparse" */
            if (create_type.size() >= 2 && create_type.front() == '"' && create_type.back() == '"')
                create_type = create_type.substr(1, create_type.size() - 2);
        } else if (line.rfind("parentFileNameHint", 0) == 0) {
            const size_t q1 = line.find('"');
            const size_t q2 = line.find('"', q1 == std::string::npos ? 0 : q1 + 1);
            if (q1 != std::string::npos && q2 != std::string::npos && q2 > q1) {
                d.parent_hint = join_path(base_dir, utf8_to_u16(line.data() + q1 + 1, q2 - q1 - 1));
                d.has_parent = true;
            }
        } else if (parse_extents) {
            if (!parse_extent_line(line, base_dir, io, d.extents, err))
                return d; /* *err set by parse_extent_line */
        }
    }
    /* compressed streams and ESX families: refused, never silently mis-read
     * (registered in PARITY-DELTAS) */
    if (create_type == "streamOptimized" || create_type == "streamMonolithic" ||
        create_type == "vmfs" || create_type == "vmfsSparse" || create_type == "vmfsRdm" ||
        create_type == "vmfsRaw") {
        *err = ERR_UNSUPPORTED;
        return d;
    }
    for (const auto& e : d.extents)
        d.total_sectors += e.sectors;
    if (parse_extents && d.extents.empty())
        *err = ERR_IMAGE_CORRUPT; /* descriptor with no extents */
    return d;
}

class VmdkDevice final : public IStorageDevice {
public:
    /* N-072: image files are static media - a torn label cannot heal */
    bool is_static_media() const override { return true; }

    VmdkDevice(std::vector<VmdkExtent> extents, u64 total_sectors,
               std::unique_ptr<IStorageDevice> parent)
        : extents_(std::move(extents)), parent_(std::move(parent)) {
        geom_.logical_sector = SECTOR;
        geom_.physical_sector = SECTOR;
        geom_.total_bytes = total_sectors * SECTOR;
        geom_.sector_count = total_sectors;
    }

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > geom_.total_bytes || len > geom_.total_bytes - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        auto* out = static_cast<u8*>(buf);
        u64 done = 0;
        while (done < len) {
            const u64 pos = off + done;
            const u64 vsec = pos / SECTOR;
            const VmdkExtent* ext = find_extent(vsec);
            if (!ext)
                return ERR_IMAGE_CORRUPT;
            const u64 local_sector = vsec - ext->start_sector;
            /* contiguous chunk: bounded by extent end (flat/zero) or the
             * grain end (sparse) - keeps the hot path multi-sector */
            u64 limit;
            if (ext->kind == VmdkExtentKind::Sparse)
                limit = (ext->start_sector +
                         ((local_sector / ext->sparse.grain_size) + 1) * ext->sparse.grain_size) *
                        SECTOR;
            else
                limit = (ext->start_sector + ext->sectors) * SECTOR;
            const u32 chunk = u32(std::min<u64>(len - done, limit - pos));
            u32 rc = OK;
            switch (ext->kind) {
            case VmdkExtentKind::Zero:
                std::memset(out + done, 0, chunk);
                break;
            case VmdkExtentKind::Flat:
                rc = io_read_exact(*ext->file.io, ext->file.handle,
                                   ext->flat_offset + local_sector * SECTOR + pos % SECTOR,
                                   out + done, chunk);
                break;
            case VmdkExtentKind::Sparse:
                rc = read_sparse(*ext, local_sector, pos, out + done, chunk);
                break;
            }
            if (rc != OK)
                return rc;
            done += chunk;
        }
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    const VmdkExtent* find_extent(u64 vsec) const {
        for (const auto& e : extents_)
            if (vsec >= e.start_sector && vsec < e.start_sector + e.sectors)
                return &e;
        return nullptr;
    }

    u32 read_sparse(const VmdkExtent& e, u64 local_sector, u64 virt_off, u8* out, u32 chunk) {
        const u64 grain = local_sector / e.sparse.grain_size;
        const u32 gde = u32(grain / e.sparse.num_gtes);
        const std::vector<u32>* gt = e.grain_table(gde);
        const u32 gte = gt ? (*gt)[grain % e.sparse.num_gtes] : 0;
        if (gte == 0) {
            /* unallocated grain: the parent chain serves it (differencing
             * disks - Device.mo's recursive parent model), else zeros */
            if (parent_)
                return parent_->read_at(virt_off, out, chunk);
            std::memset(out, 0, chunk);
            return OK;
        }
        const u64 in_grain = (local_sector % e.sparse.grain_size) * SECTOR + virt_off % SECTOR;
        return io_read_exact(*e.file.io, e.file.handle, u64(gte) * SECTOR + in_grain, out, chunk);
    }

    std::vector<VmdkExtent> extents_;
    std::unique_ptr<IStorageDevice> parent_;
    Geometry geom_;
};

std::unique_ptr<IStorageDevice> open_vmdk(ImageFileIo& io, const std::u16string& path, u32 depth,
                                          u32* err) {
    if (depth > 32) {
        *err = ERR_DEPTH_LIMIT;
        return nullptr;
    }
    HeldFile file;
    file.io = &io;
    u32 e = 0;
    file.handle = io.open(path, &e);
    if (file.handle == 0) {
        *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    file.bytes = io.size(file.handle);
    if (file.bytes < SECTOR) {
        *err = ERR_TRUNCATED;
        return nullptr;
    }

    u8 head[SECTOR];
    if (io_read_exact(io, file.handle, 0, head, SECTOR) != OK) {
        *err = ERR_READ_FAILED;
        return nullptr;
    }

    const std::u16string base_dir = dir_of(path);
    VmdkDescriptor desc;

    if (rd32(head) == VMDK_MAGIC) {
        /* monolithic sparse: this file is the (single) sparse extent; the
         * embedded descriptor (when present) is at descriptorOffset */
        const VmdkSparseHeader sh = parse_vmdk_sparse_header(head, SECTOR);
        if (!sh.valid) {
            *err = ERR_BAD_MAGIC;
            return nullptr;
        }
        VmdkExtent ext;
        ext.file = std::move(file);
        ext.kind = VmdkExtentKind::Sparse;
        ext.sectors = sh.capacity;
        ext.sparse = sh;
        const u64 grains = (sh.capacity + sh.grain_size - 1) / sh.grain_size;
        const u64 gdes = (grains + sh.num_gtes - 1) / sh.num_gtes;
        if (gdes > (1u << 24) || gdes * 4 + sh.gd_offset * SECTOR > ext.file.bytes) {
            *err = ERR_IMAGE_CORRUPT;
            return nullptr;
        }
        ext.gd.resize(size_t(gdes), 0);
        if (gdes && io_read_exact(io, ext.file.handle, sh.gd_offset * SECTOR, ext.gd.data(),
                                  u32(gdes * 4)) != OK) {
            *err = ERR_IMAGE_CORRUPT;
            return nullptr;
        }
        std::vector<VmdkExtent> exts;
        exts.push_back(std::move(ext));

        /* embedded descriptor: createType + parentFileNameHint (the extent
         * list of a monolithic image is the file itself; refusal families
         * like streamOptimized are honored from the strict parse) */
        if (sh.descriptor_size && sh.descriptor_off) {
            const u64 dbytes = sh.descriptor_size * SECTOR;
            if (sh.descriptor_off * SECTOR + dbytes > exts[0].file.bytes) {
                *err = ERR_IMAGE_CORRUPT;
                return nullptr;
            }
            std::vector<char> text(size_t(dbytes) + 1, 0);
            if (io_read_exact(io, exts[0].file.handle, sh.descriptor_off * SECTOR, text.data(),
                              u32(dbytes)) == OK) {
                text[size_t(dbytes)] = 0;
                u32 derr = OK;
                const VmdkDescriptor d =
                    parse_vmdk_descriptor(std::string(text.data()), base_dir, io, &derr, false);
                if (derr != OK) {
                    *err = derr; /* streamOptimized / VMFS / corrupt keys */
                    return nullptr;
                }
                desc.has_parent = d.has_parent;
                desc.parent_hint = d.parent_hint;
            }
        }
        desc.extents = std::move(exts);
        desc.total_sectors = sh.capacity;
    } else if (head[0] == '#' || head[0] == 'R' || head[0] == 'C') {
        /* descriptor file: read it whole (bounded) and parse */
        const u64 dmax = std::min<u64>(file.bytes, 1 << 20);
        std::vector<char> text(size_t(dmax) + 1, 0);
        if (io_read_exact(io, file.handle, 0, text.data(), u32(dmax)) != OK) {
            *err = ERR_READ_FAILED;
            return nullptr;
        }
        text[size_t(dmax)] = 0;
        const std::string s(text.data());
        if (s.rfind("# Disk DescriptorFile", 0) != 0 && s.rfind("RW ", 0) != 0 &&
            s.rfind("RDONLY ", 0) != 0) {
            *err = ERR_BAD_MAGIC;
            return nullptr;
        }
        u32 derr = OK;
        desc = parse_vmdk_descriptor(s, base_dir, io, &derr);
        if (derr != OK) {
            *err = derr;
            return nullptr;
        }
        if (desc.extents.empty()) {
            *err = ERR_IMAGE_CORRUPT;
            return nullptr;
        }
    } else {
        *err = ERR_BAD_MAGIC; /* the og's "Unsupported or corrupt VMDK" */
        return nullptr;
    }

    /* differencing chain: unallocated grains read from the parent */
    std::unique_ptr<IStorageDevice> parent;
    if (desc.has_parent) {
        if (desc.parent_hint.empty()) {
            *err = ERR_IMAGE_CORRUPT;
            return nullptr;
        }
        u32 perr = 0;
        parent = open_image_impl(io, desc.parent_hint, depth + 1, &perr);
        if (!parent) {
            *err = perr;
            return nullptr;
        }
    }

    return std::unique_ptr<IStorageDevice>(
        new VmdkDevice(std::move(desc.extents), desc.total_sectors, std::move(parent)));
}

/* ------------------------------------------------------------------ */
/* VHD                                                                 */
/* ------------------------------------------------------------------ */

constexpr u32 VHD_DISK_FIXED = 2;
constexpr u32 VHD_DISK_DYNAMIC = 3;
constexpr u32 VHD_DISK_DIFFERENCING = 4;

struct VhdFooter {
    u64 data_offset = 0;
    u64 current_size = 0;
    u32 disk_type = 0;
    bool valid = false;
};

/* VHD one's-complement rule (verified against qemu-written footers):
 * stored == ~S where S = plain byte sum of the structure with the
 * checksum field zeroed. The comparison is 32-bit, NOT byte-wise. */
bool vhd_sum_ok(const u8* p, size_t len, size_t checksum_off) {
    u32 sum = 0;
    for (size_t i = 0; i < len; ++i)
        if (i < checksum_off || i >= checksum_off + 4)
            sum += p[i];
    const u32 stored = be32(p + checksum_off);
    return stored == 0xFFFFFFFFu - sum;
}

VhdFooter parse_vhd_footer(const u8* p) {
    VhdFooter f;
    if (std::memcmp(p, "conectix", 8) != 0)
        return f;
    if (!vhd_sum_ok(p, SECTOR, 0x40))
        return f;
    f.data_offset = be64(p + 0x10);
    f.current_size = be64(p + 0x30);
    f.disk_type = be32(p + 0x3C);
    f.valid = true;
    return f;
}

class VhdDevice final : public IStorageDevice {
public:
    /* N-072: image files are static media - a torn label cannot heal */
    bool is_static_media() const override { return true; }

    VhdDevice(HeldFile file, u32 disk_type, u64 current_size, u32 block_size, std::vector<u32> bat,
              std::unique_ptr<IStorageDevice> parent)
        : file_(std::move(file)), disk_type_(disk_type), block_size_(block_size),
          bat_(std::move(bat)), parent_(std::move(parent)) {
        geom_.logical_sector = SECTOR;
        geom_.physical_sector = SECTOR;
        geom_.total_bytes = current_size;
        geom_.sector_count = current_size / SECTOR;
    }

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > geom_.total_bytes || len > geom_.total_bytes - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (disk_type_ == VHD_DISK_FIXED)
            return io_read_exact(*file_.io, file_.handle, off, buf, len);
        auto* out = static_cast<u8*>(buf);
        u64 done = 0;
        while (done < len) {
            const u64 pos = off + done;
            const u64 block = pos / block_size_;
            if (block >= bat_.size())
                return ERR_IMAGE_CORRUPT;
            const u32 entry = bat_[size_t(block)];
            const u64 in_block = pos % block_size_;
            const u32 chunk = u32(std::min<u64>(len - done, block_size_ - in_block));
            u32 rc = OK;
            if (entry == 0xFFFFFFFF) {
                if (disk_type_ == VHD_DISK_DIFFERENCING && parent_)
                    rc = parent_->read_at(pos, out + done, chunk);
                else
                    std::memset(out + done, 0, chunk); /* unallocated */
            } else if (disk_type_ == VHD_DISK_DIFFERENCING) {
                rc = read_diff_block(u64(entry) * SECTOR, in_block, pos, out + done, chunk);
            } else {
                rc = io_read_exact(*file_.io, file_.handle, u64(entry) * SECTOR + SECTOR + in_block,
                                   out + done, chunk);
                /* a dynamic block's unwritten tail may sit past EOF in
                 * files trimmed by the writer; those sectors read as zeros
                 * (dynamic-VHD semantics). Real IO errors propagate. */
                if (rc == ERR_RANGE_OUT_OF_BOUNDS || rc == ERR_TRUNCATED) {
                    std::memset(out + done, 0, chunk);
                    rc = OK;
                }
            }
            if (rc != OK)
                return rc;
            done += chunk;
        }
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    u32 read_diff_block(u64 block_file, u64 in_block, u64 virt_off, u8* out, u32 chunk) {
        /* sector bitmap: 512 bytes, byte i bit j = sector i*8+j; set =
         * child owns the sector, clear = parent serves it */
        const u64 sector_in_block = in_block / SECTOR;
        u8 bitmap[SECTOR];
        if (bitmap_block_ != block_file) {
            if (io_read_exact(*file_.io, file_.handle, block_file, bitmap, SECTOR) != OK)
                return ERR_READ_FAILED;
            std::memcpy(bitmap_cache_, bitmap, SECTOR);
            bitmap_block_ = block_file;
        }
        const u8 bits = bitmap_cache_[size_t(sector_in_block / 8)];
        if ((bits & (1u << (sector_in_block % 8))) == 0) {
            if (!parent_)
                return ERR_IMAGE_CORRUPT;
            return parent_->read_at(virt_off, out, chunk);
        }
        return io_read_exact(*file_.io, file_.handle, block_file + SECTOR + in_block, out, chunk);
    }

    HeldFile file_;
    u32 disk_type_ = VHD_DISK_DYNAMIC;
    u32 block_size_ = 0;
    std::vector<u32> bat_;
    std::unique_ptr<IStorageDevice> parent_;
    Geometry geom_;
    /* whole-block bitmap cache (single-block working set; correct because
     * bitmap_block_ identifies the cached block) */
    u64 bitmap_block_ = ~u64(0);
    u8 bitmap_cache_[SECTOR] = {0};
};

std::unique_ptr<IStorageDevice> open_vhd(ImageFileIo& io, const std::u16string& path, u32 depth,
                                         u32* err) {
    if (depth > 32) {
        *err = ERR_DEPTH_LIMIT;
        return nullptr;
    }
    HeldFile file;
    file.io = &io;
    u32 e = 0;
    file.handle = io.open(path, &e);
    if (file.handle == 0) {
        *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    file.bytes = io.size(file.handle);
    if (file.bytes < SECTOR) {
        *err = ERR_TRUNCATED;
        return nullptr;
    }

    /* the footer lives at the end; a copy sits at offset 0 (dynamic disks)
     * - prefer the tail footer, fall back to the head copy */
    u8 footer[SECTOR];
    if (io_read_exact(io, file.handle, file.bytes - SECTOR, footer, SECTOR) != OK) {
        *err = ERR_READ_FAILED;
        return nullptr;
    }
    VhdFooter f = parse_vhd_footer(footer);
    if (!f.valid && file.bytes >= 2 * SECTOR) {
        if (io_read_exact(io, file.handle, 0, footer, SECTOR) == OK)
            f = parse_vhd_footer(footer);
    }
    if (!f.valid) {
        *err = ERR_BAD_CHECKSUM;
        return nullptr;
    }
    if (f.disk_type == VHD_DISK_FIXED) {
        if (f.current_size > file.bytes) {
            *err = ERR_TRUNCATED;
            return nullptr;
        }
        return std::unique_ptr<IStorageDevice>(new FlatImageDevice(std::move(file), SECTOR));
    }
    if (f.disk_type != VHD_DISK_DYNAMIC && f.disk_type != VHD_DISK_DIFFERENCING) {
        *err = ERR_UNSUPPORTED;
        return nullptr;
    }

    /* dynamic disk header at footer->dataOffset */
    u8 dh[1024];
    if (f.data_offset == 0xFFFFFFFFFFFFFFFFULL || f.data_offset + sizeof(dh) > file.bytes ||
        io_read_exact(io, file.handle, f.data_offset, dh, sizeof(dh)) != OK) {
        *err = ERR_IMAGE_CORRUPT;
        return nullptr;
    }
    if (std::memcmp(dh, "cxsparse", 8) != 0) {
        *err = ERR_BAD_MAGIC;
        return nullptr;
    }
    if (!vhd_sum_ok(dh, sizeof(dh), 0x24)) {
        *err = ERR_BAD_CHECKSUM;
        return nullptr;
    }
    const u64 table_offset = be64(dh + 0x10);
    const u32 max_entries = be32(dh + 0x1C);
    const u32 block_size = be32(dh + 0x20);
    if (block_size == 0 || block_size % SECTOR != 0 || max_entries == 0 ||
        max_entries > (1u << 24) || table_offset + u64(max_entries) * 4 > file.bytes) {
        *err = ERR_IMAGE_CORRUPT;
        return nullptr;
    }
    std::vector<u8> bat_raw(u64(max_entries) * 4);
    if (io_read_exact(io, file.handle, table_offset, bat_raw.data(), u32(bat_raw.size())) != OK) {
        *err = ERR_READ_FAILED;
        return nullptr;
    }
    std::vector<u32> bat(max_entries);
    for (u32 i = 0; i < max_entries; ++i)
        bat[i] = be32(bat_raw.data() + u64(i) * 4);

    /* differencing: resolve the parent via W2ku/W2ru locators (UTF-16BE) */
    std::unique_ptr<IStorageDevice> parent;
    if (f.disk_type == VHD_DISK_DIFFERENCING) {
        std::u16string parent_path;
        for (u32 li = 0; li < 8; ++li) {
            const u8* loc = dh + 0x240 + u64(li) * 24;
            const u32 platform = be32(loc);
            const u32 data_len = be32(loc + 8);
            const u64 data_off = be64(loc + 16);
            if (platform != 0x57326B75 /* W2ku */ && platform != 0x57327275 /* W2ru */)
                continue;
            if (data_len == 0 || data_len > 4096 || data_len % 2 != 0 ||
                data_off + data_len > file.bytes)
                continue;
            std::vector<u8> raw(data_len);
            if (io_read_exact(io, file.handle, data_off, raw.data(), data_len) != OK)
                continue;
            parent_path = be16_to_u16(raw.data(), data_len);
            break;
        }
        if (parent_path.empty()) {
            *err = ERR_IMAGE_CORRUPT; /* differencing without a usable parent */
            return nullptr;
        }
        const std::u16string resolved =
            path_is_absolute(parent_path) ? parent_path : join_path(dir_of(path), parent_path);
        u32 perr = 0;
        parent = open_image_impl(io, resolved, depth + 1, &perr);
        if (!parent) {
            *err = perr;
            return nullptr;
        }
    }

    return std::unique_ptr<IStorageDevice>(new VhdDevice(std::move(file), f.disk_type,
                                                         f.current_size, block_size, std::move(bat),
                                                         std::move(parent)));
}

/* ------------------------------------------------------------------ */
/* VHDX                                                                */
/* ------------------------------------------------------------------ */

constexpr u64 VHDX_FILE_SIG = 0x656C696678646876ULL; /* "vhdxfile" */
constexpr u32 VHDX_HEAD_SIG = 0x64616568;            /* "head" */
constexpr u32 VHDX_REGI_SIG = 0x69676572;            /* "regi" */
constexpr u64 VHDX_META_SIG = 0x617461646174656DULL; /* "metadata" */
constexpr u32 VHDX_LOGE_SIG = 0x65676F6C;            /* "loge" */
constexpr u32 VHDX_DESC_SIG = 0x63736564;            /* "desc" */
constexpr u32 VHDX_ZERO_SIG = 0x6F72657A;            /* "zero" */
constexpr u32 VHDX_DATA_SIG = 0x61746164;            /* "data" */
constexpr u64 VHDX_BAT_FILE_OFF_MASK = 0xFFFFFFFFFFF00000ULL;
constexpr u32 VHDX_BAT_STATE_MASK = 0x07;

/* CRC-32C (Castagnoli, reflected poly 0x82F63B78) - the VHDX checksum. */
struct Crc32cTable {
    u32 t[256];
    Crc32cTable() {
        for (u32 i = 0; i < 256; ++i) {
            u32 c = i;
            for (int k = 0; k < 8; ++k)
                c = (c >> 1) ^ (0x82F63B78u & (0u - (c & 1)));
            t[i] = c;
        }
    }
};
u32 crc32c(const u8* p, size_t len) {
    static const Crc32cTable tab;
    u32 crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        crc = tab.t[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}
u32 crc32c_skip4(const u8* p, size_t len) {
    /* checksum with the u32 at offset 4 treated as zero (header entries) */
    u32 crc = 0xFFFFFFFFu;
    static const Crc32cTable tab;
    const u8 zeros[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < len; ++i) {
        const u8 b = (i >= 4 && i < 8) ? zeros[i - 4] : p[i];
        crc = tab.t[(crc ^ b) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

struct VhdxHeaderInfo {
    u64 log_offset = 0;
    u32 log_length = 0;
    u8 log_guid[16] = {0};
    bool log_guid_zero = true;
};

bool vhdx_header_parse(const u8* blk, VhdxHeaderInfo* out, u64* sequence) {
    if (rd32(blk) != VHDX_HEAD_SIG)
        return false;
    if (crc32c_skip4(blk, 4096) != rd32(blk + 4))
        return false;
    /* spec: log version 0, file version 1 */
    if (rd16(blk + 0x40) != 0 || rd16(blk + 0x42) != 1)
        return false;
    *sequence = rd64(blk + 8);
    out->log_length = rd32(blk + 0x44);
    out->log_offset = rd64(blk + 0x48);
    std::memcpy(out->log_guid, blk + 0x30, 16);
    out->log_guid_zero = true;
    for (u8 b : out->log_guid)
        if (b)
            out->log_guid_zero = false;
    return true;
}

/* in-memory log overlay: byte ranges served from the log instead of the
 * file body (read-only replay - the input file is never modified, the
 * forensic posture a recovery tool owes its evidence) */
struct LogOverlayRange {
    u64 offset;
    u64 length;
    std::vector<u8> bytes; /* empty = zeros */
};

class VhdxDevice final : public IStorageDevice {
public:
    /* N-072: image files are static media - a torn label cannot heal */
    bool is_static_media() const override { return true; }

    VhdxDevice(HeldFile file, u64 virt_size, u32 block_size, u32 logical_sector,
               std::vector<u64> bat, u32 chunk_ratio_bits, std::vector<LogOverlayRange> overlay)
        : file_(std::move(file)), bat_(std::move(bat)), block_size_(block_size),
          chunk_ratio_bits_(chunk_ratio_bits), overlay_(std::move(overlay)) {
        geom_.logical_sector = logical_sector;
        geom_.physical_sector = logical_sector;
        geom_.total_bytes = virt_size;
        geom_.sector_count = logical_sector ? virt_size / logical_sector : 0;
    }

    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > geom_.total_bytes || len > geom_.total_bytes - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        auto* out = static_cast<u8*>(buf);
        u64 done = 0;
        while (done < len) {
            const u64 pos = off + done;
            const u64 block = pos / block_size_;
            if (block >= bat_.size()) {
                /* virtual size is BAT-covered by construction; being here
                 * means a malformed image - refuse, never fabricate */
                return ERR_IMAGE_CORRUPT;
            }
            const u64 entry = bat_[size_t(block)];
            const u32 state = u32(entry & VHDX_BAT_STATE_MASK);
            const u64 in_block = pos % block_size_;
            const u32 chunk = u32(std::min<u64>(len - done, block_size_ - in_block));
            u32 rc = OK;
            if (state == 6) { /* PAYLOAD_BLOCK_FULLY_PRESENT */
                rc = serve((entry & VHDX_BAT_FILE_OFF_MASK) + in_block, out + done, chunk);
            } else if (state == 7) {
                /* PARTIALLY_PRESENT requires a differencing parent -
                 * unsupported surface (registered); refuse precisely */
                return ERR_IMAGE_CORRUPT;
            } else {
                /* NOT_PRESENT/UNDEFINED/ZERO/UNMAPPED read as zeros */
                std::memset(out + done, 0, chunk);
            }
            if (rc != OK)
                return rc;
            done += chunk;
        }
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    u64 bat_entry_slot(u64 block) const { return block + (block >> chunk_ratio_bits_); }
    static u64 bat_slot_of(const std::vector<u64>&, u64) { return 0; } /* unused */

    /* file read with the (sorted) log overlay applied first. Log
     * descriptors journal FILE offsets - the overlay is keyed on them,
     * not on virtual offsets (the two diverge on any non-flat layout). */
    u32 serve(u64 file_off, u8* out, u32 chunk) {
        /* first range whose end is past file_off */
        size_t lo = 0, hi = overlay_.size();
        while (lo < hi) {
            const size_t mid = (lo + hi) / 2;
            if (overlay_[mid].offset + overlay_[mid].length <= file_off)
                lo = mid + 1;
            else
                hi = mid;
        }
        const bool covered = lo < overlay_.size() && overlay_[lo].offset <= file_off;
        if (covered) {
            const LogOverlayRange& r = overlay_[lo];
            const u32 take = u32(std::min<u64>(chunk, r.offset + r.length - file_off));
            if (r.bytes.empty())
                std::memset(out, 0, take);
            else
                std::memcpy(out, r.bytes.data() + (file_off - r.offset), take);
            if (take < chunk)
                return serve(file_off + take, out + take, chunk - take);
            return OK;
        }
        if (lo < overlay_.size() && overlay_[lo].offset < file_off + chunk) {
            /* file data up to the next overlay range */
            const u32 take = u32(overlay_[lo].offset - file_off);
            const u32 rc = io_read_exact(*file_.io, file_.handle, file_off, out, take);
            if (rc != OK)
                return rc;
            return serve(file_off + take, out + take, chunk - take);
        }
        return io_read_exact(*file_.io, file_.handle, file_off, out, chunk);
    }

    HeldFile file_;
    std::vector<u64> bat_;
    u32 block_size_ = 0;
    u32 chunk_ratio_bits_ = 0;
    std::vector<LogOverlayRange> overlay_;
    Geometry geom_;
};

/* VHDX log entry (qemu vhdx-log.c semantics, read-only) */
struct VhdxLogEntry {
    u64 file_off; /* position inside the log region */
    u32 entry_length;
    u64 sequence;
    u32 descriptor_count;
};

bool vhdx_log_entry_valid(ImageFileIo& io, u64 h, const VhdxHeaderInfo& hdr, u64 log_start,
                          u32 log_len, u64 log_pos, u64 file_bytes, u64 expect_seq,
                          bool expect_exact, VhdxLogEntry* out) {
    u8 raw[64];
    if (log_pos + 64 > log_len)
        return false;
    if (io_read_exact(io, h, log_start + log_pos, raw, 64) != OK)
        return false;
    if (rd32(raw) != VHDX_LOGE_SIG)
        return false;
    const u32 entry_length = rd32(raw + 8);
    if (entry_length == 0 || entry_length > log_len || entry_length % 4096 != 0)
        return false;
    const u64 seq = rd64(raw + 16);
    if (seq == 0)
        return false;
    if (expect_exact && seq != expect_seq + 1)
        return false;
    const u32 desc_count = rd32(raw + 24);
    if (u64(desc_count) * 32 > entry_length)
        return false;
    if (std::memcmp(raw + 32, hdr.log_guid, 16) != 0)
        return false;
    /* a log claiming more file than exists = truncated image (qemu rule) */
    if (rd64(raw + 48) > file_bytes)
        return false;
    /* checksum over the whole entry (checksum field zeroed) */
    std::vector<u8> entry(entry_length);
    if (io_read_exact(io, h, log_start + log_pos, entry.data(), entry_length) != OK)
        return false;
    std::memset(entry.data() + 4, 0, 4);
    if (crc32c(entry.data(), entry_length) != rd32(raw + 4))
        return false;
    out->file_off = log_pos;
    out->entry_length = entry_length;
    out->sequence = seq;
    out->descriptor_count = desc_count;
    return true;
}

std::vector<LogOverlayRange> vhdx_build_log_overlay(ImageFileIo& io, u64 h,
                                                    const VhdxHeaderInfo& hdr, u64 log_start,
                                                    u32 log_len, u64 file_bytes, u32* err) {
    std::vector<LogOverlayRange> overlay;
    if (hdr.log_guid_zero || log_len == 0)
        return overlay; /* no log present (spec) */
    if (log_len % (1024 * 1024) != 0 || hdr.log_offset < 1024 * 1024) {
        *err = ERR_IMAGE_CORRUPT;
        return overlay;
    }

    /* find the valid entry chain with the highest sequence number */
    struct Chain {
        u64 start;
        u64 count;
        u64 last_seq;
    };
    Chain best{0, 0, 0};
    bool have_best = false;
    const u32 sectors = log_len / 4096;
    for (u32 start = 0; start < sectors;) {
        VhdxLogEntry e;
        if (!vhdx_log_entry_valid(io, h, hdr, log_start, log_len, u64(start) * 4096, file_bytes, 0,
                                  false, &e)) {
            ++start; /* advance one sector (qemu inc_and_exit) */
            continue;
        }
        Chain chain{u64(start) * 4096, 1, e.sequence};
        u64 pos = (u64(start) * 4096 + e.entry_length) % log_len;
        u64 expect = e.sequence;
        for (;;) {
            VhdxLogEntry next;
            if (!vhdx_log_entry_valid(io, h, hdr, log_start, log_len, pos, file_bytes, expect, true,
                                      &next))
                break;
            chain.count++;
            chain.last_seq = next.sequence;
            expect = next.sequence;
            pos = (pos + next.entry_length) % log_len;
        }
        if (!have_best || chain.last_seq > best.last_seq) {
            best = chain;
            have_best = true;
        }
        start += u32(e.entry_length / 4096);
    }
    if (!have_best)
        return overlay; /* log region present but empty of valid entries */

    /* replay the winning chain into the overlay, in sequence order (later
     * entries override overlapping earlier ones) */
    u64 pos = best.start;
    for (u64 n = 0; n < best.count; ++n) {
        VhdxLogEntry e;
        if (!vhdx_log_entry_valid(io, h, hdr, log_start, log_len, pos, file_bytes, 0, false, &e))
            break;
        std::vector<u8> entry(e.entry_length);
        if (io_read_exact(io, h, log_start + pos, entry.data(), e.entry_length) != OK)
            break;
        /* descriptor sectors: ceil((count+2)/128); data sectors follow */
        const u32 desc_sectors = (e.descriptor_count + 2 + 127) / 128;
        u64 data_off = u64(desc_sectors) * 4096;
        for (u32 di = 0; di < e.descriptor_count; ++di) {
            const u8* d = entry.data() + 64 + u64(di) * 32;
            const u32 sig = rd32(d);
            const u64 file_offset = rd64(d + 0x10);
            if (sig == VHDX_DESC_SIG) {
                /* VHDXLogDescriptor (qemu/spec layout): sig@0x00,
                 * trailing_bytes u32@0x04, leading_bytes u64@0x08,
                 * file_offset u64@0x10, sequence u64@0x18 */
                const u64 seq = rd64(d + 0x18);
                const u8* ds = entry.data() + data_off;
                if (data_off + 4096 > e.entry_length || rd32(ds) != VHDX_DATA_SIG ||
                    rd32(ds + 4) != u32(seq >> 32) || rd32(ds + 4092) != u32(seq & 0xFFFFFFFF)) {
                    *err = ERR_IMAGE_CORRUPT;
                    return overlay;
                }
                LogOverlayRange r;
                r.offset = file_offset;
                r.length = 4096;
                r.bytes.resize(4096);
                std::memcpy(r.bytes.data(), d + 8, 8);         /* leading */
                std::memcpy(r.bytes.data() + 8, ds + 8, 4084); /* body */
                std::memcpy(r.bytes.data() + 4092, d + 4, 4);  /* trailing */
                overlay.push_back(std::move(r));
                data_off += 4096;
            } else if (sig == VHDX_ZERO_SIG) {
                const u64 zero_len = rd64(d + 8);
                if (zero_len == 0 || zero_len % 4096 != 0 || zero_len > (1u << 31)) {
                    *err = ERR_IMAGE_CORRUPT;
                    return overlay;
                }
                LogOverlayRange r;
                r.offset = file_offset;
                r.length = zero_len;
                overlay.push_back(std::move(r));
            } else {
                *err = ERR_IMAGE_CORRUPT;
                return overlay;
            }
        }
        pos = (pos + e.entry_length) % log_len;
    }
    std::sort(
        overlay.begin(), overlay.end(),
        [](const LogOverlayRange& a, const LogOverlayRange& b) { return a.offset < b.offset; });
    return overlay;
}

std::unique_ptr<IStorageDevice> open_vhdx(ImageFileIo& io, const std::u16string& path, u32 depth,
                                          u32* err) {
    (void)depth; /* differencing is refused below - no recursion yet */
    HeldFile file;
    file.io = &io;
    u32 e = 0;
    file.handle = io.open(path, &e);
    if (file.handle == 0) {
        *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    file.bytes = io.size(file.handle);
    if (file.bytes < 1024 * 1024) {
        *err = ERR_TRUNCATED;
        return nullptr;
    }
    u8 sig[8];
    if (io_read_exact(io, file.handle, 0, sig, 8) != OK || rd64(sig) != VHDX_FILE_SIG) {
        *err = ERR_BAD_MAGIC;
        return nullptr;
    }

    /* header pair at 0x10000 / 0x20000: valid-CRC ones, highest sequence */
    VhdxHeaderInfo hinfo;
    u64 best_seq = 0;
    int have_header = 0;
    for (const u64 hoff : {u64(0x10000), u64(0x20000)}) {
        u8 blk[4096];
        if (io_read_exact(io, file.handle, hoff, blk, sizeof(blk)) != OK)
            continue;
        VhdxHeaderInfo hi;
        u64 seq = 0;
        if (!vhdx_header_parse(blk, &hi, &seq))
            continue;
        if (!have_header || seq > best_seq) {
            hinfo = hi;
            best_seq = seq;
            have_header = 1;
        }
    }
    if (!have_header) {
        *err = ERR_BAD_CHECKSUM;
        return nullptr;
    }

    /* region table pair at 0x30000 / 0x40000 */
    u64 bat_offset = 0, bat_length = 0, meta_offset = 0, meta_length = 0;
    bool have_bat = false, have_meta = false;
    for (const u64 roff : {u64(0x30000), u64(0x40000)}) {
        std::vector<u8> rt(64 * 1024);
        if (io_read_exact(io, file.handle, roff, rt.data(), u32(rt.size())) != OK)
            continue;
        if (rd32(rt.data()) != VHDX_REGI_SIG)
            continue;
        if (crc32c_skip4(rt.data(), rt.size()) != rd32(rt.data() + 4))
            continue;
        const u32 count = rd32(rt.data() + 8);
        if (count > 2047)
            continue;
        for (u32 i = 0; i < count; ++i) {
            const u8* en = rt.data() + 16 + u64(i) * 32;
            const u32 g1 = rd32(en);
            const u32 g2 = rd16(en + 4);
            const u32 g3 = rd16(en + 6);
            const u64 foff = rd64(en + 16);
            const u32 flen = rd32(en + 24);
            /* BAT region: 2DC27766-F623-4200-9D64-115E9BFD4A08;
             * Metadata:  8B7CA206-4790-4B9A-B8FE-575F050F886E (the
             * data3 halves were transposed in the first cut - pinned
             * byte-exact against the qemu-written region table) */
            if (!have_bat && g1 == 0x2DC27766 && g2 == 0xF623 && g3 == 0x4200) {
                bat_offset = foff;
                bat_length = flen;
                have_bat = true;
            }
            if (!have_meta && g1 == 0x8B7CA206 && g2 == 0x4790 && g3 == 0x4B9A) {
                meta_offset = foff;
                meta_length = flen;
                have_meta = true;
            }
        }
        if (have_bat && have_meta)
            break;
    }
    if (!have_bat || !have_meta) {
        *err = ERR_IMAGE_CORRUPT;
        return nullptr;
    }
    if (bat_offset + bat_length > file.bytes || meta_offset + meta_length > file.bytes) {
        *err = ERR_TRUNCATED;
        return nullptr;
    }

    /* metadata: block size, virtual size, logical sector size, parent flag */
    u8 mh[32];
    if (io_read_exact(io, file.handle, meta_offset, mh, sizeof(mh)) != OK ||
        rd64(mh) != VHDX_META_SIG) {
        *err = ERR_IMAGE_CORRUPT;
        return nullptr;
    }
    const u32 meta_count = rd16(mh + 10);
    u64 virt_size = 0;
    u32 block_size = 0, logical_sector = 512;
    bool has_parent = false;
    for (u32 i = 0; i < meta_count && i < 2047; ++i) {
        u8 me[32];
        if (io_read_exact(io, file.handle, meta_offset + 32 + u64(i) * 32, me, sizeof(me)) != OK) {
            *err = ERR_READ_FAILED;
            return nullptr;
        }
        const u32 g1 = rd32(me);
        const u32 g2 = rd16(me + 4);
        const u32 g3 = rd16(me + 6);
        const u32 moff = rd32(me + 16);
        const u32 mlen = rd32(me + 20);
        const bool is_file_params = g1 == 0xCAA16737 && g2 == 0xFA36 && g3 == 0x4D43;
        const bool is_virt_size = g1 == 0x2FA54224 && g2 == 0xCD1B && g3 == 0x4876;
        const bool is_logical = g1 == 0x8141BF1D && g2 == 0xA96F && g3 == 0x4709;
        if (!is_file_params && !is_virt_size && !is_logical)
            continue;
        if (u64(moff) + mlen > meta_length || mlen > 8) {
            *err = ERR_IMAGE_CORRUPT;
            return nullptr;
        }
        u8 val[8] = {0};
        if (io_read_exact(io, file.handle, meta_offset + moff, val, mlen) != OK) {
            *err = ERR_READ_FAILED;
            return nullptr;
        }
        if (is_file_params) {
            block_size = rd32(val);
            const u32 data_bits = mlen >= 8 ? rd32(val + 4) : 0;
            has_parent = (data_bits & 0x02) != 0;
        } else if (is_virt_size) {
            virt_size = rd64(val);
        } else {
            logical_sector = rd32(val);
        }
    }
    if (block_size < 1024 * 1024 || block_size > 256 * 1024 * 1024 ||
        (block_size & (block_size - 1)) != 0 || virt_size == 0 ||
        (logical_sector != 512 && logical_sector != 4096)) {
        *err = ERR_IMAGE_CORRUPT;
        return nullptr;
    }
    if (has_parent) {
        /* differencing VHDX needs parent-locator metadata + sector
         * bitmaps - registered remainder, refused precisely */
        *err = ERR_UNSUPPORTED;
        return nullptr;
    }

    /* log overlay (read-only replay) */
    u32 log_err = OK;
    std::vector<LogOverlayRange> overlay = vhdx_build_log_overlay(
        io, file.handle, hinfo, hinfo.log_offset, hinfo.log_length, file.bytes, &log_err);
    if (log_err != OK) {
        *err = log_err;
        return nullptr;
    }

    /* BAT: payload blocks + interleaved SB entries (chunk ratio) */
    const u64 data_blocks = (virt_size + block_size - 1) / block_size;
    u32 chunk_ratio_bits = 0;
    {
        const u64 chunk_ratio = (u64(1) << 23) * logical_sector / block_size;
        while (chunk_ratio_bits < 40 && (u64(1) << (chunk_ratio_bits + 1)) <= chunk_ratio)
            ++chunk_ratio_bits;
    }
    const u32 bat_entries = u32(data_blocks + ((data_blocks - 1) >> chunk_ratio_bits));
    if (u64(bat_entries) * 8 > bat_length) {
        *err = ERR_IMAGE_CORRUPT;
        return nullptr;
    }
    std::vector<u8> bat_raw(u64(bat_entries) * 8);
    if (io_read_exact(io, file.handle, bat_offset, bat_raw.data(), u32(bat_raw.size())) != OK) {
        *err = ERR_READ_FAILED;
        return nullptr;
    }
    std::vector<u64> bat(bat_entries);
    for (u32 i = 0; i < bat_entries; ++i)
        bat[i] = rd64(bat_raw.data() + u64(i) * 8);

    return std::unique_ptr<IStorageDevice>(new VhdxDevice(std::move(file), virt_size, block_size,
                                                          logical_sector, std::move(bat),
                                                          chunk_ratio_bits, std::move(overlay)));
}

} // namespace

/* ------------------------------------------------------------------ */
/* open_image - the og's extension dispatch (DRWUI:948237-948409)      */
/* ------------------------------------------------------------------ */

ImageFormat classify_extension(const std::u16string& path_utf16) {
    auto iends = [&](const char16_t* suffix) {
        const size_t n = std::u16string(suffix).size();
        if (path_utf16.size() < n)
            return false;
        for (size_t i = 0; i < n; ++i) {
            const char16_t a = path_utf16[path_utf16.size() - n + i];
            const char16_t b = suffix[i];
            if (a >= u'A' && a <= u'Z' ? a + 32 != b : a != b)
                return false;
        }
        return true;
    };
    if (iends(u".vmdk"))
        return ImageFormat::Vmdk;
    if (iends(u".vhd"))
        return ImageFormat::Vhd;
    if (iends(u".vhdx"))
        return ImageFormat::Vhdx;
    /* .edisk/.img/.dmg/.raw/unknown: the og's flat fallback bucket
     * (.edisk is refused later with a precise error - see open_image). */
    return ImageFormat::Flat;
}

Lba0Layout classify_lba0(const u8* lba0, size_t len) {
    if (len < SECTOR)
        return Lba0Layout::Unknown;
    const bool gpt = len >= SECTOR + 8 && std::memcmp(lba0 + SECTOR, "EFI PART", 8) == 0;
    const bool mbr = lba0[SECTOR - 2] == 0x55 && lba0[SECTOR - 1] == 0xAA;
    if (gpt)
        return Lba0Layout::Gpt;
    if (mbr)
        return Lba0Layout::Mbr;
    return Lba0Layout::Unknown;
}

namespace {

std::unique_ptr<IStorageDevice> open_image_impl(ImageFileIo& io, const std::u16string& path,
                                                u32 depth, u32* err) {
    if (err)
        *err = OK;
    if (depth > 32) {
        if (err)
            *err = ERR_DEPTH_LIMIT;
        return nullptr;
    }
    const ImageFormat fmt = classify_extension(path);
    if (fmt == ImageFormat::Vmdk)
        return open_vmdk(io, path, depth, err);
    if (fmt == ImageFormat::Vhd)
        return open_vhd(io, path, depth, err);
    if (fmt == ImageFormat::Vhdx)
        return open_vhdx(io, path, depth, err);
    /* flat fallback bucket: .img/.dmg/.raw/unknown - exactly the og's
     * CreateFileW + GetFileSizeEx path. .edisk (proprietary EaseUS
     * format, undecoded in the corpus) is the registered exclusion. */
    auto iends = [&](const char16_t* suffix) {
        const size_t n = std::u16string(suffix).size();
        if (path.size() < n)
            return false;
        for (size_t i = 0; i < n; ++i) {
            const char16_t a = path[path.size() - n + i];
            const char16_t b = suffix[i];
            if (a >= u'A' && a <= u'Z' ? a + 32 != b : a != b)
                return false;
        }
        return true;
    };
    if (iends(u".edisk")) {
        if (err)
            *err = ERR_UNSUPPORTED;
        return nullptr;
    }
    HeldFile file;
    file.io = &io;
    u32 e = 0;
    file.handle = io.open(path, &e);
    if (file.handle == 0) {
        if (err)
            *err = ERR_OPEN_FAILED;
        return nullptr;
    }
    file.bytes = io.size(file.handle);
    if (file.bytes == 0) {
        io.close(file.handle);
        if (err)
            *err = ERR_TRUNCATED; /* an empty file cannot be a disk */
        return nullptr;
    }
    return std::unique_ptr<IStorageDevice>(new FlatImageDevice(std::move(file), SECTOR));
}

} // namespace

std::unique_ptr<IStorageDevice> open_image(ImageFileIo& io, const std::u16string& path_utf16,
                                           u32* err) {
    return open_image_impl(io, path_utf16, 0, err);
}

} // namespace storage
} // namespace cr
