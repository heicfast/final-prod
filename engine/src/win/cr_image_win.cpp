/*
 * cr_image_win.cpp - Win32 ImageFileIo for the S-030 disk-image devices
 * (CreateFileW + GetFileSizeEx, matching the og's flat-fallback primitives
 * at DRWUI_decompiled.c :948389-948393).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/storage/cr_image.h"
#include <windows.h>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace cr {
namespace win {

/* Real-file ImageFileIo: handles map 1:1 to Win32 file handles. Thread
 * safe (handle table locked; reads are position-independent OVERLAPPED
 * so they never share file pointers). */
class WinImageFileIo final : public storage::ImageFileIo {
public:
    ~WinImageFileIo() override {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& kv : files_)
            CloseHandle(HANDLE(kv.first));
        files_.clear();
    }

    u32 open(const std::u16string& path_utf16, u32* err) override {
        if (path_utf16.size() >= 32768) {
            if (err)
                *err = ERR_BAD_PARAMETER;
            return 0;
        }
        std::wstring path(path_utf16.begin(), path_utf16.end());
        HANDLE h = CreateFileW(path.c_str(), GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) {
            if (err)
                *err = ERR_OPEN_FAILED;
            return 0;
        }
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(h, &size) || size.QuadPart <= 0) {
            CloseHandle(h);
            if (err)
                *err = ERR_OPEN_FAILED;
            return 0;
        }
        std::lock_guard<std::mutex> lock(mu_);
        next_id_ += 2; /* odd nonzero ids; 0 is the error sentinel */
        files_.emplace(next_id_, h);
        /* MSVC C4244: the u32 handle is a bounded generation counter by
         * design; the clamp keeps the narrowing truthful under /WX */
        return u32(next_id_ > 0xFFFFFFFFull ? 0xFFFFFFFFull : next_id_);
    }

    u32 read(u64 file, u64 offset, void* buffer, u32 len) override {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = files_.find(file);
        if (it == files_.end())
            return ERR_BAD_PARAMETER;
        OVERLAPPED ov{};
        ov.Offset = DWORD(offset & 0xFFFFFFFFull);
        ov.OffsetHigh = DWORD(offset >> 32);
        DWORD done = 0;
        u64 total = 0;
        auto* out = static_cast<u8*>(buffer);
        while (total < len) {
            const DWORD want = len - total > 0x40000000u ? 0x40000000u : DWORD(len - total);
            if (!ReadFile(it->second, out + total, want, &done, &ov))
                return ERR_READ_FAILED;
            if (done == 0)
                return ERR_TRUNCATED;
            total += done;
            const u64 pos = offset + total;
            ov.Offset = DWORD(pos & 0xFFFFFFFFull);
            ov.OffsetHigh = DWORD(pos >> 32);
        }
        return OK;
    }

    u64 size(u64 file) const override {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = files_.find(file);
        if (it == files_.end())
            return 0;
        LARGE_INTEGER sz{};
        if (!GetFileSizeEx(it->second, &sz))
            return 0;
        return u64(sz.QuadPart);
    }

    void close(u64 file) override {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = files_.find(file);
        if (it == files_.end())
            return;
        CloseHandle(it->second);
        files_.erase(it);
    }

private:
    mutable std::mutex mu_;
    std::unordered_map<u64, HANDLE> files_;
    u64 next_id_ = 0;
};

/* The platform-owned ImageFileIo singleton used by rrecov's --image path. */
storage::ImageFileIo& image_file_io() {
    static WinImageFileIo io;
    return io;
}

} // namespace win
} // namespace cr
