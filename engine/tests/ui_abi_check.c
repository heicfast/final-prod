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

/* S-029 apply-consent driver: records calls, replies g_apply_reply */
static unsigned g_apply_reply = 0;
static cr_ui_apply_plan g_apply_plan;

static uint32_t apply_consent_cb(void* user, const cr_ui_apply_plan* plan) {
    unsigned* calls = (unsigned*)user;
    ++*calls;
    g_apply_plan = *plan;
    return g_apply_reply;
}

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

    put16(img, FAT1_OFF + 0u * 2u, 0xFFF8);
    put16(img, FAT1_OFF + 1u * 2u, 0xFFFF); /* media pair (M-015) */
    put16(img, FAT2_OFF + 0u * 2u, 0xFFF8);
    put16(img, FAT2_OFF + 1u * 2u, 0xFFFF);
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

/* typed event stream (GAP-S6-10): counts per CR_EVT_*; unknown types ignored
 * (forward-compat rule, ABI-CONTRACT section 6) */
static unsigned g_evt_stage = 0, g_evt_progress = 0, g_evt_files = 0, g_evt_err = 0,
                g_evt_checkpt = 0, g_evt_lost = 0, g_evt_repair = 0, g_evt_hb = 0,
                g_evt_unknown = 0;
static void event_cb(void* user, const cr_ui_event* evt) {
    (void)user;
    if (!evt)
        return;
    switch (evt->type) {
    case 1u:
        ++g_evt_stage;
        break;
    case 2u:
        ++g_evt_progress;
        break;
    case 3u:
        ++g_evt_files;
        break;
    case 4u:
        ++g_evt_err;
        break;
    case 5u:
        ++g_evt_checkpt;
        break;
    case 6u:
        ++g_evt_lost;
        break;
    case 7u:
        ++g_evt_repair;
        break;
    case CR_EVT_HEARTBEAT: /* N-043: the liveness watchdog (slow lanes) */
        ++g_evt_hb;
        break;
    default:
        ++g_evt_unknown;
        break; /* must be ignored, not fatal */
    }
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
    char ckpt_path[512];
    uint32_t needed = 0, row_count = 0, qcount = 0, queue_n = 0, i;
    uint64_t total = 0;
    unsigned progress_calls = 0, recovery_calls = 0;
    uint32_t* dfs = NULL;
    int doc_idx;

    /* ---- ABI gate ---- */
    CHECK_EQ_U64(cr_ui_abi_version(), CR_UI_ABI_VERSION);
    CHECK(cr_ui_last_error() != NULL);

    /* ---- carve-format registry (ops #7-#8 parity; ABI-CONTRACT 7a) ---- */
    {
        uint32_t fmt_n = cr_ui_carve_format_count();
        cr_ui_carve_format fmt;
        uint32_t k, grammar_n = 0, raw_n = 0;
        CHECK(fmt_n > 0);
        for (k = 0; k < fmt_n; ++k) {
            CHECK_EQ_U64(cr_ui_carve_format_info(k, &fmt), CR_UI_OK);
            CHECK(fmt.layer <= 1u);
            CHECK(fmt.ext[0] != '\0');
            CHECK(strchr(fmt.ext, '.') == NULL);
            CHECK(fmt.ext[sizeof(fmt.ext) - 1] == '\0'); /* NUL-terminated within the field */
            CHECK(fmt.base_confidence <= 1000u);
            CHECK(fmt.magic_len <= 16u);
            if (fmt.layer == 0u)
                ++grammar_n;
            else
                ++raw_n;
        }
        CHECK(grammar_n > 0); /* grammar layer present (jpg et al.) */
        CHECK(raw_n > 0);     /* raw-sign layer present (the 139-format table) */
        /* error contract: out-of-range + null out */
        CHECK_EQ_U64(cr_ui_carve_format_info(fmt_n, &fmt), CR_UI_ERR_BAD_PARAMETER);
        CHECK_EQ_U64(cr_ui_carve_format_info(0, NULL), CR_UI_ERR_BAD_PARAMETER);
        CHECK(strlen(cr_ui_last_error()) > 0);
    }

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

    /* ---- scan (full default pipeline: events + checkpoints) ---- */
    memset(&cfg, 0, sizeof(cfg)); /* zero-init must be the SAFE default */
    cfg.progress = progress_cb;
    cfg.progress_user = &progress_calls;
    snprintf(ckpt_path, sizeof(ckpt_path), "%s.ckpt", img);
    cfg.checkpoint_path_utf8 = ckpt_path;
    CHECK_EQ_U64(cr_ui_scan_create(&cfg, &scan), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_scan_attach_device(scan, dev), CR_UI_OK);
    /* typed event stream: subscribe BEFORE run (ABI-CONTRACT section 6);
     * subscribing while running is refused */
    CHECK_EQ_U64(cr_ui_scan_set_event_fn(scan, event_cb, NULL), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_scan_run(scan), CR_UI_OK);
    CHECK(progress_calls > 0);
    CHECK(g_evt_stage > 0);    /* stage transitions observed */
    CHECK(g_evt_progress > 0); /* progress samples observed */
    CHECK(g_evt_files > 0);    /* files-found counters observed */
    CHECK_EQ_U64(g_evt_unknown, 0u);
    /* unsubscribe + resubscribe round-trip (resubscribe after run: the
     * controller is spent, but the call must not corrupt state) */
    CHECK_EQ_U64(cr_ui_scan_set_event_fn(scan, NULL, NULL), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_scan_set_event_fn(scan, event_cb, NULL), CR_UI_OK);
    CHECK_EQ_U64(cr_ui_scan_get_stats(scan, &st), CR_UI_OK);
    CHECK(st.files_found >= 1);   /* DOC.TXT */
    CHECK(st.deleted_found >= 1); /* DELFILE.TXT */
    /* S-023: the grading pass ran at completion (the assistant block is
     * populated; existing DOC.TXT graded High, deleted DELFILE.TXT
     * carries a grade, the gauge obeys the small-result rule) */
    {
        cr_ui_assistant_stats as;
        memset(&as, 0, sizeof(as));
        CHECK_EQ_U64(cr_ui_scan_assistant(scan, &as), CR_UI_OK);
        CHECK(as.graded_count >= 2);
        CHECK_EQ_U64(as.total_file_count, as.graded_count);
        CHECK_EQ_U64(as.total, as.total_file_count);
        CHECK(as.total < 100 ? as.matched == 0 : 1); /* small-result rule */
        CHECK_EQ_U64(as.quick_scan_exist_file_count >= 1, 1);
    }
    CHECK_EQ_U64(cr_ui_scan_telemetry(scan, &tel), CR_UI_OK);
    CHECK_EQ_U64(tel.overall_permille, 1000u);
    CHECK_EQ_U64(tel.running, 0u);
    /* checkpoints were written at the stage boundary (G-13) */
    {
        uint64_t ckpt_n = 0;
        CHECK_EQ_U64(cr_ui_scan_checkpoint_count(scan, &ckpt_n), CR_UI_OK);
        CHECK(ckpt_n >= 1);
    }
    /* double-run refusal (one-shot lifecycle, CLI parity) */
    CHECK(cr_ui_scan_run(scan) != CR_UI_OK);

    /* ---- cancel-before-run contract (single-threaded C proof; the
     * mid-run cross-thread cancel semantics are the C++ suite's
     * CancelSaveAndResume / PauseBlocksAndResumes) ---- */
    {
        cr_ui_scan* sc = NULL;
        cr_ui_scan_config c2;
        memset(&c2, 0, sizeof(c2));
        c2.stage_mask = 1;
        CHECK_EQ_U64(cr_ui_scan_create(&c2, &sc), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_scan_attach_device(sc, dev), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_scan_cancel(sc), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_scan_run(sc), CR_UI_ERR_CANCELLED);
        /* the cancelled scan still saves a truthful session (D-18) */
        {
            char cpath[512];
            snprintf(cpath, sizeof(cpath), "%s.cancel.crsf", img);
            CHECK_EQ_U64(cr_ui_scan_save_session(sc, cpath), CR_UI_OK);
            remove(cpath);
        }
        cr_ui_scan_close(sc);
        sc = NULL;
    }

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
        /* S-023: the og's CRecoverability verdict - existing file with
         * size >= 1 is grade 2 (High); never grade-less */
        CHECK_EQ_U64(info.grade, 2u);
        CHECK_EQ_U64(info.overlap, 0u);
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
        /* S-023: rows carry the grade (High for the existing DOC.TXT) */
        {
            int saw_doc = 0;
            for (i = 0; i < row_count; ++i)
                if (strcmp(rows[i].name, "DOC.TXT") == 0) {
                    saw_doc = 1;
                    CHECK_EQ_U64(rows[i].grade, 2u);
                }
            CHECK(saw_doc);
        }
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
        /* path_prefix (ABI-since core-api, og path filter group): the row's
         * own full path as the prefix selects exactly that row; a bogus
         * prefix selects nothing */
        CHECK(rows[0].path != NULL);
        if (rows[0].path != NULL) {
            char doc_path[600];
            snprintf(doc_path, sizeof(doc_path), "%s", rows[0].path);
            cr_ui_query_rows_free(rows, row_count);
            rows = NULL;
            memset(&q, 0, sizeof(q));
            q.path_prefix = doc_path;
            q.limit = 50;
            CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
            CHECK_EQ_U64(total, 1u);
            cr_ui_query_rows_free(rows, row_count);
            rows = NULL;
            memset(&q, 0, sizeof(q));
            q.path_prefix = "NO/SUCH/PREFIX";
            q.count_only = 1;
            CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
            CHECK_EQ_U64(total, 0u);
            CHECK(rows == NULL);
        } else {
            cr_ui_query_rows_free(rows, row_count);
            rows = NULL;
        }
    }

    /* ---- S-023: per-selection re-grade (grade-1 only; grade 0/2 final) ---- */
    {
        uint32_t regraded = 0xFFFFFFFFu;
        uint64_t ids[2];
        size_t n_ids = 0;
        /* collect one grade-1 (average) node id via a deleted_only query
         * (deleted structured files stay average unless a rule raised
         * them) + the existing DOC.TXT (grade 2 - must be refused) */
        cr_ui_query q;
        memset(&q, 0, sizeof(q));
        q.deleted_only = 1;
        q.limit = 10;
        CHECK_EQ_U64(cr_ui_query_run(sess, &q, &rows, &row_count, &total), CR_UI_OK);
        for (i = 0; i < row_count && n_ids < 2; ++i) {
            CHECK(rows[i].grade <= 2u); /* domain guard */
            if (rows[i].grade == 1u && n_ids < 1)
                ids[n_ids++] = rows[i].node_id;
        }
        cr_ui_query_rows_free(rows, row_count);
        rows = NULL;
        if (doc_node_id)
            ids[n_ids++] = doc_node_id; /* grade 2: refused by the gate */
        CHECK(n_ids >= 1);
        CHECK_EQ_U64(cr_ui_session_regrade(sess, ids, (uint32_t)n_ids, &regraded), CR_UI_OK);
        CHECK(regraded <= 1); /* at most the single grade-1 node ran */
    }

    /* ---- disk map (GAP-S6-11 data producer: session path, auto geometry) ---- */
    {
        cr_ui_disk_map_geometry want, got;
        uint8_t *alloc = NULL, *cov = NULL, *found = NULL, *conf = NULL;
        uint64_t bad[8] = {0};
        uint32_t bad_n = 0;
        memset(&want, 0, sizeof(want)); /* zero-init = auto */
        CHECK_EQ_U64(cr_ui_disk_map(sess, NULL, &want, &got, &alloc, &cov, &found, &conf, bad,
                                    (uint32_t)(sizeof(bad) / (2 * sizeof(uint64_t))), &bad_n),
                     CR_UI_OK);
        CHECK(got.cell_count > 0);
        CHECK(alloc != NULL);
        CHECK(cov != NULL);
        /* S-009: the persisted engine allocation bitmap (the FAT on this
         * fixture) surfaces as free cells through the four-overlay C
         * consumer call - the og's IBuildVolumeBitmap free/used truth;
         * the sparse fixture is overwhelmingly provably free and the
         * active DOC.TXT cell stays used (1) */
        {
            uint64_t free_cells = 0, used_cells = 0;
            uint64_t ci;
            for (ci = 0; ci < got.cell_count; ++ci) {
                if (alloc[ci] == 2u)
                    ++free_cells;
                else if (alloc[ci] == 1u)
                    ++used_cells;
            }
            CHECK(free_cells > got.cell_count / 2);
            CHECK(used_cells >= 1);
            CHECK(got.cluster_bytes == 512u); /* the map's cluster hint */
        }
        cr_ui_disk_map_free(alloc);
        cr_ui_disk_map_free(cov);
        cr_ui_disk_map_free(found);
        cr_ui_disk_map_free(conf);
        /* the two-call bad_ranges pattern: NULL buffer -> needed count
         * (zero bad ranges on this healthy fixture -> OK; cap 0 with a
         * nonzero map -> TRUNCATED after filling - both are contract-clean) */
        {
            uint32_t need2 = 0;
            const uint64_t rc_dm =
                cr_ui_disk_map(sess, NULL, &want, &got, &alloc, NULL, NULL, NULL, NULL, 0, &need2);
            CHECK(rc_dm == CR_UI_OK || rc_dm == CR_UI_ERR_TRUNCATED);
            cr_ui_disk_map_free(alloc);
            alloc = NULL;
        }
        /* snapshot path REQUIRES view geometry (ABI-CONTRACT section 7) */
        CHECK(cr_ui_disk_map(NULL, snap, NULL, &got, &alloc, NULL, NULL, NULL, NULL, 0, &bad_n) ==
              CR_UI_ERR_BAD_PARAMETER);
        cr_ui_disk_map_free(alloc);
        alloc = NULL;
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

    /* ---- recovery: enqueue variants (subtree + snapshot) + the
     * cancel-before-process contract (single-threaded C proof; the mid-run
     * cross-thread cancel is the C++ suite's domain) ---- */
    {
        cr_ui_recovery* rec2 = NULL;
        cr_ui_recovery_config rc2;
        cr_ui_recovery_stats rs2;
        char dest2[512];
        memset(&rc2, 0, sizeof(rc2));
        snprintf(dest2, sizeof(dest2), "%s.dest2", img);
        CHECK_EQ_U64(cr_ui_recovery_create(dev, &rc2, &rec2), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_recovery_set_destination(rec2, dest2), CR_UI_OK);
        /* subtree enqueue: a file node's subtree is itself; the root-family
         * variant is exercised in the C++ suite (RecoverySubtreeAndRename) */
        CHECK(doc_node_id != 0);
        CHECK_EQ_U64(cr_ui_recovery_enqueue_subtree(rec2, sess, doc_node_id), CR_UI_OK);
        {
            uint32_t qn = 0;
            CHECK_EQ_U64(cr_ui_recovery_queue_count(rec2, &qn), CR_UI_OK);
            CHECK(qn >= 1);
        }
        /* snapshot-based enqueue variants compile + queue */
        if (doc_idx >= 0) {
            const uint64_t rc_es = cr_ui_recovery_enqueue_snapshot(rec2, snap, (uint32_t)doc_idx);
            CHECK_EQ_U64(rc_es, CR_UI_OK);
        }
        if (doc_idx >= 0) {
            const uint64_t rc_ess =
                cr_ui_recovery_enqueue_subtree_snapshot(rec2, snap, (uint32_t)doc_idx);
            /* a file node's subtree is itself: OK; leaf-refusal is also
             * contract-clean (the C++ suite pins the exact semantics) */
            CHECK(rc_ess == CR_UI_OK || rc_ess == CR_UI_ERR_BAD_PARAMETER);
        }
        /* cancel BEFORE process: process returns ERR_CANCELLED with zero
         * committed work (the D-18 truthful-queue contract) */
        CHECK_EQ_U64(cr_ui_recovery_cancel(rec2), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_recovery_process(rec2), CR_UI_ERR_CANCELLED);
        CHECK_EQ_U64(cr_ui_recovery_get_stats(rec2, &rs2), CR_UI_OK);
        CHECK_EQ_U64(rs2.done, 0u);
        cr_ui_recovery_close(rec2);
        rec2 = NULL;
        {
            char p[600];
            snprintf(p, sizeof(p), "%s/DOC.TXT", dest2);
            remove(p);
            remove(dest2);
        }
    }

    /* ---- session resume + identity verification (the resume flow) ---- */
    {
        cr_ui_scan* rscan = NULL;
        cr_ui_device* rdev = NULL;
        cr_ui_scan_config rcfg;
        memset(&rcfg, 0, sizeof(rcfg)); /* the ORIGINAL config's zero-init */
        CHECK_EQ_U64(cr_ui_open_image(img, 0, 0, &rdev), CR_UI_OK);
        CHECK_EQ_U64(cr_ui_session_resume(sess, &rcfg, &rscan), CR_UI_OK);
        CHECK(rscan != NULL);
        CHECK_EQ_U64(cr_ui_scan_attach_device(rscan, rdev), CR_UI_OK);
        /* the boot-hash identity check: same image -> OK (positive path;
        the mismatch path is the C++ suite's domain) */
        CHECK_EQ_U64(cr_ui_scan_verify_identity(rscan), CR_UI_OK);
        /* resume of a fully-completed session: run is a truthful no-op */
        CHECK_EQ_U64(cr_ui_scan_run(rscan), CR_UI_OK);
        {
            cr_ui_scan_stats rst;
            CHECK_EQ_U64(cr_ui_scan_get_stats(rscan, &rst), CR_UI_OK);
            CHECK(rst.files_found >= 1);
        }
        cr_ui_scan_close(rscan);
        cr_ui_device_close(rdev);
    }

    /* ---- S-029: lost-partition apply (consent-gated, C-ABI) ---- */
    {
        /* a separate MBR disk image: one live NTFS entry, the lost
         * candidate at LBA 3200 (clear of the live extent) */
        static const unsigned long long DISK_SECS = 4096;
        char apath[640];
        unsigned char* aimg;
        cr_ui_device* adev = NULL;
        cr_ui_apply_candidate cand;
        cr_ui_apply_result res;
        unsigned consent_calls = 0;
        FILE* af;
        size_t w;
        snprintf(apath, sizeof(apath), "%s.apply", img);
        aimg = (unsigned char*)calloc((size_t)DISK_SECS, 512u);
        CHECK(aimg != NULL);
        put32(aimg, 440, 0xAABBCCDDul);
        aimg[446 + 4] = 0x07;
        put32(aimg, 446 + 8, 2048);
        put32(aimg, 446 + 12, 1024);
        aimg[510] = 0x55;
        aimg[511] = 0xAA;
        af = fopen(apath, "wb");
        CHECK(af != NULL);
        w = fwrite(aimg, 1, (size_t)DISK_SECS * 512u, af);
        fclose(af);
        CHECK(w == (size_t)DISK_SECS * 512u);
        free(aimg);

        CHECK_EQ_U64(cr_ui_open_image(apath, 0, 0, &adev), CR_UI_OK);
        memset(&cand, 0, sizeof(cand));
        cand.start_lba = 3200;
        cand.sector_count = 512;
        cand.fs = 0; /* NTFS */
        memset(&res, 0, sizeof(res));

        /* NULL consent = default deny, nothing written */
        CHECK_EQ_U64(cr_ui_apply_lost_partition(adev, &cand, 0, NULL, NULL, &res),
                     0x50080004u /* ERR_APPLY_CONSENT_DENIED */);
        CHECK_EQ_U64(res.outcome, 1u);
        CHECK_EQ_U64(res.undo_entries, 0u);

        /* deny callback: same refusal */
        g_apply_reply = 0;
        CHECK_EQ_U64(
            cr_ui_apply_lost_partition(adev, &cand, 0, apply_consent_cb, &consent_calls, &res),
            0x50080004u);
        CHECK_EQ_U64(consent_calls, 1u);
        CHECK_EQ_U64(g_apply_plan.mbr_type, 0x07u);
        CHECK_EQ_U64(g_apply_plan.slot, 1u);
        CHECK_EQ_U64(g_apply_plan.signature, 0xAABBCCDDu);

        /* approve: the entry lands in the image file */
        g_apply_reply = 1;
        CHECK_EQ_U64(
            cr_ui_apply_lost_partition(adev, &cand, 0, apply_consent_cb, &consent_calls, &res),
            CR_UI_OK);
        CHECK_EQ_U64(res.outcome, 0u);
        CHECK_EQ_U64(res.applied, 1u);
        CHECK_EQ_U64(res.undo_entries, 1u);
        {
            unsigned char sec0[512];
            FILE* vf = fopen(apath, "rb");
            CHECK(vf != NULL);
            CHECK(fread(sec0, 1, 512, vf) == 512);
            fclose(vf);
            CHECK(sec0[446 + 16 + 4] == 0x07);
            CHECK(sec0[446 + 16 + 8] == (3200 & 0xFF));
            CHECK(sec0[446 + 16 + 9] == ((3200 >> 8) & 0xFF));
            CHECK(sec0[446 + 4] == 0x07); /* live entry preserved */
            CHECK(sec0[510] == 0x55 && sec0[511] == 0xAA);
        }

        /* undo restores, container persists */
        CHECK_EQ_U64(cr_ui_apply_undo_last(adev, &res), CR_UI_OK);
        CHECK_EQ_U64(res.outcome, 0u);
        {
            unsigned char sec0[512];
            FILE* vf = fopen(apath, "rb");
            CHECK(vf != NULL);
            CHECK(fread(sec0, 1, 512, vf) == 512);
            fclose(vf);
            CHECK(sec0[446 + 16 + 4] == 0x00); /* slot 1 empty again */
            CHECK(sec0[446 + 16 + 8] == 0x00);
        }
        {
            char upath[704]; /* apath (<=639) + ".undo" + NUL, provably untruncated */
            snprintf(upath, sizeof(upath), "%s.undo", apath);
            CHECK_EQ_U64(cr_ui_apply_save_undo(adev, upath), CR_UI_OK);
            remove(upath);
        }

        /* error paths: bad fs value + null handles */
        cand.fs = 999;
        CHECK_EQ_U64(
            cr_ui_apply_lost_partition(adev, &cand, 0, apply_consent_cb, &consent_calls, &res),
            0x50080006u /* ERR_APPLY_REFUSED: unknown family */);
        CHECK_EQ_U64(cr_ui_apply_lost_partition(NULL, &cand, 0, NULL, NULL, &res),
                     CR_UI_ERR_BAD_PARAMETER);
        CHECK_EQ_U64(cr_ui_apply_lost_partition(adev, NULL, 0, NULL, NULL, &res),
                     CR_UI_ERR_BAD_PARAMETER);
        CHECK_EQ_U64(cr_ui_apply_undo_last(adev, NULL), CR_UI_ERR_BAD_PARAMETER);
        cr_ui_device_close(adev);
        remove(apath);
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
    remove(ckpt_path);
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
