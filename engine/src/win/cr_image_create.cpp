/*
 * cr_image_create.cpp - S-040: the flat-raw disk-image producer (win layer).
 *
 * Clean-room: plain Win32 file IO (CreateFileW/WriteFile/FlushFileBuffers/
 * MoveFileExW/WideCharToMultiByte) over the portable copy loop
 * (storage::create_image_stream, cr_image.cpp). No proprietary code or
 * strings; the corpus cites live in the header and the finding.
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_image_create.h"

#include "cr/crypto/cr_sha256.h"
#include "cr/win/cr_transport.h"

#include <windows.h>

namespace cr {
namespace win {
namespace {

/* UTF-16 -> UTF-8 (the sidecar's basename must survive non-ASCII paths) */
std::string to_utf8(const std::u16string& s) {
    if (s.empty())
        return {};
    const int need = WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(s.data()),
                                         int(s.size()), nullptr, 0, nullptr, nullptr);
    if (need <= 0)
        return {};
    std::string out(size_t(need), '\0');
    WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<const wchar_t*>(s.data()), int(s.size()),
                        &out[0], need, nullptr, nullptr);
    return out;
}

/* the standard sidecar line: "<hex>  <basename>" */
std::string sidecar_line(const u8 sha256[32], const std::u16string& final_path_utf16) {
    const std::string hex = crypto::to_hex(sha256, 32);
    const std::string full = to_utf8(final_path_utf16);
    size_t slash = full.find_last_of('/');
    const size_t bslash = full.find_last_of('\\');
    if (bslash != std::string::npos && (slash == std::string::npos || bslash > slash))
        slash = bslash;
    const std::string base = (slash == std::string::npos) ? full : full.substr(slash + 1);
    return hex + "  " + base + "\n";
}

class FileSink final : public storage::ImageCreateSink {
public:
    ~FileSink() {
        if (h_ != INVALID_HANDLE_VALUE)
            CloseHandle(h_);
    }
    bool open(const std::u16string& path) {
        h_ = CreateFileW(reinterpret_cast<const wchar_t*>(path.c_str()), GENERIC_WRITE,
                         0 /* exclusive while streaming */, nullptr, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
        return h_ != INVALID_HANDLE_VALUE;
    }
    u32 write(const void* data, u32 len) override {
        if (h_ == INVALID_HANDLE_VALUE)
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
    bool flush() { return FlushFileBuffers(h_) != FALSE; }
    void close() {
        if (h_ != INVALID_HANDLE_VALUE) {
            CloseHandle(h_);
            h_ = INVALID_HANDLE_VALUE;
        }
    }

private:
    HANDLE h_ = INVALID_HANDLE_VALUE;
};

bool rename_file(const std::u16string& from, const std::u16string& to) {
    return MoveFileExW(reinterpret_cast<const wchar_t*>(from.c_str()),
                       reinterpret_cast<const wchar_t*>(to.c_str()),
                       MOVEFILE_REPLACE_EXISTING) != FALSE;
}

/* write the digest sidecar next to the committed image; the image itself
 * is already complete and renamed at this point - a sidecar failure is
 * reported to the caller (the image stays committed; the operation's rc
 * is still truthful: something failed) */
u32 write_sidecar(const std::u16string& final_path, const u8 sha256[32]) {
    const std::u16string sidecar = final_path + u".sha256";
    FileSink f;
    if (!f.open(sidecar))
        return ERR_WRITE_FAILED;
    const std::string line = sidecar_line(sha256, final_path);
    const u32 rc = f.write(line.data(), u32(line.size()));
    if (rc != OK)
        return rc;
    if (!f.flush())
        return ERR_WRITE_FAILED;
    return OK; /* ~FileSink closes */
}

} // namespace

u32 create_image_file(IStorageDevice& src, const std::u16string& out_path_utf16,
                      CancelToken* cancel, storage::ImageCreateProgressFn progress,
                      void* progress_user, storage::ImageCreateStats* stats) {
    if (out_path_utf16.empty())
        return ERR_BAD_PARAMETER;
    const std::u16string temp = out_path_utf16 + u".crimg-tmp";
    const std::u16string partial = out_path_utf16 + u".partial";

    FileSink sink;
    if (!sink.open(temp))
        return ERR_OPEN_FAILED;

    storage::ImageCreateStats local{};
    const u32 rc = storage::create_image_stream(src, sink, cancel, progress, progress_user, &local);
    if (rc == OK) {
        if (!sink.flush()) {
            sink.close();
            /* the image content is on disk but uncommitted (no rename):
             * keep it as the explicit .partial, never claim completion */
            rename_file(temp, partial);
            if (stats)
                *stats = local;
            return ERR_WRITE_FAILED;
        }
        sink.close();
        if (!rename_file(temp, out_path_utf16)) {
            rename_file(temp, partial);
            if (stats)
                *stats = local;
            return ERR_WRITE_FAILED;
        }
        /* committed: the sidecar is the last step; its failure is
         * reported (the image itself is complete and renamed) */
        const u32 sidecar_rc = write_sidecar(out_path_utf16, local.sha256);
        if (stats)
            *stats = local;
        return sidecar_rc;
    }

    /* cancel or mid-stream failure: close, keep the partial under the
     * explicit .partial name (best-effort rename; the original rc wins) */
    sink.close();
    rename_file(temp, partial);
    if (stats)
        *stats = local;
    return rc;
}

u32 create_disk_image(u32 disk, const wchar_t* service, const std::u16string& out_path_utf16,
                      CancelToken* cancel, storage::ImageCreateProgressFn progress,
                      void* progress_user, storage::ImageCreateStats* stats) {
    u32 err = 0;
    std::unique_ptr<IStorageDevice> dev = open_disk(disk, service, &err);
    if (!dev)
        return err;
    return create_image_file(*dev, out_path_utf16, cancel, progress, progress_user, stats);
}

} // namespace win
} // namespace cr
