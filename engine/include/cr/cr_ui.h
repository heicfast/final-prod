/*
 * cr_ui.h - stable C ABI shim over the core (G-01, issue #1).
 *
 * The wizard (and any external host) integrates through this header ONLY:
 * opaque handles, `cr_ui_*` functions returning u32 `cr::Err` codes (the
 * SAME numeric space as cr_error.h - 0 == OK), no C++ types crossing the
 * boundary, no exceptions escaping (catch-all per entry -> ERR_INTERNAL +
 * cr_ui_last_error()), UTF-8 `const char*` in, caller buffers out (or
 * cr_ui_string_free for heap strings - ownership documented per function).
 *
 * ABI policy: cr_ui_abi_version() returns CR_UI_ABI_VERSION; breaking
 * changes (signature, field, semantics) bump it; additions are appended
 * without a bump. A consumer compiled against an older header fails the
 * version check explicitly.
 *
 * Threading: any handle's methods may be called from any thread unless
 * stated otherwise; the progress/report callbacks fire on the worker
 * thread that produced the event and must not block. Strings/structs a
 * callback hands out are valid only inside the callback.
 *
 * Platform notes: the disk/volume enumeration + health surfaces are
 * Windows-only (the win layer); the rest is pure core and works on both
 * supported platforms (the CI builds the .so on Linux and exercises the
 * full lifecycle with a plain C consumer).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#ifndef CR_UI_H_
#define CR_UI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
#define CR_UI_API __declspec(dllexport)
#else
#define CR_UI_API __attribute__((visibility("default")))
#endif

#define CR_UI_ABI_VERSION 1

/* cr::Err space (cr_error.h): 0 == OK; families 0x5002..0x5009.
 * The shim adds family 0x500A: bad handle / busy / ABI mismatch. */

/* ---- version + diagnostics ---- */

CR_UI_API uint32_t cr_ui_abi_version(void);
/* Thread-local diagnostic for the last failed call on THIS thread.
 * Valid until the next cr_ui_* call from the same thread. */
CR_UI_API const char* cr_ui_last_error(void);

/* ---- handles ---- */

typedef struct cr_ui_ctx cr_ui_ctx;         /* context: logging + identity */
typedef struct cr_ui_scan cr_ui_scan;       /* one scan run (device + tree) */
typedef struct cr_ui_session cr_ui_session; /* loaded session (post-scan) */
typedef struct cr_ui_snapshot cr_ui_snapshot;
typedef struct cr_ui_recover cr_ui_recover;
typedef struct cr_ui_preview cr_ui_preview;

CR_UI_API uint32_t cr_ui_ctx_new(cr_ui_ctx** out);
CR_UI_API uint32_t cr_ui_ctx_free(cr_ui_ctx* ctx);

/* ---- scan lifecycle ----
 * The scan runs on a caller-supplied thread: cr_ui_scan_run blocks until
 * completion/cancel; cr_ui_scan_start_async spawns a worker. Exactly one
 * run per handle. */

struct cr_ui_scan_config {
    uint32_t abi_size; /* = sizeof(cr_ui_scan_config); versioned */
    uint8_t run_quick; /* stages; at least one must be set */
    uint8_t run_deep;
    uint8_t run_carve;
    uint8_t whole_disk;    /* lost-partition discovery */
    uint32_t threads;      /* 0 = auto */
    char volume_label[32]; /* UTF-8, NUL-padded (session VolumeInfo) */
};

/* Scan a raw image FILE (portable surface: the file is opened read-only
 * and serves the whole-disk view; whole_disk=0 scans the first
 * partition/volume). */
CR_UI_API uint32_t cr_ui_scan_start_file(cr_ui_ctx* ctx, const struct cr_ui_scan_config* config,
                                         const char* utf8_image_path, cr_ui_scan** out);

/* Progress callback: called from the scan thread at engine loop heads.
 * stage: 0=quick 1=deep 2=carve; *_permille: 0..1000 (monotonic).
 * bytes_total==0 means unknown. Return 0 to CONTINUE (cancellation is a
 * separate cr_ui_scan_cancel call; the callback must not block). */
typedef void (*cr_ui_progress_fn)(void* user, uint32_t stage, uint32_t stage_permille,
                                  uint32_t overall_permille, uint64_t bytes_done,
                                  uint64_t bytes_total);

CR_UI_API uint32_t cr_ui_scan_run(cr_ui_scan* scan, cr_ui_progress_fn progress, void* user);
/* Result of cr_ui_scan_run: 0 (completed), an error code, or 0x30000001
 * (cancelled - partial results + session save stay valid). */
#define CR_UI_CANCELLED 0x30000001u

CR_UI_API uint32_t cr_ui_scan_cancel(cr_ui_scan* scan); /* idempotent */
CR_UI_API uint32_t cr_ui_scan_pause(cr_ui_scan* scan);  /* idempotent */
CR_UI_API uint32_t cr_ui_scan_resume(cr_ui_scan* scan); /* idempotent */

struct cr_ui_telemetry {
    uint32_t abi_size;
    uint32_t stage_done_mask;  /* bit0 quick, bit1 deep, bit2 carve */
    uint32_t overall_permille; /* 0..1000 */
    uint64_t files_found;
    uint64_t deleted_found;
    uint64_t carved_found;
    uint64_t bytes_scanned;
    uint64_t elapsed_ms;
    uint32_t paused;       /* 1 while paused */
    uint32_t error_ranges; /* merged bad-sector ranges */
    uint64_t checkpoints;
};
CR_UI_API uint32_t cr_ui_scan_telemetry(cr_ui_scan* scan, struct cr_ui_telemetry* out);

CR_UI_API uint32_t cr_ui_scan_stats(cr_ui_scan* scan, uint64_t* files_found,
                                    uint64_t* deleted_found, uint64_t* carved_found);

/* Save the session (the same record set the CLI --out writes). */
CR_UI_API uint32_t cr_ui_scan_save(cr_ui_scan* scan, const char* utf8_path);

CR_UI_API uint32_t cr_ui_scan_free(cr_ui_scan* scan);

/* ---- snapshot (G-02) ----
 * capture=0 -> stage-boundary state (partial flag cleared); capture!=0
 * marks SNAP_PARTIAL (mid-stage truth). */
CR_UI_API uint32_t cr_ui_snapshot_take(cr_ui_scan* scan, uint32_t partial, cr_ui_snapshot** out);
/* From a loaded session (post-scan query surface). */
CR_UI_API uint32_t cr_ui_snapshot_from_session(cr_ui_session* session, cr_ui_snapshot** out);
CR_UI_API uint32_t cr_ui_snapshot_free(cr_ui_snapshot* snap);

struct cr_ui_node {
    uint32_t abi_size;
    uint32_t index; /* stable while the snapshot lives */
    uint32_t parent;
    uint64_t size;       /* logical bytes */
    uint64_t modified;   /* 100-ns since 1601 */
    uint32_t attributes; /* DOS attributes (0x10 = dir) */
    uint32_t origin;     /* tree::Origin 0..3 */
    uint32_t deleted;    /* tree::DeletedState 0..3 */
    uint32_t confidence; /* tree::Confidence 0..4 */
    uint32_t fs_kind;
    uint64_t record_ref;
    uint32_t category; /* query::Category 0..6 */
    uint32_t is_full;  /* root/structure rows */
};
CR_UI_API uint32_t cr_ui_snapshot_count(const cr_ui_snapshot* snap, uint32_t* out);
CR_UI_API uint32_t cr_ui_snapshot_node(const cr_ui_snapshot* snap, uint32_t index,
                                       struct cr_ui_node* out, char* name_buf, uint32_t name_cap);
/* Children of one row, paged: returns up to cap indices; total_out gets
 * the full child count. */
CR_UI_API uint32_t cr_ui_snapshot_children(const cr_ui_snapshot* snap, uint32_t index,
                                           uint32_t offset, uint32_t cap, uint32_t* indices,
                                           uint32_t* total_out);

/* ---- query (G-12) ---- */

struct cr_ui_query {
    uint32_t abi_size;
    uint32_t category_mask; /* bits 1..6; 0 = all */
    uint64_t size_min;
    uint64_t size_max;       /* 0 = no upper bound */
    uint64_t modified_from;  /* 100-ns since 1601; 0 = no floor */
    uint64_t modified_to;    /* 0 = no ceiling */
    uint32_t confidence_min; /* 0..4 */
    uint8_t deleted_only;
    const char* name_substring; /* UTF-8, case-insensitive; NULL = all */
    uint32_t sort;              /* 0 none, 1 name, 2 size, 3 modified,
                                 * 4 confidence, 5 path */
    uint8_t sort_desc;
    uint64_t offset;
    uint32_t limit; /* 0 = all */
    uint8_t count_only;
};
struct cr_ui_row {
    uint32_t abi_size;
    uint32_t node_index;
    uint64_t size;
    uint64_t modified;
    uint32_t origin;
    uint32_t deleted;
    uint32_t confidence;
    uint32_t category;
};
CR_UI_API uint32_t cr_ui_snapshot_query(const cr_ui_snapshot* snap, const struct cr_ui_query* q,
                                        struct cr_ui_row* rows, uint32_t cap, uint32_t* rows_out,
                                        uint64_t* total_out);

/* ---- sessions (loaded) ---- */

CR_UI_API uint32_t cr_ui_session_load(cr_ui_ctx* ctx, const char* utf8_path, cr_ui_session** out);
CR_UI_API uint32_t cr_ui_session_free(cr_ui_session* session);

/* ---- recovery (G-02b value path + G-09 report) ---- */

/* Destination IO: either a real directory path (Windows: the Win32
 * DestIo; documented), or explicit callbacks (portable surface - the C
 * consumer test uses these). Exactly one of the two. */
struct cr_ui_dest_io {
    uint32_t abi_size;
    void* user;
    uint32_t (*create_temp)(void* user, uint64_t item_id, const char* utf8_final,
                            const char* utf8_suffix);
    uint32_t (*write)(void* user, uint64_t item_id, uint64_t at, const void* data, uint32_t len);
    uint32_t (*commit)(void* user, uint64_t item_id);
    void (*close)(void* user, uint64_t item_id);
    uint8_t (*exists)(void* user, const char* utf8_final);
    uint32_t (*mkdir)(void* user, const char* utf8_path);
};

CR_UI_API uint32_t cr_ui_recover_new(cr_ui_snapshot* snap, const char* utf8_dest_dir,
                                     cr_ui_recover** out);
CR_UI_API uint32_t cr_ui_recover_new_io(cr_ui_snapshot* snap, const struct cr_ui_dest_io* io,
                                        cr_ui_recover** out);
/* Queue one snapshot row (value-enqueue: the live tree is never touched). */
CR_UI_API uint32_t cr_ui_recover_add(cr_ui_recover* rec, uint32_t node_index);
typedef void (*cr_ui_recover_progress_fn)(void* user, uint64_t done, uint64_t total);
CR_UI_API uint32_t cr_ui_recover_run(cr_ui_recover* rec, cr_ui_recover_progress_fn progress,
                                     void* user);
/* Machine-readable result report (recovery-report-schema.md v1); caller
 * frees with cr_ui_string_free. */
CR_UI_API uint32_t cr_ui_recover_report(cr_ui_recover* rec, char** out_json);
CR_UI_API uint32_t cr_ui_recover_free(cr_ui_recover* rec);

CR_UI_API void cr_ui_string_free(char* s);

/* ---- preview (G-10) ---- */

CR_UI_API uint32_t cr_ui_preview_open(cr_ui_snapshot* snap, uint32_t node_index,
                                      cr_ui_preview** out);
/* Read up to cap bytes at the node's logical offset (EOF-clamped,
 * zero-filled unreadable spans). Returns the byte count in *got. */
CR_UI_API uint32_t cr_ui_preview_read(cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t cap,
                                      uint32_t* got);
/* Extracted metadata as JSON (bounded extractors; the caps default);
 * caller frees with cr_ui_string_free. */
CR_UI_API uint32_t cr_ui_preview_metadata(cr_ui_preview* pv, char** out_json);
CR_UI_API uint32_t cr_ui_preview_free(cr_ui_preview* pv);

/* ---- Windows-only surfaces (the win layer) ---- */
#if defined(_WIN32)
/* Physical disks as JSON (the CLI `list` projection). */
CR_UI_API uint32_t cr_ui_list_disks(char** out_json);
/* SMART/health as JSON (the CLI `health` projection). */
CR_UI_API uint32_t cr_ui_health_disk(uint32_t disk, char** out_json);
/* Scan a real physical disk (the rrecov `scan --disk N` path: RDA
 * preferred, direct fallback). */
CR_UI_API uint32_t cr_ui_scan_start_disk(cr_ui_ctx* ctx, const struct cr_ui_scan_config* config,
                                         uint32_t disk, cr_ui_scan** out);
#endif

/* C convenience typedefs (the structs above are usable either way). */
typedef struct cr_ui_scan_config cr_ui_scan_config;
typedef struct cr_ui_telemetry cr_ui_telemetry;
typedef struct cr_ui_node cr_ui_node;
typedef struct cr_ui_query cr_ui_query;
typedef struct cr_ui_row cr_ui_row;
typedef struct cr_ui_dest_io cr_ui_dest_io;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_UI_H_ */
