/* test_cr_ui.cpp - the cr_ui C-ABI shim suite (GAP G-01 / issue #1).
 *
 * Drives the flat-C surface from C++ (the plain-C compile/link/lifecycle
 * proof is the separate cr_ui_abi_check target): lifecycle parity with
 * the CLI flows, cancel/pause token semantics, save-on-cancel + resume,
 * snapshot enumeration, query filters/sort/paging, preview reads, the
 * recovery walk (subtree-from-root = the CLI plan()), conflict rename,
 * report JSON truth, error paths, and the platform-truth answers of the
 * win-backed surfaces on non-Windows hosts.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include <gtest/gtest.h>
#include "cr/cr_ui.h"
#include "cr/session/cr_lastscan.h"
#include "cr/cr_log.h"
/* S-009 pure-builder tests: the core symbols resolve from libcr_ui
 * (which embeds the core); these includes are for the types only. */
#include "cr/storage/cr_device.h"
#include "cr/tree/cr_diskmap.h"
#include "cr/tree/cr_node.h"

#include "test_tmpdir.hpp"

#include <algorithm> /* N-094 verb-lock pin: std::find over the dispatched set */
#include <atomic>
#include <cctype> /* N-094 verb-lock pin: isalnum over usage tokens */
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <future>
#include <vector>

/* the W1c autosave uniquifier: MSVC deprecates the POSIX name (C4996
 * warning-as-error under /W4 /WX, run 40693014319) - the ISO conformant
 * name on Windows is _getpid */
#ifdef _WIN32
#include <process.h>
#define CR_TEST_PID() _getpid()
#else
#include <unistd.h>
#define CR_TEST_PID() ::getpid()
#endif

/* ---------------------------------------------------------------- */
/* synthetic FAT16 volume (test_orchestrator's fixture + PHOTO.JPG)  */
/* ---------------------------------------------------------------- */

namespace {

constexpr unsigned kImgSecs = 5074;
constexpr size_t kFAT1 = 2 * 512, kFAT2 = 22 * 512, kROOT = 42 * 512, kDATA = 74 * 512;

void put16(std::vector<unsigned char>& v, size_t at, unsigned x) {
    v[at] = (unsigned char)(x & 0xFFu);
    v[at + 1] = (unsigned char)((x >> 8) & 0xFFu);
}
void put32v(std::vector<unsigned char>& v, size_t at, unsigned long x) {
    v[at] = (unsigned char)(x & 0xFFu);
    v[at + 1] = (unsigned char)((x >> 8) & 0xFFu);
    v[at + 2] = (unsigned char)((x >> 16) & 0xFFu);
    v[at + 3] = (unsigned char)((x >> 24) & 0xFFu);
}

void dentry(std::vector<unsigned char>& img, size_t slot, const char* n11, unsigned attr,
            unsigned cl, unsigned long sz, bool del) {
    unsigned char* e = img.data() + kROOT + slot * 32u;
    std::memset(e, 0, 32);
    for (int i = 0; i < 11; ++i)
        e[i] = (unsigned char)n11[i];
    if (del)
        e[0] = 0xE5u;
    e[11] = (unsigned char)attr;
    put16(img, kROOT + slot * 32u + 20, cl >> 16);
    put16(img, kROOT + slot * 32u + 26, cl & 0xFFFFu);
    put32v(img, kROOT + slot * 32u + 28, sz);
}

/* DOC.TXT (1000 B), DELFILE.TXT (deleted, 500 B), PHOTO.JPG (200 B) */
std::vector<unsigned char> build_fat16() {
    std::vector<unsigned char> img(size_t(kImgSecs) * 512, 0);
    std::memcpy(img.data() + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    img[0x0D] = 1;
    put16(img, 0x0E, 2);
    img[0x10] = 2;
    put16(img, 0x11, 512);
    put16(img, 0x13, kImgSecs);
    img[0x15] = 0xF8;
    put16(img, 0x16, 20);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    img[0x26] = 0x29;
    std::memcpy(img.data() + 0x2B, "TESTVOL    ", 11);
    std::memcpy(img.data() + 0x36, "FAT16   ", 8);
    img[510] = 0x55;
    img[511] = 0xAA;

    /* M-015: canonical media words (real-volume shape) */
    put16(img, kFAT1 + 0 * 2, 0xFFF8);
    put16(img, kFAT1 + 1 * 2, 0xFFFF);
    put16(img, kFAT2 + 0 * 2, 0xFFF8);
    put16(img, kFAT2 + 1 * 2, 0xFFFF);
    put16(img, kFAT1 + 5 * 2, 0xFFFF);
    put16(img, kFAT2 + 5 * 2, 0xFFFF); /* root dir */
    put16(img, kFAT1 + 10 * 2, 11);
    put16(img, kFAT2 + 10 * 2, 11);
    put16(img, kFAT1 + 11 * 2, 12);
    put16(img, kFAT2 + 11 * 2, 12);
    put16(img, kFAT1 + 12 * 2, 0xFFFF);
    put16(img, kFAT2 + 12 * 2, 0xFFFF); /* doc */
    put16(img, kFAT1 + 20 * 2, 0xFFFF);
    put16(img, kFAT2 + 20 * 2, 0xFFFF); /* del file */
    put16(img, kFAT1 + 30 * 2, 31);
    put16(img, kFAT2 + 30 * 2, 31);
    put16(img, kFAT1 + 31 * 2, 0xFFFF);
    put16(img, kFAT2 + 31 * 2, 0xFFFF); /* photo */

    dentry(img, 0, "DOC     TXT", 0x20, 10, 1000, false);
    dentry(img, 1, "DELFILE TXT", 0x20, 20, 500, true);
    dentry(img, 2, "PHOTO   JPG", 0x20, 30, 200, false);
    std::memcpy(img.data() + kDATA + (10 - 2) * 512, "DOCDATA", 7);
    std::memcpy(img.data() + kDATA + (20 - 2) * 512, "DELDATA", 7);
    /* 200 bytes of repeating pattern for PHOTO.JPG (cluster 30, 2 clusters) */
    {
        unsigned char* p = img.data() + kDATA + (30 - 2) * 512;
        for (unsigned i = 0; i < 200; ++i)
            p[i] = (unsigned char)('A' + (i % 26));
    }
    return img;
}

/* M-071: the junk-carrying variant — the og's FilterFolder hidden set
 * planted beside real files, so the bulk-walk gates are provable at the
 * ABI surface: DESKTOP.INI (file list) + an LFN "System Volume
 * Information" dir (fat-name list, with KEEP.ME inside — the M-018
 * record's "entries, not contents" case: the child stays a query row
 * while the bulk walks skip the flagged dir's subtree). */
std::vector<unsigned char> build_fat16_junk() {
    auto img = build_fat16();
    /* free clusters for the new entries: 40 (desktop.ini), 50 (SVI dir),
     * 51 (KEEP.ME inside it) */
    put16(img, kFAT1 + 40 * 2, 0xFFFF);
    put16(img, kFAT2 + 40 * 2, 0xFFFF);
    put16(img, kFAT1 + 50 * 2, 0xFFFF);
    put16(img, kFAT2 + 50 * 2, 0xFFFF);
    put16(img, kFAT1 + 51 * 2, 0xFFFF);
    put16(img, kFAT2 + 51 * 2, 0xFFFF);
    /* slot 3: DESKTOP.INI (hidden junk file, 29 bytes) */
    dentry(img, 3, "DESKTOP INI", 0x20, 40, 29, false);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, "[.ShellClassInfo]", 17);
    /* slot 4+: LFN "System Volume Information" -> SYSTEM~1 (dir) */
    {
        const std::u16string name = u"System Volume Information";
        unsigned char alias[11];
        const char* a11 = "SYSTEM~1   ";
        std::memcpy(alias, a11, 11);
        cr::u32 sum = 0;
        for (int i = 0; i < 11; ++i)
            sum = ((sum & 1) << 7) + (sum >> 1) + cr::u32(alias[i]);
        const cr::u8 chk = cr::u8(sum & 0xFF);
        const size_t frags = (name.size() + 12) / 13;
        /* fragments physically DESCENDING (last first), checksum ties to
         * the 8.3 alias that follows (the engine's lfn_assemble contract,
         * test_orchestrator's writer) */
        for (size_t f = frags; f >= 1; --f) {
            cr::u8* e = img.data() + kROOT + (4 + (frags - f)) * 32u;
            std::memset(e, 0xFF, 32);
            e[0] = cr::u8(f == frags ? 0x40 | f : f);
            e[11] = 0x0F; /* LFN */
            e[12] = 0;
            e[13] = chk;
            e[26] = 0;
            e[27] = 0;
            static const size_t off[3] = {1, 14, 28};
            static const int cnt[3] = {5, 6, 2};
            size_t pos = (f - 1) * 13;
            for (int pi = 0; pi < 3; ++pi) {
                for (int ci = 0; ci < cnt[pi]; ++ci) {
                    const cr::u16 ch =
                        pos < name.size() ? name[pos] : (pos == name.size() ? 0x0000 : 0xFFFF);
                    e[off[pi] + 2 * size_t(ci)] = cr::u8(ch & 0xFF);
                    e[off[pi] + 2 * size_t(ci) + 1] = cr::u8(ch >> 8);
                    ++pos;
                }
            }
        }
        /* the alias dentry after the LFN chain: the directory, cluster 50 */
        dentry(img, 4 + frags, "SYSTEM~1   ", 0x10, 50, 0, false);
        /* KEEP.ME inside the SVI dir (cluster 51, 6 bytes) */
        cr::u8* sub = img.data() + kDATA + (50 - 2) * 512;
        std::memset(sub, 0, 512);
        for (int i = 0; i < 11; ++i)
            sub[i] = cr::u8("KEEP    ME "[i]);
        sub[11] = 0x20; /* plain file */
        put16(img, kDATA + (50 - 2) * 512 + 20, 51 >> 16);
        put16(img, kDATA + (50 - 2) * 512 + 26, 51 & 0xFFFFu);
        put32v(img, kDATA + (50 - 2) * 512 + 28, 6);
        std::memcpy(img.data() + kDATA + (51 - 2) * 512, "KEEPME", 6);
    }
    return img;
}

std::string write_fixture(const char* tag) {
    const std::string path = cr_test::tmp_path("cr_ui", tag);
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return {};
    const auto img = build_fat16();
    const size_t n = std::fwrite(img.data(), 1, img.size(), f);
    std::fclose(f);
    if (n != img.size())
        return {};
    return path;
}

/* M-116: the archive fixture variant - STASH.ZIP (a stored-method
 * multi-entry zip: 3 named members, entry 1 CRC-less) on its own
 * cluster, so the two-call archive ABI is provable end-to-end at the
 * same surface the og's results-page archive view serves. */
std::vector<unsigned char> build_stash_zip() {
    std::vector<unsigned char> zip;
    const auto z16 = [&zip](unsigned v) {
        zip.push_back((unsigned char)(v));
        zip.push_back((unsigned char)(v >> 8));
    };
    const auto z32 = [&zip](unsigned long v) {
        zip.push_back((unsigned char)(v));
        zip.push_back((unsigned char)(v >> 8));
        zip.push_back((unsigned char)(v >> 16));
        zip.push_back((unsigned char)(v >> 24));
    };
    const char* names[3] = {"readme.txt", "dir/photo.jpg", "data.bin"};
    const unsigned crc[3] = {0xA1B2C3D4u, 0u, 0x11223344u};
    std::vector<unsigned> lho(3, 0);
    std::vector<std::vector<unsigned char>> cds;
    for (int i = 0; i < 3; ++i) {
        lho[size_t(i)] = unsigned(zip.size());
        const unsigned char sig4[4] = {'P', 'K', 3, 4};
        zip.insert(zip.end(), sig4, sig4 + 4);
        z16(20);
        z16(0);
        z16(0);
        z16(0);
        z16(0);
        z32(crc[i]);
        z32(100);
        z32(100);
        z16(unsigned(std::strlen(names[i])));
        z16(0);
        for (const char* c = names[i]; *c; ++c)
            zip.push_back((unsigned char)(*c));
        zip.insert(zip.end(), size_t(100), (unsigned char)('a' + i));
        std::vector<unsigned char> cd;
        const auto c16 = [&cd](unsigned v) {
            cd.push_back((unsigned char)(v));
            cd.push_back((unsigned char)(v >> 8));
        };
        const auto c32 = [&cd](unsigned long v) {
            cd.push_back((unsigned char)(v));
            cd.push_back((unsigned char)(v >> 8));
            cd.push_back((unsigned char)(v >> 16));
            cd.push_back((unsigned char)(v >> 24));
        };
        const unsigned char cdsig[4] = {'P', 'K', 1, 2};
        cd.insert(cd.end(), cdsig, cdsig + 4);
        c16(20);
        c16(20);
        c16(0);
        c16(0);
        c16(0);
        c16(0);
        c32(crc[i]);
        c32(100);
        c32(100);
        c16(unsigned(std::strlen(names[i])));
        c16(0);
        c16(0);
        c16(0);
        c16(0);
        c32(0);
        c32(lho[size_t(i)]);
        for (const char* c = names[i]; *c; ++c)
            cd.push_back((unsigned char)(*c));
        cds.push_back(std::move(cd));
    }
    const size_t cd_at = zip.size();
    size_t cd_len = 0;
    for (const auto& cd : cds) {
        zip.insert(zip.end(), cd.begin(), cd.end());
        cd_len += cd.size();
    }
    const unsigned char eocd[4] = {'P', 'K', 5, 6};
    zip.insert(zip.end(), eocd, eocd + 4);
    z16(0);
    z16(0);
    z16(3);
    z16(3);
    /* EOCD size/offset fields are u32 zip fields: the fixture values are
     * tiny, the explicit (unsigned long) matches z32's parameter (the
     * dentry size-cast convention below) and silences MSVC C4267 */
    z32((unsigned long)cd_len);
    z32((unsigned long)cd_at);
    z16(0);
    return zip;
}

std::string write_fixture_zip(const char* tag) {
    const std::string path = cr_test::tmp_path("cr_ui", tag);
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return {};
    auto img = build_fat16();
    const auto zip = build_stash_zip();
    const unsigned first_free = 60; /* clusters 60..: the zip body */
    size_t need = (zip.size() + 511) / 512;
    for (unsigned c = 0; c < need; ++c)
        put16(img, kFAT1 + (first_free + c) * 2, first_free + c + 1);
    put16(img, kFAT1 + (first_free + need - 1) * 2, 0xFFFF);
    for (unsigned c = 0; c < need; ++c)
        put16(img, kFAT2 + (first_free + c) * 2, first_free + c + 1);
    put16(img, kFAT2 + (first_free + need - 1) * 2, 0xFFFF);
    dentry(img, 3, "STASH   ZIP", 0x20, first_free, unsigned(zip.size()),
           false); /* slot 3: the first free slot (a zero entry ends the FAT dir walk) */
    std::memcpy(img.data() + kDATA + (first_free - 2) * 512, zip.data(), zip.size());
    const size_t n = std::fwrite(img.data(), 1, img.size(), f);
    std::fclose(f);
    if (n != img.size())
        return {};
    return path;
}

unsigned g_progress_calls = 0;
void progress_cb(void* user, uint32_t, uint32_t, uint32_t, uint64_t, uint64_t, uint32_t) {
    ++*static_cast<unsigned*>(user);
}
void recovery_progress_cb(void* user, uint64_t, uint64_t) {
    ++*static_cast<unsigned*>(user);
}

struct Ui {
    cr_ui_device* dev = nullptr;
    cr_ui_scan* scan = nullptr;
    cr_ui_session* sess = nullptr;
    cr_ui_snapshot* snap = nullptr;
    cr_ui_recovery* rec = nullptr;
    ~Ui() {
        if (rec)
            cr_ui_recovery_close(rec);
        if (snap)
            cr_ui_snapshot_close(snap);
        if (sess)
            cr_ui_session_close(sess);
        if (scan)
            cr_ui_scan_close(scan);
        if (dev)
            cr_ui_device_close(dev);
    }
};

/* M-116: the zip-fixture scan flows (the scan_to_session shape). */
bool scan_zip_to_session(const char* tag, Ui& u, std::string& img_out, std::string& crsf_out) {
    img_out = write_fixture_zip(tag);
    if (img_out.empty())
        return false;
    if (cr_ui_open_image(img_out.c_str(), 0, 0, &u.dev) != CR_UI_OK)
        return false;
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    if (cr_ui_scan_create(&cfg, &u.scan) != CR_UI_OK)
        return false;
    if (cr_ui_scan_attach_device(u.scan, u.dev) != CR_UI_OK)
        return false;
    if (cr_ui_scan_run(u.scan) != CR_UI_OK)
        return false;
    crsf_out = img_out + ".crsf";
    if (cr_ui_scan_save_session(u.scan, crsf_out.c_str()) != CR_UI_OK)
        return false;
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    if (cr_ui_session_load(crsf_out.c_str(), &u.sess) != CR_UI_OK)
        return false;
    return true;
}

/* run a full scan over a fresh fixture and return the session+device in u;
 * the scan handle is closed after saving (the CLI process shape). */
bool scan_to_session(const char* tag, Ui& u, std::string& img_out, std::string& crsf_out) {
    img_out = write_fixture(tag);
    if (img_out.empty())
        return false;
    if (cr_ui_open_image(img_out.c_str(), 0, 0, &u.dev) != CR_UI_OK)
        return false;
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    if (cr_ui_scan_create(&cfg, &u.scan) != CR_UI_OK)
        return false;
    if (cr_ui_scan_attach_device(u.scan, u.dev) != CR_UI_OK)
        return false;
    if (cr_ui_scan_run(u.scan) != CR_UI_OK)
        return false;
    crsf_out = img_out + ".crsf";
    if (cr_ui_scan_save_session(u.scan, crsf_out.c_str()) != CR_UI_OK)
        return false;
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    if (cr_ui_session_load(crsf_out.c_str(), &u.sess) != CR_UI_OK)
        return false;
    return true;
}

/* M-071: the junk-fixture scan (the scan_to_session shape over
 * build_fat16_junk) — the og's FilterFolder hidden set planted beside
 * real files so the bulk recovery walks are provable at the ABI. */
bool scan_junk_to_session(const char* tag, Ui& u, std::string& img_out, std::string& crsf_out) {
    img_out = cr_test::tmp_path("cr_ui", tag);
    FILE* f = std::fopen(img_out.c_str(), "wb");
    if (!f)
        return false;
    const auto img = build_fat16_junk();
    const size_t n = std::fwrite(img.data(), 1, img.size(), f);
    std::fclose(f);
    if (n != img.size())
        return false;
    if (cr_ui_open_image(img_out.c_str(), 0, 0, &u.dev) != CR_UI_OK)
        return false;
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    if (cr_ui_scan_create(&cfg, &u.scan) != CR_UI_OK)
        return false;
    if (cr_ui_scan_attach_device(u.scan, u.dev) != CR_UI_OK)
        return false;
    if (cr_ui_scan_run(u.scan) != CR_UI_OK)
        return false;
    crsf_out = img_out + ".crsf";
    if (cr_ui_scan_save_session(u.scan, crsf_out.c_str()) != CR_UI_OK)
        return false;
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    if (cr_ui_session_load(crsf_out.c_str(), &u.sess) != CR_UI_OK)
        return false;
    return true;
}

/* find a node id by name through the query surface */
uint64_t query_node_id(cr_ui_session* sess, const char* name) {
    cr_ui_query q{};
    q.name_substring = name;
    q.limit = 50;
    cr_ui_query_row* rows = nullptr;
    uint32_t n = 0;
    uint64_t total = 0;
    if (cr_ui_query_run(sess, &q, &rows, &n, &total) != CR_UI_OK)
        return 0;
    uint64_t id = 0;
    for (uint32_t i = 0; i < n; ++i)
        if (std::strcmp(rows[i].name, name) == 0)
            id = rows[i].node_id;
    cr_ui_query_rows_free(rows, n);
    return id;
}

} // namespace

/* ---------------------------------------------------------------- */

TEST(CrUi, AbiVersionAndLastError) {
    EXPECT_EQ(cr_ui_abi_version(), CR_UI_ABI_VERSION);
    EXPECT_NE(cr_ui_last_error(), nullptr);
    /* a failing call leaves a diagnostic behind */
    cr_ui_device* dev = nullptr;
    ASSERT_EQ(cr_ui_open_image("/nonexistent/x.img", 0, 0, &dev), 0x50020001u);
    EXPECT_GT(std::strlen(cr_ui_last_error()), 0u);
}

/* ops #7-#8 parity (API-SURFACE-AUDIT §2.1): the og exposes its analyser
 * registry views (EUGetAnalyserList/Map/Multimap + GetRawUserInfoMap); our
 * carve-format enumeration is the equivalent. Contract (ABI level ONLY —
 * this test links the cr_ui DLL, whose boundary the internal tables do not
 * cross on Windows): deterministic grammar-then-rawsign ordering, field
 * sanity, two-pass stability, BAD_PARAMETER on out-of-range/null. The
 * internal-table consistency is enforced by construction in cr_ui.cpp
 * (cr_ui_carve_format_count is the direct sum of the two compile-time
 * tables it enumerates). */
TEST(CrUi, CarveFormatRegistryEnumeration) {
    const uint32_t n = cr_ui_carve_format_count();
    ASSERT_GT(n, 0u);

    /* two identical passes: deterministic order + stability */
    std::vector<cr_ui_carve_format> a(n), b(n);
    for (uint32_t i = 0; i < n; ++i)
        ASSERT_EQ(cr_ui_carve_format_info(i, &a[i]), CR_UI_OK);
    for (uint32_t i = 0; i < n; ++i)
        ASSERT_EQ(cr_ui_carve_format_info(i, &b[i]), CR_UI_OK);

    uint32_t grammar_seen = 0, raw_seen = 0;
    bool raw_started = false;
    for (uint32_t i = 0; i < n; ++i) {
        EXPECT_EQ(std::memcmp(&a[i], &b[i], sizeof(cr_ui_carve_format)), 0) << "row " << i;
        ASSERT_GT(std::strlen(a[i].ext), 0u) << "row " << i;
        EXPECT_EQ(std::strchr(a[i].ext, '.'), nullptr) << "ext carries no dot: " << a[i].ext;
        for (const char* p = a[i].ext; *p; ++p)
            EXPECT_TRUE((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-')
                << "ext is lowercase/digit/dash: " << a[i].ext;
        EXPECT_EQ(a[i].ext[sizeof(a[i].ext) - 1], '\0') << "NUL-terminated in-field";
        EXPECT_LE(a[i].base_confidence, 1000u);
        EXPECT_LE(a[i].magic_len, 16u);
        EXPECT_LE(a[i].magic_offset, 65536u);
        if (a[i].layer == 0) {
            ASSERT_FALSE(raw_started) << "grammar rows are contiguous and first";
            ++grammar_seen;
        } else {
            ASSERT_EQ(a[i].layer, 1u);
            raw_started = true;
            ++raw_seen;
            EXPECT_GT(a[i].max_size, 0u) << "raw-sign rows carry converted byte sizes (S-015)";
        }
    }
    EXPECT_GT(grammar_seen, 0u);
    EXPECT_GT(raw_seen, 0u);

    /* error contract */
    cr_ui_carve_format sink{};
    EXPECT_EQ(cr_ui_carve_format_info(n, &sink), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_carve_format_info(n + 1000000u, &sink), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_carve_format_info(0, nullptr), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_GT(std::strlen(cr_ui_last_error()), 0u);
}

/* N-140: the per-format carve tally at the ABI - the og's GetRawUserInfoMap
 * surface (per-extension counts served pre-computed; DRWUI renders the
 * post-scan "found N .jpg" breakdown). A scan with a grammar-layer JPEG
 * planted in a FREE cluster (carve-visible; live files are free-map
 * suppressed, M-005) must answer the breakdown through the tally pair
 * without a tree walk. RED pre-fix: the exports did not exist. */
TEST(CrUi, ScanCarveTallyPerFormat) {
    /* a JPEG plant in free cluster 100 (unlinked in the FAT - carve
     * visible on both layers) */
    auto img = build_fat16();
    const size_t DATA = 74 * 512;
    {
        unsigned char* at = img.data() + DATA + (100 - 2) * 512;
        at[0] = 0xFF;
        at[1] = 0xD8;
        at[2] = 0xFF;
        at[3] = 0xE0;
        at[4] = 0;
        at[5] = 16;
        std::memcpy(at + 6, "JFIF", 4);
        at[10] = 1;
        at[11] = 1;
        at[14] = 1;
        at[15] = 0;
        size_t p = 20;
        at[p] = 0xFF;
        at[p + 1] = 0xFE;
        at[p + 2] = 0x01;
        at[p + 3] = 0xD2;
        for (int i = 0; i < 462; ++i)
            at[p + 4 + i] = (unsigned char)('A' + (i % 26));
        p += 2 + 466;
        at[p] = 0xFF;
        at[p + 1] = 0xC0;
        at[p + 2] = 0;
        at[p + 3] = 17;
        at[p + 4] = 8;
        at[p + 5] = 0;
        at[p + 6] = 64;
        at[p + 7] = 0;
        at[p + 8] = 48;
        at[p + 9] = 1;
        at[p + 10] = 1;
        at[p + 11] = 0x22;
        p += 2 + 17;
        at[p] = 0xFF;
        at[p + 1] = 0xDA;
        at[p + 2] = 0;
        at[p + 3] = 8;
        for (int i = 0; i < 8; ++i)
            at[p + 4 + i] = 0x11;
        p += 2 + 12;
        at[p] = 0x00;
        at[p + 1] = 0x11;
        at[p + 2] = 0x22;
        at[p + 3] = 0x33;
        at[p + 4] = 0xFF;
        at[p + 5] = 0xD9;
    }
    const std::string path = cr_test::tmp_path("cr_ui", "tally");
    FILE* f = std::fopen(path.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(std::fwrite(img.data(), 1, img.size(), f), img.size());
    std::fclose(f);

    Ui u;
    ASSERT_EQ(cr_ui_open_image(path.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);

    /* the counting space: the same rows cr_ui_carve_format_count()
     * enumerates, plus the aggregate user-sign slot */
    const uint32_t want_slots = cr_ui_carve_format_count() + 1;
    const uint32_t slots = cr_ui_scan_carve_tally_count(u.scan);
    EXPECT_EQ(slots, want_slots) << "grammar rows + raw-sign rows + the user-sign aggregate";

    /* the JPG row answers with a nonzero count (the free-cluster plant) */
    uint32_t jpg_id = UINT32_MAX;
    for (uint32_t i = 0; i < cr_ui_carve_format_count(); ++i) {
        cr_ui_carve_format fi{};
        ASSERT_EQ(cr_ui_carve_format_info(i, &fi), CR_UI_OK);
        if (std::strcmp(fi.ext, "jpg") == 0 && fi.layer == 0) {
            jpg_id = i;
            break;
        }
    }
    ASSERT_NE(jpg_id, UINT32_MAX) << "the grammar registry must enumerate jpg";
    uint64_t jpg_count = 0;
    ASSERT_EQ(cr_ui_scan_carve_tally(u.scan, jpg_id, &jpg_count), CR_UI_OK);
    EXPECT_GE(jpg_count, 1u) << "the carved JPEG must tick its row";

    /* the contract: out-of-range and null are BAD_PARAMETER, never a
     * fabricated count */
    uint64_t sink = 0;
    EXPECT_EQ(cr_ui_scan_carve_tally(u.scan, slots, &sink), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_scan_carve_tally(u.scan, 0, nullptr), CR_UI_ERR_BAD_PARAMETER);
    /* every in-range slot answers OK (zeros included - an empty map row,
     * the og's GetRawUserInfoMap shape) */
    for (uint32_t i = 0; i < slots; ++i)
        ASSERT_EQ(cr_ui_scan_carve_tally(u.scan, i, &sink), CR_UI_OK) << "slot " << i;
}

/* N-144: the TxtViewFiles text-snapshot thumbnail at the ABI - the og
 * dispatches by extension membership (DRWUI :422447-455): a .txt node
 * renders the content-preview image (the first <=100 bytes), never the
 * image decode path. The DOC.TXT node (the fixture's text file) with
 * printable content must answer CR_UI_OK with source_format "txt".
 * RED pre-fix: cr_ui_thumbnail_format_supported("txt") answered 0 and
 * generate returned the named Unsupported. */
TEST(CrUi, TextSnapshotThumbnailForTxtNode) {
    EXPECT_EQ(1u, cr_ui_thumbnail_format_supported("txt"));

    /* the fixture's DOC.TXT (cluster 10) carries printable text */
    auto img = build_fat16();
    const size_t DATA = 74 * 512;
    const char text[] = "recovered notes: the quick brown fox";
    std::memcpy(img.data() + DATA + (10 - 2) * 512, text, sizeof(text) - 1);
    const std::string path = cr_test::tmp_path("cr_ui", "txtsnap");
    FILE* f = std::fopen(path.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(std::fwrite(img.data(), 1, img.size(), f), img.size());
    std::fclose(f);

    Ui u;
    ASSERT_EQ(cr_ui_open_image(path.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    const std::string crsf = path + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);

    const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(doc, 0u) << "the .txt node must be in the session tree";

    /* the size probe: the text snapshot answers the full canvas */
    cr_ui_thumbnail_info info{};
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, doc, nullptr, nullptr, 0, &info), CR_UI_OK);
    EXPECT_EQ(256u, info.width);
    EXPECT_EQ(256u, info.height);
    EXPECT_EQ(256u * 256u * 4u, info.bytes_needed);
    EXPECT_STREQ("txt", info.source_format);

    /* the fill: white canvas with black glyph ink (non-empty by
     * construction - the printable head is present) */
    std::vector<uint8_t> px(info.bytes_needed, 0);
    cr_ui_thumbnail_info info2{};
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, doc, nullptr, px.data(), uint32_t(px.size()),
                                       &info2),
              CR_UI_OK);
    bool any_dark = false, any_light = false;
    for (size_t i = 0; i + 3 < px.size(); i += 4) {
        if (px[i] == 0 && px[i + 1] == 0 && px[i + 2] == 0)
            any_dark = true;
        if (px[i] == 255 && px[i + 1] == 255 && px[i + 2] == 255)
            any_light = true;
    }
    EXPECT_TRUE(any_dark) << "the text preview carries glyph ink";
    EXPECT_TRUE(any_light) << "the text preview carries the white canvas";
}

/* M-073: the scan-time folder filter at the ABI surface - the og's
 * PushFolderFilter config leg (cr_ui_scan_config.filter_folders_utf8).
 * The seeded volume carries an in-scope folder (IN) and an out-of-scope
 * folder (OUT) plus root files; with the filter set, only IN's subtree
 * reaches the saved session (query rows prove it end-to-end). */
TEST(CrUi, ScanTimeFolderFilterPrunesAtTheAbi) {
    /* fixture: FOLDER1 (cluster 40) holds IN.TXT (41); FOLDER2 (cluster
     * 42) holds OUT.TXT (43); DOC.TXT/PHOTO.JPG/DELFILE.TXT stay at root */
    auto img = build_fat16();
    put16(img, kFAT1 + 40 * 2, 0xFFFF);
    put16(img, kFAT2 + 40 * 2, 0xFFFF);
    put16(img, kFAT1 + 41 * 2, 0xFFFF);
    put16(img, kFAT2 + 41 * 2, 0xFFFF);
    put16(img, kFAT1 + 42 * 2, 0xFFFF);
    put16(img, kFAT2 + 42 * 2, 0xFFFF);
    put16(img, kFAT1 + 43 * 2, 0xFFFF);
    put16(img, kFAT2 + 43 * 2, 0xFFFF);
    dentry(img, 3, "FOLDER1    ", 0x10, 40, 0, false);
    dentry(img, 4, "FOLDER2    ", 0x10, 42, 0, false);
    for (int k = 0; k < 11; ++k) {
        img[kDATA + (40 - 2) * 512 + k] = (unsigned char)".           "[k];
        img[kDATA + (40 - 2) * 512 + 32 + k] = (unsigned char)"..          "[k];
        img[kDATA + (40 - 2) * 512 + 64 + k] = (unsigned char)"IN      TXT"[k];
        img[kDATA + (42 - 2) * 512 + k] = (unsigned char)".           "[k];
        img[kDATA + (42 - 2) * 512 + 32 + k] = (unsigned char)"..          "[k];
        img[kDATA + (42 - 2) * 512 + 64 + k] = (unsigned char)"OUT     TXT"[k];
    }
    img[kDATA + (40 - 2) * 512 + 32 + 11] = 0x10; /* '.' dir */
    img[kDATA + (40 - 2) * 512 + 64 + 11] = 0x20; /* IN.TXT file */
    put16(img, kDATA + (40 - 2) * 512 + 64 + 26, 41);
    put32v(img, kDATA + (40 - 2) * 512 + 64 + 28, 16);
    img[kDATA + (42 - 2) * 512 + 32 + 11] = 0x10; /* '.' dir */
    img[kDATA + (42 - 2) * 512 + 64 + 11] = 0x20; /* OUT.TXT file */
    put16(img, kDATA + (42 - 2) * 512 + 64 + 26, 43);
    put32v(img, kDATA + (42 - 2) * 512 + 64 + 28, 16);
    std::memcpy(img.data() + kDATA + (41 - 2) * 512, "INDATA", 6);
    std::memcpy(img.data() + kDATA + (43 - 2) * 512, "OUTDATA", 7);

    const std::string path = cr_test::tmp_path("cr_ui", "m073");
    FILE* f = std::fopen(path.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(std::fwrite(img.data(), 1, img.size(), f), img.size());
    std::fclose(f);

    Ui u;
    ASSERT_EQ(cr_ui_open_image(path.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    const char* folders[1] = {"FOLDER1"};
    cfg.filter_folders_utf8 = folders;
    cfg.filter_folder_count = 1;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    const std::string crsf = path + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);

    /* the in-scope file is a query row; every out-of-scope row is gone
     * (OUT.TXT, DOC.TXT, PHOTO.JPG - and DELFILE.TXT's deleted row) */
    EXPECT_NE(query_node_id(u.sess, "IN.TXT"), 0u);
    EXPECT_EQ(query_node_id(u.sess, "OUT.TXT"), 0u);
    EXPECT_EQ(query_node_id(u.sess, "DOC.TXT"), 0u);
    EXPECT_EQ(query_node_id(u.sess, "PHOTO.JPG"), 0u);
    EXPECT_EQ(query_node_id(u.sess, "#ELFILE.TXT"), 0u);
}

/* M-073 zero-init safety: a host compiled before the append passes a
 * config without the tail fields - the scan behaves exactly as before. */
TEST(CrUi, ScanConfigZeroInitFilterOff) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("m073off", u, img, crsf));
    /* every base-fixture row survives when no filter is configured */
    EXPECT_NE(query_node_id(u.sess, "DOC.TXT"), 0u);
    EXPECT_NE(query_node_id(u.sess, "PHOTO.JPG"), 0u);
}

TEST(CrUi, ScanLifecycleOverImage) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("life", u, img, crsf));

    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_session_stats(u.sess, &st), CR_UI_OK);
    EXPECT_GE(st.files_found, 2u); /* DOC + PHOTO (DELFILE is deleted-found) */
    EXPECT_GE(st.deleted_found, 1u);

    cr_ui_session_info info{};
    ASSERT_EQ(cr_ui_session_get_info(u.sess, &info), CR_UI_OK);
    EXPECT_EQ(info.fs_family, 2u); /* FsFamily::Fat */
    EXPECT_EQ(info.stage_done_mask & 1u, 1u);

    cr_ui_geometry geom{};
    ASSERT_EQ(cr_ui_device_geometry(u.dev, &geom), CR_UI_OK);
    EXPECT_EQ(geom.total_bytes, size_t(kImgSecs) * 512);
    char transport[16] = {0};
    uint32_t needed = 0;
    ASSERT_EQ(cr_ui_device_transport(u.dev, transport, sizeof(transport), &needed), CR_UI_OK);
    EXPECT_STREQ(transport, "image");
    EXPECT_GT(g_progress_calls, 0u);

    /* double-open of the same image is independent (no shared state) */
    cr_ui_device* second = nullptr;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &second), CR_UI_OK);
    cr_ui_device_close(second);
}

TEST(CrUi, CancelSaveAndResume) {
    const std::string img = write_fixture("cancel");
    ASSERT_FALSE(img.empty());
    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);

    /* cancel BEFORE run: engines abort at their first loop-head poll -
     * deterministic, and exactly the UI's stop-button-before-start case */
    cr_ui_scan_config cfg{};
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
    EXPECT_EQ(cr_ui_scan_run(u.scan), 0x50020005u); /* ERR_CANCELLED */

    /* save-on-cancel: the session persists the partial truth */
    const std::string crsf = img + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;

    /* resume: a fresh controller continues from the stored mask */
    ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);
    cr_ui_scan_config rcfg{};
    ASSERT_EQ(cr_ui_session_resume(u.sess, &rcfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    /* identity verify: same image -> the boot hash matches */
    EXPECT_EQ(cr_ui_scan_verify_identity(u.scan), CR_UI_OK);
    /* verify on a NON-resumed scan is a bad request */
    /* (covered in ErrorPaths) */
    EXPECT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_GE(st.files_found, 2u);
    /* the double-run refusal (one-shot lifecycle) */
    EXPECT_NE(cr_ui_scan_run(u.scan), CR_UI_OK);
}

namespace {
/* W1c (R-52): the completion auto-save event observer. */
unsigned g_autosave_events = 0;
uint64_t g_autosave_rc = 1;
void autosave_event_cb(void*, const cr_ui_event* evt) {
    if (evt->type == CR_EVT_AUTOSAVED) {
        ++g_autosave_events;
        g_autosave_rc = evt->autosave_rc;
    }
}
} // namespace

namespace {
/* S-057: cancel from the files-found event - deterministic stop AFTER
 * results exist (the engine aborts at its next loop-head poll). The
 * callback ALSO observes the autosave event (one event slot per scan -
 * the second set_event_fn would replace the first). */
cr_ui_scan* g_cancel_target = nullptr;
void cancel_and_observe(void*, const cr_ui_event* evt) {
    if (evt->type == CR_EVT_AUTOSAVED) {
        ++g_autosave_events;
        g_autosave_rc = evt->autosave_rc;
        return;
    }
    if (evt->type == CR_EVT_FILES_FOUND && evt->files_found >= 1 && g_cancel_target)
        cr_ui_scan_cancel(g_cancel_target);
}
} // namespace

TEST(CrUi, CancelledRunWithResultsAutoSaves) {
    /* S-057: the og's stop window auto-saves - the stop path runs its
     * own organize ("Scan stopped and found %1 files.", DRWUI
     * :1549399-421) into the SAME completion handler whose auto-save
     * gates (:612571-587) carry no stopped-check. A user who stops a
     * scan after files are found keeps them: an auto_*.crsf lands in
     * autosave_dir, stats.autosaved flips, and the session LOADS with
     * the partial tree. Pre-fix: the cancel path returned before the
     * auto-save block ever ran (no file, autosaved == 0). */
    const std::string img = write_fixture("stopsave");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/s057_autosave_" + std::to_string(CR_TEST_PID());
    std::filesystem::create_directories(adir);
    g_autosave_events = 0;
    g_autosave_rc = 1;

    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.autosave_dir_utf8 = adir.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, cancel_and_observe, nullptr), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    g_cancel_target = u.scan;
    /* the subscription cancels once files exist; the engine stops at
     * its next poll and run() returns ERR_CANCELLED */
    /* (single event slot: the combined callback both cancels and
     * observes the autosave outcome) */
    EXPECT_EQ(cr_ui_scan_run(u.scan), 0x50020005u); /* ERR_CANCELLED */
    g_cancel_target = nullptr;
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_EQ(st.autosaved, 1u) << "the stop window auto-saves (S-057)";
    EXPECT_EQ(g_autosave_events, 1u);
    EXPECT_EQ(g_autosave_rc, 0u);
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_EQ(n, 1u);
    /* the partial session loads and carries the found-so-far tree */
    std::vector<cr_ui_session_row> rows(1);
    uint32_t cap = 1;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), rows.data(), &cap), CR_UI_OK);
    EXPECT_EQ(rows[0].valid, 1u);
    cr_ui_session* sess = nullptr;
    ASSERT_EQ(cr_ui_session_load(rows[0].path, &sess), CR_UI_OK);
    cr_ui_scan* resumed = nullptr;
    cr_ui_scan_config rcfg{};
    ASSERT_EQ(cr_ui_session_resume(sess, &rcfg, &resumed), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(resumed, u.dev), CR_UI_OK);
    EXPECT_EQ(cr_ui_scan_run(resumed), CR_UI_OK); /* resumes and completes */
    cr_ui_scan_stats rst{};
    ASSERT_EQ(cr_ui_scan_get_stats(resumed, &rst), CR_UI_OK);
    EXPECT_GE(rst.files_found, 1u) << "the stopped scan's partial tree persisted and resumes";
    cr_ui_scan_close(resumed);
    cr_ui_session_close(sess);
}

TEST(CrUi, CancelledRunZeroFoundWritesNothing) {
    /* S-057 gate pin: the og never writes a nothing-found session - the
     * results-found gate holds in the stop window exactly as in the
     * completion window. Cancel BEFORE any results (the
     * stop-button-before-start shape): no file, no event, autosaved 0. */
    const std::string img = write_fixture("stopempty");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/s057_empty_" + std::to_string(CR_TEST_PID());
    std::filesystem::create_directories(adir);
    g_autosave_events = 0;

    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.autosave_dir_utf8 = adir.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, autosave_event_cb, nullptr), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
    EXPECT_EQ(cr_ui_scan_run(u.scan), 0x50020005u);
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_EQ(st.autosaved, 0u) << "zero-found stop writes nothing (the og gate holds)";
    EXPECT_EQ(g_autosave_events, 0u);
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
    EXPECT_EQ(n, 0u);
}

TEST(CrUi, AutoSaveOnCompletion) {
    /* W1c (R-52): the og's completion SaveScan flow - zero-init = OFF;
     * on a COMPLETED run the auto session lands in autosave_dir under
     * the N-061 auto_<label>_<ts> naming; the event fires ONCE with the
     * rc and stats.autosaved flips; since S-057 a user-STOPPED run with
     * results auto-saves too (the stop-window leg is pinned separately
     * in CancelledRunWithResultsAutoSaves); the W1a list reads the auto
     * session back (the surfaces compose). */
    const std::string img = write_fixture("autosave");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/w1c_autosave_" + std::to_string(CR_TEST_PID());
    std::filesystem::create_directories(adir);
    g_autosave_events = 0;
    g_autosave_rc = 1;

    /* default zero-init: OFF - no event, no file, stats.autosaved == 0 */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, autosave_event_cb, nullptr), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        cr_ui_scan_stats st{};
        ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
        EXPECT_EQ(st.autosaved, 0u);
        EXPECT_EQ(g_autosave_events, 0u);
        uint32_t n = 0;
        ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
        EXPECT_EQ(n, 0u);
    }

    /* completion: exactly ONE auto session, valid, N-061 naming, event
     * once with rc 0, stats flag set */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, autosave_event_cb, nullptr), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        cr_ui_scan_stats st{};
        ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
        EXPECT_EQ(st.autosaved, 1u);
        EXPECT_EQ(g_autosave_events, 1u);
        EXPECT_EQ(g_autosave_rc, 0u);
        uint32_t n = 0;
        ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
        ASSERT_EQ(n, 1u); /* exactly one - never a per-stage double */
        std::vector<cr_ui_session_row> rows(1);
        uint32_t cap = 1;
        ASSERT_EQ(cr_ui_session_list(adir.c_str(), rows.data(), &cap), CR_UI_OK);
        EXPECT_EQ(rows[0].valid, 1u);
        EXPECT_EQ(rows[0].error, CR_UI_OK);
        EXPECT_GT(rows[0].save_unix, 0ull);
        const char* base = std::strrchr(rows[0].path, '/');
        const char* base2 = std::strrchr(rows[0].path, '\\');
        if (base2 && (!base || base2 > base))
            base = base2;
        ASSERT_NE(base, nullptr);
        EXPECT_EQ(std::strncmp(base + 1, "auto_", 5), 0); /* the N-061 naming */
        EXPECT_NE(std::strstr(base + 1, ".crsf"), nullptr);
    }

    /* cancelled run: never auto-saves (the og's flow is the completion
     * window - save-on-cancel is the explicit cr_ui_scan_save_session
     * surface) */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
        EXPECT_EQ(cr_ui_scan_run(u.scan), 0x50020005u); /* ERR_CANCELLED */
        cr_ui_scan_stats st{};
        ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
        EXPECT_EQ(st.autosaved, 0u);
        uint32_t n = 99;
        ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
        EXPECT_EQ(n, 1u); /* still exactly the ONE from the completion */
    }

    std::filesystem::remove_all(adir);
}

TEST(CrUi, AutoSaveSkipsEmptyResults) {
    /* S-044: the og's results-found gate (FUN_1403964b0 != 0 at every
     * trigger site) - a scan that found NOTHING never auto-saves: no
     * session file, no CR_EVT_AUTOSAVED, stats.autosaved stays 0. The
     * W1a session list never sees nothing-found runs. */
    const std::string dir = cr_test::tmp_path("cr_ui", "s044_empty");
    std::filesystem::create_directories(dir);
    const std::string blank = dir + "/blank.img";
    {
        /* an all-zero image: no FS, no partition table, no carve keys -
         * a completed scan with zero results */
        std::vector<unsigned char> zeros(2 * 1024 * 1024, 0);
        FILE* f = std::fopen(blank.c_str(), "wb");
        ASSERT_NE(f, nullptr);
        ASSERT_EQ(std::fwrite(zeros.data(), 1, zeros.size(), f), zeros.size());
        std::fclose(f);
    }
    const std::string adir = dir + "/autosave";
    std::filesystem::create_directories(adir);
    g_autosave_events = 0;
    g_autosave_rc = 1;
    Ui u;
    ASSERT_EQ(cr_ui_open_image(blank.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.autosave_dir_utf8 = adir.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, autosave_event_cb, nullptr), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_EQ(st.files_found, 0u) << "the blank image must find nothing";
    EXPECT_EQ(st.lost_partitions, 0u);
    EXPECT_EQ(st.autosaved, 0u) << "an empty scan never auto-saves";
    EXPECT_EQ(g_autosave_events, 0u) << "no event for a nothing-found run";
    uint32_t n = 99;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
    EXPECT_EQ(n, 0u) << "no auto session file for a nothing-found run";
    /* close the scan and the device BEFORE the teardown remove_all: a
     * live handle to blank.img blocks the delete on Windows (POSIX
     * unlinks fine - the release-27 catch: the remove_all threw and the
     * whole test reported as failed). The handles are nulled after the
     * explicit close so Ui's own destructor does not double-close them. */
    cr_ui_scan_close(u.scan);
    cr_ui_device_close(u.dev);
    u.scan = nullptr;
    u.dev = nullptr;
    std::filesystem::remove_all(dir);
}

TEST(CrUi, AutoSaveSkipsAfterExplicitSave) {
    /* S-044: the og's not-already-saved gate (+0x388 == 0 at 2 of 3
     * trigger sites) - a session saved explicitly mid-scan (the pause
     * window is the og's own mid-scan save posture: the engine is
     * parked at a checkpoint, the tree is consistent) suppresses the
     * completion auto-save: exactly ONE file total (the explicit one),
     * no CR_EVT_AUTOSAVED, stats.autosaved == 0. */
    const std::string img = write_fixture("s044_saved");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/s044_autosave_" + std::to_string(CR_TEST_PID());
    std::filesystem::create_directories(adir);
    const std::string explicit_path =
        dir + "/s044_explicit_" + std::to_string(CR_TEST_PID()) + ".crsf";
    g_autosave_events = 0;
    g_autosave_rc = 1;
    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.autosave_dir_utf8 = adir.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, autosave_event_cb, nullptr), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);

    /* park the scan mid-run, save explicitly from the main thread (the
     * engine is blocked at its checkpoint - no concurrent tree access),
     * then let it complete */
    ASSERT_EQ(cr_ui_scan_pause(u.scan), CR_UI_OK);
    std::atomic<bool> done{false};
    std::thread runner([&] {
        EXPECT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        done.store(true);
    });
    bool saw_paused = false;
    for (int i = 0; i < 200 && !done.load(); ++i) {
        cr_ui_telemetry tel{};
        if (cr_ui_scan_telemetry(u.scan, &tel) == CR_UI_OK && tel.paused == 1u)
            saw_paused = true;
        if (saw_paused)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(saw_paused) << "the scan must reach the parked state";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, explicit_path.c_str()), CR_UI_OK)
        << "the mid-scan explicit save (the pause window)";
    ASSERT_EQ(cr_ui_scan_resume(u.scan), CR_UI_OK);
    runner.join();
    ASSERT_TRUE(done.load());

    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_NE(st.files_found, 0u) << "the fixture has files (the gate is the save, not emptiness)";
    EXPECT_EQ(st.autosaved, 0u) << "an already-explicitly-saved run never auto-saves";
    EXPECT_EQ(g_autosave_events, 0u) << "no event after an explicit save";
    uint32_t n = 99;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
    EXPECT_EQ(n, 0u) << "no auto file - exactly the ONE explicit session total";
    /* the explicit file itself is a valid session */
    cr_ui_session* sess = nullptr;
    ASSERT_EQ(cr_ui_session_load(explicit_path.c_str(), &sess), CR_UI_OK);
    cr_ui_session_close(sess);
    std::filesystem::remove_all(adir);
    std::filesystem::remove(explicit_path);
}

TEST(CrUi, AutoSaveSkipsResumedRun) {
    /* S-044: the og's not-import gate (+0x1c8 == 0 at every trigger
     * site) - a RESUMED run never re-auto-saves: the session already
     * exists (it was imported); completion of the continuation writes
     * no second auto file. */
    const std::string img = write_fixture("s044_resume");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/s044_resume_as_" + std::to_string(CR_TEST_PID());
    std::filesystem::create_directories(adir);
    const std::string crsf = img + ".crsf";
    g_autosave_events = 0;
    g_autosave_rc = 1;

    /* first run: explicitly saved (no autosave configured) */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
        EXPECT_EQ(cr_ui_scan_run(u.scan), 0x50020005u); /* ERR_CANCELLED */
        ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    }
    /* resume with autosave configured: completion writes NO auto file */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);
        cr_ui_scan_config rcfg{};
        rcfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_session_resume(u.sess, &rcfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, autosave_event_cb, nullptr), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        cr_ui_scan_stats st{};
        ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
        EXPECT_NE(st.files_found, 0u);
        EXPECT_EQ(st.autosaved, 0u) << "a resumed run never re-auto-saves";
        EXPECT_EQ(g_autosave_events, 0u);
        uint32_t n = 99;
        ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
        EXPECT_EQ(n, 0u) << "no auto file for an imported/resumed session";
    }
    std::filesystem::remove_all(adir);
    std::filesystem::remove(crsf);
}

TEST(CrUi, SessionListTwoCallPatternAndRowTruth) {
    /* W1a (R-56): the LastScanRecordView surface - the capacity probe,
     * the under-capacity TRUNCATED answer, and per-row truth
     * cross-checked against the loaded session's own info/stats (the
     * list must agree with the load on every shared field). */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("sesslist", u, img, crsf));
    const std::string dir = std::filesystem::path(crsf).parent_path().string();

    /* bad arguments */
    uint32_t n = 0;
    EXPECT_EQ(cr_ui_session_list(nullptr, nullptr, &n), 0x50030003u);
    EXPECT_EQ(cr_ui_session_list(dir.c_str(), nullptr, nullptr), 0x50030003u);

    /* capacity probe: the directory has at least OUR session */
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_GE(n, 1u);

    std::vector<cr_ui_session_row> rows(n);

    /* under-capacity: TRUNCATED + the filled count (the snapshot_dfs
     * contract) */
    if (n >= 2) {
        uint32_t cap = n - 1;
        EXPECT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_ERR_TRUNCATED);
        EXPECT_EQ(cap, n - 1u);
    }

    /* full rows: OUR session's row agrees with the loaded session */
    uint32_t cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    ASSERT_EQ(cap, n);
    const cr_ui_session_row* row = nullptr;
    for (const auto& r : rows)
        if (crsf == r.path)
            row = &r;
    ASSERT_NE(row, nullptr);
    ASSERT_EQ(row->valid, 1u);
    EXPECT_EQ(row->error, CR_UI_OK);
    EXPECT_GT(row->save_unix, 0ull); /* the v5 stamp from the save path */
    cr_ui_session_info si{};
    ASSERT_EQ(cr_ui_session_get_info(u.sess, &si), CR_UI_OK);
    EXPECT_EQ(row->fs_family, si.fs_family);
    EXPECT_EQ(row->stage_done_mask, si.stage_done_mask);
    EXPECT_EQ(row->configured_mask, si.configured_mask);
    EXPECT_STREQ(row->label, si.label);
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_session_stats(u.sess, &st), CR_UI_OK);
    EXPECT_EQ(row->files_found, st.files_found);
    EXPECT_EQ(row->deleted_found, st.deleted_found);
    EXPECT_EQ(row->carved_found, st.carved_found);
    EXPECT_EQ(row->bytes_scanned, st.bytes_scanned);

    /* bad directory: ERR_OPEN_FAILED - the listing fails loud, never a
     * silent empty answer */
    uint32_t m = 4;
    EXPECT_EQ(cr_ui_session_list("no-such-dir-w1a", nullptr, &m), 0x50020001u);
    EXPECT_EQ(cr_ui_session_list("no-such-dir-w1a", rows.data(), &m), 0x50020001u);
}

namespace {
/* W1b (R-54): the load-progress event observer. */
struct LoadEvents {
    unsigned events = 0;
    uint64_t last_done = 0, last_total = 0;
    uint32_t last_pm = 0;
    bool terminal = false;
    volatile uint32_t* cancel = nullptr;
    bool cancel_on_first = false;
};
void load_event_cb(void* user, const cr_ui_event* evt) {
    auto* st = static_cast<LoadEvents*>(user);
    if (evt->type != CR_EVT_SESSION_LOAD_PROGRESS)
        return;
    ++st->events;
    st->last_done = evt->bytes_done;
    st->last_total = evt->bytes_total;
    st->last_pm = evt->stage_permille;
    if (evt->bytes_done == evt->bytes_total)
        st->terminal = true;
    if (st->cancel_on_first && st->cancel)
        *st->cancel = 1; /* deterministic same-thread cancel (the og polls
                          * the progress object from the loading thread) */
}
} // namespace

TEST(CrUi, SessionLoadProgressEventsAndCancelFlag) {
    /* W1b (R-54): the load-side twin of the N-060 save machinery -
     * coalesced CR_EVT_SESSION_LOAD_PROGRESS events (first tick,
     * permille steps, terminal) and the CALLER-OWNED cancel flag polled
     * per node (the og's per-child IsCancelled mirror). */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("loadprog", u, img, crsf));

    /* the NULL/NULL variant IS the plain load (the contract) */
    cr_ui_session* s1 = nullptr;
    ASSERT_EQ(cr_ui_session_load_with_events(crsf.c_str(), nullptr, nullptr, nullptr, &s1),
              CR_UI_OK);
    ASSERT_NE(s1, nullptr);
    cr_ui_session_close(s1);

    /* events fire; the terminal tick lands done == total at permille 1000 */
    {
        cr_ui_session* s2 = nullptr;
        LoadEvents le;
        ASSERT_EQ(cr_ui_session_load_with_events(crsf.c_str(), load_event_cb, &le, nullptr, &s2),
                  CR_UI_OK);
        ASSERT_NE(s2, nullptr);
        cr_ui_session_close(s2);
        EXPECT_GE(le.events, 1u);
        EXPECT_TRUE(le.terminal);
        EXPECT_GT(le.last_total, 0u);
        EXPECT_EQ(le.last_done, le.last_total);
        EXPECT_EQ(le.last_pm, 1000u);
    }

    /* cancel: the flag aborts with CR_UI_ERR_CANCELLED and the out param
     * is NEVER half-claimed (a torn load hands out no session) */
    {
        volatile uint32_t cancel = 0;
        cr_ui_session* s3 = reinterpret_cast<cr_ui_session*>(uintptr_t(0x1234));
        LoadEvents le;
        le.cancel = &cancel;
        le.cancel_on_first = true;
        EXPECT_EQ(cr_ui_session_load_with_events(crsf.c_str(), load_event_cb, &le, &cancel, &s3),
                  0x50020005u);
        EXPECT_EQ(s3, reinterpret_cast<cr_ui_session*>(uintptr_t(0x1234)));
        EXPECT_GE(le.events, 1u);
        EXPECT_FALSE(le.terminal);
    }
}

TEST(CrUi, SessionListUnreadableFileGetsAnErrorRow) {
    /* W1a (R-56): a junk .crsf in the directory still gets a row:
     * valid = 0 + the decode error - the list never hides damage. */
    const std::string junk = cr_test::tmp_path("cr_ui", "_w1a_junk.crsf");
    {
        FILE* f = std::fopen(junk.c_str(), "wb");
        ASSERT_NE(f, nullptr);
        for (int i = 0; i < 100; ++i)
            std::fputc('x', f); /* >= the 64-byte header gate: BAD_MAGIC */
        std::fclose(f);
    }
    const std::string dir = std::filesystem::path(junk).parent_path().string();
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_GE(n, 1u);
    std::vector<cr_ui_session_row> rows(n);
    uint32_t cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    const cr_ui_session_row* row = nullptr;
    for (const auto& r : rows)
        if (junk == r.path)
            row = &r;
    ASSERT_NE(row, nullptr);
    EXPECT_EQ(row->valid, 0u);
    EXPECT_EQ(row->error, 0x50030001u); /* ERR_BAD_MAGIC at the container header */
    EXPECT_EQ(row->save_unix, 0ull);
    std::remove(junk.c_str());
}

TEST(CrUi, SessionListOgTimeOrdering) {
    /* N-116: the og's LastScanRecordView rows are TIME-ordered - the
     * record-2 SYSTEMTIME comparator (FUN_140273550: SystemTimeToFileTime
     * over row+0x20, !(ftA < ftB) = newest first) is what makes the
     * "Continue last scanning" banner pick the right file. Core listed
     * path-byte order; a host rendering row 0 as the most recent session
     * got the wrong answer. Two sessions in ONE isolated directory, the
     * NEWER one path-sorting LAST, >1 s apart so the second stamps
     * differ: row 0 must be the newer file. */
    Ui u1;
    std::string img1, crsf1;
    ASSERT_TRUE(scan_to_session("older", u1, img1, crsf1));
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));
    Ui u2;
    std::string img2, crsf2;
    ASSERT_TRUE(scan_to_session("newer", u2, img2, crsf2));

    /* the isolated listing dir: the OLDER session under the path-first
     * name, the NEWER under the path-last name (path order and time
     * order disagree BY CONSTRUCTION). The expected paths are built in
     * the NATIVE separator (std::filesystem::path): the listing rows are
     * native-joined and the '/'-joined form is a Linux-only compare (the
     * release-27 catch - three session-list pins Windows-red on it) */
    const std::string dir = cr_test::tmp_path("cr_ui", "_n116_order");
    std::filesystem::create_directories(dir);
    const std::string aa = (std::filesystem::path(dir) / "aa_older.crsf").string();
    const std::string zz = (std::filesystem::path(dir) / "zz_newer.crsf").string();
    ASSERT_TRUE(
        std::filesystem::copy_file(crsf1, aa, std::filesystem::copy_options::overwrite_existing));
    ASSERT_TRUE(
        std::filesystem::copy_file(crsf2, zz, std::filesystem::copy_options::overwrite_existing));

    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_EQ(n, 2u);
    std::vector<cr_ui_session_row> rows(n);
    uint32_t cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    ASSERT_EQ(cap, 2u);
    /* newest first: the path-LAST file is row 0, and its stamp is the
     * greater one (or equal, never smaller - the 1.2 s gap makes it
     * strictly greater in practice) */
    EXPECT_EQ(zz, rows[0].path) << "row 0 must be the NEWEST session (the og's "
                                   "time ordering), not the path-first one";
    EXPECT_EQ(aa, rows[1].path);
    EXPECT_GE(rows[0].save_unix, rows[1].save_unix);
    ASSERT_EQ(rows[0].valid, 1u);
    ASSERT_EQ(rows[1].valid, 1u);
    std::filesystem::remove_all(dir);
}

TEST(CrUi, SessionListTieBreakDeterministic) {
    /* N-116's tie-break: equal save_unix (the same session copied under
     * two names) orders by path bytes ascending - the deterministic
     * documented rule, so the row order is always reproducible. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("tie", u, img, crsf));
    const std::string dir = cr_test::tmp_path("cr_ui", "_n116_tie");
    std::filesystem::create_directories(dir);
    ASSERT_TRUE(std::filesystem::copy_file(crsf, (std::filesystem::path(dir) / "bb.crsf").string(),
                                           std::filesystem::copy_options::overwrite_existing));
    ASSERT_TRUE(std::filesystem::copy_file(crsf, (std::filesystem::path(dir) / "aa.crsf").string(),
                                           std::filesystem::copy_options::overwrite_existing));
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_EQ(n, 2u);
    std::vector<cr_ui_session_row> rows(n);
    uint32_t cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    EXPECT_EQ((std::filesystem::path(dir) / "aa.crsf").string(), rows[0].path);
    EXPECT_EQ((std::filesystem::path(dir) / "bb.crsf").string(), rows[1].path);
    EXPECT_EQ(rows[0].save_unix, rows[1].save_unix);
    std::filesystem::remove_all(dir);
}

TEST(CrUi, SessionListVolumeLabelFullWidthRoundTrips) {
    /* The og's record-2 volumeName[100] (strncpy 99, rsf-format
     * --scan-session.md 5.2 +0x04) vs the core's 31-char session card: a
     * 60-char HOST label now round-trips COMPLETE through the v6 record
     * tail into row.label_full (the ABI append); the legacy label[32]
     * keeps its 31-char prefix view (old callers see exactly what they
     * always saw); a short label's label_full == label (the uniform
     * degrade - v1..v5 sessions surface the legacy card unchanged). */
    const std::string label60 =
        "012345678901234567890123456789012345678901234567890123456789"; /* 60 */
    Ui u;
    const std::string img = write_fixture("sesslw");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.volume_label_utf8 = label60.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    const std::string crsf = img + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);

    /* the loaded session's info surface carries the complete label; the
     * legacy card field keeps the 31-char prefix (NUL at [31]) */
    cr_ui_session_info si{};
    ASSERT_EQ(cr_ui_session_get_info(u.sess, &si), CR_UI_OK);
    EXPECT_STREQ(si.label_full, label60.c_str());
    EXPECT_EQ(std::strlen(si.label), 31u);
    EXPECT_EQ(std::string(si.label), label60.substr(0, 31));

    /* the list row agrees with the load on the full label */
    const std::string dir = std::filesystem::path(crsf).parent_path().string();
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_GE(n, 1u);
    std::vector<cr_ui_session_row> rows(n);
    uint32_t cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    const cr_ui_session_row* row = nullptr;
    for (const auto& r : rows)
        if (crsf == r.path)
            row = &r;
    ASSERT_NE(row, nullptr);
    ASSERT_EQ(row->valid, 1u);
    EXPECT_STREQ(row->label_full, label60.c_str());
    EXPECT_EQ(std::strlen(row->label), 31u);
    EXPECT_EQ(std::string(row->label), label60.substr(0, 31));

    /* the short-label degrade: a fresh scan with a tiny label carries
     * label_full == label (the uniform join; old-width files identical) */
    Ui u2;
    const std::string img2 = write_fixture("sesssh");
    ASSERT_FALSE(img2.empty());
    ASSERT_EQ(cr_ui_open_image(img2.c_str(), 0, 0, &u2.dev), CR_UI_OK);
    cr_ui_scan_config cfg2{};
    cfg2.volume_label_utf8 = "tiny";
    ASSERT_EQ(cr_ui_scan_create(&cfg2, &u2.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u2.scan, u2.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u2.scan), CR_UI_OK);
    const std::string crsf2 = img2 + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u2.scan, crsf2.c_str()), CR_UI_OK);
    cr_ui_scan_close(u2.scan);
    u2.scan = nullptr;
    /* re-probe: the second save added a file to the shared dir (the
     * two-call contract - a stale capacity returns TRUNCATED, never a
     * silent short read) */
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    rows.resize(n);
    cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    const cr_ui_session_row* row2 = nullptr;
    for (const auto& r : rows)
        if (crsf2 == r.path)
            row2 = &r;
    ASSERT_NE(row2, nullptr);
    EXPECT_STREQ(row2->label_full, "tiny");
    EXPECT_STREQ(row2->label, "tiny");
}

TEST(CrUi, SessionListScopeSupersetPinsAutoAndManual) {
    /* The scope rule: the og's home list keeps ONLY names containing
     * BOTH auto_ and .rsf (FUN_140273940, DRWUI:432326-432333 +
     * rsf-format 5.5); the core lists EVERY *.crsf - the REGISTERED
     * SUPERSET (a manual save is a resumable session too; a host wanting
     * the og's exact home view filters client-side). Pin: BOTH classes
     * list, both valid, same save_unix for the same underlying file. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("scope", u, img, crsf));
    const std::string dir = cr_test::tmp_path("cr_ui", "_scope_superset");
    std::filesystem::create_directories(dir);
    ASSERT_TRUE(std::filesystem::copy_file(
        crsf, (std::filesystem::path(dir) / "auto_test_10_14_23_13_21.crsf").string(),
        std::filesystem::copy_options::overwrite_existing));
    ASSERT_TRUE(
        std::filesystem::copy_file(crsf, (std::filesystem::path(dir) / "manual-save.crsf").string(),
                                   std::filesystem::copy_options::overwrite_existing));
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), nullptr, &n), CR_UI_OK);
    ASSERT_EQ(n, 2u);
    std::vector<cr_ui_session_row> rows(n);
    uint32_t cap = n;
    ASSERT_EQ(cr_ui_session_list(dir.c_str(), rows.data(), &cap), CR_UI_OK);
    const std::string auto_path =
        (std::filesystem::path(dir) / "auto_test_10_14_23_13_21.crsf").string();
    const std::string manual_path = (std::filesystem::path(dir) / "manual-save.crsf").string();
    const cr_ui_session_row* auto_row = nullptr;
    const cr_ui_session_row* manual_row = nullptr;
    for (const auto& r : rows) {
        if (std::string(r.path) == auto_path)
            auto_row = &r;
        if (std::string(r.path) == manual_path)
            manual_row = &r;
    }
    ASSERT_NE(auto_row, nullptr);
    ASSERT_NE(manual_row, nullptr);
    EXPECT_EQ(auto_row->valid, 1u);   /* the og's home-list class */
    EXPECT_EQ(manual_row->valid, 1u); /* the superset's extra class */
    EXPECT_EQ(auto_row->save_unix, manual_row->save_unix);
    EXPECT_GT(auto_row->save_unix, 0ull);
}

TEST(CrUi, EnumerationRefreshContractPinned) {
    /* N-098: the always-fresh enumeration contract - each
     * cr_ui_enumerate_disks call re-queries the OS, no cache, no retained
     * state; calling again IS the refresh (the og's modal flow re-runs the
     * same machinery; the chrome is host-side). The exe-level two-call
     * probe runs on the Windows lane; this pin guards the invariant where
     * every lane can see it: the header contract sentence must stay, and
     * the implementation must keep constructing a local list per call (a
     * future cache MUST come with a contract update - which this test
     * then fails loudly). */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    const size_t at = hdr.find("cr_ui_enumerate_disks");
    ASSERT_NE(std::string::npos, at);
    const std::string block = hdr.substr(at > 800 ? at - 800 : 0, 900);
    EXPECT_NE(std::string::npos, block.find("RE-QUERIES the OS"))
        << "the enumeration contract must state the always-fresh rule";
    EXPECT_NE(std::string::npos, block.find("no cache"))
        << "the enumeration contract must deny caching explicitly";
    /* the implementation: a LOCAL PhysicalDiskList per invocation, no
     * static or member state in the enumerate path */
    const std::string src = cr_test::read_repo_text("src/ui/cr_ui.cpp");
    ASSERT_FALSE(src.empty());
    const size_t impl = src.find("u32 enumerate_disks_impl(");
    ASSERT_NE(std::string::npos, impl);
    const size_t impl_end = src.find("\nu32 ", impl + 10);
    const std::string body =
        src.substr(impl, (impl_end == std::string::npos ? src.size() : impl_end) - impl);
    EXPECT_NE(std::string::npos, body.find("win::PhysicalDiskList list;"))
        << "the enumerate path must construct its list locally per call";
    EXPECT_EQ(std::string::npos, body.find("static"))
        << "no static state may appear inside enumerate_disks_impl";
    /* the C++-only refresh IOCTL stays documented, not exported */
    const std::string gap = cr_test::read_repo_text("docs/REFERENCE-API-GAP.md");
    ASSERT_FALSE(gap.empty());
    EXPECT_NE(std::string::npos, gap.find("calling again IS the refresh"))
        << "R-86's row must carry the pull-model equivalence statement";
}

TEST(CrUi, EnumerationExFlagGrammarRefusesUnknownBits) {
    /* M-076: the _ex enumeration grammar - unknown flag bits are refused
     * with CR_UI_ERR_BAD_PARAMETER BEFORE the platform check (every lane
     * observes the same grammar; a host must not learn about an ignored
     * flag from a platform-dependent success). Known bits proceed to the
     * platform truth (ERR_UNSUPPORTED here). */
    cr_ui_disk_info di;
    uint32_t ac = 0;
    EXPECT_EQ(cr_ui_enumerate_disks_ex(&di, &ac, 0xDEADBEEFu), 0x50030003u)
        << "unknown flag bits must be refused, not ignored";
    EXPECT_NE(std::strstr(cr_ui_last_error(), "unknown enumeration flag bits"), nullptr)
        << "the refusal must name the grammar violation";
    EXPECT_EQ(cr_ui_enumerate_disks_ex(&di, &ac, CR_ENUM_FLAG_TRIM | 0x10u), 0x50030003u)
        << "a known bit plus an unknown bit is still refused";
#ifndef _WIN32
    /* platform truth: the valid-grammar call reaches the win-backed
     * surface and answers ERR_UNSUPPORTED, never a silent fake */
    EXPECT_EQ(cr_ui_enumerate_disks_ex(&di, &ac, 0u), 0x50030008u);
    EXPECT_EQ(cr_ui_enumerate_disks_ex(&di, &ac, CR_ENUM_FLAG_TRIM), 0x50030008u);
    EXPECT_NE(std::strstr(cr_ui_last_error(), "Windows platform layer"), nullptr);
#endif
    /* the header contract sentences (the opt-in + the never-guessed rule)
     * are pinned by Enumerate.TrimChipOptInShipsInAbiAndCli */
}

TEST(CrUi, SessionListCliAndAbiOrderAgree) {
    /* N-116's CLI half: rrecov's `sessions` verb must enumerate in the
     * SAME order the ABI returns (save time first, path tie-break) - the
     * exe is Windows-only, so this pins the verb's source: the rows
     * re-sort by the ABI's time key after the path-sorted enumeration
     * (the enumeration sort stays - it is the deterministic input). */
    const std::string src = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(src.empty());
    const size_t verb = src.find("int print_sessions(");
    ASSERT_NE(std::string::npos, verb);
    const std::string body = src.substr(verb, 4000);
    EXPECT_NE(std::string::npos, body.find("the ABI's time key"))
        << "print_sessions must re-sort its rows by save time (newest first, "
           "path tie-break) - the same key cr_ui_session_list returns";
    /* and the stale path-only row order must not return: the old comment
     * ('the ABI's deterministic row order' on the enumeration sort) was
     * the pre-N-116 contract marker */
    EXPECT_EQ(std::string::npos, body.find("the ABI's deterministic row order"))
        << "the enumeration sort is the deterministic INPUT, not the row "
           "order contract";
}

TEST(CrUi, PauseBlocksAndResumes) {
    const std::string img = write_fixture("pause");
    ASSERT_FALSE(img.empty());
    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);

    /* pause BEFORE run: engines block in checkpoint(); telemetry shows it */
    ASSERT_EQ(cr_ui_scan_pause(u.scan), CR_UI_OK);
    std::atomic<bool> done{false};
    std::thread runner([&] {
        EXPECT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        done.store(true);
    });
    bool saw_paused = false;
    for (int i = 0; i < 200 && !done.load(); ++i) { /* up to ~2 s */
        cr_ui_telemetry tel{};
        if (cr_ui_scan_telemetry(u.scan, &tel) == CR_UI_OK && tel.paused == 1u)
            saw_paused = true;
        if (saw_paused)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(saw_paused);
    ASSERT_EQ(cr_ui_scan_resume(u.scan), CR_UI_OK);
    runner.join();
    ASSERT_TRUE(done.load());
    cr_ui_telemetry tel{};
    ASSERT_EQ(cr_ui_scan_telemetry(u.scan, &tel), CR_UI_OK);
    EXPECT_EQ(tel.overall_permille, 1000u);
}

TEST(CrUi, ScanBusyStoppingQueries) {
    /* N-075: the og's reference-out query signals - sigIsBusy(bool&) x14,
     * sigIsStopping(bool&) x8 (corpus UIUX/25-connect-wiring-graph.md
     * :125-144) - surfaced as synchronous ABI query ops. The pause gate is
     * a deterministic mid-run window: busy must be true while run()
     * executes, stopping must flip the moment cancel() lands (the LIVE
     * token request), and both must answer at rest. stopping stays true
     * after the run ends: the token is one-shot, the honest "a cancel was
     * requested on this handle" answer. */
    const std::string img = write_fixture("busyq");
    ASSERT_FALSE(img.empty());
    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);

    /* at rest: neither busy nor stopping */
    uint32_t busy = 5, stopping = 5;
    ASSERT_EQ(cr_ui_scan_busy(u.scan, &busy), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_stopping(u.scan, &stopping), CR_UI_OK);
    EXPECT_EQ(busy, 0u) << "a scan that never ran is not busy";
    EXPECT_EQ(stopping, 0u) << "no cancel was requested";

    /* mid-run through the pause gate: busy=true, stopping still false */
    ASSERT_EQ(cr_ui_scan_pause(u.scan), CR_UI_OK);
    std::atomic<bool> done{false};
    std::atomic<uint32_t> run_rc{0};
    std::thread runner([&] {
        run_rc = cr_ui_scan_run(u.scan);
        done = true;
    });
    bool saw_busy = false;
    for (int i = 0; i < 300 && !done.load(); ++i) { /* up to ~3 s */
        uint32_t b = 0;
        if (cr_ui_scan_busy(u.scan, &b) == CR_UI_OK && b == 1u)
            saw_busy = true;
        if (saw_busy)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ASSERT_TRUE(saw_busy) << "busy must be observable while run() executes";
    ASSERT_EQ(cr_ui_scan_busy(u.scan, &busy), CR_UI_OK);
    EXPECT_EQ(busy, 1u);
    ASSERT_EQ(cr_ui_scan_stopping(u.scan, &stopping), CR_UI_OK);
    EXPECT_EQ(stopping, 0u) << "parked is not stopping - no cancel requested";

    /* cancel while parked: stopping flips immediately. The post-cancel
     * busy==1 observation ("still winding down") was a race by
     * construction (run 41027072251: the unparked runner cleared
     * `running` before this thread's adjacent query - busy read 0
     * where the line demanded 1): between token.cancel() and
     * running.store(false) the exit path is a heartbeat join (unarmed
     * in this test) and one store, so no timing contract makes the
     * transition observable. The deterministic contract stays fully
     * pinned: busy==1 while run() is live was proven above at the
     * park; stopping is visible at once (next line); run() returns
     * CANCELLED and busy settles to 0 after the join (both below). */
    ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_stopping(u.scan, &stopping), CR_UI_OK);
    EXPECT_EQ(stopping, 1u) << "the live cancel request must be visible at once";

    runner.join();
    ASSERT_TRUE(done.load());
    EXPECT_EQ(run_rc.load(), CR_UI_ERR_CANCELLED) << "cancel during the pause gate aborts the run";

    /* after the cancelled run: busy=false; the one-shot request stays
     * visible (the og's isStopping answers the same for its one-shot
     * stop event) */
    ASSERT_EQ(cr_ui_scan_busy(u.scan, &busy), CR_UI_OK);
    EXPECT_EQ(busy, 0u);
    ASSERT_EQ(cr_ui_scan_stopping(u.scan, &stopping), CR_UI_OK);
    EXPECT_EQ(stopping, 1u) << "the token is one-shot: the request persists";

    /* the bad-argument contract of both query ops */
    EXPECT_EQ(cr_ui_scan_busy(nullptr, &busy), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_scan_busy(u.scan, nullptr), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_scan_stopping(nullptr, &stopping), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_scan_stopping(u.scan, nullptr), CR_UI_ERR_BAD_PARAMETER);
}

TEST(CrUi, SnapshotEnumeration) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("snap", u, img, crsf));
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);

    uint32_t count = 0;
    ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
    EXPECT_GE(count, 3u); /* roots + 3 files */
    uint32_t partial = 1;
    ASSERT_EQ(cr_ui_snapshot_partial(u.snap, &partial), CR_UI_OK);
    EXPECT_EQ(partial, 0u); /* quiesced tree */

    /* two-call DFS */
    uint32_t needed = 0;
    ASSERT_EQ(cr_ui_snapshot_dfs(u.snap, nullptr, &needed), CR_UI_OK);
    ASSERT_GT(needed, 0u);
    std::vector<uint32_t> order(needed);
    uint32_t filled = needed;
    ASSERT_EQ(cr_ui_snapshot_dfs(u.snap, order.data(), &filled), CR_UI_OK);
    EXPECT_EQ(filled, needed);
    /* a too-small buffer is TRUNCATED, never a lie */
    uint32_t small = 1;
    std::vector<uint32_t> tiny(1);
    EXPECT_EQ(cr_ui_snapshot_dfs(u.snap, tiny.data(), &small), 0x50030004u);

    /* node fields + the truncated-name contract */
    bool saw_doc = false;
    for (uint32_t i = 0; i < count && !saw_doc; ++i) {
        cr_ui_node_info info{};
        char name[512];
        ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
        if (std::strcmp(name, "DOC.TXT") == 0) {
            saw_doc = true;
            EXPECT_EQ(info.logical_size, 1000u);
            EXPECT_EQ(info.deleted, 0u);
            EXPECT_EQ(info.is_full, 0u);
            EXPECT_EQ(info.name_len, 7u);
            char three[3];
            EXPECT_EQ(cr_ui_snapshot_node(u.snap, i, &info, three, sizeof(three)), 0x50030004u);
            EXPECT_EQ(info.name_len, 7u); /* full length survives truncation */
        }
    }
    EXPECT_TRUE(saw_doc);
}

TEST(CrUi, QueryFiltersSortPaging) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("query", u, img, crsf));

    /* zero-init = match-all */
    cr_ui_query q{};
    uint64_t total = 0;
    cr_ui_query_row* rows = nullptr;
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_GE(total, 3u);
    const uint64_t full_total = total;
    cr_ui_query_rows_free(rows, n);

    /* path_prefix (ABI-since core-api, the og AdvanceFilterCondition path
     * group): the root segment of an ACTUAL row's path selects the whole
     * tree (case-insensitive); a non-matching prefix selects nothing; a
     * full-path prefix selects exactly that row */
    {
        q = cr_ui_query{};
        q.limit = 50;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        ASSERT_GE(n, 1u);
        ASSERT_NE(rows[0].path, nullptr);
        const std::string p0(rows[0].path);
        const auto slash = p0.find('/');
        ASSERT_NE(slash, std::string::npos) << "row path carries the root segment: " << p0;
        const std::string root_seg = p0.substr(0, slash);
        const std::string up_seg = [](std::string s) {
            for (char& c : s)
                if (c >= 'a' && c <= 'z')
                    c = char(c - 'a' + 'A');
            return s;
        }(root_seg);
        /* the DOC row's full path as the prefix -> exactly itself */
        std::string doc_path;
        for (uint32_t i = 0; i < n; ++i)
            if (rows[i].name && std::strcmp(rows[i].name, "DOC.TXT") == 0)
                doc_path = rows[i].path;
        ASSERT_FALSE(doc_path.empty());
        cr_ui_query_rows_free(rows, n);
        rows = nullptr;

        q = cr_ui_query{};
        q.path_prefix = root_seg.c_str();
        q.count_only = 1;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_EQ(total, full_total) << "root-segment prefix selects the whole tree";

        q = cr_ui_query{};
        q.path_prefix = up_seg.c_str();
        q.count_only = 1;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_EQ(total, full_total) << "prefix matching is ASCII-case-insensitive";

        q = cr_ui_query{};
        q.path_prefix = "NO/SUCH/PREFIX";
        q.count_only = 1;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_EQ(total, 0u);

        q = cr_ui_query{};
        q.path_prefix = doc_path.c_str();
        q.limit = 50;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_EQ(total, 1u);
        ASSERT_EQ(n, 1u);
        EXPECT_STREQ(rows[0].name, "DOC.TXT");
        cr_ui_query_rows_free(rows, n);
        rows = nullptr;
    }

    /* category: .jpg -> Image */
    q = cr_ui_query{};
    const char* exts[] = {".jpg"};
    q.extensions = exts;
    q.extension_count = 1;
    q.limit = 50;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_EQ(total, 1u);
    ASSERT_EQ(n, 1u);
    EXPECT_STREQ(rows[0].name, "PHOTO.JPG");
    EXPECT_EQ(rows[0].category, 1u); /* Category::Image */
    cr_ui_query_rows_free(rows, n);

    /* deleted_only keeps DELFILE.TXT */
    q = cr_ui_query{};
    q.deleted_only = 1;
    q.limit = 50;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_GE(total, 1u);
    cr_ui_query_rows_free(rows, n);

    /* name sort + paging: the deleted 8.3 name renders as "#ELFILE.TXT"
     * (the 0xE5 marker destroys the first character; the og's '#'
     * placeholder - N-172 - sorts FIRST at 0x23), so the ascending order
     * is #ELFILE.TXT, DOC.TXT, PHOTO.JPG -> page [1,1] = DOC.TXT */
    q = cr_ui_query{};
    q.sort = 1; /* name */
    q.offset = 1;
    q.limit = 1;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    ASSERT_EQ(n, 1u);
    EXPECT_STREQ(rows[0].name, "DOC.TXT");
    cr_ui_query_rows_free(rows, n);

    /* size range: only DOC.TXT is >= 600 bytes */
    q = cr_ui_query{};
    q.size_min = 600;
    q.limit = 50;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_EQ(total, 1u);
    if (n == 1) {
        EXPECT_STREQ(rows[0].name, "DOC.TXT");
    }
    cr_ui_query_rows_free(rows, n);

    /* count_only: no rows materialized */
    q = cr_ui_query{};
    q.count_only = 1;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_EQ(rows, nullptr);
    EXPECT_EQ(n, 0u);
    EXPECT_GE(total, 3u);
}

TEST(CrUi, PreviewSessionAndSnapshot) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("prev", u, img, crsf));
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);

    const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(doc, 0u);

    cr_ui_preview* pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, doc, &pv), CR_UI_OK);
    uint64_t size = 0;
    ASSERT_EQ(cr_ui_preview_size(pv, &size), CR_UI_OK);
    EXPECT_EQ(size, 1000u);
    char buf[16] = {0};
    uint32_t placed = 0;
    ASSERT_EQ(cr_ui_preview_read(pv, 0, buf, 7, &placed), CR_UI_OK);
    EXPECT_EQ(placed, 7u);
    EXPECT_STREQ(buf, "DOCDATA");
    /* EOF short read */
    uint64_t big = 0;
    ASSERT_EQ(cr_ui_preview_size(pv, &big), CR_UI_OK);
    std::vector<char> tail(64, 0);
    ASSERT_EQ(cr_ui_preview_read(pv, big - 4, tail.data(), 64, &placed), CR_UI_OK);
    EXPECT_EQ(placed, 4u);
    uint64_t unread = 99;
    uint32_t ranges = 99;
    ASSERT_EQ(cr_ui_preview_unreadable(pv, &unread, &ranges), CR_UI_OK);
    EXPECT_EQ(unread, 0u);
    EXPECT_EQ(ranges, 0u);
    cr_ui_metadata meta{};
    ASSERT_EQ(cr_ui_preview_metadata(pv, &meta), CR_UI_OK);
    EXPECT_GT(std::strlen(meta.kind), 0u);
    cr_ui_preview_close(pv);

    /* snapshot variant: same bytes, independent of the session handle */
    uint32_t count = 0;
    ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
    int doc_idx = -1;
    for (uint32_t i = 0; i < count && doc_idx < 0; ++i) {
        cr_ui_node_info info{};
        char name[512];
        ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
        if (std::strcmp(name, "DOC.TXT") == 0)
            doc_idx = int(i);
    }
    ASSERT_GE(doc_idx, 0);
    pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_snapshot(u.dev, u.snap, uint32_t(doc_idx), &pv), CR_UI_OK);
    std::memset(buf, 0, sizeof(buf));
    ASSERT_EQ(cr_ui_preview_read(pv, 0, buf, 7, &placed), CR_UI_OK);
    EXPECT_STREQ(buf, "DOCDATA");
    cr_ui_preview_close(pv);

    /* a root/virtual node has no previewable content (roots are created
     * first in every tree, so node id 0 is one) */
    pv = nullptr;
    EXPECT_NE(cr_ui_preview_open_session(u.dev, u.sess, 0u, &pv), CR_UI_OK);
    EXPECT_EQ(pv, nullptr);
}

TEST(CrUi, M101TagsExtractedMarkAndBranchAxes) {
    /* M-101 (this lane's rider over the racing seat): the Tags Extracted
     * PRODUCER + the ABI exposure. The seat landed the carrier (the props
     * flag + query::tags_only + session bit-14) but no stamping path and
     * no ABI surface; this rider is both. A real EXIF read through
     * preview_metadata marks the live session node's props (the seat's
     * documented semantics: orientation/datetime, duration/codec,
     * dimensions); the query axis + row fields + snapshot node_info
     * observe the mark; a text neighbor stays unmarked; session close
     * invalidates the mark path (the private copy still serves metadata);
     * the versions axis maps (the seat's u64 hash filter). */
    /* the fixture: PHOTO.JPG carrying a minimal EXIF APP1 (Orientation 6)
     * - 40 bytes at the file's cluster, the pattern tail stays behind it */
    auto img = build_fat16();
    {
        unsigned char exif_jpeg[] = {
            0xFF, 0xD8,                   /* SOI */
            0xFF, 0xE1, 0x00, 0x22,       /* APP1, len 34 */
            'E',  'x',  'i',  'f',  0, 0, /* "Exif\0\0" */
            'I',  'I',  0x2A, 0x00,       /* TIFF II, magic */
            0x08, 0x00, 0x00, 0x00,       /* IFD0 at TIFF+8 */
            0x01, 0x00,                   /* 1 entry */
            0x12, 0x01,                   /* tag 274 Orientation */
            0x03, 0x00,                   /* type 3 SHORT */
            0x01, 0x00, 0x00, 0x00,       /* count 1 */
            0x06, 0x00, 0x00, 0x00,       /* value 6 (inline) */
            0x00, 0x00, 0x00, 0x00,       /* next IFD: none */
            0xFF, 0xD9                    /* EOI */
        };
        unsigned char* p = img.data() + kDATA + (30 - 2) * 512;
        std::memcpy(p, exif_jpeg, sizeof(exif_jpeg));
    }
    const std::string img_path = cr_test::tmp_path("cr_ui", "m101.jpg.img");
    {
        FILE* f = std::fopen(img_path.c_str(), "wb");
        ASSERT_NE(f, nullptr);
        ASSERT_EQ(std::fwrite(img.data(), 1, img.size(), f), img.size());
        std::fclose(f);
    }

    Ui u;
    ASSERT_EQ(cr_ui_open_image(img_path.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    const std::string crsf = img_path + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);

    const uint64_t photo = query_node_id(u.sess, "PHOTO.JPG");
    ASSERT_NE(photo, 0u);
    const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(doc, 0u);

    /* before the read: the axis is empty (zero-init safe) */
    {
        cr_ui_query q{};
        q.tags_only = 1;
        q.limit = 50;
        cr_ui_query_row* rows = nullptr;
        uint32_t n = 0;
        uint64_t total = 0;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_EQ(total, 0u) << "nothing is tag-marked before any read";
        EXPECT_EQ(n, 0u);
        cr_ui_query_rows_free(rows, n);
    }

    /* the tag read: preview metadata on PHOTO.JPG extracts Orientation 6 */
    cr_ui_preview* pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, photo, &pv), CR_UI_OK);
    cr_ui_metadata meta{};
    ASSERT_EQ(cr_ui_preview_metadata(pv, &meta), CR_UI_OK);
    EXPECT_EQ(std::string(meta.kind), "image");
    EXPECT_EQ(std::string(meta.orientation), "6") << "the EXIF read happened";
    cr_ui_preview_close(pv);

    /* after the read: PHOTO.JPG is the only tag-marked row; the row
     * carries the axis; DOC.TXT stays unmarked; the versions axis maps
     * (no FAT groups -> empty) and zero-inits off */
    {
        cr_ui_query q{};
        q.tags_only = 1;
        q.limit = 50;
        cr_ui_query_row* rows = nullptr;
        uint32_t n = 0;
        uint64_t total = 0;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        ASSERT_EQ(total, 1u);
        ASSERT_EQ(n, 1u);
        EXPECT_STREQ(rows[0].name, "PHOTO.JPG");
        EXPECT_EQ(rows[0].tags_extracted, 1u);
        EXPECT_EQ(rows[0].version_group, 0u)
            << "FAT files carry no group (the seat: NTFS-scoped stamping)";
        cr_ui_query_rows_free(rows, n);
    }
    {
        cr_ui_query q{}; /* default: both axes off */
        q.limit = 50;
        cr_ui_query_row* rows = nullptr;
        uint32_t n = 0;
        uint64_t total = 0;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_GE(total, 2u); /* DOC + PHOTO at least */
        for (uint32_t i = 0; i < n; ++i) {
            if (std::strcmp(rows[i].name, "PHOTO.JPG") == 0) {
                EXPECT_EQ(rows[i].tags_extracted, 1u);
            }
            if (std::strcmp(rows[i].name, "DOC.TXT") == 0) {
                EXPECT_EQ(rows[i].tags_extracted, 0u)
                    << "a text file yields no tag-family datum: unmarked";
                EXPECT_EQ(rows[i].version_group, 0u);
            }
        }
        cr_ui_query_rows_free(rows, n);
    }
    {
        cr_ui_query q{}; /* the versions axis maps at the ABI: a nonzero
                          * group filter over an all-zero session (the
                          * seat stamps only structured NTFS nodes) */
        q.version_group = 0x1234ull;
        q.limit = 50;
        cr_ui_query_row* rows = nullptr;
        uint32_t n = 0;
        uint64_t total = 0;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        EXPECT_EQ(total, 0u)
            << "no group members on a FAT session (the seat: NTFS-scoped stamping)";
        cr_ui_query_rows_free(rows, n);
    }

    /* the snapshot node_info carries the mark */
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);
    {
        uint32_t count = 0;
        ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
        int photo_idx = -1;
        for (uint32_t i = 0; i < count && photo_idx < 0; ++i) {
            cr_ui_node_info info{};
            char name[512];
            ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
            if (std::strcmp(name, "PHOTO.JPG") == 0) {
                photo_idx = int(i);
                EXPECT_EQ(info.tags_extracted, 1u) << "the mark rides the snapshot rows";
                EXPECT_EQ(info.version_group, 0u);
            }
            if (std::strcmp(name, "DOC.TXT") == 0) {
                EXPECT_EQ(info.tags_extracted, 0u) << "text: unmarked through the snapshot rows";
            }
        }
        ASSERT_GE(photo_idx, 0);
    }

    /* session close invalidates the mark path: the preview's private copy
     * still serves metadata, nothing is marked (the watch is dead), and
     * the handle stays safe to use */
    {
        cr_ui_preview* pv2 = nullptr;
        ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, doc, &pv2), CR_UI_OK);
        cr_ui_session_close(u.sess);
        u.sess = nullptr; /* the Ui dtor must not double-close */
        cr_ui_metadata meta2{};
        ASSERT_EQ(cr_ui_preview_metadata(pv2, &meta2), CR_UI_OK)
            << "metadata still served from the private copy after close";
        EXPECT_GT(std::strlen(meta2.kind), 0u)
            << "metadata still served (kind sniffed) after close";
        cr_ui_preview_close(pv2);
    }
}

TEST(CrUi, RecoverySubtreeAndRename) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("rec", u, img, crsf));

    const std::string dest = cr_test::tmp_path("cr_ui", ".dest");
    cr_ui_recovery_config rcfg{};
    unsigned recovery_calls = 0;
    rcfg.progress = recovery_progress_cb;
    rcfg.progress_user = &recovery_calls;
    ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &u.rec), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_set_destination(u.rec, dest.c_str()), CR_UI_OK);

    /* enqueue before set_destination is a bad request (fresh handle) */
    {
        cr_ui_recovery* bad = nullptr;
        ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &bad), CR_UI_OK);
        const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
        EXPECT_EQ(cr_ui_recovery_enqueue_node(bad, u.sess, doc), 0x50030003u);
        cr_ui_recovery_close(bad);
    }

    /* subtree from the POPULATED root behaves like the CLI plan(): every
     * child file (deleted included - the CLI has no filter here), dirs as
     * items; roots themselves are never enqueued */
    {
        uint32_t count = 0;
        ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);
        ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
        uint64_t root_with_kids = 0;
        bool found = false; /* id 0 is a VALID root id - flag, not the id */
        for (uint32_t i = 0; i < count && !found; ++i) {
            cr_ui_node_info info{};
            char name[512];
            ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
            if (info.is_full && info.first_child != CR_UI_SNAP_NONE) {
                root_with_kids = info.node_id;
                found = true;
            }
        }
        ASSERT_TRUE(found);
        ASSERT_EQ(cr_ui_recovery_enqueue_subtree(u.rec, u.sess, root_with_kids), CR_UI_OK);
    }

    /* intra-queue duplicate of DOC.TXT -> collision rename "(2)" */
    const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(doc, 0u);
    ASSERT_EQ(cr_ui_recovery_enqueue_node(u.rec, u.sess, doc), CR_UI_OK);

    uint32_t queued = 0;
    ASSERT_EQ(cr_ui_recovery_queue_count(u.rec, &queued), CR_UI_OK);
    EXPECT_GE(queued, 4u); /* 3 subtree files + the duplicate */

    ASSERT_EQ(cr_ui_recovery_process(u.rec), CR_UI_OK);
    EXPECT_GT(recovery_calls, 0u);

    cr_ui_recovery_stats rs{};
    ASSERT_EQ(cr_ui_recovery_get_stats(u.rec, &rs), CR_UI_OK);
    EXPECT_GE(rs.done, 4u);
    EXPECT_EQ(rs.errors, 0u);

    /* the report carries every item with truthful outcomes */
    char* json = nullptr;
    ASSERT_EQ(cr_ui_recovery_report_json(u.rec, img.c_str(), &json), CR_UI_OK);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::strstr(json, "DOC.TXT"), nullptr);
    EXPECT_NE(std::strstr(json, "schema_version"), nullptr);
    EXPECT_NE(std::strstr(json, "computed"), nullptr); /* streamed hash status */
    cr_ui_string_free(json);

    /* bytes on disk: the original name and the renamed duplicate */
    const std::string doc_path = dest + "/DOC.TXT";
    FILE* f = std::fopen(doc_path.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    if (f) {
        char got[8] = {0};
        EXPECT_EQ(std::fread(got, 1, 7, f), 7u);
        EXPECT_EQ(std::memcmp(got, "DOCDATA", 7), 0);
        std::fclose(f);
    }
    const std::string renamed = dest + "/DOC_1.TXT";
    EXPECT_TRUE(std::filesystem::exists(renamed));

    /* snapshot-based enqueue (G-02b): same content through the value path */
    {
        cr_ui_recovery* r2 = nullptr;
        ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &r2), CR_UI_OK);
        const std::string dest2 = cr_test::tmp_path("cr_ui", ".dest2");
        ASSERT_EQ(cr_ui_recovery_set_destination(r2, dest2.c_str()), CR_UI_OK);
        uint32_t count = 0;
        ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
        int doc_idx = -1;
        for (uint32_t i = 0; i < count && doc_idx < 0; ++i) {
            cr_ui_node_info info{};
            char name[512];
            ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
            if (std::strcmp(name, "DOC.TXT") == 0)
                doc_idx = int(i);
        }
        ASSERT_GE(doc_idx, 0);
        ASSERT_EQ(cr_ui_recovery_enqueue_snapshot(r2, u.snap, uint32_t(doc_idx)), CR_UI_OK);
        ASSERT_EQ(cr_ui_recovery_process(r2), CR_UI_OK);
        cr_ui_recovery_stats rs2{};
        ASSERT_EQ(cr_ui_recovery_get_stats(r2, &rs2), CR_UI_OK);
        EXPECT_EQ(rs2.done, 1u);
        cr_ui_recovery_close(r2);
        std::error_code ec;
        std::filesystem::remove_all(dest2, ec);
    }

    std::error_code ec;
    std::filesystem::remove_all(dest, ec);
}

/* M-071: recover-all never writes the og's hidden junk set — the exact
 * regression the N-064 rrecov.cpp reconstruction caused at the CLI, pinned
 * here at the ABI on the same walk class (cr_ui's session plan is the
 * W-lane's recover-all surface; the CLI plan() carries the identical gate,
 * restored verbatim from the M-018 landing 69507cb). */
TEST(CrUi, RecoverAllSkipsJunkFlaggedNodes) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_junk_to_session("junkra", u, img, crsf));

    /* precondition: the query surface hides the junk set (M-018,
     * unregressed) while SVI's child stays a row (entries, not contents) */
    {
        cr_ui_query q{};
        q.limit = 100;
        cr_ui_query_row* rows = nullptr;
        uint32_t n = 0;
        uint64_t total = 0;
        ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
        auto has = [&](const char* name) {
            for (uint32_t i = 0; i < n; ++i)
                if (std::strcmp(rows[i].name, name) == 0)
                    return true;
            return false;
        };
        EXPECT_FALSE(has("DESKTOP.INI"));
        EXPECT_FALSE(has("System Volume Information"));
        EXPECT_TRUE(has("DOC.TXT"));
        EXPECT_TRUE(has("KEEP.ME")) << "SVI's child stays a query row (M-018: entries, "
                                       "not contents)";
        cr_ui_query_rows_free(rows, n);
    }

    /* the bulk walk from the session root: junk file skipped, junk dir
     * skipped with its subtree, real files written */
    const std::string dest = cr_test::tmp_path("cr_ui", ".junkdest");
    cr_ui_recovery_config rcfg{};
    ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &u.rec), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_set_destination(u.rec, dest.c_str()), CR_UI_OK);
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);
    {
        uint32_t count = 0;
        ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
        uint64_t root_with_kids = 0;
        bool found = false;
        for (uint32_t i = 0; i < count && !found; ++i) {
            cr_ui_node_info info{};
            char name[512];
            ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
            if (info.is_full && info.first_child != CR_UI_SNAP_NONE) {
                root_with_kids = info.node_id;
                found = true;
            }
        }
        ASSERT_TRUE(found);
        ASSERT_EQ(cr_ui_recovery_enqueue_subtree(u.rec, u.sess, root_with_kids), CR_UI_OK);
    }
    ASSERT_EQ(cr_ui_recovery_process(u.rec), CR_UI_OK);

    EXPECT_TRUE(std::filesystem::exists(dest + "/DOC.TXT"));
    EXPECT_TRUE(std::filesystem::exists(dest + "/PHOTO.JPG"));
    EXPECT_FALSE(std::filesystem::exists(dest + "/DESKTOP.INI"))
        << "the hidden junk file must not be written by a bulk walk";
    EXPECT_FALSE(std::filesystem::exists(dest + "/System Volume Information"))
        << "the hidden junk dir must not be written by a bulk walk";
    EXPECT_FALSE(std::filesystem::exists(dest + "/System Volume Information/KEEP.ME"))
        << "the skipped junk dir takes its subtree with it (the M-018 gate "
           "form: continue before descent)";

    std::error_code ec;
    std::filesystem::remove_all(dest, ec);
}

/* M-071: the snapshot bulk walk carries the same gate — snapshot rows
 * persist the junk flag and the value-path plan skips them. */
TEST(CrUi, RecoverAllSnapshotWalkSkipsJunkFlaggedNodes) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_junk_to_session("junksnap", u, img, crsf));
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);

    /* find the full root's snapshot index */
    uint32_t count = 0;
    ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
    uint32_t root_idx = CR_UI_SNAP_NONE;
    for (uint32_t i = 0; i < count && root_idx == CR_UI_SNAP_NONE; ++i) {
        cr_ui_node_info info{};
        char name[512];
        ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
        if (info.is_full && info.first_child != CR_UI_SNAP_NONE)
            root_idx = i;
    }
    ASSERT_NE(root_idx, CR_UI_SNAP_NONE);

    const std::string dest = cr_test::tmp_path("cr_ui", ".junksnap");
    cr_ui_recovery_config rcfg{};
    ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &u.rec), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_set_destination(u.rec, dest.c_str()), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_enqueue_subtree_snapshot(u.rec, u.snap, root_idx), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_process(u.rec), CR_UI_OK);

    EXPECT_TRUE(std::filesystem::exists(dest + "/DOC.TXT"));
    EXPECT_FALSE(std::filesystem::exists(dest + "/DESKTOP.INI"));
    EXPECT_FALSE(std::filesystem::exists(dest + "/System Volume Information"));

    std::error_code ec;
    std::filesystem::remove_all(dest, ec);
}

/* M-071: the og's filter is a results-set default, not a hard block —
 * explicitly-selected junk is still recoverable (single-node enqueue,
 * both the live and the snapshot path). */
TEST(CrUi, RecoverExplicitSelectionStillAllowsJunk) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_junk_to_session("junkexp", u, img, crsf));
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &u.snap), CR_UI_OK);

    /* the junk rows are not query rows (that is the point); find them in
     * the snapshot's value capture, which preserves every node */
    uint32_t count = 0;
    ASSERT_EQ(cr_ui_snapshot_node_count(u.snap, &count), CR_UI_OK);
    uint32_t junk_idx = CR_UI_SNAP_NONE;
    for (uint32_t i = 0; i < count && junk_idx == CR_UI_SNAP_NONE; ++i) {
        cr_ui_node_info info{};
        char name[512];
        ASSERT_EQ(cr_ui_snapshot_node(u.snap, i, &info, name, sizeof(name)), CR_UI_OK);
        if (std::strcmp(name, "DESKTOP.INI") == 0)
            junk_idx = i;
    }
    ASSERT_NE(junk_idx, CR_UI_SNAP_NONE) << "the snapshot preserves junk rows";

    const std::string dest = cr_test::tmp_path("cr_ui", ".junkexp");
    cr_ui_recovery_config rcfg{};
    ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &u.rec), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_set_destination(u.rec, dest.c_str()), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_enqueue_snapshot(u.rec, u.snap, junk_idx), CR_UI_OK);
    ASSERT_EQ(cr_ui_recovery_process(u.rec), CR_UI_OK);

    EXPECT_TRUE(std::filesystem::exists(dest + "/DESKTOP.INI"))
        << "explicit selection is not a bulk surface - the og's filter "
           "hides junk from results, it does not forbid recovering it";
    /* the bytes are the real ones (not a truncated/garbage write) */
    FILE* f = std::fopen((dest + "/DESKTOP.INI").c_str(), "rb");
    ASSERT_NE(f, nullptr);
    if (f) {
        char got[18] = {0};
        EXPECT_EQ(std::fread(got, 1, 17, f), 17u);
        EXPECT_EQ(std::memcmp(got, "[.ShellClassInfo]", 17), 0);
        std::fclose(f);
    }

    std::error_code ec;
    std::filesystem::remove_all(dest, ec);
}

TEST(CrUi, ErrorPathsAndPlatformTruth) {
    /* null / bad arguments across the surface */
    EXPECT_EQ(cr_ui_open_image(nullptr, 0, 0, nullptr), 0x50030003u);
    EXPECT_EQ(cr_ui_scan_create(nullptr, nullptr), 0x50030003u);
    EXPECT_EQ(cr_ui_scan_run(nullptr), 0x50030003u);
    EXPECT_EQ(cr_ui_session_load("", nullptr), 0x50030003u);
    EXPECT_EQ(cr_ui_query_run(nullptr, nullptr, nullptr, nullptr, nullptr), 0x50030003u);
    EXPECT_EQ(cr_ui_snapshot_node(nullptr, 0, nullptr, nullptr, 0), 0x50030003u);
    EXPECT_EQ(cr_ui_recovery_process(nullptr), 0x50030003u);
#ifndef _WIN32
    /* on hosts without the platform layer the health surface itself is
     * unsupported - parameter validation cannot precede a nonexistent
     * surface (the honest answer is ERR_UNSUPPORTED, not BAD_PARAMETER) */
    EXPECT_EQ(cr_ui_health_read(0, nullptr, nullptr, 0, nullptr), 0x50030008u);
#else
    EXPECT_EQ(cr_ui_health_read(0, nullptr, nullptr, 0, nullptr), 0x50030003u);
#endif

    /* close(NULL) is a no-op on every handle family */
    cr_ui_device_close(nullptr);
    cr_ui_scan_close(nullptr);
    cr_ui_session_close(nullptr);
    cr_ui_snapshot_close(nullptr);
    cr_ui_preview_close(nullptr);
    cr_ui_recovery_close(nullptr);
    cr_ui_query_rows_free(nullptr, 0);
    cr_ui_string_free(nullptr);

    /* nonexistent image -> ERR_OPEN_FAILED + a diagnostic */
    cr_ui_device* dev = nullptr;
    EXPECT_EQ(cr_ui_open_image("/nonexistent/cr_ui_err.img", 0, 0, &dev), 0x50020001u);
    EXPECT_EQ(dev, nullptr);
    EXPECT_NE(std::strstr(cr_ui_last_error(), "cannot open image"), nullptr);

    /* zero-stage mask is refused (a scan that runs nothing is a lie) */
    Ui u;
    const std::string img = write_fixture("err");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 8; /* only unknown bits */
    cr_ui_scan* s = nullptr;
    EXPECT_EQ(cr_ui_scan_create(&cfg, &s), 0x50030003u);
    EXPECT_EQ(s, nullptr);
    cfg.stage_mask = 7;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &s), CR_UI_OK);
    /* run without a device is a bad request */
    EXPECT_EQ(cr_ui_scan_run(s), 0x50030003u);
    /* verify_identity on a non-resumed scan is a bad request */
    EXPECT_EQ(cr_ui_scan_verify_identity(s), 0x50030003u);
    /* save before any run is a bad request (nothing to save) */
    EXPECT_EQ(cr_ui_scan_save_session(s, "/nonexistent/x.crsf"), 0x50030003u);
    cr_ui_scan_close(s);

#ifndef _WIN32
    /* platform truth: the win-backed surfaces answer ERR_UNSUPPORTED,
     * never a silent fake - and the diagnostic says why */
    cr_ui_device* d = nullptr;
    cr_ui_disk_info di;
    cr_ui_smart_report hr;
    uint32_t ac = 0;
    EXPECT_EQ(cr_ui_open_disk(0, &d), 0x50030008u);
    EXPECT_EQ(cr_ui_open_volume('C', &d), 0x50030008u);
    EXPECT_EQ(cr_ui_enumerate_disks(&di, &ac), 0x50030008u);
    EXPECT_EQ(cr_ui_enumerate_volumes(nullptr, &ac), 0x50030008u);
    EXPECT_EQ(cr_ui_health_read(0, &hr, nullptr, 0, &ac), 0x50030008u);
    EXPECT_NE(std::strstr(cr_ui_last_error(), "Windows platform layer"), nullptr);
#endif
}

/* ---- GAP-S6-10: the typed event stream ---- */

namespace {
struct EventLog {
    std::vector<cr_ui_event> events;
    uint32_t stage_transitions = 0;
    uint32_t progress_samples = 0;
    uint32_t files_events = 0;
    uint32_t checkpoints = 0;
    uint32_t repair_events = 0;
    uint32_t heartbeats = 0; /* N-043: known type (slow lanes may stall >5s) */
    uint32_t unknown_types = 0;
    bool saw_quick = false, saw_deep = false, saw_carve = false;
    void record(const cr_ui_event* e) {
        events.push_back(*e);
        switch (e->type) {
        case CR_EVT_SCAN_STAGE:
            ++stage_transitions;
            if (e->stage == 0)
                saw_quick = true;
            if (e->stage == 1)
                saw_deep = true;
            if (e->stage == 2)
                saw_carve = true;
            break;
        case CR_EVT_SCAN_PROGRESS:
            ++progress_samples;
            break;
        case CR_EVT_REPAIR: /* S-008: known type of this build */
            ++repair_events;
            break;
        case CR_EVT_HEARTBEAT: /* N-043: known type (slow lanes may stall) */
            ++heartbeats;
            break;
        case CR_EVT_FILES_FOUND:
            ++files_events;
            break;
        case CR_EVT_CHECKPOINT:
            ++checkpoints;
            break;
        default:
            if (e->type > CR_EVT_HEARTBEAT)
                ++unknown_types; /* must never happen from this build */
            break;
        }
    }
};
void event_cb(void* user, const cr_ui_event* e) {
    static_cast<EventLog*>(user)->record(e);
}
} // namespace

TEST(CrUi, TypedEventStreamFullLifecycle) {
    Ui u;
    std::string img = write_fixture("events");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    EventLog log;
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 7; /* quick + deep + carve: three stage transitions */
    const std::string ck_path = img + ".ck";
    cfg.checkpoint_path_utf8 = ck_path.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, event_cb, &log), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);

    /* stage transitions: in scan order, exactly once each */
    EXPECT_GE(log.stage_transitions, 3u);
    EXPECT_TRUE(log.saw_quick);
    EXPECT_TRUE(log.saw_deep);
    EXPECT_TRUE(log.saw_carve);
    /* stage order: quick before carve before deep - N-181's RAW-first
     * order (the og starts the carve controllers before the FS deep
     * controllers, DRWUI:916484-916509; the core runs the carve stage
     * before the deep extras whenever the engine pass already emitted
     * its nodes - the quick+deep+carve configuration here). Re-pinned
     * from quick<deep<carve at the N-181 landing (the M-140 re-pin
     * precedent: the behavioral contract changed by a corpus-verified
     * fix). */
    int quick_i = -1, deep_i = -1, carve_i = -1;
    for (size_t i = 0; i < log.events.size(); ++i) {
        if (log.events[i].type == CR_EVT_SCAN_STAGE) {
            if (log.events[i].stage == 0 && quick_i < 0)
                quick_i = int(i);
            if (log.events[i].stage == 1 && deep_i < 0)
                deep_i = int(i);
            if (log.events[i].stage == 2 && carve_i < 0)
                carve_i = int(i);
        }
    }
    EXPECT_LT(quick_i, carve_i);
    EXPECT_LT(carve_i, deep_i);
    /* progress samples: coalesced, monotonic overall permille */
    EXPECT_GE(log.progress_samples, 1u);
    uint32_t last_pm = 0;
    for (const auto& e : log.events)
        if (e.type == CR_EVT_SCAN_PROGRESS) {
            EXPECT_GE(e.overall_permille, last_pm);
            last_pm = e.overall_permille;
        }
    /* files-found: cumulative, monotonic, ended non-zero (the fixture
     * carries structured + carved files) */
    EXPECT_GE(log.files_events, 1u);
    uint64_t last_files = 0;
    for (const auto& e : log.events)
        if (e.type == CR_EVT_FILES_FOUND) {
            EXPECT_GE(e.files_found, last_files);
            last_files = e.files_found;
        }
    EXPECT_GT(last_files, 0u);
    /* checkpoints: the G-13 cadence fired (3 stage boundaries) */
    EXPECT_GE(log.checkpoints, 1u);
    /* no unknown event types from this build */
    EXPECT_EQ(log.unknown_types, 0u);
    /* the payload is POD-serializable: no self-referencing pointers by
     * construction (compile-time shape; sanity: sizes stable) */
    EXPECT_EQ(sizeof(cr_ui_event) % 8u, 0u);

    /* subscribing WHILE run() executes is refused (the mid-run swap could
     * double-deliver or drop events) - proven with the pause gate: pause
     * the token before run, run on a worker thread, the scan parks in the
     * cooperative gate with running=true, the subscribe is refused, then
     * resume lets the scan finish cleanly */
    {
        Ui u2;
        std::string img2 = write_fixture("events_mid");
        ASSERT_FALSE(img2.empty());
        ASSERT_EQ(cr_ui_open_image(img2.c_str(), 0, 0, &u2.dev), CR_UI_OK);
        cr_ui_scan_config cfg2{};
        ASSERT_EQ(cr_ui_scan_create(&cfg2, &u2.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u2.scan, u2.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_pause(u2.scan), CR_UI_OK); /* park at the gate */
        auto fut = std::async(std::launch::async, [&u2] { return cr_ui_scan_run(u2.scan); });
        /* wait for the running flag via telemetry (bounded; the pause
         * parks the scan inside the first engine gate) */
        cr_ui_telemetry t;
        for (int i = 0; i < 3000; ++i) {
            if (cr_ui_scan_telemetry(u2.scan, &t) == CR_UI_OK && t.running)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        EXPECT_EQ(t.running, 1u) << "the scan must be parked RUNNING in the pause gate";
        /* the pause parks the scan with running=true -> refusal expected */
        EXPECT_EQ(cr_ui_scan_set_event_fn(u2.scan, event_cb, &log),
                  0x50030003u /* ERR_BAD_PARAMETER */);
        ASSERT_EQ(cr_ui_scan_resume(u2.scan), CR_UI_OK);
        ASSERT_EQ(fut.get(), CR_UI_OK);
    }
}

TEST(CrUi, EventStreamUnsubscribeAndResubscribe) {
    Ui u;
    std::string img = write_fixture("events2");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    EventLog log;
    cr_ui_scan_config cfg{};
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    /* subscribe then unsubscribe before run: zero events must arrive */
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, event_cb, &log), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, nullptr, nullptr), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    EXPECT_EQ(log.events.size(), 0u) << "unsubscribed scans must emit nothing";
}

/* ---- N-043: the liveness heartbeat watchdog ---- */

namespace {
struct HeartbeatLog {
    std::mutex mu;
    std::vector<cr_ui_event> events;
    std::vector<std::thread::id> threads;
    void record(const cr_ui_event* e) {
        std::lock_guard<std::mutex> lock(mu);
        events.push_back(*e);
        threads.push_back(std::this_thread::get_id());
    }
};
void hb_event_cb(void* user, const cr_ui_event* e) {
    static_cast<HeartbeatLog*>(user)->record(e);
}

/* N-062: the cr::log sink recorder (the memory monitor rides the seam) */
struct MemoryLog {
    std::mutex mu;
    std::vector<std::string> lines;
    static void sink(cr::log::Level lvl, const char* line, void* user) {
        auto* m = static_cast<MemoryLog*>(user);
        std::lock_guard<std::mutex> lock(m->mu);
        if (lvl == cr::log::Level::Warn)
            m->lines.push_back(line);
    }
    size_t count(const char* needle) const {
        size_t n = 0;
        for (const auto& l : lines)
            if (l.find(needle) != std::string::npos)
                ++n;
        return n;
    }
};
} // namespace

TEST(CrUi, HeartbeatDuringStall) {
    /* N-043 (the finding's fix-plan option (b), the honest heartbeat): a
     * pause-parked scan is a DETERMINISTIC stall - no CR_EVT_SCAN_PROGRESS
     * fires while parked, run() is in flight - so the 5 s watchdog (the
     * og's FUN_140577100 cadence, DRWUI_decompiled.c:910565-910585) must
     * fire CR_EVT_HEARTBEAT from its OWN thread. The honest divergence:
     * the og's watchdog bumps its coarse progress counter (+0xe8) and
     * re-fires the progress callback - FABRICATED movement; ours must
     * carry the STANDING permille (zero synthetic advance) so the full
     * log incl. heartbeats stays strictly monotonic (the finding's
     * Progress.HeartbeatDuringStall + PermilleStrictlyMonotonicUnder-
     * Heartbeat pair, proven on one deterministic stall fixture - two
     * 6+ second stalls would double the suite for no extra coverage). */
    Ui u;
    std::string img = write_fixture("hb");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    HeartbeatLog log;
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 7;
    /* N-062: the memory monitor rides the same watchdog cadence - a
     * synthetic 1 MiB threshold is always exceeded, so the parked stall
     * window must also produce the Warn pressure line through the
     * cr::log seam (one parked window proves both the heartbeat and
     * the monitor) */
    cfg.memory_warn_mb = 1;
    MemoryLog mlog;
    cr::log::set_sink(&MemoryLog::sink, &mlog);
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, hb_event_cb, &log), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_pause(u.scan), CR_UI_OK); /* park at the first gate */
    auto fut = std::async(std::launch::async, [&u] { return cr_ui_scan_run(u.scan); });
    cr_ui_telemetry t{};
    for (int i = 0; i < 3000; ++i) {
        if (cr_ui_scan_telemetry(u.scan, &t) == CR_UI_OK && t.running)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ASSERT_EQ(t.running, 1u) << "the scan must park RUNNING inside the pause gate";

    /* the stall window: poll for the watchdog's first beat (bounded at
     * 12 s; the cadence is 5 s from the last progress - the park fires
     * none, so the clock runs from run start) */
    bool saw_hb = false;
    uint32_t hb_pm = 0, last_progress_pm = 0;
    bool saw_progress = false;
    std::thread::id hb_thread{};
    for (int i = 0; i < 12000 && !saw_hb; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::lock_guard<std::mutex> lock(log.mu);
        for (size_t k = 0; k < log.events.size(); ++k) {
            if (log.events[k].type == CR_EVT_HEARTBEAT) {
                saw_hb = true;
                hb_pm = log.events[k].overall_permille;
                hb_thread = log.threads[k];
            } else if (log.events[k].type == CR_EVT_SCAN_PROGRESS) {
                saw_progress = true;
                last_progress_pm = log.events[k].overall_permille;
            }
        }
    }
    /* EXPECT (not ASSERT): a failure must NOT return early here - the
     * parked scan + the async run thread below MUST be released or the
     * fixture teardown deadlocks against the pause gate */
    EXPECT_TRUE(saw_hb) << "the watchdog must beat during a >5s stall";
    /* the standing truth, never a synthetic advance (the og's +1 bump is
     * the registered divergence this pin forbids) */
    EXPECT_EQ(hb_pm, saw_progress ? last_progress_pm : 0u)
        << "heartbeats must carry the standing permille - no fabricated movement";
    /* from the watchdog's OWN thread (not the test thread; the scan runs
     * on the async thread) */
    if (saw_hb) {
        EXPECT_NE(hb_thread, std::this_thread::get_id());
    }

    /* N-062: the monitor fired on the watchdog cadence during the same
     * parked window (every 5 s tick, at/above the 1 MiB threshold) */
    EXPECT_GE(mlog.count("memory pressure: working_set="), 1u)
        << "the memory monitor must ride the watchdog cadence";
    /* detach the log sink BEFORE the recorders die */
    cr::log::set_sink(nullptr, nullptr);

    /* always resume and complete cleanly with the watchdog joined */
    ASSERT_EQ(cr_ui_scan_resume(u.scan), CR_UI_OK);
    ASSERT_EQ(fut.get(), CR_UI_OK);

    /* full-log monotonicity INCLUDING heartbeats (progress + heartbeat
     * events never regress overall_permille) */
    uint32_t last = 0;
    std::lock_guard<std::mutex> lock(log.mu);
    for (const auto& e : log.events) {
        if (e.type == CR_EVT_SCAN_PROGRESS || e.type == CR_EVT_HEARTBEAT) {
            EXPECT_GE(e.overall_permille, last);
            last = e.overall_permille;
        }
    }
}

/* ---- N-060: save-path progress + atomic cancel ---- */

namespace {
std::string read_all_bytes(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}
} // namespace

namespace {
struct SaveEvtLog {
    cr_ui_scan* scan = nullptr;
    bool cancel_enabled = false; /* part 2 only: the success path first */
    uint32_t save_events = 0;
    uint32_t last_pm = 0xFFFFFFFFu;
    std::vector<uint32_t> pms;
    bool overall_always_standing_1000 = true;
    void record(const cr_ui_event* e) {
        if (e->type != CR_EVT_SAVE_PROGRESS)
            return;
        ++save_events;
        pms.push_back(e->stage_permille);
        if (e->overall_permille != 1000u)
            overall_always_standing_1000 = false;
        /* cancel the save from the FIRST save-progress event - the
         * og's per-child IsCancelled teardown (the save-scoped flag,
         * NOT the scan token) */
        if (cancel_enabled && save_events == 1)
            cr_ui_scan_save_cancel(scan);
    }
};
void save_evt_cb(void* user, const cr_ui_event* e) {
    static_cast<SaveEvtLog*>(user)->record(e);
}
} // namespace

TEST(CrUi, SaveProgressEventsAndAtomicCancel) {
    /* N-060 end-to-end: the save emits coalesced CR_EVT_SAVE_PROGRESS
     * (first tick + permille steps; overall stays the standing 1000 -
     * the scan is complete); cr_ui_scan_save_cancel DURING the save (the
     * og's IUIProgressInfo::IsCancelled - the save progress object's own
     * state, NOT the scan flag: save-on-scan-cancel stays intact) aborts
     * with CR_UI_ERR_CANCELLED; the atomic temp-then-rename keeps the
     * FINAL path byte-identical to the previous good session and leaves
     * no staging .tmp. */
    Ui u;
    std::string img = write_fixture("savecancel");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 7;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);

    namespace fs = std::filesystem;
    const fs::path dir =
        fs::temp_directory_path() / (std::string("cr_n060_") + std::to_string(CR_TEST_GETPID()));
    std::error_code ec;
    fs::remove_all(dir, ec);
    fs::create_directories(dir);
    const fs::path final_path = dir / "session.crsf";
    const fs::path tmp_path = dir / "session.crsf.tmp";

    /* 1. the success path commits atomically: final exists, no .tmp */
    SaveEvtLog log;
    log.scan = u.scan;
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, save_evt_cb, &log), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, final_path.string().c_str()), CR_UI_OK);
    EXPECT_TRUE(fs::exists(final_path, ec));
    EXPECT_FALSE(fs::exists(tmp_path, ec)) << "no staging file after a committed save";
    EXPECT_GE(log.save_events, 1u) << "the save must emit progress events";
    EXPECT_TRUE(log.overall_always_standing_1000)
        << "save progress never touches the completed scan's standing permille";
    uint32_t last = 0;
    for (uint32_t pm : log.pms) {
        EXPECT_GE(pm, last) << "save stage_permille is monotonic";
        last = pm;
    }
    /* the committed file's bytes are the baseline the cancel must keep */
    const std::string good = read_all_bytes(final_path);
    EXPECT_FALSE(good.empty());

    /* 2. the cancel path: aborting mid-save keeps the previous good file */
    log.save_events = 0;
    log.pms.clear();
    log.cancel_enabled = true; /* arm the cancel for this save */
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, final_path.string().c_str()),
              0x50020005u /* CR_UI_ERR_CANCELLED */);
    EXPECT_GE(log.save_events, 1u) << "the cancel rode a save-progress event";
    EXPECT_EQ(read_all_bytes(final_path), good)
        << "an aborted save leaves the previous good session byte-identical";
    EXPECT_FALSE(fs::exists(tmp_path, ec)) << "no staging file after an aborted save";
    /* the save cancel is save-scoped: the SCAN token was never touched */
    cr_ui_telemetry t{};
    ASSERT_EQ(cr_ui_scan_telemetry(u.scan, &t), CR_UI_OK);
    EXPECT_EQ(t.cancelled, 0u) << "save cancel never cancels the scan";

    fs::remove_all(dir, ec);
}

/* ---- N-040: ABI doc-truthfulness pins ---- */

namespace {
struct ThreadLog {
    std::atomic<uint32_t> events{0};
    std::atomic<bool> saw_non_run_thread{false};
    std::thread::id run_thread{};
    void record(const cr_ui_event*) {
        events.fetch_add(1, std::memory_order_relaxed);
        if (std::this_thread::get_id() != run_thread)
            saw_non_run_thread.store(true, std::memory_order_relaxed);
    }
};
void thread_cb(void* user, const cr_ui_event* e) {
    static_cast<ThreadLog*>(user)->record(e);
}
} // namespace

TEST(CrUi, EventThreadContractPinned) {
    /* N-040 D3: the header's threading truth is "events may fire from
     * ENGINE WORKER THREADS" (cr_ui.h contract block; the trampoline at
     * cr_ui.cpp performs direct calls with no marshaling - documented in
     * both files). The old ABI-CONTRACT claim "Fired on the thread
     * running run() only" was FALSE. This pin keeps the doc, the header
     * and the behavior in lockstep:
     *  (a) doc-truth: the contract line names engine worker threads and
     *      the false run()-thread-only claim is gone;
     *  (b) behavior: serial scans (threads=1) deliver on the run()
     *      thread - deterministic; parallel scans tolerate (and in
     *      practice observe) worker-thread delivery - the callback
     *      records it without any host-side marshaling requirement. */
    const std::string doc = cr_test::read_repo_text("docs/ABI-CONTRACT.md");
    ASSERT_FALSE(doc.empty());
    EXPECT_NE(doc.find("Fired on engine worker threads, the thread running `run()`, or the"),
              std::string::npos)
        << "ABI-CONTRACT must carry the truthful thread contract (the N-043 "
           "heartbeat watchdog is the third delivery thread)";
    EXPECT_EQ(doc.find("Fired on the thread running `run()` only"), std::string::npos)
        << "the false run()-thread-only claim must never return";
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_NE(hdr.find("events may fire from ENGINE WORKER"), std::string::npos)
        << "the header contract block must keep the worker-thread truth";

    /* (b) serial: every delivery on the run() thread (deterministic - the
     * serial scan paths run on the caller; the pin catches an accidental
     * async emission that would break single-threaded hosts) */
    {
        Ui u;
        std::string img = write_fixture("evthread_serial");
        ASSERT_FALSE(img.empty());
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        ThreadLog log;
        cr_ui_scan_config cfg{};
        cfg.threads = 1;
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, thread_cb, &log), CR_UI_OK);
        log.run_thread = std::this_thread::get_id();
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        EXPECT_GE(log.events.load(), 1u);
        EXPECT_FALSE(log.saw_non_run_thread.load())
            << "serial scans must deliver every event on the run() thread";
    }
    /* (b) parallel: worker-thread delivery is ALLOWED (the contract's
     * allowance; observed in practice whenever the pool wins a window -
     * not asserted as a requirement, only tolerated and recorded) */
    {
        Ui u;
        std::string img = write_fixture("evthread_par");
        ASSERT_FALSE(img.empty());
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        ThreadLog log;
        cr_ui_scan_config cfg{};
        cfg.threads = 3;
        cfg.stage_mask = 7;
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, thread_cb, &log), CR_UI_OK);
        log.run_thread = std::this_thread::get_id();
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        EXPECT_GE(log.events.load(), 1u);
        /* worker delivery observed is legal; not required (pool scheduling
         * is not a contract) - the run must simply complete cleanly with
         * the recording callback active from any thread */
    }
}

TEST(CrUi, SessionSaveWritesCrsfOnly) {
    /* N-040 D1 pin: the save flow writes EXACTLY ONE file - the .crsf.
     * The SQLite sidecar (<session>.crsidx) documented pre-fix never
     * existed (ABI-CONTRACT section 8 now records the
     * REJECTED-BY-REFERENCE closure); this pin guarantees no sidecar
     * file ever appears next to a saved session. */
    Ui u;
    std::string img = write_fixture("saveonly");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 7;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    namespace fs = std::filesystem;
    const fs::path dir =
        fs::temp_directory_path() / (std::string("cr_n040_") + std::to_string(CR_TEST_GETPID()));
    fs::create_directories(dir);
    const fs::path crsf = dir / "session.crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.string().c_str()), CR_UI_OK);
    std::error_code ec;
    size_t files = 0;
    for (auto it = fs::directory_iterator(dir, ec); it != fs::directory_iterator(); ++it) {
        (void)it->is_regular_file(ec);
        ++files;
    }
    EXPECT_EQ(files, 1u) << "the save flow must write exactly the .crsf - no sidecar";
    EXPECT_TRUE(fs::exists(crsf, ec));
    EXPECT_FALSE(fs::exists(dir / "session.crsidx", ec))
        << "the rejected SQLite sidecar must never appear";
    fs::remove_all(dir, ec);
}

TEST(CrUi, AbiVersionAnnotationsAreTruthful) {
    /* N-040 D4 pin: no "ABI-since 1.1/1.2" fake-version annotations may
     * return to the header - no v1.1 or v1.2 was ever shippable (the
     * macro read 1 until the deliberate v2 feature-batch bump, M-036).
     * Appended fields are labeled with their landing (change id + era)
     * per ABI-CONTRACT section 9. */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("ABI-since 1.1"), std::string::npos)
        << "no v1.1 marker ever shipped - the label must not return";
    EXPECT_EQ(hdr.find("ABI-since 1.2"), std::string::npos)
        << "no v1.2 marker ever shipped - the label must not return";
    const std::string doc = cr_test::read_repo_text("docs/ABI-CONTRACT.md");
    ASSERT_FALSE(doc.empty());
    EXPECT_NE(doc.find("no v1.1 or v1.2"), std::string::npos)
        << "section 9 must keep the truthful sub-version history";
}

/* ---- GAP-S6-11: the disk-map data producer ---- */

TEST(CrUi, DiskMapSessionGeometryAndOverlays) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("dmap", u, img, crsf));
    /* the session carried a full scan (carve completed): coverage must be
     * all-1, the geometry must report the session view, and the found /
     * confidence / allocation overlays must reflect real extents. */
    cr_ui_disk_map_geometry want{};
    cr_ui_disk_map_geometry got{};
    uint8_t *alloc = nullptr, *cov = nullptr, *fnd = nullptr, *conf = nullptr;
    uint32_t bad_count = 0;
    ASSERT_EQ(cr_ui_disk_map(u.sess, nullptr, &want, &got, &alloc, &cov, &fnd, &conf, nullptr, 0,
                             &bad_count),
              CR_UI_OK);
    ASSERT_NE(alloc, nullptr);
    ASSERT_NE(cov, nullptr);
    ASSERT_NE(fnd, nullptr);
    ASSERT_NE(conf, nullptr);
    EXPECT_GT(got.cell_count, 0u);
    /* S-009: the session's free-map record is the cluster hint, so the
     * grid is cluster-granular (512 on this fixture) - not the 64 KiB
     * default that pre-record sessions degrade to. */
    EXPECT_GE(got.cell_bytes, 512u);
    EXPECT_EQ(got.cluster_bytes, 512u);
    /* coverage: the fixture scan completed carve -> every cell covered */
    bool all_cov = true;
    for (uint64_t i = 0; i < got.cell_count; ++i)
        if (cov[i] == 0)
            all_cov = false;
    EXPECT_TRUE(all_cov) << "carve completion must mark the whole view covered";
    /* found: at least one cell with content (the fixture files) */
    bool any_found = false;
    for (uint64_t i = 0; i < got.cell_count; ++i)
        if (fnd[i] > 0)
            any_found = true;
    EXPECT_TRUE(any_found) << "the fixture's file extents must appear in the found overlay";
    /* confidence: max ordinal of covering content is in 0..4 */
    for (uint64_t i = 0; i < got.cell_count; ++i)
        EXPECT_LE(conf[i], 4u);
    /* allocation values are the documented enum */
    for (uint64_t i = 0; i < got.cell_count; ++i)
        EXPECT_LE(alloc[i], 3u);
    cr_ui_disk_map_free(alloc);
    cr_ui_disk_map_free(cov);
    cr_ui_disk_map_free(fnd);
    cr_ui_disk_map_free(conf);

    /* exactly-one-of rule + overlay requirement */
    EXPECT_NE(cr_ui_disk_map(u.sess, nullptr, &want, &got, nullptr, nullptr, nullptr, nullptr,
                             nullptr, 0, &bad_count),
              CR_UI_OK);
    /* want view outside the session slice is refused (fresh out pointer) */
    cr_ui_disk_map_geometry bad_want{};
    bad_want.start_byte = 1ull << 48;
    bad_want.total_bytes = 4096;
    uint8_t* fnd2 = nullptr;
    EXPECT_NE(cr_ui_disk_map(u.sess, nullptr, &bad_want, &got, &fnd2, nullptr, nullptr, nullptr,
                             nullptr, 0, &bad_count),
              CR_UI_OK);
    EXPECT_EQ(fnd2, nullptr) << "refused calls must not allocate overlays";
    cr_ui_disk_map_free(fnd2);
}

TEST(CrUi, DiskMapSnapshotRequiresViewGeometry) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("dmap2", u, img, crsf));
    cr_ui_snapshot* snap = nullptr;
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &snap), CR_UI_OK);
    /* snapshots carry no view: want->start/total are REQUIRED */
    cr_ui_disk_map_geometry want{};
    cr_ui_disk_map_geometry got{};
    uint8_t* fnd = nullptr;
    uint32_t bad_count = 0;
    EXPECT_NE(cr_ui_disk_map(nullptr, snap, &want, &got, &fnd, nullptr, nullptr, nullptr, nullptr,
                             0, &bad_count),
              CR_UI_OK);
    /* with the view supplied the map builds; the image is the whole view */
    want.start_byte = 0;
    want.total_bytes = 512ull * 5074; /* the FAT16 fixture size */
    ASSERT_EQ(cr_ui_disk_map(nullptr, snap, &want, &got, &fnd, nullptr, nullptr, nullptr, nullptr,
                             0, &bad_count),
              CR_UI_OK);
    ASSERT_NE(fnd, nullptr);
    EXPECT_GT(got.cell_count, 0u);
    cr_ui_disk_map_free(fnd);
    cr_ui_snapshot_close(snap);
}

/* ---- S-009: the free overlay (the engine bitmap truth) ---- */

TEST(CrUi, DiskMapFreeOverlayFromFatTable) {
    /* The FAT IS the allocation bitmap: the fixture's table marks
     * clusters 5,10,11,12,20,30,31 in use; everything else is entry 0
     * = free. After a full scan + session round-trip, the disk map must
     * paint FREE cells where the table says free, USED where the active
     * structured file lives (DOC.TXT, chain 10-12), and leave UNKNOWN
     * where neither claim applies (the deleted file's chain 20: its FAT
     * entry is non-zero so it is not free, and deleted content never
     * marks USED - the honest neither-state). */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("dmap_free", u, img, crsf));
    cr_ui_disk_map_geometry want{};
    cr_ui_disk_map_geometry got{};
    uint8_t* alloc = nullptr;
    uint32_t bad_count = 0;
    ASSERT_EQ(cr_ui_disk_map(u.sess, nullptr, &want, &got, &alloc, nullptr, nullptr, nullptr,
                             nullptr, 0, &bad_count),
              CR_UI_OK);
    ASSERT_NE(alloc, nullptr);
    /* the session's free-map record is the cluster hint: 512-byte cells */
    ASSERT_EQ(got.cluster_bytes, 512u);
    ASSERT_EQ(got.cell_bytes, 512u);
    /* data base = sector 74; cluster c lives at cell 72 + c */
    const unsigned cell_doc = 72 + 10;   /* DOC.TXT first cluster */
    const unsigned cell_del = 72 + 20;   /* DELFILE.TXT (deleted) */
    const unsigned cell_free = 72 + 200; /* far-away free cluster */
    ASSERT_LT(cell_free, unsigned(got.cell_count));
    EXPECT_EQ(alloc[cell_doc], 1u) << "active structured content is USED, never free";
    EXPECT_EQ(alloc[cell_del], 0u) << "deleted chain: FAT non-zero (not free), deleted (not USED)";
    EXPECT_EQ(alloc[cell_free], 2u) << "bitmap-free cluster with no content paints FREE";
    /* the overwhelming majority of the fixture is free */
    uint64_t free_cells = 0;
    for (uint64_t i = 0; i < got.cell_count; ++i)
        if (alloc[i] == 2u)
            ++free_cells;
    EXPECT_GT(free_cells, 4500u) << "the fixture has ~5000 free clusters";
    cr_ui_disk_map_free(alloc);
}

TEST(CrUi, DiskMapFreeCellsSurviveSessionReloadAndSnapshot) {
    /* The same session through BOTH read paths: the loaded session
     * (REC_ALLOC_MAP round-trip) and the value snapshot captured from
     * it. Both must report the same free population - the record's
     * downsampled granularity rebuilds the identical grid. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("dmap_rt", u, img, crsf));
    uint64_t free_via_session = 0;
    {
        cr_ui_disk_map_geometry want{}, got{};
        uint8_t* alloc = nullptr;
        uint32_t bad_count = 0;
        ASSERT_EQ(cr_ui_disk_map(u.sess, nullptr, &want, &got, &alloc, nullptr, nullptr, nullptr,
                                 nullptr, 0, &bad_count),
                  CR_UI_OK);
        for (uint64_t i = 0; i < got.cell_count; ++i)
            if (alloc[i] == 2u)
                ++free_via_session;
        cr_ui_disk_map_free(alloc);
        ASSERT_GT(free_via_session, 4500u);
    }
    cr_ui_snapshot* snap = nullptr;
    ASSERT_EQ(cr_ui_session_snapshot(u.sess, &snap), CR_UI_OK);
    cr_ui_disk_map_geometry want{}, got{};
    uint8_t* alloc = nullptr;
    uint32_t bad_count = 0;
    want.start_byte = 0;
    want.total_bytes = 512ull * 5074;
    ASSERT_EQ(cr_ui_disk_map(nullptr, snap, &want, &got, &alloc, nullptr, nullptr, nullptr, nullptr,
                             0, &bad_count),
              CR_UI_OK);
    uint64_t free_via_snapshot = 0;
    for (uint64_t i = 0; i < got.cell_count; ++i)
        if (alloc[i] == 2u)
            ++free_via_snapshot;
    EXPECT_EQ(free_via_snapshot, free_via_session)
        << "the snapshot must carry the same free-map truth as the session";
    /* the captured slice bounds the snapshot's view */
    cr_ui_disk_map_geometry oob{}, got2{};
    uint8_t* f2 = nullptr;
    oob.start_byte = 512ull * 5074;
    oob.total_bytes = 4096;
    EXPECT_EQ(cr_ui_disk_map(nullptr, snap, &oob, &got2, &f2, nullptr, nullptr, nullptr, nullptr, 0,
                             &bad_count),
              0x50020004u /* ERR_RANGE_OUT_OF_BOUNDS */);
    EXPECT_EQ(f2, nullptr);
    cr_ui_disk_map_free(alloc);
    cr_ui_disk_map_free(f2);
    cr_ui_snapshot_close(snap);
}

TEST(CrUi, DiskMapScanSnapshotCarriesFreeMap) {
    /* A completed scan's value snapshot carries the engine free map too
     * (the !running capture path): the mid-scan surface paints free
     * cells without ever loading the session file. */
    Ui u;
    const std::string img = write_fixture("dmap_ss");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    cr_ui_snapshot* snap = nullptr;
    ASSERT_EQ(cr_ui_scan_snapshot(u.scan, &snap), CR_UI_OK);
    cr_ui_disk_map_geometry want{}, got{};
    uint8_t* alloc = nullptr;
    uint32_t bad_count = 0;
    want.start_byte = 0;
    want.total_bytes = 512ull * 5074;
    ASSERT_EQ(cr_ui_disk_map(nullptr, snap, &want, &got, &alloc, nullptr, nullptr, nullptr, nullptr,
                             0, &bad_count),
              CR_UI_OK);
    ASSERT_NE(alloc, nullptr);
    EXPECT_EQ(got.cluster_bytes, 512u) << "the free map is the cluster hint on this path too";
    uint64_t free_cells = 0;
    for (uint64_t i = 0; i < got.cell_count; ++i)
        if (alloc[i] == 2u)
            ++free_cells;
    EXPECT_GT(free_cells, 4500u);
    cr_ui_disk_map_free(alloc);
    cr_ui_snapshot_close(snap);
}

/* ---- S-008: pre-scan repair through the C ABI (image device) ---- */

namespace {

unsigned g_consent_calls = 0;
cr_ui_repair_plan g_consent_plan{};
uint32_t g_consent_reply = 1; /* consent by default */

uint32_t repair_consent_cb(void* user, const cr_ui_repair_plan* plan) {
    ++*static_cast<unsigned*>(user);
    g_consent_plan = *plan;
    return g_consent_reply;
}

std::string write_torn_fat_fixture(const char* tag) {
    const std::string path = cr_test::tmp_path("cr_ui", tag);
    auto img = build_fat16();
    /* FAT16 entry signatures on both copies... */
    for (size_t fat : {kFAT1, kFAT2}) {
        img[fat + 0] = 0xF8;
        img[fat + 1] = 0xFF;
        img[fat + 2] = 0xFF;
        img[fat + 3] = 0xFF;
    }
    /* ...then tear FAT1's signature sector (the finding's damaged-FAT1 case) */
    std::memset(img.data() + kFAT1, 0xAB, 512);
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return {};
    const size_t n = std::fwrite(img.data(), 1, img.size(), f);
    std::fclose(f);
    return n == img.size() ? path : std::string();
}

} // namespace

TEST(CrUi, PreScanRepairConsentGateAndUndoLog) {
    /* The full C-ABI lifecycle: the shim analyzes pre-scan, surfaces the
     * plan through the consent callback, executes the transactional
     * repair into the IMAGE FILE after consent, reports it through the
     * typed event stream + stats, and persists the undo log. */
    Ui u;
    std::string img = write_torn_fat_fixture("repair");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    EventLog log;
    g_consent_calls = 0;
    g_consent_reply = 1;
    std::memset(&g_consent_plan, 0, sizeof(g_consent_plan));
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 1; /* quick only */
    cfg.pre_scan_repair = 1;
    cfg.repair_consent = repair_consent_cb;
    cfg.repair_consent_user = &g_consent_calls;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_set_event_fn(u.scan, event_cb, &log), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);

    /* the consent saw the plan (family, one action, not refused) */
    ASSERT_EQ(g_consent_calls, 1u);
    EXPECT_EQ(g_consent_plan.detected, 1u);
    EXPECT_EQ(std::string(g_consent_plan.fs_family), "fat");
    EXPECT_EQ(g_consent_plan.action_count, 1u);
    EXPECT_EQ(g_consent_plan.refused, 0u);
    EXPECT_NE(std::string(g_consent_plan.reason).find("FAT"), std::string::npos);

    /* stats carry the outcome */
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_EQ(st.repair_ran, 1u);
    EXPECT_EQ(st.repair_planned, 1u);
    EXPECT_EQ(st.repair_applied, 1u);
    EXPECT_EQ(st.repair_denied, 0u);

    /* the typed event stream saw ready -> applied */
    int ready = -1, applied = -1;
    for (size_t i = 0; i < log.events.size(); ++i) {
        if (log.events[i].type == CR_EVT_REPAIR) {
            if (log.events[i].repair_outcome == 0 && ready < 0)
                ready = int(i);
            if (log.events[i].repair_outcome == 1 && applied < 0)
                applied = int(i);
        }
    }
    ASSERT_GE(ready, 0);
    ASSERT_GE(applied, 0);
    EXPECT_LT(ready, applied);
    EXPECT_EQ(log.events[size_t(ready)].repair_actions, 1u);
    EXPECT_EQ(log.events[size_t(applied)].repair_actions, 1u);
    EXPECT_EQ(log.unknown_types, 0u);

    /* the repair LANDED in the image file (the write side of the device) */
    {
        FILE* f = std::fopen(img.c_str(), "rb");
        ASSERT_NE(f, nullptr);
        unsigned char fat1[4] = {};
        ASSERT_EQ(std::fseek(f, long(kFAT1), 0), 0);
        ASSERT_EQ(std::fread(fat1, 1, 4, f), 4u);
        std::fclose(f);
        EXPECT_EQ(fat1[0], 0xF8);
        EXPECT_EQ(fat1[1], 0xFF);
    }

    /* the undo log persists and is a valid container */
    const std::string undo_path = img + ".undo";
    ASSERT_EQ(cr_ui_scan_repair_save_undo(u.scan, undo_path.c_str()), CR_UI_OK);
    FILE* f = std::fopen(undo_path.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    unsigned char magic[8] = {};
    ASSERT_EQ(std::fread(magic, 1, 8, f), 8u);
    std::fclose(f);
    EXPECT_EQ(0, std::memcmp(magic, "CRUNDO01", 8));

    /* nothing applied -> save_undo refuses truthfully */
    {
        Ui u2;
        std::string img2 = write_fixture("repair_clean");
        ASSERT_FALSE(img2.empty());
        /* clean fixture: no FAT signature -> both copies damaged -> no
         * action; the undo-save contract must refuse */
        ASSERT_EQ(cr_ui_open_image(img2.c_str(), 0, 0, &u2.dev), CR_UI_OK);
        cr_ui_scan_config cfg2{};
        cfg2.stage_mask = 1;
        ASSERT_EQ(cr_ui_scan_create(&cfg2, &u2.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u2.scan, u2.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u2.scan), CR_UI_OK);
        cr_ui_scan_stats st2{};
        ASSERT_EQ(cr_ui_scan_get_stats(u2.scan, &st2), CR_UI_OK);
        EXPECT_EQ(st2.repair_ran, 0u); /* pre_scan_repair off: nothing ran */
        EXPECT_EQ(cr_ui_scan_repair_save_undo(u2.scan, (img2 + ".u").c_str()),
                  0x50030003u /* ERR_BAD_PARAMETER */);
    }
}

TEST(CrUi, PreScanRepairDeniedLeavesFileUntouched) {
    Ui u;
    std::string img = write_torn_fat_fixture("repair_deny");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    g_consent_calls = 0;
    g_consent_reply = 0; /* deny */
    cr_ui_scan_config cfg{};
    cfg.stage_mask = 1;
    cfg.pre_scan_repair = 1;
    cfg.repair_consent = repair_consent_cb;
    cfg.repair_consent_user = &g_consent_calls;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    ASSERT_EQ(g_consent_calls, 1u);
    cr_ui_scan_stats st{};
    ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
    EXPECT_EQ(st.repair_ran, 1u);
    EXPECT_EQ(st.repair_planned, 1u);
    EXPECT_EQ(st.repair_applied, 0u);
    EXPECT_EQ(st.repair_denied, 1u);
    /* the image file is byte-identical to the torn fixture (no write) */
    {
        FILE* f = std::fopen(img.c_str(), "rb");
        ASSERT_NE(f, nullptr);
        unsigned char fat1[4] = {};
        ASSERT_EQ(std::fseek(f, long(kFAT1), 0), 0);
        ASSERT_EQ(std::fread(fat1, 1, 4, f), 4u);
        std::fclose(f);
        EXPECT_EQ(fat1[0], 0xAB);
        EXPECT_EQ(fat1[1], 0xAB);
    }
    EXPECT_EQ(cr_ui_scan_repair_save_undo(u.scan, (img + ".u").c_str()),
              0x50030003u /* nothing applied */);
}

TEST(CrUi, ImageSourceDestinationNearImageAccepted) {
    /* N-013 sub-claim refutation pin: the finding expected
     * ImageSourceToSameDiskRefused; REFUTED first-hand - the image file is
     * filesystem-protected (destination writes never touch its allocated
     * extents; source reads go through the file handle, never raw \\.\X:),
     * and the og's same-volume detection guards LIVE volume sources with
     * a UI *recommendation* ("We highly recommend saving the found data to
     * a different drive" - corpus architecture/04 §7 step 1). Recovering
     * from an image to a folder on the SAME volume (the standard flow)
     * must be accepted; over-refusal would break every nearby-destination
     * recovery. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("imgsrc", u, img, crsf));
    cr_ui_recovery_config rcfg{};
    ASSERT_EQ(cr_ui_recovery_create(u.dev, &rcfg, &u.rec), CR_UI_OK);
    /* same tmp area as the image -> same volume by construction */
    const std::string dest = cr_test::tmp_path("cr_ui", ".dest-near");
    EXPECT_EQ(cr_ui_recovery_set_destination(u.rec, dest.c_str()), CR_UI_OK);
}

/* ---- N-014: atomic checkpoint staging + commit-by-rename ------------ */

TEST(CrUi, CheckpointAtomicStagingAndCommit) {
    /* A full scan with a checkpoint path: the final path must hold the
     * LAST COMMITTED checkpoint (a valid, loadable session container)
     * and the .tmp staging file must be cleaned up (N-014's cleanup
     * contract). Pre-fix the file was opened "wb" on the FINAL path and
     * patched in place - torn on crash. */
    const std::string img = write_fixture("ckatomic");
    ASSERT_FALSE(img.empty());
    const std::string ck = cr_test::tmp_path("ckatomic", ".rsf");
    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.checkpoint_path_utf8 = ck.c_str();
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    /* the final path holds a loadable session container */
    FILE* f = std::fopen(ck.c_str(), "rb");
    ASSERT_NE(f, nullptr) << "the committed checkpoint must exist at the final path";
    std::vector<unsigned char> first(std::fseek(f, 0, SEEK_END) ? 0 : size_t(std::ftell(f)));
    std::rewind(f);
    ASSERT_EQ(std::fread(first.data(), 1, first.size(), f), first.size());
    std::fclose(f);
    ASSERT_GE(first.size(), size_t(16)) << "a finalized container is never empty";
    /* the staging file is cleaned up after the scan */
    EXPECT_EQ(std::fopen((ck + ".tmp").c_str(), "rb"), nullptr)
        << "the .tmp staging file must not survive the scan";
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    /* and the container loads as a session */
    ASSERT_EQ(cr_ui_session_load(ck.c_str(), &u.sess), CR_UI_OK);
}

TEST(CrUi, CheckpointPreviousGoodSurvivesTornEmit) {
    /* The N-014 crash window: a SECOND scan on the same checkpoint path
     * that never reaches a commit (immediate cancel - the process-died-
     * mid-emit shape) must leave the PREVIOUS good checkpoint at the
     * final path BYTE-IDENTICAL. Pre-fix the "wb" open truncated the
     * final path at scan start - the previous good state was destroyed
     * BEFORE any checkpoint was even attempted. */
    const std::string img = write_fixture("cktorn");
    ASSERT_FALSE(img.empty());
    const std::string ck = cr_test::tmp_path("cktorn", ".rsf");
    /* scan 1: completes; the final path holds the good checkpoint */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.checkpoint_path_utf8 = ck.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    }
    FILE* f = std::fopen(ck.c_str(), "rb");
    ASSERT_NE(f, nullptr);
    std::vector<unsigned char> good(size_t(std::fseek(f, 0, SEEK_END) ? 0 : std::ftell(f)));
    std::rewind(f);
    ASSERT_EQ(std::fread(good.data(), 1, good.size(), f), good.size());
    std::fclose(f);
    ASSERT_GE(good.size(), size_t(16));
    /* scan 2: crash-simulation - cancel before run; no checkpoint ever
     * commits; the staging tmp is created and torn away at teardown */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.checkpoint_path_utf8 = ck.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
        EXPECT_EQ(cr_ui_scan_run(u.scan), 0x50020005u); /* ERR_CANCELLED */
    }
    /* the previous good checkpoint survives byte-identical */
    f = std::fopen(ck.c_str(), "rb");
    ASSERT_NE(f, nullptr) << "the final path must never be destroyed by a torn emit";
    std::vector<unsigned char> after(size_t(std::fseek(f, 0, SEEK_END) ? 0 : std::ftell(f)));
    std::rewind(f);
    ASSERT_EQ(std::fread(after.data(), 1, after.size(), f), after.size());
    std::fclose(f);
    EXPECT_EQ(after, good) << "previous good checkpoint preserved byte-identical";
    /* the torn staging file did not survive either */
    EXPECT_EQ(std::fopen((ck + ".tmp").c_str(), "rb"), nullptr);
}

/* ---------------------------------------------------------------- */
/* S-029: lost-partition APPLY through the C ABI (consent, undo)     */
/* ---------------------------------------------------------------- */

namespace {

/* an MBR disk image: one live NTFS entry (slot 0, LBA 2048/1024),
 * signature 0xAABBCCDD; the lost candidate at LBA 3200/512. */
std::vector<unsigned char> build_mbr_disk() {
    std::vector<unsigned char> img(size_t(4096) * 512, 0);
    unsigned char* s0 = img.data();
    const auto put32l = [](unsigned char* p, unsigned long v) {
        p[0] = static_cast<unsigned char>(v & 0xFF);
        p[1] = static_cast<unsigned char>((v >> 8) & 0xFF);
        p[2] = static_cast<unsigned char>((v >> 16) & 0xFF);
        p[3] = static_cast<unsigned char>((v >> 24) & 0xFF);
    };
    put32l(s0 + 440, 0xAABBCCDD);
    s0[446 + 4] = 0x07;
    put32l(s0 + 446 + 8, 2048);
    put32l(s0 + 446 + 12, 1024);
    s0[510] = 0x55;
    s0[511] = 0xAA;
    return img;
}

std::string write_mbr_disk(const char* tag) {
    const std::string path = cr_test::tmp_path("cr_ui", tag);
    const auto img = build_mbr_disk();
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return {};
    const size_t n = std::fwrite(img.data(), 1, img.size(), f);
    std::fclose(f);
    return n == img.size() ? path : std::string();
}

std::vector<unsigned char> read_whole_file(const std::string& path) {
    std::vector<unsigned char> out;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return out;
    unsigned char buf[65536];
    size_t n = 0;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0)
        out.insert(out.end(), buf, buf + n);
    std::fclose(f);
    return out;
}

unsigned g_apply_consent_calls = 0;
unsigned g_apply_consent_reply = 0;
cr_ui_apply_plan g_apply_consent_plan{};

} // namespace

uint32_t apply_consent_cb(void* user, const cr_ui_apply_plan* plan) {
    ++*static_cast<unsigned*>(user);
    g_apply_consent_plan = *plan;
    return g_apply_consent_reply;
}

TEST(CrUi, LostApplyConsentGateAndUndo) {
    /* The S-029 C-ABI lifecycle: plan -> consent (default-deny) ->
     * transactional write into the IMAGE FILE -> undo restore -> undo
     * container persistence. */
    Ui u;
    std::string img = write_mbr_disk("apply");
    ASSERT_FALSE(img.empty());
    const std::vector<unsigned char> pristine = read_whole_file(img);
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);

    cr_ui_apply_candidate cand{};
    cand.start_lba = 3200;
    cand.sector_count = 512;
    cand.fs = 0 /* NTFS */;
    cr_ui_apply_result res{};

    /* NULL consent: default-deny, nothing written */
    g_apply_consent_calls = 0;
    EXPECT_EQ(cr_ui_apply_lost_partition(u.dev, &cand, 0, nullptr, nullptr, &res),
              0x50080004u /* ERR_APPLY_CONSENT_DENIED */);
    EXPECT_EQ(res.outcome, 1u);
    EXPECT_EQ(res.applied, 0u);
    EXPECT_EQ(res.undo_entries, 0u);
    EXPECT_EQ(read_whole_file(img), pristine);

    /* denying callback: same refusal, callback saw the plan */
    g_apply_consent_reply = 0;
    EXPECT_EQ(
        cr_ui_apply_lost_partition(u.dev, &cand, 0, apply_consent_cb, &g_apply_consent_calls, &res),
        0x50080004u);
    ASSERT_EQ(g_apply_consent_calls, 1u);
    EXPECT_EQ(g_apply_consent_plan.refused, 0u);
    EXPECT_EQ(g_apply_consent_plan.mbr_type, 0x07u);
    EXPECT_EQ(g_apply_consent_plan.slot, 1u);
    EXPECT_EQ(g_apply_consent_plan.signature, 0xAABBCCDDu);
    EXPECT_EQ(read_whole_file(img), pristine);

    /* approving callback: the entry lands in the image file */
    g_apply_consent_reply = 1;
    EXPECT_EQ(
        cr_ui_apply_lost_partition(u.dev, &cand, 0, apply_consent_cb, &g_apply_consent_calls, &res),
        CR_UI_OK);
    EXPECT_EQ(res.outcome, 0u);
    EXPECT_EQ(res.applied, 1u);
    EXPECT_EQ(res.undo_entries, 1u);
    const std::vector<unsigned char> applied = read_whole_file(img);
    ASSERT_EQ(applied.size(), pristine.size());
    EXPECT_EQ(applied[446 + 16 + 4], 0x07u);
    EXPECT_EQ(applied[446 + 16 + 8], 3200 & 0xFF);
    EXPECT_EQ(applied[446 + 16 + 9], (3200 >> 8) & 0xFF);
    EXPECT_EQ(applied[446 + 16 + 12], 512 & 0xFF);
    EXPECT_EQ(applied[446 + 16 + 13], (512 >> 8) & 0xFF);
    /* the live entry + signature + 55AA preserved */
    EXPECT_EQ(applied[446 + 4], 0x07u);
    EXPECT_EQ(applied[440], 0xDD);
    EXPECT_EQ(applied[510], 0x55);
    EXPECT_EQ(applied[511], 0xAA);

    /* undo restores byte-exact */
    EXPECT_EQ(cr_ui_apply_undo_last(u.dev, &res), CR_UI_OK);
    EXPECT_EQ(res.outcome, 0u);
    EXPECT_EQ(res.applied, 1u);
    EXPECT_EQ(read_whole_file(img), pristine);

    /* the undo container persists (CRUNDO01) */
    const std::string undo_path = img + ".undo";
    EXPECT_EQ(cr_ui_apply_save_undo(u.dev, undo_path.c_str()), CR_UI_OK);
    const std::vector<unsigned char> blob = read_whole_file(undo_path);
    EXPECT_GT(blob.size(), 8u);
}

TEST(CrUi, LostApplyRefusesOverlapAndSlices) {
    /* planner refusals surface through the ABI with reasons; a volume
     * slice (image opened at nonzero offset) is refused outright. */
    Ui u;
    std::string img = write_mbr_disk("applyref");
    ASSERT_FALSE(img.empty());
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);

    cr_ui_apply_candidate cand{};
    cand.start_lba = 2500; /* overlaps the live 2048..3072 */
    cand.sector_count = 600;
    cand.fs = 0;
    cr_ui_apply_result res{};
    g_apply_consent_calls = 0;
    g_apply_consent_reply = 1;
    EXPECT_EQ(
        cr_ui_apply_lost_partition(u.dev, &cand, 0, apply_consent_cb, &g_apply_consent_calls, &res),
        0x50080006u /* ERR_APPLY_REFUSED */);
    EXPECT_EQ(res.outcome, 2u);
    EXPECT_NE(std::string(res.reason).find("overlap"), std::string::npos);
    EXPECT_EQ(g_apply_consent_calls, 0u); /* refused plans never reach consent */

    /* GPT single-entry scope: refused (v1 is the MBR path) */
    Ui ug;
    std::string gimg = img + ".gpt";
    {
        auto d = build_mbr_disk();
        std::memset(d.data() + 446, 0, 64);
        d[446 + 4] = 0xEE; /* protective */
        FILE* f = std::fopen(gimg.c_str(), "wb");
        ASSERT_NE(f, nullptr);
        ASSERT_EQ(std::fwrite(d.data(), 1, d.size(), f), d.size());
        std::fclose(f);
    }
    ASSERT_EQ(cr_ui_open_image(gimg.c_str(), 0, 0, &ug.dev), CR_UI_OK);
    EXPECT_EQ(cr_ui_apply_lost_partition(ug.dev, &cand, 0, apply_consent_cb, &g_apply_consent_calls,
                                         &res),
              0x50080006u);
    EXPECT_EQ(res.outcome, 2u);
    EXPECT_NE(std::string(res.reason).find("GPT"), std::string::npos);

    /* a slice view (nonzero offset) is not the disk */
    Ui us;
    std::string simg = img + ".slice";
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 512, 0, &us.dev), CR_UI_OK);
    cand.start_lba = 3200;
    cand.sector_count = 512;
    EXPECT_EQ(cr_ui_apply_lost_partition(us.dev, &cand, 0, apply_consent_cb, &g_apply_consent_calls,
                                         &res),
              0x50080006u);
    EXPECT_EQ(res.outcome, 2u);
    EXPECT_NE(std::string(res.reason).find("slice"), std::string::npos);
}

/* ------------------------------------------------------------------ */
/* M-029: the og filter-bar bucket IDs through the ABI                 */
/* ------------------------------------------------------------------ */

TEST(CrUi, QueryApplyBucketsFillsAndRejects) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("buckets", u, img, crsf));

    /* full population for reference */
    cr_ui_query q{};
    uint64_t total = 0;
    cr_ui_query_row* rows = nullptr;
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    ASSERT_GE(total, 3u);
    const uint64_t full_total = total;
    cr_ui_query_rows_free(rows, n);
    rows = nullptr;

    /* Date bucket 0x103 (Last 1 month): the range becomes a concrete
     * local-day window ending today - and since the fixture's files carry
     * FAT-epoch dates (1980), the translated window excludes them all,
     * proving the range flows through the query path */
    q = cr_ui_query{};
    ASSERT_EQ(cr_ui_query_apply_buckets(&q, 0x103, 0, 0, 0, 0, 0), CR_UI_OK);
    EXPECT_GT(q.modified_from, 0u);
    EXPECT_GT(q.modified_to, q.modified_from);
    const uint64_t day = 24ull * 3600 * 10'000'000;
    EXPECT_GE(q.modified_to - q.modified_from, 29 * day); /* 30 local days */
    EXPECT_LE(q.modified_to - q.modified_from, 31 * day); /* DST-tolerant */
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_EQ(total, 0u) << "1980-dated fixture rows cannot fall in the last month";
    cr_ui_query_rows_free(rows, n);

    /* Date bucket 0 (none): the axis is untouched */
    q = cr_ui_query{};
    q.modified_from = 0;
    q.modified_to = 0;
    ASSERT_EQ(cr_ui_query_apply_buckets(&q, 0, 0, 0, 0, 0, 0), CR_UI_OK);
    EXPECT_EQ(q.modified_from, 0u);
    EXPECT_EQ(q.modified_to, 0u);

    /* Size bucket 0x1000 (< 512KB): everything in the fixture */
    q = cr_ui_query{};
    ASSERT_EQ(cr_ui_query_apply_buckets(&q, 0, 0x1000, 0, 0, 0, 0), CR_UI_OK);
    EXPECT_EQ(q.size_min, 0u);
    EXPECT_EQ(q.size_max, uint64_t(512 * 1024));
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_EQ(total, full_total);
    cr_ui_query_rows_free(rows, n);

    /* Size bucket 0x1004 (> 1 GiB): nothing in the fixture */
    q = cr_ui_query{};
    ASSERT_EQ(cr_ui_query_apply_buckets(&q, 0, 0x1004, 0, 0, 0, 0), CR_UI_OK);
    EXPECT_EQ(q.size_min, uint64_t(1024 * 1024 * 1024) + 1);
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    EXPECT_EQ(total, 0u);
    cr_ui_query_rows_free(rows, n);

    /* Size custom 0x1006: inclusive [500, 1000] catches both boundary
     * rows - DOC.TXT (1000 B, the inclusive upper bound) and DELFILE.TXT
     * (500 B, the inclusive lower bound) - but not PHOTO.JPG (200 B) */
    q = cr_ui_query{};
    ASSERT_EQ(cr_ui_query_apply_buckets(&q, 0, 0x1006, 0, 0, 500, 1000), CR_UI_OK);
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    ASSERT_EQ(total, 2u);
    /* the deleted 8.3 name surfaces as "#ELFILE.TXT" (the 0xE5 marker
     * destroys the first character) - match by size for robustness */
    bool saw_doc = false, saw_del = false;
    for (uint32_t i = 0; i < n; ++i) {
        if (rows[i].size == 1000)
            saw_doc = true;
        if (rows[i].size == 500)
            saw_del = true;
    }
    EXPECT_TRUE(saw_doc);
    EXPECT_TRUE(saw_del);
    cr_ui_query_rows_free(rows, n);

    /* the skipped IDs (0x104, 0x1005), unknown values, and a null query
     * are rejected - the core never invents bucket semantics */
    q = cr_ui_query{};
    EXPECT_EQ(cr_ui_query_apply_buckets(&q, 0x104, 0, 0, 0, 0, 0), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_query_apply_buckets(&q, 0, 0x1005, 0, 0, 0, 0), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_query_apply_buckets(&q, 0x9999, 0, 0, 0, 0, 0), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_query_apply_buckets(&q, 0, 0x1234, 0, 0, 0, 0), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_query_apply_buckets(nullptr, 0, 0, 0, 0, 0, 0), CR_UI_ERR_BAD_PARAMETER);
    /* rejection leaves the query untouched */
    EXPECT_EQ(q.modified_from, 0u);
    EXPECT_EQ(q.size_min, 0u);
}

TEST(CrUi, RawCategoryNameNamesTheCarveRoot) {
    /* M-029: the og's per-scan "filter_generalraw" naming surface - the
     * configured display name becomes the carve root's role (and rides
     * the session round-trip; the resume path recognizes it) */
    const std::string img = write_fixture("rawname");
    ASSERT_FALSE(img.empty());
    Ui u;
    ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.raw_category_name_utf8 = "General Raw Files";
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);

    /* the carve root carries the configured name; the standard roots are
     * untouched */
    cr_ui_snapshot* snap = nullptr;
    ASSERT_EQ(cr_ui_scan_snapshot(u.scan, &snap), CR_UI_OK);
    uint32_t count = 0;
    ASSERT_EQ(cr_ui_snapshot_node_count(snap, &count), CR_UI_OK);
    ASSERT_GT(count, 0u);
    bool saw_custom = false, saw_found = false;
    char name[128];
    for (uint32_t i = 0; i < count; ++i) {
        cr_ui_node_info info{};
        if (cr_ui_snapshot_node(snap, i, &info, name, sizeof(name)) != CR_UI_OK)
            continue;
        if (!info.is_full)
            continue; /* only roots carry roles */
        if (std::strcmp(name, "General Raw Files") == 0)
            saw_custom = true;
        if (std::strcmp(name, "Found") == 0)
            saw_found = true;
    }
    cr_ui_snapshot_close(snap);
    EXPECT_TRUE(saw_found) << "the standard Found root is unaffected";
    EXPECT_TRUE(saw_custom) << "the carve root carries the configured name";
}

/* ---- M-048: the user-signature registration ABI (since 2.1) ---- */

TEST(CrUi, UserSignRegistrationSurface) {
    /* register -> count -> info round-trip; a bad payload is atomic
     * (the previous set survives) */
    const char* good = "{\"abc\": {\"signs\": [\"0,DEADBEEF\"], \"mime\": \"application/x-abc\"},"
                       " \"zz9\": {\"signs\": [\"16,0011\"], \"mime\": \"application/x-zz9\"}}";
    ASSERT_EQ(0u, cr_ui_register_user_signs(good, cr::u32(std::strlen(good))));
    ASSERT_EQ(2u, cr_ui_user_sign_count());
    cr_ui_user_sign s0{};
    ASSERT_EQ(0u, cr_ui_user_sign_info(0, &s0));
    EXPECT_STREQ("abc", s0.ext);
    EXPECT_STREQ("application/x-abc", s0.mime);
    EXPECT_EQ(0u, s0.offset);
    EXPECT_EQ(4u, s0.sign_len);
    EXPECT_EQ(0xDE, s0.bytes[0]);
    EXPECT_EQ(0xEF, s0.bytes[3]);
    cr_ui_user_sign s1{};
    ASSERT_EQ(0u, cr_ui_user_sign_info(1, &s1));
    EXPECT_STREQ("zz9", s1.ext);
    EXPECT_EQ(16u, s1.offset);
    EXPECT_EQ(2u, s1.sign_len);
    /* out of range + null out */
    cr_ui_user_sign dummy{};
    EXPECT_NE(0u, cr_ui_user_sign_info(2, &dummy));
    EXPECT_NE(0u, cr_ui_user_sign_info(0, nullptr));
    /* bad payload: atomic (the good set survives) */
    EXPECT_NE(0u, cr_ui_register_user_signs("{not json", 9));
    EXPECT_EQ(2u, cr_ui_user_sign_count());
    /* null/empty */
    EXPECT_NE(0u, cr_ui_register_user_signs(nullptr, 0));
    /* re-registration replaces (the og's config-reload posture) */
    const char* one = "{\"q\": {\"signs\": [\"0,AA\"]}}";
    ASSERT_EQ(0u, cr_ui_register_user_signs(one, cr::u32(std::strlen(one))));
    EXPECT_EQ(1u, cr_ui_user_sign_count());
}

TEST(CrUi, UserSignAbiCarriesTheFullS039Widths) {
    /* M-067 (the S-039 convergence residue): the ABI struct must carry
     * the full v3 caps - the og's longest MIME (73 chars, the pptx office
     * family) and the og's longest sign (44 B) survive the info round-trip
     * NUL-terminated. Pre-v3 (mime[64]/bytes[16]): the 73-char MIME's
     * memcpy left mime[] with NO terminator (a host string read ran past
     * the field) and only 16 of the 44 sign bytes surfaced. */
    const char* mime73 = "application/vnd.openxmlformats-officedocument."
                         "presentationml.presentation";
    ASSERT_EQ(73u, std::strlen(mime73)) << "the og's longest MIME (pptx)";
    /* the og's own 44-B img sign, verbatim */
    const char* sign44 =
        "7E742C015070024D52010000000800000001000031000000310000004301FF0001000800010000007e742c01";
    const std::string json =
        std::string("{\"img\": {\"signs\": [\"0,") + sign44 + "\"], \"mime\": \"" + mime73 + "\"}}";
    ASSERT_EQ(0u, cr_ui_register_user_signs(json.data(), cr::u32(json.size())));

    cr_ui_user_sign s{};
    ASSERT_EQ(0u, cr_ui_user_sign_info(0, &s));
    EXPECT_EQ(0, std::strcmp(s.mime, mime73))
        << "the 73-char MIME round-trips whole and NUL-terminated";
    EXPECT_EQ('\0', s.mime[73]) << "the terminator is inside the field";
    EXPECT_EQ(44u, s.sign_len) << "the 44-B sign's length round-trips";
    EXPECT_EQ(0x7E, s.bytes[0]);
    EXPECT_EQ(0x01, s.bytes[43]) << "the 44th byte surfaces (pre-v3: 16 of 44)";
    /* restore the empty set for the other tests */
    const char* empty = "{\"q\": {\"signs\": [\"0,AA\"]}}";
    ASSERT_EQ(0u, cr_ui_register_user_signs(empty, cr::u32(std::strlen(empty))));
}

TEST(CrUi, TransportErrorAdvisoryZeroOnCleanState) {
    /* M-030 claim (f): the ABI can now report the transport-open
     * diagnostic (ABI-since 2.2, additive). DIRECT-ERA UPDATE
     * (session-96): the clean-state-zero premise holds again - the
     * direct open is the only leg on Windows. The current contract:
     * on a non-elevated host the open REFUSES with
     * ERR_ELEVATION_REQUIRED and the advisory carries that code (the
     * presentation channel for hosts that surface transport
     * diagnostics); on an elevated host the direct open either
     * succeeds (advisory resets to 0) or fails with the target error.
     * The Linux stub still answers UNSUPPORTED and records nothing. */
    cr_ui_device* d = nullptr;
    const uint32_t rc = cr_ui_open_disk(0, &d);
#ifdef _WIN32
    if (cr::win::is_elevated()) {
        /* elevated: open succeeds (advisory 0) or fails with a target
         * error; the advisory carries whichever it was */
        if (rc == 0u) {
            ASSERT_NE(d, nullptr);
            cr_ui_device_close(d);
            EXPECT_EQ(cr_ui_last_transport_error(), 0u) << "success resets the advisory";
        } else {
            EXPECT_EQ(d, nullptr);
            /* the refusal class is visible through the advisory */
            EXPECT_NE(cr_ui_last_transport_error(), 0u);
        }
    } else {
        /* non-elevated: the classified privilege refusal, exactly */
        EXPECT_EQ(rc, 0x500C0001u) << "the ERR_ELEVATION_REQUIRED refusal";
        EXPECT_EQ(d, nullptr);
        EXPECT_EQ(cr_ui_last_transport_error(), 0x500C0001u)
            << "the advisory carries the refusal class";
    }
#else
    EXPECT_EQ(rc, CR_UI_ERR_UNSUPPORTED);
    EXPECT_EQ(cr_ui_last_transport_error(), 0u);
#endif
}

/* ---- api-fix session: ABI doc-truth pins (rewritten for the direct
 * era by session-96 - the driver-era pins died with the driver) ---- */

TEST(CrUi, AbiHeaderNamesNoPhantomErrorMacros) {
    /* api-fix: cr_ui.h once documented `CR_UI_ERR_INVALID_PARAM` in the
     * query_apply_buckets comment - a macro that never existed (the
     * implementation returns ERR_BAD_PARAMETER, 0x50030003). No
     * phantom identifier may return to the header. */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("CR_UI_ERR_INVALID_PARAM"), std::string::npos)
        << "the header must not name a macro that does not exist";
}

TEST(CrUi, TransportDocCarriesDirectTruth) {
    /* session-96: the transport contract must advertise exactly the two
     * live values - "direct" and "image" - and must not resurrect the
     * removed "rda" value. */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    const size_t at = hdr.find("cr_ui_device_transport");
    ASSERT_NE(at, std::string::npos);
    /* the contract comment block sits just above the declaration */
    const std::string around = hdr.substr(at > 600 ? at - 600 : 0, 700);
    EXPECT_NE(around.find("\"direct\""), std::string::npos)
        << "the transport contract must advertise the direct value";
    EXPECT_EQ(around.find("\"rda\""), std::string::npos) << "the removed rda value must not return";
}

TEST(CrUi, AbiContractCarriesDirectEraTruth) {
    /* session-96: ABI-CONTRACT's normative error table (section 4) must
     * carry ERR_ELEVATION_REQUIRED (the direct era's single refusal
     * class) and must NOT carry the removed ERR_DRIVER_* family; the
     * event block (section 6) must still carry CR_EVT_FS_CHECK (type
     * 10) with the M-041 struct tail. */
    const std::string doc = cr_test::read_repo_text("docs/ABI-CONTRACT.md");
    ASSERT_FALSE(doc.empty());
    EXPECT_NE(doc.find("ERR_ELEVATION_REQUIRED"), std::string::npos)
        << "section 4 must list the privilege-refusal class";
    EXPECT_EQ(doc.find("ERR_DRIVER_ABSENT"), std::string::npos)
        << "the removed driver family must not be documented";
    EXPECT_NE(doc.find("CR_EVT_FS_CHECK"), std::string::npos)
        << "section 6 must list event type 10";
    EXPECT_NE(doc.find("check_errors"), std::string::npos)
        << "section 6's struct sketch must carry the M-041 tail fields";
}

TEST(CrUi, DriverSurfaceGoneFromAbiHeader) {
    /* session-96: the driver-status ABI surface is REMOVED, not stubbed.
     * The header must not declare cr_ui_driver_status, the state
     * constants, or the status struct; the ABI version is 4. */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("cr_ui_driver_status"), std::string::npos)
        << "the export must not be declared";
    EXPECT_EQ(hdr.find("CR_UI_DRIVER_STATE"), std::string::npos)
        << "the state constants must not be declared";
    EXPECT_EQ(hdr.find("ERR_DRIVER_"), std::string::npos)
        << "the driver-era refusal family must not be documented";
    EXPECT_NE(hdr.find("#define CR_UI_ABI_VERSION 4u"), std::string::npos)
        << "the removal bumps the ABI version to 4";
}

TEST(CrUi, OutOfScopeRowsCarryTruthfulRegistrations) {
    /* S-050's disposition pin (the finding's own test idea: "a docs
     * test that every SURFACE-INVENTORY OOS row's cited registration
     * exists and contains the row - this drift class is systematic").
     * The file-repair line's row cites CAPABILITY-MATRIX §3; the
     * pre-fix drift was exactly this: the citation pointed at a §3
     * that did not list the row. The registration + the D-137 delta
     * must both exist and name the line. */
    const std::string inv = cr_test::read_repo_text("diff-to-fix/SURFACE-INVENTORY.md");
    ASSERT_FALSE(inv.empty());
    /* the row itself must cite the REGISTERED state, not the audited-drift state */
    EXPECT_NE(inv.find("REGISTERED OUT OF SCOPE (S-050 disposition"), std::string::npos)
        << "the file-repair row cites the formal registration";
    const std::string cap = cr_test::read_repo_text("docs/CAPABILITY-MATRIX.md");
    ASSERT_FALSE(cap.empty());
    EXPECT_NE(cap.find("The file-repair product line (S-050's formal disposition"),
              std::string::npos)
        << "CAPABILITY-MATRIX §3 carries the exclusion row";
    EXPECT_NE(cap.find("Fixo"), std::string::npos)
        << "the exclusion row carries the provenance rationale";
    const std::string deltas = cr_test::read_repo_text("docs/PARITY-DELTAS.md");
    ASSERT_FALSE(deltas.empty());
    EXPECT_NE(deltas.find("D-137"), std::string::npos)
        << "the scope-exclusion delta with the future implementer's entry points";
}

TEST(CrUi, CliHelpListsEveryDispatchedVerb) {
    /* N-094: the -h usage text omitted the sessions verb while wmain
     * dispatched it (10 lines listed vs 11 verbs reachable) - a verb
     * invisible in the product's own help. The exe is Windows-only, so
     * this pin asserts the same invariant at the source level: every
     * verb the dispatch accepts must appear in the usage block AND in
     * the header's verb table (the lock's fourth site, so the
     * three-site verb lock cannot drift again). */
    const std::string src = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(src.empty());

    /* the dispatched set: every `if (cmd == L"<verb>")` after wmain */
    const size_t wmain = src.find("int wmain(");
    ASSERT_NE(std::string::npos, wmain) << "wmain not found";
    std::vector<std::string> verbs;
    const std::string dispatch_pat = "if (cmd == L\"";
    for (size_t pos = wmain; (pos = src.find(dispatch_pat, pos)) != std::string::npos;) {
        const size_t start = pos + dispatch_pat.size();
        const size_t end = src.find('"', start);
        ASSERT_NE(std::string::npos, end);
        verbs.push_back(src.substr(start, end - start));
        pos = end;
    }
    ASSERT_GE(verbs.size(), 8u) << "dispatch extraction broke (found " << verbs.size() << ")";

    /* the usage block: between `usage:` and the privilege-prerequisite note */
    const size_t usage_at = src.find("usage:");
    ASSERT_NE(std::string::npos, usage_at);
    const size_t usage_end = src.find("privilege prerequisite:", usage_at);
    ASSERT_NE(std::string::npos, usage_end);
    const std::string usage = src.substr(usage_at, usage_end - usage_at);

    /* the header's verb table: between "Commands:" and "Exit codes:" */
    const size_t hdr_cmds = src.find("Commands:");
    ASSERT_NE(std::string::npos, hdr_cmds);
    const size_t hdr_end = src.find("Exit codes:", hdr_cmds);
    ASSERT_NE(std::string::npos, hdr_end);
    const std::string header = src.substr(hdr_cmds, hdr_end - hdr_cmds);

    for (const std::string& v : verbs) {
        /* the usage source lines end with either a flag (`rrecov scan --disk`)
         * or the literal `\n` escape - accept both spellings */
        const bool in_usage = usage.find("rrecov " + v + " ") != std::string::npos ||
                              usage.find("rrecov " + v + "\\n") != std::string::npos;
        EXPECT_TRUE(in_usage) << "verb '" << v << "' is dispatched but has no -h usage line";
        /* the header table's lines read ` *   <verb> [flags]` */
        const std::string needle = "\n *   " + v + " ";
        EXPECT_NE(std::string::npos, header.find(needle))
            << "verb '" << v << "' is dispatched but missing from the header verb table";
    }
    /* and the block lists nothing the dispatch lacks (a usage line for an
     * unimplemented verb is just as false) */
    size_t line = 0;
    while ((line = usage.find("  rrecov ", line)) != std::string::npos) {
        const size_t start = line + strlen("  rrecov ");
        size_t end = start;
        while (end < usage.size() && (isalnum(static_cast<unsigned char>(usage[end])) ||
                                      usage[end] == '-' || usage[end] == '_'))
            ++end;
        const std::string listed = usage.substr(start, end - start);
        if (!listed.empty()) {
            EXPECT_NE(verbs.end(), std::find(verbs.begin(), verbs.end(), listed))
                << "the -h block lists '" << listed << "' but wmain never dispatches it";
        }
        line = end;
    }
}

/* ---- M-102/M-083: the thumbnail ABI ---- */

namespace {

/* the 32x24 quadrant image (TL red / TR green / BL blue / BR yellow) as a
 * PNG (exact colors) and a 45%-truncated JPEG (the disclosure case), both
 * embedded so the cr_ui suite is self-contained (the thumbnail engine's
 * file fixtures are staged beside cr_tests, not cr_ui_tests) */
static const unsigned char kQuad32x24Png[] = {
    137, 80,  78,  71,  13,  10,  26,  10,  0,   0,  0,   13, 73,  72,  68,  82,  0,   0,   0,
    32,  0,   0,   0,   24,  8,   2,   0,   0,   0,  20,  49, 104, 99,  0,   0,   0,   55,  73,
    68,  65,  84,  120, 156, 99,  252, 207, 64,  26, 96,  36, 81,  3,   19,  137, 230, 147, 12,
    70,  45,  24,  181, 96,  212, 130, 225, 96,  1,  35,  3,  3,   105, 133, 203, 255, 255, 140,
    36,  169, 31,  250, 65,  52,  106, 193, 168, 5,  163, 22, 208, 193, 2,   0,   9,   177, 5,
    44,  229, 141, 103, 181, 0,   0,   0,   0,   73, 69,  78, 68,  174, 66,  96,  130,
};

/* the same image as JPEG-90, cut at 45%: 344 of 766 bytes - stb's zero-fill leniency decodes it Ok;
 * the contract REQUIRES the truncated disclosure */
static const unsigned char kQuad32x24JpgTrunc[] = {
    255, 216, 255, 224, 0,   16,  74,  70,  73,  70,  0,   1,   1,   0,   0,   1,   0,   1,   0,
    0,   255, 219, 0,   67,  0,   3,   2,   2,   3,   2,   2,   3,   3,   3,   3,   4,   3,   3,
    4,   5,   8,   5,   5,   4,   4,   5,   10,  7,   7,   6,   8,   12,  10,  12,  12,  11,  10,
    11,  11,  13,  14,  18,  16,  13,  14,  17,  14,  11,  11,  16,  22,  16,  17,  19,  20,  21,
    21,  21,  12,  15,  23,  24,  22,  20,  24,  18,  20,  21,  20,  255, 219, 0,   67,  1,   3,
    4,   4,   5,   4,   5,   9,   5,   5,   9,   20,  13,  11,  13,  20,  20,  20,  20,  20,  20,
    20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,
    20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,  20,
    20,  20,  20,  20,  20,  20,  255, 192, 0,   17,  8,   0,   24,  0,   32,  3,   1,   34,  0,
    2,   17,  1,   3,   17,  1,   255, 196, 0,   31,  0,   0,   1,   5,   1,   1,   1,   1,   1,
    1,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,
    11,  255, 196, 0,   181, 16,  0,   2,   1,   3,   3,   2,   4,   3,   5,   5,   4,   4,   0,
    0,   1,   125, 1,   2,   3,   0,   4,   17,  5,   18,  33,  49,  65,  6,   19,  81,  97,  7,
    34,  113, 20,  50,  129, 145, 161, 8,   35,  66,  177, 193, 21,  82,  209, 240, 36,  51,  98,
    114, 130, 9,   10,  22,  23,  24,  25,  26,  37,  38,  39,  40,  41,  42,  52,  53,  54,  55,
    56,  57,  58,  67,  68,  69,  70,  71,  72,  73,  74,  83,  84,  85,  86,  87,  88,  89,  90,
    99,  100, 101, 102, 103, 104, 105, 106, 115, 116, 117, 118, 119, 120, 121, 122, 131, 132, 133,
    134, 135, 136, 137, 138, 146, 147, 148, 149, 150, 151, 152, 153, 154, 162, 163, 164, 165, 166,
    167, 168,
};

/* sizes: png=112 jpg=766 trunc=344 */

/* a FAT16 carrying ONE real image: the quadrant PNG at cluster 40 */
std::vector<unsigned char> build_fat16_png() {
    auto img = build_fat16();
    const size_t need = (sizeof(kQuad32x24Png) + 511) / 512;
    /* unsigned loop var (the file's write_fixture_zip convention): the FAT
     * chain values 41+i fit unsigned, so put16's (size_t, unsigned) params
     * see no narrowing (MSVC C4267 on the size_t ternary) */
    for (unsigned i = 0; i < need; ++i)
        put16(img, kFAT1 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i),
            put16(img, kFAT2 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i);
    dentry(img, 3, "LOGO    PNG", 0x20, 40, (unsigned long)sizeof(kQuad32x24Png), false);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, kQuad32x24Png, sizeof(kQuad32x24Png));
    return img;
}

/* the PNG as a DELETED entry (0xE5): deleted non-carved files fall to
 * the generic grading path (below 2), so the M-083 promotion is
 * observable - existing files grade 2 at insert (FUN_14037a8a0) and
 * would make the test vacuous */
std::vector<unsigned char> build_fat16_png_deleted() {
    auto img = build_fat16();
    const size_t need = (sizeof(kQuad32x24Png) + 511) / 512;
    for (unsigned i = 0; i < need; ++i)
        put16(img, kFAT1 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i),
            put16(img, kFAT2 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i);
    dentry(img, 3, "LOGO    PNG", 0x20, 40, (unsigned long)sizeof(kQuad32x24Png), true);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, kQuad32x24Png, sizeof(kQuad32x24Png));
    return img;
}

/* the same shape with the truncated JPEG as LOGO2.JPG */
std::vector<unsigned char> build_fat16_jpg_trunc() {
    auto img = build_fat16();
    const size_t need = (sizeof(kQuad32x24JpgTrunc) + 511) / 512;
    for (unsigned i = 0; i < need; ++i)
        put16(img, kFAT1 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i),
            put16(img, kFAT2 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i);
    dentry(img, 3, "LOGO2   JPG", 0x20, 40, (unsigned long)sizeof(kQuad32x24JpgTrunc), false);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, kQuad32x24JpgTrunc,
                sizeof(kQuad32x24JpgTrunc));
    return img;
}

/* M-115: the damaged-but-readable PNG - intact extents, corrupt IDAT
 * entropy (one flipped byte inside the zlib stream). The strict PNG
 * decoder refuses the stream (DecodeFailed) while every byte reads
 * cleanly: the og's read-verdict promotion fires (grade 2), the
 * pre-fix decode-gated core did not. */
std::vector<unsigned char> build_fat16_png_damaged() {
    auto img = build_fat16();
    const size_t need = (sizeof(kQuad32x24Png) + 511) / 512;
    for (unsigned i = 0; i < need; ++i)
        put16(img, kFAT1 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i),
            put16(img, kFAT2 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i);
    /* DELETED entry: deleted non-carved files take the generic grading
     * path (below 2) so the promotion is observable (the existing-file
     * insert-grade-2 would make the test vacuous) */
    dentry(img, 3, "DAMAGE  PNG", 0x20, 40, (unsigned long)sizeof(kQuad32x24Png), true);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, kQuad32x24Png, sizeof(kQuad32x24Png));
    /* corrupt one byte inside the IDAT zlib stream (offset 41 in the
     * embedded array: past signature+IHDR+IDAT-header, inside the
     * compressed payload) - inflate refuses the stream */
    img[kDATA + (40 - 2) * 512 + 41] ^= 0xFF;
    return img;
}

/* M-115: the <=20-byte decodable image - a 1x1 PBM ("P1 1 1 0") is 8
 * bytes, decodes through the stb PNM path, and is UNDER the og's
 * 0x14-byte work-item floor: the og never creates a work item (never
 * promotes); the pre-fix core decoded it and promoted. */
static const unsigned char kTinyPbm8[] = {'P', '1', '\n', '1', ' ', '1', '\n', '0'};

std::vector<unsigned char> build_fat16_tiny_pbm() {
    auto img = build_fat16();
    put16(img, kFAT1 + 40 * 2, 0xFFFF);
    put16(img, kFAT2 + 40 * 2, 0xFFFF);
    dentry(img, 3, "TINY    PBM", 0x20, 40, sizeof(kTinyPbm8), true);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, kTinyPbm8, sizeof(kTinyPbm8));
    return img;
}

/* M-115 (the S47 lane's convergence supplement): a JPEG-shaped file with
 * clean extents but NO SOF - the marker walk (jpeg_dims) fails
 * deterministically at the first non-marker byte, so the decode is refused
 * while every read is clean. Exercises the DIMS-seam of the read-evidence
 * promotion (the parallel lane's damaged-PNG fixture exercises the
 * entropy-seam - together the two refusal classes cover both DecodeFailed
 * exits the promotion consumes). */
static const unsigned char kJpgNoSof[] = {
    255, 216, 255, 224, 0, 16, 74, 70, 73, 70, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 255, 254, 0, 4, 65, 66,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   0,   0, 0, 0,  0,
    0,   0,   0,   0,   0, 0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, 0,
};

/* the no-SOF JPEG as a DELETED entry (0xE5 -> '#'): deleted non-carved files
 * grade below 2, so the read-evidence promotion is observable */
std::vector<unsigned char> build_fat16_jpg_corrupt() {
    auto img = build_fat16();
    const size_t need = (sizeof(kJpgNoSof) + 511) / 512;
    for (unsigned i = 0; i < need; ++i)
        put16(img, kFAT1 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i),
            put16(img, kFAT2 + (40 + i) * 2, i + 1 == need ? 0xFFFF : 41 + i);
    dentry(img, 3, "BAD     JPG", 0x20, 40, (unsigned long)sizeof(kJpgNoSof), true);
    std::memcpy(img.data() + kDATA + (40 - 2) * 512, kJpgNoSof, sizeof(kJpgNoSof));
    return img;
}

bool scan_img_to_session(const char* tag, const std::vector<unsigned char>& bytes, Ui& u,
                         std::string& img_out, std::string& crsf_out) {
    img_out = cr_test::tmp_path("cr_ui", tag);
    FILE* f = std::fopen(img_out.c_str(), "wb");
    if (!f)
        return false;
    const size_t n = std::fwrite(bytes.data(), 1, bytes.size(), f);
    std::fclose(f);
    if (n != bytes.size())
        return false;
    if (cr_ui_open_image(img_out.c_str(), 0, 0, &u.dev) != CR_UI_OK)
        return false;
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    if (cr_ui_scan_create(&cfg, &u.scan) != CR_UI_OK)
        return false;
    if (cr_ui_scan_attach_device(u.scan, u.dev) != CR_UI_OK)
        return false;
    if (cr_ui_scan_run(u.scan) != CR_UI_OK)
        return false;
    crsf_out = img_out + ".crsf";
    if (cr_ui_scan_save_session(u.scan, crsf_out.c_str()) != CR_UI_OK)
        return false;
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    if (cr_ui_session_load(crsf_out.c_str(), &u.sess) != CR_UI_OK)
        return false;
    return true;
}

/* the node's current grade through the query surface */
uint32_t grade_of(cr_ui_session* sess, const char* name) {
    cr_ui_query q{};
    q.name_substring = name;
    q.limit = 50;
    cr_ui_query_row* rows = nullptr;
    uint32_t n = 0;
    uint64_t total = 0;
    if (cr_ui_query_run(sess, &q, &rows, &n, &total) != CR_UI_OK)
        return 0xFFFFFFFFu;
    uint32_t g = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < n; ++i)
        if (std::strcmp(rows[i].name, name) == 0)
            g = rows[i].grade;
    cr_ui_query_rows_free(rows, n);
    return g;
}

} // namespace

TEST(CrUi, ThumbnailTwoCallPattern) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("thumb2c", build_fat16_png(), u, img, crsf));
    const uint64_t logo = query_node_id(u.sess, "LOGO.PNG");
    ASSERT_NE(logo, 0u);

    /* call 1: the size probe (rgba NULL) */
    cr_ui_thumbnail_info info{};
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, logo, nullptr, nullptr, 0, &info), CR_UI_OK);
    EXPECT_EQ(32u, info.width);
    EXPECT_EQ(24u, info.height);
    EXPECT_EQ(32u * 24u * 4u, info.bytes_needed);
    EXPECT_EQ(0u, info.from_embedded);
    EXPECT_EQ(0u, info.truncated);
    EXPECT_STREQ("png", info.source_format);

    /* too-small buffer: the named refusal with the answer preserved */
    std::vector<uint8_t> small(16, 0xAA);
    cr_ui_thumbnail_info info2{};
    EXPECT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, logo, nullptr, small.data(), 16, &info2),
              0x50030009u);
    EXPECT_EQ(32u * 24u * 4u, info2.bytes_needed);

    /* call 2: the fill - the quadrant corners are exact (PNG, lossless) */
    std::vector<uint8_t> px(info.bytes_needed, 0);
    cr_ui_thumbnail_info info3{};
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, logo, nullptr, px.data(), uint32_t(px.size()),
                                       &info3),
              CR_UI_OK);
    ASSERT_EQ(px.size(), size_t(info3.width) * info3.height * 4u);
    auto at = [&](uint32_t x, uint32_t y) { return px.data() + (size_t(y) * info3.width + x) * 4; };
    EXPECT_EQ(255, at(2, 2)[0]);
    EXPECT_EQ(0, at(2, 2)[1]);
    EXPECT_EQ(0, at(29, 2)[0]);
    EXPECT_EQ(255, at(29, 2)[1]);
    EXPECT_EQ(255, at(2, 21)[2]); /* BL quadrant is BLUE */
    EXPECT_EQ(255, at(29, 21)[0]);
    EXPECT_EQ(255, at(29, 21)[1]);
    EXPECT_EQ(255, at(2, 2)[3]); /* opaque */

    /* the caps pass-through: max_edge 8 downscale to 8x6 */
    {
        cr_ui_thumbnail_caps caps{};
        caps.max_edge = 8;
        cr_ui_thumbnail_info info8{};
        ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, logo, &caps, nullptr, 0, &info8),
                  CR_UI_OK);
        EXPECT_EQ(8u, info8.width);
        EXPECT_EQ(6u, info8.height);
    }
}

TEST(CrUi, ThumbnailTruncationDisclosed) {
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("thumbtr", build_fat16_jpg_trunc(), u, img, crsf));
    const uint64_t logo = query_node_id(u.sess, "LOGO2.JPG");
    ASSERT_NE(logo, 0u);
    cr_ui_thumbnail_info info{};
    const uint32_t rc = cr_ui_thumbnail_generate(u.dev, u.sess, logo, nullptr, nullptr, 0, &info);
    /* stb's zero-fill leniency: Ok + truncated=1, or the honest refusal -
     * either way the truncation is NEVER silent (both carry the flag/row) */
    if (rc == CR_UI_OK) {
        EXPECT_EQ(1u, info.truncated) << "Ok-with-truncation must disclose it";
        EXPECT_STREQ("jpeg", info.source_format);
    } else {
        EXPECT_EQ(0x50030004u, rc) << "a refused decode is ERR_TRUNCATED";
        EXPECT_EQ(1u, info.truncated);
    }
}

TEST(CrUi, ThumbnailUnsupportedFormat) {
    /* the format probe: the v1 matrix true, the deferred families false */
    EXPECT_EQ(1u, cr_ui_thumbnail_format_supported("jpeg"));
    EXPECT_EQ(1u, cr_ui_thumbnail_format_supported("png"));
    EXPECT_EQ(1u, cr_ui_thumbnail_format_supported("tiff"));
    EXPECT_EQ(1u, cr_ui_thumbnail_format_supported("raw"));
    EXPECT_EQ(0u, cr_ui_thumbnail_format_supported("webp"));
    EXPECT_EQ(0u, cr_ui_thumbnail_format_supported("heic"));
    EXPECT_EQ(0u, cr_ui_thumbnail_format_supported(""));
    EXPECT_EQ(0x50030003u, cr_ui_thumbnail_format_supported(nullptr));

    /* N-144 re-pin: a text file NO LONGER refuses - the TxtViewFiles
     * dispatch renders the content-preview snapshot (the og's
     * :422447-455 branch); the refusal class is now the non-text
     * unsupported families (webp/heic/...). The text leg's success is
     * pinned by CrUi.TextSnapshotThumbnailForTxtNode. */
    EXPECT_EQ(1u, cr_ui_thumbnail_format_supported("txt"));
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_to_session("thumbun", u, img, crsf));
    const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(doc, 0u);
    cr_ui_thumbnail_info info{};
    const uint32_t trc = cr_ui_thumbnail_generate(u.dev, u.sess, doc, nullptr, nullptr, 0, &info);
    EXPECT_TRUE(trc == CR_UI_OK || trc == 0x50030008u)
        << "the text node answers the snapshot (OK when the head carries a "
           "renderable byte, Unsupported when it does not) - never the "
           "image-decode refusal class";
    if (trc == CR_UI_OK) {
        EXPECT_STREQ("txt", info.source_format);
    }
}

TEST(CrUi, ThumbnailSuccessPromotesGradeTo2) {
    Ui u;
    std::string img, crsf;
    /* the DELETED-PNG fixture: deleted non-carved files take the generic
     * grading path (below 2), so the promotion is observable */
    ASSERT_TRUE(scan_img_to_session("thumbgr", build_fat16_png_deleted(), u, img, crsf));
    const uint64_t logo =
        query_node_id(u.sess, "#OGO.PNG"); /* the og placeholder renders 0xE5 as # - N-172 */
    const uint64_t doc_id = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(logo, 0u) << "the deleted PNG must be a result row";
    ASSERT_NE(doc_id, 0u);
    const uint32_t g0 = grade_of(u.sess, "#OGO.PNG");
    ASSERT_NE(g0, 0xFFFFFFFFu);
    ASSERT_LT(g0, 2u) << "precondition: the deleted non-carved node starts below 2 "
                         "(otherwise the promotion test is vacuous)";
    const uint32_t gdoc = grade_of(u.sess, "DOC.TXT");
    ASSERT_NE(gdoc, 0xFFFFFFFFu);

    /* N-144 re-pin: the text node's thumbnail is the TEXT SNAPSHOT now -
     * when it succeeds the M-115 read-gated promotion fires (the og's own
     * +0x28 trigger class, FUN_1403d88e0's promotion seam); the failed-
     * thumbnail-never-promotes invariant is carried by the snapshot's
     * OWN refusal (the non-renderable head). The DOC.TXT fixture content
     * ('DOCDATA'-class printable bytes) succeeds, so the text node
     * promotes exactly like the og's text-snapshot population. */
    cr_ui_thumbnail_info info{};
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, doc_id, nullptr, nullptr, 0, &info),
              CR_UI_OK);
    EXPECT_EQ(2u, grade_of(u.sess, "DOC.TXT"))
        << "the successful text snapshot promotes (the og's +0x28 class)";

    /* M-083: the og's grading side-effect - thumbnail success promotes
     * the node to grade 2 (FUN_1402636e0 :422498-518), never downgrades,
     * and leaves other nodes alone */
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, logo, nullptr, nullptr, 0, &info), CR_UI_OK);
    EXPECT_EQ(2u, grade_of(u.sess, "#OGO.PNG"));
    EXPECT_EQ(2u, grade_of(u.sess, "DOC.TXT")) << "both promoted nodes keep their grade";

    /* idempotence: already-grade-2 nodes are a no-op (the != 2 guard) */
    ASSERT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, logo, nullptr, nullptr, 0, &info), CR_UI_OK);
    EXPECT_EQ(2u, grade_of(u.sess, "#OGO.PNG"));
}

/* M-079: the auto-save volume-label fill (the CLI is MSVC-only; this is
 * the established source-truth pin pattern - N-074/N-071 - with the
 * behavior itself proven on the Windows lane + the naming half pinned
 * in test_session). The fill site, the --label override, the letter
 * capture, and the truthed config comment must all stay shipped. */
TEST(CrUi, CliScanFillsAutoSaveVolumeLabel) {
    const std::string rrecov = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(rrecov.empty());
    /* the fill site exists and rides the identity-card fill */
    EXPECT_NE(rrecov.find("fill_volume_label(target, cfg)"), std::string::npos)
        << "the M-079 fill call must ship at scan setup";
    /* the derivation sources: the volume letter + GetVolumeInformationW,
     * and the image-stem fallback */
    EXPECT_NE(rrecov.find("target.volume_letter = v[0]"), std::string::npos)
        << "the --volume target's letter must be captured for the derivation";
    EXPECT_NE(rrecov.find("GetVolumeInformationW(root, label,"), std::string::npos)
        << "the label query (the og's GetVolumeName, rsf-format:167)";
    /* the explicit override flag + the usage line */
    EXPECT_NE(rrecov.find("L\"--label\""), std::string::npos) << "the --label override must parse";
    EXPECT_NE(rrecov.find("[--label s]"), std::string::npos) << "the usage text must show the flag";
    /* the config comment is truthful (the session-10 addendum's step 0:
     * the false 'the CLI/win layer reads it from the volume entry' claim
     * without the wiring - now the wiring exists, the comment names it) */
    const std::string scan_h = cr_test::read_repo_text("include/cr/orchestrator/cr_scan.h");
    ASSERT_FALSE(scan_h.empty());
    EXPECT_NE(scan_h.find("The CLI fills it at scan setup"), std::string::npos)
        << "the volume_label comment must describe the real wiring";
    EXPECT_NE(scan_h.find("M-079"), std::string::npos)
        << "the comment cites the finding that wired the fill";
    EXPECT_EQ(scan_h.find("the CLI/win layer reads it from the volume entry; engines"),
              std::string::npos)
        << "the pre-fix false-claim sentence (unwired) must stay gone";
}

TEST(CrUi, ThumbnailCapsAllowEmbeddedValidated) {
    /* N-137: the header's "validated 0/1" claim is ENFORCED - 0 and 1
     * are accepted (the knob's only two values; the embedded fast path
     * is always preferred - og parity, no off position), anything above
     * 1 is refused with ERR_BAD_PARAMETER. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("thumbcaps", build_fat16_jpg_trunc(), u, img, crsf));
    const uint64_t logo = query_node_id(u.sess, "LOGO2.JPG");
    ASSERT_NE(logo, 0u);
    cr_ui_thumbnail_info info{};
    for (const uint32_t v : {0u, 1u}) {
        cr_ui_thumbnail_caps caps{};
        caps.allow_embedded = v;
        const uint32_t rc = cr_ui_thumbnail_generate(u.dev, u.sess, logo, &caps, nullptr, 0, &info);
        EXPECT_NE(rc, 0x50030003u) << "allow_embedded=" << v << " must be accepted";
    }
    cr_ui_thumbnail_caps caps{};
    caps.allow_embedded = 7;
    EXPECT_EQ(0x50030003u, cr_ui_thumbnail_generate(u.dev, u.sess, logo, &caps, nullptr, 0, &info))
        << "an out-of-range allow_embedded must be refused (the header's claim, enforced)";
}

TEST(CrUi, ThumbnailHeaderMatchesContract) {
    /* N-137 doc-truth pin: the thumbnail section's error-code claims
     * must equal ABI-CONTRACT 7i's - the "never returned" denial of the
     * DecodeFailed->ERR_TRUNCATED mapping is gone, the refusal mapping
     * is stated, and the caps note records the no-off-position design. */
    const std::string hdr = cr_test::read_repo_text("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    const size_t gen = hdr.find("cr_ui_thumbnail_generate");
    ASSERT_NE(gen, std::string::npos);
    const std::string sec = hdr.substr(gen > 1600 ? gen - 1600 : 0, 2200);
    EXPECT_EQ(sec.find("never returned"), std::string::npos)
        << "the denial of the ERR_TRUNCATED refusal mapping must stay gone";
    EXPECT_NE(sec.find("decode REFUSAL"), std::string::npos)
        << "the refusal mapping must be stated in the section comment";
    const size_t caps = hdr.find("cr_ui_thumbnail_caps {");
    ASSERT_NE(caps, std::string::npos);
    const std::string capsec = hdr.substr(caps > 700 ? caps - 700 : 0, 900);
    EXPECT_NE(capsec.find("no off position"), std::string::npos)
        << "the allow_embedded design note must be in the caps comment";
    EXPECT_NE(capsec.find("0 or 1"), std::string::npos) << "the validated range must be stated";
}

/* ------------------------------------------------------------------ */
/* S-054: the destination-arming wiring fence                          */
/* ------------------------------------------------------------------ */

TEST(DestArming, ProductionWiringCallsSpansApi) {
    /* S-054 (the N-013 regression): the unrelated M-036 commit 1ebd1bcc
     * silently reverted BOTH production destination-arming sites back to
     * the pre-N-013 first-extent-only, drive-letter-only shape while the
     * helpers and their tests survived - the suite stayed green over a
     * SEVERE safety regression because nothing pinned the WIRING. This
     * fence fails the day either production path stops feeding the
     * writer every span of the destination volume (the 1ebd1bcc class).
     * Source-level pin: the arming seam is Windows-gated (the live-path
     * assertion runs on the Windows lane; this pin guards both). */
    const std::string cli = cr_test::read_repo_text("src/app/rrecov.cpp");
    EXPECT_NE(std::string::npos, cli.find("destination_volume_letter"))
        << "the CLI recover path must resolve ANY resolvable destination "
           "path (the drive-letter gate misses junction/relative/UNC)";
    EXPECT_NE(std::string::npos, cli.find("volume_extents"))
        << "the CLI recover path must fetch ALL extents of the destination";
    EXPECT_NE(std::string::npos, cli.find("set_destination_spans"))
        << "the CLI recover path must arm the writer with every span";

    const std::string ui = cr_test::read_repo_text("src/ui/cr_ui.cpp");
    EXPECT_NE(std::string::npos, ui.find("destination_volume_letter"))
        << "the cr_ui destination path must resolve any resolvable path";
    EXPECT_NE(std::string::npos, ui.find("volume_extents"))
        << "the cr_ui destination path must fetch ALL extents";
    EXPECT_NE(std::string::npos, ui.find("set_destination_spans"))
        << "the cr_ui destination path must arm every span";
}

TEST(DestArming, HeaderContractStillClaimsAllSpans) {
    /* S-054's contract half: the published cr_transport.h contract
     * ("destination arming must use volume_extents() (all spans)") must
     * stay in the header the integrators read - removing the contract
     * text to match broken code would be the N-135 doc-truth class. */
    const std::string hdr = cr_test::read_repo_text("include/cr/win/cr_transport.h");
    EXPECT_NE(std::string::npos,
              hdr.find("destination arming must use volume_extents() (all spans)"));
}

/* ---- M-116: the archive-inventory ABI (the two-call pattern) ---- */

TEST(CrUi, ArchiveEntryTwoCallPattern) {
    /* the browse-inside inventory crosses the C ABI: count probe, then
     * per-index fill, names/sizes/CRC intact, out-of-range refused,
     * non-archive nodes count 0 (never an error). */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_zip_to_session("arch", u, img, crsf));

    const uint64_t zip_id = query_node_id(u.sess, "STASH.ZIP");
    ASSERT_NE(zip_id, 0u) << "the zip fixture must scan into the tree";
    cr_ui_preview* pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, zip_id, &pv), CR_UI_OK);
    uint32_t count = 99;
    ASSERT_EQ(cr_ui_archive_entry_count(pv, &count), CR_UI_OK);
    ASSERT_EQ(count, 3u) << "the ArchivePreviewModel inventory at the ABI";

    cr_ui_archive_entry e{};
    ASSERT_EQ(cr_ui_archive_entry_at(pv, 0, &e), CR_UI_OK);
    EXPECT_STREQ(e.name, "readme.txt");
    EXPECT_EQ(e.size, 100u);
    EXPECT_EQ(e.compressed, 100u);
    EXPECT_EQ(e.crc_present, 1u);
    ASSERT_EQ(cr_ui_archive_entry_at(pv, 1, &e), CR_UI_OK);
    EXPECT_STREQ(e.name, "dir/photo.jpg");
    EXPECT_EQ(e.crc_present, 0u) << "the damaged-entry marker crosses (CRC 0)";
    ASSERT_EQ(cr_ui_archive_entry_at(pv, 2, &e), CR_UI_OK);
    EXPECT_STREQ(e.name, "data.bin");
    EXPECT_EQ(e.crc_present, 1u);
    /* out-of-range refused (not a crash, not silent) */
    EXPECT_NE(cr_ui_archive_entry_at(pv, 3, &e), CR_UI_OK);
    /* bad pointers refused */
    EXPECT_NE(cr_ui_archive_entry_count(pv, nullptr), CR_UI_OK);
    EXPECT_NE(cr_ui_archive_entry_at(nullptr, 0, &e), CR_UI_OK);
    cr_ui_preview_close(pv);

    /* a non-archive node: count 0, never an error (the empty browse) */
    const uint64_t doc = query_node_id(u.sess, "DOC.TXT");
    ASSERT_NE(doc, 0u);
    pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, doc, &pv), CR_UI_OK);
    ASSERT_EQ(cr_ui_archive_entry_count(pv, &count), CR_UI_OK);
    EXPECT_EQ(count, 0u);
    EXPECT_NE(cr_ui_archive_entry_at(pv, 0, &e), CR_UI_OK)
        << "no entries: every index is out of range";
    cr_ui_preview_close(pv);
}

TEST(CrUi, ArchiveEntryDamagedZipSalvageAndAbstain) {
    /* the damaged-zip contract at the ABI, both honest halves:
     * (a) a member's local header destroyed - the CD is self-contained,
     *     the inventory still enumerates (the og's family exists to
     *     browse damaged zips; the engine's M-066 salvage crosses);
     * (b) the CD itself unreachable (the EOCD's cd_off past EOF) - the
     *     salvage ABSTAINS: count 0, kind still archive, no error, no
     *     crash (the browse view renders an empty damaged listing). */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_zip_to_session("archlfh", u, img, crsf));
    {
        /* destroy the SECOND member's local header magic (mid-file):
         * the file image's zip body starts at cluster 60's data; the
         * second LFH sits 30+10+100 bytes into the body */
        FILE* f = std::fopen(img.c_str(), "r+b");
        ASSERT_NE(f, nullptr);
        const long body_at = 74 * 512 + 58 * 512; /* kDATA + (60-2)*512 */
        const long second_lfh = body_at + 30 + 10 + 100;
        ASSERT_EQ(std::fseek(f, second_lfh, SEEK_SET), 0);
        std::fputc(0xDE, f);
        std::fputc(0xAD, f);
        std::fputc(0xBE, f);
        std::fputc(0xEF, f);
        std::fclose(f);
    }
    /* re-scan the damaged image into a fresh session */
    {
        Ui u2;
        std::string img2 = img; /* scan the damaged image manually */
        ASSERT_EQ(cr_ui_open_image(img2.c_str(), 0, 0, &u2.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.progress = progress_cb;
        cfg.progress_user = &g_progress_calls;
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u2.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u2.scan, u2.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u2.scan), CR_UI_OK);
        const std::string csf2 = img2 + ".crsf";
        ASSERT_EQ(cr_ui_scan_save_session(u2.scan, csf2.c_str()), CR_UI_OK);
        cr_ui_scan_close(u2.scan);
        u2.scan = nullptr;
        ASSERT_EQ(cr_ui_session_load(csf2.c_str(), &u2.sess), CR_UI_OK);
        const uint64_t zip_id = query_node_id(u2.sess, "STASH.ZIP");
        ASSERT_NE(zip_id, 0u);
        cr_ui_preview* pv = nullptr;
        ASSERT_EQ(cr_ui_preview_open_session(u2.dev, u2.sess, zip_id, &pv), CR_UI_OK);
        uint32_t count = 0;
        ASSERT_EQ(cr_ui_archive_entry_count(pv, &count), CR_UI_OK);
        EXPECT_EQ(count, 3u) << "the CD walk does not need the local headers";
        cr_ui_archive_entry e{};
        ASSERT_EQ(cr_ui_archive_entry_at(pv, 0, &e), CR_UI_OK);
        EXPECT_STREQ(e.name, "readme.txt");
        cr_ui_preview_close(pv);
    }
    /* (b) the CD unreachable: abstain, no error */
    Ui u3;
    std::string img3, crsf3;
    ASSERT_TRUE(scan_zip_to_session("archcd", u3, img3, crsf3));
    {
        /* the zip body lives at cluster 60; find its EOCD by scanning
         * backward from the body start for the PK\x05\x06 signature,
         * then point cd_off (EOCD+16) far past EOF */
        FILE* f = std::fopen(img3.c_str(), "rb+");
        ASSERT_NE(f, nullptr);
        const long body_at = 74 * 512 + 58 * 512; /* kDATA + (60-2)*512 */
        ASSERT_EQ(std::fseek(f, body_at, SEEK_SET), 0);
        std::vector<unsigned char> body(4096);
        const size_t got = std::fread(body.data(), 1, body.size(), f);
        ASSERT_GT(got, 100u);
        long eocd = -1;
        for (size_t i = got - 4; i > 0; --i) {
            if (body[i] == 'P' && body[i + 1] == 'K' && body[i + 2] == 5 && body[i + 3] == 6) {
                eocd = long(i);
                break;
            }
        }
        ASSERT_GT(eocd, 0) << "the EOCD must be inside the zip body head";
        ASSERT_EQ(std::fseek(f, body_at + eocd + 16, SEEK_SET), 0);
        std::fputc(0x7F, f);
        std::fputc(0xFF, f);
        std::fputc(0xFF, f);
        std::fputc(0xFF, f);
        std::fclose(f);
    }
    {
        Ui u4;
        ASSERT_EQ(cr_ui_open_image(img3.c_str(), 0, 0, &u4.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.progress = progress_cb;
        cfg.progress_user = &g_progress_calls;
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u4.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u4.scan, u4.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u4.scan), CR_UI_OK);
        const std::string crsf4 = img3 + ".crsf";
        ASSERT_EQ(cr_ui_scan_save_session(u4.scan, crsf4.c_str()), CR_UI_OK);
        cr_ui_scan_close(u4.scan);
        u4.scan = nullptr;
        ASSERT_EQ(cr_ui_session_load(crsf4.c_str(), &u4.sess), CR_UI_OK);
        const uint64_t zip_id = query_node_id(u4.sess, "STASH.ZIP");
        ASSERT_NE(zip_id, 0u);
        cr_ui_preview* pv = nullptr;
        ASSERT_EQ(cr_ui_preview_open_session(u4.dev, u4.sess, zip_id, &pv), CR_UI_OK);
        cr_ui_metadata meta{};
        ASSERT_EQ(cr_ui_preview_metadata(pv, &meta), CR_UI_OK);
        EXPECT_EQ(std::string(meta.kind), "archive") << "the sniff still routes";
        uint32_t count = 99;
        ASSERT_EQ(cr_ui_archive_entry_count(pv, &count), CR_UI_OK);
        EXPECT_EQ(count, 0u) << "the unreachable CD abstains (no error, no crash)";
        cr_ui_preview_close(pv);
    }
}

TEST(CrUi, ArchiveEntryHostileBound512) {
    /* the 512-entry hostile bound at the ABI layer over an FS-origin
     * node (the exact file extent; the carve layer's extent arithmetic
     * is its own audited surface): a 700-member zip lists exactly 512
     * entries with truncated=1 (the preview budget model - the eager
     * extractor's cap, M-066's registered bound). */
    auto img = build_fat16();
    /* the 700-member stored zip (names f0000.bin..f0699.bin, 4-byte
     * bodies, real CRCs) */
    std::vector<unsigned char> zip;
    const auto z16 = [&zip](unsigned v) {
        zip.push_back((unsigned char)(v));
        zip.push_back((unsigned char)(v >> 8));
    };
    const auto z32 = [&zip](unsigned long v) {
        zip.push_back((unsigned char)(v));
        zip.push_back((unsigned char)(v >> 8));
        zip.push_back((unsigned char)(v >> 16));
        zip.push_back((unsigned char)(v >> 24));
    };
    char name[32];
    std::vector<unsigned> lho;
    std::vector<std::vector<unsigned char>> cds;
    for (unsigned i = 0; i < 700; ++i) {
        std::snprintf(name, sizeof(name), "f%04u.bin", i);
        lho.push_back(unsigned(zip.size()));
        const unsigned char s4[4] = {'P', 'K', 3, 4};
        zip.insert(zip.end(), s4, s4 + 4);
        z16(20);
        z16(0);
        z16(0);
        z16(0);
        z16(0);
        z32(0x01020304u);
        z32(4);
        z32(4);
        z16(unsigned(std::strlen(name)));
        z16(0);
        for (const char* c = name; *c; ++c)
            zip.push_back((unsigned char)(*c));
        zip.insert(zip.end(), 4, 'x');
        std::vector<unsigned char> cd;
        const auto c16 = [&cd](unsigned v) {
            cd.push_back((unsigned char)(v));
            cd.push_back((unsigned char)(v >> 8));
        };
        const auto c32 = [&cd](unsigned long v) {
            cd.push_back((unsigned char)(v));
            cd.push_back((unsigned char)(v >> 8));
            cd.push_back((unsigned char)(v >> 16));
            cd.push_back((unsigned char)(v >> 24));
        };
        const unsigned char c4[4] = {'P', 'K', 1, 2};
        cd.insert(cd.end(), c4, c4 + 4);
        c16(20);
        c16(20);
        c16(0);
        c16(0);
        c16(0);
        c16(0);
        c32(0x01020304u);
        c32(4);
        c32(4);
        c16(unsigned(std::strlen(name)));
        c16(0);
        c16(0);
        c16(0);
        c16(0);
        c32(0);
        c32(lho.back());
        for (const char* c = name; *c; ++c)
            cd.push_back((unsigned char)(*c));
        cds.push_back(std::move(cd));
    }
    const size_t cd_at = zip.size();
    size_t cd_len = 0;
    for (const auto& cd : cds) {
        zip.insert(zip.end(), cd.begin(), cd.end());
        cd_len += cd.size();
    }
    const unsigned char eocd[4] = {'P', 'K', 5, 6};
    zip.insert(zip.end(), eocd, eocd + 4);
    z16(0);
    z16(0);
    z16(700);
    z16(700);
    z32((unsigned long)cd_len);
    z32((unsigned long)cd_at);
    z16(0);

    /* the FAT fixture: STASH.ZIP on cluster 60 (the write_fixture_zip
     * shape, inlined here for the 700-member body) */
    const unsigned first_free = 60;
    const size_t need = (zip.size() + 511) / 512;
    for (unsigned c = 0; c < need; ++c) {
        put16(img, kFAT1 + (first_free + c) * 2, first_free + c + 1);
        put16(img, kFAT2 + (first_free + c) * 2, first_free + c + 1);
    }
    put16(img, kFAT1 + (first_free + need - 1) * 2, 0xFFFF);
    put16(img, kFAT2 + (first_free + need - 1) * 2, 0xFFFF);
    dentry(img, 3, "STASH   ZIP", 0x20, first_free, unsigned(zip.size()), false);
    std::memcpy(img.data() + kDATA + (first_free - 2) * 512, zip.data(), zip.size());

    const std::string path = cr_test::tmp_path("cr_ui", "arch700.img");
    FILE* f = std::fopen(path.c_str(), "wb");
    ASSERT_NE(f, nullptr);
    const size_t wn = std::fwrite(img.data(), 1, img.size(), f);
    std::fclose(f);
    ASSERT_EQ(wn, img.size());

    Ui u;
    ASSERT_EQ(cr_ui_open_image(path.c_str(), 0, 0, &u.dev), CR_UI_OK);
    cr_ui_scan_config cfg{};
    cfg.progress = progress_cb;
    cfg.progress_user = &g_progress_calls;
    ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    const std::string crsf = path + ".crsf";
    ASSERT_EQ(cr_ui_scan_save_session(u.scan, crsf.c_str()), CR_UI_OK);
    cr_ui_scan_close(u.scan);
    u.scan = nullptr;
    ASSERT_EQ(cr_ui_session_load(crsf.c_str(), &u.sess), CR_UI_OK);
    const uint64_t zip_id = query_node_id(u.sess, "STASH.ZIP");
    ASSERT_NE(zip_id, 0u);
    cr_ui_preview* pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, zip_id, &pv), CR_UI_OK);
    uint64_t pvsz = 0;
    ASSERT_EQ(cr_ui_preview_size(pv, &pvsz), CR_UI_OK);
    ASSERT_EQ(pvsz, zip.size()) << "the FS-origin node carries the exact file";
    uint32_t entries = 0;
    ASSERT_EQ(cr_ui_archive_entry_count(pv, &entries), CR_UI_OK);
    EXPECT_EQ(entries, 512u) << "the hostile bound at the ABI layer";
    cr_ui_archive_entry e{};
    ASSERT_EQ(cr_ui_archive_entry_at(pv, 511, &e), CR_UI_OK);
    EXPECT_EQ(std::string(e.name).size(), 9u) << "the 512th entry fills";
    cr_ui_metadata meta{};
    ASSERT_EQ(cr_ui_preview_metadata(pv, &meta), CR_UI_OK);
    EXPECT_EQ(meta.truncated, 1u) << "the bound discloses";
    cr_ui_preview_close(pv);
}

/* ---- A-013: the CLI resume-without-out truncate-before-read --------- */

TEST(CrUi, CliResumeSinkNeverTruncatesTheSessionSource) {
    /* A-013 (ACUTE): the CLI's --resume-without---out flow opened the
     * checkpoint sink "wb" on effective_out - which IS the resume file -
     * BEFORE the resume read, destroying the user's only session copy
     * via the program's own printed recovery command. The fix adopts the
     * N-014/CkSink shape at the CLI: every emit stages into <path>.tmp
     * and commits by rename, so the final path is never opened for
     * truncation at all. This pin (source-level: the exe is
     * Windows-only, the N-094 pattern) asserts the wiring:
     *   - the atomic sink + its commit callback are wired;
     *   - the staging open targets the TMP path, never effective_out;
     *   - the destructive pattern (a truncating open of effective_out)
     *     is gone;
     * The end-to-end round-trip (scan --out F; scan --resume F; tree
     * survives) rides the Windows CI junk-gate step (3b). */
    const std::string src = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(src.empty());

    /* the atomic sink exists and is wired with ALL FOUR callbacks */
    EXPECT_NE(std::string::npos, src.find("struct CliCkSink"))
        << "the A-013 atomic checkpoint sink (the N-014/CkSink shape)";
    EXPECT_NE(std::string::npos, src.find("cfg.checkpoint.commit = cli_ck_commit_cb;"))
        << "the commit-by-rename hook is wired";
    EXPECT_NE(std::string::npos, src.find("cfg.checkpoint.user = &ck_sink;"));
    EXPECT_NE(std::string::npos, src.find("MOVEFILE_REPLACE_EXISTING"))
        << "the replace-rename commit (the og's fresh-file crash safety)";

    /* the staging open targets the TMP path */
    EXPECT_NE(std::string::npos,
              src.find("_wfopen_s(&ck_sink.f, ck_sink.tmp_path.c_str(), L\"wb\")"))
        << "the sink's truncating open lands on the staging tmp only";

    /* the destructive pattern is GONE: no truncating open of the final
     * path anywhere (pre-fix: _wfopen_s(&ck_file, effective_out, "wb")
     * textually preceded the resume read) */
    EXPECT_EQ(std::string::npos, src.find("_wfopen_s(&ck_file, effective_out"))
        << "the pre-fix truncating open of the resume target";
    EXPECT_EQ(std::string::npos, src.find("_wfopen_s(&fin, effective_out.c_str(), L\"wb\")"))
        << "no other truncating open of the aliased path";

    /* the resume read still reads the FINAL path intact ("rb", after
     * the sink wiring - order-safe because the sink never truncates) */
    EXPECT_NE(std::string::npos, src.find("_wfopen_s(&fin, resume_path.c_str(), L\"rb\")"));

    /* the sink's error paths clean the staging tmp (no litter, and a
     * failed resume leaves the source byte-identical) */
    EXPECT_NE(std::string::npos, src.find("std::filesystem::remove(ck_sink.tmp_path, ec);"));
}

/* ===================================================================== */
/* M-115 (diff-to-fix): the og's grade promotion is READ-gated, not     */
/* decode-gated (DRWUI :422507-514, binary-verified 0x140263b2c-b9a) - */
/* a clean recovery read (FUN_1403eba50 verdict 0, :422442) promotes   */
/* the node to grade 2 and the later decode's failure never retracts   */
/* it. The core promoted only on decode success (under-promoting the   */
/* damaged-but-readable population) and had no <=0x14 work-item floor  */
/* (over-promoting tiny decodables).                                    */
/* ===================================================================== */

TEST(CrUi, ThumbnailPromotesOnReadNotDecode) {
    /* the damaged-but-readable case - the finding's dominant real-world
     * class: intact extents (every byte reads cleanly), corrupt IDAT
     * entropy (the strict PNG decoder refuses). The og: read verdict 0
     * -> grade 2. Fail-before: the core returned ERR_TRUNCATED at the
     * DecodeFailed map and never promoted. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("thumbrd", build_fat16_png_damaged(), u, img, crsf));
    /* the deleted first char renders as the og's '#' placeholder (N-172) */
    uint64_t damaged = query_node_id(u.sess, "#AMAGE.PNG");
    if (damaged == 0u)
        damaged = query_node_id(u.sess, "DAMAGE.PNG");
    ASSERT_NE(damaged, 0u) << "the deleted damaged PNG must be a result row";
    const char* dname = query_node_id(u.sess, "#AMAGE.PNG") != 0u ? "#AMAGE.PNG" : "DAMAGE.PNG";
    const uint32_t g0 = grade_of(u.sess, dname);
    ASSERT_NE(g0, 0xFFFFFFFFu);
    ASSERT_LT(g0, 2u) << "precondition: the deleted node starts below 2";

    cr_ui_thumbnail_info info{};
    const uint32_t rc =
        cr_ui_thumbnail_generate(u.dev, u.sess, damaged, nullptr, nullptr, 0, &info);
    /* the decode refuses the corrupt stream: ERR_TRUNCATED is the
     * honest refusal - but the READ was clean, so the promotion fired */
    EXPECT_EQ(0x50030004u, rc) << "the damaged stream still refuses the decode";
    EXPECT_EQ(2u, grade_of(u.sess, dname)) << "M-115: the clean read promotes - decode-independent";
}

TEST(CrUi, ThumbnailTinyFileDoesNotPromote) {
    /* the <=0x14 edge: the og's dispatcher skips work-item creation for
     * files with size <= 20 (DRWUI :421391-421403) - an 8-byte 1x1 PBM
     * decodes through the stb PNM path, but neither the og NOR the
     * post-fix core may promote through the thumbnail surface.
     * Fail-before: the pre-fix core returned Ok and promoted. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("thumbtiny", build_fat16_tiny_pbm(), u, img, crsf));
    const uint64_t tiny = query_node_id(u.sess, "#INY.PBM");
    ASSERT_NE(tiny, 0u) << "the deleted 8-byte PBM must be a result row";
    /* capture the grade BEFORE: the M-134 deep-read grading rule may
     * legitimately promote the node at scan completion (its preceding
     * sector is zero-filled - the og's prev-sector-tail-zero rule); the
     * M-115 claim is narrower: the THUMBNAIL surface must never promote
     * under the floor. Isolate by comparing before/after the call. */
    const uint32_t g_before = grade_of(u.sess, "#INY.PBM");
    ASSERT_NE(g_before, 0xFFFFFFFFu);
    cr_ui_thumbnail_info info{};
    EXPECT_EQ(0x50030008u,
              cr_ui_thumbnail_generate(u.dev, u.sess, tiny, nullptr, nullptr, 0, &info))
        << "the 20-byte work-item floor refuses the generation";
    EXPECT_EQ(g_before, grade_of(u.sess, "#INY.PBM"))
        << "no promotion through the thumbnail surface under the floor";
}

TEST(CrUi, ThumbnailUnreadableSpanNeverPromotes) {
    /* the read-verdict boundary: a span the reader CANNOT fetch (the
     * og's verdict -3 class, data-but-flagged-failed) does NOT promote
     * - read_ok stays false when the generation gained an unreadable
     * span. Built from the damaged fixture with an out-of-device
     * extent is not expressible here, so the pin is the engine truth
     * already covered by the UnreadableSpan tests plus this ABI-level
     * guard: the promotion ONLY fires on read_ok, never on rc alone. */
    const std::string src = cr_test::read_repo_text("src/ui/cr_ui.cpp");
    ASSERT_FALSE(src.empty());
    EXPECT_NE(src.find("if (thumb.read_ok && node->grade() != tree::Grade::High)"),
              std::string::npos)
        << "the promotion must be gated on the disclosed read verdict";
    EXPECT_EQ(src.find("if (st == preview::ThumbStatus::Ok) {\n        node->set_grade"),
              std::string::npos)
        << "the decode-gated promotion shape must stay gone";
}

TEST(CrUi, ThumbnailPromotesOnReadNotDecodeDimsRefusal) {
    /* M-115 (the S47 lane's convergence supplement to the parallel lane's
     * landing): the same read-evidence promotion proven at the DIMS seam -
     * a JPEG-shaped file with clean extents but NO SOF refuses at
     * jpeg_dims (native_dims), the other DecodeFailed exit; the og's
     * promote-on-read (FUN_1403eba50 verdict==0, the guard :422507-514)
     * fires identically. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("m115dims", build_fat16_jpg_corrupt(), u, img, crsf));
    const uint64_t bad = query_node_id(u.sess, "#AD.JPG");
    ASSERT_NE(bad, 0u) << "the deleted no-SOF JPEG must be a result row";
    const uint32_t g0 = grade_of(u.sess, "#AD.JPG");
    ASSERT_NE(g0, 0xFFFFFFFFu);
    ASSERT_LT(g0, 2u) << "precondition: the deleted non-carved node starts below 2";

    cr_ui_thumbnail_info info{};
    EXPECT_EQ(cr_ui_thumbnail_generate(u.dev, u.sess, bad, nullptr, nullptr, 0, &info), 0x50030004u)
        << "the decoder refusal still returns ERR_TRUNCATED";
    EXPECT_EQ(1u, info.truncated);
    EXPECT_EQ(2u, grade_of(u.sess, "#AD.JPG"))
        << "M-115: the clean-read dims-seam refusal promotes exactly like the og";
}

TEST(CrUi, PreviewPaneNeverPromotes) {
    /* M-115 doctrine pin: the og's category-3 "PreViewFile" request never
     * promotes through the thumbnail path (the != 3 clause at
     * :422507-514, the cat-3 early return at :652280-281); the core's ABI
     * has no category channel - the PREVIEW surface is the pane
     * equivalent and must leave grades untouched. */
    Ui u;
    std::string img, crsf;
    ASSERT_TRUE(scan_img_to_session("m115prev", build_fat16_jpg_corrupt(), u, img, crsf));
    const uint64_t bad = query_node_id(u.sess, "#AD.JPG");
    ASSERT_NE(bad, 0u);
    const uint32_t g0 = grade_of(u.sess, "#AD.JPG");
    ASSERT_NE(g0, 0xFFFFFFFFu);
    ASSERT_LT(g0, 2u) << "precondition: below 2 before the preview reads";

    cr_ui_preview* pv = nullptr;
    ASSERT_EQ(cr_ui_preview_open_session(u.dev, u.sess, bad, &pv), CR_UI_OK);
    uint64_t size = 0;
    ASSERT_EQ(cr_ui_preview_size(pv, &size), CR_UI_OK);
    EXPECT_EQ(sizeof(kJpgNoSof), size_t(size));
    std::vector<char> buf(64, 0);
    uint32_t placed = 0;
    ASSERT_EQ(cr_ui_preview_read(pv, 0, buf.data(), 32, &placed), CR_UI_OK);
    EXPECT_EQ(32u, placed);
    cr_ui_preview_close(pv); /* LSan: no leaked CrPreview/PreviewReader */
    EXPECT_EQ(g0, grade_of(u.sess, "#AD.JPG"))
        << "the preview surface never promotes - the pane doctrine";
}

TEST(CrUi, PauseStdinReaderMapsKeysOntoTheToken) {
    /* N-178's CLI leg (the exe is Windows-only, so this pins the verb's
     * source per the N-116 convention): --pause-stdin must map the
     * keyboard onto the SAME PausableToken the ABI pair drives - 'p' or
     * SPACE pauses, 'r' or Enter resumes, Ctrl-C stays cancel, and the
     * reader terminates on the token's cancel (never a blocking read:
     * the process can always exit). */
    const std::string src = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(src.empty());
    const size_t reader = src.find("DWORD WINAPI pause_stdin_reader(");
    ASSERT_NE(std::string::npos, reader) << "the pause reader must exist";
    const std::string body = src.substr(reader, 2400);
    /* the pause keys */
    EXPECT_NE(std::string::npos, body.find("ch == L'p' || ch == L'P' || ch == L' '"))
        << "'p'/SPACE must pause";
    EXPECT_NE(std::string::npos, body.find(".pause();"))
        << "the pause key must drive the token's pause()";
    /* the resume keys */
    EXPECT_NE(std::string::npos, body.find("ch == L'r' || ch == L'R' || ch == L'\\r'"))
        << "'r'/Enter must resume";
    EXPECT_NE(std::string::npos, body.find(".resume();"))
        << "the resume key must drive the token's resume()";
    /* the never-blocking posture: the console poll with a timeout */
    EXPECT_NE(std::string::npos, body.find("WaitForSingleObject(in, 200)"))
        << "the reader must poll with a timeout, never block";
    EXPECT_NE(std::string::npos, body.find("g_cancel_token.cancelled()"))
        << "the reader must terminate on the token's cancel";
    /* the flag parses into the scan verb */
    EXPECT_NE(std::string::npos, src.find("--pause-stdin")) << "--pause-stdin must be a scan flag";
    /* the usage block documents the surface */
    EXPECT_NE(std::string::npos, src.find("[--pause-stdin]"))
        << "the usage block must list the flag";
}

/* ---- M-154: the auto-session lifecycle (retention-one + pointer) ------ */

TEST(CrUi, AutosaveRetentionDeletesPreviousAndPointerNamesIt) {
    /* M-154: the og's retention loop (DRWUI:442815-442833) - before
     * writing a new auto session, the previous auto .rsf named by the
     * LastScanInfo pointer is DELETED: exactly one auto session is ever
     * live. Two consecutive auto-saving scans in one dir leave exactly
     * ONE file, and the pointer names it (the og's [Info] four fields,
     * :443060-443086 - the core's last_scan.info sidecar). */
    const std::string img = write_fixture("m154ret");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/m154_ret_" + std::to_string(CR_TEST_PID());
    std::filesystem::remove_all(adir);
    std::filesystem::create_directories(adir);

    for (int round = 0; round < 2; ++round) {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        cr_ui_scan_stats st{};
        ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
        EXPECT_EQ(st.autosaved, 1u) << "round " << round;
    }
    /* exactly ONE auto session remains - the second round's retention
     * deleted the first's */
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
    EXPECT_EQ(n, 1u) << "retention-one: one live auto session after two rounds";
    /* the pointer names it (the pointed resume) */
    char pointed[1024] = {0};
    ASSERT_EQ(cr_ui_session_last_autosave_path(adir.c_str(), pointed, sizeof(pointed)), CR_UI_OK);
    const std::string pointed_s(pointed);
    EXPECT_NE(std::strstr(pointed_s.c_str(), "auto_"), nullptr)
        << "the pointer names the auto session";
    EXPECT_TRUE(std::filesystem::exists(pointed_s)) << "the pointed file is present";
    /* the pointer's four fields: read the sidecar directly */
    cr::session::LastScanInfo info;
    ASSERT_TRUE(cr::session::last_scan_info_read(adir, info));
    EXPECT_EQ(info.save_path, pointed_s);
    EXPECT_NE(std::strstr(info.name.c_str(), "auto_"), nullptr);
    EXPECT_GT(info.name.size(), 5u);
    EXPECT_GT(info.count, 0u) << "the og's Count field carries the node count";
    EXPECT_GT(info.time, 0ull) << "the og's Time field carries the save stamp";
    EXPECT_TRUE(info.exists);
}

TEST(CrUi, AutosaveManualSaveSurvivesRetention) {
    /* M-154: manual saves are NEVER deleted by the retention pass (the
     * og's DeleteFileA targets the pointer's SavePath, which only ever
     * names an auto_*.rsf the writer itself recorded; the core's
     * auto_-name gate is the safety hardening - a corrupted pointer
     * naming a manual save must not delete it). */
    const std::string img = write_fixture("m154man");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/m154_man_" + std::to_string(CR_TEST_PID());
    std::filesystem::remove_all(adir);
    std::filesystem::create_directories(adir);

    /* a manual save first (the explicit session sink) */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        const std::string manual = adir + "/manual_save.crsf";
        EXPECT_EQ(cr_ui_scan_save_session(u.scan, manual.c_str()), CR_UI_OK);
    }
    /* then an auto-saving run: the retention must keep the manual file */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    }
    EXPECT_TRUE(std::filesystem::exists(adir + "/manual_save.crsf"))
        << "the manual save survives the retention pass";
    uint32_t n = 0;
    ASSERT_EQ(cr_ui_session_list(adir.c_str(), nullptr, &n), CR_UI_OK);
    EXPECT_EQ(n, 2u) << "the manual file + the one live auto session";
}

TEST(CrUi, AutosaveDeadPointerDoesNotBlock) {
    /* M-154: the og's PathFileExistsA validation (DRWUI:443209-443214)
     * - a pointer whose SavePath is dead (the file deleted out-of-band)
     * never blocks the new save; the pointed-resume surface reports
     * absence honestly (the host falls back to the list). */
    const std::string img = write_fixture("m154dead");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/m154_dead_" + std::to_string(CR_TEST_PID());
    std::filesystem::remove_all(adir);
    std::filesystem::create_directories(adir);

    /* round 1: a live auto session + pointer */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    }
    /* kill the pointed file out-of-band (the dead-pointer shape) */
    char pointed[1024] = {0};
    ASSERT_EQ(cr_ui_session_last_autosave_path(adir.c_str(), pointed, sizeof(pointed)), CR_UI_OK);
    std::filesystem::remove(pointed);
    /* the pointed resume now reports absence */
    char again[1024] = {0};
    EXPECT_EQ(cr_ui_session_last_autosave_path(adir.c_str(), again, sizeof(again)), 0x50020004u)
    /* ERR_RANGE_OUT_OF_BOUNDS: no live pointer */;
    /* round 2: the dead pointer never blocks the new save */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
        cr_ui_scan_stats st{};
        ASSERT_EQ(cr_ui_scan_get_stats(u.scan, &st), CR_UI_OK);
        EXPECT_EQ(st.autosaved, 1u) << "the dead pointer never blocks the save";
    }
    /* the pointer now names the new session (the same-second shape can
     * regenerate the identical name - the existence is the assertion) */
    char fresh[1024] = {0};
    ASSERT_EQ(cr_ui_session_last_autosave_path(adir.c_str(), fresh, sizeof(fresh)), CR_UI_OK);
    EXPECT_TRUE(std::filesystem::exists(fresh)) << "the pointer names the new session";
}

/* Session-84 (coherence): the pointed-resume ABI half - cr_ui.h:927
 * declared cr_ui_session_resume_last in b2bc1970 but the racing window
 * dropped the implementation (the phantom-export class: MSVC LNK2001
 * the moment Build+test runs; the Linux build silently emits nothing).
 * The pin covers every contract arm of cr_ui.h:917-927: the pointed
 * load through the shared path (no enumeration, no sort), BAD_PARAMETER
 * on null args, and the dead-pointer classes as ERR_OPEN_FAILED
 * (0x50020001) - including the two tamper refusals (the S82 rider's
 * rule extended to the load: a sidecar whose fields disagree, or whose
 * name is not an auto session, is refused, never followed). */
TEST(CrUi, SessionResumeLastLoadsPointedAutoAndRefusesTampered) {
    const std::string img = write_fixture("m154rl");
    ASSERT_FALSE(img.empty());
    const std::string dir = std::filesystem::path(img).parent_path().string();
    const std::string adir = dir + "/m154_rl_" + std::to_string(CR_TEST_PID());
    std::filesystem::remove_all(adir);
    std::filesystem::create_directories(adir);

    /* one auto-saving run: the auto session + the pointer */
    {
        Ui u;
        ASSERT_EQ(cr_ui_open_image(img.c_str(), 0, 0, &u.dev), CR_UI_OK);
        cr_ui_scan_config cfg{};
        cfg.autosave_dir_utf8 = adir.c_str();
        ASSERT_EQ(cr_ui_scan_create(&cfg, &u.scan), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_attach_device(u.scan, u.dev), CR_UI_OK);
        ASSERT_EQ(cr_ui_scan_run(u.scan), CR_UI_OK);
    }
    /* (1) the pointed load: same session the direct load sees */
    char pointed[1024] = {0};
    ASSERT_EQ(cr_ui_session_last_autosave_path(adir.c_str(), pointed, sizeof(pointed)), CR_UI_OK);
    cr_ui_session* direct = nullptr;
    ASSERT_EQ(cr_ui_session_load(pointed, &direct), CR_UI_OK);
    cr_ui_scan_stats dst{};
    ASSERT_EQ(cr_ui_session_stats(direct, &dst), CR_UI_OK);
    cr_ui_session* resumed = nullptr;
    ASSERT_EQ(cr_ui_session_resume_last(adir.c_str(), &resumed), CR_UI_OK);
    cr_ui_scan_stats rst{};
    ASSERT_EQ(cr_ui_session_stats(resumed, &rst), CR_UI_OK);
    EXPECT_EQ(rst.files_found, dst.files_found)
        << "the pointed resume loads the same session as the direct load";
    cr_ui_session_close(resumed);
    cr_ui_session_close(direct);

    /* (2) null args: BAD_PARAMETER */
    EXPECT_EQ(cr_ui_session_resume_last(nullptr, &resumed), CR_UI_ERR_BAD_PARAMETER);
    EXPECT_EQ(cr_ui_session_resume_last(adir.c_str(), nullptr), CR_UI_ERR_BAD_PARAMETER);

    /* (3) a dir with no sidecar: ERR_OPEN_FAILED */
    const std::string no_ptr = adir + "_nopointer";
    std::filesystem::create_directories(no_ptr);
    EXPECT_EQ(cr_ui_session_resume_last(no_ptr.c_str(), &resumed), 0x50020001u)
    /* ERR_OPEN_FAILED: no pointer sidecar */;

    /* (4) tampered name (not an auto session): refused, never followed -
     * save_path still names the real, loadable file */
    cr::session::LastScanInfo good;
    ASSERT_TRUE(cr::session::last_scan_info_read(adir, good));
    cr::session::LastScanInfo info = good;
    info.name = "manual_save.crsf";
    ASSERT_TRUE(cr::session::last_scan_info_write(adir, info));
    EXPECT_EQ(cr_ui_session_resume_last(adir.c_str(), &resumed), 0x50020001u)
    /* ERR_OPEN_FAILED: the pointer does not name an auto session */;

    /* (5) tampered save_path (fields disagree): refused even though the
     * target is a real, loadable session - the smuggled-path refusal */
    const std::string decoy = adir + "/decoy_save.crsf";
    std::filesystem::copy_file(good.save_path, decoy);
    info = good;
    info.save_path = decoy;
    ASSERT_TRUE(cr::session::last_scan_info_write(adir, info));
    EXPECT_EQ(cr_ui_session_resume_last(adir.c_str(), &resumed), 0x50020001u)
    /* ERR_OPEN_FAILED: save_path does not name the gated name */;

    /* (6) dead pointer (the pointed file deleted out-of-band) */
    ASSERT_TRUE(cr::session::last_scan_info_write(adir, good));
    std::filesystem::remove(good.save_path);
    std::filesystem::remove(decoy);
    EXPECT_EQ(cr_ui_session_resume_last(adir.c_str(), &resumed), 0x50020001u)
    /* ERR_OPEN_FAILED: the pointed file does not exist */;
}

/* Session-84 (coherence): the image verb's --create selector - the
 * five-site surface (dispatch / -h / usage header / docs / the parser)
 * was four-site until now: 13fe7258 landed the verb with the documented
 * form `image --create --disk N --out file.img` (usage header :43, -h
 * block, the driver-era doc set, PLAN-API-EXPANSION Wave 4,
 * CI-SCENARIOS.md:30, REFERENCE-API-GAP.md:336/:435, and
 * Run-Benchmark.ps1's image-create probe) while cmd_image's parser only
 * accepted --disk/--out - the documented invocation died at "unknown
 * image option: --create" (exit 1). The parser site is pinned here the
 * established source-truth way (M-079/N-074/N-071 pattern); the create
 * flow itself is Windows-lane behavior (privilege class (a)). */
TEST(CrUi, CliImageCreateFlagParses) {
    const std::string rrecov = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(rrecov.empty());
    /* the parser accepts the documented mode selector as a valueless flag */
    EXPECT_NE(rrecov.find("has_flag(argc, argv, i, L\"--create\")"), std::string::npos)
        << "cmd_image must accept the documented --create selector "
           "(the five-site surface's parser site)";
    /* the four pre-existing sites stay shipped: the usage header form,
     * the -h form, the dispatch, and the --disk/--out pair */
    EXPECT_NE(rrecov.find("image --create --disk N --out file.img"), std::string::npos)
        << "the usage header's documented form must stay";
    EXPECT_NE(rrecov.find("L\"image\""), std::string::npos) << "the dispatch site must stay";
    EXPECT_NE(rrecov.find("image needs --disk N and --out file.img"), std::string::npos)
        << "the missing-args refusal must stay (the parser still demands both)";
}

/* Session-84 (coherence): the disconnect hint must name a real verb - the
 * N-125 exit-5 message at rrecov.cpp:2092 said `rrecov resume %ls`, but no
 * `resume` verb exists in the dispatch (the 13-verb table has no such
 * entry; the real reattach path is `scan --resume <file> --disk N |
 * --volume C:`, parsed at rrecov.cpp:1559 and stated by the sibling
 * cancel-resume message at :1921). A hint that names a nonexistent
 * command is the user-facing wiring class this session hunts. */
TEST(CrUi, CliDisconnectHintNamesRealVerb) {
    const std::string rrecov = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(rrecov.empty());
    EXPECT_NE(rrecov.find("rrecov scan --resume %ls"), std::string::npos)
        << "the exit-5 disconnect hint must name the real resume form "
           "(scan --resume), never a phantom `rrecov resume` verb";
}

/* Session-84 (coherence, D14): the -h block's scan section omitted six
 * live flags (--autosave-dir/--label/--pause-stdin/--user-signs/
 * --bitlocker-dict/--check) while the parse loop accepted them - the
 * N-094 class (a surface invisible in the product's own help), flagged
 * by the Phase-0 CLI sheet and repaired in the same session. The pin
 * holds the invariant the same way N-094 holds the verb invariant:
 * every flag cmd_scan's parse loop accepts must appear in the -h usage
 * block. */
TEST(CrUi, CliHelpListsEveryScanFlag) {
    const std::string src = cr_test::read_repo_text("src/app/rrecov.cpp");
    ASSERT_FALSE(src.empty());

    /* the scan parse loop: from cmd_scan's opening to the refusal line */
    const size_t scan_start = src.find("int cmd_scan(");
    ASSERT_NE(std::string::npos, scan_start) << "cmd_scan not found";
    const size_t scan_end = src.find("scan needs --disk", scan_start);
    ASSERT_NE(std::string::npos, scan_end) << "scan parse loop end not found";
    const std::string scan_body = src.substr(scan_start, scan_end - scan_start);

    /* every flag the loop accepts, both valueless and valued */
    std::vector<std::string> flags;
    for (size_t pos = 0; (pos = scan_body.find("L\"--", pos)) != std::string::npos;) {
        const size_t start = pos + 3;
        const size_t end = scan_body.find('"', start);
        ASSERT_NE(std::string::npos, end);
        const std::string name = scan_body.substr(start, end - start);
        /* full-match flag names only: prose fragments (e.g. a comment
         * quoting "--quick-only disables it") must not count as flags */
        if (name.size() > 2 &&
            name.find_first_not_of("-abcdefghijklmnopqrstuvwxyz0123456789") == std::string::npos)
            flags.push_back(name);
        pos = end;
    }
    ASSERT_GE(flags.size(), 20u) << "scan-flag extraction broke (found " << flags.size() << ")";

    /* the -h usage block: the wprintf'd help between the scan verb line
     * and the tree verb line */
    const size_t h_start = src.find("rrecov scan --disk N | --volume C: | --image file.vmdk");
    ASSERT_NE(std::string::npos, h_start) << "-h scan section not found";
    const size_t h_end = src.find("rrecov tree --in", h_start);
    ASSERT_NE(std::string::npos, h_end) << "-h tree line not found";
    const std::string help_block = src.substr(h_start, h_end - h_start);

    for (const auto& f : flags) {
        /* --volume/--disk/--image are target selectors, already on the
         * first line; every other parsed flag must appear in the block */
        if (f == "--disk" || f == "--volume" || f == "--image")
            continue;
        EXPECT_NE(std::string::npos, help_block.find(f))
            << "the -h scan section must list the parsed flag " << f
            << " (the N-094 class: a surface invisible in the product's own help)";
    }
}
