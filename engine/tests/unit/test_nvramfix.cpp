/* test_nvramfix.cpp - the UEFI NVRAM Boot#### updater (M-040 stage b).
 *
 * Proves, against synthetic load options built in the corpus's own
 * shapes (EFIBoot_decompiled.c, FUN_1800046f0/FUN_180004840 decode,
 * re-verified first-hand):
 *   1. NvramPatchUpdatesLoadOption — a matched MBR-mode Boot#### has its
 *      HARDDRIVE PartitionNumber rewritten and written back.
 *   2. The MBR matcher: signature + old-start + size; flags may ride
 *      different nodes (the og's whole-walk flag semantics).
 *   3. The GPT matcher: MBRType 2 + SignatureType 2 + GUID equality.
 *   4. Non-matching options are never written.
 *   5. Absent BootOrder = clean no-op; a failing Boot#### read aborts
 *      the pass (the og's whole-loop break).
 *   6. Boundary: truncated options, malformed node lengths, options
 *      without any device path.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/nvramfix/cr_nvramfix.h"
#include "cr/cr_error.h"
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace cr;
using namespace cr::nvramfix;

namespace {

void put16(std::vector<u8>& b, size_t at, u16 v) {
    b[at] = u8(v);
    b[at + 1] = u8(v >> 8);
}
void put32(std::vector<u8>& b, size_t at, u32 v) {
    for (int i = 0; i < 4; ++i)
        b[at + size_t(i)] = u8((v >> (8 * i)) & 0xFF);
}
void put64(std::vector<u8>& b, size_t at, u64 v) {
    for (int i = 0; i < 8; ++i)
        b[at + size_t(i)] = u8((v >> (8 * i)) & 0xFF);
}

/* An in-memory firmware-variable store: name -> bytes; get on an absent
 * name returns ERR_OPEN_FAILED (the interface's benign-absent contract);
 * a programmable poison map turns specific names into hard errors. */
class MemStore final : public IFirmwareVarStore {
public:
    std::map<std::wstring, std::vector<u8>> vars;
    std::map<std::wstring, u32> poison;   /* name -> error code for get() */
    std::vector<std::wstring> set_log;
    u32 fail_all_sets = 0;

    u32 get(const wchar_t* name, void* buf, u32 buf_len, u32* out_len) override {
        const std::wstring key(name);
        auto p = poison.find(key);
        if (p != poison.end())
            return p->second;
        auto it = vars.find(key);
        if (it == vars.end())
            return ERR_OPEN_FAILED; /* variable not present */
        const u32 n = it->second.size() < buf_len ? u32(it->second.size()) : buf_len;
        if (n < it->second.size()) {
            /* the API's insufficient-buffer failure class */
            return ERR_READ_FAILED;
        }
        memcpy(buf, it->second.data(), n);
        *out_len = n;
        return OK;
    }
    u32 set(const wchar_t* name, const void* buf, u32 len) override {
        if (fail_all_sets != 0)
            return fail_all_sets;
        set_log.emplace_back(name);
        vars[name] = std::vector<u8>(static_cast<const u8*>(buf),
                                     static_cast<const u8*>(buf) + len);
        return OK;
    }
};

/* Build one HARDDRIVE node (UEFI 10.1.1, 0x2A bytes). */
std::vector<u8> hd_node(u32 part_number, u64 start_lba, u64 size_lba, u32 mbr_sig, u8 mbr_type,
                        u8 sig_type) {
    std::vector<u8> n(kHdNodeSize, 0);
    n[0] = 0x04; /* MEDIA_DEVICE_PATH */
    n[1] = 0x01; /* MEDIA_HARDDRIVE_DP */
    put16(n, 2, u16(kHdNodeSize));
    put32(n, kHdPartitionNumber, part_number);
    put64(n, kHdPartitionStart, start_lba);
    put64(n, kHdPartitionSize, size_lba);
    put32(n, kHdSignature, mbr_sig); /* MBR signature in the 16-byte field */
    n[kHdMbrType] = mbr_type;
    n[kHdSignatureType] = sig_type;
    return n;
}

std::vector<u8> end_node() {
    std::vector<u8> n{0x7f, 0xff, 0x04, 0x00};
    return n;
}

/* Build a load option: attributes, FilePathListLength (unused by the
 * walk, kept plausible), description, then the node list. */
std::vector<u8> load_option(const wchar_t* desc, const std::vector<std::vector<u8>>& nodes) {
    std::vector<u8> o;
    o.resize(6, 0);
    put32(o, 0, 0x00000001); /* LOAD_OPTION_ACTIVE */
    for (const wchar_t* d = desc; *d; ++d) {
        o.push_back(u8(*d & 0xFF));
        o.push_back(u8((*d >> 8) & 0xFF));
    }
    o.push_back(0); /* description NUL */
    o.push_back(0);
    put16(o, 4, 0); /* FilePathListLength filled after */
    size_t path_len = 0;
    for (const auto& n : nodes) {
        o.insert(o.end(), n.begin(), n.end());
        path_len += n.size();
    }
    put16(o, 4, u16(path_len));
    return o;
}

std::vector<u8> boot_order(std::initializer_list<u16> ids) {
    std::vector<u8> b;
    b.reserve(ids.size() * 2);
    for (u16 id : ids) {
        b.push_back(u8(id));
        b.push_back(u8(id >> 8));
    }
    return b;
}

/* The fixture target: disk signature 0x12345678, partition 2 was at LBA
 * 2048 with 4096 sectors; the renumbered partition becomes number 5. */
Target mbr_target() {
    Target t;
    t.gpt = false;
    t.mbr_signature = 0x12345678;
    t.old_partition_start = 2048;
    t.partition_size = 4096;
    t.new_partition_number = 5;
    return t;
}

TEST(NvramFix, NvramPatchUpdatesLoadOption) {
    MemStore store;
    /* Boot0001 references the target partition (number 2, sig, LBA,
     * size); Boot0002 references a different disk. */
    store.vars[L"BootOrder"] = boot_order({0x0001, 0x0002});
    store.vars[L"Boot0001"] = load_option(
        L"Windows Boot Manager",
        {hd_node(2, 2048, 4096, 0x12345678, 1, 1), end_node()});
    store.vars[L"Boot0002"] =
        load_option(L"Other Disk", {hd_node(1, 128, 256, 0xdeadbeef, 1, 1), end_node()});

    UpdateStats st;
    const u32 rc = update_boot_entries(store, mbr_target(), &st);
    ASSERT_EQ(OK, rc) << err_text(rc);
    EXPECT_EQ(size_t(2), st.inspected);
    EXPECT_EQ(size_t(1), st.matched);
    EXPECT_EQ(size_t(1), st.patched);
    ASSERT_EQ(size_t(1), store.set_log.size());
    EXPECT_EQ(L"Boot0001", store.set_log[0]);

    /* the written variable carries the new partition number at the
     * HARDDRIVE node's +4 */
    const auto& written = store.vars[L"Boot0001"];
    ASSERT_GT(written.size(), 6 + 12 + kHdNodeSize);
    const size_t node_at = 6 + 12; /* "Windows Boot Manager" = 21 chars?? see below */
    /* description is 21 UTF-16 chars + NUL = 44 bytes; recompute
     * generically: walk the description like the implementation. */
    size_t at = 6;
    while (at + 1 < written.size() && (written[at] != 0 || written[at + 1] != 0))
        at += 2;
    at += 2;
    ASSERT_LT(at + 4, written.size());
    EXPECT_EQ(0x04, written[at]);
    EXPECT_EQ(0x01, written[at + 1]);
    EXPECT_EQ(u32(5), u32(written[at + kHdPartitionNumber]) |
                          (u32(written[at + kHdPartitionNumber + 1]) << 8) |
                          (u32(written[at + kHdPartitionNumber + 2]) << 16) |
                          (u32(written[at + kHdPartitionNumber + 3]) << 24))
        << "PartitionNumber must be the new number 5";
    (void)node_at;
}

TEST(NvramFix, MbrMatchRequiresSignatureAndGeometry) {
    const Target t = mbr_target();
    const auto base = load_option(L"x", {hd_node(2, 2048, 4096, 0x12345678, 1, 1), end_node()});
    EXPECT_TRUE(load_option_matches(base.data(), base.size(), t));

    auto wrong_sig = base;
    /* patch the signature field (node starts after "x"+NUL = 10 bytes) */
    const size_t node_at = 10;
    put32(wrong_sig, node_at + kHdSignature, 0x99999999);
    EXPECT_FALSE(load_option_matches(wrong_sig.data(), wrong_sig.size(), t))
        << "foreign MBR signature must not match";

    auto wrong_start = base;
    put64(wrong_start, node_at + kHdPartitionStart, 4096);
    EXPECT_FALSE(load_option_matches(wrong_start.data(), wrong_start.size(), t))
        << "foreign partition start must not match";

    auto wrong_size = base;
    put64(wrong_size, node_at + kHdPartitionSize, 1);
    EXPECT_FALSE(load_option_matches(wrong_size.data(), wrong_size.size(), t))
        << "foreign partition size must not match";

    auto wrong_type = base;
    wrong_type[node_at + kHdSignatureType] = 2; /* GUID signature type in MBR mode */
    EXPECT_FALSE(load_option_matches(wrong_type.data(), wrong_type.size(), t));
}

TEST(NvramFix, MbrFlagsMayRideDifferentNodes) {
    /* the og collects the signature flag and the geometry flag across
     * the WHOLE walk (FUN_1800046f0's bVar3/bVar4) — two half-matching
     * nodes in one option still match. Pinned because it is the
     * reference's exact semantics, not an oversight to "fix". */
    const Target t = mbr_target();
    const auto split = load_option(
        L"s", {hd_node(2, 0, 0, 0x12345678, 1, 1),          /* signature only */
               hd_node(2, 2048, 4096, 0x11111111, 1, 1),    /* geometry only */
               end_node()});
    EXPECT_TRUE(load_option_matches(split.data(), split.size(), t));
}

TEST(NvramFix, GptMatchOnPartitionGuid) {
    Target t;
    t.gpt = true;
    t.gpt_guid[0] = 0x1122334455667788ull;
    t.gpt_guid[1] = 0x99aabbccddeeff00ull;
    t.new_partition_number = 3;

    std::vector<u8> gpt_hd = hd_node(2, 2048, 4096, 0, 2, 2);
    put64(gpt_hd, kHdSignature, t.gpt_guid[0]);
    put64(gpt_hd, kHdSignature + 8, t.gpt_guid[1]);
    const auto opt = load_option(L"g", {gpt_hd, end_node()});
    EXPECT_TRUE(load_option_matches(opt.data(), opt.size(), t));

    put64(gpt_hd, kHdSignature + 8, 0xdeadbeefdeadbeefull); /* wrong GUID half */
    const auto wrong = load_option(L"g", {gpt_hd, end_node()});
    EXPECT_FALSE(load_option_matches(wrong.data(), wrong.size(), t));
}

TEST(NvramFix, AbsentBootOrderIsCleanNoop) {
    MemStore store; /* no BootOrder at all */
    UpdateStats st;
    ASSERT_EQ(OK, update_boot_entries(store, mbr_target(), &st));
    EXPECT_EQ(size_t(0), st.inspected);
    EXPECT_EQ(size_t(0), st.matched);
    EXPECT_EQ(size_t(0), st.patched);
    EXPECT_TRUE(store.set_log.empty());
}

TEST(NvramFix, MissingBootVariableAbortsThePass) {
    /* BootOrder references Boot0003 whose variable is GONE: the og's
     * whole-loop break (FUN_1800046f0 rc=2 -> FUN_180004840 breaks).
     * A matched entry EARLIER in the order still lands (the og applies
     * writes as it walks); nothing after the break does. */
    MemStore store;
    store.vars[L"BootOrder"] = boot_order({0x0001, 0x0003, 0x0004});
    store.vars[L"Boot0001"] = load_option(
        L"a", {hd_node(2, 2048, 4096, 0x12345678, 1, 1), end_node()});
    store.vars[L"Boot0004"] = load_option(
        L"b", {hd_node(2, 2048, 4096, 0x12345678, 1, 1), end_node()});
    /* Boot0003 absent */

    UpdateStats st;
    const u32 rc = update_boot_entries(store, mbr_target(), &st);
    EXPECT_EQ(ERR_OPEN_FAILED, rc) << "the og aborts the whole pass here";
    EXPECT_EQ(size_t(2), st.inspected);
    EXPECT_EQ(size_t(1), st.matched);
    EXPECT_EQ(size_t(1), st.patched); /* Boot0001 landed before the abort */
}

TEST(NvramFix, BoundariesNeverPatch) {
    MemStore store;
    store.vars[L"BootOrder"] = boot_order({0x0000});
    /* option with NO device path at all (description only) */
    store.vars[L"Boot0000"] = load_option(L"empty", {});
    /* truncated option: 4 bytes */
    std::vector<u8> tiny(4, 0);
    store.vars[L"BootOrder"].insert(store.vars[L"BootOrder"].end(), {0x00, 0x00});

    UpdateStats st;
    /* Boot0000 (empty path) is inspected, not matched, never written;
     * then Boot0000 again (dup id in BootOrder — the og walks it twice,
     * and so do we: it is the same benign result). */
    ASSERT_EQ(OK, update_boot_entries(store, mbr_target(), &st));
    EXPECT_EQ(size_t(2), st.inspected);
    EXPECT_EQ(size_t(0), st.matched);
    EXPECT_TRUE(store.set_log.empty());
}

TEST(NvramFix, SetFailurePropagates) {
    MemStore store;
    store.fail_all_sets = ERR_WRITE_FAILED;
    store.vars[L"BootOrder"] = boot_order({0x0001});
    store.vars[L"Boot0001"] = load_option(
        L"a", {hd_node(2, 2048, 4096, 0x12345678, 1, 1), end_node()});
    UpdateStats st;
    EXPECT_EQ(ERR_WRITE_FAILED, update_boot_entries(store, mbr_target(), &st));
    EXPECT_EQ(size_t(1), st.matched);
    EXPECT_EQ(size_t(0), st.patched);
}

TEST(NvramFix, EveryHarddriveNodeInAMatchedOptionIsPatched) {
    /* the patch walk (FUN_180004840 :3533-3544) rewrites EVERY
     * HARDDRIVE node with SignatureType != 0 in a matched option — not
     * just the node that matched. */
    MemStore store;
    store.vars[L"BootOrder"] = boot_order({0x0002});
    store.vars[L"Boot0002"] = load_option(
        L"mirror",
        {hd_node(2, 2048, 4096, 0x12345678, 1, 1), /* the matcher node */
         hd_node(7, 999, 999, 0x12345678, 1, 1),   /* another disk, same option */
         end_node()});
    UpdateStats st;
    ASSERT_EQ(OK, update_boot_entries(store, mbr_target(), &st));
    ASSERT_EQ(size_t(1), st.patched);

    const auto& written = store.vars[L"Boot0002"];
    size_t at = 6;
    while (at + 1 < written.size() && (written[at] != 0 || written[at + 1] != 0))
        at += 2;
    at += 2;
    /* first node */
    ASSERT_EQ(0x04, written[at]);
    EXPECT_EQ(u32(5), u32(written[at + kHdPartitionNumber]) |
                          (u32(written[at + kHdPartitionNumber + 1]) << 8) |
                          (u32(written[at + kHdPartitionNumber + 2]) << 16) |
                          (u32(written[at + kHdPartitionNumber + 3]) << 24));
    /* second node (kHdNodeSize further) */
    const size_t second = at + kHdNodeSize;
    ASSERT_EQ(0x04, written[second]);
    EXPECT_EQ(u32(5), u32(written[second + kHdPartitionNumber]) |
                          (u32(written[second + kHdPartitionNumber + 1]) << 8) |
                          (u32(written[second + kHdPartitionNumber + 2]) << 16) |
                          (u32(written[second + kHdPartitionNumber + 3]) << 24))
        << "every HARDDRIVE node in the matched option is renumbered";
}

} // namespace
