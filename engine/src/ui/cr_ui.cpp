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
#include "cr/cr_log.h"
#include "cr/cr_errmod.h"

#include "cr/apply/cr_apply.h"
#include "cr/orchestrator/cr_scan.h"
#include "cr/carve/cr_signatures.h"
#include "cr/carve/cr_rawsigns.h"
#include "cr/carve/cr_usersigns.h"
#include "cr/tree/cr_diskmap.h"
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
#include "cr/win/cr_elevate.h"
#include "cr/win/cr_enumerate.h"
#include "cr/win/cr_winenv.h" /* win::ENUMERATE_DISK_BOUND: M-030's 200-disk space; MSVC include hygiene */
#include "cr/win/cr_health.h"
#include "cr/win/cr_transport.h"
#include "cr/win/cr_winenv.h" /* M-030: ENUMERATE_DISK_BOUND (the 200-disk grammar bound) */
#include "cr/storage/cr_decrypt_device.h" /* M-033: in-stack XTS wrapper */
#include <windows.h>
#include <psapi.h> /* N-062: PROCESS_MEMORY_COUNTERS/GetProcessMemoryInfo */
#include <share.h> /* _SH_DENYNO (S-008: shared image opens) */
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h> /* W1a (R-56): the *.crsf directory enumeration */
#endif

#include <algorithm>
#include <atomic>
#include <mutex>
#include <cerrno>
#include <chrono>
#include <thread>
#include <cctype>
#include <cstdarg>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <functional>
#include <map>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace cr;

/* M-049: the ABI's CR_ERRMOD_ and CR_ERROP_ constants are pinned to the
 * core's cr_errmod.h attribution set at compile time - the two surfaces
 * must never drift (the ABI header stays self-contained C; this is the
 * executable form of that coupling). */
static_assert(CR_ERRMOD_NONE == cr::errmod::kNone, "ABI/core module id drift");
static_assert(CR_ERRMOD_ORCHESTRATOR == cr::errmod::kOrchestrator, "ABI/core module id drift");
static_assert(CR_ERRMOD_NTFS == cr::errmod::kNtfs, "ABI/core module id drift");
static_assert(CR_ERRMOD_FAT == cr::errmod::kFat, "ABI/core module id drift");
static_assert(CR_ERRMOD_EXFAT == cr::errmod::kExfat, "ABI/core module id drift");
static_assert(CR_ERRMOD_EXT4 == cr::errmod::kExt4, "ABI/core module id drift");
static_assert(CR_ERRMOD_BTRFS == cr::errmod::kBtrfs, "ABI/core module id drift");
static_assert(CR_ERRMOD_HFSPLUS == cr::errmod::kHfsplus, "ABI/core module id drift");
static_assert(CR_ERRMOD_REFS == cr::errmod::kRefs, "ABI/core module id drift");
static_assert(CR_ERRMOD_ZFS == cr::errmod::kZfs, "ABI/core module id drift");
static_assert(CR_ERRMOD_CARVE == cr::errmod::kCarve, "ABI/core module id drift");
static_assert(CR_ERRMOD_LOST == cr::errmod::kLost, "ABI/core module id drift");
static_assert(CR_ERRMOD_LDM == cr::errmod::kLdm, "ABI/core module id drift");
static_assert(CR_ERROP_NONE == cr::errmod::op_none, "ABI/core operation id drift");
static_assert(CR_ERROP_READ == cr::errmod::op_read, "ABI/core operation id drift");

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

/* ---- N-014: atomic checkpoint adapter --------------------------------- */
/* (file-scope so the callbacks below can see it) */
struct CkSink {
    FILE* f = nullptr; /* the STAGING file (<path>.tmp) */
    std::string final_path;
    std::string tmp_path;
};
/* Writes stage into <path>.tmp; every successful finalize() commits via
 * fflush + fclose + rename(tmp -> final) + a fresh staging reopen. A
 * crash mid-emit leaves the FINAL path holding the previous good
 * checkpoint (the og's fresh-file-per-save crash-safety - rsf-format
 * --scan-session.md §5.5: a torn save never destroys the previous good
 * state) and a torn .tmp the next run overwrites. The pre-fix "wb"
 * open on the FINAL path truncated it on every emit - a crash between
 * truncate and finalize destroyed the previous checkpoint wholesale. */
u32 ck_write_cb(void* user, const void* bytes, u32 len) {
    auto* s = static_cast<CkSink*>(user);
    return s->f ? f_write_cb(s->f, bytes, len) : u32(ERR_SESSION_IO);
}
u64 ck_tell_cb(void* user) {
    auto* s = static_cast<CkSink*>(user);
    return s->f ? f_tell_cb(s->f) : u64(0);
}
u32 ck_seek_cb(void* user, u64 absolute) {
    auto* s = static_cast<CkSink*>(user);
    return s->f ? f_seek_cb(s->f, absolute) : u32(ERR_SESSION_IO);
}
u32 ck_commit_cb(void* user) {
    auto* s = static_cast<CkSink*>(user);
    if (!s || !s->f)
        return ERR_SESSION_IO;
    std::fflush(s->f);
    std::fclose(s->f);
    s->f = nullptr;
#ifdef _WIN32
    /* the staging handle is CLOSED: the replace-rename cannot hit a
     * sharing violation (MoveFileExW on an open file needs
     * FILE_SHARE_DELETE, which fopen does not request). */
    if (!MoveFileExW(u8_to_ws(s->tmp_path.c_str()).c_str(), u8_to_ws(s->final_path.c_str()).c_str(),
                     MOVEFILE_REPLACE_EXISTING))
        return ERR_SESSION_IO;
#else
    if (std::rename(s->tmp_path.c_str(), s->final_path.c_str()) != 0)
        return ERR_SESSION_IO;
#endif
    /* staging for the NEXT emit (truncating the just-renamed-away tmp) */
    s->f = open_file(s->tmp_path.c_str(), true);
    return s->f ? OK : ERR_SESSION_IO;
}

/* ---- W1a (R-56): *.crsf directory enumeration (the list input) -------
 * Cross-platform, UTF-8 in/out, byte-sorted by full path (the row order
 * is deterministic - the CLI golden depends on it). FALSE only when the
 * directory itself cannot be read; individual files are never filtered
 * here (the row's error field is the honest per-file report). */
#ifdef _WIN32
bool list_crsf_files(const char* utf8_dir, std::vector<std::string>& out) {
    std::wstring pattern = u8_to_ws(utf8_dir);
    /* accept both "C:\x\y" and "C:\x\y\" - exactly one separator */
    if (!pattern.empty() && pattern.back() == L'\\')
        pattern += L"*.crsf";
    else
        pattern += L"\\*.crsf";
    WIN32_FIND_DATAW fd;
    HANDLE h =
        FindFirstFileExW(pattern.c_str(), FindExInfoBasic, &fd, FindExSearchNameMatch, nullptr, 0);
    if (h == INVALID_HANDLE_VALUE) {
        /* the empty-vs-missing discriminator (runs 40694592245 + 40700058960):
         * the ERROR CODE is NOT the reliable axis - the observed windows-2022
         * matrix came back INVERTED from the documented one (a nonexistent
         * dir reports ERROR_FILE_NOT_FOUND, an existing EMPTY dir reports a
         * different code), so keying on any single code breaks one class or
         * the other. The version-proof truth is the DIRECTORY-EXISTS probe:
         * an existing directory with zero matches is the empty-list SUCCESS
         * (POSIX opendir succeeds on the same dir; the W1c contract pins
         * OK + n==0), while a missing path - or a plain FILE handed as a
         * dir, POSIX ENOTDIR - is the genuine failure (the no-such-dir-*
         * rows in SessionListTwoCallPattern and the abi C-gate pin
         * ERR_OPEN_FAILED). Known divergence, accepted: a directory whose
         * LIST permission is denied but whose attributes read (a rare ACL
         * shape) reports the empty list, where POSIX opendir would fail -
         * the existence probe is the only axis that is right on every
         * version for the two pinned classes. */
        const DWORD attr = GetFileAttributesW(u8_to_ws(utf8_dir).c_str());
        return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
    const std::wstring dir_w = u8_to_ws(utf8_dir);
    /* join with the CALLER's separator convention (run 40689196858): a
     * forward-slash dir (the CMake/ctest path style) must stay
     * forward-slash - a backslash join produced mixed-separator row
     * paths that never strcmp-matched the caller's own path (green on
     * POSIX, red on Windows). Both separators LOAD fine on Windows;
     * comparability is the contract the abi-check pins. */
    std::wstring sep;
    if (!dir_w.empty() && (dir_w.back() == L'/' || dir_w.back() == L'\\'))
        sep = L"";
    else if (dir_w.find(L'/') != std::wstring::npos && dir_w.find(L'\\') == std::wstring::npos)
        sep = L"/";
    else
        sep = L"\\";
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        /* the 8.3 short-name quirk: the pattern can match on the SHORT
         * name - verify the LONG name's tail too (case-insensitive) */
        const std::wstring name(fd.cFileName);
        if (name.size() < 5)
            continue;
        const wchar_t* t = name.c_str() + name.size() - 5;
        if (t[0] != L'.' || towlower(t[1]) != L'c' || towlower(t[2]) != L'r' ||
            towlower(t[3]) != L's' || towlower(t[4]) != L'f')
            continue;
        out.push_back(ws_to_u8(dir_w + sep + name));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(out.begin(), out.end());
    return true;
}
#else
bool list_crsf_files(const char* utf8_dir, std::vector<std::string>& out) {
    DIR* d = opendir(utf8_dir);
    if (!d)
        return false;
    const std::string dir(utf8_dir);
    const std::string sep = (!dir.empty() && dir.back() == '/') ? "" : "/";
    while (dirent* e = readdir(d)) {
        const char* n = e->d_name;
        const size_t len = std::strlen(n);
        if (len < 5)
            continue;
        const char* t = n + len - 5;
        if (t[0] != '.' || tolower(u8(t[1])) != 'c' || tolower(u8(t[2])) != 'r' ||
            tolower(u8(t[3])) != 's' || tolower(u8(t[4])) != 'f')
            continue;
        out.push_back(dir + sep + n);
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return true;
}
#endif
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
    std::unique_ptr<IStorageDevice> dev;        /* the view handed to the core */
    std::unique_ptr<IStorageDevice> hold;       /* parent disk (slices borrow it) */
    std::unique_ptr<IStorageDevice> slice_hold; /* M-033: the slice under a
                                                   decrypting device */
    u32 disk = 0;
    bool disk_known = false;
    u64 volume_len = 0; /* nonzero = a volume slice (not whole-disk) */
    const char* transport = "image";
    /* S-008 pre-scan repair: image devices remember their source so the
     * shim can reopen the SAME bytes read-write for the consented repair. */
    std::string image_path;
    u64 image_offset = 0;
    /* M-033: in-stack XTS decrypt is active on this view (locked volume
     * scanned with a user-supplied FVEK). */
    bool xts_layer = false;
    /* S-029 lost-partition apply: the retained undo log of the last
     * apply executed through this device (undo_last / save_undo).
     * Device-level state: the apply operates on the disk the device
     * represents, not on any scan. */
    repair::UndoLog apply_undo;
    bool have_apply_undo = false;
};

/* read-only, sector-granular file-backed device (the portable source for
 * tests, the ABI check, and image scanning; sub-sector tail bytes are not
 * addressable - a 512-byte-sector device by construction) */
class ImageFileDevice final : public IStorageDevice {
public:
    static std::unique_ptr<ImageFileDevice> open(const char* u8path, u64 offset, u64 bytes,
                                                 u32* err) {
        *err = OK;
#ifdef _WIN32
        /* S-008: _SH_DENYNO, NOT _wfopen_s - the CRT _s variants open
         * exclusive (MS docs: "If you need to share the file, use _fsopen
         * or _wfsopen ... _SH_DENYNO"). The pre-scan repair writer reopens
         * the SAME image read-write while this read view is live. */
        const std::wstring w = u8_to_ws(u8path);
        FILE* f = _wfsopen(w.c_str(), L"rb", _SH_DENYNO);
        if (!f) {
            *err = ERR_OPEN_FAILED;
            return nullptr;
        }
#else
        FILE* f = std::fopen(u8path, "rb");
        if (!f) {
            *err = ERR_OPEN_FAILED;
            return nullptr;
        }
#endif
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

/* S-008: read-write view over an image file, mapped through the image
 * offset - the write side of ImageFileDevice (same bytes, opened "r+b"
 * so nothing is truncated). Sector-granular bounds identical to the read
 * view; repair writes land inside the scanned image only.
 * S-029: the base is apply::IApplyDevice (IS-A IWriteDevice) so the same
 * class serves the repair writer AND the apply writer - image files have
 * no OS disk layout, so the apply device controls keep their portable
 * OK defaults (the sector write IS the commit; the undo log + verified
 * read-back carry the transaction). */
class ImageWriteDevice final : public apply::IApplyDevice {
public:
    static std::unique_ptr<ImageWriteDevice> open(const char* u8path, u64 offset, u64 bytes,
                                                  u32* err) {
        *err = OK;
#ifdef _WIN32
        /* S-008: shared open (_SH_DENYNO) - the read view holds the same
         * file; _wfopen_s would open exclusive and fail here. */
        const std::wstring w = u8_to_ws(u8path);
        FILE* f = _wfsopen(w.c_str(), L"r+b", _SH_DENYNO);
        if (!f) {
            *err = ERR_OPEN_FAILED;
            return nullptr;
        }
#else
        FILE* f = std::fopen(u8path, "r+b");
        if (!f) {
            *err = ERR_OPEN_FAILED;
            return nullptr;
        }
#endif
        const u64 fsize = file_size_of(f);
        if (bytes == 0)
            bytes = fsize > offset ? fsize - offset : 0;
        if (offset > fsize || bytes == 0 || offset + bytes > fsize) {
            std::fclose(f);
            *err = ERR_BAD_PARAMETER;
            return nullptr;
        }
        const u64 total = bytes & ~u64(511);
        if (total == 0) {
            std::fclose(f);
            *err = ERR_BAD_PARAMETER;
            return nullptr;
        }
        return std::unique_ptr<ImageWriteDevice>(new ImageWriteDevice(f, offset, total));
    }
    ~ImageWriteDevice() override {
        if (f_) {
            std::fflush(f_);
            std::fclose(f_);
        }
    }
    u32 read_at(u64 off, void* buf, u32 len) override {
        if (off > total_ || len > total_ - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        if (fseek64(f_, base_ + off) != 0 || std::fread(buf, 1, len, f_) != len)
            return ERR_READ_FAILED;
        return OK;
    }
    u32 write_at(u64 off, const void* buf, u32 len) override {
        if (off > total_ || len > total_ - off)
            return ERR_RANGE_OUT_OF_BOUNDS;
        if (len == 0)
            return OK;
        if (fseek64(f_, base_ + off) != 0 || std::fwrite(buf, 1, len, f_) != len)
            return ERR_WRITE_FAILED;
        return OK;
    }
    u64 size() const override { return total_; }

private:
    ImageWriteDevice(FILE* f, u64 base, u64 total) : f_(f), base_(base), total_(total) {}
    FILE* f_;
    u64 base_, total_;
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
    /* Serializes the ctrl POINTER against the any-thread readers
     * (cr_ui_scan_telemetry / cr_ui_scan_checkpoint_count - documented
     * "any thread, any time") and the two writer sites (fresh-run
     * creation in scan_run_impl, session-resume adoption in
     * session_open_scan - the latter completes before the handle is
     * handed out, so the lock there is for TSan-visible ordering only).
     * Lock order: ctrl_mu never nests with progress_mu; ctrl_mu -> tel_.m
     * only (ScanController::telemetry takes its own lock) - no inversion. */
    std::mutex ctrl_mu;
    orchestrator::ScanStats stats;
    cr_ui_progress_fn progress = nullptr;
    void* progress_user = nullptr;
    /* GAP-S6-10: typed event sink (subscribed pre-run; read once at start) */
    cr_ui_event_fn event_fn = nullptr;
    void* event_user = nullptr;
    std::atomic<bool> running{false};
    bool started = false;  /* run() entered at least once (one-shot) */
    bool has_ctrl = false; /* fresh-run or resumed controller exists */
    bool is_resumed = false;
    bool autosaved = false; /* W1c (R-52): the completion auto-save landed */
    /* S-008 pre-scan repair: the C-ABI consent callback + the lazily
     * opened write view (image: same file r+b; Windows disk/volume: the
     * RW transport mapped through the device slice). */
    cr_ui_repair_consent_fn repair_consent = nullptr;
    void* repair_consent_user = nullptr;
    std::unique_ptr<repair::IWriteDevice> repair_writer;
    u32 resumed_boot_hash = 0;
    std::string ck_path;
    /* N-014: the atomic checkpoint sink - staging file (<path>.tmp) +
     * commit-by-rename adapter (final path never truncated in place). */
    std::unique_ptr<struct CkSink> ck_sink;
    /* progress bridge throttle state. The orchestrator fires the bridge from
     * EVERY carve/pool worker concurrently (engine_progress_bridge_ is the
     * per-thread entry), so the dedup pair and the user callback invocation
     * are serialized under this mutex - the callback stays single-flight,
     * strictly monotonic in overall_permille (stale out-of-order deliveries
     * are dropped), and stage flips at a steady permille still emit. */
    std::mutex progress_mu;
    u32 last_pm = 0xFFFFFFFFu;
    u32 last_stage = 0xFFFFFFFFu;
    /* N-043 heartbeat watchdog: fires CR_EVT_HEARTBEAT from its own
     * thread every 5000 ms while run() is in flight and no
     * CR_EVT_SCAN_PROGRESS arrived in the window - the og's liveness
     * watchdog (DRWUI_decompiled.c:910565-910585, CreateThread
     * :910864), honest variant: no synthetic permille advance, the
     * payload carries the standing telemetry truth. last_progress_at
     * / saw_progress / run_started_at are guarded by heartbeat_mu;
     * heartbeat_stop + the cv shut the thread down at run end (and
     * defensively at close). */
    /* N-062: the memory monitor's threshold (MiB; copied from
     * cr_ui_scan_config.memory_warn_mb at creation - the shim's own
     * copy; the core ScanConfig stays untouched). */
    u32 memory_warn_mb = 0;
    /* N-060: SAVE-SCOPED cancel flag (the og's IUIProgressInfo
     * IsCancelled is the save progress object's own state, NOT the scan
     * flag - rsf-format--scan-session.md flow section). cr_ui_scan_
     * save_cancel sets it; the save adapter polls it per node; the
     * scan's PausableToken is untouched (save-on-scan-cancel intact). */
    std::atomic<bool> save_cancel{false};
    std::thread heartbeat_thread;
    std::mutex heartbeat_mu;
    std::condition_variable heartbeat_cv;
    bool heartbeat_stop = false;
    std::chrono::steady_clock::time_point run_started_at{};
    std::chrono::steady_clock::time_point last_progress_at{};
    bool saw_progress = false;
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
    /* S-009: value-captured session truth (a snapshot of a loaded
     * session carries the error map + the free map + the slice view,
     * so the disk map's snapshot path can paint bad and free cells -
     * mid-scan maps no longer show an empty bad overlay). */
    ReadErrorMap errors;
    storage::FreeMap free_map;
    u64 slice_start = 0;
    u64 slice_bytes = 0;
    bool have_view = false;
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
    /* S-023: the scan-analysis block (copied from the first session the
     * recovery enqueues from; reports stay truthful for standalone
     * device-scope recoveries - absent = the block is omitted). */
    bool have_analysis = false;
    recovery::report::ScanAnalysis analysis;
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

/* GAP-S6-10: core event -> cr_ui_event trampoline (field-for-field; both
 * sides are PODs by contract - ABI-CONTRACT section 6). Fired from engine
 * worker threads exactly like the core sink; no shim-side serialization
 * (documented). */
void scan_event_trampoline_(const orchestrator::event::Event& e, void* user) {
    auto* s = static_cast<CrScan*>(user);
    if (!s || !s->event_fn)
        return;
    cr_ui_event out;
    out.type = u32(e.type);
    out.stage = e.stage;
    out.timestamp_ms = e.timestamp_ms;
    out.bytes_done = e.bytes_done;
    out.bytes_total = e.bytes_total;
    out.stage_permille = e.stage_permille;
    out.overall_permille = e.overall_permille;
    out.files_found = e.files_found;
    out.deleted_found = e.deleted_found;
    out.carved_found = e.carved_found;
    out.err_start_bytes = e.err_start_bytes;
    out.err_len_bytes = e.err_len_bytes;
    out.error_ranges = e.error_ranges;
    /* M-049: the error-context triple (module/operation/retry state) -
     * field-for-field like every other payload member. */
    out.err_module = e.err_module;
    out.err_operation = e.err_operation;
    out.err_attempts = e.err_attempts;
    out.checkpoint_index = e.checkpoint_index;
    out.candidate_lba = e.candidate_lba;
    out.repair_actions = u32(e.repair_actions);
    out.repair_outcome = e.repair_outcome;
    out.check_errors = e.check_errors;
    out.check_family = e.check_family;
    /* N-043: every progress arrival resets the heartbeat watchdog's
     * stall clock (the og watches its engine step counter +0x8fc;
     * CR_EVT_SCAN_PROGRESS is that counter's echo on this surface). */
    if (u32(e.type) == CR_EVT_SCAN_PROGRESS) {
        std::lock_guard<std::mutex> hb(s->heartbeat_mu);
        s->last_progress_at = std::chrono::steady_clock::now();
        s->saw_progress = true;
    }
    s->event_fn(s->event_user, &out);
}

/* N-043: the liveness heartbeat watchdog - the og's 5 s watchdog thread
 * (FUN_140577100, DRWUI_decompiled.c:910565-910585: Sleep(5000), and when
 * the engine step counter +0x8fc lags the expected count +0x8f8, bump the
 * coarse progress +0xe8 and re-fire the progress callback; started via
 * CreateThread at :910864). HONEST VARIANT - the registered divergence
 * this implementation pins: the og fabricates coarse-progress movement
 * during stalls; we fire a dedicated CR_EVT_HEARTBEAT that carries the
 * STANDING telemetry truth (permille never advances without work). A
 * host can therefore distinguish "alive, stalled on I/O" from "hung"
 * without ever being shown fake progress. */
/* N-062: the process working set in MiB (the platform sample the pure
 * cr::log::check_memory_pressure consumes - the core logs no OS headers,
 * so the sample lives here at the shim). 0 on an unreadable source
 * (never above a nonzero threshold - a failed sample never warns). */
static u64 working_set_mb_() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
        return 0;
    return u64(pmc.WorkingSetSize >> 20);
#else
    /* /proc/self/statm field 2 = resident pages (the second long) */
    std::ifstream in("/proc/self/statm");
    unsigned long long total = 0, resident = 0;
    if (!(in >> total >> resident))
        return 0;
    const long page = sysconf(_SC_PAGESIZE);
    return page > 0 ? u64(resident * u64(page)) >> 20 : 0;
#endif
}

void heartbeat_loop_(CrScan* s, orchestrator::ScanController* ctrl) {
    std::unique_lock<std::mutex> lock(s->heartbeat_mu);
    for (;;) {
        s->heartbeat_cv.wait_for(lock, std::chrono::milliseconds(5000));
        if (s->heartbeat_stop)
            return;
        /* N-062: the memory monitor rides every watchdog tick (the og's
         * CMemMonitorManager cadence on the Log.mo sink) - independent of
         * the heartbeat stall state. check_memory_pressure is a no-op
         * when the threshold is 0 (the conservative default: OFF). */
        if (s->memory_warn_mb)
            cr::log::check_memory_pressure(s->memory_warn_mb, working_set_mb_());
        if (!s->running.load(std::memory_order_acquire))
            continue;
        const auto now = std::chrono::steady_clock::now();
        const auto last = s->saw_progress ? s->last_progress_at : s->run_started_at;
        if (now - last < std::chrono::milliseconds(5000))
            continue; /* live progress in the window - no heartbeat */
        lock.unlock();
        /* standing truth only; telemetry takes its own lock (never
         * nested under heartbeat_mu - the trampoline takes heartbeat_mu
         * only after tel_publish_ has released tel_.m) */
        const orchestrator::ScanTelemetrySnapshot t = ctrl->telemetry();
        u32 stage = 0xFFFFFFFFu;
        u32 stage_pm = 0xFFFFFFFFu;
        if (!t.quick.completed) {
            stage = 0;
            stage_pm = t.quick.permille;
        } else if (!t.deep.completed) {
            stage = 1;
            stage_pm = t.deep.permille;
        } else if (!t.carve.completed) {
            stage = 2;
            stage_pm = t.carve.permille;
        }
        cr_ui_event out;
        std::memset(&out, 0, sizeof(out));
        out.type = CR_EVT_HEARTBEAT;
        out.stage = stage;
        out.timestamp_ms = u32(t.elapsed_ms & 0xFFFFFFFFull);
        out.bytes_done = t.bytes_scanned; /* carve-true bytes so far */
        out.bytes_total = 0;              /* unknown - honest, never invented */
        out.stage_permille = stage_pm;
        out.overall_permille = t.overall_permille; /* standing, never bumped */
        out.files_found = t.files_found;
        out.deleted_found = t.deleted_found;
        out.carved_found = t.carved_found;
        out.error_ranges = u32(t.errors.ranges().size());
        if (s->event_fn)
            s->event_fn(s->event_user, &out);
        lock.lock();
    }
}

bool scan_progress_bridge_(u32 overall_pm, void* user) {
    auto* s = static_cast<CrScan*>(user);
    if (!s || !s->progress)
        return true;
    /* Single-flight + monotonic: the orchestrator may invoke this bridge
     * concurrently from every pool worker (each carve slice completion
     * publishes through its own thread), so everything below the lock is
     * serialized. A delivery carrying an overall_permille OLDER than the
     * last emitted one (thread read a snapshot earlier, lost the lock
     * race) is dropped rather than letting the UI see a regression. */
    std::lock_guard<std::mutex> lock(s->progress_mu);
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
    if (s->last_pm != 0xFFFFFFFFu && overall_pm < s->last_pm)
        return true; /* stale out-of-order delivery (sentinel excluded) */
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
        case 11: /* M-044: UDF - classification + carve-only (no engine) */
            out.fs_hint = orchestrator::FsFamily::Udf;
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
    copy_card(out.autosave_dir, sizeof(out.autosave_dir), in ? in->autosave_dir_utf8 : nullptr);
    copy_card(out.volume_model, sizeof(out.volume_model), in ? in->volume_model_utf8 : nullptr);
    copy_card(out.volume_serial, sizeof(out.volume_serial), in ? in->volume_serial_utf8 : nullptr);
    copy_card(out.volume_firmware, sizeof(out.volume_firmware),
              in ? in->volume_firmware_utf8 : nullptr);
    copy_card(out.volume_wwn, sizeof(out.volume_wwn), in ? in->volume_wwn_utf8 : nullptr);
    copy_card(out.volume_interface, sizeof(out.volume_interface),
              in ? in->volume_interface_utf8 : nullptr);
    /* M-029: the raw/carve category root's display name (the og's
     * "filter_generalraw" surface); empty = the default role */
    copy_card(out.raw_category_name, sizeof(out.raw_category_name),
              in ? in->raw_category_name_utf8 : nullptr);
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
    d->image_path = utf8_path; /* S-008: the write side reopens these */
    d->image_offset = offset;
    *out = reinterpret_cast<cr_ui_device*>(d);
    return ret_ok();
}

/* M-030 (diff-to-fix): the transport-open diagnostic. The pre-fix ABI
 * could not distinguish "driver absent" from "driver open failed with
 * X" - the silent RDA->direct fallback overwrote the RDA leg's error
 * (the finding's claim (f)). open_disk_impl records the surviving leg's
 * error here; cr_ui_last_transport_error() reads it. 0 = the last open
 * succeeded (or no open was attempted on this process). */
std::atomic<uint32_t> g_last_transport_error{0};
void set_last_transport_error_(uint32_t e) {
    g_last_transport_error.store(e, std::memory_order_release);
}

#ifdef _WIN32

u32 open_disk_impl(uint32_t disk, cr_ui_device** out) {
    if (!out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: out");
    /* Driver-only (docs/DRIVER-REQUIRED.md; PLAN-DRIVER-ONLY R4/R12): the
     * WinPE gate, the availability probe and the direct fallback are all
     * REMOVED. A non-Ready driver state is a classified refusal - the
     * ERR_DRIVER_* code is the return value, the diagnostic channel
     * (cr_ui_last_transport_error) and the UI presentation contract. */
    win::DriverStatus st;
    if (win::query_driver_status(win::kDefaultDriverService, &st) == OK) {
        const win::DriverState ds = win::driver_state(st);
        if (ds != win::DriverState::Ready) {
            const u32 rc = win::driver_state_error(ds);
            set_last_transport_error_(rc);
            return ret_(rc,
                        "driver prerequisite not met (state %s) - see "
                        "docs/DRIVER-REQUIRED.md",
                        win::driver_state_name(ds));
        }
    }
    u32 err = 0;
    if (auto dev = win::open_rda_partition(win::kDefaultDriverService, disk, 0, &err)) {
        auto* d = new CrDevice();
        d->dev = std::move(dev);
        d->disk = disk;
        d->disk_known = true;
        d->transport = "rda";
        *out = reinterpret_cast<cr_ui_device*>(d);
        set_last_transport_error_(0);
        return ret_ok();
    }
    /* driver Ready but the TARGET would not open: a target error, never a
     * silent transport switch (the pre-driver-only code fell back to
     * \\.\PhysicalDriveN here - CORE-DISK-ACCESS F5/F6). */
    set_last_transport_error_(err);
    return ret_(err ? err : u32(ERR_OPEN_FAILED), "cannot open disk %u through the RDA driver path",
                disk);
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
    /* Driver-only (PLAN-DRIVER-ONLY R5/R6): classified refusal - no probe,
     * no whole-disk direct fallback (CORE-DISK-ACCESS F7/F8). */
    win::DriverStatus st;
    if (win::query_driver_status(win::kDefaultDriverService, &st) == OK) {
        const win::DriverState ds = win::driver_state(st);
        if (ds != win::DriverState::Ready) {
            const u32 rc = win::driver_state_error(ds);
            set_last_transport_error_(rc);
            return ret_(rc,
                        "driver prerequisite not met (state %s) - see "
                        "docs/DRIVER-REQUIRED.md",
                        win::driver_state_name(ds));
        }
    }
    u32 err = 0;
    std::unique_ptr<IStorageDevice> whole =
        win::open_rda_partition(win::kDefaultDriverService, disk, 0, &err);
    if (!whole) {
        set_last_transport_error_(err);
        return ret_(err ? err : u32(ERR_OPEN_FAILED),
                    "cannot open disk %u for volume %c: through the RDA driver path", disk, letter);
    }
    const char* transport = "rda";
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

/* M-033: locked-volume open with a user-supplied FVEK - the in-stack
 * XTS-AES decrypt path (the og's layer: FVE keys -> XTS decrypt below
 * the engines; corpus architecture doc section 8 + the BitLockerLib
 * artifact). utf8_hex_fvek: 64 hex chars (AES-128-XTS) or 128 (256).
 * Without a key the locked-volume refusal (ERR_FVE_LOCKED) stands -
 * this export is the opt-in the og's in-product key entry is. */
u32 open_volume_with_key_impl(char letter, const char* utf8_hex_fvek, cr_ui_device** out) {
    if (!out || !letter || !utf8_hex_fvek || !utf8_hex_fvek[0])
        return ret_(ERR_BAD_PARAMETER, "bad argument: letter/key/out");
    /* decode the hex key (strict) */
    const size_t n = strlen(utf8_hex_fvek);
    if (n != 64 && n != 128)
        return ret_(ERR_BAD_PARAMETER,
                    "FVEK must be 64 hex chars (AES-128-XTS) or 128 (AES-256-XTS)");
    std::vector<u8> fvek(n / 2);
    for (size_t k = 0; k < n; k += 2) {
        int b = 0;
        for (int j = 0; j < 2; j++) {
            const int c = utf8_hex_fvek[k + j];
            int v;
            if (c >= '0' && c <= '9')
                v = c - '0';
            else if (c >= 'a' && c <= 'f')
                v = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                v = c - 'A' + 10;
            else
                return ret_(ERR_BAD_PARAMETER, "FVEK: non-hex character at %zu", k);
            b = (b << 4) | v;
        }
        fvek[k / 2] = u8(b);
    }
    const win::bitlocker::Status bst = win::bitlocker::status(wchar_t(letter));
    if (bst == win::bitlocker::Status::Clear)
        return ret_(ERR_BAD_PARAMETER, "%c: volume is not BitLocker-encrypted", letter);
    if (bst == win::bitlocker::Status::Locked &&
        win::bitlocker::try_auto_unlock(wchar_t(letter)) == OK) {
        /* auto-unlock succeeded: the OS decrypts - the plain path */
        return open_volume_impl(letter, out);
    }
    /* Locked or Unknown (locked volumes deny boot reads) with a user key:
     * scan the raw extent through the in-stack decrypt device. */
    u32 disk = 0;
    u64 start = 0, len = 0;
    if (!win::volume_extent(wchar_t(letter), &disk, &start, &len))
        return ret_(ERR_OPEN_FAILED, "cannot map %c: to a disk extent", letter);
    /* Driver-only (PLAN-DRIVER-ONLY R5/R6): classified refusal - no probe,
     * no whole-disk direct fallback (CORE-DISK-ACCESS F7/F8). */
    win::DriverStatus st;
    if (win::query_driver_status(win::kDefaultDriverService, &st) == OK) {
        const win::DriverState ds = win::driver_state(st);
        if (ds != win::DriverState::Ready) {
            const u32 rc = win::driver_state_error(ds);
            set_last_transport_error_(rc);
            return ret_(rc,
                        "driver prerequisite not met (state %s) - see "
                        "docs/DRIVER-REQUIRED.md",
                        win::driver_state_name(ds));
        }
    }
    u32 err = 0;
    std::unique_ptr<IStorageDevice> whole =
        win::open_rda_partition(win::kDefaultDriverService, disk, 0, &err);
    if (!whole) {
        set_last_transport_error_(err);
        return ret_(err ? err : u32(ERR_OPEN_FAILED),
                    "cannot open disk %u for volume %c: through the RDA driver path", disk, letter);
    }
    const char* transport = "rda";
    auto slice = PartitionSlice::create(whole.get(), start, len, nullptr);
    if (!slice)
        return ret_(ERR_OPEN_FAILED, "slice creation failed for %c: (@%llu +%llu)", letter,
                    static_cast<unsigned long long>(start), static_cast<unsigned long long>(len));
    auto dec = DecryptingDevice::create(slice.get(), fvek.data(), fvek.size());
    if (!dec)
        return ret_(ERR_BAD_PARAMETER, "FVEK rejected (XTS-weak or malformed key material)");
    auto* d = new CrDevice();
    d->hold = std::move(whole);
    d->slice_hold = std::move(slice);
    d->dev = std::move(dec);
    d->disk = disk;
    d->disk_known = true;
    d->volume_len = len;
    d->transport = transport;
    d->xts_layer = true;
    *out = reinterpret_cast<cr_ui_device*>(d);
    return ret_ok();
}

u32 unlock_volume_with_dictionary_impl(char letter, const char* const* utf8_user_passwords,
                                       uint32_t count) {
    /* N-064: the og's Dict flow as the host's explicit consent action. */
    std::vector<std::wstring> wide;
    wide.reserve(count);
    for (uint32_t k = 0; k < count; ++k) {
        if (!utf8_user_passwords[k])
            continue;
        /* The dictionary grammar is ASCII (the win layer enforces the same
         * rule): any non-ASCII candidate is refused, not silently mapped. */
        std::wstring w;
        bool ascii = true;
        for (const char* p = utf8_user_passwords[k]; *p != '\0'; ++p) {
            if (static_cast<unsigned char>(*p) >= 0x80) {
                ascii = false;
                break;
            }
            w.push_back(wchar_t(static_cast<unsigned char>(*p)));
        }
        if (!ascii)
            return ret_(ERR_BAD_PARAMETER, "dictionary candidate %u is not ASCII", k);
        wide.push_back(std::move(w));
    }
    std::vector<const wchar_t*> ptrs;
    ptrs.reserve(wide.size());
    for (const std::wstring& w : wide)
        ptrs.push_back(w.c_str());
    const u32 rc =
        win::bitlocker::unlock_with_dictionary(wchar_t(letter), ptrs.data(), ptrs.size());
    if (rc == OK)
        return ret_ok();
    return ret_(rc, "dictionary unlock exhausted (%u + %u candidates)", unsigned(228),
                unsigned(ptrs.size()));
}

/* The driver-status surface (docs/DRIVER-REQUIRED.md section 3; ABI-since
 * 2.x, additive): fills every facet from the platform query and maps the
 * classified state to the C enum. The QUERY itself never fails on a driver
 * condition - it reports it (out->state carries the class); only a null
 * argument or a query-machinery failure returns non-zero. */
u32 driver_status_impl(cr_ui_driver_status_t* out) {
    if (!out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: out");
    std::memset(out, 0, sizeof(*out));
    win::DriverStatus st;
    const u32 qrc = win::query_driver_status(win::kDefaultDriverService, &st);
    if (qrc != OK)
        return ret_(qrc, "driver status query failed");
    const win::DriverState ds = win::driver_state(st);
    switch (ds) {
    case win::DriverState::Ready:
        out->state = CR_UI_DRIVER_STATE_READY;
        break;
    case win::DriverState::Absent:
        out->state = CR_UI_DRIVER_STATE_ABSENT;
        break;
    case win::DriverState::NotStarted:
        out->state = CR_UI_DRIVER_STATE_NOT_STARTED;
        break;
    case win::DriverState::Unauthorized:
        out->state = CR_UI_DRIVER_STATE_UNAUTHORIZED;
        break;
    case win::DriverState::VersionMismatch:
        out->state = CR_UI_DRIVER_STATE_VERSION_MISMATCH;
        break;
    case win::DriverState::Errored:
    default:
        out->state = CR_UI_DRIVER_STATE_ERRORED;
        break;
    }
    out->service_present = st.service_present ? 1u : 0u;
    out->service_running = st.service_running ? 1u : 0u;
    out->device_openable = st.device_openable ? 1u : 0u;
    out->authorized = st.authorized ? 1u : 0u;
    out->info_valid = st.info_valid ? 1u : 0u;
    out->major = st.major;
    out->minor = st.minor;
    out->required_major = win::kDriverWireMajorRequired;
    out->flags = st.flags;
    out->max_disks = st.max_disks;
    out->ioctl_base = st.ioctl_base;
    out->last_gle = st.last_gle;
    return ret_ok();
}

#else /* !_WIN32 */

u32 open_disk_impl(uint32_t disk, cr_ui_device** out) {
    (void)disk;
    (void)out;
    /* M-030: no transport leg runs on the platform stub - the advisory
     * reads clean (0) for the attempt (nothing recorded, nothing lost).
     */
    set_last_transport_error_(0);
    return ret_(ERR_UNSUPPORTED, "cr_ui_open_disk requires the Windows platform layer");
}

u32 open_volume_impl(char letter, cr_ui_device** out) {
    (void)letter;
    (void)out;
    return ret_(ERR_UNSUPPORTED, "cr_ui_open_volume requires the Windows platform layer");
}

u32 open_volume_with_key_impl(char letter, const char* utf8_hex_fvek, cr_ui_device** out) {
    (void)letter;
    (void)utf8_hex_fvek;
    (void)out;
    return ret_(ERR_UNSUPPORTED, "cr_ui_open_volume_with_key requires the Windows platform layer");
}

u32 unlock_volume_with_dictionary_impl(char letter, const char* const* utf8_user_passwords,
                                       uint32_t count) {
    (void)letter;
    (void)utf8_user_passwords;
    (void)count;
    return ret_(ERR_UNSUPPORTED,
                "cr_ui_unlock_volume_with_dictionary requires the Windows platform layer");
}

u32 driver_status_impl(cr_ui_driver_status_t* out) {
    if (!out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: out");
    /* No platform layer on this lane: the surface is honestly unsupported
     * (the classifier itself is platform-independent and unit-pinned in
     * tests/unit/test_driver_status.cpp; the Windows runner proves the real
     * query - PLAN-DRIVER-ONLY section 7). */
    return ret_(ERR_UNSUPPORTED, "cr_ui_driver_status requires the Windows platform layer");
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
extern "C" CR_UI_API uint32_t cr_ui_last_transport_error(void) {
    /* M-030: the transport-open diagnostic read side. The value is the
     * surviving transport leg's error code from the LAST cr_ui_open_disk
     * call on this process (0 = the last open succeeded / none yet).
     * Advisory only: never gates a return code, never clears state - the
     * open calls' own return codes stay authoritative. */
    return g_last_transport_error.load(std::memory_order_acquire);
}
extern "C" CR_UI_API uint32_t cr_ui_open_volume(char letter, cr_ui_device** out) {
    return guarded(open_volume_impl, letter, out);
}
extern "C" CR_UI_API uint32_t cr_ui_open_volume_with_key(char letter, const char* utf8_hex_fvek,
                                                         cr_ui_device** out) {
    return guarded(open_volume_with_key_impl, letter, utf8_hex_fvek, out);
}
extern "C" CR_UI_API uint32_t cr_ui_driver_status(cr_ui_driver_status_t* out) {
    return guarded(driver_status_impl, out);
}
extern "C" CR_UI_API uint32_t cr_ui_unlock_volume_with_dictionary(
    char letter, const char* const* utf8_user_passwords, uint32_t count) {
    return guarded(unlock_volume_with_dictionary_impl, letter, utf8_user_passwords, count);
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
            else if (d.table.layout == table::TableLayout::Ebr)
                /* N-057: APPENDED code 3 (0=none/1=MBR/2=GPT frozen);
                 * the extended-chain layout is a first-class og
                 * classification (MOM Partition.md) */
                o.layout = 3;
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

/* ---- S-008 pre-scan repair bridges (CrScan* as hook user) ---- */
bool ui_repair_consent(const repair::RepairPlan& plan, void* user) {
    auto* s = static_cast<CrScan*>(user);
    if (!s || !s->repair_consent)
        return false; /* default-deny: never execute without a callback */
    cr_ui_repair_plan p;
    std::memset(&p, 0, sizeof(p));
    p.detected = plan.detected ? 1u : 0u;
    copy_bstr(p.fs_family, sizeof(p.fs_family), plan.fs_family.c_str());
    p.action_count = u32(plan.actions.size());
    p.refused = plan.refused ? 1u : 0u;
    const std::string& reason =
        plan.refused ? plan.refusal_reason
                     : (plan.actions.empty() ? plan.refusal_reason : plan.actions[0].reason);
    copy_bstr(p.reason, sizeof(p.reason), reason.c_str());
    p.advisory_count = u32(plan.advisories.size());
    return s->repair_consent(s->repair_consent_user, &p) != 0;
}

/* (A-002: the former UiVolumeWriteSlice is gone - the orchestrator's
 * repair::rebase_plan is the single offset-basis mechanism, so the UI's
 * writer is always the parent-basis device.) */

repair::IWriteDevice* ui_repair_open_writer(void* user) {
    auto* s = static_cast<CrScan*>(user);
    if (!s || !s->dev)
        return nullptr;
    CrDevice* d = s->dev;
    /* A-002: the writer is ALWAYS a parent-basis device (the whole image
     * file / the whole RW disk); the orchestrator rebases the
     * slice-relative plan onto that basis before execute (repair::
     * rebase_plan). Mapping here too would double the offset. */
    if (!d->image_path.empty()) {
        u32 err = OK;
        auto w = ImageWriteDevice::open(d->image_path.c_str(), 0, 0, &err);
        if (!w)
            return nullptr;
        s->repair_writer = std::move(w);
        return s->repair_writer.get();
    }
#ifdef _WIN32
    if (d->disk_known) {
        u32 err = OK;
        /* driver-only (PLAN-DRIVER-ONLY R9): the direct RW retry is gone */
        auto w = win::open_disk_rw(d->disk, win::kDefaultDriverService, &err);
        if (!w)
            return nullptr;
        s->repair_writer = std::move(w);
        return s->repair_writer.get();
    }
#endif
    return nullptr; /* unknown source: repair unavailable, never guessed */
}

void ui_repair_close_writer(repair::IWriteDevice* /*dev*/, void* user) {
    static_cast<CrScan*>(user)->repair_writer.reset();
}

void arm_repair_hooks(CrScan* s, const cr_ui_scan_config* cfg) {
    if (!cfg || !cfg->pre_scan_repair)
        return;
    s->cfg.pre_scan_repair = true;
    s->repair_consent = cfg->repair_consent;
    s->repair_consent_user = cfg->repair_consent_user;
    s->cfg.repair.consent = ui_repair_consent;
    s->cfg.repair.open_writer = ui_repair_open_writer;
    s->cfg.repair.close_writer = ui_repair_close_writer;
    s->cfg.repair.user = s;
}

u32 scan_create_impl(const cr_ui_scan_config* cfg, cr_ui_scan** out) {
    if (!out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: out");
    auto* s = new CrScan();
    const u32 rc = fill_scan_config(cfg, s->cfg);
    if (rc != OK) {
        delete s;
        return rc; /* fill_scan_config already set last_error */
    }
    arm_repair_hooks(s, cfg); /* S-008: pre-scan repair consent gate */
    if (cfg)
        s->memory_warn_mb = cfg->memory_warn_mb; /* N-062 */
    if (cfg && cfg->pre_scan_check)              /* M-041: the damage-audit phase */
        s->cfg.pre_scan_check = true;
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
    /* N-066: the record-3 axes from the (N-080-fixed) enumeration. */
    const std::string fw = ws_to_u8(disks[disk].firmware);
    const std::string wwn = ws_to_u8(disks[disk].wwn);
    const std::string ifc = ws_to_u8(disks[disk].interface_desc);
    copy_card(cfg.volume_firmware, sizeof(cfg.volume_firmware), fw.c_str());
    copy_card(cfg.volume_wwn, sizeof(cfg.volume_wwn), wwn.c_str());
    copy_card(cfg.volume_interface, sizeof(cfg.volume_interface), ifc.c_str());
}

/* cross-disk LDM resolver (the cmd_scan block, verbatim semantics) */
void arm_ldm_resolver(CrScan* s) {
    if (!s->cfg.whole_disk || !s->dev->disk_known || s->dev->volume_len != 0)
        return;
    table::LdmInfo probe;
    if (table::parse_ldm_database(s->dev->dev.get(), probe) != OK || !probe.found)
        return;
    /* M-030: the LDM resolver enumerates the og's full 200-disk space
     * (the driver CREATE grammar accepts disk# <= 199 - eprdtdrv_
     * decompiled.c :184; the pin lives in cr_winenv.h). The pre-fix 32
     * cap was exactly wrong here: cross-disk LDM resolution is the
     * multi-disk path where hosts beyond 32 disks matter most. */
    win::PhysicalDiskList list;
    if (win::enumerate_disks(win::ENUMERATE_DISK_BOUND, list) != OK)
        return;
    auto guid_map = std::make_shared<std::map<table::LdmGuid, IStorageDevice*>>();
    for (const auto& pd : list) {
        u32 oerr = 0;
        /* driver-only (PLAN-DRIVER-ONLY R8): the direct retry is gone - a
         * member that will not open through RDA simply does not resolve */
        auto dev = win::open_disk(pd.number, win::kDefaultDriverService, &oerr);
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

/* W1c (R-52): defined below (after the N-060 adapter block); the
 * completion auto-save in scan_run_impl calls it. */
u32 write_session_atomic(CrScan* s, const std::string& final_path,
                         session::SerializeProgress progress, void* progress_user);

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

    /* checkpoint wiring (CLI: --out / --resume in-place). N-014: the
     * sink stages into <path>.tmp and commits by rename - the final path
     * is never truncated in place. */
    if (!s->ck_path.empty() && !s->ck_sink) {
        s->ck_sink.reset(new CkSink);
        s->ck_sink->final_path = s->ck_path;
        s->ck_sink->tmp_path = s->ck_path + ".tmp";
        s->ck_sink->f = open_file(s->ck_sink->tmp_path.c_str(), true);
        if (!s->ck_sink->f)
            return ret_(ERR_SESSION_IO, "cannot create checkpoint file %s",
                        s->ck_sink->tmp_path.c_str());
        s->cfg.checkpoint.write = ck_write_cb;
        s->cfg.checkpoint.tell = ck_tell_cb;
        s->cfg.checkpoint.seek = ck_seek_cb;
        s->cfg.checkpoint.commit = ck_commit_cb;
        s->cfg.checkpoint.user = s->ck_sink.get();
    }

#ifdef _WIN32
    arm_ldm_resolver(s);
#endif

    {
        std::lock_guard<std::mutex> lock(s->ctrl_mu);
        if (!s->ctrl) {
            s->ctrl.reset(new orchestrator::ScanController(s->tree, s->cfg));
            s->has_ctrl = true;
        }
    }
    s->started = true;
    {
        std::lock_guard<std::mutex> lock(s->heartbeat_mu);
        s->run_started_at = std::chrono::steady_clock::now();
        s->saw_progress = false;
        s->heartbeat_stop = false;
    }
    s->running.store(true);
    /* N-043/N-062: the liveness watchdog (when events are subscribed -
     * a silent host cannot observe a heartbeat - OR the memory monitor
     * is armed; N-062's monitor rides the same 5 s cadence through the
     * cr::log seam, which a silent host can still consume). ctrl is
     * stable for the whole run; the raw pointer is captured under
     * ctrl_mu. */
    if (s->event_fn || s->memory_warn_mb) {
        std::lock_guard<std::mutex> lock(s->ctrl_mu);
        if (s->ctrl)
            s->heartbeat_thread = std::thread(heartbeat_loop_, s, s->ctrl.get());
    }
    const u32 rc = s->ctrl->run(s->dev->dev.get(), &s->token, scan_progress_bridge_, s, s->stats);
    {
        std::lock_guard<std::mutex> lock(s->heartbeat_mu);
        s->heartbeat_stop = true;
    }
    s->heartbeat_cv.notify_all();
    if (s->heartbeat_thread.joinable())
        s->heartbeat_thread.join();
    s->running.store(false);
    if (s->ck_sink) {
        if (s->ck_sink->f)
            std::fclose(s->ck_sink->f);
            /* the staging file is transient: the final path holds the last
             * COMMITTED checkpoint; a leftover .tmp is a torn never-committed
             * emit (N-014's crash window) - remove it, never trust it. */
#ifdef _WIN32
        DeleteFileW(u8_to_ws(s->ck_sink->tmp_path.c_str()).c_str());
#else
        std::remove(s->ck_sink->tmp_path.c_str());
#endif
        s->ck_sink.reset();
        s->cfg.checkpoint = orchestrator::ScanConfig::CheckpointSink{};
    }
    if (rc != OK)
        return ret_(rc, "scan %s", err_text(rc));
    /* W1c (R-52): the completion auto-save - the og's SaveScan window
     * (rsf-format section 5.5 + UIUX/30:64-67). ONLY a completed run
     * auto-saves (a cancelled run never does - the og's flow is the
     * completion window, and save-on-cancel is the explicit --out/
     * cr_ui_scan_save_session surface). Silent write, ONE outcome event
     * (never silent failure), the stats flag. The name is the N-061
     * corpus convention: auto_<label>_<M>_<D>_<H>_<m>_<s> with the
     * name-forbidden-char sanitizer. */
    if (s->cfg.autosave_dir[0] != 0) {
        const std::string dir(s->cfg.autosave_dir);
        const std::string sep = (!dir.empty() && (dir.back() == '/' || dir.back() == '\\')) ? "" :
#ifdef _WIN32
                                                                                            "\\";
#else
                                                                                            "/";
#endif
        const std::string name =
            session::auto_session_name(u8_to_u16(s->cfg.volume_label)) + ".crsf";
        const u32 arc = write_session_atomic(s, dir + sep + name, nullptr, nullptr);
        s->autosaved = (arc == OK);
        if (s->event_fn) {
            cr_ui_event out;
            std::memset(&out, 0, sizeof(out));
            out.type = CR_EVT_AUTOSAVED;
            out.stage = 0xFFFFFFFFu;
            out.autosave_rc = arc;
            s->event_fn(s->event_user, &out);
        }
    }
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

/* N-075: the og's reference-out query signals as synchronous ABI ops.
 * busy reads the shim's run-lifecycle atomic (set in scan_run_impl
 * before the controller starts, cleared after the heartbeat join);
 * stopping reads the LIVE token request — a locked read on
 * PausableToken, safe from any thread, and true from cr_ui_scan_cancel
 * until run() returns (it stays true after the run ends: the token is
 * one-shot by construction, which is the honest "a cancel was
 * requested on this scan handle" answer the og's isStopping also gives
 * for its one-shot stop event). */
u32 scan_busy_impl(cr_ui_scan* scan, uint32_t* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    *out = reinterpret_cast<CrScan*>(scan)->running.load(std::memory_order_acquire) ? 1u : 0u;
    return ret_ok();
}

u32 scan_stopping_impl(cr_ui_scan* scan, uint32_t* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    *out = reinterpret_cast<CrScan*>(scan)->token.cancelled() ? 1u : 0u;
    return ret_ok();
}

u32 scan_set_event_fn_impl(cr_ui_scan* scan, cr_ui_event_fn fn, void* user) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    auto* s = reinterpret_cast<CrScan*>(scan);
    if (s->running.load())
        return ret_(ERR_BAD_PARAMETER, "scan is running (subscribe before run)");
    s->event_fn = fn;
    s->event_user = user;
    /* fresh path: the config seed reaches the controller at construction
     * (run() builds it from s->cfg); resumed path: the controller already
     * exists - set its sink directly (pre-run only, enforced above) */
    s->cfg.event_fn = fn ? scan_event_trampoline_ : nullptr;
    s->cfg.event_user = fn ? static_cast<void*>(s) : nullptr;
    if (s->has_ctrl) {
        std::lock_guard<std::mutex> lock(s->ctrl_mu);
        if (s->ctrl) {
            const u32 rc = s->ctrl->set_event_sink(fn ? scan_event_trampoline_ : nullptr,
                                                   fn ? static_cast<void*>(s) : nullptr);
            if (rc != OK)
                return ret_(rc, "set_event_sink: %s", err_text(rc));
        }
    }
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
    std::lock_guard<std::mutex> lock(s->ctrl_mu);
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
    /* S-008 pre-scan repair outcome */
    out->repair_ran = st.repair_ran ? 1u : 0u;
    out->repair_planned = st.repair_actions_planned;
    out->repair_applied = st.repair_actions_applied;
    out->repair_denied = (st.repair_consent_denied || st.repair_writer_unavailable) ? 1u : 0u;
    /* M-041 damage-audit report summary */
    out->check_ran = st.check_ran ? 1u : 0u;
    out->check_errors = u32(st.check_report.errors.size());
    out->check_detected = st.check_detected ? 1u : 0u;
    /* read-truth accounting (the fake-scan fix): the coverage + probe
     * blindness flags - the host's "these regions were NOT scanned"
     * presentation surface */
    out->bytes_unreadable = st.bytes_unreadable;
    out->probe_unreadable = st.probe_unreadable ? 1u : 0u;
    out->autosaved = s->autosaved ? 1u : 0u; /* W1c (R-52): the completion
                                              * auto-save landed (live runs) */
    return ret_ok();
}

/* S-023: the scan-completion grading statistics (post-scan pass). */
u32 scan_assistant_impl(cr_ui_scan* scan, cr_ui_assistant_stats* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    const tree::grading::AssistantStats& a = s->stats.assistant;
    std::memset(out, 0, sizeof(*out));
    out->total_file_count = a.total_file_count;
    out->raw_count = a.raw_count;
    out->lost_partition_exist_file_count = a.lost_partition_exist_file_count;
    out->lost_partition_delete_file_count = a.lost_partition_delete_file_count;
    out->quick_scan_exist_file_count = a.quick_scan_exist_file_count;
    out->quick_scan_delete_file_count = a.quick_scan_delete_file_count;
    out->deep_scan_file_count = a.deep_scan_file_count;
    out->quick_scan_delete_extend = a.quick_scan_delete_extend;
    for (size_t i = 0; i < 8; ++i)
        out->category_counts[i] = a.category_counts[i];
    out->matched = a.matched;
    out->total = a.total;
    out->small_result = a.small_result ? 1u : 0u;
    out->graded_count = a.graded_count;
    return ret_ok();
}

/* S-008: persist the pre-scan repair undo log (durable container). */
u32 scan_repair_save_undo_impl(cr_ui_scan* scan, const char* utf8_path) {
    if (!scan || !utf8_path)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/path");
    auto* s = reinterpret_cast<CrScan*>(scan);
    if (!s->started)
        return ret_(ERR_BAD_PARAMETER, "scan has not run");
    if (s->stats.repair_actions_applied == 0 || s->stats.repair_undo.entries.empty())
        return ret_(ERR_BAD_PARAMETER, "no repair was applied (no undo log)");
    FILE* f = open_file(utf8_path, true);
    if (!f)
        return ret_(ERR_OPEN_FAILED, "cannot create undo log %s", utf8_path);
    std::vector<u8> blob;
    s->stats.repair_undo.serialize(blob);
    const size_t n = std::fwrite(blob.data(), 1, blob.size(), f);
    std::fclose(f);
    if (n != blob.size())
        return ret_(ERR_SESSION_IO, "short write on %s", utf8_path);
    return ret_ok();
}

/* =========================================================================
 * S-029: lost-partition APPLY (consent-gated, transactional)
 * ========================================================================= */
namespace {

/* the apply consent context: the C-ABI callback + user pair handed to
 * cr_ui_apply_lost_partition (ui_apply_consent bridges core->POD). */
struct UiApplyConsentCtx {
    cr_ui_apply_consent_fn fn = nullptr;
    void* user = nullptr;
};

bool ui_apply_consent(const apply::ApplyPlan& plan, void* user) {
    auto* ctx = static_cast<UiApplyConsentCtx*>(user);
    if (!ctx || !ctx->fn)
        return false; /* default-deny: never execute without a callback */
    cr_ui_apply_plan p;
    std::memset(&p, 0, sizeof(p));
    p.refused = plan.refused ? 1u : 0u;
    p.scope = plan.scope == apply::ApplyScope::RebuildTable ? 1u : 0u;
    p.start_lba = plan.start_lba;
    p.sector_count = plan.sector_count;
    p.mbr_type = plan.mbr_type;
    p.slot = plan.entry_index;
    p.regenerate_signature = plan.regenerate_signature ? 1u : 0u;
    p.signature = plan.mbr_signature;
    copy_bstr(p.reason, sizeof(p.reason), plan.refused ? plan.refusal_reason : std::string());
    return ctx->fn(ctx->user, &p) != 0;
}

/* opens the apply write view for the device: image files reopen r+b at
 * offset 0 (the whole image IS the disk); Windows disks open the RW
 * transport with the corpus IOCTL chain. NULL = no write view (the
 * caller surfaces refused-with-reason, never a guessed write). */
std::unique_ptr<apply::IApplyDevice> ui_apply_open_writer(CrDevice* d) {
    if (!d->image_path.empty()) {
        if (d->image_offset != 0)
            return nullptr; /* a slice view is not the disk */
        u32 err = OK;
        return ImageWriteDevice::open(d->image_path.c_str(), 0, 0, &err);
    }
#ifdef _WIN32
    if (d->disk_known) {
        u32 err = OK;
        /* driver-only (PLAN-DRIVER-ONLY R9): the direct apply-view retry is
         * gone; the S-029 device-control chain issues through the RDA bound
         * handle (the driver passes non-session IOCTLs through) */
        return win::open_disk_apply_rw(d->disk, win::kDefaultDriverService, &err);
    }
#endif
    return nullptr;
}

void fill_apply_result(cr_ui_apply_result* out, u32 applied, u32 outcome, u64 undo_entries,
                       const char* reason) {
    std::memset(out, 0, sizeof(*out));
    out->applied = applied;
    out->outcome = outcome;
    out->undo_entries = undo_entries;
    copy_bstr(out->reason, sizeof(out->reason), reason ? reason : "");
}

} // namespace

u32 apply_lost_partition_impl(cr_ui_device* dev, const cr_ui_apply_candidate* cand,
                              uint32_t rebuild_table, cr_ui_apply_consent_fn consent,
                              void* consent_user, cr_ui_apply_result* out) {
    if (!dev || !cand || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/cand/out");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    std::memset(out, 0, sizeof(*out));

    /* the table lives at the disk's LBA 0: only whole-disk views apply
     * (volume slices AND image views opened at a nonzero offset) */
    if (d->volume_len != 0 || (!d->image_path.empty() && d->image_offset != 0)) {
        fill_apply_result(out, 0, 2, 0,
                          "volume slices cannot apply (the table lives at disk LBA 0)");
        return ret_(ERR_APPLY_REFUSED, "apply refused: device is a volume slice");
    }
    if (cand->fs > u32(lost::CandidateFs::BitLocker)) {
        fill_apply_result(out, 0, 2, 0, "unknown candidate family");
        return ret_(ERR_APPLY_REFUSED, "apply refused: bad candidate fs %u", cand->fs);
    }

    lost::Candidate c;
    c.start_lba = cand->start_lba;
    c.sector_count = cand->sector_count;
    c.fs = lost::CandidateFs(cand->fs);

    /* plan (READ-ONLY) against the live table of the same view */
    table::DiskTable tbl;
    if (u32 trc = table::parse_disk_table(d->dev.get(), tbl))
        return ret_(trc, "apply refused: cannot read the partition table (%s)", err_text(trc));
    apply::ApplyPlan plan;
    const u32 prc = apply::plan_apply(d->dev.get(), c, plan, tbl.partitions,
                                      rebuild_table ? apply::ApplyScope::RebuildTable
                                                    : apply::ApplyScope::SingleEntry);
    if (plan.refused || prc != OK) {
        fill_apply_result(out, 0, 2, 0, plan.refusal_reason.c_str());
        return ret_(ERR_APPLY_REFUSED, "apply refused: %s", plan.refusal_reason.c_str());
    }

    /* the write view (consent may still deny; nothing opened is written) */
    auto wdev = ui_apply_open_writer(d);
    if (!wdev) {
        fill_apply_result(out, 0, 2, 0, "no write view available for this device");
        return ret_(ERR_APPLY_REFUSED,
                    "apply refused: no write view (open the disk/image for writing)");
    }

    UiApplyConsentCtx ctx{consent, consent_user};
    repair::UndoLog undo;
    const u32 rc = apply::execute_apply(wdev.get(), plan, ui_apply_consent, &ctx, undo);

    /* retain the undo log on the device (undo_last / save_undo) */
    d->apply_undo = undo;
    d->have_apply_undo = !undo.entries.empty();

    if (rc == OK) {
        fill_apply_result(out, 1, 0, undo.entries.size(), nullptr);
        return ret_ok();
    }
    switch (rc) {
    case ERR_APPLY_CONSENT_DENIED:
        fill_apply_result(out, 0, 1, undo.entries.size(), "consent denied (nothing written)");
        return ret_(rc, "apply consent denied");
    case ERR_APPLY_STALE:
        fill_apply_result(out, 0, 3, undo.entries.size(),
                          "the partition table changed since the plan");
        return ret_(rc, "apply refused: the table changed since the plan");
    case ERR_VOLUME_LOCK_FAILED:
        fill_apply_result(out, 0, 2, undo.entries.size(),
                          "volume busy (quiesce refused - nothing written)");
        return ret_(rc, "apply refused: volume busy");
    case ERR_APPLY_REFRESH_FAILED:
        fill_apply_result(out, 1, 4, undo.entries.size(),
                          "applied but the disk-properties refresh failed (undo available)");
        return ret_(rc, "applied but refresh failed (undo available)");
    default:
        fill_apply_result(out, 0, 5, undo.entries.size(), err_text(rc));
        return ret_(rc, "apply failed: %s", err_text(rc));
    }
}

u32 apply_undo_last_impl(cr_ui_device* dev, cr_ui_apply_result* out) {
    if (!dev || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/out");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    std::memset(out, 0, sizeof(*out));
    if (!d->have_apply_undo || d->apply_undo.entries.empty()) {
        fill_apply_result(out, 0, 2, 0, "no apply undo log on this device");
        return ret_(ERR_BAD_PARAMETER, "no apply undo log on this device");
    }
    auto wdev = ui_apply_open_writer(d);
    if (!wdev) {
        fill_apply_result(out, 0, 5, d->apply_undo.entries.size(), "no write view available");
        return ret_(ERR_APPLY_REFUSED, "undo refused: no write view");
    }
    const u32 rc = apply::undo_apply(*wdev, d->apply_undo);
    if (rc == OK) {
        fill_apply_result(out, 1, 0, d->apply_undo.entries.size(), nullptr);
        return ret_ok();
    }
    if (rc == ERR_APPLY_REFRESH_FAILED) {
        fill_apply_result(out, 1, 4, d->apply_undo.entries.size(),
                          "restored but the disk-properties refresh failed");
        return ret_(rc, "undo restored but refresh failed");
    }
    fill_apply_result(out, 0, 5, d->apply_undo.entries.size(), err_text(rc));
    return ret_(rc, "undo failed: %s", err_text(rc));
}

u32 apply_save_undo_impl(cr_ui_device* dev, const char* utf8_path) {
    if (!dev || !utf8_path)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dev/path");
    auto* d = reinterpret_cast<CrDevice*>(dev);
    if (!d->have_apply_undo || d->apply_undo.entries.empty())
        return ret_(ERR_BAD_PARAMETER, "no apply undo log on this device");
    FILE* f = open_file(utf8_path, true);
    if (!f)
        return ret_(ERR_OPEN_FAILED, "cannot create undo log %s", utf8_path);
    std::vector<u8> blob;
    d->apply_undo.serialize(blob);
    const size_t n = std::fwrite(blob.data(), 1, blob.size(), f);
    std::fclose(f);
    if (n != blob.size())
        return ret_(ERR_SESSION_IO, "short write on %s", utf8_path);
    return ret_ok();
}

u32 scan_checkpoint_count_impl(cr_ui_scan* scan, uint64_t* out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    std::lock_guard<std::mutex> lock(s->ctrl_mu);
    *out = s->ctrl ? s->ctrl->checkpoint_count() : 0;
    return ret_ok();
}

/* N-060: the save-path progress/cancel adapter. The og's save rides the
 * scan's cancellation flag with per-child IsCancelled teardown and
 * reports progress inside the save window (rsf-format--scan-session.md
 * flow section). The core hook is per-node; this adapter (a) consults
 * the SAME token the scan used - cr_ui_scan_cancel during the save is
 * the save-cancel - and (b) coalesces CR_EVT_SAVE_PROGRESS events
 * (first tick, permille steps, terminal - the event cadence the scan
 * progress bridge uses, never per node). */
struct SaveProgressState {
    CrScan* s = nullptr;
    u32 last_pm = 0xFFFFFFFFu;
    bool first = true;
};
bool save_progress_adapter_(u64 done, u64 total, void* user) {
    auto* st = static_cast<SaveProgressState*>(user);
    CrScan* s = st->s;
    /* per-child IsCancelled equivalent: the SAVE-SCOPED flag is checked
     * on EVERY tick, events or not (a silent host can still cancel the
     * save); the scan token is deliberately NOT consulted - the og's
     * save-cancel is the progress object's own state, so a scan
     * cancelled mid-run can still persist its partial session */
    if (s->save_cancel.load(std::memory_order_relaxed))
        return false;
    if (!s->event_fn)
        return true;
    const u32 pm = total ? u32(done * 1000 / total) : 1000;
    if (st->first || pm != st->last_pm || done == total) {
        st->first = false;
        st->last_pm = pm;
        cr_ui_event out;
        std::memset(&out, 0, sizeof(out));
        out.type = CR_EVT_SAVE_PROGRESS;
        out.stage = 0xFFFFFFFFu;
        out.stage_permille = pm;     /* the save's own 0..1000 */
        out.overall_permille = 1000; /* standing - the scan is complete */
        out.bytes_done = done;       /* nodes serialized */
        out.bytes_total = total;     /* total nodes */
        s->event_fn(s->event_user, &out);
    }
    return true;
}

/* W1c (R-52): the shared atomic session write - tmp + finalize + rename
 * (the N-014 discipline), used by the explicit N-060 save (progress
 * adapter wired) and the completion auto-save (silent). A cancel or
 * failure leaves the previous good final file and removes the staging
 * .tmp (never trust a torn emit). */
u32 write_session_atomic(CrScan* s, const std::string& final_path,
                         session::SerializeProgress progress, void* progress_user) {
    const std::string tmp_path = final_path + ".tmp";
    FILE* f = open_file(tmp_path.c_str(), true);
    if (!f)
        return ERR_SESSION_IO;
    session::Writer w(f_write_cb, f_tell_cb, f_seek_cb, f);
    w.set_flags(session::CRSF_FLAG_ABS_EXTENTS); /* F-1: absolute basis */
    u32 rc = w.open(0x52434F56);
    if (rc == OK)
        rc = s->ctrl->save_session(w, s->stats, progress, progress_user);
    if (rc == OK)
        rc = w.finalize();
    std::fclose(f);
    if (rc == OK) {
        std::error_code ec;
        std::filesystem::rename(tmp_path, final_path, ec);
        if (ec)
            rc = ERR_SESSION_IO;
    }
    if (rc != OK) {
#ifdef _WIN32
        DeleteFileW(u8_to_ws(tmp_path.c_str()).c_str());
#else
        std::remove(tmp_path.c_str());
#endif
    }
    return rc;
}

u32 scan_save_session_impl(cr_ui_scan* scan, const char* utf8_path) {
    if (!scan || !utf8_path || !*utf8_path)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/path");
    auto* s = reinterpret_cast<CrScan*>(scan);
    if (!s->has_ctrl)
        return ret_(ERR_BAD_PARAMETER, "scan has not run (nothing to save)");
    /* N-060: atomic temp-then-rename (the N-014 checkpoint discipline;
     * the og's crash-safety rule - a torn save never destroys the
     * previous good session file). The final path is only ever replaced
     * by a fully finalized container; a cancel or failure leaves the
     * previous good file and removes the staging .tmp. */
    s->save_cancel.store(false, std::memory_order_relaxed); /* fresh save */
    SaveProgressState spst;
    spst.s = s;
    const u32 rc = write_session_atomic(s, std::string(utf8_path), save_progress_adapter_, &spst);
    if (rc == OK)
        return ret_ok();
    return ret_(rc, "session save failed (%s)", err_text(rc));
}

u32 scan_save_cancel_impl(cr_ui_scan* scan) {
    if (!scan)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan");
    reinterpret_cast<CrScan*>(scan)->save_cancel.store(true, std::memory_order_relaxed);
    return ret_ok();
}

u32 scan_snapshot_impl(cr_ui_scan* scan, cr_ui_snapshot** out) {
    if (!scan || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: scan/out");
    auto* s = reinterpret_cast<CrScan*>(scan);
    const bool running = s->running.load();
    auto snap = tree::Snapshot::capture(s->tree, running);
    if (!snap)
        return ret_(ERR_NO_MEMORY, "snapshot allocation failed");
    auto* w = new CrSnapshot();
    w->snap = std::move(snap);
    /* S-009: the mid-scan snapshot carries the LIVE error map (the og's
     * AnalysisGridWidget primary use - UIUX/04:110 - paints bad cells
     * while scanning). Telemetry's copy is taken under the controller
     * lock (race-free); the free map is captured only when the scan is
     * NOT running (the engine pass fills it mid-run - reading it live
     * would be racy, and an absent map paints unknown, never a guess). */
    {
        std::lock_guard<std::mutex> lock(s->ctrl_mu);
        if (s->ctrl) {
            w->errors = s->ctrl->telemetry().errors;
            if (!running)
                w->free_map = s->stats.free_map;
        }
    }
    if (s->dev != nullptr && s->dev->dev != nullptr) {
        w->slice_start = s->dev->dev->absolute_base();
        w->slice_bytes = s->dev->dev->size();
        w->have_view = w->slice_bytes != 0;
    }
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
        if (s->ck_sink) {
            if (s->ck_sink->f)
                std::fclose(s->ck_sink->f);
                /* N-014: the staging file is transient (a torn never-committed
                 * emit); the final path holds the last committed checkpoint. */
#ifdef _WIN32
            DeleteFileW(u8_to_ws(s->ck_sink->tmp_path.c_str()).c_str());
#else
            std::remove(s->ck_sink->tmp_path.c_str());
#endif
        }
        /* N-043: defensively stop + join the heartbeat watchdog - a
         * joinable std::thread member would std::terminate the process
         * at deletion (close while run() is in flight is a host bug,
         * but the failure mode must not be termination) */
        {
            std::lock_guard<std::mutex> lock(s->heartbeat_mu);
            s->heartbeat_stop = true;
        }
        s->heartbeat_cv.notify_all();
        if (s->heartbeat_thread.joinable())
            s->heartbeat_thread.join();
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
extern "C" CR_UI_API uint32_t cr_ui_scan_set_event_fn(cr_ui_scan* scan, cr_ui_event_fn fn,
                                                      void* user) {
    return guarded(scan_set_event_fn_impl, scan, fn, user);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_cancel(cr_ui_scan* scan) {
    return guarded(scan_cancel_impl, scan);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_busy(cr_ui_scan* scan, uint32_t* out) {
    return guarded(scan_busy_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_stopping(cr_ui_scan* scan, uint32_t* out) {
    return guarded(scan_stopping_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_telemetry(cr_ui_scan* scan, cr_ui_telemetry* out) {
    return guarded(scan_telemetry_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_get_stats(cr_ui_scan* scan, cr_ui_scan_stats* out) {
    return guarded(scan_stats_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_assistant(cr_ui_scan* scan, cr_ui_assistant_stats* out) {
    return guarded(scan_assistant_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_checkpoint_count(cr_ui_scan* scan, uint64_t* out) {
    return guarded(scan_checkpoint_count_impl, scan, out);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_repair_save_undo(cr_ui_scan* scan, const char* utf8_path) {
    return guarded(scan_repair_save_undo_impl, scan, utf8_path);
}

extern "C" CR_UI_API uint32_t cr_ui_apply_lost_partition(
    cr_ui_device* dev, const cr_ui_apply_candidate* cand, uint32_t rebuild_table,
    cr_ui_apply_consent_fn consent, void* consent_user, cr_ui_apply_result* out) {
    return guarded(apply_lost_partition_impl, dev, cand, rebuild_table, consent, consent_user, out);
}

extern "C" CR_UI_API uint32_t cr_ui_apply_undo_last(cr_ui_device* dev, cr_ui_apply_result* out) {
    return guarded(apply_undo_last_impl, dev, out);
}

extern "C" CR_UI_API uint32_t cr_ui_apply_save_undo(cr_ui_device* dev, const char* utf8_path) {
    return guarded(apply_save_undo_impl, dev, utf8_path);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_save_session(cr_ui_scan* scan, const char* utf8_path) {
    return guarded(scan_save_session_impl, scan, utf8_path);
}
extern "C" CR_UI_API uint32_t cr_ui_scan_save_cancel(cr_ui_scan* scan) {
    return guarded(scan_save_cancel_impl, scan);
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

/* W1b (R-54): the load-path progress/cancel adapter - the N-060
 * save_progress_adapter_ mirror. The og's restore reports progress inside
 * the load window and polls IsCancelled after EVERY child
 * (rsf-format--scan-session.md flow section). (a) the cancel check rides
 * the CALLER-OWNED flag - polled on every node, events or not (a silent
 * host can still cancel, same rule as the save side); (b) events coalesce
 * (first tick, permille steps, terminal - the save event cadence). */
struct LoadProgressState {
    cr_ui_event_fn fn = nullptr;
    void* user = nullptr;
    volatile uint32_t* cancel = nullptr;
    u32 last_pm = 0xFFFFFFFFu;
    bool first = true;
};
bool load_progress_adapter_(u64 done, u64 total, void* user) {
    auto* st = static_cast<LoadProgressState*>(user);
    /* per-child IsCancelled equivalent: the caller-owned flag, polled on
     * EVERY node (any thread may set it; zero-init safe by contract) */
    if (st->cancel && *st->cancel != 0)
        return false;
    if (!st->fn)
        return true;
    const u32 pm = total ? u32(done * 1000 / total) : 1000;
    if (st->first || pm != st->last_pm || done == total) {
        st->first = false;
        st->last_pm = pm;
        cr_ui_event out;
        std::memset(&out, 0, sizeof(out));
        out.type = CR_EVT_SESSION_LOAD_PROGRESS;
        out.stage = 0xFFFFFFFFu;
        out.stage_permille = pm;            /* the load's own 0..1000 */
        out.overall_permille = 0xFFFFFFFFu; /* no scan context */
        out.bytes_done = done;              /* nodes deserialized */
        out.bytes_total = total;            /* persisted node count */
        st->fn(st->user, &out);
    }
    return true;
}

u32 session_load_with_events_impl(const char* utf8_path, cr_ui_event_fn fn, void* user,
                                  volatile uint32_t* cancel_flag, cr_ui_session** out) {
    if (!utf8_path || !*utf8_path || !out)
        return ret_(ERR_BAD_PARAMETER, "bad argument: path/out");
    FILE* f = open_file(utf8_path, false);
    if (!f)
        return ret_(ERR_OPEN_FAILED, "cannot open session %s", utf8_path);
    auto* sess = new CrSession();
    sess->path = utf8_path;
    session::Reader r(f_read_cb, f_seek_cb, f_size_cb, f);
    LoadProgressState lp;
    lp.fn = fn;
    lp.user = user;
    lp.cancel = cancel_flag;
    const u32 rc = orchestrator::ScanController::load_session_summary(
        r, sess->tree, sess->stats, sess->info, nullptr, load_progress_adapter_, &lp);
    std::fclose(f);
    if (rc != OK) {
        delete sess; /* the partial tree is discarded - a torn load never
                      * half-claims state (W1b contract) */
        return ret_(rc, "session load failed (%s)", err_text(rc));
    }
    *out = reinterpret_cast<cr_ui_session*>(sess);
    return ret_ok();
}

u32 session_list_impl(const char* utf8_dir, cr_ui_session_row* rows, uint32_t* count) {
    if (!utf8_dir || !*utf8_dir || !count)
        return ret_(ERR_BAD_PARAMETER, "bad argument: dir/count");
    std::vector<std::string> files;
    if (!list_crsf_files(utf8_dir, files))
        return ret_(ERR_OPEN_FAILED, "cannot enumerate session dir %s", utf8_dir);
    const u32 needed = u32(files.size());
    if (!rows) {
        *count = needed;
        return ret_ok();
    }
    const u32 cap = *count;
    const u32 n = needed < cap ? needed : cap;
    for (u32 i = 0; i < n; ++i) {
        cr_ui_session_row& row = rows[i];
        std::memset(&row, 0, sizeof(row));
        /* the path itself may need truncation (a 1024-byte row field is a
         * display convenience, not a limit on what loads - callers hand
         * the full path they enumerated themselves to session_load) */
        const bool path_truncated = files[i].size() >= sizeof(row.path);
        copy_cstr(row.path, sizeof(row.path), files[i].c_str());
        FILE* f = open_file(files[i].c_str(), false);
        if (!f) {
            row.valid = 0;
            row.error = u32(ERR_OPEN_FAILED);
            continue;
        }
        session::Reader r(f_read_cb, f_seek_cb, f_size_cb, f);
        session::SessionSummary s;
        const u32 rc = orchestrator::ScanController::read_summary(r, s);
        std::fclose(f);
        if (rc != OK) {
            row.valid = 0;
            row.error = rc;
            continue;
        }
        row.valid = 1;
        row.error = path_truncated ? u32(ERR_TRUNCATED) : u32(OK);
        row.fs_family = s.have_volume ? s.volume.fs_kind : 0;
        row.stage_done_mask = s.state.stage_done_mask;
        row.configured_mask = s.configured_mask;
        row.files_found = s.state.files_found;
        row.deleted_found = s.state.deleted_found;
        row.carved_found = s.state.carved_found;
        row.bytes_scanned = s.state.bytes_scanned;
        row.elapsed_ms = s.volume.elapsed_ms;
        row.save_unix = s.volume.save_unix;
        copy_cstr(row.label, sizeof(row.label), s.volume.label);
    }
    *count = n;
    return ret_(n == needed ? OK : u32(ERR_TRUNCATED), "%s", "");
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
    /* read-truth (the fake-scan fix): the session record persists the error
     * map (not the derived fields) - recompute the coverage from the
     * restored map so a loaded session tells the same truth a live run
     * would. probe_unreadable is 0 for loaded sessions (the flag is
     * live-probe metadata, honestly absent - the map's coverage at offset 0
     * remains visible through error_ranges/bytes_unreadable). */
    out->bytes_unreadable = st.errors.total_bytes();
    return ret_ok();
}

/* S-023: per-selection re-grade on the stable session tree. */
u32 session_regrade_impl(cr_ui_session* sess, const uint64_t* node_ids, uint32_t count,
                         uint32_t* regraded) {
    if (!sess || !regraded)
        return ret_(ERR_BAD_PARAMETER, "bad argument: sess/regraded");
    if (count != 0 && !node_ids)
        return ret_(ERR_BAD_PARAMETER, "bad argument: ids");
    auto* s = reinterpret_cast<CrSession*>(sess);
    if (!s->stats.assistant.graded_count) {
        /* no grading pass ever ran (pre-S-023 session or never scanned):
         * honest refusal - grades would be fabricated */
        return ret_(ERR_BAD_PARAMETER, "session carries no grading pass");
    }
    tree::grading::GradeContext gctx; /* per-node fs_kind is the truth source */
    tree::grading::DeviceSlot slot;
    gctx.devices.push_back(slot);
    u32 n = 0;
    std::vector<tree::FileNode*> stack;
    for (tree::FileNode* r : s->tree.roots())
        stack.push_back(r);
    while (!stack.empty()) {
        tree::FileNode* nd = stack.back();
        stack.pop_back();
        for (tree::FileNode* c = nd->first_child(); c; c = c->next_sibling())
            stack.push_back(c);
        if (nd->is_full() || !nd->props())
            continue;
        for (uint32_t i = 0; i < count; ++i) {
            if (nd->node_id() == node_ids[i]) {
                if (tree::grading::regrade_node(nd, gctx))
                    ++n;
                break;
            }
        }
    }
    *regraded = n;
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
    /* S-009: carry the session's map truth by value (race-free reads) */
    w->errors = s->stats.errors;
    w->free_map = s->stats.free_map;
    w->slice_start = s->info.slice_offset;
    w->slice_bytes = s->info.slice_bytes;
    w->have_view = s->info.slice_bytes != 0;
    *out = reinterpret_cast<cr_ui_snapshot*>(w);
    return ret_ok();
}

/* GAP-S6-11: the disk-map data producer over a session or a snapshot.
 * Session path: the scanned view + carve completion + error map come from
 * the loaded session (truth on disk). Snapshot path: the view geometry
 * MUST come from `want` (snapshots are value objects). */
u32 disk_map_impl(cr_ui_session* sess, cr_ui_snapshot* snap, const cr_ui_disk_map_geometry* want,
                  cr_ui_disk_map_geometry* got, uint8_t** allocation, uint8_t** coverage,
                  uint8_t** found, uint8_t** confidence, uint64_t* bad_ranges, uint32_t bad_cap,
                  uint32_t* bad_count) {
    if ((sess == nullptr) == (snap == nullptr))
        return ret_(ERR_BAD_PARAMETER, "exactly one of sess/snap must be non-null");
    if (!got)
        return ret_(ERR_BAD_PARAMETER, "bad argument: got");
    if (!allocation && !coverage && !found && !confidence)
        return ret_(ERR_BAD_PARAMETER, "at least one overlay is required");
    if (bad_count == nullptr)
        return ret_(ERR_BAD_PARAMETER, "bad argument: bad_count");

    viz::DiskMapInputs in;
    std::unique_ptr<tree::Snapshot> owned_snap;
    const tree::Snapshot* snap_view = nullptr;
    ReadErrorMap errors;

    if (sess != nullptr) {
        auto* s = reinterpret_cast<CrSession*>(sess);
        in.slice_start = s->info.slice_offset;
        in.slice_bytes = s->info.slice_bytes;
        in.fs_family = s->info.fs_family;
        /* S-009: the session's free-map record is the cluster hint when
         * present (its granularity is the map's own cell size); sessions
         * saved before the record keep the 64 KiB default. */
        in.cluster_bytes = s->stats.free_map.valid() ? s->stats.free_map.cluster_bytes : 0;
        in.carve_completed = (s->info.stage_done_mask & orchestrator::STAGE_CARVE) != 0;
        errors = s->stats.errors; /* copy: builder reads it without locks */
        in.errors = &errors;
        in.free_map = s->stats.free_map.valid() ? &s->stats.free_map : nullptr;
        owned_snap = tree::Snapshot::capture(s->tree, false);
        if (!owned_snap)
            return ret_(ERR_NO_MEMORY, "snapshot capture failed");
        snap_view = owned_snap.get();
        /* caller override of the mapped view (zoom into a range) */
        if (want && want->total_bytes != 0) {
            if (want->start_byte < in.slice_start ||
                want->start_byte + want->total_bytes > in.slice_start + in.slice_bytes)
                return ret_(ERR_RANGE_OUT_OF_BOUNDS, "want view is outside the session slice");
            in.slice_start = want->start_byte;
            in.slice_bytes = want->total_bytes;
        }
    } else {
        auto* w = reinterpret_cast<CrSnapshot*>(snap);
        snap_view = w->snap.get();
        if (!want || want->total_bytes == 0)
            return ret_(ERR_BAD_PARAMETER,
                        "snapshots carry no view: want->start_byte/total_bytes are required");
        /* S-009: the captured session truth feeds the snapshot path too
         * (errors + free map from the snapshot's owning session); the
         * view comes from `want` bounded by the captured slice. */
        if (w->have_view &&
            (want->start_byte < w->slice_start ||
             want->start_byte + want->total_bytes > w->slice_start + w->slice_bytes))
            return ret_(ERR_RANGE_OUT_OF_BOUNDS, "want view is outside the session slice");
        errors = w->errors;
        in.errors = w->errors.ranges().empty() ? nullptr : &errors;
        in.free_map = w->free_map.valid() ? &w->free_map : nullptr;
        in.slice_start = want->start_byte;
        in.slice_bytes = want->total_bytes;
        in.cluster_bytes = w->free_map.valid() ? w->free_map.cluster_bytes : want->cluster_bytes;
        in.carve_completed = false; /* mid-scan value view: extent coverage */
    }

    std::vector<u8> a, cv, f, cf;
    std::vector<std::pair<u64, u64>> bad;
    viz::DiskMapGeometry g;
    const u32 rc = viz::build_disk_map(in, *snap_view, g, allocation ? &a : nullptr,
                                       coverage ? &cv : nullptr, found ? &f : nullptr,
                                       confidence ? &cf : nullptr, bad, bad_cap, bad_count);
    if (rc != OK && rc != ERR_TRUNCATED)
        return ret_(rc, "disk map: %s", err_text(rc));
    *got = cr_ui_disk_map_geometry{};
    got->start_byte = g.start_byte;
    got->total_bytes = g.total_bytes;
    got->cell_bytes = g.cell_bytes;
    got->cell_count = g.cell_count;
    got->cluster_bytes = g.cluster_bytes;
    got->fs_family = g.fs_family;
    auto move_out = [](std::vector<u8>& v, uint8_t** out) -> bool {
        if (out == nullptr)
            return true;
        if (v.empty()) {
            *out = nullptr;
            return true;
        }
        uint8_t* p = static_cast<uint8_t*>(std::malloc(v.size()));
        if (p == nullptr)
            return false;
        std::memcpy(p, v.data(), v.size());
        *out = p;
        return true;
    };
    if (!move_out(a, allocation) || !move_out(cv, coverage) || !move_out(f, found) ||
        !move_out(cf, confidence))
        return ret_(ERR_NO_MEMORY, "overlay allocation failed");
    if (bad_ranges && bad_cap > 0) {
        const u32 pairs = u32(bad.size()) < bad_cap ? u32(bad.size()) : bad_cap;
        for (u32 i = 0; i < pairs; ++i) {
            bad_ranges[u64(i) * 2] = bad[i].first;
            bad_ranges[u64(i) * 2 + 1] = bad[i].second;
        }
    }
    return rc == OK ? ret_ok() : ret_(rc, "bad_ranges truncated (%u pairs needed)", *bad_count);
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
    arm_repair_hooks(scan, original_cfg); /* S-008: resumed scans keep the consent gate */
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
extern "C" CR_UI_API uint32_t cr_ui_session_load_with_events(const char* utf8_path,
                                                             cr_ui_event_fn fn, void* user,
                                                             volatile uint32_t* cancel_flag,
                                                             cr_ui_session** out) {
    return guarded(session_load_with_events_impl, utf8_path, fn, user, cancel_flag, out);
}
extern "C" CR_UI_API uint32_t cr_ui_disk_map(cr_ui_session* sess, cr_ui_snapshot* snap,
                                             const cr_ui_disk_map_geometry* want,
                                             cr_ui_disk_map_geometry* got, uint8_t** allocation,
                                             uint8_t** coverage, uint8_t** found,
                                             uint8_t** confidence, uint64_t* bad_ranges,
                                             uint32_t bad_cap, uint32_t* bad_count) {
    return guarded(disk_map_impl, sess, snap, want, got, allocation, coverage, found, confidence,
                   bad_ranges, bad_cap, bad_count);
}

extern "C" CR_UI_API void cr_ui_disk_map_free(uint8_t* overlay) {
    std::free(overlay);
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
extern "C" CR_UI_API uint32_t cr_ui_session_regrade(cr_ui_session* sess, const uint64_t* node_ids,
                                                    uint32_t count, uint32_t* regraded) {
    return guarded(session_regrade_impl, sess, node_ids, count, regraded);
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
extern "C" CR_UI_API uint32_t cr_ui_session_list(const char* utf8_dir, cr_ui_session_row* rows,
                                                 uint32_t* count) {
    return guarded(session_list_impl, utf8_dir, rows, count);
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
    out->grade = n.grade;
    out->overlap = n.overlap ? 1u : 0u;
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
    cq.filter.category_mask = q->category_mask ? q->category_mask : 0x1FFu;
    cq.filter.size_min = q->size_min;
    cq.filter.size_max = q->size_max ? q->size_max : ~u64(0);
    cq.filter.modified_from = q->modified_from;
    cq.filter.modified_to = q->modified_to ? q->modified_to : ~u64(0);
    cq.filter.confidence_min = q->confidence_min;
    for (uint32_t i = 0; i < q->extension_count; ++i)
        cq.filter.extensions.push_back(q->extensions[i] ? q->extensions[i] : "");
    if (q->name_substring)
        cq.filter.name_substring = q->name_substring;
    if (q->path_prefix) /* ABI-since core-api: the og path filter group */
        cq.filter.path_prefix = q->path_prefix;
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
    case 6: /* S-023: grade sort (the og's comparator puts grade-2 first) */
        cq.sort = query::SortKey::Grade;
        break;
    case 7: /* M-029: the og's Type column (iconIndex category order,
             * then extension, then name) */
        cq.sort = query::SortKey::Type;
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
        arr[i].grade = r.node ? u32(r.node->grade()) : 1u;
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

namespace {

u32 query_apply_buckets_impl(cr_ui_query* q, uint32_t date_bucket, uint32_t size_bucket,
                             uint64_t date_custom_from, uint64_t date_custom_to,
                             uint64_t size_custom_min, uint64_t size_custom_max) {
    if (!q)
        return ret_(ERR_BAD_PARAMETER, "bad argument: q");
    /* seed a core filter from the query's current ranges (the same
     * sentinel convention query_run_impl reads: 0 = unbounded) */
    query::Filter f;
    f.size_min = q->size_min;
    f.size_max = q->size_max ? q->size_max : ~u64(0);
    f.modified_from = q->modified_from;
    f.modified_to = q->modified_to ? q->modified_to : ~u64(0);
    /* the host clock as FILETIME (the og derives its cached day bounds at
     * scan-controller init; the query-time derivation is the same
     * day-truncated-local math) */
    const u64 now_ft = u64(std::time(nullptr) + 11'644'473'600) * 10'000'000;
    if (!query::apply_date_bucket(f, u16(date_bucket), now_ft, date_custom_from, date_custom_to)) {
        /* the skipped Date IDs (0x104) and unknown values: no invented
         * semantics - reject with the exact bad ID in the diagnostic */
        return ret_(ERR_BAD_PARAMETER, "date_bucket 0x%x is not an og filter-bar bucket ID",
                    date_bucket);
    }
    if (!query::apply_size_bucket(f, u16(size_bucket), size_custom_min, size_custom_max)) {
        return ret_(ERR_BAD_PARAMETER, "size_bucket 0x%x is not an og filter-bar bucket ID",
                    size_bucket);
    }
    /* write back ONLY the axes a bucket actually replaced: the None
     * buckets (and any rejection above) leave the ABI query's fields
     * byte-identical, sentinel convention included */
    if (date_bucket != 0) {
        q->modified_from = f.modified_from;
        q->modified_to = f.modified_to;
    }
    if (size_bucket != 0) {
        q->size_min = f.size_min;
        q->size_max = f.size_max;
    }
    return ret_ok();
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_query_apply_buckets(
    cr_ui_query* q, uint32_t date_bucket, uint32_t size_bucket, uint64_t date_custom_from,
    uint64_t date_custom_to, uint64_t size_custom_min, uint64_t size_custom_max) {
    return guarded(query_apply_buckets_impl, q, date_bucket, size_bucket, date_custom_from,
                   date_custom_to, size_custom_min, size_custom_max);
}

/* =========================================================================
 * carve-format registry (ops #7-#8 parity: the og's analyser-registry views
 * EUGetAnalyserList/Map/Multimap + GetRawUserInfoMap on RAWSearchDLL -
 * corpus rawsearch-carving--deep-dive.md section 1). Grammar rows first
 * (FormatId order), then raw-sign rows (kFormats order); both tables only
 * ever APPEND, so the enumeration order is stable across releases.
 * ========================================================================= */

namespace {

void copy_ext_24(char (&dst)[24], const char* src) {
    u32 n = 0;
    while (src && src[n] && n < sizeof(dst) - 1) {
        dst[n] = src[n];
        ++n;
    }
    dst[n] = 0;
}

} // namespace

extern "C" CR_UI_API uint32_t cr_ui_carve_format_count(void) {
    return guarded(
        []() -> u32 { return u32(carve::registry_size()) + carve::rawsigns::kFormatCount; });
}

extern "C" CR_UI_API uint32_t cr_ui_carve_format_info(uint32_t index, cr_ui_carve_format* out) {
    return guarded(
        [](u32 i, cr_ui_carve_format* o) -> u32 {
            if (!o)
                return ret_(ERR_BAD_PARAMETER, "carve_format_info: out is null");
            const u32 grammar_n = u32(carve::registry_size());
            const u32 raw_n = carve::rawsigns::kFormatCount;
            std::memset(o, 0, sizeof(*o));
            if (i < grammar_n) {
                const carve::Signature& s = carve::registry()[i];
                o->layer = 0;
                copy_ext_24(o->ext, s.ext);
                o->min_size = s.min_size;
                o->max_size = s.max_size;
                o->base_confidence = s.base_confidence;
                o->magic_len = s.header_len;
                o->magic_offset = s.header_offset;
                if (s.header_len)
                    std::memcpy(o->magic, s.header, s.header_len);
                return ret_ok();
            }
            if (i < grammar_n + raw_n) {
                const u32 fmt = i - grammar_n;
                const carve::rawsigns::RawSignFormat& f = carve::rawsigns::kFormats[fmt];
                o->layer = 1;
                copy_ext_24(o->ext, f.ext);
                o->min_size = f.min_size;
                o->max_size = f.max_size;
                /* the format's strongest header-sign confidence (the table
                 * stores confidence per sign, not per format) */
                u16 best = 0;
                for (u32 k = 0; k < carve::rawsigns::kHeaderSignCount; ++k)
                    if (carve::rawsigns::kHeaderSigns[k].fmt == fmt &&
                        carve::rawsigns::kHeaderSigns[k].conf > best)
                        best = carve::rawsigns::kHeaderSigns[k].conf;
                o->base_confidence = best;
                return ret_ok();
            }
            return ret_(ERR_BAD_PARAMETER, "carve_format_info: index %u out of range (%u)", i,
                        grammar_n + raw_n);
        },
        index, out);
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
    /* F-1: extents are absolute - preview through the PARENT disk when the
     * scan device is a volume slice (same basis rule as the recovery writer) */
    IStorageDevice* src = d->dev.get();
    if (d->volume_len != 0 && d->hold)
        src = d->hold.get();
    p->reader.reset(new preview::PreviewReader(src, p->owned.get()));
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
    /* F-1: extents are absolute - preview through the PARENT disk when the
     * scan device is a volume slice (same basis rule as the recovery writer) */
    IStorageDevice* src = d->dev.get();
    if (d->volume_len != 0 && d->hold)
        src = d->hold.get();
    p->reader.reset(new preview::PreviewReader(src, p->owned.get()));
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
    /* F-1: tree extents are ABSOLUTE device offsets. When the scan device
     * is a volume slice, the writer must read through the PARENT disk
     * (absolute reads against the slice would shift every file by the
     * partition start). Image-file devices without a slice keep their
     * own basis (volume_len == 0). */
    IStorageDevice* src = d->dev.get();
    if (d->volume_len != 0 && d->hold)
        src = d->hold.get();
    r->writer.reset(new recovery::SafeWriter(*r->io, src, wcfg));
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
 * content items; empty leaves survive because every dir is enqueued.
 * M-018/M-071: bulk walks skip junk-flagged nodes — the og's
 * FilterFolder hidden set never reaches the recovery queue through a
 * result-set surface (explicit single-node selection stays allowed). */
u32 plan_enqueue_session(CrRecovery* r, tree::FileNode* node, const std::u16string& path,
                         u64* queued) {
    for (auto* c = node->first_child(); c; c = c->next_sibling()) {
        if (!c->props())
            continue;
        if ((c->props()->flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
            continue; /* M-018/M-071: the hidden junk set */
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
    /* S-023: adopt the session's scan analysis once (the report's block) */
    if (!r->have_analysis && s->stats.assistant.graded_count) {
        const tree::grading::AssistantStats& a = s->stats.assistant;
        r->analysis.total_file_count = a.total_file_count;
        r->analysis.raw_count = a.raw_count;
        r->analysis.lost_partition_exist_file_count = a.lost_partition_exist_file_count;
        r->analysis.lost_partition_delete_file_count = a.lost_partition_delete_file_count;
        r->analysis.quick_scan_exist_file_count = a.quick_scan_exist_file_count;
        r->analysis.quick_scan_delete_file_count = a.quick_scan_delete_file_count;
        r->analysis.deep_scan_file_count = a.deep_scan_file_count;
        r->analysis.quick_scan_delete_extend = a.quick_scan_delete_extend;
        for (size_t k = 0; k < 8; ++k)
            r->analysis.category_counts[k] = a.category_counts[k];
        r->analysis.matched = a.matched;
        r->analysis.total = a.total;
        r->analysis.small_result = a.small_result;
        r->have_analysis = true;
    }
    /* (merge-storm residue removed here: a byte-identical duplicate of the
     * S-023 adoption block above sat back-to-back with the live one. The
     * twin above sets have_analysis under the same graded_count condition,
     * so the second copy's guard could never be true - unreachable code,
     * deleted with zero behavior change; the live block keeps the
     * finding's full contract.) */
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
            if ((cn.flags & tree::NODE_FLAG_JUNK_FILTERED) != 0)
                continue; /* M-018/M-071: the hidden junk set (snapshot rows
                           * carry the flag the same way live nodes do) */
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
    out->nothing_to_recover = r->stats.nothing_to_recover; /* GAP-S6-7 */
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
                                 r->stats),
        r->have_analysis ? &r->analysis : nullptr);
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
        rep->temperature_c_x10 = nv.decidegrees_c(); /* N-042: one source with the CLI print */
        rep->available_spare = nv.available_spare;
        rep->available_spare_threshold = nv.available_spare_threshold;
        rep->percentage_used = nv.percentage_used;
        rep->data_units_read = nv.data_units_read;
        rep->data_units_written = nv.data_units_written;
        rep->power_on_hours_nvme = nv.power_on_hours;
        rep->unsafe_shutdowns = nv.unsafe_shutdowns;
        rep->media_errors = nv.media_errors;
        rep->host_reads = nv.host_reads;
        rep->host_writes = nv.host_writes;
        rep->controller_busy_time = nv.controller_busy_time;
        rep->num_error_log_entries = nv.num_error_log_entries;
        rep->data_bytes_read = nv.data_bytes_read;
        rep->data_bytes_written = nv.data_bytes_written;
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

/* ---- M-048: the user-extensible signature database (ABI-since 2.1) ---- */

extern "C" CR_UI_API uint32_t cr_ui_register_user_signs(const char* json_bytes, uint32_t json_len) {
    if (json_bytes == nullptr || json_len == 0)
        return ERR_BAD_PARAMETER;
    carve::usersigns::UserSignSet set;
    if (u32 rc = carve::usersigns::parse(json_bytes, size_t(json_len), set))
        return rc;
    return carve::usersigns::install_set(std::move(set));
}

extern "C" CR_UI_API uint32_t cr_ui_user_sign_count(void) {
    return carve::usersigns::active_count();
}

extern "C" CR_UI_API uint32_t cr_ui_user_sign_info(uint32_t index, cr_ui_user_sign* out) {
    if (out == nullptr)
        return ERR_BAD_PARAMETER;
    std::memset(out, 0, sizeof(*out));
    /* snapshot under the registry's own lock discipline: match_at/count
     * are the guarded surfaces; the info walk takes one consistent copy */
    const u32 n = carve::usersigns::active_count();
    if (index >= n)
        return ERR_BAD_PARAMETER;
    /* one consistent copy under the registry's lock (the count-then-read
     * window is closed by re-validating against the snapshot) */
    carve::usersigns::UserSignSet snap;
    if (u32 rc = carve::usersigns::snapshot(snap))
        return rc;
    if (index >= snap.signs.size())
        return ERR_BAD_PARAMETER; /* the set changed between calls */
    const carve::usersigns::UserSign& s = snap.signs[index];
    std::memcpy(out->ext, s.ext, sizeof(out->ext));
    std::memcpy(out->mime, s.mime, sizeof(out->mime));
    out->offset = s.offset;
    out->sign_len = s.len;
    std::memcpy(out->bytes, s.bytes, sizeof(out->bytes));
    return OK;
}
