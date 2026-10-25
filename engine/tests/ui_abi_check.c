/*
 * ui_abi_check.c - the cr_ui C-consumer ABI check (GAP G-01 / issue #1,
 * UI-READINESS-PLAN W2-3): proves the header compiles and links with a
 * PLAIN C compiler (no C++ runtime, no C++ types) and drives the FULL
 * lifecycle against a synthetic FAT16 image, 1:1 with the CLI flows:
 *   list(enumeration surface) -> scan -> save session -> tree/snapshot
 *   -> query -> preview -> recover -> report.
 *
 * Build/run: linked against the cr_ui shared library; the image path is
 * argv[1] (a scratch file the test creates and removes). Exit 0 = green.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------------------------------------------------------------- */
/* tiny check macros (no framework - plain C is the point)           */
/* ---------------------------------------------------------------- */

static int g_failed = 0;

#define CHECK(cond)                                                                                \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                 \
            g_failed = 1;                                                                          \
        }                                                                                          \
    } while (0)

#define CHECK_EQ_U64(a, b)                                                                         \
    do {                                                                                           \
        unsigned long long a_ = (unsigned long long)(a);                                           \
        unsigned long long b_ = (unsigned long long)(b);                                           \
        if (a_ != b_) {                                                                            \
            printf("FAIL %s:%d: %s == %s (%llu != %llu)\n", __FILE__, __LINE__, #a, #b, a_, b_);   \
            g_failed = 1;                                                                          \
        }                                                                                          \
    } while (0)

/* ---------------------------------------------------------------- */
/* synthetic FAT16 volume (the test_orchestrator fixture, ported)    */
/* ---------------------------------------------------------------- */

static void put16(unsigned char* b, size_t at, unsigned x) {
    b[at] = (unsigned char)(x & 0xFFu);
    b[at + 1] = (unsigned char)((x >> 8) & 0xFFu);
}
static void put32(unsigned char* b, size_t at, unsigned long x) {
    b[at] = (unsigned char)(x & 0xFFu);
    b[at + 1] = (unsigned char)((x >> 8) & 0xFFu);
    b[at + 2] = (unsigned char)((x >> 16) & 0xFFu);
    b[at + 3] = (unsigned char)((x >> 24) & 0xFFu);
}

#define IMG_SECS 5074u
#define FAT1_OFF (2u * 512u)
#define FAT2_OFF (22u * 512u)
#define ROOT_OFF (42u * 512u)
#define DATA_OFF (74u * 512u)

static void dentry(unsigned char* img, size_t slot, const char* n11, unsigned attr, unsigned cl,
                   unsigned sz, int del) {
    unsigned char* e = img + ROOT_OFF + slot * 32u;
    size_t i;
    memset(e, 0, 32);
    for (i = 0; i < 11; ++i)
        e[i] = (unsigned char)n11[i];
    if (del)
        e[0] = 0xE5u;
    e[11] = (unsigned char)attr;
    put16(img, ROOT_OFF + slot * 32u + 20, (unsigned)(cl >> 16));
    put16(img, ROOT_OFF + slot * 32u + 26, (unsigned)(cl & 0xFFFFu));
    put32(img, ROOT_OFF + slot * 32u + 28, sz);
}

static int build_fat16(const char* path) {
    unsigned char* img = (unsigned char*)calloc(IMG_SECS, 512u);
    FILE* f;
    size_t w;
    if (!img)
        return 0;
    memcpy(img + 3, "MSWIN4.1", 8);
    put16(img, 0x0B, 512);
    img[0x0D] = 1;
    put16(img, 0x0E, 2);
    img[0x10] = 2;
    put16(img, 0x11, 512);
    put16(img, 0x13, IMG_SECS);
    img[0x15] = 0xF8;
    put16(img, 0x16, 20);
    put16(img, 0x18, 63);
    put16(img, 0x1A, 255);
    img[0x26] = 0x29;
    memcpy(img + 0x2B, "TESTVOL    ", 11);
    memcpy(img + 0x36, "FAT16   ", 8);
    img[510] = 0x55;
    img[511] = 0xAA;

    put16(img, FAT1_OFF + 5u * 2u, 0xFFFF);
    put16(img, FAT2_OFF + 5u * 2u, 0xFFFF); /* root dir */
    put16(img, FAT1_OFF + 10u * 2u, 11);
    put16(img, FAT2_OFF + 10u * 2u, 11);
    put16(img, FAT1_OFF + 11u * 2u, 12);
    put16(img, FAT2_OFF + 11u * 2u, 12);
    put16(img, FAT1_OFF + 12u * 2u, 0xFFFF);
    put16(img, FAT2_OFF + 12u * 2u, 0xFFFF); /* doc */
    put16(img, FAT1_OFF + 20u * 2u, 0xFFFF);
    put16(img, FAT2_OFF + 20u * 2u, 0xFFFF); /* del file */

    dentry(img, 0, "DOC     TXT", 0x20, 10, 1000, 0);
    dentry(img, 1, "DELFILE TXT", 0x20, 20, 500, 1);
    memcpy(img + DATA_OFF + (10u - 2u) * 512u, "DOCDATA", 7);
    memcpy(img + DATA_OFF + (20u - 2u) * 512u, "DELDATA", 7);

    f = fopen(path, "wb");
    if (!f) {
        free(img);
        return 0;
    }
    w = fwrite(img, 1u, (size_t)IMG_SECS * 512u, f);
    fclose(f);
    free(img);
    return w == (size_t)IMG_SECS * 512u;
}

/* ---------------------------------------------------------------- */
/* progress callback (worker thread; counts + monotonicity)          */
/* ---------------------------------------------------------------- */

static unsigned g_progress_calls = 0;
static unsigned g_last_overall = 0;

static void progress_cb(void* user, uint32_t stage, uint32_t stage_pm, uint32_t overall_pm,
                        uint64_t bytes_done, uint64_t bytes_total, uint32_t error_ranges) {
    unsigned* calls = (unsigned*)user;
    (void)stage;
    (void)stage_pm;
    (void)bytes_done;
    (void)bytes_total;
    (void)error_ranges;
    if (overall_pm + 1u < g_last_overall + 1u) { /* went backwards? */
        printf("FAIL: progress went backwards (%u -> %u)\n", g_last_overall, overall_pm);
        g_failed = 1;
    }
    g_last_overall = overall_pm;
    ++*calls;
    ++g_progress_calls;
}

static unsigned g_recovery_progress_calls = 0;
static void recovery_progress_cb(void* user, uint64_t done, uint64_t total) {
    unsigned* calls = (unsigned*)user;
    (void)done;
    (void)total;
    ++*calls;
}

/* S-008 pre-scan repair consent (records the plan it saw; consents) */
static unsigned g_abi_consent_action_count = 0;
static unsigned g_abi_consent_detected = 0;
static char g_abi_consent_family[16] = {0};
static uint32_t abi_repair_consent(void* user, const cr_ui_repair_plan* plan) {
    unsigned* calls = (unsigned*)user;
    ++*calls;
    g_abi_consent_action_count = plan->action_count;
    g_abi_consent_detected = plan->detected;
    snprintf(g_abi_consent_family, sizeof(g_abi_consent_family), "%s", plan->fs_family);
    return 1; /* consent */
}

/* find a snapshot row by (case-insensitive ASCII) name; returns index or -1 */
static int snap_find(cr_ui_snapshot* snap, const char* want) {
    uint32_t n = 0, i;
    char buf[512];
    if (cr_ui_snapshot_node_count(snap, &n) != CR_UI_OK)
        return -1;
    for (i = 0; i < n; ++i) {
        cr_ui_node_info info;
        size_t k;
        int match = 1;
        if (cr_ui_snapshot_node(snap, i, &info, buf, (uint32_t)sizeof(buf)) != CR_UI_OK)
            return -1;
        if (strlen(want) != info.name_len)
            continue;
        for (k = 0; k < info.name_len; ++k) {
            char a = buf[k], b = want[k];
            if (a >= 'a' && a <= 'z')
                a = (char)(a - 32);
            if (b >= 'a' && b <= 'z')
                b = (char)(b - 32);
            if (a != b) {
                match = 0;
                break;
            }
        }
        if (match && !info.is_full)
            return (int)i;
    }
    return -1;
}

int main(int argc, char** argv) {
    const char* img = argc > 1 ? argv[1] : "cr_ui_abi_check.img";
    char crsf_path[512], dest_dir[512];
    cr_ui_device* dev = NULL;
    cr_ui_scan* scan = NULL;
    cr_ui_snapshot* snap = NULL;
    cr_ui_session* sess = NULL;
    cr_ui_preview* pv = NULL;
    cr_ui_recovery* rec = NULL;
    cr_ui_scan_config cfg;
    cr_ui_scan_stats st;
    cr_ui_telemetry tel;
    cr_ui_geometry geom;
    cr_ui_query_row* rows = NULL;
    uint64_t doc_node_id = 0;
    char transport[16];
    uint32_t needed = 0, row_count = 0, qcount = 0, queue_n = 0, i;
    uint64_t total = 0;
    unsigned progress_calls = 0, recovery_calls = 0;
    uint32_t* dfs = NULL;
    int doc_idx;

    /* ---- ABI gate ---- */
    CHECK_EQ_U64(cr_ui_abi_version(), CR_UI_ABI_VERSION);
    CHECK(cr_ui_last_error() != NULL);

    /* ---- fixture ---- */
    CHECK(build_fat16(img));

    /* ---- device ---- */
    CHECK_EQ_U64(cr_ui_open_image(img, 0, 0, &dev), CR_UI_OK);
    CHECK(dev != NULL);
    CHECK_EQ_U64(cr_ui_device_geometry(dev, &geom), CR_UI_OK);
    CHECK_EQ_U64(geom.total_bytes, (unsigned long long)IMG_SECS * 512u);
    CHECK_EQ_U64(geom.logical_sector, 512u);
    CHECK_EQ_U64(cr_ui_device_transport(dev, transport, sizeof(transport), &needed), CR_UI_OK);
    CHECK(strcmp(transport, "image") == 0);
    /* bad-open error path */
    {
        cr_ui_device* bad = NULL;
        CHECK(cr_ui_open_image("/nonexistent/no-such-file.img", 0, 0, &bad) != CR_UI_OK);
        CHECK(bad == NULL);
        CHECK(strlen(cr_ui_last_error()) > 0);
    }

    /* ---- scan (full default pipeline) ---- */
    memset(&cfg, 0, sizeof(cfg)); /* zero-init must be the SAFE default */
    cfg.progress = progress_cb;
    cfg.progress_user = &progress_calls;
    CHECK_EQ_U64(cr_ui_scan_create(&cfg, &scan), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_scan_attach_device(scan, dev), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_scan_run(scan), CR_UI_OK);
    CHECK(progress_calls > 0);
    CHECK_EQ_U64(cr_ui_scan_get_stats(scan, &st), CR_UI_OK);
    CHECK(st.files_found >= 1);   /* DOC.TXT */
    CHECK(st.deleted_found >= 1); /* DELFILE.TXT */
    CHECK_EQ_U64(cr_ui_scan_telemetry(scan, &tel), CR_UI_OK);
    CHECK_EQ_U64(tel.overall_permille, 1000u);
    CHECK_EQ_U64(tel.running, 0u);
    /* double-run refusal (one-shot lifecycle, CLI parity) */
    CHECK(cr_ui_scan_run(scan) != CR_UI_OK);

    /* ---- S-008 pre-scan repair (consent-gated, image device) ---- */
    {
        cr_ui_device* dev2 = NULL;
        cr_ui_scan* scan2 = NULL;
        cr_ui_event evt;
        unsigned consent_calls = 0;
        char undo_path[512];
        /* FAT16 entry signatures on both copies, then a torn FAT1
         * signature sector: the classic damaged-mirror case */
        {
            unsigned char sig[4] = {0xF8, 0xFF, 0xFF, 0xFF};
            unsigned char torn[512];
            unsigned char fat1_head[4];
            FILE* f = fopen(img, "r+b");
            CHECK(f != NULL);
            memset(torn, 0xAB, sizeof(torn));
            if (f) {
                CHECK(fseek(f, (long)FAT1_OFF, SEEK_SET) == 0);
                CHECK(fwrite(sig, 1, 4, f) == 4);
                CHECK(fseek(f, (long)FAT2_OFF, SEEK_SET) == 0);
                CHECK(fwrite(sig, 1, 4, f) == 4);
                CHECK(fseek(f, (long)FAT1_OFF, SEEK_SET) == 0);
                CHECK(fwrite(torn, 1, sizeof(torn), f) == sizeof(torn)); /* torn sector */
                /* the torn fill must not accidentally keep the signature */
                CHECK(fseek(f, (long)FAT1_OFF, SEEK_SET) == 0);
                CHECK(fread(fat1_head, 1, 4, f) == 4);
                CHECK(memcmp(fat1_head, torn, 4) == 0);
                fclose(f);
            }
        }
        memset(&evt, 0, sizeof(evt)); /* new event fields compile + default zero */
        evt.type = CR_EVT_REPAIR;
        evt.repair_actions = 1;
        evt.repair_outcome = 1;
        CHECK(evt.type == 7u);
        CHECK_EQ_U64(evt.repair_actions, 1u);
        CHECK_EQ_U64(sizeof(cr_ui_repair_plan) >= sizeof("ntfs") + 8u, 1u);
        CHECK_EQ_U64(cr_ui_open_image(img, 0, 0, &dev2), CR_UI_OK);
        memset(&cfg, 0, sizeof(cfg));
        cfg.stage_mask = 1; /* quick only */
        cfg.pre_scan_repair = 1;
        cfg.repair_consent = abi_repair_consent;
        cfg.repair_consent_user = &consent_calls;
        CHECK_EQ_U64(cr_ui_scan_create(&cfg, &scan2), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_scan_attach_device(scan2, dev2), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_scan_run(scan2), CR_UI_OK);
        CHECK_EQ_U64(consent_calls, 1u);
        CHECK_EQ_U64(g_abi_consent_action_count, 1u);
        CHECK(g_abi_consent_detected == 1u);
        CHECK(strcmp(g_abi_consent_family, "fat") == 0);
        CHECK_EQ_U64(cr_ui_scan_get_stats(scan2, &st), CR_UI_OK);
        CHECK_EQ_U64(st.repair_ran, 1u);
        CHECK_EQ_U64(st.repair_planned, 1u);
        CHECK_EQ_U64(st.repair_applied, 1u);
        CHECK_EQ_U64(st.repair_denied, 0u);
        /* the transaction landed in the image file */
        {
            unsigned char fat1_head[4];
            FILE* f = fopen(img, "rb");
            CHECK(f != NULL);
            if (f) {
                CHECK(fseek(f, (long)FAT1_OFF, SEEK_SET) == 0);
                CHECK(fread(fat1_head, 1, 4, f) == 4);
                fclose(f);
                CHECK_EQ_U64(fat1_head[0], 0xF8u);
                CHECK_EQ_U64(fat1_head[1], 0xFFu);
            }
        }
        /* the undo container persists (magic checked) */
        snprintf(undo_path, sizeof(undo_path), "%s.undo", img);
        CHECK_EQ_U64(cr_ui_scan_repair_save_undo(scan2, undo_path), CR_UI_OK);
        {
            unsigned char magic[8];
            FILE* f = fopen(undo_path, "rb");
            CHECK(f != NULL);
            if (f) {
                CHECK(fread(magic, 1, 8, f) == 8);
                fclose(f);
                CHECK(memcmp(magic, "CRUNDO01", 8) == 0);
            }
            remove(undo_path);
        }
        /* nothing-applied refusal contract is covered by the C++ suite
         * (CrUi.PreScanRepairDeniedLeavesFileUntouched); here the surface
         * must compile and round-trip under /W4 /WX */
        cr_ui_scan_close(scan2);
        cr_ui_device_close(dev2);
        scan2 = NULL;
        dev2 = NULL;
    }

    /* ---- session save ---- */
    snprintf(crsf_path, sizeof(crsf_path), "%s.crsf", img);
    CHECK_EQ_U64(cr_ui_scan_save_session(scan, crsf_path), CR_UI_OK);
    cr_ui_scan_close(scan);
    scan = NULL;

    /* ---- snapshot (fresh, from a session) + enumeration ---- */
    CHECK_EQ_U64(cr_ui_session_load(crsf_path, &sess), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_session_stats(sess, &st), CR_UI_OK);
    CHECK(st.files_found >= 1);
    {
        cr_ui_session_info sinfo;
        CHECK_EQ_U64(cr_ui_session_get_info(sess, &sinfo), CR_UI_OK);
        CHECK_EQ_U64(sinfo.fs_family, 2u); /* FsFamily::Fat */
        CHECK(sinfo.stage_done_mask & 1u); /* quick completed */
    }
    CHECK_EQ_U64(cr_ui_session_snapshot(sess, &snap), CR_UI_OK);
    {
        uint32_t partial = 1;
        CHECK_EQ_U64(cr_ui_snapshot_partial(snap, &partial), CR_UI_OK);
        CHECK_EQ_U64(partial, 0u); /* quiesced tree */
    }
    CHECK_EQ_U64(cr_ui_snapshot_node_count(snap, &needed), CR_UI_OK);
    CHECK(needed > 0);
    /* DFS two-call pattern */
    CHECK_EQ_U64(cr_ui_snapshot_dfs(snap, NULL, &needed), CR_UI_OK);
    dfs = (uint32_t*)malloc(needed * sizeof(uint32_t));
    CHECK(dfs != NULL);
    qcount = needed;
    CHECK_EQ_U64(cr_ui_snapshot_dfs(snap, dfs, &qcount), CR_UI_OK);
    CHECK_EQ_U64(qcount, needed);
    /* DOC.TXT row present with truthful fields */
    doc_idx = snap_find(snap, "DOC.TXT");
    CHECK(doc_idx >= 0);
    if (doc_idx >= 0) {
        cr_ui_node_info info;
        char name[512];
        CHECK_EQ_U64(
            cr_ui_snapshot_node(snap, (uint32_t)doc_idx, &info, name, (uint32_t)sizeof(name)),
            CR_UI_OK);
        CHECK(strcmp(name, "DOC.TXT") == 0);
        CHECK_EQ_U64(info.logical_size, 1000u);
        CHECK_EQ_U64(info.deleted, 0u);
        doc_node_id = info.node_id;
        CHECK(doc_node_id != 0);
        /* truncated name buffer -> CR_UI_ERR_TRUNCATED with the full length */
        CHECK(cr_ui_snapshot_node(snap, (uint32_t)doc_idx, &info, name, 3) == CR_UI_ERR_TRUNCATED);
        CHECK_EQ_U64(info.name_len, 7u);
    }

    /* ---- query (zero-init = match-all; then filters) ---- */
    {
        cr_ui_query q;
        memset(&q, 0, sizeof(q));
        q.count_only = 1;
        CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
        CHECK(rows == NULL);
        CHECK(total >= 2);
        memset(&q, 0, sizeof(q));
        q.limit = 50;
        CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
        CHECK(row_count >= 1);
        for (i = 0; i < row_count; ++i)
            if (strcmp(rows[i].name, "DOC.TXT") == 0)
                doc_node_id = rows[i].node_id;
        CHECK(doc_node_id != 0);
        cr_ui_query_rows_free(rows, row_count);
        rows = NULL;
        /* deleted_only keeps DELFILE.TXT */
        memset(&q, 0, sizeof(q));
        q.deleted_only = 1;
        q.limit = 50;
        CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
        CHECK(total >= 1);
        cr_ui_query_rows_free(rows, row_count);
        rows = NULL;
        /* name filter narrows to DOC.TXT */
        memset(&q, 0, sizeof(q));
        q.name_substring = "doc";
        q.limit = 50;
        CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
        CHECK_EQ_U64(total, 1u);
        CHECK_EQ_U64(row_count, 1u);
        CHECK(strcmp(rows[0].name, "DOC.TXT") == 0);
        cr_ui_query_rows_free(rows, row_count);
        rows = NULL;
    }

    /* ---- preview (session + snapshot variants) ---- */
    CHECK_EQ_U64(cr_ui_preview_open_session(dev, sess, doc_node_id, &pv), CR_UI_OK);
    if (pv) {
        uint64_t size = 0;
        uint32_t placed = 0;
        char buf[16] = {0};
        cr_ui_metadata meta;
        CHECK_EQ_U64(cr_ui_preview_size(pv, &size), CR_UI_OK);
        CHECK_EQ_U64(size, 1000u);
        CHECK_EQ_U64(cr_ui_preview_read(pv, 0, buf, 7, &placed), CR_UI_OK);
        CHECK_EQ_U64(placed, 7u);
        CHECK(strcmp(buf, "DOCDATA") == 0);
        memset(&meta, 0, sizeof(meta));
        CHECK_EQ_U64(cr_ui_preview_metadata(pv, &meta), CR_UI_OK);
        CHECK(strlen(meta.kind) > 0);
        cr_ui_preview_close(pv);
        pv = NULL;
    }
    if (doc_idx >= 0) {
        CHECK_EQ_U64(cr_ui_preview_open_snapshot(dev, snap, (uint32_t)doc_idx, &pv), CR_UI_OK);
        if (pv) {
            uint32_t placed = 0;
            char buf[16] = {0};
            CHECK_EQ_U64(cr_ui_preview_read(pv, 0, buf, 7, &placed), CR_UI_OK);
            CHECK(strcmp(buf, "DOCDATA") == 0);
            cr_ui_preview_close(pv);
            pv = NULL;
        }
    }

    /* ---- recovery (subtree = the CLI `recover` walk) ---- */
    snprintf(dest_dir, sizeof(dest_dir), "%s.dest", img);
    {
        cr_ui_recovery_config rc;
        memset(&rc, 0, sizeof(rc));
        rc.progress = recovery_progress_cb;
        rc.progress_user = &recovery_calls;
        CHECK_EQ_U64(cr_ui_recovery_create(dev, &rc, &rec), CR_UI_OK);
    }
    CHECK_EQ_U64(cr_ui_recovery_set_destination(rec, dest_dir), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_recovery_enqueue_node(rec, sess, doc_node_id), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_recovery_queue_count(rec, &queue_n), CR_UI_OK);
    CHECK_EQ_U64(queue_n, 1u);
    CHECK_EQ_U64(cr_ui_recovery_process(rec), CR_UI_OK);
    CHECK(recovery_calls > 0);
    {
        cr_ui_recovery_stats rs;
        char* json = NULL;
        char recovered[512], check_path[600];
        FILE* f;
        CHECK_EQ_U64(cr_ui_recovery_get_stats(rec, &rs), CR_UI_OK);
        CHECK_EQ_U64(rs.done, 1u);
        CHECK_EQ_U64(rs.errors, 0u);
        CHECK_EQ_U64(cr_ui_recovery_report_json(rec, img, &json), CR_UI_OK);
        CHECK(json != NULL);
        CHECK(strstr(json, "DOC.TXT") != NULL);
        CHECK(strstr(json, "\"schema_version\"") != NULL);
        cr_ui_string_free(json);
        /* the recovered bytes on disk (portable FileDestIo: '/' paths) */
        snprintf(check_path, sizeof(check_path), "%s/DOC.TXT", dest_dir);
        f = fopen(check_path, "rb");
        CHECK(f != NULL);
        if (f) {
            size_t n = fread(recovered, 1, 7, f);
            fclose(f);
            CHECK(n == 7);
            CHECK(strncmp(recovered, "DOCDATA", 7) == 0);
        }
    }

    /* ---- platform-truth surfaces ---- */
#ifndef _WIN32
    {
        cr_ui_device* d = NULL;
        cr_ui_disk_info di;
        cr_ui_smart_report hr;
        uint32_t ac = 0;
        CHECK_EQ_U64(cr_ui_open_disk(0, &d), CR_UI_ERR_UNSUPPORTED);
        CHECK_EQ_U64(cr_ui_open_volume('C', &d), CR_UI_ERR_UNSUPPORTED);
        CHECK_EQ_U64(cr_ui_enumerate_disks(&di, &ac), CR_UI_ERR_UNSUPPORTED);
        CHECK_EQ_U64(cr_ui_enumerate_volumes(NULL, &ac), CR_UI_ERR_UNSUPPORTED);
        CHECK_EQ_U64(cr_ui_health_read(0, &hr, NULL, 0, &ac), CR_UI_ERR_UNSUPPORTED);
        CHECK(strstr(cr_ui_last_error(), "Windows platform layer") != NULL);
    }
#endif

    /* ---- error paths ---- */
    CHECK(cr_ui_scan_verify_identity(scan) != CR_UI_OK); /* NULL handle */
    CHECK(cr_ui_query_run(NULL, NULL, NULL, NULL, NULL) == CR_UI_ERR_BAD_PARAMETER);

    /* ---- teardown ---- */
    cr_ui_recovery_close(rec);
    cr_ui_snapshot_close(snap);
    cr_ui_session_close(sess);
    cr_ui_device_close(dev);
    free(dfs);
    remove(crsf_path);
    {
        char p[600];
        snprintf(p, sizeof(p), "%s/DOC.TXT", dest_dir);
        remove(p);
        snprintf(p, sizeof(p), "%s.tmp", img); /* in case anything strayed */
        remove(p);
        remove(dest_dir);
    }
    remove(img);

    if (g_failed) {
        printf("cr_ui_abi_check: FAILED (%u progress, %u recovery calls)\n", g_progress_calls,
               g_recovery_progress_calls);
        return 1;
    }
    printf("cr_ui_abi_check: OK (%u progress calls, %u recovery calls)\n", g_progress_calls,
           g_recovery_progress_calls);
    return 0;
}
