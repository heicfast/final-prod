/*
 * cr_rescue_win.cpp - the Win32 rescue-image driver (F-33/F-34).
 *
 * See cr_rescue_win.h for the disposition contract. The sink does
 * positioned writes (SetFilePointerEx + WriteFile); the mapfile IO is
 * atomic (tmp + FlushFileBuffers + MoveFileExW with the 5x50ms
 * replace-while-open retry - the AV/indexer lock pattern); the resume
 * checks refuse mismatches instead of silently continuing.
 *
 * Clean-room: plain Win32 file IO. The behavioral reference is the
 * SectorForge sheet (MIT; imager.rs/mapfile.rs algorithms documented in
 * worklog/SECTORFORGE-FEATURES.md sections 2.1/2.2) and GNU ddrescue's
 * documented mapfile format.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_rescue_win.h"
#include "cr/cr_log.h"

#include <windows.h>

namespace cr {
namespace win {
namespace {

class WinRescueImageSink final : public storage::RescueImageSink {
public:
    ~WinRescueImageSink() {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }

    /* fresh: CREATE_ALWAYS. resume: OPEN_ALWAYS (the existing bytes stay;
     * the map says which are real). `sparse_hint`: FSCTL_SET_SPARSE
     * best-effort (a reverse run's first write at the end would otherwise
     * zero-fill the whole image; a FAT/exFAT destination declines). */
    bool open(const std::u16string& path, bool resume, bool sparse_hint) {
        h_ = CreateFileW(reinterpret_cast<const wchar_t*>(path.c_str()), GENERIC_WRITE,
                         0 /* exclusive while imaging */, nullptr,
                         resume ? OPEN_ALWAYS : CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h_ == INVALID_HANDLE_VALUE)
            return false;
        if (sparse_hint) {
            /* best-effort: NTFS accepts, FAT/exFAT declines silently */
            DWORD bytes = 0;
            DeviceIoControl(h_, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &bytes, nullptr);
        }
        return true;
    }

    u64 size() const {
        LARGE_INTEGER sz{};
        if (h_ == INVALID_HANDLE_VALUE || !GetFileSizeEx(h_, &sz))
            return 0;
        return sz.QuadPart < 0 ? 0 : u64(sz.QuadPart);
    }

    u32 pwrite(u64 offset, const void* data, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_WRITE_FAILED;
        LARGE_INTEGER li;
        li.QuadPart = LONGLONG(offset);
        if (!SetFilePointerEx(h_, li, nullptr, FILE_BEGIN))
            return ERR_WRITE_FAILED;
        DWORD done_total = 0;
        while (done_total < len) {
            DWORD done = 0;
            if (!WriteFile(h_, static_cast<const u8*>(data) + done_total, len - done_total, &done,
                           nullptr) ||
                done == 0)
                return ERR_WRITE_FAILED;
            done_total += done;
        }
        return OK;
    }

    u32 flush() override {
        if (h_ == INVALID_HANDLE_VALUE)
            return ERR_WRITE_FAILED;
        return FlushFileBuffers(h_) != FALSE ? u32(OK) : u32(ERR_WRITE_FAILED);
    }

    void close() {
        if (h_ != INVALID_HANDLE_VALUE) {
            CloseHandle(h_);
            h_ = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE h_ = INVALID_HANDLE_VALUE;
};

bool file_exists(const std::u16string& path) {
    return GetFileAttributesW(reinterpret_cast<const wchar_t*>(path.c_str())) !=
           INVALID_FILE_ATTRIBUTES;
}

bool read_all_file(const std::u16string& path, std::string* out) {
    HANDLE h = CreateFileW(reinterpret_cast<const wchar_t*>(path.c_str()), GENERIC_READ,
                           FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    LARGE_INTEGER sz{};
    if (!GetFileSizeEx(h, &sz) || sz.QuadPart < 0 || sz.QuadPart > (64 << 20)) {
        CloseHandle(h);
        return false; /* a mapfile is small; a huge one is not ours */
    }
    out->resize(size_t(sz.QuadPart));
    DWORD done_total = 0;
    while (done_total < out->size()) {
        DWORD done = 0;
        if (!ReadFile(h, out->data() + done_total, DWORD(out->size() - done_total), &done,
                      nullptr) ||
            done == 0) {
            CloseHandle(h);
            return false;
        }
        done_total += done;
    }
    CloseHandle(h);
    return true;
}

bool write_all_file_atomic(const std::u16string& path, const std::string& text) {
    const std::u16string tmp = path + u".tmp";
    HANDLE h = CreateFileW(reinterpret_cast<const wchar_t*>(tmp.c_str()), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE)
        return false;
    DWORD done_total = 0;
    bool ok = true;
    while (ok && done_total < text.size()) {
        DWORD done = 0;
        if (!WriteFile(h, text.data() + done_total, DWORD(text.size() - done_total), &done,
                       nullptr) ||
            done == 0)
            ok = false;
        else
            done_total += done;
    }
    if (ok)
        ok = FlushFileBuffers(h) != FALSE;
    CloseHandle(h);
    if (!ok) {
        (void)DeleteFileW(reinterpret_cast<const wchar_t*>(tmp.c_str()));
        return false;
    }
    /* the replace-while-open retry (AV/indexer/sync locks): 5x50ms */
    const wchar_t* from = reinterpret_cast<const wchar_t*>(tmp.c_str());
    const wchar_t* to = reinterpret_cast<const wchar_t*>(path.c_str());
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (MoveFileExW(from, to, MOVEFILE_REPLACE_EXISTING))
            return true;
        Sleep(50);
    }
    (void)DeleteFileW(reinterpret_cast<const wchar_t*>(tmp.c_str()));
    return false;
}

class WinRescueMapIo final : public storage::RescueMapIo {
public:
    explicit WinRescueMapIo(std::u16string path) : path_(std::move(path)) {}

    u32 write_map(const std::string& text, bool forced) override {
        if (!write_all_file_atomic(path_, text)) {
            if (forced)
                return ERR_WRITE_FAILED;
            return ERR_WRITE_FAILED; /* the caller's periodic policy swallows */
        }
        return OK;
    }

    u32 read_map(std::string* out) override {
        if (!read_all_file(path_, out))
            return ERR_OPEN_FAILED;
        return OK;
    }

    bool map_exists() const override { return file_exists(path_); }

private:
    std::u16string path_;
};

/* the map's highest Finished end (the destination must hold at least
 * this when resuming, or the map claims bytes the image does not have) */
u64 map_max_finished_end(const storage::RescueMap& m) {
    u64 end = 0;
    for (const auto& b : m.blocks())
        if (b.status == storage::RescueStatus::Finished)
            end = std::max(end, b.pos + b.size);
    return end;
}

} // namespace

u32 create_rescue_image(IStorageDevice& src, const std::u16string& out_path_utf16,
                        storage::RescueImagerConfig cfg, CancelToken* cancel,
                        storage::RescueProgressFn progress, void* progress_user,
                        storage::RescueStats* stats) {
    if (out_path_utf16.empty())
        return ERR_BAD_PARAMETER;
    const std::u16string map_path = out_path_utf16 + u".mapfile";
    WinRescueMapIo map_io(map_path);
    WinRescueImageSink sink;

    storage::RescueMap preloaded;
    bool resume = false;
    if (map_io.map_exists()) {
        std::string text;
        if (map_io.read_map(&text) != OK) {
            cr::log::write(cr::log::Level::Info,
                           "rescue: mapfile exists but cannot be read: refusing (never guessing)");
            return ERR_OPEN_FAILED;
        }
        if (!storage::RescueMap::parse(text, &preloaded)) {
            cr::log::write(cr::log::Level::Info,
                           "rescue: mapfile is malformed: refusing (never guessing)");
            return ERR_BAD_PARAMETER;
        }
        if (preloaded.total() != src.size()) {
            cr::log::write(cr::log::Level::Info,
                           "rescue: mapfile covers %llu bytes, source is %llu: wrong map for this "
                           "source - refusing",
                           static_cast<unsigned long long>(preloaded.total()),
                           static_cast<unsigned long long>(src.size()));
            return ERR_BAD_PARAMETER;
        }
        if (preloaded.finished_bytes() > 0) {
            if (!file_exists(out_path_utf16)) {
                cr::log::write(
                    cr::log::Level::Info,
                    "rescue: mapfile claims rescued bytes but the destination is missing - "
                    "resuming would leave those parts empty: refusing");
                return ERR_BAD_PARAMETER;
            }
            if (!sink.open(out_path_utf16, true, false)) {
                cr::log::write(cr::log::Level::Info,
                               "rescue: cannot open the destination for resume");
                return ERR_OPEN_FAILED;
            }
            if (sink.size() < map_max_finished_end(preloaded)) {
                cr::log::write(cr::log::Level::Info,
                               "rescue: destination is shorter than the mapfile's rescued prefix - "
                               "refusing");
                sink.close();
                return ERR_BAD_PARAMETER;
            }
            resume = true;
        }
    }

    if (!resume) {
        if (!sink.open(out_path_utf16, false, cfg.reverse)) {
            cr::log::write(cr::log::Level::Info, "rescue: cannot create the destination image");
            return ERR_OPEN_FAILED;
        }
    }

    storage::RescueImager imager(src, sink, map_io, cfg);
    storage::RescueStats local{};
    const u32 rc =
        imager.run(resume ? &preloaded : nullptr, cancel, progress, progress_user, &local);
    local.resumed = resume;
    /* the final map save is the imager's own forced checkpoint; the
     * destination stays open until after it (ordering: image flush ->
     * map save) - the imager's checkpoint_ already flushed the sink. */
    sink.close();
    if (stats)
        *stats = local;
    cr::log::write(cr::log::Level::Info,
                   "rescue image: rc=0x%08x rescued=%llu bad=%llu pending=%llu%s", rc,
                   static_cast<unsigned long long>(local.rescued_bytes),
                   static_cast<unsigned long long>(local.bad_bytes),
                   static_cast<unsigned long long>(local.pending_bytes),
                   local.cancelled ? " (cancelled - resumable)" : "");
    return rc;
}

} // namespace win
} // namespace cr
