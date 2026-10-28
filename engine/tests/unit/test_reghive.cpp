/* test_reghive.cpp - registry hive (regf) parser + FVEAutoUnlock harvest
 * tests (diff-to-fix S-014).
 *
 * The fixture builder synthesizes minimal regf hives with the exact layout
 * the og's RegLib.mo parses (offsets decoded from RegLib_decompiled.c - see
 * cr_reghive.cpp's header comment). Every test builds its hive from scratch,
 * so the parser is exercised against corpus-shaped bytes, not against
 * real-filesystem state.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/reghive/cr_reghive.h"
#include "cr/reghive/cr_offlineos.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::reghive;

namespace {

/* ---------------- synthetic regf fixture builder (corpus layout) -------- */

struct CellBuf {
    std::vector<u8> bytes; /* cell content INCLUDING the i32 size dword */
    i32 size_field = 0;
};

class HiveBuilder {
public:
    HiveBuilder() {
        /* one 0x1000-byte hbin at file offset 0x1000, room to grow */
        bins_.resize(0x1000, 0);
        std::memcpy(bins_.data(), "hbin", 4);
        put32(bins_, 4, 0);      /* offset from first hbin */
        put32(bins_, 8, 0x1000); /* bin size */
        /* root nk at first cell offset 0x20 */
        root_off_ = 0x20;
        root_cell_ = add_nk(0, "ROOTKEY", 0);
        (void)root_cell_;
    }

    /* add an nk cell; returns its hbin-relative offset. parent is the
     * offset of the parent nk; registers the child in the parent's lf list
     * (rebuilding the list cell each time - simple and correct). */
    u32 add_nk(u32 parent, const std::string& name, u16 flags) {
        std::vector<u8> rec(0x4C + (flags & 0x20 ? name.size() : name.size() * 2), 0);
        rec[0] = 'n';
        rec[1] = 'k';
        put16(rec, 0x02, flags);
        const bool ascii = (flags & 0x20) != 0;
        if (ascii) {
            std::memcpy(rec.data() + 0x4C, name.data(), name.size());
            put16(rec, 0x48, u16(name.size()));
        } else {
            for (size_t i = 0; i < name.size(); ++i)
                put16(rec, 0x4C + i * 2, name[i]);
            put16(rec, 0x48, u16(name.size() * 2));
        }
        const u32 off = append_cell(rec);
        if (parent != 0)
            children_[parent].push_back(off);
        return off;
    }

    /* add a vk cell under an nk; returns the vk offset */
    u32 add_vk(u32 nk, const std::string& name, u32 type, const std::vector<u8>& data) {
        std::vector<u8> rec(0x12 + name.size(), 0);
        rec[0] = 'v';
        rec[1] = 'k';
        put16(rec, 0x02, u16(name.size()));
        put16(rec, 0x10, 1); /* ASCII name flag */
        std::memcpy(rec.data() + 0x12, name.data(), name.size());
        put32(rec, 0x0C, type);
        if (data.size() <= 4) {
            put32(rec, 0x04, u32(data.size()) | 0x80000000u); /* inline */
            for (size_t i = 0; i < data.size(); ++i)
                rec[0x08 + i] = data[i];
        } else {
            put32(rec, 0x04, u32(data.size()));
            const u32 doff = append_cell(data);
            put32(rec, 0x08, doff);
        }
        const u32 off = append_cell(rec);
        values_[nk].push_back(off);
        return off;
    }

    /* finalize: build lf lists + value lists + header; returns the hive */
    std::vector<u8> build() {
        /* subkey lists */
        for (const auto& [parent, kids] : children_) {
            std::vector<u8> rec(4 + kids.size() * 8, 0);
            rec[0] = 'l';
            rec[1] = 'f';
            put16(rec, 0x02, u16(kids.size()));
            for (size_t i = 0; i < kids.size(); ++i) {
                put32(rec, 4 + i * 8, kids[i]);
                put32(rec, 4 + i * 8 + 4, 0); /* hash (unused by the walk) */
            }
            const u32 list_off = append_cell(rec);
            patch_nk(parent, 0x14, u32(kids.size()));
            patch_nk(parent, 0x1C, list_off);
        }
        /* value lists: cell of u32 offsets, count from the nk */
        for (const auto& [nk, vks] : values_) {
            std::vector<u8> rec(vks.size() * 4, 0);
            for (size_t i = 0; i < vks.size(); ++i)
                put32(rec, i * 4, vks[i]);
            const u32 list_off = append_cell(rec);
            patch_nk(nk, 0x24, u32(vks.size()));
            patch_nk(nk, 0x28, list_off);
        }
        /* header */
        std::vector<u8> hive(0x1000 + bins_.size(), 0);
        std::memcpy(hive.data(), "regf", 4);
        put32(hive, 0x18, 6); /* minor version 6 (modern) */
        put32(hive, REGF_ROOT_CELL_OFF, root_off_);
        put32(hive, REGF_BINS_SIZE_OFF, u32(bins_.size()));
        std::memcpy(hive.data() + 0x1000, bins_.data(), bins_.size());
        /* XOR checksum over the first 0x1FC bytes */
        u32 acc = 0;
        for (u32 off = 0; off < 0x1FC; off += 4)
            acc ^= get32(hive, off);
        put32(hive, 0x1FC, acc);
        return hive;
    }

    /* rebuild with a corrupted checksum */
    std::vector<u8> build_bad_checksum() {
        auto hive = build();
        hive[0x1FC] ^= 0x5A;
        return hive;
    }

    u32 root() const { return root_off_; }

private:
    std::vector<u8> bins_;
    u32 root_off_ = 0;
    u32 root_cell_ = 0;
    std::vector<std::pair<u32, std::vector<u8>>> cells_;
    std::map<u32, std::vector<u32>> children_;
    std::map<u32, std::vector<u32>> values_;

    static void put16(std::vector<u8>& v, size_t at, u16 x) {
        v[at] = u8(x);
        v[at + 1] = u8(x >> 8);
    }
    static void put32(std::vector<u8>& v, size_t at, u32 x) {
        for (int i = 0; i < 4; ++i)
            v[at + i] = u8(x >> (8 * i));
    }
    static u32 get32(const std::vector<u8>& v, size_t at) {
        return u32(v[at]) | (u32(v[at + 1]) << 8) | (u32(v[at + 2]) << 16) | (u32(v[at + 3]) << 24);
    }

    /* append a cell into the current bin (grows the bin by 0x1000 chunks);
     * cells are 8-aligned like real hives */
    u32 append_cell(const std::vector<u8>& rec) {
        u32 off = next_off_;
        const size_t need = 4 + rec.size();
        const size_t aligned = (need + 7) & ~size_t(7);
        while (off + aligned > bins_.size())
            bins_.resize(bins_.size() + 0x1000, 0);
        put32(bins_, off, u32(-i64(aligned))); /* negative = allocated */
        std::memcpy(bins_.data() + off + 4, rec.data(), rec.size());
        /* refresh the hbin size field */
        put32(bins_, 8, u32(bins_.size()));
        next_off_ = off + u32(aligned);
        return off;
    }

    /* patch a field of a previously-written nk record (find by offset) */
    void patch_nk(u32 nk_off, u32 field, u32 value) {
        /* the nk record sits at nk_off+4 in bins_ */
        put32(bins_, nk_off + 4 + field, value);
    }

    u32 next_off_ = 0x20;
};

/* the S-014 canonical fixture: \ControlSet001\Control\FVEAutoUnlock\{GUID}\Data */
struct FveHive {
    HiveBuilder b;
    u32 cs = 0, control = 0, fve = 0;

    FveHive() {
        cs = b.add_nk(b.root(), "ControlSet001", 0);
        control = b.add_nk(cs, "Control", 0);
        fve = b.add_nk(control, "FVEAutoUnlock", 0);
    }

    u32 add_key(const std::string& guid, const std::vector<u8>& blob) {
        const u32 sub = b.add_nk(fve, guid, 0);
        b.add_vk(sub, "Data", 3, blob);
        return sub;
    }
};

} // namespace

/* ------------------------------------------------------------- parsing */

TEST(Reghive, ParsesValidHiveAndRejectsGarbage) {
    FveHive fh;
    auto hive = fh.b.build();
    RegistryHive rh;
    EXPECT_TRUE(rh.open(hive.data(), hive.size()));

    std::vector<u8> junk(0x2000, 0xAB);
    RegistryHive rj;
    EXPECT_FALSE(rj.open(junk.data(), junk.size()));

    RegistryHive rs;
    EXPECT_FALSE(rs.open(hive.data(), hive.size() / 2)); /* truncated */
}

TEST(Reghive, RejectsCorruptedHeaderChecksum) {
    /* the og refuses the hive when the XOR checksum mismatches
     * (RegLib_decompiled.c :2376-2377) - the parser must too */
    FveHive fh;
    auto hive = fh.b.build_bad_checksum();
    RegistryHive rh;
    EXPECT_FALSE(rh.open(hive.data(), hive.size()));
}

TEST(Reghive, WalksControlSetKeyFamilyCaseInsensitive) {
    FveHive fh;
    auto hive = fh.b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    EXPECT_NE(u64(0), rh.find_key("\\ControlSet001\\Control\\FVEAutoUnlock"));
    /* registry names compare case-insensitively */
    EXPECT_NE(u64(0), rh.find_key("\\controlset001\\CONTROL\\fveautounlock"));
    EXPECT_EQ(u64(0), rh.find_key("\\ControlSet002\\Control\\FVEAutoUnlock"));
    EXPECT_EQ(u64(0), rh.find_key("\\ControlSet001\\Control\\Missing"));
}

TEST(Reghive, EnumeratesSubkeyNames) {
    FveHive fh;
    fh.add_key("{AAAAAAAA-0000-0000-0000-000000000001}", {1, 2, 3});
    fh.add_key("{BBBBBBBB-0000-0000-0000-000000000002}", {4, 5, 6});
    auto hive = fh.b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    const u64 fve = rh.find_key("\\ControlSet001\\Control\\FVEAutoUnlock");
    ASSERT_NE(u64(0), fve);
    std::vector<std::string> names;
    ASSERT_TRUE(rh.enum_subkeys(fve, names));
    ASSERT_EQ(size_t(2), names.size());
}

TEST(Reghive, ReadsBinaryValueDataPointedAndInline) {
    FveHive fh;
    std::vector<u8> blob(200);
    for (size_t i = 0; i < blob.size(); ++i)
        blob[i] = u8(i * 7);
    fh.add_key("{AAAAAAAA-0000-0000-0000-000000000001}", blob);
    /* inline (<= 4 bytes) data + a non-binary decoy value on another key */
    const u32 sub2 = fh.b.add_nk(fh.fve, "{CCCCCCCC-0000-0000-0000-000000000003}", 0);
    fh.b.add_vk(sub2, "Data", 3, {0x11, 0x22, 0x33});
    fh.b.add_vk(sub2, "SomeString", 1, {'h', 'i', '!', 0});
    auto hive = fh.b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));

    const u64 k1 = rh.find_subkey(rh.find_key("\\ControlSet001\\Control\\FVEAutoUnlock"),
                                  "{AAAAAAAA-0000-0000-0000-000000000001}");
    ASSERT_NE(u64(0), k1);
    u32 type = 0;
    std::vector<u8> data;
    ASSERT_TRUE(rh.read_value(k1, "Data", type, data));
    EXPECT_EQ(3u, type);
    EXPECT_EQ(blob, data);

    const u64 k2 = rh.find_subkey(rh.find_key("\\ControlSet001\\Control\\FVEAutoUnlock"),
                                  "{CCCCCCCC-0000-0000-0000-000000000003}");
    ASSERT_NE(u64(0), k2);
    ASSERT_TRUE(rh.read_value(k2, "Data", type, data));
    EXPECT_EQ(3u, type);
    EXPECT_EQ(std::vector<u8>({0x11, 0x22, 0x33}), data);
    /* value lookup is by NAME; "SomeString" is REG_SZ, not the harvest
     * target, but readable as a value */
    ASSERT_TRUE(rh.read_value(k2, "SomeString", type, data));
    EXPECT_EQ(1u, type);
    EXPECT_FALSE(rh.read_value(k2, "NoSuchValue", type, data));
}

TEST(Reghive, UnicodeKeyNamesWalkAsAscii) {
    /* unicode nk names (flags without 0x20): stored as UTF-16LE, walked
     * case-insensitively after ASCII folding */
    HiveBuilder b;
    const u32 cs = b.add_nk(b.root(), "ControlSet001", 0);
    const u32 control = b.add_nk(cs, "Control", 0);
    const u32 fve = b.add_nk(control, "FVEAutoUnlock", 0);
    (void)fve;
    auto hive = b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    EXPECT_NE(u64(0), rh.find_key("\\ControlSet001\\Control\\FVEAutoUnlock"));
}

/* ------------------------------------------------------------- harvest */

TEST(FveAutoUnlock, HarvestsPerGuidDataBlobs) {
    FveHive fh;
    std::vector<u8> blob1(64, 0xAA), blob2(96, 0xBB);
    fh.add_key("{AAAAAAAA-0000-0000-0000-000000000001}", blob1);
    fh.add_key("{BBBBBBBB-0000-0000-0000-000000000002}", blob2);
    /* a decoy: a subkey with a non-binary Data value must not be harvested */
    const u32 decoy = fh.b.add_nk(fh.fve, "{DEADBEEF-0000-0000-0000-000000000004}", 0);
    fh.b.add_vk(decoy, "Data", 1, {'x', 0});
    auto hive = fh.b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));

    std::vector<AutoUnlockKey> keys;
    EXPECT_EQ(size_t(2), harvest_fve_auto_unlock(rh, keys));
    ASSERT_EQ(size_t(2), keys.size());
    EXPECT_EQ("{AAAAAAAA-0000-0000-0000-000000000001}", keys[0].volume_guid);
    EXPECT_EQ(blob1, keys[0].blob);
    EXPECT_EQ("{BBBBBBBB-0000-0000-0000-000000000002}", keys[1].volume_guid);
    EXPECT_EQ(blob2, keys[1].blob);
}

TEST(FveAutoUnlock, HarvestCapsBlobsAt256Bytes) {
    /* the og's pipe record memcpy(Data, min(size, 0x100)) - a 512-byte
     * Data value is harvested as its first 256 bytes */
    FveHive fh;
    std::vector<u8> big(512);
    for (size_t i = 0; i < big.size(); ++i)
        big[i] = u8(i);
    fh.add_key("{AAAAAAAA-0000-0000-0000-000000000001}", big);
    auto hive = fh.b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    std::vector<AutoUnlockKey> keys;
    EXPECT_EQ(size_t(1), harvest_fve_auto_unlock(rh, keys));
    ASSERT_EQ(size_t(1), keys.size());
    EXPECT_EQ(size_t(256), keys[0].blob.size());
    EXPECT_EQ(0, std::memcmp(big.data(), keys[0].blob.data(), 256));
}

TEST(FveAutoUnlock, HarvestCapsRecordsAt100AndSkipsTinyBlobs) {
    /* corpus caps: <= 100 records; blobs < 32 bytes are not BEK-class
     * material - skipped like the og's >=32B read floor implies */
    FveHive fh;
    for (int i = 0; i < 105; ++i) {
        char guid[50];
        std::snprintf(guid, sizeof(guid), "{%08X-0000-0000-0000-000000000000}", unsigned(i));
        const std::vector<u8> blob(48, u8(i));
        fh.add_key(guid, blob);
    }
    /* plus a too-small blob and an over-long subkey name */
    fh.add_key("{SHORTBLB-0000-0000-0000-000000000009}", {1, 2, 3, 4, 5, 6, 7});
    auto hive = fh.b.build();
    RegistryHive rh;
    ASSERT_TRUE(rh.open(hive.data(), hive.size()));
    std::vector<AutoUnlockKey> keys;
    harvest_fve_auto_unlock(rh, keys);
    EXPECT_EQ(size_t(kFveMaxRecords), keys.size()); /* capped at 100 */
}

/* ------------------------------------------------------------- selection */

TEST(FveAutoUnlock, SelectionPrefersMatchingVolumeGuid) {
    std::vector<AutoUnlockKey> keys;
    AutoUnlockKey a;
    a.volume_guid = "{AAAAAAAA-0000-0000-0000-000000000001}";
    a.blob = {1};
    AutoUnlockKey b;
    b.volume_guid = "{BBBBBBBB-0000-0000-0000-000000000002}";
    b.blob = {2};
    keys.push_back(a);
    keys.push_back(b);

    /* match first, then the other in enumeration order */
    auto order = select_auto_unlock_attempts(keys, "{aaaaaaaa-0000-0000-0000-000000000001}");
    ASSERT_EQ(size_t(2), order.size());
    EXPECT_EQ(&keys[0], order[0]);
    EXPECT_EQ(&keys[1], order[1]);

    /* no match for the target: plain enumeration order */
    order = select_auto_unlock_attempts(keys, "{CCCCCCCC-0000-0000-0000-000000000003}");
    ASSERT_EQ(size_t(2), order.size());
    EXPECT_EQ(&keys[0], order[0]);
    EXPECT_EQ(&keys[1], order[1]);

    /* match on the second record */
    order = select_auto_unlock_attempts(keys, "{bbbbbbbb-0000-0000-0000-000000000002}");
    ASSERT_EQ(size_t(2), order.size());
    EXPECT_EQ(&keys[1], order[0]);
    EXPECT_EQ(&keys[0], order[1]);

    /* no target GUID known: all records, enumeration order */
    order = select_auto_unlock_attempts(keys, nullptr);
    ASSERT_EQ(size_t(2), order.size());
    EXPECT_EQ(&keys[0], order[0]);
    EXPECT_EQ(&keys[1], order[1]);
}

/* -------------------------------------------------------- bounds safety */

TEST(Reghive, TruncatedAndMutatedHivesParseOrRefuseWithoutCrashing) {
    /* deterministic mutation sweep: flip bytes across the header and body;
     * the parser must either refuse or parse - never read OOB (ASan/UBSan
     * would catch it). This is the structure-aware seed for fuzz_reghive. */
    FveHive fh;
    fh.add_key("{AAAAAAAA-0000-0000-0000-000000000001}", std::vector<u8>(64, 0x42));
    auto hive = fh.b.build();
    std::vector<u8> mut = hive;
    for (size_t off = 0; off < mut.size(); off += 97) {
        mut[off] = u8(mut[off] ^ 0xFF);
        RegistryHive rh;
        if (rh.open(mut.data(), mut.size())) {
            std::vector<AutoUnlockKey> keys;
            harvest_fve_auto_unlock(rh, keys);
            rh.find_key("\\ControlSet001\\Control\\FVEAutoUnlock");
        }
        mut[off] = u8(mut[off] ^ 0xFF);
    }
    /* truncation sweep */
    for (size_t cut = 0; cut < hive.size(); cut += 531) {
        RegistryHive rh;
        if (rh.open(hive.data(), cut)) {
            std::vector<AutoUnlockKey> keys;
            harvest_fve_auto_unlock(rh, keys);
        }
    }
    SUCCEED();
}

/* ------------------------------------------------- M-082: offline OS profile */

namespace {

/* the SOFTWARE side: \Microsoft\Windows NT\CurrentVersion with the og's
 * 8-field identity set (DeviceManager_decompiled.c :62034-62119) */
struct SoftwareHive {
    HiveBuilder b;
    u32 cv = 0;

    SoftwareHive() {
        const u32 ms = b.add_nk(b.root(), "Microsoft", 0);
        const u32 winnt = b.add_nk(ms, "Windows NT", 0);
        cv = b.add_nk(winnt, "CurrentVersion", 0);
    }
    void add(const char* name, const std::string& v) {
        b.add_vk(cv, name, 1 /* REG_SZ */, std::vector<u8>(v.begin(), v.end()));
    }
};

/* the SYSTEM side: \MountedDevices + \Select\Current + <CS>\Control\
 * ProductOptions\ProductType (the og's walk order, :62120-62322) */
struct SystemHive {
    HiveBuilder b;
    u32 mounted = 0, select = 0;

    SystemHive() {
        mounted = b.add_nk(b.root(), "MountedDevices", 0);
        select = b.add_nk(b.root(), "Select", 0);
    }
    void set_current(u32 n) {
        std::vector<u8> v(4);
        for (int i = 0; i < 4; ++i)
            v[i] = u8(n >> (8 * i));
        b.add_vk(select, "Current", 4 /* REG_DWORD */, v);
    }
    u32 add_control_set(const std::string& n, const char* product_type) {
        u32 cs = b.add_nk(b.root(), n, 0);
        u32 control = b.add_nk(cs, "Control", 0);
        u32 po = b.add_nk(control, "ProductOptions", 0);
        if (product_type)
            b.add_vk(po, "ProductType", 1,
                     std::vector<u8>(product_type, product_type + std::strlen(product_type)));
        return cs;
    }
    void add_dos_device(const std::string& vname, u32 sig, u64 off) {
        std::vector<u8> v(12);
        for (int i = 0; i < 4; ++i)
            v[i] = u8(sig >> (8 * i));
        for (int i = 0; i < 8; ++i)
            v[4 + i] = u8(off >> (8 * i));
        b.add_vk(mounted, vname, 3 /* REG_BINARY */, v);
    }
};

} // namespace

TEST(OfflineOs, ProfileReadsCurrentVersion) {
    SoftwareHive sw;
    sw.add("BuildLab", "22621.nt_amd64fre.220606-1250");
    sw.add("BuildLabEx", "22621.1.amd64fre.rs_onecore_base.220606-1250");
    sw.add("CSDVersion", "Service Pack 1");
    sw.add("CurrentBuildNumber", "22621");
    sw.add("CurrentVersion", "6.1");
    sw.add("ProductName", "Windows 10 Pro");
    sw.add("ProductId", "00330-80000-00000-AA123");
    sw.add("SystemRoot", "\\Device\\HarddiskVolume3\\Windows"); /* NOT x: form */

    auto bytes = sw.b.build();
    RegistryHive hive;
    ASSERT_TRUE(hive.open(bytes.data(), bytes.size()));
    offlineos::OsProfile p;
    ASSERT_TRUE(offlineos::profile_os(&hive, nullptr, p));
    EXPECT_STREQ(p.build_lab, "22621.nt_amd64fre.220606-1250");
    EXPECT_STREQ(p.build_lab_ex, "22621.1.amd64fre.rs_onecore_base.220606-1250");
    EXPECT_STREQ(p.csd_version, "Service Pack 1");
    EXPECT_STREQ(p.current_build, "22621");
    EXPECT_STREQ(p.current_version, "6.1");
    EXPECT_STREQ(p.product_name, "Windows 10 Pro");
    EXPECT_STREQ(p.product_id, "00330-80000-00000-AA123");
    /* the og's gate: SystemRoot must be a drive-letter form - a
     * \Device\... root is REJECTED (no copy, no SYSTEM pass) */
    EXPECT_STREQ(p.system_root, "");
    EXPECT_FALSE(p.has_mounted_device);
    EXPECT_EQ(p.product_type, 0u);
}

TEST(OfflineOs, ControlSetResolvesViaSelectWithFallback) {
    { /* \Select\Current = 2 -> "\ControlSet002" (the og's %03d
       * rendering - DAT_18007d37c decoded from DeviceManager.mo) */
        SystemHive sy;
        sy.set_current(2);
        sy.add_control_set("ControlSet001", nullptr);
        sy.add_control_set("ControlSet002", "WinNT");
        auto bytes = sy.b.build();
        RegistryHive hive;
        ASSERT_TRUE(hive.open(bytes.data(), bytes.size()));
        EXPECT_EQ(offlineos::resolve_control_set(hive), "\\ControlSet002");
    }
    { /* no \Select\Current -> the "\ControlSet001" fallback (:62213-62226) */
        SystemHive sy;
        sy.add_control_set("ControlSet001", nullptr);
        auto bytes = sy.b.build();
        RegistryHive hive;
        ASSERT_TRUE(hive.open(bytes.data(), bytes.size()));
        EXPECT_EQ(offlineos::resolve_control_set(hive), "\\ControlSet001");
    }
}

TEST(OfflineOs, SystemPartitionFromMountedDevices) {
    SoftwareHive sw;
    sw.add("SystemRoot", "C:\\Windows"); /* the accepted drive-letter form */
    SystemHive sy;
    sy.add_dos_device("\\DosDevices\\C:", 0x1A2B3C4Du, 0x100000ull); /* sig + 1 MiB */
    /* a decoy letter must not claim the profile */
    sy.add_dos_device("\\DosDevices\\D:", 0xDEADBEEFu, 0x200000ull);
    sy.set_current(1);
    sy.add_control_set("ControlSet001", "WinNT");

    auto swb = sw.b.build(), syb = sy.b.build();
    RegistryHive soft, sys;
    ASSERT_TRUE(soft.open(swb.data(), swb.size()));
    ASSERT_TRUE(sys.open(syb.data(), syb.size()));
    offlineos::OsProfile p;
    ASSERT_TRUE(offlineos::profile_os(&soft, &sys, p));
    /* SystemRoot lowercased by the og's _strlwr_s gate */
    EXPECT_STREQ(p.system_root, "c:\\windows");
    EXPECT_TRUE(p.has_mounted_device);
    EXPECT_EQ(p.disk_signature, 0x1A2B3C4Du);
    EXPECT_EQ(p.partition_offset, 0x100000ull);
    EXPECT_EQ(p.product_type, offlineos::OsProfile::kProductWinNt);
}

TEST(OfflineOs, ServerVsClientClassification) {
    SoftwareHive sw;
    sw.add("SystemRoot", "E:\\WINNT");
    struct Case {
        const char* product_type;
        u8 want;
    };
    const Case cases[] = {
        {"WinNT", offlineos::OsProfile::kProductWinNt},
        {"winnt", offlineos::OsProfile::kProductWinNt}, /* lstrcmpiA: ci */
        {"ServerNT", offlineos::OsProfile::kProductServerNt},
        {"LanmanNT", offlineos::OsProfile::kProductLanmanNt},
        {"Terminal Server", 0u}, /* unknown -> stays 0 */
    };
    for (const Case& c : cases) {
        SystemHive sy;
        sy.set_current(1);
        sy.add_control_set("ControlSet001", c.product_type);
        auto swb = sw.b.build(), syb = sy.b.build();
        RegistryHive soft, sys;
        ASSERT_TRUE(soft.open(swb.data(), swb.size()));
        ASSERT_TRUE(sys.open(syb.data(), syb.size()));
        offlineos::OsProfile p;
        ASSERT_TRUE(offlineos::profile_os(&soft, &sys, p)) << c.product_type;
        EXPECT_EQ(p.product_type, c.want) << c.product_type;
        EXPECT_EQ(p.disk_signature, 0u) << "no MountedDevices value: stays zero";
        EXPECT_FALSE(p.has_mounted_device);
    }
}

TEST(OfflineOs, ToleratesMissingKeysAndOddValueShapes) {
    { /* SOFTWARE without the CurrentVersion family: an empty-ish profile,
       * not a failure (the og's skip-if-absent shape) */
        HiveBuilder b;
        auto bytes = b.build();
        RegistryHive hive;
        ASSERT_TRUE(hive.open(bytes.data(), bytes.size()));
        offlineos::OsProfile p;
        ASSERT_TRUE(offlineos::profile_os(&hive, nullptr, p));
        EXPECT_STREQ(p.product_name, "");
    }
    { /* a MountedDevices value that is NOT exactly 12 bytes: the og's
       * local_1a8 == 0xc gate - no location claim */
        SoftwareHive sw;
        sw.add("SystemRoot", "C:\\Windows");
        SystemHive sy;
        sy.mounted = sy.b.add_nk(sy.b.root(), "MountedDevices", 0);
        sy.b.add_vk(sy.mounted, "\\DosDevices\\C:", 3, std::vector<u8>(8, 0xAA));
        auto swb = sw.b.build(), syb = sy.b.build();
        RegistryHive soft, sys;
        ASSERT_TRUE(soft.open(swb.data(), swb.size()));
        ASSERT_TRUE(sys.open(syb.data(), syb.size()));
        offlineos::OsProfile p;
        ASSERT_TRUE(offlineos::profile_os(&soft, &sys, p));
        EXPECT_FALSE(p.has_mounted_device);
    }
    { /* null software hive: refused */
        offlineos::OsProfile p;
        EXPECT_FALSE(offlineos::profile_os(nullptr, nullptr, p));
    }
}
