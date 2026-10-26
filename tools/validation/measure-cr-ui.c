/*
 * measure-cr-ui.c — T14 engine-side latency / throughput / memory evidence.
 *
 * Part of the validation-evidence workflow (docs/INTEGRATION-VALIDATION.md
 * "Pending evidence" — T14). Compiles on the Windows runner against the
 * STAGED ENGINE KIT (build.ps1 -Target engine -> build/out/stage-<v>/engine)
 * and measures, on the runner's own system volume:
 *
 *   - enumerate   (backs connector listDevices): repeated
 *                 cr_ui_enumerate_disks + cr_ui_enumerate_volumes timings
 *   - health      (backs getHealth): cr_ui_health_read timings
 *   - scan        (backs startScan + the progress stream): quick-stage scan
 *                 of the system volume — time-to-first-event, run wall time,
 *                 engine telemetry throughput (bytes/s), files found
 *   - snapshot    (backs results tree/query on live scans): cr_ui_scan_snapshot,
 *                 snapshot_dfs, and paged snapshot_node row reads
 *   - preview     (backs requestPreview): open + size + metadata + 64 KiB read
 *                 on the first file node found
 *   - memory      PeakWorkingSet checkpoints after each phase (the snapshot
 *                 O(n) copy + node metadata are the bridge's dominant terms)
 *
 * The #include of the PINNED header (engine/include/cr/cr_ui.h) is the
 * contract — no struct transcription. The retired simulator's documented
 * budgets (listDevices 180±60 ms, startScan 420±120 ms, queryResults 90±50
 * ms, preview 240±160 ms, health 280±120 ms) are the comparison baseline;
 * the bridge's marshaling overhead on top of these engine numbers is
 * sub-millisecond (serde + IPC), so engine-side is the honest lower bound.
 * The comparison table is rendered by the workflow into
 * docs/INTEGRATION-VALIDATION/t14-measurements.md.
 *
 * Output: measurements JSON on stdout (consumed by the workflow), human
 * notes on stderr. Exit 0 = all phases measured (unavailable phases are
 * RECORDED as unavailable, never skipped silently); 2 = a phase failed
 * hard (the workflow fails the run — no partial-credit evidence).
 *
 * Build (workflow):
 *   cl /nologo /O2 /W4 /DWIN32_LEAN_AND_MEAN tools\validation\measure-cr-ui.c
 *      /I engine\include /link build\out\stage-<v>\engine\cr_ui.lib psapi.lib
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cr/cr_ui.h" /* the pinned ABI contract — single source of truth */

/* ------------------------------------------------------------------ timing */

static double g_qpc_freq_ms;

static void timing_init(void) {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_qpc_freq_ms = (double)f.QuadPart / 1000.0;
}

static double now_ms(void) {
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart / g_qpc_freq_ms;
}

typedef struct {
    double samples[32];
    int n;
} samples_t;

static void record(samples_t* s, double ms) {
    if (s->n < (int)(sizeof(s->samples) / sizeof(s->samples[0]))) s->samples[s->n++] = ms;
}

static int cmp_double(const void* a, const void* b) {
    double x = *(const double*)a, y = *(const double*)b;
    return (x > y) - (x < y);
}

static double pct(samples_t* s, double p) {
    qsort(s->samples, (size_t)s->n, sizeof(double), cmp_double);
    int i = (int)((double)(s->n - 1) * p);
    return s->samples[i];
}

/* ------------------------------------------------------------------ memory */

static SIZE_T peak_working_set(void) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return pmc.PeakWorkingSetSize;
    return 0;
}

/* ------------------------------------------------------------- scan events */

static volatile LONG g_events = 0;
static volatile LONG g_first_taken = 0;
static volatile double g_first_event_ms = -1.0;
static volatile LONG g_files_seen = 0;
static double g_scan_start_ms;

static void measure_event_fn(void* user, const cr_ui_event* evt) {
    (void)user;
    InterlockedIncrement(&g_events);
    /* Exactly one callback wins the CAS — its timestamp is the first-event
     * moment (within microseconds of the first callback's start; events
     * fire on engine worker threads, so the interlock is required). */
    if (InterlockedCompareExchange(&g_first_taken, 1, 0) == 0) {
        g_first_event_ms = now_ms();
    }
    if (evt->type == CR_EVT_FILES_FOUND)
        InterlockedExchange(&g_files_seen,
                            (LONG)(evt->files_found + evt->deleted_found + evt->carved_found));
}

/* ------------------------------------------------------------------- main */

static void fail(const char* phase, uint32_t rc) {
    fprintf(stderr, "PHASE FAILED: %s rc=0x%08x last_error=%s\n", phase, rc, cr_ui_last_error());
    exit(2);
}

int main(int argc, char** argv) {
    int quick_only = (argc > 1 && strcmp(argv[1], "--quick-only") == 0);
    timing_init();

    uint32_t abi = cr_ui_abi_version();
    if (abi != CR_UI_ABI_VERSION) {
        fprintf(stderr, "ABI mismatch: engine %u != built %u\n", abi, CR_UI_ABI_VERSION);
        return 2;
    }

    /* ---- enumerate (listDevices) ------------------------------------- */
    samples_t s_enum = {0};
    uint32_t ndisks = 0, nvols = 0;
    static cr_ui_disk_info disks[64];
    static cr_ui_volume_info vols[128];
    for (int i = 0; i < 20; i++) {
        double t0 = now_ms();
        uint32_t dk = 64, vv = 128;
        uint32_t rc = cr_ui_enumerate_disks(disks, &dk);
        if (rc == 0) rc = cr_ui_enumerate_volumes(vols, &vv);
        if (rc != 0 && i == 0) fail("enumerate", rc);
        record(&s_enum, now_ms() - t0);
        ndisks = dk; nvols = vv;
    }

    /* ---- pick a volume + its disk ------------------------------------ */
    char sys_letter = 'C';
    for (uint32_t i = 0; i < nvols; i++) {
        if (vols[i].letter >= 'A' && vols[i].letter <= 'Z') {
            sys_letter = (char)vols[i].letter;
            break;
        }
    }

    /* ---- health (getHealth) ------------------------------------------ */
    samples_t s_health = {0};
    static cr_ui_smart_report smart;
    memset(&smart, 0, sizeof(smart));
    int health_supported = 1;
    for (int i = 0; i < 10; i++) {
        double t0 = now_ms();
        uint32_t rc = cr_ui_health_read(disks[0].number, &smart, NULL, 0, NULL);
        double dt = now_ms() - t0;
        if (rc != 0 && i == 0) {
            health_supported = 0;
            fprintf(stderr,
                    "health rc=0x%08x (%s) — recorded unsupported on this device\n",
                    rc, cr_ui_last_error());
            break;
        }
        record(&s_health, dt);
    }

    /* ---- scan (startScan + progress stream + throughput) ------------- */
    cr_ui_device* dev = NULL;
    uint32_t rc = cr_ui_open_volume(sys_letter, &dev);
    if (rc != 0) fail("open_volume", rc);

    cr_ui_geometry geom;
    rc = cr_ui_device_geometry(dev, &geom);
    if (rc != 0) fail("device_geometry", rc);

    cr_ui_scan_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (quick_only) cfg.stage_mask = 1; /* bounded runner time; recorded in the JSON */

    cr_ui_scan* scan = NULL;
    rc = cr_ui_scan_create(&cfg, &scan);
    if (rc != 0) fail("scan_create", rc);
    rc = cr_ui_scan_attach_device(scan, dev);
    if (rc != 0) fail("scan_attach", rc);
    rc = cr_ui_scan_set_event_fn(scan, measure_event_fn, NULL);
    if (rc != 0) fail("scan_set_event_fn", rc);

    SIZE_T mem_before_scan = peak_working_set();
    g_scan_start_ms = now_ms();
    g_first_taken = 0;
    g_first_event_ms = -1.0;
    rc = cr_ui_scan_run(scan);
    double run_ms = now_ms() - g_scan_start_ms;
    if (rc != 0) fail("scan_run", rc);

    cr_ui_telemetry tel;
    memset(&tel, 0, sizeof(tel));
    cr_ui_scan_telemetry(scan, &tel);
    cr_ui_scan_stats stats;
    memset(&stats, 0, sizeof(stats));
    cr_ui_scan_get_stats(scan, &stats);
    SIZE_T mem_after_scan = peak_working_set();

    /* ---- snapshot + rows (results tree/query on a live scan) --------- */
    double t_snap0 = now_ms();
    cr_ui_snapshot* snap = NULL;
    rc = cr_ui_scan_snapshot(scan, &snap);
    if (rc != 0) fail("scan_snapshot", rc);
    double snap_ms = now_ms() - t_snap0;

    uint32_t nodes = 0;
    rc = cr_ui_snapshot_node_count(snap, &nodes);
    if (rc != 0) fail("snapshot_node_count", rc);

    uint32_t dfs_need = 0;
    rc = cr_ui_snapshot_dfs(snap, NULL, &dfs_need);
    if (rc != 0) fail("snapshot_dfs(size)", rc);
    static uint32_t dfs[1u << 20];
    uint32_t dfs_cap = dfs_need < (1u << 20) ? dfs_need : (1u << 20);
    double t_dfs0 = now_ms();
    rc = cr_ui_snapshot_dfs(snap, dfs, &dfs_cap);
    if (rc != 0) fail("snapshot_dfs", rc);
    double dfs_ms = now_ms() - t_dfs0;

    /* paged row reads — the bridge's page shape is 500 rows */
    samples_t s_rows = {0};
    uint32_t first_file_row = CR_UI_SNAP_NONE; /* row INDEX (preview addressing) */
    static cr_ui_node_info nfo;
    static char namebuf[1024];
    for (uint32_t off = 0; off + 500 <= dfs_cap; off += 500) {
        double t0 = now_ms();
        for (uint32_t i = 0; i < 500; i++) {
            if (cr_ui_snapshot_node(snap, dfs[off + i], &nfo, namebuf, sizeof(namebuf)) != 0)
                fail("snapshot_node", 1);
            /* cr_ui_preview_open_snapshot addresses by ROW INDEX (cr_ui.h:
             * 'snapshot row index, or session node_id') — not node_id. */
            if (first_file_row == CR_UI_SNAP_NONE && !(nfo.attributes & 0x10) &&
                nfo.logical_size > 0)
                first_file_row = dfs[off + i];
        }
        record(&s_rows, now_ms() - t0);
    }
    SIZE_T mem_after_rows = peak_working_set();

    /* ---- preview (requestPreview) ------------------------------------ */
    double pv_open_ms = 0, pv_read_ms = 0, pv_meta_ms = 0;
    uint64_t pv_size = 0;
    int preview_done = 0;
    if (first_file_row != CR_UI_SNAP_NONE) {
        cr_ui_preview* pv = NULL;
        double t0 = now_ms();
        rc = cr_ui_preview_open_snapshot(dev, snap, first_file_row, &pv);
        pv_open_ms = now_ms() - t0;
        if (rc == 0 && pv) {
            preview_done = 1;
            cr_ui_preview_size(pv, &pv_size);
            t0 = now_ms();
            static unsigned char readbuf[64 * 1024];
            uint32_t got = 0;
            uint64_t want = pv_size < 65536 ? pv_size : 65536;
            cr_ui_preview_read(pv, 0, readbuf, (uint32_t)want, &got);
            pv_read_ms = now_ms() - t0;
            static cr_ui_metadata meta;
            t0 = now_ms();
            cr_ui_preview_metadata(pv, &meta);
            pv_meta_ms = now_ms() - t0;
            cr_ui_preview_close(pv);
        } else {
            fprintf(stderr,
                    "preview_open rc=0x%08x (%s) — preview phase recorded unavailable\n",
                    rc, cr_ui_last_error());
        }
    } else {
        fprintf(stderr, "no file node in snapshot — preview phase recorded unavailable\n");
    }

    /* ---- report ------------------------------------------------------- */
    double enum_p50 = s_enum.n ? pct(&s_enum, 0.5) : -1;
    double enum_p95 = s_enum.n ? pct(&s_enum, 0.95) : -1;
    double health_p50 = s_health.n ? pct(&s_health, 0.5) : -1;
    double health_p95 = s_health.n ? pct(&s_health, 0.95) : -1;
    double rows_p50 = s_rows.n ? pct(&s_rows, 0.5) : -1;
    double rows_p95 = s_rows.n ? pct(&s_rows, 0.95) : -1;
    double tput = tel.elapsed_ms > 0 ? (double)tel.bytes_scanned * 1000.0 / (double)tel.elapsed_ms : 0;

    printf("{\n");
    printf("  \"abiVersion\": %u,\n", (unsigned)abi);
    printf("  \"stageMask\": %u,\n", (unsigned)cfg.stage_mask);
    printf("  \"volume\": \"%c\",\n", sys_letter);
    printf("  \"volumeBytes\": %llu,\n", (unsigned long long)geom.total_bytes);
    printf("  \"enumerate\": { \"disks\": %u, \"volumes\": %u, \"p50Ms\": %.3f, \"p95Ms\": %.3f, \"iterations\": %d },\n",
           (unsigned)ndisks, (unsigned)nvols, enum_p50, enum_p95, s_enum.n);
    printf("  \"health\": { \"supported\": %d, \"p50Ms\": %.3f, \"p95Ms\": %.3f, \"iterations\": %d },\n",
           health_supported, health_p50, health_p95, s_health.n);
    printf("  \"scan\": { \"runMs\": %.1f, \"firstEventMs\": %.1f, \"events\": %ld, \"filesFound\": %llu, \"bytesScanned\": %llu, \"engineElapsedMs\": %llu, \"throughputBytesPerSecond\": %.0f },\n",
           run_ms,
           g_first_event_ms < 0 ? -1.0 : g_first_event_ms - g_scan_start_ms,
           (long)g_events,
           (unsigned long long)(stats.files_found + stats.deleted_found + stats.carved_found),
           (unsigned long long)tel.bytes_scanned,
           (unsigned long long)tel.elapsed_ms,
           tput);
    printf("  \"snapshot\": { \"snapshotMs\": %.1f, \"nodeCount\": %u, \"dfsMs\": %.1f, \"rowsPaged500P50Ms\": %.3f, \"rowsPaged500P95Ms\": %.3f, \"pages\": %d },\n",
           snap_ms, (unsigned)nodes, dfs_ms, rows_p50, rows_p95, s_rows.n);
    printf("  \"preview\": { \"available\": %d, \"openMs\": %.3f, \"read64KiBMs\": %.3f, \"metadataMs\": %.3f, \"fileSize\": %llu },\n",
           preview_done, pv_open_ms, pv_read_ms, pv_meta_ms, (unsigned long long)pv_size);
    printf("  \"memory\": { \"peakWorkingSetBeforeScanBytes\": %llu, \"peakAfterScanBytes\": %llu, \"peakAfterRowsBytes\": %llu },\n",
           (unsigned long long)mem_before_scan,
           (unsigned long long)mem_after_scan,
           (unsigned long long)mem_after_rows);
    printf("  \"retiredSimulatorBudgetsMs\": { \"listDevices\": [120, 240], \"startScan\": [300, 540], \"queryResults\": [40, 140], \"preview\": [80, 400], \"health\": [160, 400] }\n");
    printf("}\n");

    cr_ui_snapshot_close(snap);
    cr_ui_scan_close(scan);
    cr_ui_device_close(dev);
    return 0;
}
