/* cr_ui.cpp - the G-01 C-ABI shim implementation (see cr_ui.h).
 *
 * Wraps the five pure-core surfaces the wizard consumes (scan lifecycle +
 * telemetry, snapshots, query, recovery + report, preview) behind opaque
 * handles and u32 cr::Err codes. No C++ type or exception crosses the
 * boundary (catch-all per entry -> ERR_INTERNAL + thread-local last_error).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/cr_ui.h"

#include "cr/orchestrator/cr_scan.h"
#include "cr/preview/cr_preview.h"
#include "cr/query/cr_query.h"
#include "cr/recovery/cr_report.h"
#include "cr/recovery/cr_safewriter.h"
#include "cr/session/cr_crsf.h"
#include "cr/storage/cr_device.h"
#include "cr/tree/cr_snapshot.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#include "cr/win/cr_destio.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_health.h"
#include "cr/win/cr_transport.h"
#include <windows.h>
#endif

using namespace cr;

namespace {

/* ---- errors ---- */

thread_local std::string g_last_error;

u32 fail(u32 code, const char* msg) {
    g_last_error = msg ? msg : "";
    return code;
}

void clear_error() {
    g_last_error.clear();
}

/* ---- shared error family (0x500A) ---- */
constexpr u32 ERR_UI_BAD_HANDLE = 0x500A0001u;
constexpr u32 ERR_UI_BAD_PARAM = 0x500A0002u;
constexpr u32 ERR_UI_ABI = 0x500A0003u;

/* ---- image-file device (portable scan surface) ---- */

/* 64-bit stdio (MSVC keeps 32-bit long/fseek) */
static long long cr_seek64(std::FILE* f, long long off, int whence) {
#if defined(_WIN32)
    return _fseeki64(f, off, whence);
#else
    return fseeko(f, off, whence);
#endif
}
static long long cr_tell64(std::FILE* f) {
#if defined(_WIN32)
    return _ftelli64(f);
#else
    return ftello(f);
#endif
}

class FileDevice final : public cr::IStorageDevice {
public:
    explicit FileDevice(const std::string& path) : path_(path) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
    }
    u32 open() {
        file_ = std::fopen(path_.c_str(), "rb");
        if (!file_)
            return cr::ERR_OPEN_FAILED;
        if (cr_seek64(file_, 0, SEEK_END) != 0)
            return cr::ERR_READ_FAILED;
        const long long sz = cr_tell64(file_);
        if (sz < 0)
            return cr::ERR_READ_FAILED;
        size_bytes_ = u64(sz);
        geom_.total_bytes = size_bytes_;
        geom_.sector_count = (size_bytes_ + 511) / 512;
        return cr::OK;
    }
    ~FileDevice() override {
        if (file_)
            std::fclose(file_);
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (!file_)
            return cr::ERR_READ_FAILED;
        if (off > size_bytes_ || u64(len) > size_bytes_ - off)
            return cr::ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return cr::OK;
        if (cr_seek64(file_, (long long)(off), SEEK_SET) != 0)
            return cr::ERR_READ_FAILED;
        if (std::fread(buf, 1, len, file_) != len)
            return cr::ERR_READ_FAILED;
        return cr::OK;
    }
    const cr::Geometry& geometry() const override { return geom_; }
    u64 size() const { return size_bytes_; }

private:
    std::string path_;
    std::FILE* file_ = nullptr;
    cr::Geometry geom_;
    u64 size_bytes_ = 0;
};

/* ---- JSON helpers (one escaping story: report::escape_json) ---- */

std::string jstr(const std::string& key, const std::string& value) {
    return "\"" + key + "\":\"" + cr::recovery::report::escape_json(value) + "\"";
}

/* ---- utf16 -> utf8 (name projection) ---- */

std::string to_utf8(const std::u16string& in) {
    std::string out;
    for (char16_t ch : in) {
        if (ch < 0x80)
            out.push_back(char(ch));
        else if (ch < 0x800) {
            out.push_back(char(0xC0 | (ch >> 6)));
            out.push_back(char(0x80 | (ch & 0x3F)));
        } else if (ch >= 0xD800 && ch <= 0xDFFF) {
            out.push_back('?'); /* lone surrogate: lossy, never invalid */
        } else {
            out.push_back(char(0xE0 | (ch >> 12)));
            out.push_back(char(0x80 | ((ch >> 6) & 0x3F)));
            out.push_back(char(0x80 | (ch & 0x3F)));
        }
    }
    return out;
}

#if defined(_WIN32)
std::string to_utf8(const std::wstring& in) {
    std::u16string u16;
    u16.reserve(in.size());
    for (wchar_t ch : in)
        u16.push_back(char16_t(ch)); /* wchar_t == u16 on Windows */
    return to_utf8(u16);
}
#endif

} // namespace

/* ---- handle definitions ---- */

struct cr_ui_ctx {
    int unused = 0; /* versioning/identity may grow here */
};

struct cr_ui_scan {
    std::unique_ptr<cr::tree::ResultTree> tree;
    std::unique_ptr<cr::orchestrator::ScanController> ctrl;
    std::unique_ptr<FileDevice> device;
    std::unique_ptr<cr::PausableToken> token;
    cr::orchestrator::ScanStats stats;
    std::atomic<bool> ran{false};
};

struct cr_ui_session {
    std::unique_ptr<cr::tree::ResultTree> tree;
    cr::orchestrator::ScanStats stats;
};

struct cr_ui_snapshot {
    std::unique_ptr<cr::tree::Snapshot> snap;
    /* preview needs the DEVICE the extents address (the scan's device;
     * sessions have none - preview then reads only resident rows) */
    cr::IStorageDevice* device = nullptr;
};

namespace {

class ShimDestIo final : public cr::recovery::DestIo {
public:
    ShimDestIo(const cr_ui_dest_io* io) : io_(*io) {}

    u32 create_temp(u64 id, const std::u16string& final_path,
                    const std::u16string& suffix) override {
        if (!io_.create_temp)
            return cr::ERR_BAD_PARAMETER;
        return io_.create_temp(io_.user, id, to_utf8(final_path).c_str(), to_utf8(suffix).c_str());
    }
    u32 mkdir(const std::u16string& path) override {
        if (!io_.mkdir)
            return cr::OK; /* no callback: the caller declares no dirs */
        return io_.mkdir(io_.user, to_utf8(path).c_str());
    }
    u32 write(u64 id, u64 at, const void* data, u32 len) override {
        if (!io_.write)
            return cr::ERR_BAD_PARAMETER;
        return io_.write(io_.user, id, at, data, len);
    }
    u32 commit(u64 id) override {
        if (!io_.commit)
            return cr::ERR_BAD_PARAMETER;
        return io_.commit(io_.user, id);
    }
    void close(u64 id) override {
        if (io_.close)
            io_.close(io_.user, id);
    }
    bool exists(const std::u16string& path) override {
        if (!io_.exists)
            return false;
        return io_.exists(io_.user, to_utf8(path).c_str()) != 0;
    }

private:
    const cr_ui_dest_io& io_; /* the caller keeps the struct alive for the
                                 recovery lifetime (documented) */
};

} // namespace

/* sessions carry no device: a truthful null device fails every read
 * (zero-fill + Salvaged on the writer path), while resident rows still
 * recover - never invented bytes. */
struct NullDevice final : cr::IStorageDevice {
    u32 read_at(u64, void*, u32) override { return cr::ERR_READ_FAILED; }
    const cr::Geometry& geometry() const override { return geom_; }
    cr::Geometry geom_{}; /* invalid-by-default (valid() == false) */
};

struct cr_ui_recover {
    cr_ui_snapshot* source = nullptr; /* BORROWED: must outlive the handle */
    std::unique_ptr<ShimDestIo> io_cb;
#if defined(_WIN32)
    std::unique_ptr<cr::win::WinDestIo> io_win;
#endif
    std::unique_ptr<cr::recovery::SafeWriter> writer;
    std::u16string dest_dir; /* per-item enqueue dir (writer concatenates) */
    std::string source_utf8;
    std::string dest_utf8;
    cr::recovery::WriterStats last_stats{};
    u64 elapsed_ms = 0;
};

NullDevice g_null_device;

struct cr_ui_preview {
    std::unique_ptr<cr::preview::PreviewReader> reader;
    cr::tree::FileProperties* props = nullptr; /* preview-owned ref */
    ~cr_ui_preview() {
        if (props)
            props->release();
    }
};

/* ---- version + diagnostics ---- */

extern "C" uint32_t cr_ui_abi_version(void) {
    return CR_UI_ABI_VERSION;
}

extern "C" const char* cr_ui_last_error(void) {
    return g_last_error.c_str();
}

/* ---- ctx ---- */

extern "C" uint32_t cr_ui_ctx_new(cr_ui_ctx** out) {
    clear_error();
    if (!out)
        return fail(ERR_UI_BAD_PARAM, "out is null");
    *out = new (std::nothrow) cr_ui_ctx();
    if (!*out)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    return cr::OK;
}

extern "C" uint32_t cr_ui_ctx_free(cr_ui_ctx* ctx) {
    clear_error();
    delete ctx;
    return cr::OK;
}

/* ---- scan lifecycle ---- */

static u32 scan_start_common(cr_ui_ctx* ctx, const cr_ui_scan_config* config,
                             std::unique_ptr<cr::IStorageDevice> device, cr_ui_scan** out) {
    clear_error();
    if (!ctx || !config || !out || !device)
        return fail(ERR_UI_BAD_PARAM, "null argument");
    if (config->abi_size != sizeof(cr_ui_scan_config))
        return fail(ERR_UI_ABI, "scan_config abi_size mismatch");
    if (!config->run_quick && !config->run_deep && !config->run_carve)
        return fail(ERR_UI_BAD_PARAM, "no stages enabled");
    cr::orchestrator::ScanConfig cfg;
    cfg.run_quick = config->run_quick != 0;
    cfg.run_deep = config->run_deep != 0;
    cfg.run_carve = config->run_carve != 0;
    cfg.whole_disk = config->whole_disk != 0;
    cfg.threads = config->threads;
    for (size_t i = 0; i + 1 < sizeof(cfg.volume_label) && config->volume_label[i]; ++i)
        cfg.volume_label[i] = config->volume_label[i];

    auto* scan = new (std::nothrow) cr_ui_scan();
    if (!scan)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    scan->tree.reset(new (std::nothrow) cr::tree::ResultTree());
    if (!scan->tree) {
        delete scan;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    scan->ctrl.reset(new (std::nothrow) cr::orchestrator::ScanController(*scan->tree, cfg));
    if (!scan->ctrl) {
        delete scan;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    scan->token.reset(new (std::nothrow) cr::PausableToken());
    if (!scan->token) {
        delete scan;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    scan->device.reset(static_cast<FileDevice*>(device.release()));
    *out = scan;
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_start_file(cr_ui_ctx* ctx, const cr_ui_scan_config* config,
                                          const char* utf8_image_path, cr_ui_scan** out) {
    if (!utf8_image_path || !utf8_image_path[0])
        return fail(ERR_UI_BAD_PARAM, "image path is empty");
    auto dev = std::make_unique<FileDevice>(utf8_image_path);
    const u32 rc = dev->open();
    if (rc != cr::OK)
        return fail(rc, "cannot open the image file");
    return scan_start_common(ctx, config, std::move(dev), out);
}

extern "C" uint32_t cr_ui_scan_run(cr_ui_scan* scan, cr_ui_progress_fn progress, void* user) {
    clear_error();
    if (!scan || !scan->ctrl || !scan->device || !scan->token)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    if (scan->ran.exchange(true))
        return fail(ERR_UI_BAD_PARAM, "scan already ran (one run per handle)");

    /* progress trampoline: the orchestrator reports overall permille;
     * stage detail rides the telemetry query instead (the callback shape
     * documents stage==-1 for the overall line) */
    struct Bridge {
        cr_ui_progress_fn fn;
        void* user;
    } bridge{progress, user};
    auto cb = [](u32 permille, void* pu) -> bool {
        auto* b = static_cast<Bridge*>(pu);
        if (b->fn)
            b->fn(b->user, 0xFFFFFFFFu, permille, permille, 0, 0);
        return true; /* cancellation goes through cr_ui_scan_cancel */
    };
    /* explicit function pointer variables: a lambda->nullptr ?: has
     * incompatible operand types under MSVC/strict gcc */
    bool (*prog_cb)(u32, void*) = progress ? +cb : nullptr;
    const u32 rc = scan->ctrl->run(scan->device.get(), scan->token.get(), prog_cb,
                                   progress ? &bridge : nullptr, scan->stats);
    if (rc == cr::ERR_CANCELLED)
        return CR_UI_CANCELLED; /* 0x30000001: partial results stay valid */
    if (rc != cr::OK)
        return fail(rc, "scan failed (cr_ui_last_error has the code; see logs)");
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_cancel(cr_ui_scan* scan) {
    clear_error();
    if (!scan || !scan->token)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    scan->token->cancel();
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_pause(cr_ui_scan* scan) {
    clear_error();
    if (!scan || !scan->token)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    scan->token->pause();
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_resume(cr_ui_scan* scan) {
    clear_error();
    if (!scan || !scan->token)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    scan->token->resume();
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_telemetry(cr_ui_scan* scan, cr_ui_telemetry* out) {
    clear_error();
    if (!scan || !scan->ctrl)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    if (!out || out->abi_size != sizeof(cr_ui_telemetry))
        return fail(ERR_UI_ABI, "telemetry abi_size mismatch");
    const cr::orchestrator::ScanTelemetrySnapshot t = scan->ctrl->telemetry();
    out->stage_done_mask = (t.quick.completed ? 1u : 0u) | (t.deep.completed ? 2u : 0u) |
                           (t.carve.completed ? 4u : 0u);
    out->overall_permille = t.overall_permille;
    out->files_found = t.files_found;
    out->deleted_found = t.deleted_found;
    out->carved_found = t.carved_found;
    out->bytes_scanned = t.bytes_scanned;
    out->elapsed_ms = t.elapsed_ms;
    out->paused = t.paused ? 1u : 0u;
    out->error_ranges = u32(t.errors.ranges().size());
    out->checkpoints = scan->ctrl->checkpoint_count();
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_stats(cr_ui_scan* scan, uint64_t* files_found,
                                     uint64_t* deleted_found, uint64_t* carved_found) {
    clear_error();
    if (!scan)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    if (files_found)
        *files_found = scan->stats.files_found;
    if (deleted_found)
        *deleted_found = scan->stats.deleted_found;
    if (carved_found)
        *carved_found = scan->stats.carved_found;
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_save(cr_ui_scan* scan, const char* utf8_path) {
    clear_error();
    if (!scan || !scan->ctrl)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    if (!utf8_path || !utf8_path[0])
        return fail(ERR_UI_BAD_PARAM, "path is empty");
    std::FILE* f = std::fopen(utf8_path, "wb");
    if (!f)
        return fail(cr::ERR_OPEN_FAILED, "cannot open the session file for writing");
    cr::session::Writer w(
        [](void* u, const void* b, u32 n) -> u32 {
            return std::fwrite(b, 1, n, static_cast<std::FILE*>(u)) == n ? cr::OK
                                                                         : cr::ERR_WRITE_FAILED;
        },
        [](void* u) -> u64 { return u64(cr_tell64(static_cast<std::FILE*>(u))); },
        [](void* u, u64 at) -> u32 {
            return cr_seek64(static_cast<std::FILE*>(u), (long long)(at), SEEK_SET) == 0
                       ? cr::OK
                       : cr::ERR_WRITE_FAILED;
        },
        f);
    u32 rc = w.open(0x52434F56u); /* 'RVOS'-shaped session id (CLI parity) */
    if (rc == cr::OK)
        rc = scan->ctrl->save_session(w, scan->stats);
    const u32 fr = w.finalize();
    std::fclose(f);
    if (rc != cr::OK)
        return fail(rc, "session save failed");
    if (fr != cr::OK)
        return fail(fr, "session finalize failed");
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_free(cr_ui_scan* scan) {
    clear_error();
    delete scan;
    return cr::OK;
}

/* ---- snapshot ---- */

extern "C" uint32_t cr_ui_snapshot_take(cr_ui_scan* scan, uint32_t partial, cr_ui_snapshot** out) {
    clear_error();
    if (!scan || !scan->tree)
        return fail(ERR_UI_BAD_HANDLE, "scan handle is null");
    if (!out)
        return fail(ERR_UI_BAD_PARAM, "out is null");
    auto* s = new (std::nothrow) cr_ui_snapshot();
    if (!s)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    s->snap = scan->tree->capture_snapshot(partial != 0);
    if (!s->snap) {
        delete s;
        return fail(cr::ERR_NO_MEMORY, "capture failed");
    }
    s->device = scan->device.get();
    *out = s;
    return cr::OK;
}

extern "C" uint32_t cr_ui_snapshot_from_session(cr_ui_session* session, cr_ui_snapshot** out) {
    clear_error();
    if (!session || !session->tree)
        return fail(ERR_UI_BAD_HANDLE, "session handle is null");
    if (!out)
        return fail(ERR_UI_BAD_PARAM, "out is null");
    auto* s = new (std::nothrow) cr_ui_snapshot();
    if (!s)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    s->snap = session->tree->capture_snapshot(false);
    if (!s->snap) {
        delete s;
        return fail(cr::ERR_NO_MEMORY, "capture failed");
    }
    *out = s;
    return cr::OK;
}

extern "C" uint32_t cr_ui_snapshot_free(cr_ui_snapshot* snap) {
    clear_error();
    delete snap;
    return cr::OK;
}

extern "C" uint32_t cr_ui_snapshot_count(const cr_ui_snapshot* snap, uint32_t* out) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (!out)
        return fail(ERR_UI_BAD_PARAM, "out is null");
    *out = snap->snap->node_count();
    return cr::OK;
}

extern "C" uint32_t cr_ui_snapshot_node(const cr_ui_snapshot* snap, uint32_t index, cr_ui_node* out,
                                        char* name_buf, uint32_t name_cap) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (!out || out->abi_size != sizeof(cr_ui_node))
        return fail(ERR_UI_ABI, "node abi_size mismatch");
    if (index >= snap->snap->node_count())
        return fail(ERR_UI_BAD_PARAM, "index out of range");
    const cr::tree::SnapNode& n = snap->snap->node(index);
    std::memset(reinterpret_cast<uint8_t*>(out) + sizeof(uint32_t), 0,
                sizeof(cr_ui_node) - sizeof(uint32_t));
    out->index = index;
    out->parent = n.parent;
    out->size = n.logical_size;
    out->modified = n.times.modified;
    out->attributes = n.attributes;
    out->origin = n.origin;
    out->deleted = n.deleted;
    out->confidence = n.confidence;
    out->fs_kind = n.fs_kind;
    out->record_ref = n.record_ref;
    out->is_full = n.full ? 1u : 0u;
    std::string nm = to_utf8(n.name);
    out->category = 0;
    if (!n.full) {
        const size_t dot = nm.find_last_of('.');
        if (dot != std::string::npos && dot + 1 < nm.size())
            out->category = u32(cr::query::category_for_extension(nm.substr(dot + 1)));
    }
    if (name_buf && name_cap) {
        const uint32_t copy =
            uint32_t(nm.size()) < name_cap - 1 ? uint32_t(nm.size()) : name_cap - 1;
        std::memcpy(name_buf, nm.data(), copy);
        name_buf[copy] = 0;
    }
    return cr::OK;
}

extern "C" uint32_t cr_ui_snapshot_children(const cr_ui_snapshot* snap, uint32_t index,
                                            uint32_t offset, uint32_t cap, uint32_t* indices,
                                            uint32_t* total_out) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (index >= snap->snap->node_count())
        return fail(ERR_UI_BAD_PARAM, "index out of range");
    const cr::tree::SnapNode& n = snap->snap->node(index);
    std::vector<uint32_t> kids;
    for (uint32_t c = n.first_child; c != cr::tree::SNAP_NONE; c = snap->snap->node(c).next_sibling)
        kids.push_back(c);
    if (total_out)
        *total_out = uint32_t(kids.size());
    if (!indices || cap == 0)
        return cr::OK; /* size-query form */
    uint32_t wrote = 0;
    for (uint32_t i = offset; i < kids.size() && wrote < cap; ++i)
        indices[wrote++] = kids[i];
    if (total_out)
        *total_out = wrote; /* paged form: rows_out semantics */
    return cr::OK;
}

/* ---- query ---- */

extern "C" uint32_t cr_ui_snapshot_query(const cr_ui_snapshot* snap, const cr_ui_query* q,
                                         cr_ui_row* rows, uint32_t cap, uint32_t* rows_out,
                                         uint64_t* total_out) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (!q || q->abi_size != sizeof(cr_ui_query))
        return fail(ERR_UI_ABI, "query abi_size mismatch");
    if (!total_out)
        return fail(ERR_UI_BAD_PARAM, "total_out is null");
    if (!q->count_only && !rows && cap != 0)
        return fail(ERR_UI_BAD_PARAM, "rows is null but cap != 0");

    /* query::run walks a ResultTree - over a SNAPSHOT we walk the copied
     * rows directly (one pass, same predicates; the snapshot owns every
     * byte so no lock is needed at all). */
    const cr::tree::Snapshot& s = *snap->snap;
    std::vector<uint32_t> matched;
    for (uint32_t i = 0; i < s.node_count(); ++i) {
        const cr::tree::SnapNode& n = s.node(i);
        if (n.full)
            continue; /* roots/structure are never result rows */
        if (q->deleted_only && n.deleted == 0)
            continue;
        if (n.logical_size < q->size_min)
            continue;
        if (q->size_max && n.logical_size > q->size_max)
            continue;
        if (q->modified_from && n.times.modified < q->modified_from)
            continue;
        if (q->modified_to && n.times.modified > q->modified_to)
            continue;
        if (n.confidence < q->confidence_min)
            continue;
        if (q->name_substring && q->name_substring[0]) {
            /* ASCII-case-insensitive substring, query::run semantics */
            const std::string nm = to_utf8(n.name);
            std::string hay, need;
            hay.reserve(nm.size());
            for (char ch : nm)
                hay.push_back(ch >= 'A' && ch <= 'Z' ? char(ch + 32) : ch);
            for (const char* p = q->name_substring; *p; ++p)
                need.push_back(*p >= 'A' && *p <= 'Z' ? char(*p + 32) : *p);
            if (hay.find(need) == std::string::npos)
                continue;
        }
        if (q->category_mask) {
            std::string nm = to_utf8(n.name);
            uint32_t cat = 0;
            const size_t dot = nm.find_last_of('.');
            if (dot != std::string::npos && dot + 1 < nm.size())
                cat = u32(cr::query::category_for_extension(nm.substr(dot + 1)));
            if ((q->category_mask & (1u << cat)) == 0)
                continue;
        }
        matched.push_back(i);
    }
    *total_out = matched.size();
    if (q->count_only)
        return cr::OK;
    uint32_t wrote = 0;
    for (uint64_t k = q->offset; k < matched.size() && (q->limit == 0 || wrote < q->limit); ++k) {
        if (wrote >= cap)
            break;
        const cr::tree::SnapNode& n = s.node(matched[size_t(k)]);
        rows[wrote].abi_size = sizeof(cr_ui_row);
        rows[wrote].node_index = matched[size_t(k)];
        rows[wrote].size = n.logical_size;
        rows[wrote].modified = n.times.modified;
        rows[wrote].origin = n.origin;
        rows[wrote].deleted = n.deleted;
        rows[wrote].confidence = n.confidence;
        std::string nm = to_utf8(n.name);
        rows[wrote].category = 0;
        const size_t dot = nm.find_last_of('.');
        if (dot != std::string::npos && dot + 1 < nm.size())
            rows[wrote].category = u32(cr::query::category_for_extension(nm.substr(dot + 1)));
        ++wrote;
    }
    if (rows_out)
        *rows_out = wrote;
    return cr::OK;
}

/* ---- sessions ---- */

extern "C" uint32_t cr_ui_session_load(cr_ui_ctx* ctx, const char* utf8_path, cr_ui_session** out) {
    clear_error();
    if (!ctx || !utf8_path || !utf8_path[0] || !out)
        return fail(ERR_UI_BAD_PARAM, "bad arguments");
    std::FILE* f = std::fopen(utf8_path, "rb");
    if (!f)
        return fail(cr::ERR_OPEN_FAILED, "cannot open the session file");
    cr::session::Reader r(
        [](void* u, void* b, u32 n) -> u32 {
            return std::fread(b, 1, n, static_cast<std::FILE*>(u)) == n ? cr::OK
                                                                        : cr::ERR_READ_FAILED;
        },
        [](void* u, u64 at) -> u32 {
            return std::fseek(static_cast<std::FILE*>(u), long(at), SEEK_SET) == 0
                       ? cr::OK
                       : cr::ERR_READ_FAILED;
        },
        [](void* u) -> u64 {
            std::FILE* fu = static_cast<std::FILE*>(u);
            const long long cur = cr_tell64(fu);
            cr_seek64(fu, 0, SEEK_END);
            const long long sz = cr_tell64(fu);
            cr_seek64(fu, cur, SEEK_SET);
            return u64(sz);
        },
        f);
    auto* sess = new (std::nothrow) cr_ui_session();
    if (!sess) {
        std::fclose(f);
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    sess->tree.reset(new (std::nothrow) cr::tree::ResultTree());
    if (!sess->tree) {
        std::fclose(f);
        delete sess;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    const u32 rc = cr::orchestrator::ScanController::load_session(r, *sess->tree, sess->stats);
    std::fclose(f);
    if (rc != cr::OK) {
        delete sess;
        return fail(rc, "session load failed");
    }
    *out = sess;
    return cr::OK;
}

extern "C" uint32_t cr_ui_session_free(cr_ui_session* session) {
    clear_error();
    delete session;
    return cr::OK;
}

/* ---- recovery ---- */

extern "C" uint32_t cr_ui_recover_new(cr_ui_snapshot* snap, const char* utf8_dest_dir,
                                      cr_ui_recover** out) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (!utf8_dest_dir || !utf8_dest_dir[0])
        return fail(ERR_UI_BAD_PARAM, "destination directory is empty");
    if (!out)
        return fail(ERR_UI_BAD_PARAM, "out is null");
#if defined(_WIN32)
    /* WinDestIo takes the destination root via the enqueue dest_dir (the
     * writer concatenates); ASCII-safe wide conversion - non-ASCII UTF-8
     * callers use the IO-callback surface */
    std::u16string wide;
    for (const char* p = utf8_dest_dir; *p; ++p)
        wide.push_back(char16_t(uint8_t(*p)));
    if (!wide.empty() && wide.back() != u'\\' && wide.back() != u'/')
        wide.push_back(u'\\');
    auto* rec = new (std::nothrow) cr_ui_recover();
    if (!rec)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    rec->io_win.reset(new (std::nothrow) cr::win::WinDestIo());
    if (!rec->io_win) {
        delete rec;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    rec->writer.reset(new (std::nothrow) cr::recovery::SafeWriter(
        *rec->io_win, snap->device ? snap->device : &g_null_device, cr::recovery::WriterConfig{}));
    if (!rec->writer) {
        delete rec;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    rec->source = snap;
    rec->dest_dir = wide;
    rec->dest_utf8 = utf8_dest_dir;
    *out = rec;
    return cr::OK;
#else
    (void)out;
    return fail(cr::ERR_UNSUPPORTED, "path-based recovery needs the Win32 DestIo - use "
                                     "cr_ui_recover_new_io on this platform");
#endif
}

extern "C" uint32_t cr_ui_recover_new_io(cr_ui_snapshot* snap, const cr_ui_dest_io* io,
                                         cr_ui_recover** out) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (!io || io->abi_size != sizeof(cr_ui_dest_io) || !io->write || !io->commit)
        return fail(ERR_UI_BAD_PARAM, "dest_io callbacks incomplete");
    if (!out)
        return fail(ERR_UI_BAD_PARAM, "out is null");
    auto* rec = new (std::nothrow) cr_ui_recover();
    if (!rec)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    rec->io_cb.reset(new (std::nothrow) ShimDestIo(io));
    if (!rec->io_cb) {
        delete rec;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    rec->writer.reset(new (std::nothrow) cr::recovery::SafeWriter(
        *rec->io_cb, snap->device ? snap->device : &g_null_device, cr::recovery::WriterConfig{}));
    if (!rec->writer) {
        delete rec;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    rec->source = snap;
    rec->dest_utf8 = "io-callbacks";
    *out = rec;
    return cr::OK;
}

extern "C" uint32_t cr_ui_recover_add(cr_ui_recover* rec, uint32_t node_index) {
    clear_error();
    if (!rec || !rec->writer || !rec->source || !rec->source->snap)
        return fail(ERR_UI_BAD_HANDLE, "recover handle is null");
    if (node_index >= rec->source->snap->node_count())
        return fail(ERR_UI_BAD_PARAM, "index out of range");
    return rec->writer->enqueue(*rec->source->snap, node_index, rec->dest_dir);
}

extern "C" uint32_t cr_ui_recover_run(cr_ui_recover* rec, cr_ui_recover_progress_fn progress,
                                      void* user) {
    clear_error();
    if (!rec || !rec->writer)
        return fail(ERR_UI_BAD_HANDLE, "recover handle is null");
    struct Bridge {
        cr_ui_recover_progress_fn fn;
        void* user;
    } bridge{progress, user};
    auto cb = [](u64 done, u64 total, void* pu) -> bool {
        auto* b = static_cast<Bridge*>(pu);
        if (b->fn)
            b->fn(b->user, done, total);
        return true;
    };
    const auto t0 = std::chrono::steady_clock::now();
    bool (*prog_cb)(u64, u64, void*) = progress ? +cb : nullptr;
    cr::recovery::WriterStats stats;
    const u32 rc = rec->writer->process(nullptr, prog_cb, progress ? &bridge : nullptr, stats);
    rec->elapsed_ms = u64(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0)
            .count());
    rec->last_stats = stats; /* surfaced through cr_ui_recover_report */
    return rc;
}

extern "C" uint32_t cr_ui_recover_report(cr_ui_recover* rec, char** out_json) {
    clear_error();
    if (!rec || !rec->writer)
        return fail(ERR_UI_BAD_HANDLE, "recover handle is null");
    if (!out_json)
        return fail(ERR_UI_BAD_PARAM, "out_json is null");
    const std::string doc = cr::recovery::report::build_report(
        rec->writer->report_items(), rec->writer->report_totals(rec->source_utf8, rec->dest_utf8,
                                                                rec->elapsed_ms, rec->last_stats));
    char* copy = new (std::nothrow) char[doc.size() + 1];
    if (!copy)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    std::memcpy(copy, doc.data(), doc.size() + 1);
    *out_json = copy;
    return cr::OK;
}

extern "C" uint32_t cr_ui_recover_free(cr_ui_recover* rec) {
    clear_error();
    delete rec;
    return cr::OK;
}

extern "C" void cr_ui_string_free(char* s) {
    delete[] s;
}

/* ---- preview ---- */

extern "C" uint32_t cr_ui_preview_open(cr_ui_snapshot* snap, uint32_t node_index,
                                       cr_ui_preview** out) {
    clear_error();
    if (!snap || !snap->snap)
        return fail(ERR_UI_BAD_HANDLE, "snapshot handle is null");
    if (node_index >= snap->snap->node_count())
        return fail(ERR_UI_BAD_PARAM, "index out of range");
    const cr::tree::SnapNode& n = snap->snap->node(node_index);
    if (n.full)
        return fail(ERR_UI_BAD_PARAM, "structure rows have no content");
    auto* pv = new (std::nothrow) cr_ui_preview();
    if (!pv)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    pv->props = snap->snap->materialize_props(node_index);
    if (!pv->props) {
        delete pv;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    pv->reader.reset(new (std::nothrow) cr::preview::PreviewReader(
        snap->device ? snap->device : &g_null_device, pv->props));
    if (!pv->reader) {
        delete pv;
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    }
    *out = pv;
    return cr::OK;
}

extern "C" uint32_t cr_ui_preview_read(cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t cap,
                                       uint32_t* got) {
    clear_error();
    if (!pv || !pv->reader)
        return fail(ERR_UI_BAD_HANDLE, "preview handle is null");
    if (!buf || !got)
        return fail(ERR_UI_BAD_PARAM, "buf/got is null");
    *got = pv->reader->read_at(offset, buf, cap);
    return cr::OK;
}

extern "C" uint32_t cr_ui_preview_metadata(cr_ui_preview* pv, char** out_json) {
    clear_error();
    if (!pv || !pv->reader)
        return fail(ERR_UI_BAD_HANDLE, "preview handle is null");
    if (!out_json)
        return fail(ERR_UI_BAD_PARAM, "out_json is null");
    const cr::preview::Metadata m = cr::preview::extract_metadata(*pv->reader);
    std::string doc;
    doc.reserve(256);
    doc += "{";
    doc += jstr("kind", m.kind);
    doc += ",";
    doc += jstr("format", m.format);
    doc += ",\"width\":";
    doc += std::to_string(m.width);
    doc += ",\"height\":";
    doc += std::to_string(m.height);
    doc += ",";
    doc += jstr("orientation", m.orientation);
    doc += ",";
    doc += jstr("datetime", m.datetime);
    doc += ",\"duration_ms\":";
    doc += std::to_string(m.duration_ms);
    doc += ",";
    doc += jstr("codec", m.codec);
    doc += ",";
    doc += jstr("text_snippet", m.text_snippet);
    doc += ",\"truncated\":";
    doc += m.truncated ? "true" : "false";
    doc += "}";
    char* copy = new (std::nothrow) char[doc.size() + 1];
    if (!copy)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    std::memcpy(copy, doc.data(), doc.size() + 1);
    *out_json = copy;
    return cr::OK;
}

extern "C" uint32_t cr_ui_preview_free(cr_ui_preview* pv) {
    clear_error();
    delete pv;
    return cr::OK;
}

#if defined(_WIN32)
/* ---- Windows-only surfaces ---- */

extern "C" uint32_t cr_ui_list_disks(char** out_json) {
    clear_error();
    if (!out_json)
        return fail(ERR_UI_BAD_PARAM, "out_json is null");
    cr::win::PhysicalDiskList disks;
    const u32 rc = cr::win::enumerate_disks(64, disks);
    if (rc != cr::OK)
        return fail(rc, "enumeration failed");
    std::string doc = "{\"disks\":[";
    bool first = true;
    for (const auto& d : disks) {
        if (!first)
            doc.push_back(',');
        first = false;
        doc += "{\"number\":";
        doc += std::to_string(d.number);
        doc += ",";
        doc += jstr("model", to_utf8(d.model));
        doc += ",";
        doc += jstr("serial", to_utf8(d.serial));
        doc += ",\"size_bytes\":";
        doc += std::to_string(d.size_bytes);
        doc += ",\"logical_sector\":";
        doc += std::to_string(d.logical_sector);
        doc += ",\"physical_sector\":";
        doc += std::to_string(d.physical_sector);
        doc += ",\"removable\":";
        doc += d.removable ? "true" : "false";
        doc += "}";
    }
    doc += "]}";
    char* copy = new (std::nothrow) char[doc.size() + 1];
    if (!copy)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    std::memcpy(copy, doc.data(), doc.size() + 1);
    *out_json = copy;
    return cr::OK;
}

extern "C" uint32_t cr_ui_health_disk(uint32_t disk, char** out_json) {
    clear_error();
    if (!out_json)
        return fail(ERR_UI_BAD_PARAM, "out_json is null");
    wchar_t path[64];
    swprintf_s(path, 64, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return fail(cr::ERR_OPEN_FAILED, "cannot open the disk (elevate?)");
    cr::win::health::SmartReport sm;
    const u32 rc = cr::win::health::read_smart(h, sm);
    CloseHandle(h);
    if (rc != cr::OK)
        return fail(rc, "SMART read failed");
    std::string doc = "{";
    doc += jstr("transport", sm.transport);
    doc += ",\"available\":";
    doc += sm.available ? "true" : "false";
    doc += ",\"failing_now\":";
    doc += sm.failing_now ? "true" : "false";
    doc += ",\"reallocated_sectors\":";
    doc += std::to_string(sm.reallocated_sectors);
    doc += ",\"pending_sectors\":";
    doc += std::to_string(sm.pending_sectors);
    doc += ",\"uncorrectable\":";
    doc += std::to_string(sm.uncorrectable);
    doc += ",\"power_on_hours\":";
    doc += std::to_string(sm.power_on_hours);
    doc += ",\"temperature_c\":";
    doc += std::to_string(sm.temperature_c);
    doc += ",\"attributes\":[";
    bool first = true;
    for (const auto& a : sm.attributes) {
        if (!first)
            doc.push_back(',');
        first = false;
        doc += "{\"id\":";
        doc += std::to_string(a.id);
        doc += ",";
        doc += jstr("name", a.name);
        doc += ",\"value\":";
        doc += std::to_string(a.value);
        doc += ",\"worst\":";
        doc += std::to_string(a.worst);
        doc += ",\"failing\":";
        doc += a.failing ? "true" : "false";
        doc += "}";
    }
    doc += "]}";
    char* copy = new (std::nothrow) char[doc.size() + 1];
    if (!copy)
        return fail(cr::ERR_NO_MEMORY, "out of memory");
    std::memcpy(copy, doc.data(), doc.size() + 1);
    *out_json = copy;
    return cr::OK;
}

extern "C" uint32_t cr_ui_scan_start_disk(cr_ui_ctx* ctx, const cr_ui_scan_config* config,
                                          uint32_t disk, cr_ui_scan** out) {
    clear_error();
    if (!ctx || !config || !out)
        return fail(ERR_UI_BAD_PARAM, "bad arguments");
    u32 err = 0;
    std::unique_ptr<cr::IStorageDevice> dev;
    if (cr::win::rda_available(L"RDA", nullptr))
        dev = cr::win::open_rda_partition(L"RDA", disk, 0, &err);
    if (!dev)
        dev = cr::win::open_physical_drive(disk, &err);
    if (!dev)
        return fail(err ? err : cr::ERR_OPEN_FAILED, "cannot open the disk");
    return scan_start_common(ctx, config, std::move(dev), out);
}
#endif
