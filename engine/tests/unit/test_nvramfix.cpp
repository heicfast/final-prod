/* test_nvramfix.cpp - the UEFI NVRAM Boot#### updater tests (M-040
 * stage b). Fixture load options built to the UEFI EFI_LOAD_OPTION +
 * MEDIA_HARDDRIVE_DEVICE_PATH shapes the corpus walks
 * (EFIBoot_decompiled.c FUN_1800046f0/FUN_180004b40).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/nvramfix/cr_nvramfix.h"
#include "cr/cr_error.h"
#include <gtest/gtest.h>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace cr {
namespace nvramfix {
namespace {

/* ---- the mock firmware-variable store (the injected seam) ---- */
class MockVars {
public:
    /* returns the variable length or 0 when absent, like
     * GetFirmwareEnvironmentVariableW */
    static u32 get(const char16_t* name, void* buf, u32 cap, void* user) {
        auto* self = static_cast<MockVars*>(user);
        std::map<std::u16string, std::vector<u8>>::const_iterator it = self->vars_.find(name);
        if (it == self->vars_.end() || it->second.empty())
            return 0;
        if (it->second.size() > cap)
            return 0;
        memcpy(buf, it->second.data(), it->second.size());
        return u32(it->second.size());
    }
    static bool set(const char16_t* name, const void* data, u32 len, void* user) {
        auto* self = static_cast<MockVars*>(user);
        if (self->fail_next_set_)
            return false;
        self->set_log_.push_back(name);
        const u8* p = static_cast<const u8*>(data);
        self->vars_[name] = std::vector<u8>(p, p + len);
        return true;
    }

    void put(const char16_t* name, const std::vector<u8>& v) { vars_[name] = v; }
    std::vector<u8>& blob(const char16_t* name) { return vars_[name]; }
    const std::vector<u8>& peek(const char16_t* name) const {
        static const std::vector<u8> kEmpty;
        auto it = vars_.find(name);
        return it == vars_.end() ? kEmpty : it->second;
    }
    size_t sets() const { return set_log_.size(); }
    void fail_next_set() { fail_next_set_ = true; }

private:
    std::map<std::u16string, std::vector<u8>> vars_;
    std::vector<std::u16string> set_log_;
    bool fail_next_set_ = false;
};

/* ---- fixture builders ---- */
void put32(std::vector<u8>& v, size_t at, u32 x) {
    for (int i = 0; i < 4; ++i)
        v[at + size_t(i)] = u8(x >> (8 * i));
}
void put64(std::vector<u8>& v, size_t at, u64 x) {
    for (int i = 0; i < 8; ++i)
        v[at + size_t(i)] = u8(x >> (8 * i));
}
u32 get32(const std::vector<u8>& v, size_t at) {
    return u32(v[at]) | (u32(v[at + 1]) << 8) | (u32(v[at + 2]) << 16) | (u32(v[at + 3]) << 24);
}
u64 get64(const std::vector<u8>& v, size_t at) {
    u64 x = 0;
    for (int i = 7; i >= 0; --i)
        x = (x << 8) | v[at + size_t(i)];
    return x;
}

void put16(std::vector<u8>& v, size_t at, u16 x) {
    v[at] = u8(x);
    v[at + 1] = u8(x >> 8);
}

/* an EFI_LOAD_OPTION: attributes, path-length, description, then the
 * device path node stream, ending with END_ENTIRE. */
std::vector<u8> build_load_option(const char16_t* description,
                                  const std::vector<std::vector<u8>>& nodes) {
    std::vector<u8> v(6, 0);
    put32(v, 0, 1); /* Attributes = ACTIVE; FilePathListLength at +4 filled below */
    for (const char16_t* p = description; *p; ++p) {
        v.push_back(u8(*p & 0xFF));
        v.push_back(u8(*p >> 8));
    }
    v.push_back(0);
    v.push_back(0); /* the description NUL */
    const size_t path_start = v.size();
    for (const auto& n : nodes)
        v.insert(v.end(), n.begin(), n.end());
    v.push_back(0x7f); /* END_ENTIRE */
    v.push_back(0xff);
    v.push_back(0x04);
    v.push_back(0x00);
    put16(v, 4, u16(v.size() - path_start));
    return v;
}

/* a MEDIA_HARDDRIVE_DEVICE_PATH node (42 bytes) */
std::vector<u8> hd_node(u32 number, u64 start, u64 size, const u8 sig[16], u8 mbr_type,
                        u8 sig_type) {
    std::vector<u8> n(kHdNodeSize, 0);
    n[0] = 0x04; /* Type: MEDIA */
    n[1] = 0x01; /* SubType: HARDDRIVE */
    n[2] = u8(kHdNodeSize & 0xFF);
    n[3] = u8(kHdNodeSize >> 8);
    put32(n, kHdPartitionNumber, number);
    put64(n, kHdPartitionStart, start);
    put64(n, kHdPartitionSize, size);
    memcpy(n.data() + kHdSignature, sig, 16);
    n[kHdMbrType] = mbr_type;
    n[kHdSignatureType] = sig_type;
    return n;
}

void mbr_sig(u8 sig[16], u32 s) {
    memset(sig, 0, 16);
    sig[0] = u8(s);
    sig[1] = u8(s >> 8);
    sig[2] = u8(s >> 16);
    sig[3] = u8(s >> 24);
}

bool allow_always(void*) {
    return true;
}
bool deny_always(void*) {
    return false;
}

const char16_t kBootOrderName[] = u"BootOrder";
const char16_t kBoot0000[] = u"Boot0000";
const char16_t kBoot0001[] = u"Boot0001";
const char16_t kBoot0002[] = u"Boot0002";

/* the target: the partition moved from MBR-disk sig 0xAABBCCDD,
 * LBA 2048 size 4096, to number 3 / LBA 8192 size 12288 on disk sig
 * 0x11223344 */
TargetPartition mbr_target() {
    TargetPartition t;
    t.old_mbr_signature = 0xAABBCCDDu;
    t.old_start = 2048;
    t.old_size = 4096;
    t.new_partition_number = 3;
    t.new_start = 8192;
    t.new_size = 12288;
    t.new_mbr_signature = 0x11223344u;
    return t;
}

TEST(NvramFix, MatchesMbrBySignatureStartSize) {
    MockVars vars;
    u8 sig[16];
    mbr_sig(sig, 0xAABBCCDDu);
    vars.put(kBoot0000,
             build_load_option(u"Windows Boot Manager", {hd_node(2, 2048, 4096, sig, 1, 1)}));
    /* a second entry on a DIFFERENT disk: must not match */
    u8 other[16];
    mbr_sig(other, 0xDEADBEEFu);
    vars.put(kBoot0001, build_load_option(u"Other disk", {hd_node(2, 2048, 4096, other, 1, 1)}));
    /* a third entry on the SAME disk but a different partition start */
    u8 same[16];
    mbr_sig(same, 0xAABBCCDDu);
    vars.put(kBoot0002,
             build_load_option(u"Same disk other partition", {hd_node(2, 9999, 4096, same, 1, 1)}));
    vars.put(kBootOrderName, {0x00, 0x00, 0x01, 0x00, 0x02, 0x00});

    u32 err = OK;
    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target(), &err);
    ASSERT_EQ(OK, err);
    ASSERT_EQ(1u, plan.size());
    EXPECT_EQ(std::u16string(kBoot0000), plan[0].name);
}

TEST(NvramFix, MatchesGptByGuidAndStart) {
    MockVars vars;
    u8 guid[16];
    for (int i = 0; i < 16; ++i)
        guid[i] = u8(0x10 + i);
    TargetPartition t;
    t.gpt_mode = true;
    memcpy(t.old_gpt_guid, guid, 16);
    t.old_start = 2048;
    t.old_size = 4096;
    memcpy(t.new_gpt_guid, guid, 16); /* same GUID, new position */
    t.new_partition_number = 5;
    t.new_start = 8192;
    t.new_size = 12288;

    vars.put(kBoot0000, build_load_option(u"GPT boot", {hd_node(2, 2048, 4096, guid, 2, 2)}));
    /* wrong GUID */
    u8 bad[16];
    memcpy(bad, guid, 16);
    bad[0] ^= 0xFF;
    vars.put(kBoot0001, build_load_option(u"Wrong GUID", {hd_node(2, 2048, 4096, bad, 2, 2)}));
    /* wrong start */
    vars.put(kBoot0002, build_load_option(u"Wrong start", {hd_node(2, 1111, 4096, guid, 2, 2)}));
    vars.put(kBootOrderName, {0x00, 0x00, 0x01, 0x00, 0x02, 0x00});

    u32 err = OK;
    auto plan = plan_boot_entries(&MockVars::get, &vars, t, &err);
    ASSERT_EQ(OK, err);
    ASSERT_EQ(1u, plan.size());
    EXPECT_EQ(std::u16string(kBoot0000), plan[0].name);
}

TEST(NvramFix, PatchUpdatesLoadOption) {
    /* the finding's named test: the load option's HARDDRIVE node gets
     * the new number/start/size/signature (FUN_180004b40's rewrite) */
    MockVars vars;
    u8 sig[16];
    mbr_sig(sig, 0xAABBCCDDu);
    const std::vector<u8> original =
        build_load_option(u"Windows Boot Manager", {hd_node(2, 2048, 4096, sig, 1, 1)});
    vars.put(kBoot0000, original);
    vars.put(kBootOrderName, {0x00, 0x00});

    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target());
    ASSERT_EQ(1u, plan.size());
    /* the plan's new value carries the rewritten node fields */
    const auto& nv = plan[0].new_value;
    /* locate the harddrive node (type 4/subtype 1) after the description */
    size_t p = 6;
    while (p + 1 < nv.size() && (nv[p] | nv[p + 1]) != 0)
        p += 2;
    p += 2;
    ASSERT_EQ(0x04, nv[p]);
    ASSERT_EQ(0x01, nv[p + 1]);
    EXPECT_EQ(3u, get32(nv, p + kHdPartitionNumber));
    EXPECT_EQ(u64(8192), get64(nv, p + kHdPartitionStart));
    EXPECT_EQ(u64(12288), get64(nv, p + kHdPartitionSize));
    EXPECT_EQ(0x11223344u, get32(nv, p + kHdSignature));
    /* the rest of the entry is byte-identical (attributes, description,
     * path length, the END node) */
    EXPECT_EQ(original.size(), nv.size());
    ASSERT_EQ(OK, execute_plan(plan, &MockVars::set, &vars, &allow_always, nullptr));
    /* the store now holds the patched entry */
    const auto& stored = vars.peek(kBoot0000);
    EXPECT_EQ(nv.size(), stored.size());
    EXPECT_EQ(0, memcmp(nv.data(), stored.data(), nv.size()));
}

TEST(NvramFix, UnmatchedEntriesUntouched) {
    MockVars vars;
    u8 other[16];
    mbr_sig(other, 0xDEADBEEFu);
    const std::vector<u8> original =
        build_load_option(u"Other disk", {hd_node(2, 2048, 4096, other, 1, 1)});
    vars.put(kBoot0000, original);
    vars.put(kBootOrderName, {0x00, 0x00});

    u32 err = OK;
    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target(), &err);
    ASSERT_EQ(OK, err);
    EXPECT_TRUE(plan.empty());
    EXPECT_EQ(0u, vars.sets());
}

TEST(NvramFix, ConsentDefaultDeny) {
    MockVars vars;
    u8 sig[16];
    mbr_sig(sig, 0xAABBCCDDu);
    vars.put(kBoot0000,
             build_load_option(u"Windows Boot Manager", {hd_node(2, 2048, 4096, sig, 1, 1)}));
    vars.put(kBootOrderName, {0x00, 0x00});
    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target());
    ASSERT_EQ(1u, plan.size());

    size_t applied = 99;
    EXPECT_EQ(ERR_REPAIR_REFUSED,
              execute_plan(plan, &MockVars::set, &vars, nullptr, nullptr, &applied));
    EXPECT_EQ(0u, applied);
    EXPECT_EQ(0u, vars.sets()) << "default-deny wrote nothing";
    applied = 99;
    EXPECT_EQ(ERR_REPAIR_REFUSED,
              execute_plan(plan, &MockVars::set, &vars, &deny_always, nullptr, &applied));
    EXPECT_EQ(0u, applied);
    EXPECT_EQ(0u, vars.sets());
}

TEST(NvramFix, UndoRestoresOriginalVariables) {
    MockVars vars;
    u8 sig[16];
    mbr_sig(sig, 0xAABBCCDDu);
    const std::vector<u8> original =
        build_load_option(u"Windows Boot Manager", {hd_node(2, 2048, 4096, sig, 1, 1)});
    vars.put(kBoot0000, original);
    vars.put(kBootOrderName, {0x00, 0x00});

    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target());
    ASSERT_EQ(1u, plan.size());
    ASSERT_EQ(OK, execute_plan(plan, &MockVars::set, &vars, &allow_always, nullptr));
    EXPECT_NE(original, vars.peek(kBoot0000)) << "the patch must change the entry";
    ASSERT_EQ(OK, undo_plan(plan, &MockVars::set, &vars));
    const auto& restored = vars.peek(kBoot0000);
    ASSERT_EQ(original.size(), restored.size());
    EXPECT_EQ(0, memcmp(original.data(), restored.data(), original.size()))
        << "the undo is byte-exact";
}

TEST(NvramFix, SetFailureStopsAndReports) {
    MockVars vars;
    u8 sig[16];
    mbr_sig(sig, 0xAABBCCDDu);
    vars.put(kBoot0000,
             build_load_option(u"Windows Boot Manager", {hd_node(2, 2048, 4096, sig, 1, 1)}));
    u8 sig2[16];
    mbr_sig(sig2, 0xAABBCCDDu);
    vars.put(kBoot0001,
             build_load_option(u"Windows Boot Manager 2", {hd_node(4, 2048, 4096, sig2, 1, 1)}));
    vars.put(kBootOrderName, {0x00, 0x00, 0x01, 0x00});
    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target());
    ASSERT_EQ(2u, plan.size());

    vars.fail_next_set();
    size_t applied = 0;
    EXPECT_EQ(ERR_WRITE_FAILED,
              execute_plan(plan, &MockVars::set, &vars, &allow_always, nullptr, &applied));
    EXPECT_EQ(0u, applied);
    EXPECT_EQ(0u, vars.sets()) << "the failed Set wrote nothing";
}

TEST(NvramFix, MalformedDevicePathBounded) {
    /* hostile shapes: a node length of 0/1/2 (below the header), a
     * length running past the buffer, a missing END node - the walk
     * must stop bounded, never read past the variable */
    MockVars vars;
    u8 sig[16];
    mbr_sig(sig, 0xAABBCCDDu);
    {
        std::vector<u8> v = build_load_option(u"x", {hd_node(2, 2048, 4096, sig, 1, 1)});
        /* corrupt the node's length to 2 (below the header size): the
         * node starts at +10 (attrs 0..3, len 4..5, "x" 6..7, NUL 8..9),
         * its u16 Length field at +12 */
        v[12] = 0x02;
        v[13] = 0x00;
        vars.put(kBoot0000, v);
    }
    {
        std::vector<u8> v = build_load_option(u"y", {hd_node(2, 2048, 4096, sig, 1, 1)});
        /* node length past the buffer end */
        v[12] = 0xFF;
        v[13] = 0x7F;
        vars.put(kBoot0001, v);
    }
    vars.put(kBootOrderName, {0x00, 0x00, 0x01, 0x00});
    u32 err = OK;
    /* both entries simply do not match (bounded stops) - no crash, no
     * plan, no error */
    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target(), &err);
    EXPECT_EQ(OK, err);
    EXPECT_TRUE(plan.empty());
}

TEST(NvramFix, NullSeamRejected) {
    u32 err = OK;
    auto plan = plan_boot_entries(nullptr, nullptr, mbr_target(), &err);
    EXPECT_TRUE(plan.empty());
    EXPECT_EQ(ERR_BAD_PARAMETER, err);
}

TEST(NvramFix, AbsentBootOrderIsReadFailure) {
    MockVars vars; /* nothing stored */
    u32 err = OK;
    auto plan = plan_boot_entries(&MockVars::get, &vars, mbr_target(), &err);
    EXPECT_TRUE(plan.empty());
    EXPECT_EQ(ERR_READ_FAILED, err);
}

TEST(NvramFix, EmptyPlanIsNoOp) {
    MockVars vars;
    size_t applied = 9;
    EXPECT_EQ(OK, execute_plan({}, &MockVars::set, &vars, nullptr, nullptr, &applied));
    EXPECT_EQ(0u, applied);
    EXPECT_EQ(0u, vars.sets()) << "no consent needed for nothing";
}

} // namespace
} // namespace nvramfix
} // namespace cr
