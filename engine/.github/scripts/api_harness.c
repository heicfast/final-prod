/*
 * api_harness.c - the API-lane test driver for the test-benchmark workflow.
 *
 * THE PRODUCT SURFACE: cr_ui.dll is the C ABI the future UI binds to
 * (include/cr/cr_ui.h, docs/ABI-CONTRACT.md); the CLI is the testing
 * vehicle. This harness drives the FULL scan -> session -> query ->
 * recovery lifecycle through the API alone, against release-zip
 * artifacts (lib/cr_ui.lib + include/ + bin/cr_ui.dll) - proving the
 * shipped SDK is consumable out-of-the-box, exactly like a UI host.
 *
 * Modes (exit codes mirror the CLI contract: 0 ok, 1 hard failure,
 * 2 partial recovery - recovery errors or hash mismatch):
 *   --abi-version                         ABI gate check
 *   --carve-count                         enumerate the carve registry
 *   --user-signs <file>                   register the shipped user-sign JSON
 *   --scan --target volume --letter Y --dest D --out O
 *          [--stages quick|deep|deepcarve|all] [--threads N]
 *          [--cancel-after S] [--label STR]
 *   --scan --target disk --disk N --whole-disk --dest D --out O [...]
 *   --resume --session F --target volume --letter Y --dest D --out O
 *
 * --scan lifecycle (timed with QueryPerformanceCounter; every stage's
 * rc checked; the session is saved AND RELOADED to prove the round-trip):
 *   open_volume/open_disk -> scan_create(+events,+progress) -> attach ->
 *   run -> stats/telemetry/assistant -> save_session -> session_load ->
 *   session_info/stats -> query(all + deleted_only + count_only) ->
 *   preview(first deleted file: size + 4 KiB read + metadata) ->
 *   recovery: create -> set_destination -> enqueue_subtree(every root) ->
 *   queue_count -> process -> get_stats -> report_json -> write report
 *
 * Outputs into --out: api-result.json (timings, stats, counts, events),
 * report.json (the G-09 recovery report), session.crsf.
 * The exit code follows the CLI recover contract so the orchestrator can
 * gate both lanes identically.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "cr/cr_ui.h"

#define HARNESS_OK 0
#define HARNESS_FAIL 1
#define HARNESS_PARTIAL 2

/* ---- event/progress sinks (thread-safe counters; callbacks must not block) */

typedef struct {
    volatile LONG events_stage;
    volatile LONG events_progress;
    volatile LONG events_files_found;
    volatile LONG events_error_range;
    volatile LONG events_checkpoint;
    volatile LONG events_lost_candidate;
    volatile LONG events_repair;
    volatile LONG events_heartbeat;
    volatile LONG events_save_progress;
    volatile LONG events_fs_check;
    volatile LONG events_other;
    volatile LONG last_overall_permille;
} sink_t;

static void event_fn(void* user, const cr_ui_event* evt) {
    sink_t* s = (sink_t*)user;
    if (!evt) return;
    switch (evt->type) {
    case 1: InterlockedIncrement(&s->events_stage); break;
    case 2: InterlockedIncrement(&s->events_progress); break;
    case 3: InterlockedIncrement(&s->events_files_found); break;
    case 4: InterlockedIncrement(&s->events_error_range); break;
    case 5: InterlockedIncrement(&s->events_checkpoint); break;
    case 6: InterlockedIncrement(&s->events_lost_candidate); break;
    case 7: InterlockedIncrement(&s->events_repair); break;
    case 8: InterlockedIncrement(&s->events_heartbeat); break;
    case 9: InterlockedIncrement(&s->events_save_progress); break;
    case 10: InterlockedIncrement(&s->events_fs_check); break;
    default: InterlockedIncrement(&s->events_other); break;
    }
}

static void progress_fn(void* user, uint32_t stage, uint32_t stage_permille,
                        uint32_t overall_permille, uint64_t bytes_done,
                        uint64_t bytes_total, uint32_t error_ranges) {
    (void)stage; (void)stage_permille; (void)bytes_done;
    (void)bytes_total; (void)error_ranges;
    sink_t* s = (sink_t*)user;
    InterlockedExchange(&s->last_overall_permille, (LONG)overall_permille);
}

static void recovery_progress_fn(void* user, uint64_t done, uint64_t total) {
    (void)user; (void)done; (void)total; /* recorded via stats; keep the sink trivial */
}

/* ---- cancel timer thread (--cancel-after) */

typedef struct {
    cr_ui_scan* scan;
    DWORD after_ms;
} cancel_req_t;

static DWORD WINAPI cancel_thread(LPVOID arg) {
    cancel_req_t* req = (cancel_req_t*)arg;
    Sleep(req->after_ms);
    cr_ui_scan_cancel(req->scan);
    return 0;
}

/* ---- timing */

static double g_freq_inv_ms;
static LARGE_INTEGER qpc_now(void) {
    LARGE_INTEGER t; QueryPerformanceCounter(&t); return t;
}
static double qpc_ms(LARGE_INTEGER t0, LARGE_INTEGER t1) {
    return (double)(t1.QuadPart - t0.QuadPart) * g_freq_inv_ms;
}

/* ---- JSON helpers (hand-rolled: no deps; fixed field order) */

static void json_escape_into(FILE* f, const char* s) {
    fputc('"', f);
    for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
        if (*p == '"' || *p == '\\') { fputc('\\', f); fputc(*p, f); }
        else if (*p < 0x20) fprintf(f, "\\u%04x", *p);
        else fputc(*p, f);
    }
    fputc('"', f);
}

static int write_text_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

static int mode_abi_version(void) {
    uint32_t v = cr_ui_abi_version();
    printf("{\"harness\":\"abi-version\",\"abi_version\":%lu,\"expected\":%lu,\"match\":%s}\n",
           (unsigned long)v, (unsigned long)CR_UI_ABI_VERSION,
           v == CR_UI_ABI_VERSION ? "true" : "false");
    return v == CR_UI_ABI_VERSION ? HARNESS_OK : HARNESS_FAIL;
}

static int mode_carve_count(void) {
    uint32_t n = cr_ui_carve_format_count();
    /* integrity: every index must read back cleanly */
    uint32_t readable = 0;
    for (uint32_t i = 0; i < n; i++) {
        cr_ui_carve_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        if (cr_ui_carve_format_info(i, &fmt) == CR_UI_OK) readable++;
    }
    printf("{\"harness\":\"carve-count\",\"carve_formats\":%lu,\"readable\":%lu}\n",
           (unsigned long)n, (unsigned long)readable);
    if (n == 0 || readable != n) return HARNESS_FAIL;
    return HARNESS_OK;
}

static int mode_user_signs(const char* file) {
    FILE* f = fopen(file, "rb");
    if (!f) { printf("{\"harness\":\"user-signs\",\"error\":\"cannot open file\"}\n"); return HARNESS_FAIL; }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0 || len > 8 * 1024 * 1024) { fclose(f); printf("{\"harness\":\"user-signs\",\"error\":\"bad size\"}\n"); return HARNESS_FAIL; }
    char* buf = (char*)malloc((size_t)len + 1);
    if (!buf) { fclose(f); return HARNESS_FAIL; }
    size_t got = fread(buf, 1, (size_t)len, f);
    fclose(f);
    if (got != (size_t)len) { free(buf); return HARNESS_FAIL; }
    buf[len] = 0;
    uint32_t rc = cr_ui_register_user_signs(buf, (uint32_t)len);
    free(buf);
    uint32_t n = cr_ui_user_sign_count();
    printf("{\"harness\":\"user-signs\",\"register_rc\":%lu,\"registered\":%lu}\n",
           (unsigned long)rc, (unsigned long)n);
    if (rc != CR_UI_OK) return HARNESS_FAIL;
    return HARNESS_OK;
}

/* ---- the scan lifecycle (shared by --scan and --resume) */

typedef struct {
    const char* target;      /* "volume" | "disk" */
    const char* letter;      /* single char for volume */
    uint32_t disk;           /* disk number for disk target */
    int whole_disk;
    const char* dest;        /* recovery destination */
    const char* out_dir;     /* evidence out dir */
    const char* label;       /* scenario label for the report */
    uint32_t stage_mask;
    uint32_t threads;
    DWORD cancel_after_ms;   /* 0 = no cancel probe */
} run_cfg_t;

static int ensure_dir(const char* path) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s", path);
    for (char* p = buf + 1; *p; p++) {
        if (*p == '\\' || *p == '/') { *p = 0; CreateDirectoryA(buf, NULL); *p = '\\'; }
    }
    CreateDirectoryA(buf, NULL);
    return 1;
}

static int run_scan_lifecycle(const run_cfg_t* cfg, cr_ui_session* resumed_session) {
    int exit_code = HARNESS_OK;
    char path[1024];
    LARGE_INTEGER t_total0 = qpc_now();

    /* ---- open the device */
    LARGE_INTEGER t_open0 = qpc_now();
    cr_ui_device* dev = NULL;
    uint32_t rc;
    if (cfg->target && strcmp(cfg->target, "volume") == 0) {
        rc = cr_ui_open_volume(cfg->letter[0], &dev);
    } else {
        rc = cr_ui_open_disk(cfg->disk, &dev);
    }
    LARGE_INTEGER t_open1 = qpc_now();
    if (rc != CR_UI_OK) {
        printf("{\"harness\":\"scan\",\"error\":\"open failed\",\"rc\":%lu,\"detail\":",
               (unsigned long)rc);
        json_escape_into(stdout, cr_ui_last_error());
        printf("}\n");
        return HARNESS_FAIL;
    }

    cr_ui_geometry geo;
    memset(&geo, 0, sizeof(geo));
    cr_ui_device_geometry(dev, &geo);
    char transport[32] = {0};
    uint32_t needed = 0;
    cr_ui_device_transport(dev, transport, (uint32_t)sizeof(transport), &needed);

    /* ---- scan create + attach (+ sinks) */
    sink_t sink;
    memset(&sink, 0, sizeof(sink));

    cr_ui_scan_config scfg;
    memset(&scfg, 0, sizeof(scfg));
    scfg.stage_mask = cfg->stage_mask;
    scfg.whole_disk = (uint32_t)cfg->whole_disk;
    scfg.threads = cfg->threads;

    cr_ui_scan* scan = NULL;
    LARGE_INTEGER t_scan_run0, t_scan_run1;

    if (resumed_session) {
        rc = cr_ui_session_resume(resumed_session, &scfg, &scan);
        if (rc != CR_UI_OK) {
            printf("{\"harness\":\"scan\",\"error\":\"session_resume failed\",\"rc\":%lu}\n", (unsigned long)rc);
            cr_ui_device_close(dev);
            return HARNESS_FAIL;
        }
        rc = cr_ui_scan_attach_device(scan, dev);
        if (rc != CR_UI_OK) { printf("{\"harness\":\"scan\",\"error\":\"attach failed\",\"rc\":%lu}\n", (unsigned long)rc); cr_ui_scan_close(scan); cr_ui_device_close(dev); return HARNESS_FAIL; }
        rc = cr_ui_scan_verify_identity(scan); /* must match (same volume) or BAD_CHECKSUM */
        if (rc == CR_UI_ERR_BAD_CHECKSUM) {
            printf("{\"harness\":\"scan\",\"error\":\"identity mismatch on resume\"}\n");
            cr_ui_scan_close(scan); cr_ui_device_close(dev);
            return HARNESS_FAIL;
        }
    } else {
        rc = cr_ui_scan_create(&scfg, &scan);
        if (rc != CR_UI_OK) { printf("{\"harness\":\"scan\",\"error\":\"scan_create failed\",\"rc\":%lu}\n", (unsigned long)rc); cr_ui_device_close(dev); return HARNESS_FAIL; }
        rc = cr_ui_scan_attach_device(scan, dev);
        if (rc != CR_UI_OK) { printf("{\"harness\":\"scan\",\"error\":\"attach failed\",\"rc\":%lu}\n", (unsigned long)rc); cr_ui_scan_close(scan); cr_ui_device_close(dev); return HARNESS_FAIL; }
        rc = cr_ui_scan_set_event_fn(scan, event_fn, &sink);
        if (rc != CR_UI_OK) { printf("{\"harness\":\"scan\",\"error\":\"set_event_fn failed\",\"rc\":%lu}\n", (unsigned long)rc); cr_ui_scan_close(scan); cr_ui_device_close(dev); return HARNESS_FAIL; }
    }

    /* optional cancel probe thread */
    HANDLE cancel_thread_h = NULL;
    cancel_req_t cancel_req;
    if (cfg->cancel_after_ms > 0 && !resumed_session) {
        cancel_req.scan = scan;
        cancel_req.after_ms = cfg->cancel_after_ms;
        cancel_thread_h = CreateThread(NULL, 0, cancel_thread, &cancel_req, 0, NULL);
    }

    /* ---- run (synchronous on this thread) */
    t_scan_run0 = qpc_now();
    rc = cr_ui_scan_run(scan);
    t_scan_run1 = qpc_now();
    int cancelled = (rc == CR_UI_ERR_CANCELLED);
    if (rc != CR_UI_OK && !cancelled) {
        printf("{\"harness\":\"scan\",\"error\":\"scan_run failed\",\"rc\":%lu,\"detail\":",
               (unsigned long)rc);
        json_escape_into(stdout, cr_ui_last_error());
        printf("}\n");
        cr_ui_scan_close(scan); cr_ui_device_close(dev);
        return HARNESS_FAIL;
    }
    if (cancel_thread_h) { WaitForSingleObject(cancel_thread_h, INFINITE); CloseHandle(cancel_thread_h); }

    /* ---- stats + telemetry + assistant */
    cr_ui_scan_stats stats;
    memset(&stats, 0, sizeof(stats));
    cr_ui_scan_get_stats(scan, &stats);
    cr_ui_telemetry tel;
    memset(&tel, 0, sizeof(tel));
    cr_ui_scan_telemetry(scan, &tel);
    cr_ui_assistant_stats assist;
    memset(&assist, 0, sizeof(assist));
    cr_ui_scan_assistant(scan, &assist);

    /* ---- save session (proves the save path; valid after a cancelled run too) */
    snprintf(path, sizeof(path), "%s\\session.crsf", cfg->out_dir);
    LARGE_INTEGER t_save0 = qpc_now();
    uint32_t save_rc = cr_ui_scan_save_session(scan, path);
    LARGE_INTEGER t_save1 = qpc_now();
    cr_ui_scan_close(scan);
    scan = NULL;
    if (save_rc != CR_UI_OK) {
        printf("{\"harness\":\"scan\",\"error\":\"save_session failed\",\"rc\":%lu}\n", (unsigned long)save_rc);
        cr_ui_device_close(dev);
        return HARNESS_FAIL;
    }

    /* ---- reload the session (proves the round-trip; the browse surface) */
    cr_ui_session* sess = NULL;
    rc = cr_ui_session_load(path, &sess);
    if (rc != CR_UI_OK) {
        printf("{\"harness\":\"scan\",\"error\":\"session_load failed\",\"rc\":%lu}\n", (unsigned long)rc);
        cr_ui_device_close(dev);
        return HARNESS_FAIL;
    }
    cr_ui_session_info info;
    memset(&info, 0, sizeof(info));
    cr_ui_session_get_info(sess, &info);

    /* ---- query: total count + deleted-only count (+ a small sample row set) */
    LARGE_INTEGER t_query0 = qpc_now();
    cr_ui_query q_all;
    memset(&q_all, 0, sizeof(q_all));
    q_all.count_only = 1;
    q_all.limit = 0;
    cr_ui_query_row* rows = NULL;
    uint32_t row_count = 0;
    uint64_t total_all = 0;
    rc = cr_ui_query_run(sess, &q_all, &rows, &row_count, &total_all);
    if (rc != CR_UI_OK) { printf("{\"harness\":\"scan\",\"error\":\"query(all) failed\",\"rc\":%lu}\n", (unsigned long)rc); cr_ui_session_close(sess); cr_ui_device_close(dev); return HARNESS_FAIL; }
    if (rows) { cr_ui_query_rows_free(rows, row_count); rows = NULL; row_count = 0; }

    cr_ui_query q_del;
    memset(&q_del, 0, sizeof(q_del));
    q_del.deleted_only = 1;
    q_del.count_only = 1;
    uint64_t total_deleted = 0;
    rc = cr_ui_query_run(sess, &q_del, &rows, &row_count, &total_deleted);
    if (rc != CR_UI_OK) { printf("{\"harness\":\"scan\",\"error\":\"query(deleted) failed\",\"rc\":%lu}\n", (unsigned long)rc); cr_ui_session_close(sess); cr_ui_device_close(dev); return HARNESS_FAIL; }
    if (rows) { cr_ui_query_rows_free(rows, row_count); rows = NULL; row_count = 0; }

    /* sample rows (first 100, any order) for the evidence record */
    cr_ui_query q_sample;
    memset(&q_sample, 0, sizeof(q_sample));
    q_sample.limit = 100;
    uint64_t dummy_total = 0;
    rc = cr_ui_query_run(sess, &q_sample, &rows, &row_count, &dummy_total);
    LARGE_INTEGER t_query1 = qpc_now();
    char sample_first_path[512] = "";
    uint64_t sample_first_node = 0;
    if (rc == CR_UI_OK && row_count > 0) {
        snprintf(sample_first_path, sizeof(sample_first_path), "%s", rows[0].path ? rows[0].path : "");
        sample_first_node = rows[0].node_id;
    }

    /* ---- preview probe: first deleted file - size + 4 KiB read + metadata */
    int preview_ok = 0;
    uint64_t preview_size = 0;
    uint32_t preview_placed = 0;
    char preview_kind[24] = "";
    cr_ui_preview* pv = NULL;
    {
        cr_ui_query q_del_row;
        memset(&q_del_row, 0, sizeof(q_del_row));
        q_del_row.deleted_only = 1;
        q_del_row.limit = 1;
        cr_ui_query_row* drows = NULL;
        uint32_t dcount = 0;
        uint64_t dtot = 0;
        if (cr_ui_query_run(sess, &q_del_row, &drows, &dcount, &dtot) == CR_UI_OK && dcount > 0) {
            if (cr_ui_preview_open_session(dev, sess, drows[0].node_id, &pv) == CR_UI_OK) {
                cr_ui_metadata meta;
                memset(&meta, 0, sizeof(meta));
                cr_ui_preview_size(pv, &preview_size);
                unsigned char buf[4096];
                if (cr_ui_preview_read(pv, 0, buf, (uint32_t)sizeof(buf), &preview_placed) == CR_UI_OK)
                    preview_ok = 1;
                if (cr_ui_preview_metadata(pv, &meta) == CR_UI_OK)
                    snprintf(preview_kind, sizeof(preview_kind), "%s", meta.kind);
                cr_ui_preview_close(pv);
            }
            cr_ui_query_rows_free(drows, dcount);
        }
    }

    /* ---- recovery: everything, hash-verify ON (the default config) */
    LARGE_INTEGER t_rec0 = qpc_now();
    cr_ui_recovery_config rcfg;
    memset(&rcfg, 0, sizeof(rcfg)); /* 0 = defaults: 1 MiB chunks, rename conflicts, hash verify ON */
    rcfg.progress = recovery_progress_fn;
    cr_ui_recovery* rec = NULL;
    rc = cr_ui_recovery_create(dev, &rcfg, &rec);
    if (rc != CR_UI_OK) {
        printf("{\"harness\":\"scan\",\"error\":\"recovery_create failed\",\"rc\":%lu}\n", (unsigned long)rc);
        if (rows) cr_ui_query_rows_free(rows, row_count);
        cr_ui_session_close(sess); cr_ui_device_close(dev);
        return HARNESS_FAIL;
    }
    rc = cr_ui_recovery_set_destination(rec, cfg->dest);
    if (rc != CR_UI_OK) {
        printf("{\"harness\":\"scan\",\"error\":\"set_destination failed\",\"rc\":%lu}\n", (unsigned long)rc);
        cr_ui_recovery_close(rec);
        if (rows) cr_ui_query_rows_free(rows, row_count);
        cr_ui_session_close(sess); cr_ui_device_close(dev);
        return HARNESS_FAIL;
    }

    /* enqueue every ROOT subtree of the session tree (recover-everything,
     * the CLI `recover` semantics; multi-root trees cover lost volumes) */
    cr_ui_snapshot* snap = NULL;
    uint32_t queued_roots = 0;
    if (cr_ui_session_snapshot(sess, &snap) == CR_UI_OK) {
        uint32_t node_count = 0;
        cr_ui_snapshot_node_count(snap, &node_count);
        char namebuf[1024];
        for (uint32_t i = 0; i < node_count; i++) {
            cr_ui_node_info ni;
            memset(&ni, 0, sizeof(ni));
            if (cr_ui_snapshot_node(snap, i, &ni, namebuf, (uint32_t)sizeof(namebuf)) != CR_UI_OK)
                continue;
            if (ni.parent == CR_UI_SNAP_NONE) {
                if (cr_ui_recovery_enqueue_subtree(rec, sess, ni.node_id) == CR_UI_OK)
                    queued_roots++;
            }
        }
        cr_ui_snapshot_close(snap);
    }
    uint32_t queued = 0;
    cr_ui_recovery_queue_count(rec, &queued);

    uint32_t proc_rc = cr_ui_recovery_process(rec);
    LARGE_INTEGER t_rec1 = qpc_now();
    cr_ui_recovery_stats rstats;
    memset(&rstats, 0, sizeof(rstats));
    cr_ui_recovery_get_stats(rec, &rstats);

    /* G-09 report (heap string -> file -> free) */
    char* report_json = NULL;
    char report_path[1024];
    snprintf(report_path, sizeof(report_path), "%s\\report.json", cfg->out_dir);
    uint32_t report_rc = cr_ui_recovery_report_json(rec, cfg->label, &report_json);
    if (report_rc == CR_UI_OK && report_json) {
        write_text_file(report_path, report_json);
        cr_ui_string_free(report_json);
    }
    cr_ui_recovery_close(rec);

    if (rows) cr_ui_query_rows_free(rows, row_count);
    cr_ui_session_close(sess);
    cr_ui_device_close(dev);

    /* partial = the CLI recover exit-2 shape: errors or hash mismatch */
    if (proc_rc == CR_UI_OK) {
        if (rstats.errors > 0 || rstats.hash_mismatch > 0) exit_code = HARNESS_PARTIAL;
    } else if (proc_rc == CR_UI_ERR_CANCELLED) {
        exit_code = HARNESS_PARTIAL;
    } else {
        printf("{\"harness\":\"scan\",\"error\":\"recovery_process failed\",\"rc\":%lu}\n", (unsigned long)proc_rc);
        return HARNESS_FAIL;
    }

    /* ---- api-result.json (fixed field order; every timing + counter) */
    LARGE_INTEGER t_total1 = qpc_now();
    snprintf(path, sizeof(path), "%s\\api-result.json", cfg->out_dir);
    FILE* f = fopen(path, "wb");
    if (!f) return HARNESS_FAIL;
    fprintf(f, "{\n");
    fprintf(f, "  \"harness\": \"cr_ui-api-lane\",\n");
    fprintf(f, "  \"label\": "); json_escape_into(f, cfg->label); fprintf(f, ",\n");
    fprintf(f, "  \"target\": \"%s\", \"whole_disk\": %s, \"stage_mask\": %lu, \"threads\": %lu,\n",
            cfg->target, cfg->whole_disk ? "true" : "false",
            (unsigned long)cfg->stage_mask, (unsigned long)cfg->threads);
    fprintf(f, "  \"transport\": "); json_escape_into(f, transport); fprintf(f, ",\n");
    fprintf(f, "  \"geometry\": {\"total_bytes\": %llu, \"logical_sector\": %lu, \"physical_sector\": %lu, \"sector_count\": %llu},\n",
            (unsigned long long)geo.total_bytes, (unsigned long)geo.logical_sector,
            (unsigned long)geo.physical_sector, (unsigned long long)geo.sector_count);
    fprintf(f, "  \"session_info\": {\"fs_family\": %lu, \"stage_done_mask\": %lu, \"configured_mask\": %lu, \"scanned_bytes\": %llu, \"elapsed_ms\": %llu, \"boot_hash\": %lu},\n",
            (unsigned long)info.fs_family, (unsigned long)info.stage_done_mask,
            (unsigned long)info.configured_mask, (unsigned long long)info.scanned_bytes,
            (unsigned long long)info.elapsed_ms, (unsigned long)info.boot_hash);
    fprintf(f, "  \"cancelled\": %s,\n", cancelled ? "true" : "false");
    fprintf(f, "  \"timings_ms\": {\"open\": %.1f, \"scan_run\": %.1f, \"save_session\": %.1f, \"query\": %.1f, \"recovery\": %.1f, \"total\": %.1f},\n",
            qpc_ms(t_open0, t_open1), qpc_ms(t_scan_run0, t_scan_run1),
            qpc_ms(t_save0, t_save1), qpc_ms(t_query0, t_query1),
            qpc_ms(t_rec0, t_rec1), qpc_ms(t_total0, t_total1));
    fprintf(f, "  \"scan_stats\": {\"files_found\": %llu, \"deleted_found\": %llu, \"carved_found\": %llu, \"lost_partitions\": %llu, \"bytes_scanned\": %llu, \"bytes_unreadable\": %llu, \"elapsed_ms\": %llu, \"error_ranges\": %lu, \"probe_unreadable\": %lu},\n",
            (unsigned long long)stats.files_found, (unsigned long long)stats.deleted_found,
            (unsigned long long)stats.carved_found, (unsigned long long)stats.lost_partitions,
            (unsigned long long)stats.bytes_scanned, (unsigned long long)stats.bytes_unreadable,
            (unsigned long long)stats.elapsed_ms, (unsigned long)stats.error_ranges,
            (unsigned long)stats.probe_unreadable);
    fprintf(f, "  \"telemetry\": {\"overall_permille\": %lu, \"bytes_scanned\": %llu, \"elapsed_ms\": %llu},\n",
            (unsigned long)sink.last_overall_permille, (unsigned long long)tel.bytes_scanned,
            (unsigned long long)tel.elapsed_ms);
    fprintf(f, "  \"assistant\": {\"total_file_count\": %llu, \"matched\": %llu, \"total\": %llu, \"small_result\": %lu, \"graded_count\": %llu},\n",
            (unsigned long long)assist.total_file_count, (unsigned long long)assist.matched,
            (unsigned long long)assist.total, (unsigned long)assist.small_result,
            (unsigned long long)assist.graded_count);
    fprintf(f, "  \"events\": {\"stage\": %ld, \"progress\": %ld, \"files_found\": %ld, \"error_range\": %ld, \"checkpoint\": %ld, \"lost_candidate\": %ld, \"repair\": %ld, \"heartbeat\": %ld, \"save_progress\": %ld, \"fs_check\": %ld, \"other\": %ld},\n",
            (long)sink.events_stage, (long)sink.events_progress, (long)sink.events_files_found,
            (long)sink.events_error_range, (long)sink.events_checkpoint,
            (long)sink.events_lost_candidate, (long)sink.events_repair,
            (long)sink.events_heartbeat, (long)sink.events_save_progress,
            (long)sink.events_fs_check, (long)sink.events_other);
    fprintf(f, "  \"query\": {\"total_all\": %llu, \"total_deleted\": %llu, \"sample_first_path\": ",
            (unsigned long long)total_all, (unsigned long long)total_deleted);
    json_escape_into(f, sample_first_path);
    fprintf(f, ", \"sample_first_node_id\": %llu},\n", (unsigned long long)sample_first_node);
    fprintf(f, "  \"preview\": {\"ok\": %s, \"size\": %llu, \"read_placed\": %lu, \"kind\": ",
            preview_ok ? "true" : "false", (unsigned long long)preview_size,
            (unsigned long)preview_placed);
    json_escape_into(f, preview_kind);
    fprintf(f, "},\n");
    fprintf(f, "  \"recovery\": {\"process_rc\": %lu, \"queued_roots\": %lu, \"queued\": %llu, \"done\": %llu, \"dirs\": %llu, \"refused\": %llu, \"errors\": %llu, \"hash_mismatch\": %llu, \"resumed\": %llu, \"salvaged\": %llu, \"nothing_to_recover\": %llu},\n",
            (unsigned long)proc_rc, (unsigned long)queued_roots,
            (unsigned long long)rstats.queued, (unsigned long long)rstats.done,
            (unsigned long long)rstats.dirs, (unsigned long long)rstats.refused,
            (unsigned long long)rstats.errors, (unsigned long long)rstats.hash_mismatch,
            (unsigned long long)rstats.resumed, (unsigned long long)rstats.salvaged,
            (unsigned long long)rstats.nothing_to_recover);
    fprintf(f, "  \"report\": {\"rc\": %lu, \"path\": ", (unsigned long)report_rc);
    json_escape_into(f, report_path);
    fprintf(f, "}\n}\n");
    fclose(f);
    printf("api-lane complete: exit=%d queued=%llu done=%llu errors=%llu hash_mismatch=%llu (result: %s\\api-result.json)\n",
           exit_code, (unsigned long long)rstats.queued, (unsigned long long)rstats.done,
           (unsigned long long)rstats.errors, (unsigned long long)rstats.hash_mismatch, cfg->out_dir);
    return exit_code;
}

/* ---- the cancel+resume probe (one --scan with --cancel-after, then --resume) */

static int mode_resume(const char* session_path, const run_cfg_t* cfg) {
    cr_ui_session* sess = NULL;
    uint32_t rc = cr_ui_session_load(session_path, &sess);
    if (rc != CR_UI_OK) {
        printf("{\"harness\":\"resume\",\"error\":\"session_load failed\",\"rc\":%lu}\n", (unsigned long)rc);
        return HARNESS_FAIL;
    }
    run_cfg_t cfg2 = *cfg;
    cfg2.cancel_after_ms = 0;
    int ec = run_scan_lifecycle(&cfg2, sess);
    cr_ui_session_close(sess);
    return ec;
}

int main(int argc, char** argv) {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    g_freq_inv_ms = 1000.0 / (double)freq.QuadPart;

    if (argc < 2) {
        fprintf(stderr,
                "usage: api_harness --abi-version | --carve-count\n"
                "       api_harness --user-signs <json-file>\n"
                "       api_harness --scan --target volume --letter Y --dest D --out O\n"
                "                    [--stages quick|deep|deepcarve|all] [--threads N]\n"
                "                    [--cancel-after SECONDS] [--label STR]\n"
                "       api_harness --scan --target disk --disk N --whole-disk --dest D --out O [...]\n"
                "       api_harness --resume --session F --target volume --letter Y --dest D --out O\n");
        return HARNESS_FAIL;
    }

    if (strcmp(argv[1], "--abi-version") == 0) return mode_abi_version();
    if (strcmp(argv[1], "--carve-count") == 0) return mode_carve_count();
    if (strcmp(argv[1], "--user-signs") == 0) {
        if (argc < 3) { fprintf(stderr, "--user-signs needs a file\n"); return HARNESS_FAIL; }
        return mode_user_signs(argv[2]);
    }

    int is_scan = (strcmp(argv[1], "--scan") == 0);
    int is_resume = (strcmp(argv[1], "--resume") == 0);
    if (!is_scan && !is_resume) {
        fprintf(stderr, "unknown mode: %s\n", argv[1]);
        return HARNESS_FAIL;
    }

    run_cfg_t cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.stage_mask = 7; /* all stages (the CLI default quick|deep|carve) */
    cfg.threads = 4;
    cfg.label = "api-lane";
    const char* session_path = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--target") == 0 && i + 1 < argc) cfg.target = argv[++i];
        else if (strcmp(argv[i], "--letter") == 0 && i + 1 < argc) cfg.letter = argv[++i];
        else if (strcmp(argv[i], "--disk") == 0 && i + 1 < argc) cfg.disk = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--whole-disk") == 0) cfg.whole_disk = 1;
        else if (strcmp(argv[i], "--dest") == 0 && i + 1 < argc) cfg.dest = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) cfg.out_dir = argv[++i];
        else if (strcmp(argv[i], "--label") == 0 && i + 1 < argc) cfg.label = argv[++i];
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) cfg.threads = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--cancel-after") == 0 && i + 1 < argc) cfg.cancel_after_ms = (DWORD)(strtoul(argv[++i], NULL, 10) * 1000);
        else if (strcmp(argv[i], "--stages") == 0 && i + 1 < argc) {
            const char* s = argv[++i];
            if (strcmp(s, "quick") == 0) cfg.stage_mask = 1;
            else if (strcmp(s, "deep") == 0) cfg.stage_mask = 3; /* quick+deep (the CLI --no-carve shape) */
            else if (strcmp(s, "deepcarve") == 0) cfg.stage_mask = 7;
            else if (strcmp(s, "all") == 0) cfg.stage_mask = 7;
            else { fprintf(stderr, "unknown --stages value: %s\n", s); return HARNESS_FAIL; }
        }
        else if (strcmp(argv[i], "--session") == 0 && i + 1 < argc) session_path = argv[++i];
        else { fprintf(stderr, "unknown argument: %s\n", argv[i]); return HARNESS_FAIL; }
    }

    if (!cfg.target || !cfg.dest || !cfg.out_dir) {
        fprintf(stderr, "--target, --dest and --out are required\n");
        return HARNESS_FAIL;
    }
    if (strcmp(cfg.target, "volume") == 0) {
        if (!cfg.letter || !cfg.letter[0]) { fprintf(stderr, "--letter is required for volume targets\n"); return HARNESS_FAIL; }
    } else if (strcmp(cfg.target, "disk") == 0) {
        /* disk number defaults handled above; letter unused */
    } else {
        fprintf(stderr, "--target must be volume or disk\n");
        return HARNESS_FAIL;
    }
    if (is_resume && !session_path) {
        fprintf(stderr, "--resume needs --session\n");
        return HARNESS_FAIL;
    }
    ensure_dir(cfg.out_dir);
    ensure_dir(cfg.dest);

    if (is_resume) return mode_resume(session_path, &cfg);
    return run_scan_lifecycle(&cfg, NULL);
}
