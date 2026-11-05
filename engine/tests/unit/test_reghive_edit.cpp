/* test_reghive_edit.cpp - M-113 acceptance: the offline hive value
 * editor + the recovered-OS boot surgery family (the og's
 * genuefisystemfile.cpp write side). The synthetic-hive builder is the
 * mountpoints2 suite's regf layout (one hbin, nk/vk/lf cells, the XOR
 * checksum header), extended with the surgery targets.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include <gtest/gtest.h>
#include "cr/reghive/cr_reghive_edit.h"
#include "cr/reghive/cr_surgery.h"
#include "cr/reghive/cr_reghive.h"
#include "cr/cr_error.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::reghive;

namespace {

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}
void put32(std::vector<u8>& v, size_t at, u32 x) {
    for (int i = 0; i < 4; ++i)
        v[at + i] = u8(x >> (8 * i));
}
u32 get32(const std::vector<u8>& v, size_t at) {
    return u32(v[at]) | (u32(v[at + 1]) << 8) | (u32(v[at + 2]) << 16) | (u32(v[at + 3]) << 24);
}

/* the mountpoints2 suite's minimal regf builder (the shared layout),
 * extended: a UTF-16 data writer for REG_SZ values */
class HiveBuilder {
public:
    HiveBuilder() {
        bins_.resize(0x1000, 0);
        std::memcpy(bins_.data(), "hbin", 4);
        put32(bins_, 8, 0x1000);
        root_off_ = 0x20;
        root_cell_ = add_nk(0, "ROOTKEY", 0);
        (void)root_cell_;
    }

    u32 add_nk(u32 parent, const std::string& name, u16 flags) {
        std::vector<u8> rec(0x4C + (flags & 0x20 ? name.size() : name.size() * 2), 0);
        rec[0] = 'n';
        rec[1] = 'k';
        put16(rec, 0x02, flags);
        if (flags & 0x20) {
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

    u32 add_vk(u32 nk, const std::string& name, u32 type, const std::vector<u8>& data) {
        std::vector<u8> rec(0x12 + name.size(), 0);
        rec[0] = 'v';
        rec[1] = 'k';
        put16(rec, 0x02, u16(name.size()));
        rec[0x10] = 1; /* ASCII name flag */
        std::memcpy(rec.data() + 0x12, name.data(), name.size());
        put32(rec, 0x0C, type);
        if (data.size() <= 4) {
            put32(rec, 0x04, u32(data.size()) | 0x80000000u);
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

    /* a REG_SZ value carrying a UTF-16 string (the BCD element shape) */
    u32 add_vk_utf16(u32 nk, const std::string& name, const std::u16string& text) {
        std::vector<u8> data(text.size() * 2);
        for (size_t i = 0; i < text.size(); ++i)
            put16(data, i * 2, u16(text[i]));
        return add_vk(nk, name, 1 /* REG_SZ */, data);
    }

    std::vector<u8> build() {
        for (const auto& [parent, kids] : children_) {
            std::vector<u8> rec(4 + kids.size() * 8, 0);
            rec[0] = 'l';
            rec[1] = 'f';
            put16(rec, 0x02, u16(kids.size()));
            for (size_t i = 0; i < kids.size(); ++i) {
                put32(rec, 4 + i * 8, kids[i]);
                put32(rec, 4 + i * 8 + 4, 0);
            }
            const u32 list_off = append_cell(rec);
            patch_nk(parent, 0x14, u32(kids.size()));
            patch_nk(parent, 0x1C, list_off);
        }
        for (const auto& [nk, vks] : values_) {
            std::vector<u8> rec(vks.size() * 4, 0);
            for (size_t i = 0; i < vks.size(); ++i)
                put32(rec, i * 4, vks[i]);
            const u32 list_off = append_cell(rec);
            patch_nk(nk, 0x24, u32(vks.size()));
            patch_nk(nk, 0x28, list_off);
        }
        std::vector<u8> hive(0x1000 + bins_.size(), 0);
        std::memcpy(hive.data(), "regf", 4);
        put32(hive, 0x18, 6);
        put32(hive, REGF_ROOT_CELL_OFF, root_off_);
        put32(hive, REGF_BINS_SIZE_OFF, u32(bins_.size()));
        std::memcpy(hive.data() + 0x1000, bins_.data(), bins_.size());
        u32 acc = 0;
        for (u32 off = 0; off < 0x1FC; off += 4)
            acc ^= get32(hive, off);
        put32(hive, 0x1FC, acc);
        return hive;
    }

    u32 root() const { return root_off_; }

private:
    std::vector<u8> bins_;
    u32 root_off_ = 0;
    u32 root_cell_ = 0;
    std::map<u32, std::vector<u32>> children_;
    std::map<u32, std::vector<u32>> values_;

    u32 append_cell(const std::vector<u8>& rec) {
        u32 off = next_off_;
        const size_t need = 4 + rec.size();
        const size_t aligned = (need + 7) & ~size_t(7);
        while (off + aligned > bins_.size())
            bins_.resize(bins_.size() + 0x1000, 0);
        put32(bins_, off, u32(-i64(aligned))); /* negative = allocated */
        std::memcpy(bins_.data() + off + 4, rec.data(), rec.size());
        put32(bins_, 8, u32(bins_.size())); /* hbin size refresh */
        next_off_ = off + u32(aligned);
        return off;
    }

    void patch_nk(u32 off, size_t at, u32 value) { put32(bins_, off + 4 + at, value); }

    u32 next_off_ = 0x20;
};

/* a SYSTEM hive carrying \MountedDevices with two DosDevices values */
std::vector<u8> build_system_hive() {
    HiveBuilder b;
    const u32 root = b.root();
    const u32 mounted = b.add_nk(root, "MountedDevices", 0);
    /* the old disk: signature 0x11111111, C: at 1 MiB, D: at 2 GiB */
    auto md = [](u32 sig, u64 off) {
        std::vector<u8> v(12);
        put32(v, 0, sig);
        for (int i = 0; i < 8; ++i)
            v[4 + i] = u8(off >> (8 * i));
        return v;
    };
    (void)b.add_vk(mounted, "\\DosDevices\\C:", 3, md(0x11111111, 1048576));
    (void)b.add_vk(mounted, "\\DosDevices\\D:", 3, md(0x11111111, 2147483648ull));
    return b.build();
}

/* a BCD-shaped hive: the boot-manager object's 23000003 element
 * referencing the boot-application object's GUID, which carries the
 * 12000004 device element */
std::vector<u8> build_bcd_hive(const std::u16string& device_text) {
    HiveBuilder b;
    const u32 root = b.root();
    const std::string mgr = surgery::kBcdBootManagerGuid; /* braces included */
    const std::string app = "{faa7f9e0-6d1c-11d9-b0e4-0050568900de}";
    const u32 objects = b.add_nk(root, "Objects", 0);
    const u32 mgr_obj = b.add_nk(objects, mgr, 0);
    const u32 mgr_elem230 = b.add_nk(mgr_obj, "Elements", 0);
    const u32 e230 = b.add_nk(mgr_elem230, "23000003", 0);
    /* the reference: the application's GUID as UTF-16 */
    std::u16string app_w;
    for (char c : app)
        app_w.push_back(char16_t(c));
    (void)b.add_vk_utf16(e230, "Element", app_w);
    const u32 app_obj = b.add_nk(objects, app, 0);
    const u32 app_elem120 = b.add_nk(app_obj, "Elements", 0);
    const u32 e120 = b.add_nk(app_elem120, "12000004", 0);
    (void)b.add_vk_utf16(e120, "Element", device_text);
    return b.build();
}

} // namespace

/* ---- the editor -------------------------------------------------------- */

TEST(HiveEdit, RewritesValueInPlaceWhenItFits) {
    auto hive = build_system_hive();
    HiveEditor ed;
    ASSERT_TRUE(ed.open(hive.data(), hive.size()));
    const u32 seq0 = ed.sequence();
    /* same size (12 bytes): pure in-place */
    std::vector<u8> v(12, 0xAB);
    ASSERT_EQ(OK, ed.set_value("\\MountedDevices", "\\DosDevices\\C:", 3, v.data(), v.size()));
    u32 type = 0;
    std::vector<u8> out;
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\C:", type, out));
    EXPECT_EQ(type, 3u);
    EXPECT_EQ(out, v);
    EXPECT_EQ(ed.sequence(), seq0 + 1) << "the sequence bumps on mutation";
    /* the buffer still opens as a valid hive (checksum current) */
    RegistryHive check;
    EXPECT_TRUE(check.open(ed.bytes().data(), ed.bytes().size()));
}

TEST(HiveEdit, GrowsValueIntoANewCellWhenLarger) {
    auto hive = build_system_hive();
    const size_t size_before = hive.size();
    HiveEditor ed;
    ASSERT_TRUE(ed.open(hive.data(), hive.size()));
    /* much larger data: a new cell must be allocated (free-cell fit or a
     * fresh hbin) - never a truncated write */
    std::vector<u8> big(600, 0x5A);
    ASSERT_EQ(OK, ed.set_value("\\MountedDevices", "\\DosDevices\\C:", 3, big.data(), big.size()));
    u32 type = 0;
    std::vector<u8> out;
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\C:", type, out));
    EXPECT_EQ(out, big);
    EXPECT_GE(ed.bytes().size(), size_before); /* grew or reused */
    RegistryHive check;
    EXPECT_TRUE(check.open(ed.bytes().data(), ed.bytes().size()));
    /* the OTHER value is untouched */
    std::vector<u8> d;
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\D:", type, d));
    EXPECT_EQ(d.size(), 12u);
    EXPECT_EQ(get32(d, 0), 0x11111111u);
}

TEST(HiveEdit, ShrinksValueAndWritesInlineBelowFour) {
    auto hive = build_system_hive();
    HiveEditor ed;
    ASSERT_TRUE(ed.open(hive.data(), hive.size()));
    /* 3 bytes -> the inline form (bit 31) */
    const u8 small[3] = {1, 2, 3};
    ASSERT_EQ(OK, ed.set_value("\\MountedDevices", "\\DosDevices\\C:", 3, small, sizeof(small)));
    u32 type = 0;
    std::vector<u8> out;
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\C:", type, out));
    EXPECT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], 1);
    EXPECT_EQ(out[2], 3);
    /* back up to 12: cell allocation again */
    std::vector<u8> v(12, 0x77);
    ASSERT_EQ(OK, ed.set_value("\\MountedDevices", "\\DosDevices\\C:", 3, v.data(), v.size()));
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\C:", type, out));
    EXPECT_EQ(out, v);
    RegistryHive check;
    EXPECT_TRUE(check.open(ed.bytes().data(), ed.bytes().size()));
}

TEST(HiveEdit, AbsentValueIsAnHonestError) {
    auto hive = build_system_hive();
    HiveEditor ed;
    ASSERT_TRUE(ed.open(hive.data(), hive.size()));
    const u32 seq0 = ed.sequence();
    const u8 v[12] = {};
    EXPECT_EQ(ERR_BAD_PARAMETER,
              ed.set_value("\\MountedDevices", "\\DosDevices\\Q:", 3, v, sizeof(v)));
    EXPECT_EQ(ERR_BAD_PARAMETER, ed.set_value("\\NoSuchKey", "x", 3, v, sizeof(v)));
    EXPECT_EQ(ed.sequence(), seq0) << "a failed write never bumps the header";
}

/* ---- the MountedDevices surgery ---------------------------------------- */

TEST(BootSurgery, MountedDevicesRewriteRoundTrip) {
    /* THE acceptance: the recovered OS's SYSTEM hive re-points to the new
     * disk - the offlineos profile (the og's DeviceManager read side)
     * must decode the NEW signature + offset after the surgery. */
    auto hive = build_system_hive();
    const surgery::MountedDeviceMapping maps[2] = {
        {'C', 0xAABBCCDD, 1048576},
        {'D', 0xAABBCCDD, (1ull << 32) + 4096}, /* past 4 GiB: exercises the
                                                 * u64's high dword */
    };
    ASSERT_EQ(OK, surgery::rewrite_mounted_devices(hive, maps, 2));

    /* read back through the surgery's own editor view... */
    HiveEditor ed;
    ASSERT_TRUE(ed.open(hive.data(), hive.size()));
    u32 type = 0;
    std::vector<u8> c, d;
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\C:", type, c));
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\D:", type, d));
    EXPECT_EQ(type, 3u);
    ASSERT_EQ(c.size(), 12u);
    EXPECT_EQ(get32(c, 0), 0xAABBCCDDu);
    EXPECT_EQ(get32(c, 4), 1048576u);
    ASSERT_EQ(d.size(), 12u);
    EXPECT_EQ(get32(d, 0), 0xAABBCCDDu);
    EXPECT_EQ(get32(d, 4), 4096u) << "the u64 offset low dword";
    EXPECT_EQ(get32(d, 8), 1u) << "the u64 offset high dword";
}

TEST(BootSurgery, MountedDevicesRejectsBadLettersAndPreservesBytes) {
    auto hive = build_system_hive();
    const std::vector<u8> pristine = hive;
    const surgery::MountedDeviceMapping bad[1] = {
        {'c', 1, 2}, /* lowercase: outside the og's SystemRoot band */
    };
    EXPECT_EQ(ERR_BAD_PARAMETER, surgery::rewrite_mounted_devices(hive, bad, 1));
    /* a mapping targeting an absent value refuses BEFORE any byte moves:
     * C: exists but E: does not - the C: mapping must NOT land */
    const surgery::MountedDeviceMapping partial[2] = {
        {'C', 0x99999999, 1},
        {'E', 0x99999999, 1},
    };
    EXPECT_EQ(ERR_BAD_PARAMETER, surgery::rewrite_mounted_devices(hive, partial, 2));
    EXPECT_EQ(hive, pristine) << "no partial rewrite: bytes byte-exact";
}

/* ---- the BCD surgery --------------------------------------------------- */

TEST(BootSurgery, BcdDeviceElementRepoint) {
    /* THE acceptance: the boot manager's referenced application's device
     * element carries the NEW device description after the surgery (the
     * og's {9dea862c} 23000003 -> 12000004 walk). */
    auto bcd = build_bcd_hive(u"partition=\\device\\harddisk0\\partition2");
    const char16_t* newdev = u"partition=\\device\\harddisk1\\partition3";
    ASSERT_EQ(OK, surgery::repoint_bcd_device_element(bcd, newdev));

    HiveEditor ed;
    ASSERT_TRUE(ed.open(bcd.data(), bcd.size()));
    /* the referenced application's 12000004 now answers the new device */
    const std::string app_key =
        "\\Objects\\{faa7f9e0-6d1c-11d9-b0e4-0050568900de}\\Elements\\12000004";
    u32 type = 0;
    std::vector<u8> out;
    ASSERT_TRUE(ed.read_value(app_key.c_str(), "Element", type, out));
    EXPECT_EQ(type, 1u) << "the og writes type 1 (REG_SZ) - the finding's "
                        << "REG_BINARY annotation corrected by the decode";
    /* the bytes are the wide string WITHOUT the terminator (wcslen*2) */
    std::u16string expect;
    for (const char16_t* p = newdev; *p; ++p)
        expect.push_back(*p);
    ASSERT_EQ(out.size(), expect.size() * 2);
    for (size_t i = 0; i < expect.size(); ++i) {
        EXPECT_EQ(out[i * 2], u8(u16(expect[i]) & 0xFF)) << "wide byte " << i;
        EXPECT_EQ(out[i * 2 + 1], u8(u16(expect[i]) >> 8)) << "wide byte " << i;
    }
    /* the boot manager's own 23000003 is untouched */
    const std::string mgr_key =
        std::string("\\Objects\\") + surgery::kBcdBootManagerGuid + "\\Elements\\23000003";
    std::vector<u8> ref;
    ASSERT_TRUE(ed.read_value(mgr_key.c_str(), "Element", type, ref));
    EXPECT_EQ(type, 1u);
}

TEST(BootSurgery, BcdSelfReferenceRefuses) {
    /* the boot manager referencing ITSELF (the og's wcslen-compare guard)
     * is refused - never a self-repoint */
    HiveBuilder b;
    const u32 root = b.root();
    const std::string mgr = std::string(surgery::kBcdBootManagerGuid);
    const u32 objects = b.add_nk(root, "Objects", 0);
    const u32 mgr_obj = b.add_nk(objects, mgr, 0);
    const u32 mgr_elem = b.add_nk(mgr_obj, "Elements", 0);
    const u32 e230 = b.add_nk(mgr_elem, "23000003", 0);
    std::u16string self_w;
    for (char c : mgr)
        self_w.push_back(char16_t(c));
    (void)b.add_vk_utf16(e230, "Element", self_w);
    auto bcd = b.build();
    EXPECT_EQ(ERR_BAD_PARAMETER, surgery::repoint_bcd_device_element(bcd, u"partition=x"));
}

TEST(BootSurgery, BcdWithoutManagerElementRefusesHonestly) {
    /* a hive with no boot-manager object is not a BCD - refuse, never a
     * guess (the og's OpenKey null-check shape) */
    HiveBuilder b;
    (void)b.add_nk(b.root(), "SomethingElse", 0);
    auto bcd = b.build();
    EXPECT_EQ(ERR_BAD_PARAMETER, surgery::repoint_bcd_device_element(bcd, u"partition=x"));
}

TEST(BootSurgery, BootstatContractPinned) {
    /* the og's byte-exact file contract (:42845-42860): offset 10, 0x01 */
    EXPECT_EQ(surgery::kBootstatOffset, 10u);
    EXPECT_EQ(surgery::kBootstatValue, 0x01);
}

/* ---- undo: the caller's pre-image -------------------------------------- */

TEST(BootSurgery, UndoIsTheCallersByteExactPreimage) {
    /* the surgery mutates only the PASSED buffer: the host that kept the
     * original bytes has byte-exact undo by writing them back */
    auto hive = build_system_hive();
    const std::vector<u8> pristine = hive;
    const surgery::MountedDeviceMapping maps[1] = {{'C', 0x42424242, 512}};
    ASSERT_EQ(OK, surgery::rewrite_mounted_devices(hive, maps, 1));
    EXPECT_NE(hive, pristine);
    /* undo: restore the pre-image; the hive reads the OLD signature */
    hive = pristine;
    HiveEditor ed;
    ASSERT_TRUE(ed.open(hive.data(), hive.size()));
    u32 type = 0;
    std::vector<u8> out;
    ASSERT_TRUE(ed.read_value("\\MountedDevices", "\\DosDevices\\C:", type, out));
    EXPECT_EQ(get32(out, 0), 0x11111111u) << "undo restores the old disk identity";
}
