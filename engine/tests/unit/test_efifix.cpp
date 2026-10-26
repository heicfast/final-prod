/* test_efifix.cpp - the UEFI NVRAM Boot#### load-option updater proof
 * set (M-040 stage b). The firmware-variable surface is INJECTED (the
 * finding's own test design: "mocked firmware API - inject the
 * resolver"), so the og's FUN_1800046f0/FUN_180004b40 semantics are
 * pinned on every platform.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_error.h"
#include "cr/efifix/cr_efifix.h"

#include <gtest/gtest.h>

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace cr {
namespace efifix {
namespace {

/* ---- the fake firmware variable store ---- */
struct FakeNvram {
    std::map<std::u16string, std::vector<u8>> vars;
    bool available = true;
    bool fail_next_set = false;
    int fail_gets_after = -1; /* >= 0: gets beyond this index fail */
    int gets = 0;
    int sets = 0;

    static bool avail(void* user) { return static_cast<FakeNvram*>(user)->available; }
    static u32 get(void* user, const char16_t* name, u8* out, u32 cap) {
        FakeNvram* self = static_cast<FakeNvram*>(user);
        const int call = self->gets++;
        if (self->fail_gets_after >= 0 && call > self->fail_gets_after)
            return 0;
        auto it = self->vars.find(std::u16string(name));
        if (it == self->vars.end())
            return 0;
        const u32 n = u32(it->second.size() < size_t(cap) ? it->second.size() : size_t(cap));
        std::memcpy(out, it->second.data(), n);
        return n;
    }
    static bool set(void* user, const char16_t* name, const u8* data, u32 len) {
        FakeNvram* self = static_cast<FakeNvram*>(user);
        ++self->sets;
        if (self->fail_next_set)
            return false;
        self->vars[std::u16string(name)] = std::vector<u8>(data, data + len);
        return true;
    }
    FirmwareVars iface() const {
        FirmwareVars fw;
        fw.available = avail;
        fw.get = get;
        fw.set = set;
        fw.user = const_cast<FakeNvram*>(this);
        return fw;
    }
};

/* ---- load-option builders (UEFI spec 3.1.3 EFI_LOAD_OPTION) ---- */
void put16(std::vector<u8>& v, u16 x) {
    v.push_back(u8(x));
    v.push_back(u8(x >> 8));
}
void put32(std::vector<u8>& v, u32 x) {
    put16(v, u16(x));
    put16(v, u16(x >> 16));
}
void put64(std::vector<u8>& v, u64 x) {
    put32(v, u32(x));
    put32(v, u32(x >> 32));
}

/* One MEDIA_HARDDRIVE_DP node: type 0x04, subtype 0x01, len 0x2A. */
struct HdNode {
    u32 number = 1;
    u64 start = 2048;
    u64 size = 1000;
    u8 sig[16] = {0};
    u8 format = 1;     /* 1 = MBR */
    u8 sigtype = 1;    /* 1 = 32-bit MBR signature */
};

void append_hd_node(std::vector<u8>& v, const HdNode& n) {
    v.push_back(0x04);
    v.push_back(0x01);
    put16(v, 0x2A);
    put32(v, n.number);
    put64(v, n.start);
    put64(v, n.size);
    for (int i = 0; i < 16; ++i)
        v.push_back(n.sig[i]);
    v.push_back(n.format);
    v.push_back(n.sigtype);
}

void append_end_node(std::vector<u8>& v) {
    v.push_back(0x7F);
    v.push_back(0xFF);
    put16(v, 4);
}

/* A load option: attributes, path length (patched after), a
 * NUL-terminated UTF-16 description, then the device path nodes. */
std::vector<u8> make_load_option(const char* desc, const std::vector<u8>& path) {
    std::vector<u8> v;
    put32(v, 0x00000001); /* LOAD_OPTION_ACTIVE */
    put16(v, 0);          /* FilePathListLength - fixed below */
    for (const char* p = desc; *p; ++p)
        put16(v, u16(*p));
    put16(v, 0); /* the description terminator */
    const size_t path_len = v.size();
    v.insert(v.end(), path.begin(), path.end());
    const u16 plen = u16(v.size() - path_len);
    v[4] = u8(plen);
    v[5] = u8(plen >> 8);
    return v;
}

void put_boot_order(FakeNvram& fw, std::initializer_list<u16> ids) {
    std::vector<u8> v;
    for (u16 id : ids)
        put16(v, id);
    fw.vars[u"BootOrder"] = v;
}

PartitionRef mbr_ref(u32 sig, u64 start, u64 size, u32 number) {
    PartitionRef r;
    r.signature[0] = u8(sig);
    r.signature[1] = u8(sig >> 8);
    r.signature[2] = u8(sig >> 16);
    r.signature[3] = u8(sig >> 24);
    r.partition_number = number;
    r.start_lba = start;
    r.size_lbas = size;
    r.gpt = false;
    return r;
}

PartitionRef gpt_ref(const u8 guid[16], u64 start, u64 size, u32 number) {
    PartitionRef r;
    std::memcpy(r.signature, guid, 16);
    r.partition_number = number;
    r.start_lba = start;
    r.size_lbas = size;
    r.gpt = true;
    return r;
}

bool allow_always(void*) { return true; }

/* ---- the finding's named test: the load-option patch ---- */
TEST(EfiFix, NvramPatchUpdatesLoadOption) {
    /* Boot0001's path names the moved MBR partition (sig 0x11223344,
     * start 2048, size 1000); Boot0002 names a different disk. After
     * the move to (start 4096, size 2000, number 3): Boot0001's
     * HARDDRIVE node carries the new position, Boot0002 is untouched,
     * BootOrder is untouched. */
    FakeNvram fw;
    HdNode moved;
    moved.number = 1;
    moved.start = 2048;
    moved.size = 1000;
    moved.sig[0] = 0x44;
    moved.sig[1] = 0x33;
    moved.sig[2] = 0x22;
    moved.sig[3] = 0x11; /* 0x11223344 little-endian */
    std::vector<u8> path1;
    append_hd_node(path1, moved);
    append_end_node(path1);
    fw.vars[u"Boot0001"] = make_load_option("Windows Boot Manager", path1);

    HdNode other;
    other.sig[0] = 0x99;
    other.start = 2048;
    other.size = 1000;
    std::vector<u8> path2;
    append_hd_node(path2, other);
    append_end_node(path2);
    fw.vars[u"Boot0002"] = make_load_option("Other disk", path2);

    put_boot_order(fw, {0x0001, 0x0002});

    const PartitionRef old_ref = mbr_ref(0x11223344, 2048, 1000, 1);
    const PartitionRef new_ref = mbr_ref(0x11223344, 4096, 2000, 3);

    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), old_ref, new_ref, allow_always, nullptr, &st));
    EXPECT_EQ(2u, st.entries_scanned);
    EXPECT_EQ(1u, st.entries_matched);
    EXPECT_EQ(1u, st.entries_patched);

    /* Boot0001 patched: partition number, start, size, signature */
    const std::vector<u8>& b1 = fw.vars[u"Boot0001"];
    ASSERT_GE(b1.size(), size_t(6 + 14 * 2 + 0x2A));
    const size_t node_off = 6 + 20 * 2 + 2; /* "Windows Boot Manager" = 20 chars
                                              * -> 40 bytes + 2 terminator + 6 header */
    const u8* node = b1.data() + node_off;
    EXPECT_EQ(0x04, node[0]);
    EXPECT_EQ(0x01, node[1]);
    EXPECT_EQ(0x2Au, u16(node[2]) | (u16(node[3]) << 8));
    EXPECT_EQ(3u, u32(node[4]) | (u32(node[5]) << 8) | (u32(node[6]) << 16) | (u32(node[7]) << 24));
    EXPECT_EQ(4096u, u64(node[8]) | (u64(node[9]) << 8) | (u64(node[10]) << 16) |
                         (u64(node[11]) << 24) | (u64(node[12]) << 32) | (u64(node[13]) << 40) |
                         (u64(node[14]) << 48) | (u64(node[15]) << 56));
    EXPECT_EQ(2000u, u64(node[16]) | (u64(node[17]) << 8) | (u64(node[18]) << 16) |
                         (u64(node[19]) << 24));
    /* the MBR signature stays (the same disk) */

    /* Boot0002 untouched */
    const std::vector<u8>& b2 = fw.vars[u"Boot0002"];
    const size_t node2_off = 6 + 10 * 2 + 2; /* "Other disk" = 10 chars */
    const u8* node2 = b2.data() + node2_off;
    EXPECT_EQ(2048u, u64(node2[8]) | (u64(node2[9]) << 8));

    /* BootOrder untouched (the updater never rewrites it) */
    const std::vector<u8>& ord = fw.vars[u"BootOrder"];
    ASSERT_EQ(4u, ord.size());
    EXPECT_EQ(0x0001, u16(ord[0]) | (u16(ord[1]) << 8));
    EXPECT_EQ(0x0002, u16(ord[2]) | (u16(ord[3]) << 8));
}

TEST(EfiFix, GptEntryMatchesByGuidOnlyAndPatchesFullGuid) {
    /* the GPT matcher ignores start/size (the og's asymmetry:
     * EFIBoot :3420-3424 matches the GUID halves only); the patch
     * rewrites all 16 GUID bytes + the position fields. */
    FakeNvram fw;
    HdNode gpt;
    gpt.format = 2;
    gpt.sigtype = 2;
    const u8 guid[16] = {0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x07, 0x18,
                         0x29, 0x3A, 0x4B, 0x5C, 0x6D, 0x7E, 0x8F, 0x90};
    std::memcpy(gpt.sig, guid, 16);
    gpt.start = 123456;
    gpt.size = 42;
    std::vector<u8> path;
    append_hd_node(path, gpt);
    append_end_node(path);
    fw.vars[u"Boot00AB"] = make_load_option("gpt entry", path);
    put_boot_order(fw, {0x00AB});

    /* old_ref with DIFFERENT start/size than the node: still matches
     * (GUID-only) */
    PartitionRef old_ref = gpt_ref(guid, 1, 1, 1);
    const u8 new_guid[16] = {0x11, 0x11, 0x11, 0x11, 0x22, 0x22, 0x22, 0x22,
                             0x33, 0x33, 0x33, 0x33, 0x44, 0x44, 0x44, 0x44};
    PartitionRef new_ref = gpt_ref(new_guid, 999, 555, 7);

    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), old_ref, new_ref, allow_always, nullptr, &st));
    EXPECT_EQ(1u, st.entries_matched);
    EXPECT_EQ(1u, st.entries_patched);

    const std::vector<u8>& b = fw.vars[u"Boot00AB"];
    const size_t node_off = 6 + 9 * 2 + 2; /* "gpt entry" = 9 chars */
    const u8* node = b.data() + node_off;
    EXPECT_EQ(0, std::memcmp(node + 0x18, new_guid, 16)) << "the full GUID rewritten";
    EXPECT_EQ(999u, u64(node[8]) | (u64(node[9]) << 8) | (u64(node[10]) << 16));
    EXPECT_EQ(7u, u32(node[4]) | (u32(node[5]) << 8));
}

TEST(EfiFix, MbrMatchRequiresSignatureAndGeometry) {
    /* the MBR matcher needs sig AND start AND size (EFIBoot :3386-3404):
     * right signature, wrong start -> no match, nothing patched. */
    FakeNvram fw;
    HdNode n;
    n.sig[3] = 0x11;
    n.start = 2048;
    n.size = 1000;
    std::vector<u8> path;
    append_hd_node(path, n);
    append_end_node(path);
    fw.vars[u"Boot0001"] = make_load_option("x", path);
    put_boot_order(fw, {0x0001});

    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), mbr_ref(0x11000000, 4096, 1000, 2),
                                      mbr_ref(0x11000000, 8192, 2000, 3), allow_always, nullptr,
                                      &st));
    EXPECT_EQ(1u, st.entries_scanned);
    EXPECT_EQ(0u, st.entries_matched);
    EXPECT_EQ(0u, st.entries_patched);
    EXPECT_EQ(0, fw.sets);
}

TEST(EfiFix, RefusedWithoutConsent) {
    /* the P62 default-deny: no callback -> nothing written,
     * ERR_REPAIR_REFUSED (the bcdfix/rawfixer model). */
    FakeNvram fw;
    HdNode n;
    n.sig[3] = 0x11;
    n.start = 2048;
    n.size = 1000;
    std::vector<u8> path;
    append_hd_node(path, n);
    append_end_node(path);
    fw.vars[u"Boot0001"] = make_load_option("x", path);
    put_boot_order(fw, {0x0001});

    NvramStats st;
    EXPECT_EQ(ERR_REPAIR_REFUSED,
              update_boot_entries(fw.iface(), mbr_ref(0x11000000, 2048, 1000, 1),
                                  mbr_ref(0x11000000, 4096, 2000, 2), nullptr, nullptr, &st));
    EXPECT_EQ(0, fw.sets);
    /* the entry's variable is byte-identical to what was seeded */
    HdNode back;
    back.number = 1;
    back.start = 2048;
    back.size = 1000;
    back.sig[3] = 0x11;
    std::vector<u8> expect_path;
    append_hd_node(expect_path, back);
    append_end_node(expect_path);
    EXPECT_EQ(make_load_option("x", expect_path), fw.vars[u"Boot0001"]);
}

TEST(EfiFix, EveryHardDriveNodeInPathPatched) {
    /* the og patches EVERY 04/01 node in the matched entry's path
     * (FUN_180004b40's do-while), not just the matching one - a
     * nested-path entry with two HARDDRIVE nodes gets both. */
    FakeNvram fw;
    HdNode a;
    a.sig[3] = 0x11;
    a.start = 2048;
    a.size = 1000;
    HdNode b;
    b.sig[3] = 0x22; /* a different disk's node in the same path */
    b.start = 100;
    b.size = 10;
    std::vector<u8> path;
    append_hd_node(path, a);
    append_hd_node(path, b);
    append_end_node(path);
    fw.vars[u"Boot0001"] = make_load_option("x", path);
    put_boot_order(fw, {0x0001});

    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), mbr_ref(0x11000000, 2048, 1000, 1),
                                      mbr_ref(0x11000000, 4096, 2000, 5), allow_always, nullptr,
                                      &st));
    EXPECT_EQ(1u, st.entries_patched);
    const std::vector<u8>& b1 = fw.vars[u"Boot0001"];
    const size_t first = 6 + 1 * 2 + 2;
    const size_t second = first + 0x2A;
    const u8* n1 = b1.data() + first;
    const u8* n2 = b1.data() + second;
    EXPECT_EQ(4096u, u64(n1[8]) | (u64(n1[9]) << 8) | (u64(n1[10]) << 16));
    EXPECT_EQ(4096u, u64(n2[8]) | (u64(n2[9]) << 8) | (u64(n2[10]) << 16))
        << "the second HARDDRIVE node patched too";
    EXPECT_EQ(5u, u32(n2[4]) | (u32(n2[5]) << 8));
}

TEST(EfiFix, MalformedOptionsNeverOverreadAndNeverMatch) {
    /* the registered hardening: an unterminated description, a
     * truncated node, and a zero node length are non-matching (the og
     * overreads its 1 MiB scratch); no crash, no write. */
    FakeNvram fw;
    /* (a) no description terminator within the buffer */
    std::vector<u8> unterm;
    put32(unterm, 1);
    put16(unterm, 0);
    for (int i = 0; i < 10; ++i)
        put16(unterm, u16('A')); /* no NUL, buffer ends */
    fw.vars[u"Boot0001"] = unterm;
    /* (b) node header claims more than the buffer holds */
    std::vector<u8> oversize;
    put32(oversize, 1);
    put16(oversize, 0);
    put16(oversize, 0);
    oversize.push_back(0x04);
    oversize.push_back(0x01);
    put16(oversize, 0x2A); /* the node needs 0x2A bytes; only 4 present */
    fw.vars[u"Boot0002"] = oversize;
    /* (c) zero node length (the og loops forever / overreads) */
    std::vector<u8> zerolen;
    put32(zerolen, 1);
    put16(zerolen, 0);
    put16(zerolen, 0);
    zerolen.push_back(0x04);
    zerolen.push_back(0x01);
    put16(zerolen, 0);
    fw.vars[u"Boot0003"] = zerolen;
    put_boot_order(fw, {0x0001, 0x0002, 0x0003});

    const PartitionRef any_mbr = mbr_ref(0x11000000, 2048, 1000, 1);
    EXPECT_FALSE(boot_entry_matches(any_mbr, unterm.data(), u32(unterm.size())));
    EXPECT_FALSE(boot_entry_matches(any_mbr, oversize.data(), u32(oversize.size())));
    EXPECT_FALSE(boot_entry_matches(any_mbr, zerolen.data(), u32(zerolen.size())));

    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), any_mbr, mbr_ref(0x11000000, 4096, 2000, 2),
                                      allow_always, nullptr, &st));
    EXPECT_EQ(3u, st.entries_scanned);
    EXPECT_EQ(0u, st.entries_matched);
    EXPECT_EQ(0, fw.sets);
}

TEST(EfiFix, UnavailableFirmwareNoopsAndMissingOrderNoops) {
    /* the readiness gate (the og's zero-GUID probe): available=false
     * or an absent BootOrder -> OK, nothing read for patching. */
    FakeNvram fw;
    fw.available = false;
    HdNode n;
    n.sig[3] = 0x11;
    std::vector<u8> path;
    append_hd_node(path, n);
    append_end_node(path);
    fw.vars[u"Boot0001"] = make_load_option("x", path);
    put_boot_order(fw, {0x0001});
    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), mbr_ref(0x11000000, 2048, 1000, 1),
                                      mbr_ref(0x11000000, 4096, 2000, 2), allow_always, nullptr,
                                      &st));
    EXPECT_EQ(0u, st.entries_scanned);
    EXPECT_EQ(0, fw.sets);

    FakeNvram no_order;
    no_order.vars[u"Boot0001"] = make_load_option("x", path);
    ASSERT_EQ(OK, update_boot_entries(no_order.iface(), mbr_ref(0x11000000, 2048, 1000, 1),
                                      mbr_ref(0x11000000, 4096, 2000, 2), allow_always, nullptr,
                                      &st));
    EXPECT_EQ(0u, st.entries_scanned);
}

TEST(EfiFix, FirmwareFailureStopsTheSweep) {
    /* the og's three break sites: a get failure or a set failure stops
     * the walk; entries already patched stay patched. */
    FakeNvram fw;
    HdNode n;
    n.sig[3] = 0x11;
    n.start = 2048;
    n.size = 1000;
    std::vector<u8> path;
    append_hd_node(path, n);
    append_end_node(path);
    fw.vars[u"Boot0001"] = make_load_option("one", path);
    fw.vars[u"Boot0002"] = make_load_option("two", path);
    fw.vars[u"Boot0003"] = make_load_option("three", path);
    put_boot_order(fw, {0x0001, 0x0002, 0x0003});
    fw.fail_next_set = true;

    NvramStats st;
    ASSERT_EQ(OK, update_boot_entries(fw.iface(), mbr_ref(0x11000000, 2048, 1000, 1),
                                      mbr_ref(0x11000000, 4096, 2000, 2), allow_always, nullptr,
                                      &st));
    EXPECT_EQ(1u, st.entries_scanned) << "the set failure stopped the sweep";
    EXPECT_EQ(1u, st.entries_matched);
    EXPECT_EQ(0u, st.entries_patched);
    /* Boot0001 kept its original position (the failed write did not
     * partially apply) */
    const std::vector<u8>& b1 = fw.vars[u"Boot0001"];
    const size_t node_off = 6 + 3 * 2 + 2;
    const u8* node = b1.data() + node_off;
    EXPECT_EQ(2048u, u64(node[8]) | (u64(node[9]) << 8));
}

TEST(EfiFix, IncompleteResolverRejected) {
    FakeNvram fw;
    FirmwareVars broken = fw.iface();
    broken.set = nullptr;
    NvramStats st;
    EXPECT_EQ(ERR_BAD_PARAMETER,
              update_boot_entries(broken, mbr_ref(1, 1, 1, 1), mbr_ref(1, 2, 2, 2), allow_always,
                                  nullptr, &st));
}

} // namespace
} // namespace efifix
} // namespace cr
