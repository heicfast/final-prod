/* cr_ui_abi_check.c - G-01/W2-3 acceptance: a PLAIN C consumer builds a
 * FAT16 image on disk, then exercises the full cr_ui lifecycle through the
 * header alone (compiled -std=c11 -Wall -Wextra -Werror, linked against
 * the shared object; run under ASan on the linux CI lane).
 *
 * Lifecycle covered: ctx, scan (image file), stats, snapshot + node
 * enumeration + children paging, query (substring + count-only), preview
 * (bounded read + metadata JSON), recovery through caller-supplied IO
 * callbacks (byte-verified content), the machine-readable report, session
 * save, and complete handle teardown.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#include "cr/cr_ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr)                                                            \
    do {                                                                       \
        const uint32_t _rc = (expr);                                           \
        if (_rc != 0) {                                                        \
            fprintf(stderr, "FAIL %s:%d: %s -> 0x%08x (%s)\n", __FILE__,       \
                    __LINE__, #expr, _rc, cr_ui_last_error());                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void put16(unsigned char* b, int at, unsigned v) {
    b[at] = (unsigned char)(v & 0xFF);
    b[at + 1] = (unsigned char)((v >> 8) & 0xFF);
}
static void put32(unsigned char* b, int at, unsigned long v) {
    for (int i = 0; i < 4; ++i)
        b[at + i] = (unsigned char)((v >> (8 * i)) & 0xFF);
}

/* the FAT16 fixture from the C++ suite (test_fat_volume.cpp), C-ified:
 * reserved 2 | FAT 2x20 | root 32 | data (cluster 2 at sector 74) */
static unsigned char g_img[5074 * 512];

static void dentry(unsigned char* slot, const char* name11, unsigned attr,
                   unsigned first_cluster, unsigned size, int deleted) {
    memcpy(slot, name11, 11);
    if (deleted)
        slot[0] = 0xE5;
    /* DirEntry wire layout (cr_fat_wire.h): name[11]@0, attr@11, nt@12,
     * tenths@13, ctime@14, cdate@16, adate@18, CLUSTER_HIGH@20,
     * wtime@22, wdate@24, CLUSTER_LOW@26, size@28 */
    slot[11] = (unsigned char)attr;
    slot[12] = 0;
    slot[13] = 0;
    put16(slot, 14, 0x6000);          /* creation time */
    put16(slot, 16, 0x5A46);          /* creation date */
    put16(slot, 18, 0x5A46);          /* access date */
    put16(slot, 20, 0);               /* first cluster high (FAT16: 0!) */
    put16(slot, 22, 0x6000);          /* write time */
    put16(slot, 24, 0x5A46);          /* write date */
    put16(slot, 26, (unsigned)first_cluster & 0xFFFF);
    put32(slot, 28, size);
}

static void build_image(void) {
    const unsigned FAT1 = 2u * 512u;
    const unsigned FAT2 = 22u * 512u;
    const unsigned ROOT = 42u * 512u;
    const unsigned DATA = 74u * 512u;

    unsigned char* b = g_img;
    memcpy(b + 3, "MSWIN4.1", 8);
    put16(b, 0x0B, 512);
    b[0x0D] = 1; /* spc */
    put16(b, 0x0E, 2);
    b[0x10] = 2; /* FAT count */
    put16(b, 0x11, 512);
    put16(b, 0x13, 5074);
    b[0x15] = 0xF8;
    put16(b, 0x16, 20);
    put16(b, 0x18, 63);
    put16(b, 0x1A, 255);
    put32(b, 0x20, 0);
    b[0x26] = 0x29;
    put32(b, 0x27, 0x1234);
    memcpy(b + 0x2B, "TESTVOLUME ", 11);
    memcpy(b + 0x36, "FAT16   ", 8);
    b[510] = 0x55;
    b[511] = 0xAA;

#define LINK(c, v)                                                             \
    do {                                                                       \
        put16(b + FAT1, (c) * 2, (v));                                         \
        put16(b + FAT2, (c) * 2, (v));                                         \
    } while (0)
    LINK(10, 11);
    LINK(11, 12);
    LINK(12, 0xFFFF);
    LINK(20, 21);
    LINK(21, 0xFFFF);
    LINK(30, 0x0000);
    LINK(5, 0xFFFF);
    LINK(15, 0xFFFF);
#undef LINK

    unsigned char* root = b + ROOT;
    /* plain 8.3 entries (the LFN machinery is C++ suite territory; the
     * shim test needs valid names, not long ones) */
    dentry(root + 0 * 32, "README  TXT", 0x20, 10, 1200, 0);
    dentry(root + 1 * 32, "DEL     TXT", 0x20, 20, 1000, 1);
    dentry(root + 2 * 32, "GONE    TXT", 0x20, 30, 500, 1);
    dentry(root + 3 * 32, "SUBDIR     ", 0x10, 5, 0, 0);

    unsigned char* sub = b + DATA + (5 - 2) * 512;
    dentry(sub + 0 * 32, ".          ", 0x10, 5, 0, 0);
    dentry(sub + 1 * 32, "..         ", 0x10, 5, 0, 0);
    dentry(sub + 2 * 32, "INNER   TXT", 0x20, 15, 512, 0);

    memcpy(b + DATA + (10 - 2) * 512, "READMEDATA", 10);
    memcpy(b + DATA + (15 - 2) * 512, "INNERDATA", 9);
}

/* ---- caller-supplied destination IO (recovery surface) ---- */

struct MemFile {
    unsigned char data[4096];
    uint64_t len;
    int committed;
};
static struct MemFile g_files[16];
static char g_names[16][64];

static uint32_t io_create_temp(void* user, uint64_t item_id, const char* final_path,
                               const char* suffix) {
    (void)user;
    (void)suffix;
    /* the writer's item ids are 1..; key the slot on the id so
     * write/commit see the same file (exactly the DestIo contract) */
    if (item_id >= 16)
        return 1;
    memset(&g_files[item_id], 0, sizeof(g_files[0]));
    snprintf(g_names[item_id], sizeof(g_names[0]), "%s", final_path);
    (void)final_path;
    return 0;
}
static uint32_t io_write(void* user, uint64_t id, uint64_t at, const void* data, uint32_t n) {
    (void)user;
    if (id >= 16)
        return 1;
    struct MemFile* f = &g_files[id];
    if (at + n > sizeof(f->data))
        return 1;
    memcpy(f->data + at, data, n);
    if (at + n > f->len)
        f->len = at + n;
    return 0;
}
static uint32_t io_commit(void* user, uint64_t id) {
    (void)user;
    if (id >= 16)
        return 1;
    g_files[id].committed = 1;
    return 0;
}
static void io_close(void* user, uint64_t id) {
    (void)user;
    (void)id;
}
static uint8_t io_exists(void* user, const char* final_path) {
    (void)user;
    (void)final_path;
    return 0;
}
static uint32_t io_mkdir(void* user, const char* path) {
    (void)user;
    (void)path;
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: cr_ui_abi_check <tmpdir>\n");
        return 2;
    }
    char img_path[512], ses_path[512];
    snprintf(img_path, sizeof(img_path), "%s/cr_ui_fat16.img", argv[1]);
    snprintf(ses_path, sizeof(ses_path), "%s/cr_ui_session.crsf", argv[1]);

    if (cr_ui_abi_version() != CR_UI_ABI_VERSION) {
        fprintf(stderr, "FAIL: ABI mismatch\n");
        return 1;
    }

    build_image();
    FILE* f = fopen(img_path, "wb");
    if (!f) {
        perror("tmp image");
        return 1;
    }
    fwrite(g_img, 1, sizeof(g_img), f);
    fclose(f);

    /* 1. context + scan */
    cr_ui_ctx* ctx = NULL;
    CHECK(cr_ui_ctx_new(&ctx));
    cr_ui_scan_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.abi_size = (uint32_t)sizeof(cfg);
    cfg.run_quick = 1;
    cfg.run_deep = 0;
    cfg.run_carve = 0;
    cfg.whole_disk = 0;
    cfg.threads = 1;
    memcpy(cfg.volume_label, "shim", 4);

    cr_ui_scan* scan = NULL;
    CHECK(cr_ui_scan_start_file(ctx, &cfg, img_path, &scan));

    static volatile int progress_calls = 0;
    cr_ui_progress_fn prog = NULL; /* exercised via the run signature */
    (void)prog;
    (void)progress_calls;
    CHECK(cr_ui_scan_run(scan, NULL, NULL));

    uint64_t files = 0, deleted = 0, carved = 0;
    CHECK(cr_ui_scan_stats(scan, &files, &deleted, &carved));
    if (files < 2) {
        fprintf(stderr, "FAIL: expected >=2 files, got %llu\n", (unsigned long long)files);
        return 1;
    }

    cr_ui_telemetry tm;
    memset(&tm, 0, sizeof(tm));
    tm.abi_size = (uint32_t)sizeof(tm);
    CHECK(cr_ui_scan_telemetry(scan, &tm));
    /* quick-only run: the weighted permille caps at quick's 40% weight
     * (40/35/25 stage weights); completion truth is the stage mask */
    if ((tm.stage_done_mask & 1u) == 0) {
        fprintf(stderr, "FAIL: quick stage not completed (mask %u)\n", tm.stage_done_mask);
        return 1;
    }
    if (tm.overall_permille < 400) {
        fprintf(stderr, "FAIL: completed quick permille %u\n", tm.overall_permille);
        return 1;
    }

    /* 2. snapshot + enumeration */
    cr_ui_snapshot* snap = NULL;
    CHECK(cr_ui_snapshot_take(scan, 0, &snap));
    uint32_t count = 0;
    CHECK(cr_ui_snapshot_count(snap, &count));
    if (count < 6) { /* roots + files */
        fprintf(stderr, "FAIL: snapshot too small: %u\n", count);
        return 1;
    }

    uint32_t readme_idx = 0xFFFFFFFFu, inner_idx = 0xFFFFFFFFu, sub_idx = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < count; ++i) {
        cr_ui_node nd;
        memset(&nd, 0, sizeof(nd));
        nd.abi_size = (uint32_t)sizeof(nd);
        char name[128];
        CHECK(cr_ui_snapshot_node(snap, i, &nd, name, sizeof(name)));
        if (strcmp(name, "README.TXT") == 0)
            readme_idx = i;
        else if (strcmp(name, "INNER.TXT") == 0)
            inner_idx = i;
        else if (strcmp(name, "SUBDIR") == 0)
            sub_idx = i;
    }
    if (readme_idx == 0xFFFFFFFFu || inner_idx == 0xFFFFFFFFu || sub_idx == 0xFFFFFFFFu) {
        /* raw subdir sector dump for diagnosis */
        {
            const unsigned DATA = 74u * 512u;
            const unsigned char* sub = g_img + DATA + (5 - 2) * 512;
            fprintf(stderr, "subdir raw: ");
            for (int i = 0; i < 96; ++i)
                fprintf(stderr, "%02x", sub[i]);
            fprintf(stderr, "\n  name-bytes: %.11s | %.11s | %.11s\n", (char*)sub,
                    (char*)(sub + 32), (char*)(sub + 64));
        }
        fprintf(stderr, "FAIL: fixture rows missing (readme/inner/subdir); all names:\n");
        for (uint32_t i = 0; i < count; ++i) {
            cr_ui_node nd;
            char name[128];
            memset(&nd, 0, sizeof(nd));
            nd.abi_size = (uint32_t)sizeof(nd);
            if (cr_ui_snapshot_node(snap, i, &nd, name, sizeof(name)) == 0)
                fprintf(stderr, "  [%u] %s (full=%u)\n", i, name, nd.is_full);
        }
        return 1;
    }

    /* children paging: '.' / '..' anchors are skipped by the walker -
     * SUBDIR carries exactly one child row (INNER.TXT); the paged form
     * also reports the name through cr_ui_snapshot_node */
    uint32_t kids[8] = {0};
    uint32_t wrote = 0;
    CHECK(cr_ui_snapshot_children(snap, sub_idx, 0, 8, kids, &wrote));
    if (wrote != 1) {
        fprintf(stderr, "FAIL: subdir children %u (want 1: INNER only)\n", wrote);
        return 1;
    }
    {
        cr_ui_node nd;
        char cname[128];
        memset(&nd, 0, sizeof(nd));
        nd.abi_size = (uint32_t)sizeof(nd);
        CHECK(cr_ui_snapshot_node(snap, kids[0], &nd, cname, sizeof(cname)));
        if (strcmp(cname, "INNER.TXT") != 0) {
            fprintf(stderr, "FAIL: subdir child is %s\n", cname);
            return 1;
        }
        if (nd.parent != sub_idx) {
            fprintf(stderr, "FAIL: child parent %u != subdir %u\n", nd.parent, sub_idx);
            return 1;
        }
    }
    /* offset paging: page 1 of 1 must be empty */
    uint32_t none = 99;
    CHECK(cr_ui_snapshot_children(snap, sub_idx, 1, 8, kids, &none));
    if (none != 0) {
        fprintf(stderr, "FAIL: paged-past-end returned %u\n", none);
        return 1;
    }

    /* 3. query: substring + count-only */
    cr_ui_query q;
    memset(&q, 0, sizeof(q));
    q.abi_size = (uint32_t)sizeof(q);
    q.name_substring = "inner";
    q.count_only = 1;
    uint64_t qtotal = 0;
    CHECK(cr_ui_snapshot_query(snap, &q, NULL, 0, NULL, &qtotal));
    if (qtotal != 1) {
        fprintf(stderr, "FAIL: 'inner' query total %llu\n", (unsigned long long)qtotal);
        return 1;
    }
    /* paged form */
    q.count_only = 0;
    q.limit = 10;
    cr_ui_row rows[10];
    memset(rows, 0, sizeof(rows));
    uint32_t rows_out = 0;
    CHECK(cr_ui_snapshot_query(snap, &q, rows, 10, &rows_out, &qtotal));
    if (rows_out != 1 || rows[0].size != 512) {
        fprintf(stderr, "FAIL: inner row bytes %u size %llu\n", rows_out,
                (unsigned long long)rows[0].size);
        return 1;
    }

    /* 4. preview: readme content + metadata */
    cr_ui_preview* pv = NULL;
    CHECK(cr_ui_preview_open(snap, readme_idx, &pv));
    unsigned char buf[16] = {0};
    uint32_t got = 0;
    CHECK(cr_ui_preview_read(pv, 0, buf, 10, &got));
    if (got != 10 || memcmp(buf, "READMEDATA", 10) != 0) {
        fprintf(stderr, "FAIL: preview content: %.*s\n", (int)got, (char*)buf);
        return 1;
    }
    char* meta_json = NULL;
    CHECK(cr_ui_preview_metadata(pv, &meta_json));
    if (!strstr(meta_json, "\"kind\"")) {
        fprintf(stderr, "FAIL: metadata json: %s\n", meta_json);
        return 1;
    }
    cr_ui_string_free(meta_json);
    CHECK(cr_ui_preview_free(pv));

    /* 5. recovery through caller IO: README.TXT (1200 bytes: 3 clusters) */
    cr_ui_dest_io io;
    memset(&io, 0, sizeof(io));
    io.abi_size = (uint32_t)sizeof(io);
    io.create_temp = io_create_temp;
    io.write = io_write;
    io.commit = io_commit;
    io.close = io_close;
    io.exists = io_exists;
    io.mkdir = io_mkdir;

    cr_ui_recover* rec = NULL;
    CHECK(cr_ui_recover_new_io(snap, &io, &rec));
    CHECK(cr_ui_recover_add(rec, readme_idx));
    CHECK(cr_ui_recover_add(rec, inner_idx));
    CHECK(cr_ui_recover_run(rec, NULL, NULL));
    char* report = NULL;
    CHECK(cr_ui_recover_report(rec, &report));
    if (!strstr(report, "\"schema_version\"") || !strstr(report, "\"items\"")) {
        fprintf(stderr, "FAIL: report json: %s\n", report);
        return 1;
    }
    cr_ui_string_free(report);
    CHECK(cr_ui_recover_free(rec));

    /* byte-verified content: item ids are 1.. - item 1 = README.TXT
     * (queued first: chain 10->11->12, marker in the first bytes), item
     * 2 = INNER.TXT. Also verify the final-name projection. */
    if (g_files[1].len != 1200) {
        fprintf(stderr, "FAIL: recovered README len %llu (name %s)\n",
                (unsigned long long)g_files[1].len, g_names[1]);
        return 1;
    }
    if (memcmp(g_files[1].data, "READMEDATA", 10) != 0) {
        fprintf(stderr, "FAIL: recovered README content\n");
        return 1;
    }
    if (strstr(g_names[1], "README.TXT") == NULL) {
        fprintf(stderr, "FAIL: README final name %s\n", g_names[1]);
        return 1;
    }
    if (g_files[2].len != 512 || memcmp(g_files[2].data, "INNERDATA", 9) != 0) {
        fprintf(stderr, "FAIL: recovered INNER content (len %llu name %s)\n",
                (unsigned long long)g_files[2].len, g_names[2]);
        return 1;
    }

    /* 6. session save */
    CHECK(cr_ui_scan_save(scan, ses_path));
    CHECK(cr_ui_scan_free(scan));

    /* 7. loaded session -> snapshot -> query (post-scan surface) */
    cr_ui_session* session = NULL;
    CHECK(cr_ui_session_load(ctx, ses_path, &session));
    cr_ui_snapshot* snap2 = NULL;
    CHECK(cr_ui_snapshot_from_session(session, &snap2));
    uint32_t count2 = 0;
    CHECK(cr_ui_snapshot_count(snap2, &count2));
    if (count2 != count) {
        fprintf(stderr, "FAIL: session round-trip count %u != %u\n", count2, count);
        return 1;
    }
    CHECK(cr_ui_snapshot_free(snap2));
    CHECK(cr_ui_session_free(session));
    CHECK(cr_ui_snapshot_free(snap)); /* the scan-side snapshot (preview/
                                         recovery borrowed it) */

    CHECK(cr_ui_ctx_free(ctx));
    remove(img_path);
    remove(ses_path);
    printf("cr_ui_abi_check: OK (full C lifecycle green)\n");
    return 0;
}
