/* hostile_smoke_s012.cpp - S-012 fuzz smoke: mutated GUID-dump fixtures
 * driven through the fuzz_ntfs harness entry (walk_mft incl. the new
 * pass-2 GUID-dump walk) under ASan. Standalone driver: the CI libFuzzer
 * lane runs the real target; this is the in-session hostile-entry smoke
 * (same pattern as the S-013 exFAT smoke).
 *
 * Build: g++ -fsanitize=address -std=c++20 -I include -I fuzz \
 *   hostile_smoke_s012.cpp <cr_core objects> -o smoke
 */
#include "fuzz_parser_common.h"
#include "cr/ntfs/cr_bootstrap.h"
#include "cr/ntfs/cr_mftwalk.h"
#include "cr/ntfs/cr_attributes.h"
#include "cr/ntfs/cr_fixup.h"
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <random>
#include <vector>

using namespace cr;
using namespace cr::ntfs;
using namespace crfuzz;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 512)
        return 0;
    MemDevice dev(data, size);
    ScanSetup s;
    u32 err = 0;
    auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(SliceShim::shim(&dev)), &err);
    if (vol) {
        std::unique_ptr<VolumeBitmap> bm;
        (void)VolumeBitmap::load(vol.get(), bm);
        WalkerFlags wf;
        WalkStats ws;
        (void)walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, s.tree, s.ra, s.rd, s.rl,
                       ws);
    }
    return 0;
}

/* seed image: the S-012 GUID-dump fixture shape (boot + MFT + a live
 * GUID-named file whose $DATA carries stale MFT records) */
static std::vector<uint8_t> seed_image();

int main(int argc, char** argv) {
    const int iters = argc > 1 ? std::atoi(argv[1]) : 200;
    std::mt19937_64 rng(0xC0FFEE);
    auto base = seed_image();
    int exercised_pass2 = 0;
    /* iteration 0: the unmutated seed (sanity: pass-2 must fire) */
    for (int i = 0; i <= iters; ++i) {
        std::vector<uint8_t> img = base;
        if (i > 0) {
            /* hostile mutations: random pokes, truncations, geometry flips */
            const int nmut = 1 + int(rng() % 16);
            for (int m = 0; m < nmut; ++m) {
                const size_t off = size_t(rng() % img.size());
                img[off] = uint8_t(rng());
            }
            switch (i % 4) {
            case 0:
                img[0x40] = uint8_t(1 + (rng() % 7)); /* clusters-per-record */
                break;
            case 1:
                if (img.size() > 2048)
                    img.resize(img.size() - 1 - size_t(rng() % 512)); /* truncate */
                break;
            case 2: { /* corrupt the GUID name bytes randomly */
                const size_t name_off = size_t(4 * 512 + 16 * 512) + 0x18 + 66 + 40;
                if (name_off + 8 < img.size())
                    for (int k = 0; k < 8; ++k)
                        img[name_off + size_t(k) * 2 + 1] = uint8_t(rng());
                break;
            }
            default:
                break;
            }
        }
        /* run through the harness entry under ASan; count pass-2 exercise */
        MemDevice dev(img.data(), img.size());
        ScanSetup s;
        u32 err = 0;
        auto vol = NtfsVolume::open(std::unique_ptr<IStorageDevice>(SliceShim::shim(&dev)), &err);
        if (vol) {
            std::unique_ptr<VolumeBitmap> bm;
            (void)VolumeBitmap::load(vol.get(), bm);
            WalkerFlags wf;
            WalkStats ws;
            (void)walk_mft(vol.get(), bm.get(), wf, nullptr, nullptr, nullptr, s.tree, s.ra, s.rd,
                           s.rl, ws);
            if (ws.guid_dump_records > 0)
                ++exercised_pass2;
            if (i == 0)
                std::printf("seed sanity: open err=%u examined=%llu emitted=%llu "
                            "guid_files=%llu guid_records=%llu destroyed=%llu damaged=%llu "
                            "meta=%llu\n",
                            err, (unsigned long long)ws.examined, (unsigned long long)ws.emitted,
                            (unsigned long long)ws.guid_dump_files,
                            (unsigned long long)ws.guid_dump_records,
                            (unsigned long long)ws.guid_dump_destroyed,
                            (unsigned long long)ws.damaged, (unsigned long long)ws.meta_skipped);
        } else if (i == 0) {
            std::printf("seed sanity: OPEN FAILED err=%u\n", err);
            /* trace the record-0 path step by step */
            std::vector<uint8_t> buf(1024, 0);
            std::printf("  read_at rc=%u\n", dev.read_at(4 * 512, buf.data(), 1024));
            RecordHeader rh;
            std::memcpy(&rh, buf.data(), sizeof(rh));
            std::printf("  magic=%08x uso=%u uss=%u attr_off=%u\n", rh.magic,
                        rh.update_sequence_offset, rh.update_sequence_size, rh.attribute_offset);
            std::printf("  fixup rc=%u\n", fixup_apply(buf.data(), 1024, rh.update_sequence_offset,
                                                       rh.update_sequence_size, 512));
            std::vector<AttrView> attrs;
            std::printf("  enum rc=%u n=%zu\n", enum_attributes(buf.data(), 1024, attrs),
                        attrs.size());
            for (const auto& a : attrs)
                std::printf("    attr type=%08x len=%u nonres=%d vlen=%u voff=%u rloff=%u\n",
                            a.type, a.record_length, (int)a.nonresident, a.value_length,
                            a.value_offset, a.runlist_offset);
        }
    }
    std::printf("hostile smoke: %d iters, pass-2 exercised on %d inputs\n", iters, exercised_pass2);
    return exercised_pass2 > 0 ? 0 : 1;
}

/* ---- minimal fixture builder (mirrors the gtest fixture) ---- */
#include <cstring>
static constexpr u32 SEC = 512;
static constexpr u32 CLUSTER = SEC;
static constexpr u64 NSECTORS = 256;
static constexpr u64 RECORD_SIZE = 1024;
static constexpr u64 MFT_LCN_OFF = 4 * SEC; /* LCN 4 */
static constexpr u64 DUMP_LCN_OFF = 200 * SEC;

static void put_u16(std::vector<uint8_t>& b, u64 off, u16 v) {
    b[size_t(off)] = uint8_t(v);
    b[size_t(off) + 1] = uint8_t(v >> 8);
}
static void put_u32(std::vector<uint8_t>& b, u64 off, u32 v) {
    put_u16(b, off, u16(v));
    put_u16(b, off + 2, u16(v >> 16));
}
static void put_u64(std::vector<uint8_t>& b, u64 off, u64 v) {
    put_u32(b, off, u32(v));
    put_u32(b, off + 4, u32(v >> 32));
}

struct RB {
    std::vector<uint8_t> buf;
    u32 pos = 0;
    RB() : buf(RECORD_SIZE, 0) {}
    void header(u64 rec_no, u16 flags) {
        put_u32(buf, 0, 0x454C4946); /* 'FILE' */
        put_u16(buf, 4, 0x30);       /* USA offset */
        put_u16(buf, 6, 3);          /* USA size */
        put_u16(buf, 16, 1);         /* sequence */
        put_u16(buf, 18, 1);         /* link count */
        put_u16(buf, 20, 0x38);      /* attr offset */
        put_u16(buf, 22, flags);
        put_u32(buf, 24, 1024);
        put_u32(buf, 28, 1024);
        put_u64(buf, 32, 0); /* base record */
        put_u32(buf, 44, u32(rec_no));
        pos = 0x38;
        u16 slots[3] = {0x0BB8, 0xAAAA, 0xBBBB};
        for (int i = 0; i < 3; ++i)
            put_u16(buf, 0x30 + u64(i) * 2, slots[i]);
        for (int i = 0; i < 2; ++i) {
            buf[size_t(u64(i) * SEC) + SEC - 2] = 0xB8;
            buf[size_t(u64(i) * SEC) + SEC - 1] = 0x0B;
        }
    }
    void si(u32 dos) {
        put_u32(buf, pos + 0, 0x10);
        put_u32(buf, pos + 4, 96);
        put_u16(buf, pos + 16, 72);   /* value_length */
        put_u16(buf, pos + 20, 0x18); /* value_offset */
        put_u32(buf, pos + 0x18 + 32, dos);
        pos += 96;
    }
    void fn(u64 parent, const char16_t* name, u32 dos, u64 rsize) {
        std::u16string n = name;
        const u32 vlen = 66 + u32(n.size()) * 2;
        const u32 alen = (0x18 + vlen + 7) / 8 * 8;
        put_u32(buf, pos + 0, 0x30);
        put_u32(buf, pos + 4, alen);
        put_u16(buf, pos + 16, u16(vlen & 0xFFFF)); /* value_length */
        put_u16(buf, pos + 20, 0x18);               /* value_offset */
        put_u64(buf, pos + 0x18 + 0, parent | (1ULL << 48));
        put_u64(buf, pos + 0x18 + 48, rsize);
        put_u32(buf, pos + 0x18 + 56, dos);
        buf[size_t(pos + 0x18 + 64)] = uint8_t(n.size());
        buf[size_t(pos + 0x18 + 65)] = 3; /* Win32+Dos */
        for (size_t i = 0; i < n.size(); ++i)
            put_u16(buf, pos + 0x18 + 66 + u64(i) * 2, u16(n[i]));
        pos += alen;
    }
    void data(u64 lcn, u64 clusters, u64 rsize) {
        /* single-run runlist: header (osize<<4 | lsize), length bytes,
         * signed-offset bytes (first run: delta from LCN 0), terminator */
        u8 rl[24] = {0};
        int p = 0;
        const int lsize = clusters > 0x7F ? 2 : 1;
        int osize = 1;
        if (lcn > 0x7FFFFF)
            osize = 4;
        else if (lcn > 0x7FFF)
            osize = 3;
        else if (lcn > 0x7F)
            osize = 2; /* signed 1-byte max is +0x7F */
        rl[p++] = uint8_t((osize << 4) | lsize);
        for (int k = 0; k < lsize; ++k)
            rl[p++] = uint8_t((clusters >> (8 * k)) & 0xFF);
        for (int k = 0; k < osize; ++k)
            rl[p++] = uint8_t((lcn >> (8 * k)) & 0xFF);
        rl[p++] = 0; /* terminator */
        const u32 rlen = u32(p);
        const u32 alen = (0x40 + rlen + 7) / 8 * 8;
        put_u32(buf, pos + 0, 0x80);
        put_u32(buf, pos + 4, alen);
        buf[size_t(pos + 8)] = 1; /* nonresident */
        put_u16(buf, pos + 10, 0x40);
        put_u64(buf, pos + 24, clusters - 1);
        put_u16(buf, pos + 32, 0x40);
        put_u64(buf, pos + 40, clusters * CLUSTER);
        put_u64(buf, pos + 48, rsize);
        put_u64(buf, pos + 56, rsize);
        std::memcpy(buf.data() + pos + 0x40, rl, rlen);
        pos += alen;
    }
    void finish() {
        put_u32(buf, pos + 0, 0xFFFFFFFF); /* terminator */
        put_u32(buf, pos + 4, 8);
        put_u32(buf, 24, pos + 8); /* bytes_in_use */
    }
};

static std::vector<uint8_t> seed_image() {
    std::vector<uint8_t> img(NSECTORS * SEC, 0);
    /* boot */
    std::memcpy(img.data() + 3, "NTFS    ", 8);
    put_u16(img, 0x0B, SEC);
    img[0x0D] = 1; /* spc */
    put_u64(img, 0x28, NSECTORS);
    put_u64(img, 0x30, 4);  /* mft lcn */
    put_u64(img, 0x38, 68); /* mirr lcn */
    img[0x40] = 2;          /* cpr */
    img[510] = 0x55;
    img[511] = 0xAA;
    /* $MFT */
    {
        RB r;
        r.header(0, 1);
        r.fn(5, u"$MFT", 0x06, 68 * CLUSTER);
        r.data(4, 68, 68 * CLUSTER);
        r.finish();
        std::memcpy(img.data() + MFT_LCN_OFF, r.buf.data(), RECORD_SIZE);
    }
    /* root */
    {
        RB r;
        r.header(5, 1 | 2);
        r.si(0);
        r.fn(5, u".", 0x00, 0);
        r.finish();
        std::memcpy(img.data() + MFT_LCN_OFF + 10 * SEC, r.buf.data(), RECORD_SIZE);
    }
    /* GUID dump file */
    {
        RB r;
        r.header(8, 1);
        r.si(0x20);
        r.fn(5, u"{8c8d1e50-9f2e-4a1b-8e6f-3c2d5a4b7e90}{3808876b-c176-4e48-b7ae-04046e6cc752}",
             0x20, 4096);
        r.data(200, 8, 4096);
        r.finish();
        std::memcpy(img.data() + MFT_LCN_OFF + 16 * SEC, r.buf.data(), RECORD_SIZE);
    }
    /* stale records in the dump */
    for (int k = 0; k < 4; ++k) {
        RB r;
        r.header(17, 0);
        r.si(0x20);
        char16_t nm[32];
        for (int c = 0; c < 12; ++c)
            nm[c] = u"stalefile.bin"[c];
        nm[11] = u'0' + char16_t(k);
        nm[12] = 0;
        r.fn(5, nm, 0x20, 2048);
        r.data(100 + u64(k) * 8, 4, 2048);
        r.finish();
        std::memcpy(img.data() + DUMP_LCN_OFF + u64(k) * RECORD_SIZE, r.buf.data(), RECORD_SIZE);
    }
    return img;
}
