/* cr_destio.cpp - Win32 DestIo (temp file + atomic rename).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_destio.h"
#include <windows.h>
#include <map>

namespace cr {
namespace win {

u32 WinDestIo::create_temp(u64 item_id, const std::u16string& final_path,
                           const std::u16string& temp_suffix) {
    std::u16string temp = final_path + temp_suffix; /* per-item suffix (audit row 35c) */
    HANDLE h = CreateFileW(reinterpret_cast<const wchar_t*>(temp.c_str()), GENERIC_WRITE, 0,
                           nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        /* N-015: ENOSPC maps to the distinct destination-full code so the
         * writer fast-fails the queue instead of attempting every item */
        const DWORD e = GetLastError();
        if (e == ERROR_DISK_FULL || e == ERROR_HANDLE_DISK_FULL)
            return ERR_DEST_FULL;
        return ERR_DEST_UNWRITABLE;
    }
    open_[item_id] = Open{h, temp, final_path};
    return OK;
}

u32 WinDestIo::write(u64 item_id, u64 offset, const void* data, u32 len) {
    auto it = open_.find(item_id);
    if (it == open_.end())
        return ERR_BAD_PARAMETER;
    OVERLAPPED ov = {};
    ov.Offset = u32(offset);
    ov.OffsetHigh = u32(offset >> 32);
    DWORD got = 0;
    if (!WriteFile(HANDLE(it->second.handle), data, len, &got, &ov) || got != len) {
        DWORD e = GetLastError();
        if (e == ERROR_IO_PENDING) {
            if (!GetOverlappedResult(HANDLE(it->second.handle), &ov, &got, TRUE) || got != len)
                return ERR_WRITE_FAILED;
        } else {
            /* N-015: ENOSPC is destination-full, not a generic write failure */
            if (e == ERROR_DISK_FULL || e == ERROR_HANDLE_DISK_FULL)
                return ERR_DEST_FULL;
            return ERR_WRITE_FAILED;
        }
    }
    return OK;
}

u64 WinDestIo::free_bytes(const std::u16string& root_utf16) const {
    /* N-015: the og's pre-write free-space computation (RecoveryModule
     * :47033-47038, GetDiskFreeSpaceExA) - the "available to caller"
     * figure at the destination root. 0 = unknown (probe failed: root
     * not yet created etc.) - the writer then skips the pre-check. */
    if (root_utf16.empty())
        return 0;
    ULARGE_INTEGER avail{};
    if (!GetDiskFreeSpaceExW(reinterpret_cast<const wchar_t*>(root_utf16.c_str()), &avail, nullptr,
                             nullptr))
        return 0;
    return u64(avail.QuadPart);
}

u32 WinDestIo::commit(u64 item_id) {
    auto it = open_.find(item_id);
    if (it == open_.end())
        return ERR_BAD_PARAMETER;
    if (!FlushFileBuffers(HANDLE(it->second.handle)))
        return ERR_WRITE_FAILED;
    /* drop the OVL write state, close, then atomic rename */
    CloseHandle(HANDLE(it->second.handle));
    it->second.handle = nullptr;
    if (!MoveFileExW(reinterpret_cast<const wchar_t*>(it->second.temp_path.c_str()),
                     reinterpret_cast<const wchar_t*>(it->second.final_path.c_str()),
                     MOVEFILE_REPLACE_EXISTING)) {
        /* N-056: the rename failed - the staged temp must NOT survive
         * (the og's recovery funnel leaves no orphaned temps; UIUX 11).
         * Delete it here; close()'s !temp_renamed gate retries if this
         * delete itself fails. The old code returned with the handle
         * already null, so close()'s handle-gated delete never fired -
         * every failed rename leaked the .part-N temp on the
         * destination. */
        DeleteFileW(reinterpret_cast<const wchar_t*>(it->second.temp_path.c_str()));
        return ERR_WRITE_FAILED;
    }
    it->second.temp_renamed = true;
    return OK;
}

void WinDestIo::close(u64 item_id) {
    auto it = open_.find(item_id);
    if (it == open_.end())
        return;
    if (it->second.handle)
        CloseHandle(HANDLE(it->second.handle));
    /* an un-committed temp file is removed (atomic semantics: never a
     * half-written final file). N-056: the gate is the RENAME STATE, not
     * the handle - commit() nulls the handle BEFORE the rename, so the
     * old handle gate could never fire after a failed rename (the
     * leaked-temp defect). Covers: write-phase failure, flush failure,
     * failed rename (commit already deleted; this is the retry), and
     * never fires after a successful rename (temp_renamed). */
    if (!it->second.temp_path.empty() && !it->second.temp_renamed) {
        DeleteFileW(reinterpret_cast<const wchar_t*>(it->second.temp_path.c_str()));
    }
    open_.erase(it);
}

bool WinDestIo::exists(const std::u16string& path_utf16) {
    return GetFileAttributesW(reinterpret_cast<const wchar_t*>(path_utf16.c_str())) !=
           INVALID_FILE_ATTRIBUTES;
}

u32 WinDestIo::mkdir(const std::u16string& path_utf16) {
    /* G-08: create every component of the chain; a component that already
     * EXISTS AS A DIRECTORY is success (idempotent reconstruction).
     * Trailing separators stripped.
     *
     * The exist-check deliberately beats ANY create error, not just
     * ERROR_ALREADY_EXISTS: drive-root-shaped prefixes ("C:", "C:\\",
     * UNC share roots) and every pre-existing chain component fail
     * CreateDirectoryW with a variety of codes (ACCESS_DENIED /
     * INVALID_NAME for drive aliases) while the component is in fact an
     * existing directory. That is exactly the POSIX contract (::mkdir
     * EEXIST == success); the earlier ALREADY_EXISTS-only branch turned
     * every drive-letter destination chain into ERR_DEST_UNWRITABLE -
     * all items of a recovery failed (MSVC lane, test #227: 4 errors /
     * 0 done). A FILE in the way still fails (not a directory). */
    std::u16string p = path_utf16;
    while (p.size() > 1 && (p.back() == u'\\' || p.back() == u'/'))
        p.pop_back();
    if (p.empty())
        return ERR_BAD_PARAMETER;
    /* keep the leading \\?\ or \\.\ prefix intact across the split */
    size_t start = 0;
    if (p.size() >= 2 && p[0] == u'\\' && p[1] == u'\\')
        start = 2;
    for (size_t i = start + 1; i <= p.size(); ++i) {
        const bool end = i == p.size();
        if (!end && p[i] != u'\\' && p[i] != u'/')
            continue;
        if (i == start + 1)
            continue; /* empty component */
        std::u16string prefix = p.substr(0, i);
        if (CreateDirectoryW(reinterpret_cast<const wchar_t*>(prefix.c_str()), nullptr))
            continue;
        const DWORD attr = GetFileAttributesW(reinterpret_cast<const wchar_t*>(prefix.c_str()));
        if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
            continue; /* exist-as-directory: the EEXIST contract */
        return ERR_DEST_UNWRITABLE;
    }
    return OK;
}

} // namespace win
} // namespace cr
