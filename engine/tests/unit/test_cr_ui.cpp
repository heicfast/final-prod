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

#include "test_tmpdir.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <future>
#include <vector>

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
    cr_ui_query_rows_free(rows, n);

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

    /* name sort + paging: the deleted 8.3 name is recovered as "_ELFILE.TXT"
     * (the 0xE5 marker destroys the first character; '_' sorts last), so the
     * ascending order is DOC.TXT, PHOTO.JPG, _ELFILE.TXT -> page [1,1] */
    q = cr_ui_query{};
    q.sort = 1; /* name */
    q.offset = 1;
    q.limit = 1;
    ASSERT_EQ(cr_ui_query_run(u.sess, &q, &rows, &n, &total), CR_UI_OK);
    ASSERT_EQ(n, 1u);
    EXPECT_STREQ(rows[0].name, "PHOTO.JPG");
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
    const std::string renamed = dest + "/DOC (2).TXT";
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
        case CR_EVT_FILES_FOUND:
            ++files_events;
            break;
        case CR_EVT_CHECKPOINT:
            ++checkpoints;
            break;
        default:
            if (e->type > CR_EVT_LOST_CANDIDATE)
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
    /* stage order: quick before deep before carve */
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
    EXPECT_LT(quick_i, deep_i);
    EXPECT_LT(deep_i, carve_i);
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
    EXPECT_GE(got.cell_bytes, 65536ull); /* default granularity class */
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
