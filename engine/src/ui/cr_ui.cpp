/*
 * cr_ui.cpp - the cr_ui C-ABI shim implementation (GAP G-01, issue #1).
 *
 * Flat-C boundary over cr_core (+ cr_win on Windows) mirroring the
 * reference's in-process engine binding: DRWUI LoadLibrary'd each
 * *SearchDLL.dll and GetProcAddress'd "EUCreate<FS>Searcher" factories
 * that returned controller interfaces; here the cr_ui_* factories return
 * opaque handles over ScanController / sessions / snapshots / the writer,
 * with C callbacks for the engine event hooks. Every flow is 1:1 with a
 * CLI line (the parity mandate): scan, tree, recover, health.
 *
 * Layer rules: this is a BINDING layer - no engine logic lives here. It
 * adapts UTF-8 C callers onto the C++ core, injects platform IO (file
 * adapters for sessions/checkpoints, DestIo backends, devices), and keeps
 * the CLI's exact semantics for every flow it wraps (identity card, LDM
 * cross-disk resolver, checkpoint sink, save-on-cancel policy, the
 * BitLocker gate, the [F12] destination-overlap arming).
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
#include "cr/session/cr_records.h"
#include "cr/table/cr_table_wire.h"
#include "cr/tree/cr_snapshot.h"

#ifdef _WIN32
#include "cr/win/cr_bitlocker.h"
#include "cr/win/cr_destio.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_health.h"
#include "cr/win/cr_transport.h"
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace cr;

/* =========================================================================
 * diagnostics (thread-local last_error; every entry returns through ret_)
 * ========================================================================= */

namespace {

thread_local std::string g_last_error = "no error";

u32 ret_ok() {
    g_last_error = "no error";
    return OK;
}

u32 ret_(u32 code, const char* fmt, ...) {
    if (code == OK) {
        g_last_error = "no error";
        return code;
    }
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    g_last_error = buf;
    return code;
}

template <typename F, typename... Args> u32 guarded(F fn, Args... args) {
    try {
        return fn(args...);
    } catch (const std::exception& e) {
        return ret_(ERR_INTERNAL, "exception: %s", e.what());
    } catch (...) {
        return ret_(ERR_INTERNAL, "unknown exception");
    }
}

/* =========================================================================
 * UTF-8 <-> UTF-16 (lossless for valid input; invalid -> U+FFFD, the
 * honest replacement - names never crash the ABI and never leak invalid
 * UTF-8 across it)
 * ========================================================================= */

void u16_append_cp(std::u16string& out, u32 cp) {
    if (cp > 0x10FFFF)
        cp = 0xFFFD;
    if (cp >= 0x10000) {
        cp -= 0x10000;
        out.push_back(u16(0xD800 + (cp >> 10)));
        out.push_back(u16(0xDC00 + (cp & 0x3FF)));
    } else {
        out.push_back(u16(cp));
    }
}

std::u16string u8_to_u16(const char* s) {
    std::u16string out;
    if (!s)
        return out;
    const u8* p = reinterpret_cast<const u8*>(s);
    while (*p) {
        u32 cp = 0xFFFD, extra = 0;
        if (*p < 0x80) {
            cp = *p;
        } else if ((*p & 0xE0) == 0xC0) {
            cp = *p & 0x1Fu;
            extra = 1;
        } else if ((*p & 0xF0) == 0xE0) {
            cp = *p & 0x0Fu;
            extra = 2;
        } else if ((*p & 0xF8) == 0xF0) {
            cp = *p & 0x07u;
            extra = 3;
        }
        bool ok = true; /* single-byte ASCII (extra == 0) is valid by construction */
        for (u32 i = 1; ok && i <= extra; ++i) {
            if ((p[i] & 0xC0) != 0x80)
                ok = false;
            else
                cp = (cp << 6) | (p[i] & 0x3Fu);
        }
        if (ok && ((extra == 1 && cp < 0x80) || (extra == 2 && cp < 0x800) ||
                   (extra == 3 && cp < 0x10000) || (cp >= 0xD800 && cp <= 0xDFFF)))
            ok = false;
        if (!ok) {
            cp = 0xFFFD;
            extra = 0;
        }
        u16_append_cp(out, cp);
        p += 1 + extra;
    }
    return out;
}

std::string u16_to_u8(const std::u16string& w) {
    std::string out;
    out.reserve(w.size());
    for (size_t i = 0; i < w.size(); ++i) {
        u32 cp = w[i];
        if (cp >= 0xD800 && cp <= 0xDBFF) {
            if (i + 1 < w.size() && w[i + 1] >= 0xDC00 && w[i + 1] <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (w[i + 1] - 0xDC00);
                ++i;
            } else {
                cp = 0xFFFD;
            }
        } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
            cp = 0xFFFD;
        }
        if (cp < 0x80) {
            out.push_back(char(cp));
        } else if (cp < 0x800) {
            out.push_back(char(0xC0 | (cp >> 6)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(char(0xE0 | (cp >> 12)));
            out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(char(0xF0 | (cp >> 18)));
            out.push_back(char(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(char(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(char(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

#ifdef _WIN32
static_assert(sizeof(wchar_t) == 2, "Windows wchar_t must be UTF-16");
std::string ws_to_u8(const std::wstring& w) {
    return u16_to_u8(std::u16string(w.begin(), w.end()));
}
std::wstring u8_to_ws(const char* s) {
    const std::u16string u = u8_to_u16(s);
    return std::wstring(u.begin(), u.end());
}
#endif

/* bounded copy: src -> dst[cap], always NUL-terminated; true when it fit */
bool copy_bstr(char* dst, size_t cap, const std::string& src) {
    if (cap == 0)
        return false;
    const size_t n = src.size() < cap - 1 ? src.size() : cap - 1;
    std::memcpy(dst, src.data(), n);
    dst[n] = 0;
    return n == src.size();
}

/* bounded copy from a fixed C-string field (label/model/serial cards) */
bool copy_cstr(char* dst, size_t cap, const char* src) {
    if (cap == 0)
        return false;
    size_t n = 0;
    while (n < cap - 1 && src[n])
        ++n;
    std::memcpy(dst, src, n);
    dst[n] = 0;
    return src[n] == 0;
}

char* dup_str(const std::string& s) {
    char* p = static_cast<char*>(std::malloc(s.size() + 1));
    if (!p)
        return nullptr;
    std::memcpy(p, s.data(), s.size());
    p[s.size()] = 0;
    return p;
}

/* copy into a fixed char array from a config string (NULL/"" -> empty) */
void copy_card(char* dst, size_t cap, const char* src) {
    if (cap == 0)
        return;
    if (!src || !*src) {
        dst[0] = 0;
        return;
    }
    const size_t n = std::strlen(src);
    const size_t k = n < cap - 1 ? n : cap - 1;
    std::memcpy(dst, src, k);
    dst[k] = 0;
}

/* =========================================================================
 * FILE adapters (sessions + checkpoints; UTF-8 paths in, 64-bit clean)
 * ========================================================================= */

#ifdef _WIN32
FILE* open_file(const char* u8path, bool write) {
    const std::wstring w = u8_to_ws(u8path);
    FILE* f = nullptr;
    if (_wfopen_s(&f, w.c_str(), write ? L"wb" : L"rb") != 0 || !f)
        return nullptr;
    return f;
}
u64 ftell64(FILE* f) {
    return u64(_ftelli64(f));
}
int fseek64(FILE* f, u64 pos) {
    return _fseeki64(f, __int64(pos), SEEK_SET);
}
#else
FILE* open_file(const char* u8path, bool write) {
    return std::fopen(u8path, write ? "wb" : "rb");
}
u64 ftell64(FILE* f) {
    return u64(ftello(f));
}
int fseek64(FILE* f, u64 pos) {
    return fseeko(f, off_t(pos), SEEK_SET);
}
#endif

u32 f_write_cb(void* user, const void* bytes, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return std::fwrite(bytes, 1, len, f) == len ? OK : ERR_SESSION_IO;
}
u64 f_tell_cb(void* user) {
    return ftell64(static_cast<FILE*>(user));
}
u32 f_seek_cb(void* user, u64 absolute) {
    return fseek64(static_cast<FILE*>(user), absolute) == 0 ? OK : ERR_SESSION_IO;
}
u32 f_read_cb(void* user, void* buf, u32 len) {
    FILE* f = static_cast<FILE*>(user);
    return std::fread(buf, 1, len, f) == len ? OK : ERR_SESSION_IO;
}

/* the CLI's file_size_cb, verbatim semantics */
u64 file_size_of(FILE* f) {
    const u64 cur = ftell64(f);
    std::fseek(f, 0, SEEK_END);
    const u64 end = ftell64(f);
    fseek64(f, cur);
    return end;
}
u64 f_size_cb(void* user) {
    return file_size_of(static_cast<FILE*>(user));
}

/* =========================================================================
 * devices
 * ========================================================================= */

struct CrDevice {
    std::unique_ptr<IStorageDevice> dev;  /* the view handed to the core */
    std::unique_ptr<IStorageDevice> hold; /* parent disk (slices borrow it) */
    u32 disk = 0;
    bool disk_known = false;
    u64 volume_len = 0; /* nonzero = a volume slice (not whole-disk) */
    const char* transport = "image";
};

/* read-only, sector-granular file-backed device (the portable source for
 * tests, the ABI check, and image scanning; sub-sector tail bytes are not
 * addressable - a 512-byte-sector device by construction) */
class ImageFileDevice final : public IStorageDevice {
public:
    static std::unique_ptr<ImageFileDevice> open(const char* u8path, u64 offset, u64 bytes,
                                                 u32* err) {
        *err = OK;
        FILE* f = open_file(u8path, false);
        if (!f) {
            *err = ERR_OPEN_FAILED;
            return nullptr;
        }
        const u64 fsize = file_size_of(f);
        if (bytes == 0)
            bytes = fsize > offset ? fsize - offset : 0;
        if (offset > fsize || bytes == 0 || offset + bytes > fsize) {
            std::fclose(f);
            *err = ERR_BAD_PARAMETER;
            return nullptr;
        }
        const u64 total = bytes & ~u64(511); /* sector-granular bound */
        if (total == 0) {
            std::fclose(f);
            *err = ERR_BAD_PARAMETER;
            return nullptr;
        }
        return std::unique_ptr<ImageFileDevice>(new ImageFileDevice(f, offset, total));
    }
    ~ImageFileDevice() override { std::fclose(f_); }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > total_ || len > total_ - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        if (fseek64(f_, base_ + off) != 0)
            return ERR_READ_FAILED;
        if (std::fread(buf, 1, len, f_) != len)
            return ERR_READ_FAILED;
        return OK;
    }
    const Geometry& geometry() const override { return geom_; }

private:
    ImageFileDevice(FILE* f, u64 base, u64 total) : f_(f), base_(base), total_(total) {
        geom_.logical_sector = 512;
        geom_.physical_sector = 512;
        geom_.sector_count = total / 512;
        geom_.total_bytes = total;
    }
    FILE* f_;
    u64 base_, total_;
    Geometry geom_;
};

/* =========================================================================
 * portable DestIo (non-Windows hosts: the ABI-check recovery surface).
 * stdio + POSIX mkdir/rename/fsync; the Win32 backend (WinDestIo) is used
 * on Windows - same contract: temp + atomic rename, recursive mkdir.
 * ========================================================================= */

#ifndef _WIN32
class FileDestIo final : public recovery::DestIo {
public:
    ~FileDestIo() override {
        for (auto& kv : open_)
            if (kv.second.f)
                std::fclose(kv.second.f);
    }

private:
    struct Open {
        FILE* f = nullptr;
        std::string temp, final_path;
    };
    std::map<u64, Open> open_;

    /* core paths use '\\'; host paths here are '/' */
    static std::string to_path(const std::u16string& p) {
        std::string s = u16_to_u8(p);
        for (char& c : s)
            if (c == '\\')
                c = '/';
        return s;
    }

    u32 create_temp(u64 id, const std::u16string& final_path,
                    const std::u16string& temp_suffix) override {
        if (open_.count(id))
            return ERR_BAD_PARAMETER;
        Open o;
        o.final_path = to_path(final_path);
        o.temp = o.final_path + u16_to_u8(temp_suffix);
        o.f = std::fopen(o.temp.c_str(), "wb");
        if (!o.f)
            return ERR_DEST_UNWRITABLE;
        open_[id] = o;
        return OK;
    }
    u32 mkdir(const std::u16string& path) override {
        std::string s = to_path(path);
        while (!s.empty() && s.back() == '/')
            s.pop_back();
        if (s.empty())
            return OK;
        for (size_t i = 1; i <= s.size(); ++i) {
            if (i == s.size() || s[i] == '/') {
                if (::mkdir(s.substr(0, i).c_str(), 0777) != 0 && errno != EEXIST)
                    return ERR_DEST_UNWRITABLE;
            }
        }
        return OK;
    }
    u32 write(u64 id, u64 off, const void* data, u32 len) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        if (fseeko(it->second.f, off_t(off), SEEK_SET) != 0)
            return ERR_WRITE_FAILED;
        if (len && std::fwrite(data, 1, len, it->second.f) != len)
            return ERR_WRITE_FAILED;
        return OK;
    }
    u32 commit(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return ERR_BAD_PARAMETER;
        const u32 rc = std::fflush(it->second.f) == 0 && fsync(fileno(it->second.f)) == 0
                           ? OK
                           : ERR_WRITE_FAILED;
        std::fclose(it->second.f);
        it->second.f = nullptr;
        if (rc != OK)
            return rc;
        if (std::rename(it->second.temp.c_str(), it->second.final_path.c_str()) != 0)
            return ERR_WRITE_FAILED;
        return OK;
    }
    void close(u64 id) override {
        auto it = open_.find(id);
        if (it == open_.end())
            return;
        if (it->second.f)
            std::fclose(it->second.f);
        open_.erase(it);
    }
    bool exists(const std::u16string& path) override {
        struct stat st;
        return ::stat(to_path(path).c_str(), &st) == 0;
    }
};
#endif

/* =========================================================================
 * handles
 * ========================================================================= */

struct CrScan {
    tree::ResultTree tree;
    orchestrator::ScanConfig cfg;
    PausableToken token;
    CrDevice* dev = nullptr; /* borrowed; outlives the scan */
    std::unique_ptr<orchestrator::ScanController> ctrl;
    orchestrator::ScanStats stats;
    cr_ui_progress_fn progress = nullptr;
    void* progress_user = nullptr;
    std::atomic<bool> running{false};
    bool started = false;  /* run() entered at least once (one-shot) */
    bool has_ctrl = false; /* fresh-run or resumed controller exists */
    bool is_resumed = false;
    u32 resumed_boot_hash = 0;
    std::string ck_path;
    FILE* ck_file = nullptr;
    /* progress bridge throttle state (scan thread only) */
    u32 last_pm = 0xFFFFFFFFu;
    u32 last_stage = 0xFFFFFFFFu;
#ifdef _WIN32
    std::vector<std::unique_ptr<IStorageDevice>> ldm_members;
#endif
};

struct CrSession {
    std::string path;
    tree::ResultTree tree;
    orchestrator::ScanStats stats;
    session::VolumeSessionInfo info;
};

struct CrSnapshot {
    std::unique_ptr<tree::Snapshot> snap;
};

struct PropReleaser {
    void operator()(tree::FileProperties* p) const {
        if (p)
            p->release();
    }
};

struct CrPreview {
    std::unique_ptr<tree::FileProperties, PropReleaser> owned;
    std::unique_ptr<preview::PreviewReader> reader;
};

struct CrRecovery {
    PausableToken token;
    std::unique_ptr<recovery::DestIo> io;
    std::unique_ptr<recovery::SafeWriter> writer;
    CrDevice* dev = nullptr;  /* borrowed */
    std::u16string dest_root; /* trailing separator, core convention */
    bool have_dest = false;
    cr_ui_recovery_progress_fn progress = nullptr;
    void* progress_user = nullptr;
    recovery::WriterStats stats{};
    std::chrono::steady_clock::time_point t0{};
    bool processed = false;
};

/* =========================================================================
 * scan progress bridge: orchestrator progress -> cr_ui_progress_fn with
 * per-stage detail derived from the telemetry truth (no invention - the
 * running stage is inferred from configured + started/completed stages)
 * ========================================================================= */

bool scan_progress_bridge_(u32 overall_pm, void* user) {
    auto* s = static_cast<CrScan*>(user);
    if (!s || !s->progress)
        return true;
    const orchestrator::ScanTelemetrySnapshot t =
        s->ctrl ? s->ctrl->telemetry() : orchestrator::ScanTelemetrySnapshot{};
    u32 stage = 0xFFFFFFFFu;
    const orchestrator::ScanConfig& c = s->cfg;
    if (c.run_carve && (t.carve.permille > 0 || t.carve.completed))
        stage = 2;
    else if (c.run_deep && (t.deep.permille > 0 || t.deep.completed))
        stage = 1;
    else if (c.run_quick && (t.quick.permille > 0 || t.quick.completed))
        stage = 0;
    else if (c.run_quick)
        stage = 0;
    else if (c.run_deep)
        stage = 1;
    else if (c.run_carve)
        stage = 2;
    if (overall_pm == s->last_pm && stage == s->last_stage)
        return true;
    s->last_pm = overall_pm;
    s->last_stage = stage;
    u32 stage_pm = 0;
    u64 bytes_done = 0, bytes_total = 0;
    if (stage == 0) {
        stage_pm = t.quick.permille;
    } else if (stage == 1) {
        stage_pm = t.deep.permille;
    } else if (stage == 2) {
        /* the carve stage counts BYTES in its units */
        stage_pm = t.carve.permille;
        bytes_done = t.carve.units_done;
        bytes_total = t.carve.units_total;
    }
    s->progress(s->progress_user, stage, stage_pm, overall_pm, bytes_done, bytes_total,
                u32(t.errors.ranges().size()));
    return true;
}

bool recovery_progress_bridge_(u64 done, u64 total, void* user) {
    auto* r = static_cast<CrRecovery*>(user);
    if (r && r->progress)
        r->progress(r->progress_user, done, total);
    return true; /* no cancellation semantics - the token is the cancel path */
}

/* =========================================================================
 * tree helpers
 * ========================================================================= */

tree::FileNode* find_node_recursive(tree::FileNode* n, u64 id) {
    if (n->node_id() == id)
        return n;
    for (auto* c = n->first_child(); c; c = c->next_sibling())
        if (auto* r = find_node_recursive(c, id))
            return r;
    return nullptr;
}

tree::FileNode* find_node(tree::ResultTree& t, u64 id) {
    for (auto* r : t.roots())
        if (auto* x = find_node_recursive(r, id))
            return x;
    return nullptr;
}

/* ancestor names strictly between the tree ROOT and `node` (both excluded),
 * each with a trailing separator - the CLI walk's path accumulation */
std::u16string rel_dir_of(const tree::FileNode* node) {
    std::vector<const tree::FileNode*> chain;
    for (const tree::FileNode* a = node->parent(); a && a->parent(); a = a->parent())
        chain.push_back(a);
    std::u16string out;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        if ((*it)->props())
            out += (*it)->props()->name + u"\\";
    return out;
}

std::u16string rel_dir_of_snap(const tree::Snapshot& snap, u32 index) {
    /* ancestor names strictly between the ROOT (a full node) and `index` */
    std::vector<u32> chain;
    for (u32 a = snap.node(index).parent;
         a != CR_UI_SNAP_NONE && !snap.node(a).full && snap.node(a).parent != CR_UI_SNAP_NONE;)
        chain.push_back(a), a = snap.node(a).parent;
    std::u16string out;
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        out += snap.node(*it).name + u"\\";
    return out;
}

/* config -> ScanConfig (shared by scan_create and session_resume) */
u32 fill_scan_config(const cr_ui_scan_config* in, orchestrator::ScanConfig& out) {
    out = orchestrator::ScanConfig{};
    const u32 mask = in && in->stage_mask ? in->stage_mask : 7u;
    if (mask & ~7u)
        return ret_(ERR_BAD_PARAMETER, "stage_mask has unknown bits (valid: 1|2|4)");
    out.run_quick = (mask & 1) != 0;
    out.run_deep = (mask & 2) != 0;
    out.run_carve = (mask & 4) != 0;
    out.whole_disk = in && in->whole_disk != 0;
    if (in && in->fs_hint) {
        switch (in->fs_hint) {
        case 1:
            out.fs_hint = orchestrator::FsFamily::Ntfs;
            break;
        case 2:
            out.fs_hint = orchestrator::FsFamily::Fat;
            break;
        case 3:
            out.fs_hint = orchestrator::FsFamily::Exfat;
            break;
        case 4:
            out.fs_hint = orchestrator::FsFamily::Ext4;
            break;
        case 6:
            out.fs_hint = orchestrator::FsFamily::Hfsplus;
            break;
        case 7:
            out.fs_hint = orchestrator::FsFamily::Btrfs;
            break;
        case 8:
            out.fs_hint = orchestrator::FsFamily::Zfs;
            break;
        case 9:
            out.fs_hint = orchestrator::FsFamily::Refs;
            break;
        default:
            return ret_(ERR_BAD_PARAMETER, "fs_hint %u is not a valid FsFamily value", in->fs_hint);
        }
    }
    if (in && in->quick_weight)
        out.weights.quick = in->quick_weight;
    if (in && in->deep_weight)
        out.weights.deep = in->deep_weight;
    if (in && in->carve_weight)
        out.weights.carve = in->carve_weight;
    if (in)
        out.threads = in->threads;
    copy_card(out.volume_label, sizeof(out.volume_label), in ? in->volume_label_utf8 : nullptr);
    copy_card(out.volume_model, sizeof(out.volume_model), in ? in->volume_model_utf8 : nullptr);
    copy_card(out.volume_serial, sizeof(out.volume_serial), in ? in->volume_serial_utf8 : nullptr);
    return OK;
}

} // namespace

/* =========================================================================
 * version & diagnostics
 * ========================================================================= */

extern "C" CR_UI_API uint32_t cr_ui_abi_version(void) {
    return CR_UI_ABI_VERSION;
}

extern "C" CR_UI_API const char* cr_ui_last_error(void) {
    return g_last_error.c_str();
}

extern "C" CR_UI_API void cr_ui_string_free(char* s) {
    std::free(s);
}

/* =========================================================================
 * devices
 * ========================================================================= */

namespace {

u32 open_image_impl(const char* utf8_path, uint64_t offset, uint64_t bytes, cr_ui_device** out) {
    if (!utf8_path || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: path/out");
    u32 err = OK;
    auto dev = ImageFileDevice::open(utf8_path, offset, bytes, &err);
    if (!dev)
        return ret_(err, "cannot open image %s", utf8_path);
    auto* d = new CrDevice();
    d->dev = std::move(dev);
    d->transport = "image";
    *out = reinterpret_cast<cr_ui_device*>(d);
    return ret_ok();
}

#ifdef _WIN32

u32 open_disk_impl(uint32_t disk, cr_ui_device** out) {
    if (!out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: out");
    u32 err = 0, gle = 0;
    if (win::rda_available(L"RDA", &gle)) {
        if (auto dev = win::open_rda_partition(L"RDA", disk, 0, &err)) {
            auto* d = new CrDevice();
            d->dev = std::move(dev);
            d->disk = disk;
            d->disk_known = true;
            d->transport = "rda";
            *out = reinterpret_cast<cr_ui_device*>(d);
            return ret_ok();
        }
    }
    auto dev = win::open_physical_drive(disk, &err);
    if (!dev)
        return ret_(err ? err : u32(ERR_OPEN_FAILED),
                    "cannot open disk %u (RDA and direct paths failed)", disk);
    auto* d = new CrDevice();
    d->dev = std::move(dev);
    d->disk = disk;
    d->disk_known = true;
    d->transport = "direct";
    *out = reinterpret_cast<cr_ui_device*>(d);
    return ret_ok();
}

u32 open_volume_impl(char letter, cr_ui_device** out) {
    if (!out || !letter)
        return ret_(ERR_BAD_PARAMETER, "bad argument: letter/out");
    const win::bitlocker::Status bst = win::bitlocker::status(wchar_t(letter));
    if (bst == win::bitlocker::Status::Unknown)
        return ret_(ERR_OPEN_FAILED,
                    "%c: boot sector unreadable - cannot determine BitLocker state "
                    "(typical for a locked volume); unlock and re-try",
                    letter);
    if (bst == win::bitlocker::Status::Locked) {
        if (win::bitlocker::try_auto_unlock(wchar_t(letter)) != OK)
            return ret_(ERR_FVE_LOCKED,
                        "%c: locked - unlock (Unlock-BitLocker -MountPoint %c: "
                        "-RecoveryPassword <key>) and re-try",
                        letter, letter);
    }
    u32 disk = 0;
    u64 start = 0, len = 0;
    if (!win::volume_extent(wchar_t(letter), &disk, &start, &len))
        return ret_(ERR_OPEN_FAILED, "cannot map %c: to a disk extent", letter);
    u32 err = 0, gle = 0;
    std::unique_ptr<IStorageDevice> whole;
    const char* transport = "direct";
    if (win::rda_available(L"RDA", &gle)) {
        if ((whole = win::open_rda_partition(L"RDA", disk, 0, &err)))
            transport = "rda";
    }
    if (!whole && !(whole = win::open_physical_drive(disk, &err)))
        return ret_(err ? err : u32(ERR_OPEN_FAILED), "cannot open disk %u for volume %c:", disk,
                    letter);
    auto slice = PartitionSlice::create(whole.get(), start, len, nullptr);
    if (!slice)
        return ret_(ERR_OPEN_FAILED, "slice creation failed for %c: (@%llu +%llu)", letter,
                    static_cast<unsigned long long>(start), static_cast<unsigned long long>(len));
    auto* d = new CrDevice();
    d->hold = std::move(whole);
    d->dev = std::move(slice);
    d->disk = disk;
    d->disk_known = true;
    d->volume_len = len;
    d->transport = transport;
    *out = reinterpret_cast<cr_ui_device*>(d);
    return ret_ok();
}

#else /* !_WIN32 */

u32 open_disk_impl(uint32_t disk, cr_ui_device** out) {
    (void)disk;
    (void)out;
    return ret_(ERR_UNSUPPORTED, "cr_ui_open_disk requires the Windows platform layer");
}

u32 open_volume_impl(char letter, cr_ui_device** out) {
    (void)letter;
    (void)out;
    return ret_(ERR_UNSUPPORTED, "cr_ui_open_volume requires the Windows platform layer");
}

#endif

u32 device_geometry_impl(cr_ui_device* dev, cr_ui_geometry* out) {
    if (!dev || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/out");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    const Geometry& g = d->dev->geometry();
    out->total_bytes = g.total_bytes;
    out->logical_sector = g.logical_sector;
    out->physical_sector = g.physical_sector;
    out->sector_count = g.sector_count;
    return ret_ok();
}

u32 device_transport_impl(cr_ui_device* dev, char* buf, uint32_t cap, uint32_t* needed) {
    if (!dev || !needed)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/needed");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    const std::string t = d->transport;
    if (!buf) {
        *needed = u32(t.size() + 1);
        return ret_ok();
    }
    if (cap == 0)
        return ret_(ERR_BAD_PARAMETER, "bad argument: cap == 0 with a buffer");
    *needed = u32(t.size() + 1);
    const bool fit = copy_bstr(buf, cap, t);
    return ret_(fit ? OK : u32(ERR_TRUNCATED), "%s", d->transport);
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_open_image(const char* utf8_path, uint64_t offset,
                                               uint64_t bytes, cr_ui_device** out) {
    return guarded(open_image_impl, utf8_path, offset, bytes, out);
}
extern "C" CR_UI_API uint32_t cr_ui_open_disk(uint32_t disk, cr_ui_device** out) {
    return guarded(open_disk_impl, disk, out);
}
extern "C" CR_UI_API uint32_t cr_ui_open_volume(char letter, cr_ui_device** out) {
    return guarded(open_volume_impl, letter, out);
}
extern "C" CR_UI_API void cr_ui_device_close(cr_ui_device* dev) {
    if (!dev)
        return;
    try {
        delete reinterpret_cast<CrDevice*>(dev);
    } catch (...) {
    }
}
extern "C" CR_UI_API uint32_t cr_ui_device_geometry(cr_ui_device* dev, cr_ui_geometry* out) {
    return guarded(device_geometry_impl, dev, out);
}
extern "C" CR_UI_API uint32_t cr_ui_device_transport(cr_ui_device* dev, char* buf, uint32_t cap,
                                                     uint32_t* needed) {
    return guarded(device_transport_impl, dev, buf, cap, needed);
}

/* =========================================================================
 * enumeration (Windows platform layer)
 * ========================================================================= */

namespace {

#ifdef _WIN32

u32 enumerate_disks_impl(cr_ui_disk_info* out, uint32_t* count) {
    if (!count)
        return ret_(ERR_BAD_PARAMETER, "bad argument: count");
    win::PhysicalDiskList list;
    const u32 rc = win::enumerate_disks(200, list);
    if (rc != OK)
        return ret_(rc, "disk enumeration failed");
    const u32 needed = u32(list.size());
    if (!out) {
        *count = needed;
        return ret_ok();
    }
    const u32 cap = *count;
    const u32 n = needed < cap ? needed : cap;
    for (u32 i = 0; i < n; ++i) {
        const win::PhysicalDisk& d = list[i];
        cr_ui_disk_info& o = out[i];
        std::memset(&o, 0, sizeof(o));
        o.number = d.number;
        o.size_bytes = d.size_bytes;
        o.logical_sector = d.logical_sector;
        o.physical_sector = d.physical_sector;
        o.removable = d.removable ? 1u : 0u;
        o.table_ok = d.table_ok ? 1u : 0u;
        if (d.table_ok) {
            if (d.table.layout == table::TableLayout::Gpt)
                o.layout = 2;
            else if (d.table.layout == table::TableLayout::Mbr)
                o.layout = 1;
        }
        copy_bstr(o.model, sizeof(o.model), ws_to_u8(d.model));
        copy_bstr(o.serial, sizeof(o.serial), ws_to_u8(d.serial));
    }
    *count = n;
    return ret_(n == needed ? OK : u32(ERR_TRUNCATED), "%s", "");
}

u32 enumerate_volumes_impl(cr_ui_volume_info* out, uint32_t* count) {
    if (!count)
        return ret_(ERR_BAD_PARAMETER, "bad argument: count");
    win::VolumeEntryList vols;
    const u32 rc = win::enumerate_volumes(vols);
    if (rc != OK)
        return ret_(rc, "volume enumeration failed");
    const u32 needed = u32(vols.size());
    if (!out) {
        *count = needed;
        return ret_ok();
    }
    const u32 cap = *count;
    const u32 n = needed < cap ? needed : cap;
    for (u32 i = 0; i < n; ++i) {
        const win::VolumeEntry& v = vols[i];
        cr_ui_volume_info& o = out[i];
        std::memset(&o, 0, sizeof(o));
        o.letter = char(v.letter ? v.letter : 0);
        o.disk = v.disk;
        o.offset_on_disk = v.offset_on_disk;
        o.bytes = v.bytes;
        o.bitlocker = v.bitlocker ? 1u : 0u;
        copy_bstr(o.fs_name, sizeof(o.fs_name), ws_to_u8(v.fs_name));
        copy_bstr(o.label, sizeof(o.label), ws_to_u8(v.label));
    }
    *count = n;
    return ret_(n == needed ? OK : u32(ERR_TRUNCATED), "%s", "");
}

#else

u32 enumerate_disks_impl(cr_ui_disk_info* out, uint32_t* count) {
    (void)out;
    (void)count;
    return ret_(ERR_UNSUPPORTED, "cr_ui_enumerate_disks requires the Windows platform layer");
}
u32 enumerate_volumes_impl(cr_ui_volume_info* out, uint32_t* count) {
    (void)out;
    (void)count;
    return ret_(ERR_UNSUPPORTED, "cr_ui_enumerate_volumes requires the Windows platform layer");
}

#endif

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_enumerate_disks(cr_ui_disk_info* out, uint32_t* count) {
    return guarded(enumerate_disks_impl, out, count);
}
extern "C" CR_UI_API uint32_t cr_ui_enumerate_volumes(cr_ui_volume_info* out, uint32_t* count) {
    return guarded(enumerate_volumes_impl, out, count);
}

/* =========================================================================
 * scan lifecycle
 * ========================================================================= */

namespace {

u32 scan_create_impl(const cr_ui_scan_config* cfg, cr_ui_scan** out) {
    if (!out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: out");
    auto* s = new CrScan();
    const u32 rc = fill_scan_config(cfg, s->cfg);
    if (rc != OK) {
        delete s;
        return rc; /* fill_scan_config already set last_error */
    }
    if (cfg && cfg->progress) {
        s->progress = cfg->progress;
        s->progress_user = cfg->progress_user;
    }
    if (cfg && cfg->checkpoint_path_utf8 && *cfg->checkpoint_path_utf8)
        s->ck_path = cfg->checkpoint_path_utf8;
    *out = reinterpret_cast<cr_ui_scan*>(s);
    return ret_ok();
}

#ifdef _WIN32
/* SESS-2 identity card from the disk enumeration (CLI fill_identity_card) */
void fill_identity_card(u32 disk, orchestrator::ScanConfig& cfg) {
    win::PhysicalDiskList disks;
    if (win::enumerate_disks(disk + 1, disks) != OK || disks.size() <= disk)
        return;
    const std::string m = ws_to_u8(disks[disk].model);
    const std::string sr = ws_to_u8(disks[disk].serial);
    copy_card(cfg.volume_model, sizeof(cfg.volume_model), m.c_str());
    copy_card(cfg.volume_serial, sizeof(cfg.volume_serial), sr.c_str());
}

/* cross-disk LDM resolver (the cmd_scan block, verbatim semantics) */
void arm_ldm_resolver(CrScan* s) {
    if (!s->cfg.whole_disk || !s->dev->disk_known || s->dev->volume_len != 0)
        return;
    table::LdmInfo probe;
    if (table::parse_ldm_database(s->dev->dev.get(), probe) != OK || !probe.found)
        return;
    win::PhysicalDiskList list;
    if (win::enumerate_disks(32, list) != OK)
        return;
    auto guid_map = std::make_shared<std::map<table::LdmGuid, IStorageDevice*>>();
    for (const auto& pd : list) {
        u32 oerr = 0;
        auto dev = win::open_disk(pd.number, true, L"RDA", &oerr);
        if (!dev)
            dev = win::open_disk(pd.number, false, L"RDA", &oerr);
        if (!dev)
            continue;
        u8 sec[512];
        if (dev->read_at(6 * 512, sec, sizeof(sec)) == OK && std::memcmp(sec, "PRIVHEAD", 8) == 0) {
            table::LdmGuid g{};
            std::memcpy(g.b, sec + 0x30, 16);
            (*guid_map)[g] = dev.get();
        }
        s->ldm_members.push_back(std::move(dev));
    }
    if (!guid_map->empty())
        s->cfg.ldm_resolver = [guid_map](const table::LdmGuid& g) -> IStorageDevice* {
            auto it = guid_map->find(g);
            return it != guid_map->end() ? it->second : nullptr;
        };
}
#endif

u32 scan_attach_impl(cr_ui_scan* scan, cr_ui_device* dev) {
    if (!scan || !dev)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/dev");
    auto* s = reinterpret_cast<CrScan*>(scan);
    auto* d = reinterpret_cast<CrDevice*>(dev);
    if (s->started)
        return ret_(ERR_BAD_PARAMETER, "scan already ran; create a new scan");
    s->dev = d;
#ifdef _WIN32
    /* identity card: caller-supplied values win; otherwise the disk
     * enumeration fills it (CLI parity, quiet on failure) */
    if (d->disk_known && s->cfg.volume_model[0] == 0)
        fill_identity_card(d->disk, s->cfg);
#endif
    return ret_ok();
}

u32 scan_run_impl(cr_ui_scan* scan) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    auto* s = reinterpret_cast<CrScan*>(scan);
    if (s->started)
        return ret_(ERR_BAD_PARAMETER, "scan already ran; create a new scan or resume");
    if (!s->dev)
        return ret_(ERR_BAD_PARAMETER, "no device attached (cr_ui_scan_attach_device)");
    if (s->running.load())
        return ret_(ERR_BAD_PARAMETER, "scan is already running on another thread");

    /* checkpoint wiring (CLI: --out / --resume in-place) */
    if (!s->ck_path.empty() && !s->ck_file) {
        s->ck_file = open_file(s->ck_path.c_str(), true);
        if (!s->ck_file)
            return ret_(ERR_SESSION_IO, "cannot create checkpoint file %s", s->ck_path.c_str());
        s->cfg.checkpoint.write = f_write_cb;
        s->cfg.checkpoint.tell = f_tell_cb;
        s->cfg.checkpoint.seek = f_seek_cb;
        s->cfg.checkpoint.user = s->ck_file;
    }

#ifdef _WIN32
    arm_ldm_resolver(s);
#endif

    if (!s->ctrl) {
        s->ctrl.reset(new orchestrator::ScanController(s->tree, s->cfg));
        s->has_ctrl = true;
    }
    s->started = true;
    s->running.store(true);
    const u32 rc = s->ctrl->run(s->dev->dev.get(), &s->token, scan_progress_bridge_, s, s->stats);
    s->running.store(false);
    if (s->ck_file) {
        std::fclose(s->ck_file);
        s->ck_file = nullptr;
        s->cfg.checkpoint = orchestrator::ScanConfig::CheckpointSink{};
    }
    if (rc != OK)
        return ret_(rc, "scan %s", err_text(rc));
    return ret_ok();
}

u32 scan_pause_impl(cr_ui_scan* scan) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    reinterpret_cast<CrScan*>(scan)->token.pause();
    return ret_ok();
}
u32 scan_resume_impl(cr_ui_scan* scan) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    reinterpret_cast<CrScan*>(scan)->token.resume();
    return ret_ok();
}
u32 scan_cancel_impl(cr_ui_scan* scan) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    reinterpret_cast<CrScan*>(scan)->token.cancel();
    return ret_ok();
}

void fill_stage_tel(const orchestrator::StageTelemetry& in, cr_ui_stage_tel& out) {
    out.permille = in.permille;
    out.units_done = in.units_done;
    out.units_total = in.units_total;
    out.bytes_done = in.bytes_done;
    out.elapsed_ms = in.elapsed_ms;
    out.completed = in.completed ? 1u : 0u;
}

u32 scan_telemetry_impl(cr_ui_scan* scan, cr_ui_telemetry* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    std::memset(out, 0, sizeof(*out));
    if (!s->ctrl)
        return ret_ok(); /* not started: the honest all-zero snapshot */
    const orchestrator::ScanTelemetrySnapshot t = s->ctrl->telemetry();
    out->overall_permille = t.overall_permille;
    fill_stage_tel(t.quick, out->quick);
    fill_stage_tel(t.deep, out->deep);
    fill_stage_tel(t.carve, out->carve);
    out->files_found = t.files_found;
    out->deleted_found = t.deleted_found;
    out->carved_found = t.carved_found;
    out->lost_partitions = t.lost_partitions;
    out->bytes_scanned = t.bytes_scanned;
    out->elapsed_ms = t.elapsed_ms;
    out->error_ranges = u32(t.errors.ranges().size());
    out->paused = t.paused ? 1u : 0u;
    out->running = t.running ? 1u : 0u;
    out->cancelled = t.cancelled ? 1u : 0u;
    return ret_ok();
}

u32 scan_stats_impl(cr_ui_scan* scan, cr_ui_scan_stats* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    const orchestrator::ScanStats& st = s->stats;
    std::memset(out, 0, sizeof(*out));
    out->files_found = st.files_found;
    out->deleted_found = st.deleted_found;
    out->carved_found = st.carved_found;
    out->lost_partitions = st.lost_partitions;
    out->bytes_scanned = st.bytes_scanned;
    out->elapsed_ms = st.elapsed_ms;
    out->cached_hits = st.cached_hits;
    out->cached_misses = st.cached_misses;
    out->error_ranges = u32(st.errors.ranges().size());
    return ret_ok();
}

u32 scan_checkpoint_count_impl(cr_ui_scan* scan, uint64_t* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    *out = s->ctrl ? s->ctrl->checkpoint_count() : 0;
    return ret_ok();
}

u32 scan_save_session_impl(cr_ui_scan* scan, const char* utf8_path) {
    if (!scan || !utf8_path || !*utf8_path)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/path");
    auto* s = reinterpret_cast<CrScan*>(scan);
    if (!s->has_ctrl)
        return ret_(ERR_BAD_PARAMETER, "scan has not run (nothing to save)");
    FILE* f = open_file(utf8_path, true);
    if (!f)
        return ret_(ERR_SESSION_IO, "cannot create session file %s", utf8_path);
    session::Writer w(f_write_cb, f_tell_cb, f_seek_cb, f);
    u32 rc = w.open(0x52434F56);
    if (rc == OK)
        rc = s->ctrl->save_session(w, s->stats);
    if (rc == OK)
        rc = w.finalize();
    std::fclose(f);
    if (rc != OK)
        return ret_(rc, "session save failed (%s)", err_text(rc));
    return ret_ok();
}

u32 scan_snapshot_impl(cr_ui_scan* scan, cr_ui_snapshot** out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    auto snap = tree::Snapshot::capture(s->tree, s->running.load());
    if (!snap)
        return ret_(ERR_NO_MEMORY, "snapshot allocation failed");
    auto* w = new CrSnapshot();
    w->snap = std::move(snap);
    *out = reinterpret_cast<cr_ui_snapshot*>(w);
    return ret_ok();
}

u32 scan_verify_identity_impl(cr_ui_scan* scan) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    auto* s = reinterpret_cast<CrScan*>(scan);
    if (!s->is_resumed)
        return ret_(ERR_BAD_PARAMETER, "not a resumed scan");
    if (s->resumed_boot_hash == 0)
        return ret_ok(); /* v1 session: unverifiable, skipped (CLI truth) */
    if (!s->dev)
        return ret_(ERR_BAD_PARAMETER, "no device attached");
    u8 boot[512] = {};
    if (s->dev->dev->read_at(0, boot, sizeof(boot)) != OK)
        return ret_ok(); /* unreadable start: unverifiable, not an error */
    if (table::crc32(boot, sizeof(boot)) != s->resumed_boot_hash)
        return ret_(ERR_BAD_CHECKSUM,
                    "volume identity mismatch - the session was saved on a different "
                    "volume (boot hash differs)");
    return ret_ok();
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_scan_create(const cr_ui_scan_config* cfg, cr_ui_scan** out) {
    return guarded(scan_create_impl, cfg, out);
}
extern "C" CR_UI_API void cr_ui_scan_close(cr_ui_scan* scan) {
    if (!scan)
        return;
    try {
        auto* s = reinterpret_cast<CrScan*>(scan);
        if (s->ck_file)
            std::fclose(s->ck_file);
        delete s;
    } catch (...) {
    }
}
extern "C" CR_UI_API uint32_t cr_ui_scan_attach_device(cr_ui_scan* scan, cr_ui_device* dev) {
    return guarded(scan_attach_impl, scan, dev);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_run(cr_ui_scan* scan) {
    return guarded(scan_run_impl, scan);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_pause(cr_ui_scan* scan) {
    return guarded(scan_pause_impl, scan);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_resume(cr_ui_scan* scan) {
    return guarded(scan_resume_impl, scan);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_cancel(cr_ui_scan* scan) {
    return guarded(scan_cancel_impl, scan);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_telemetry(cr_ui_scan* scan, cr_ui_telemetry* out) {
    return guarded(scan_telemetry_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_get_stats(cr_ui_scan* scan, cr_ui_scan_stats* out) {
    return guarded(scan_stats_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_checkpoint_count(cr_ui_scan* scan, uint64_t* out) {
    return guarded(scan_checkpoint_count_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_save_session(cr_ui_scan* scan, const char* utf8_path) {
    return guarded(scan_save_session_impl, scan, utf8_path);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_snapshot(cr_ui_scan* scan, cr_ui_snapshot** out) {
    return guarded(scan_snapshot_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_verify_identity(cr_ui_scan* scan) {
    return guarded(scan_verify_identity_impl, scan);
}

/* =========================================================================
 * sessions
 * ========================================================================= */

namespace {

u32 session_load_impl(const char* utf8_path, cr_ui_session** out) {
    if (!utf8_path || !*utf8_path || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: path/out");
    FILE* f = open_file(utf8_path, false);
    if (!f)
        return ret_(ERR_OPEN_FAILED, "cannot open session %s", utf8_path);
    auto* sess = new CrSession();
    sess->path = utf8_path;
    session::Reader r(f_read_cb, f_seek_cb, f_size_cb, f);
    const u32 rc =
        orchestrator::ScanController::load_session_summary(r, sess->tree, sess->stats, sess->info);
    std::fclose(f);
    if (rc != OK) {
        delete sess;
        return ret_(rc, "session load failed (%s)", err_text(rc));
    }
    *out = reinterpret_cast<cr_ui_session*>(sess);
    return ret_ok();
}

u32 session_info_impl(cr_ui_session* sess, cr_ui_session_info* out) {
    if (!sess || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: sess/out");
    auto* s = reinterpret_cast<CrSession*>(sess);
    const session::VolumeSessionInfo& i = s->info;
    std::memset(out, 0, sizeof(*out));
    out->fs_family = i.fs_family;
    out->stage_done_mask = i.stage_done_mask;
    out->configured_mask = i.configured_mask;
    out->slice_offset = i.slice_offset;
    out->slice_bytes = i.slice_bytes;
    out->logical_sector = i.logical_sector;
    out->physical_sector = i.physical_sector;
    out->scanned_bytes = i.scanned_bytes;
    out->elapsed_ms = i.elapsed_ms;
    out->boot_hash = i.boot_hash;
    copy_cstr(out->label, sizeof(out->label), i.label);
    copy_cstr(out->model, sizeof(out->model), i.model);
    copy_cstr(out->serial, sizeof(out->serial), i.serial);
    return ret_ok();
}

u32 session_stats_impl(cr_ui_session* sess, cr_ui_scan_stats* out) {
    if (!sess || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: sess/out");
    auto* s = reinterpret_cast<CrSession*>(sess);
    const orchestrator::ScanStats& st = s->stats;
    std::memset(out, 0, sizeof(*out));
    out->files_found = st.files_found;
    out->deleted_found = st.deleted_found;
    out->carved_found = st.carved_found;
    out->lost_partitions = st.lost_partitions;
    out->bytes_scanned = st.bytes_scanned;
    out->elapsed_ms = st.elapsed_ms;
    out->error_ranges = u32(st.errors.ranges().size());
    return ret_ok();
}

u32 session_snapshot_impl(cr_ui_session* sess, cr_ui_snapshot** out) {
    if (!sess || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: sess/out");
    auto* s = reinterpret_cast<CrSession*>(sess);
    auto snap = tree::Snapshot::capture(s->tree, false);
    if (!snap)
        return ret_(ERR_NO_MEMORY, "snapshot allocation failed");
    auto* w = new CrSnapshot();
    w->snap = std::move(snap);
    *out = reinterpret_cast<cr_ui_snapshot*>(w);
    return ret_ok();
}

u32 session_resume_impl(cr_ui_session* sess, const cr_ui_scan_config* original_cfg,
                        cr_ui_scan** out) {
    if (!sess || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: sess/out");
    auto* s = reinterpret_cast<CrSession*>(sess);
    auto* scan = new CrScan();
    const u32 crc = fill_scan_config(original_cfg, scan->cfg);
    if (crc != OK) {
        delete scan;
        return crc;
    }
    if (original_cfg && original_cfg->progress) {
        scan->progress = original_cfg->progress;
        scan->progress_user = original_cfg->progress_user;
    }
    if (original_cfg && original_cfg->checkpoint_path_utf8 && *original_cfg->checkpoint_path_utf8)
        scan->ck_path = original_cfg->checkpoint_path_utf8;
    FILE* f = open_file(s->path.c_str(), false);
    if (!f) {
        delete scan;
        return ret_(ERR_OPEN_FAILED, "cannot re-open session %s", s->path.c_str());
    }
    session::Reader r(f_read_cb, f_seek_cb, f_size_cb, f);
    std::unique_ptr<orchestrator::ScanController> ctrl;
    session::VolumeSessionInfo info;
    const u32 rc =
        orchestrator::ScanController::resume(r, scan->tree, scan->cfg, scan->stats, info, ctrl);
    std::fclose(f);
    if (rc != OK) {
        delete scan;
        return ret_(rc, "resume failed (%s)", err_text(rc));
    }
    scan->ctrl = std::move(ctrl);
    scan->has_ctrl = true;
    scan->is_resumed = true;
    scan->resumed_boot_hash = info.boot_hash;
    *out = reinterpret_cast<cr_ui_scan*>(scan);
    return ret_ok();
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_session_load(const char* utf8_path, cr_ui_session** out) {
    return guarded(session_load_impl, utf8_path, out);
}
extern "C" CR_UI_API void cr_ui_session_close(cr_ui_session* sess) {
    if (!sess)
        return;
    try {
        delete reinterpret_cast<CrSession*>(sess);
    } catch (...) {
    }
}
extern "C" CR_UI_API uint32_t cr_ui_session_get_info(cr_ui_session* sess, cr_ui_session_info* out) {
    return guarded(session_info_impl, sess, out);
}
extern "C" CR_UI_API uint32_t cr_ui_session_stats(cr_ui_session* sess, cr_ui_scan_stats* out) {
    return guarded(session_stats_impl, sess, out);
}
extern "C" CR_UI_API uint32_t cr_ui_session_snapshot(cr_ui_session* sess, cr_ui_snapshot** out) {
    return guarded(session_snapshot_impl, sess, out);
}
extern "C" CR_UI_API uint32_t cr_ui_session_resume(cr_ui_session* sess,
                                                   const cr_ui_scan_config* original_cfg,
                                                   cr_ui_scan** out) {
    return guarded(session_resume_impl, sess, original_cfg, out);
}

/* =========================================================================
 * snapshot enumeration
 * ========================================================================= */

namespace {

u32 snapshot_node_count_impl(cr_ui_snapshot* snap, uint32_t* out) {
    if (!snap || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: snap/out");
    *out = reinterpret_cast<CrSnapshot*>(snap)->snap->node_count();
    return ret_ok();
}

u32 snapshot_partial_impl(cr_ui_snapshot* snap, uint32_t* out) {
    if (!snap || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: snap/out");
    *out = reinterpret_cast<CrSnapshot*>(snap)->snap->partial() ? 1u : 0u;
    return ret_ok();
}

u32 snapshot_node_impl(cr_ui_snapshot* snap, uint32_t index, cr_ui_node_info* out, char* name_buf,
                       uint32_t name_cap) {
    if (!snap || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: snap/out");
    auto* w = reinterpret_cast<CrSnapshot*>(snap);
    if (index >= w->snap->node_count())
        return ret_(ERR_BAD_PARAMETER, "node index %u out of range (%u nodes)", index,
                    w->snap->node_count());
    const tree::SnapNode& n = w->snap->node(index);
    std::memset(out, 0, sizeof(*out));
    out->parent = n.parent;
    out->first_child = n.first_child;
    out->next_sibling = n.next_sibling;
    out->prev_sibling = n.prev_sibling;
    out->node_id = n.node_id;
    out->fs_kind = n.fs_kind;
    out->record_ref = n.record_ref;
    out->origin = n.origin;
    out->deleted = n.deleted;
    out->confidence = n.confidence;
    out->is_full = n.full ? 1u : 0u;
    out->logical_size = n.logical_size;
    out->allocated_size = n.allocated_size;
    out->created = n.times.created;
    out->modified = n.times.modified;
    out->changed = n.times.changed;
    out->accessed = n.times.accessed;
    out->attributes = n.attributes;
    out->flags = n.flags;
    out->extent_count = u32(n.extents.size());
    out->resident_bytes = u32(n.resident_data.size());
    out->carve_format = n.carve_format;
    out->carve_confidence = n.carve_confidence;
    out->hash_valid = n.hash_valid ? 1u : 0u;
    std::memcpy(out->sha128, n.sha128, 16);
    const std::string name = u16_to_u8(n.name);
    out->name_len = u32(name.size());
    if (!name_buf)
        return ret_ok();
    if (name_cap == 0)
        return ret_(ERR_BAD_PARAMETER, "bad argument: name_cap == 0 with a buffer");
    const bool fit = copy_bstr(name_buf, name_cap, name);
    return ret_(fit ? OK : u32(ERR_TRUNCATED), "%s", "");
}

u32 snapshot_dfs_impl(cr_ui_snapshot* snap, uint32_t* indices, uint32_t* count) {
    if (!snap || !count)
        return ret_(ERR_BAD_PARAMETER, "bad argument: snap/count");
    auto* w = reinterpret_cast<CrSnapshot*>(snap);
    std::vector<u32> order;
    w->snap->dfs(order);
    const u32 needed = u32(order.size());
    if (!indices) {
        *count = needed;
        return ret_ok();
    }
    const u32 cap = *count;
    const u32 n = needed < cap ? needed : cap;
    if (n)
        std::memcpy(indices, order.data(), size_t(n) * sizeof(u32));
    *count = n;
    return ret_(n == needed ? OK : u32(ERR_TRUNCATED), "%s", "");
}

} // namespace

extern "C" CR_UI_API void cr_ui_snapshot_close(cr_ui_snapshot* snap) {
    if (!snap)
        return;
    try {
        delete reinterpret_cast<CrSnapshot*>(snap);
    } catch (...) {
    }
}
extern "C" CR_UI_API uint32_t cr_ui_snapshot_node_count(cr_ui_snapshot* snap, uint32_t* out) {
    return guarded(snapshot_node_count_impl, snap, out);
}
extern "C" CR_UI_API uint32_t cr_ui_snapshot_partial(cr_ui_snapshot* snap, uint32_t* out) {
    return guarded(snapshot_partial_impl, snap, out);
}
extern "C" CR_UI_API uint32_t cr_ui_snapshot_node(cr_ui_snapshot* snap, uint32_t index,
                                                  cr_ui_node_info* out, char* name_buf,
                                                  uint32_t name_cap) {
    return guarded(snapshot_node_impl, snap, index, out, name_buf, name_cap);
}
extern "C" CR_UI_API uint32_t cr_ui_snapshot_dfs(cr_ui_snapshot* snap, uint32_t* indices,
                                                 uint32_t* count) {
    return guarded(snapshot_dfs_impl, snap, indices, count);
}

/* =========================================================================
 * query (session trees)
 * ========================================================================= */

namespace {

u32 query_run_impl(cr_ui_session* sess, const cr_ui_query* q, cr_ui_query_row** rows,
                   uint32_t* row_count, uint64_t* total_matched) {
    if (!sess || !q || !rows || !row_count || !total_matched)
        return ret_(ERR_BAD_PARAMETER, "bad argument: sess/q/rows/row_count/total_matched");
    auto* s = reinterpret_cast<CrSession*>(sess);
    query::Query cq;
    cq.filter.category_mask = q->category_mask ? q->category_mask : 0xFFu;
    cq.filter.size_min = q->size_min;
    cq.filter.size_max = q->size_max ? q->size_max : ~u64(0);
    cq.filter.modified_from = q->modified_from;
    cq.filter.modified_to = q->modified_to ? q->modified_to : ~u64(0);
    cq.filter.confidence_min = q->confidence_min;
    for (uint32_t i = 0; i < q->extension_count; ++i)
        cq.filter.extensions.push_back(q->extensions[i] ? q->extensions[i] : "");
    if (q->name_substring)
        cq.filter.name_substring = q->name_substring;
    cq.filter.deleted_only = q->deleted_only != 0;
    cq.filter.origin_mask = q->origin_mask ? q->origin_mask : 0xFu;
    switch (q->sort) {
    case 1:
        cq.sort = query::SortKey::Name;
        break;
    case 2:
        cq.sort = query::SortKey::Size;
        break;
    case 3:
        cq.sort = query::SortKey::Modified;
        break;
    case 4:
        cq.sort = query::SortKey::Confidence;
        break;
    case 5:
        cq.sort = query::SortKey::Path;
        break;
    default:
        cq.sort = query::SortKey::None;
        break;
    }
    cq.dir = q->dir ? query::SortDir::Desc : query::SortDir::Asc;
    cq.offset = q->offset;
    cq.limit = q->limit ? q->limit : 100;
    cq.count_only = q->count_only != 0;

    const query::Result res = query::run(s->tree, cq);
    *total_matched = res.total_matched;
    const u32 n = u32(res.rows.size());
    if (n == 0) {
        *rows = nullptr;
        *row_count = 0;
        return ret_ok();
    }
    cr_ui_query_row* arr = static_cast<cr_ui_query_row*>(std::calloc(n, sizeof(cr_ui_query_row)));
    if (!arr)
        return ret_(ERR_NO_MEMORY, "row array allocation failed");
    for (u32 i = 0; i < n; ++i) {
        const query::Row& r = res.rows[i];
        arr[i].node_id = r.node ? r.node->node_id() : 0;
        arr[i].name = dup_str(r.name_utf8);
        arr[i].path = dup_str(r.path_utf8);
        arr[i].size = r.size;
        arr[i].modified = r.modified;
        arr[i].origin = r.origin;
        arr[i].deleted = r.deleted;
        arr[i].confidence = r.confidence;
        arr[i].category = r.category;
        if (!arr[i].name || !arr[i].path) {
            for (u32 k = 0; k <= i; ++k) {
                std::free(arr[k].name);
                std::free(arr[k].path);
            }
            std::free(arr);
            return ret_(ERR_NO_MEMORY, "row string allocation failed");
        }
    }
    *rows = arr;
    *row_count = n;
    return ret_ok();
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_query_run(cr_ui_session* sess, const cr_ui_query* q,
                                              cr_ui_query_row** rows, uint32_t* row_count,
                                              uint64_t* total_matched) {
    return guarded(query_run_impl, sess, q, rows, row_count, total_matched);
}
extern "C" CR_UI_API void cr_ui_query_rows_free(cr_ui_query_row* rows, uint32_t row_count) {
    if (!rows)
        return;
    for (uint32_t i = 0; i < row_count; ++i) {
        std::free(rows[i].name);
        std::free(rows[i].path);
    }
    std::free(rows);
}

/* =========================================================================
 * preview
 * ========================================================================= */

namespace {

u32 preview_open_snapshot_impl(cr_ui_device* dev, cr_ui_snapshot* snap, uint32_t index,
                               cr_ui_preview** out) {
    if (!dev || !snap || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/snap/out");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    auto* w = reinterpret_cast<CrSnapshot*>(snap);
    if (index >= w->snap->node_count())
        return ret_(ERR_BAD_PARAMETER, "node index %u out of range", index);
    const tree::SnapNode& n = w->snap->node(index);
    if (n.full)
        return ret_(ERR_BAD_PARAMETER, "root/virtual nodes have no previewable content");
    tree::FileProperties* props = w->snap->materialize_props(index);
    if (!props)
        return ret_(ERR_NO_MEMORY, "props materialization failed");
    auto* p = new CrPreview();
    p->owned.reset(props);
    p->reader.reset(new preview::PreviewReader(d->dev.get(), p->owned.get()));
    *out = reinterpret_cast<cr_ui_preview*>(p);
    return ret_ok();
}

u32 preview_open_session_impl(cr_ui_device* dev, cr_ui_session* sess, uint64_t node_id,
                              cr_ui_preview** out) {
    if (!dev || !sess || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/sess/out");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    auto* s = reinterpret_cast<CrSession*>(sess);
    tree::FileNode* node = find_node(s->tree, node_id);
    if (!node)
        return ret_(ERR_BAD_PARAMETER, "node id %llu not found in the session tree",
                    static_cast<unsigned long long>(node_id));
    if (!node->props())
        return ret_(ERR_BAD_PARAMETER, "node has no content properties");
    if (node->is_full())
        return ret_(ERR_BAD_PARAMETER, "root/virtual nodes have no previewable content");
    /* the preview owns a private copy - the session may close independently */
    tree::FileProperties* props = node->props()->clone();
    if (!props)
        return ret_(ERR_NO_MEMORY, "props clone failed");
    auto* p = new CrPreview();
    p->owned.reset(props);
    p->reader.reset(new preview::PreviewReader(d->dev.get(), p->owned.get()));
    *out = reinterpret_cast<cr_ui_preview*>(p);
    return ret_ok();
}

u32 preview_size_impl(cr_ui_preview* pv, uint64_t* out) {
    if (!pv || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: pv/out");
    *out = reinterpret_cast<CrPreview*>(pv)->reader->size();
    return ret_ok();
}

u32 preview_read_impl(cr_ui_preview* pv, uint64_t offset, void* buf, uint32_t len,
                      uint32_t* placed) {
    if (!pv || !placed)
        return ret_(ERR_BAD_PARAMETER, "bad argument: pv/placed");
    if (len && !buf)
        return ret_(ERR_BAD_PARAMETER, "bad argument: buf NULL with len > 0");
    *placed = reinterpret_cast<CrPreview*>(pv)->reader->read_at(offset, buf, len);
    return ret_ok();
}

u32 preview_metadata_impl(cr_ui_preview* pv, cr_ui_metadata* out) {
    if (!pv || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: pv/out");
    auto* p = reinterpret_cast<CrPreview*>(pv);
    const preview::Metadata m = preview::extract_metadata(*p->reader, preview::Caps{});
    std::memset(out, 0, sizeof(*out));
    bool fit = true;
    fit &= copy_bstr(out->kind, sizeof(out->kind), m.kind);
    fit &= copy_bstr(out->format, sizeof(out->format), m.format);
    fit &= copy_bstr(out->orientation, sizeof(out->orientation), m.orientation);
    fit &= copy_bstr(out->datetime, sizeof(out->datetime), m.datetime);
    fit &= copy_bstr(out->codec, sizeof(out->codec), m.codec);
    fit &= copy_bstr(out->text_snippet, sizeof(out->text_snippet), m.text_snippet);
    out->width = m.width;
    out->height = m.height;
    out->duration_ms = m.duration_ms;
    out->truncated = (m.truncated || !fit) ? 1u : 0u;
    return ret_ok();
}

u32 preview_unreadable_impl(cr_ui_preview* pv, uint64_t* total_bytes, uint32_t* range_count) {
    if (!pv || !total_bytes || !range_count)
        return ret_(ERR_BAD_PARAMETER, "bad argument: pv/total_bytes/range_count");
    auto* p = reinterpret_cast<CrPreview*>(pv);
    *total_bytes = p->reader->unreadable_spans().total_bytes();
    *range_count = u32(p->reader->unreadable_spans().ranges().size());
    return ret_ok();
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_preview_open_snapshot(cr_ui_device* dev, cr_ui_snapshot* snap,
                                                          uint32_t index, cr_ui_preview** out) {
    return guarded(preview_open_snapshot_impl, dev, snap, index, out);
}
extern "C" CR_UI_API uint32_t cr_ui_preview_open_session(cr_ui_device* dev, cr_ui_session* sess,
                                                         uint64_t node_id, cr_ui_preview** out) {
    return guarded(preview_open_session_impl, dev, sess, node_id, out);
}
extern "C" CR_UI_API void cr_ui_preview_close(cr_ui_preview* pv) {
    if (!pv)
        return;
    try {
        delete reinterpret_cast<CrPreview*>(pv);
    } catch (...) {
    }
}
extern "C" CR_UI_API uint32_t cr_ui_preview_size(cr_ui_preview* pv, uint64_t* out) {
    return guarded(preview_size_impl, pv, out);
}
extern "C" CR_UI_API uint32_t cr_ui_preview_read(cr_ui_preview* pv, uint64_t offset, void* buf,
                                                 uint32_t len, uint32_t* placed) {
    return guarded(preview_read_impl, pv, offset, buf, len, placed);
}
extern "C" CR_UI_API uint32_t cr_ui_preview_metadata(cr_ui_preview* pv, cr_ui_metadata* out) {
    return guarded(preview_metadata_impl, pv, out);
}
extern "C" CR_UI_API uint32_t cr_ui_preview_unreadable(cr_ui_preview* pv, uint64_t* total_bytes,
                                                       uint32_t* range_count) {
    return guarded(preview_unreadable_impl, pv, total_bytes, range_count);
}

/* =========================================================================
 * recovery
 * ========================================================================= */

namespace {

u32 recovery_create_impl(cr_ui_device* source, const cr_ui_recovery_config* cfg,
                         cr_ui_recovery** out) {
    if (!source || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: source/out");
    auto* d = reinterpret_cast<CrDevice*>(source);
    auto* r = new CrRecovery();
    r->dev = d;
    recovery::WriterConfig wcfg;
    if (cfg) {
        if (cfg->read_chunk)
            wcfg.read_chunk = cfg->read_chunk;
        if (cfg->rename_attempts)
            wcfg.rename_attempts = cfg->rename_attempts;
        wcfg.verify_hash = cfg->skip_hash_verify == 0;
        wcfg.reconstruct_dirs = cfg->skip_dir_reconstruct == 0;
        switch (cfg->conflict) {
        case 1:
            wcfg.conflict = recovery::ConflictPolicy::Skip;
            break;
        case 2:
            wcfg.conflict = recovery::ConflictPolicy::Overwrite;
            break;
        default:
            wcfg.conflict = recovery::ConflictPolicy::Rename;
            break;
        }
        if (cfg->progress) {
            r->progress = cfg->progress;
            r->progress_user = cfg->progress_user;
        }
    }
#ifdef _WIN32
    r->io.reset(new win::WinDestIo());
#else
    r->io.reset(new FileDestIo());
#endif
    r->writer.reset(new recovery::SafeWriter(*r->io, d->dev.get(), wcfg));
    *out = reinterpret_cast<cr_ui_recovery*>(r);
    return ret_ok();
}

u32 recovery_set_destination_impl(cr_ui_recovery* rec, const char* utf8_path) {
    if (!rec || !utf8_path || !*utf8_path)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/path");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    std::u16string root = u8_to_u16(utf8_path);
    if (!root.empty() && root.back() != u'\\')
        root.push_back(u'\\');
    r->dest_root = root;
    r->have_dest = true;
#ifdef _WIN32
    /* [F12] arming: the destination volume's disk extent feeds the
     * writer's overlap refusal (image-file sources have file-internal
     * extents and cannot be evaluated - documented platform truth) */
    if (utf8_path[0] && utf8_path[1] == ':' && r->dev->disk_known &&
        std::strcmp(r->dev->transport, "image") != 0) {
        u32 ddisk = 0;
        u64 dstart = 0, dlen = 0;
        if (win::volume_extent(wchar_t(utf8_path[0]), &ddisk, &dstart, &dlen))
            r->writer->set_destination(dstart, dlen);
    }
#endif
    return ret_ok();
}

/* the CLI's recursive plan(): dirs enqueue as structure items, files as
 * content items; empty leaves survive because every dir is enqueued */
u32 plan_enqueue_session(CrRecovery* r, tree::FileNode* node, const std::u16string& path,
                         u64* queued) {
    for (auto* c = node->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        const bool is_dir = (c->props()->attributes & 0x10) != 0;
        const u32 eq = r->writer->enqueue(c, path);
        if (eq == OK)
            ++*queued;
        else if (eq != ERR_DEST_OVERLAP)
            return eq; /* overlap is a counted refusal, not a failure */
        if (is_dir) {
            const u32 rc = plan_enqueue_session(r, c, path + c->props()->name + u"\\", queued);
            if (rc != OK)
                return rc;
        }
    }
    return OK;
}

u32 recovery_enqueue_node_impl(cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id) {
    if (!rec || !sess)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/sess");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    auto* s = reinterpret_cast<CrSession*>(sess);
    if (!r->have_dest)
        return ret_(ERR_BAD_PARAMETER, "call cr_ui_recovery_set_destination first");
    tree::FileNode* node = find_node(s->tree, node_id);
    if (!node)
        return ret_(ERR_BAD_PARAMETER, "node id %llu not found in the session tree",
                    static_cast<unsigned long long>(node_id));
    if (!node->props())
        return ret_(ERR_BAD_PARAMETER, "node has no content properties");
    const std::u16string dir = r->dest_root + rel_dir_of(node);
    const u32 eq = r->writer->enqueue(node, dir);
    return ret_(eq, "enqueue %s", err_text(eq)); /* overlap callers count by code */
}

u32 recovery_enqueue_subtree_impl(cr_ui_recovery* rec, cr_ui_session* sess, uint64_t node_id) {
    if (!rec || !sess)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/sess");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    auto* s = reinterpret_cast<CrSession*>(sess);
    if (!r->have_dest)
        return ret_(ERR_BAD_PARAMETER, "call cr_ui_recovery_set_destination first");
    tree::FileNode* node = find_node(s->tree, node_id);
    if (!node)
        return ret_(ERR_BAD_PARAMETER, "node id %llu not found in the session tree",
                    static_cast<unsigned long long>(node_id));
    const std::u16string dir = r->dest_root + rel_dir_of(node);
    /* self-enqueue files AND real directories (the writer turns dir nodes
     * into mkdir items); roots/virtuals are walked only - the CLI's
     * plan() starts at roots and never enqueues them */
    if (node->props() && !node->is_full()) {
        const u32 eq = r->writer->enqueue(node, dir);
        if (eq != OK && eq != ERR_DEST_OVERLAP)
            return ret_(eq, "enqueue %s", err_text(eq));
    }
    /* children of a directory land one level deeper; a root's children
     * land directly at dir (CLI plan() parity); files have none */
    const bool is_dir = node->props() && (node->props()->attributes & 0x10) != 0;
    const std::u16string child_base =
        (!node->is_full() && is_dir) ? dir + node->props()->name + u"\\" : dir;
    u64 queued = 0;
    const u32 rc = plan_enqueue_session(r, node, child_base, &queued);
    if (rc != OK)
        return ret_(rc, "subtree enqueue %s", err_text(rc));
    return ret_ok();
}

u32 recovery_enqueue_snapshot_impl(cr_ui_recovery* rec, cr_ui_snapshot* snap, uint32_t index) {
    if (!rec || !snap)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/snap");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    auto* w = reinterpret_cast<CrSnapshot*>(snap);
    if (!r->have_dest)
        return ret_(ERR_BAD_PARAMETER, "call cr_ui_recovery_set_destination first");
    if (index >= w->snap->node_count())
        return ret_(ERR_BAD_PARAMETER, "node index %u out of range", index);
    const tree::SnapNode& n = w->snap->node(index);
    if (n.full)
        return ret_(ERR_BAD_PARAMETER, "root/virtual nodes are not enqueueable");
    const std::u16string dir = r->dest_root + rel_dir_of_snap(*w->snap, index);
    const u32 eq = r->writer->enqueue(*w->snap, index, dir);
    return ret_(eq, "enqueue %s", err_text(eq));
}

u32 recovery_enqueue_subtree_snapshot_impl(cr_ui_recovery* rec, cr_ui_snapshot* snap,
                                           uint32_t index) {
    if (!rec || !snap)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/snap");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    auto* w = reinterpret_cast<CrSnapshot*>(snap);
    if (!r->have_dest)
        return ret_(ERR_BAD_PARAMETER, "call cr_ui_recovery_set_destination first");
    const tree::Snapshot& s = *w->snap;
    if (index >= s.node_count())
        return ret_(ERR_BAD_PARAMETER, "node index %u out of range", index);
    const tree::SnapNode& n = s.node(index);
    const std::u16string dir = r->dest_root + rel_dir_of_snap(s, index);
    u64 queued = 0;
    /* self-enqueue files AND real directories; roots/virtuals are walked
     * only (CLI plan() parity - it never enqueues the roots) */
    if (!n.full) {
        const u32 eq0 = r->writer->enqueue(s, index, dir);
        if (eq0 != OK && eq0 != ERR_DEST_OVERLAP)
            return ret_(eq0, "enqueue %s", err_text(eq0));
        if (eq0 == OK)
            ++queued;
    }
    /* then the recursive plan() over the snapshot topology: a real
     * directory's children land one level deeper; a root's children land
     * directly at dir; files have none */
    std::function<u32(u32, const std::u16string&)> plan = [&](u32 at,
                                                              const std::u16string& path) -> u32 {
        for (u32 c = s.node(at).first_child; c != CR_UI_SNAP_NONE; c = s.node(c).next_sibling) {
            const tree::SnapNode& cn = s.node(c);
            if (!cn.full && cn.name.empty() && cn.logical_size == 0 && cn.extents.empty() &&
                (cn.attributes & 0x10) == 0)
                continue; /* defensive: the CLI's !props() skip */
            const bool cdir = (cn.attributes & 0x10) != 0;
            const u32 eq = r->writer->enqueue(s, c, path);
            if (eq == OK)
                ++queued;
            else if (eq != ERR_DEST_OVERLAP)
                return eq;
            if (cdir) {
                const u32 rc = plan(c, path + cn.name + u"\\");
                if (rc != OK)
                    return rc;
            }
        }
        return OK;
    };
    {
        const bool is_dir = (n.attributes & 0x10) != 0;
        const std::u16string child_base = (!n.full && is_dir) ? dir + n.name + u"\\" : dir;
        const u32 rc = plan(index, child_base);
        if (rc != OK)
            return ret_(rc, "subtree enqueue %s", err_text(rc));
    }
    return ret_ok();
}

u32 recovery_queue_count_impl(cr_ui_recovery* rec, uint32_t* out) {
    if (!rec || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/out");
    const auto& q = reinterpret_cast<CrRecovery*>(rec)->writer->queue();
    *out = q.size() > 0xFFFFFFFFull ? 0xFFFFFFFFu : u32(q.size());
    return ret_ok();
}

u32 recovery_process_impl(cr_ui_recovery* rec) {
    if (!rec)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    if (!r->have_dest)
        return ret_(ERR_BAD_PARAMETER, "call cr_ui_recovery_set_destination first");
    r->t0 = std::chrono::steady_clock::now();
    r->processed = true;
    const u32 rc = r->writer->process(&r->token, recovery_progress_bridge_, r, r->stats);
    if (rc != OK)
        return ret_(rc, "recovery %s", err_text(rc));
    return ret_ok();
}

u32 recovery_cancel_impl(cr_ui_recovery* rec) {
    if (!rec)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec");
    reinterpret_cast<CrRecovery*>(rec)->token.cancel();
    return ret_ok();
}

u32 recovery_stats_impl(cr_ui_recovery* rec, cr_ui_recovery_stats* out) {
    if (!rec || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/out");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    std::memset(out, 0, sizeof(*out));
    out->queued = r->stats.queued;
    out->done = r->stats.done;
    out->refused = r->stats.refused;
    out->errors = r->stats.errors;
    out->hash_mismatch = r->stats.hash_mismatch;
    out->resumed = r->stats.resumed;
    out->salvaged = r->stats.salvaged;
    out->dirs = r->stats.dirs;
    return ret_ok();
}

u32 recovery_report_json_impl(cr_ui_recovery* rec, const char* source_utf8, char** json_out) {
    if (!rec || !json_out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rec/json_out");
    auto* r = reinterpret_cast<CrRecovery*>(rec);
    const u64 elapsed = r->processed ? u64(std::chrono::duration_cast<std::chrono::milliseconds>(
                                               std::chrono::steady_clock::now() - r->t0)
                                               .count())
                                     : 0;
    const std::string doc = recovery::report::build_report(
        r->writer->report_items(),
        r->writer->report_totals(source_utf8 ? source_utf8 : "", u16_to_u8(r->dest_root), elapsed,
                                 r->stats));
    char* p = dup_str(doc);
    if (!p)
        return ret_(ERR_NO_MEMORY, "report allocation failed");
    *json_out = p;
    return ret_ok();
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_recovery_create(cr_ui_device* source,
                                                    const cr_ui_recovery_config* cfg,
                                                    cr_ui_recovery** out) {
    return guarded(recovery_create_impl, source, cfg, out);
}
extern "C" CR_UI_API void cr_ui_recovery_close(cr_ui_recovery* rec) {
    if (!rec)
        return;
    try {
        delete reinterpret_cast<CrRecovery*>(rec);
    } catch (...) {
    }
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_set_destination(cr_ui_recovery* rec,
                                                             const char* utf8_path) {
    return guarded(recovery_set_destination_impl, rec, utf8_path);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_enqueue_node(cr_ui_recovery* rec, cr_ui_session* sess,
                                                          uint64_t node_id) {
    return guarded(recovery_enqueue_node_impl, rec, sess, node_id);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_enqueue_subtree(cr_ui_recovery* rec,
                                                             cr_ui_session* sess,
                                                             uint64_t node_id) {
    return guarded(recovery_enqueue_subtree_impl, rec, sess, node_id);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_enqueue_snapshot(cr_ui_recovery* rec,
                                                              cr_ui_snapshot* snap,
                                                              uint32_t index) {
    return guarded(recovery_enqueue_snapshot_impl, rec, snap, index);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_enqueue_subtree_snapshot(cr_ui_recovery* rec,
                                                                      cr_ui_snapshot* snap,
                                                                      uint32_t index) {
    return guarded(recovery_enqueue_subtree_snapshot_impl, rec, snap, index);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_queue_count(cr_ui_recovery* rec, uint32_t* out) {
    return guarded(recovery_queue_count_impl, rec, out);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_process(cr_ui_recovery* rec) {
    return guarded(recovery_process_impl, rec);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_cancel(cr_ui_recovery* rec) {
    return guarded(recovery_cancel_impl, rec);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_get_stats(cr_ui_recovery* rec,
                                                       cr_ui_recovery_stats* out) {
    return guarded(recovery_stats_impl, rec, out);
}
extern "C" CR_UI_API uint32_t cr_ui_recovery_report_json(cr_ui_recovery* rec,
                                                         const char* source_utf8, char** json_out) {
    return guarded(recovery_report_json_impl, rec, source_utf8, json_out);
}

/* =========================================================================
 * health (Windows platform layer)
 * ========================================================================= */

namespace {

#ifdef _WIN32

u32 health_read_impl(uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs,
                     uint32_t attr_cap, uint32_t* attr_count) {
    if (!rep || !attr_count)
        return ret_(ERR_BAD_PARAMETER, "bad argument: rep/attr_count");
    std::memset(rep, 0, sizeof(*rep));
    *attr_count = 0;
    wchar_t path[64];
    swprintf_s(path, 64, L"\\\\.\\PhysicalDrive%u", disk);
    HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        const bool elev = win::is_elevated();
        return ret_(ERR_OPEN_FAILED, "cannot open disk %u: %s", disk,
                    elev ? "access denied even elevated (device busy or locked by another "
                           "process)"
                         : "administrator rights required (elevate and retry)");
    }
    win::health::SmartReport sm;
    const u32 src_rc = win::health::read_smart(h, sm);
    if (src_rc == OK && sm.available) {
        rep->available = 1;
        copy_bstr(rep->transport, sizeof(rep->transport), sm.transport);
        rep->ata_version = sm.ata_version;
        rep->smart_version = sm.smart_version;
        rep->failing_now = sm.failing_now ? 1u : 0u;
        rep->reallocated_sectors = sm.reallocated_sectors;
        rep->pending_sectors = sm.pending_sectors;
        rep->uncorrectable = sm.uncorrectable;
        rep->power_on_hours = sm.power_on_hours;
        rep->spin_retry = sm.spin_retry;
        rep->ssd_wear_level = sm.ssd_wear_level;
        rep->temperature_c = sm.temperature_c;
        rep->attribute_count = u32(sm.attributes.size());
        const u32 n = rep->attribute_count < attr_cap ? rep->attribute_count : attr_cap;
        for (u32 i = 0; i < n; ++i) {
            const win::health::SmartAttribute& a = sm.attributes[i];
            attrs[i].id = a.id;
            attrs[i].value = a.value;
            attrs[i].worst = a.worst;
            attrs[i].raw = u64(a.raw[0]) | (u64(a.raw[1]) << 8) | (u64(a.raw[2]) << 16) |
                           (u64(a.raw[3]) << 24) | (u64(a.raw[4]) << 32) | (u64(a.raw[5]) << 40);
            attrs[i].failing = a.failing ? 1u : 0u;
            copy_bstr(attrs[i].name, sizeof(attrs[i].name), a.name);
        }
        *attr_count = rep->attribute_count; /* needed size either way */
    }
    win::health::NvmeHealth nv;
    const u32 nv_rc = win::health::read_nvme_health(h, nv);
    if (nv_rc == OK && nv.available) {
        rep->nvme_available = 1;
        rep->critical_warning = nv.critical_warning;
        rep->temperature_c_x10 =
            u32(nv.temperature_k) * 10 >= 2731 ? u32(nv.temperature_k) * 10 - 2731 : 0;
        rep->available_spare = nv.available_spare;
        rep->available_spare_threshold = nv.available_spare_threshold;
        rep->percentage_used = nv.percentage_used;
        rep->data_units_read = nv.data_units_read;
        rep->data_units_written = nv.data_units_written;
        rep->power_on_hours_nvme = nv.power_on_hours;
        rep->unsafe_shutdowns = nv.unsafe_shutdowns;
        rep->media_errors = nv.media_errors;
    }
    rep->ssd_like = win::health::looks_like_ssd(h) ? 1u : 0u;
    bool trim = false;
    if (win::health::query_trim_state(trim) == OK) {
        rep->trim_known = 1;
        rep->trim_enabled = trim ? 1u : 0u;
    }
    CloseHandle(h);
    if (!rep->available && !rep->nvme_available)
        return ret_(ERR_HEALTH_UNAVAILABLE,
                    "no health source available for disk %u (SMART rc=0x%08x, NVMe rc=0x%08x)",
                    disk, src_rc, nv_rc);
    return ret_ok();
}

#else

u32 health_read_impl(uint32_t disk, cr_ui_smart_report* rep, cr_ui_smart_attr* attrs,
                     uint32_t attr_cap, uint32_t* attr_count) {
    (void)disk;
    (void)rep;
    (void)attrs;
    (void)attr_cap;
    (void)attr_count;
    return ret_(ERR_UNSUPPORTED, "cr_ui_health_read requires the Windows platform layer");
}

#endif

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_health_read(uint32_t disk, cr_ui_smart_report* rep,
                                                cr_ui_smart_attr* attrs, uint32_t attr_cap,
                                                uint32_t* attr_count) {
    return guarded(health_read_impl, disk, rep, attrs, attr_cap, attr_count);
}
