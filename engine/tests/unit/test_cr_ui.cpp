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
#include "cr/cr_log.h"
/* S-009 pure-builder tests: the core symbols resolve from libcr_ui
 * (which embeds the core); these includes are for the types only. */
#include "cr/storage/cr_device.h"
#include "cr/tree/cr_diskmap.h"
#include "cr/tree/cr_node.h"

#include "test_tmpdir.hpp"

#include <atomic>
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

TEST(CrUi, AutoSaveOnCompletion) {
    /* W1c (R-52): the og's completion SaveScan flow - zero-init = OFF;
     * on a COMPLETED run the auto session lands in autosave_dir under
     * the N-061 auto_<label>_<ts> naming; the event fires ONCE with the
     * rc and stats.autosaved flips; a CANCELLED run never auto-saves;
     * the W1a list reads the auto session back (the surfaces compose). */
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

    /* cancel while parked: stopping flips immediately, busy stays true
     * while the run winds down through the gate */
    ASSERT_EQ(cr_ui_scan_cancel(u.scan), CR_UI_OK);
    ASSERT_EQ(cr_ui_scan_stopping(u.scan, &stopping), CR_UI_OK);
    EXPECT_EQ(stopping, 1u) << "the live cancel request must be visible at once";
    ASSERT_EQ(cr_ui_scan_busy(u.scan, &busy), CR_UI_OK);
    EXPECT_EQ(busy, 1u) << "run() is still winding down after cancel";

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
    const std::string doc = cr_test::read_repo_file("docs/ABI-CONTRACT.md");
    ASSERT_FALSE(doc.empty());
    EXPECT_NE(doc.find("Fired on engine worker threads, the thread running `run()`, or the"),
              std::string::npos)
        << "ABI-CONTRACT must carry the truthful thread contract (the N-043 "
           "heartbeat watchdog is the third delivery thread)";
    EXPECT_EQ(doc.find("Fired on the thread running `run()` only"), std::string::npos)
        << "the false run()-thread-only claim must never return";
    const std::string hdr = cr_test::read_repo_file("include/cr/cr_ui.h");
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
    const std::string hdr = cr_test::read_repo_file("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("ABI-since 1.1"), std::string::npos)
        << "no v1.1 marker ever shipped - the label must not return";
    EXPECT_EQ(hdr.find("ABI-since 1.2"), std::string::npos)
        << "no v1.2 marker ever shipped - the label must not return";
    const std::string doc = cr_test::read_repo_file("docs/ABI-CONTRACT.md");
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
    /* the deleted 8.3 name surfaces as "_ELFILE.TXT" (the 0xE5 marker
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
     * diagnostic (ABI-since 2.2, additive). DRIVER-ONLY SESSION UPDATE:
     * the clean-state-zero premise is stale - there is no direct leg on
     * Windows anymore. The current contract (docs/DRIVER-REQUIRED.md
     * section 6): on a driverless host the open REFUSES with the
     * classified ERR_DRIVER_ABSENT code and the advisory carries that
     * class (the presentation channel for hosts that surface transport
     * diagnostics); on a driver-attached host the grammar open succeeds
     * and a successful open resets the advisory to 0. The Linux stub
     * still answers UNSUPPORTED and records nothing. Fail-before (this
     * session): the pre-driver-only Windows leg asserted the direct-open
     * clean state that no longer exists. */
    cr_ui_device* d = nullptr;
    const uint32_t rc = cr_ui_open_disk(0, &d);
#ifdef _WIN32
    cr_ui_driver_status_t st;
    ASSERT_EQ(cr_ui_driver_status(&st), 0u) << "the status query itself must succeed";
    if (st.state == CR_UI_DRIVER_STATE_READY) {
        EXPECT_EQ(rc, 0u) << "driver-attached host: the grammar open succeeds";
        ASSERT_NE(d, nullptr);
        cr_ui_device_close(d);
        EXPECT_EQ(cr_ui_last_transport_error(), 0u) << "success resets the advisory";
    } else {
        EXPECT_EQ(rc, 0x500B0001u) << "driverless host: the classified ERR_DRIVER_ABSENT refusal";
        EXPECT_EQ(d, nullptr);
        EXPECT_EQ(cr_ui_last_transport_error(), 0x500B0001u)
            << "the advisory carries the refusal class (DRIVER-REQUIRED.md section 6)";
    }
#else
    EXPECT_EQ(rc, CR_UI_ERR_UNSUPPORTED);
    EXPECT_EQ(cr_ui_last_transport_error(), 0u);
#endif
}

/* ---- api-fix session: ABI doc-truth pins (the driver-era drift batch) ---- */

TEST(CrUi, AbiHeaderNamesNoPhantomErrorMacros) {
    /* api-fix: cr_ui.h once documented `CR_UI_ERR_INVALID_PARAM` in the
     * query_apply_buckets comment - a macro that never existed (the
     * implementation returns ERR_BAD_PARAMETER, 0x50030003). No
     * phantom identifier may return to the header. */
    const std::string hdr = cr_test::read_repo_file("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    EXPECT_EQ(hdr.find("CR_UI_ERR_INVALID_PARAM"), std::string::npos)
        << "the header must not name a macro that does not exist";
}

TEST(CrUi, TransportDocCarriesNoDirectValue) {
    /* api-fix: the cr_ui_device_transport comment once listed "direct"
     * as a third value - unreachable since PLAN-DRIVER-ONLY (the code
     * only ever sets "rda"/"image"). The stale value must not return. */
    const std::string hdr = cr_test::read_repo_file("include/cr/cr_ui.h");
    ASSERT_FALSE(hdr.empty());
    const size_t at = hdr.find("cr_ui_device_transport");
    ASSERT_NE(at, std::string::npos);
    /* the contract comment block sits just above the declaration */
    const std::string around = hdr.substr(at > 600 ? at - 600 : 0, 700);
    EXPECT_EQ(around.find("\"rda\", \"direct\""), std::string::npos)
        << "the transport contract must not advertise the removed direct value";
}

TEST(CrUi, AbiContractCarriesDriverEraTruth) {
    /* api-fix: ABI-CONTRACT's normative error table (section 4) must
     * carry the ERR_DRIVER_* family the ABI opens actually return, and
     * the event block (section 6) must carry CR_EVT_FS_CHECK (type 10)
     * with the M-041 struct tail. The section 7e rewrite must not
     * resurrect the removed RDA->direct fallback story. */
    const std::string doc = cr_test::read_repo_file("docs/ABI-CONTRACT.md");
    ASSERT_FALSE(doc.empty());
    EXPECT_NE(doc.find("ERR_DRIVER_ABSENT"), std::string::npos)
        << "section 4 must list the driver-prerequisite refusal family";
    EXPECT_NE(doc.find("CR_EVT_FS_CHECK"), std::string::npos)
        << "section 6 must list event type 10";
    EXPECT_NE(doc.find("check_errors"), std::string::npos)
        << "section 6's struct sketch must carry the M-041 tail fields";
    EXPECT_NE(doc.find("0x500B0005"), std::string::npos)
        << "the full 0x500B family belongs in the error table";
    /* the removed fallback story must stay gone */
    const size_t s7e = doc.find("### 7e.");
    ASSERT_NE(s7e, std::string::npos);
    const std::string tail = doc.substr(s7e, 2200);
    EXPECT_EQ(tail.find("direct fallback succeeds"), std::string::npos)
        << "section 7e must describe the driver-only advisory, not the removed fallback";
    EXPECT_EQ(tail.find("WinPE environment gate"), std::string::npos)
        << "the WinPE gate was removed with the fallback";
}

TEST(CrUi, DriverWireCodesFreeOfPhantomZeroInDocs) {
    /* api-fix: the extended IOCTL codes are 0x6002440/0x6002444/0x6002448
     * (CTL_CODE_RDA(fn), wire-truth fix commit 3bda8e2). The
     * phantom-zero 0x6000244x literals were swept from the normative
     * docs and code comments; this pin keeps them out. The identity
     * codes 0x60002400/0x60002404/0x60002408 are RAW values and stay. */
    const char* const files[] = {
        "docs/DRIVER-REQUIRED.md",
        "docs/DRIVER-CONTRACT.md",
        "docs/DRIVER-RELEASE.md",
        "docs/PLAN-DRIVER-ONLY.md",
        "docs/CORE-API-CURRENT.md",
        "docs/CORE-DISK-ACCESS.md",
        "docs/CAPABILITY-MATRIX.md",
        "docs/PARITY-DELTAS.md",
        "include/cr/win/cr_driver_status.h",
        "src/win/cr_transport.cpp",
    };
    for (const char* f : files) {
        const std::string txt = cr_test::read_repo_file(f);
        ASSERT_FALSE(txt.empty()) << f;
        EXPECT_EQ(txt.find("0x60002440"), std::string::npos)
            << f << ": phantom-zero GET_INFO literal";
        EXPECT_EQ(txt.find("0x60002444"), std::string::npos)
            << f << ": phantom-zero OPEN_TARGET literal";
        EXPECT_EQ(txt.find("0x60002448"), std::string::npos)
            << f << ": phantom-zero GET_VERSION literal";
    }
}
