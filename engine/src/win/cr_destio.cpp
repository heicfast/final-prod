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
    if (h == INVALID_HANDLE_VALUE)
        return ERR_DEST_UNWRITABLE;
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
            return ERR_WRITE_FAILED;
        }
    }
    return OK;
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
        return ERR_WRITE_FAILED;
    }
    return OK;
}

void WinDestIo::close(u64 item_id) {
    auto it = open_.find(item_id);
    if (it == open_.end())
        return;
    if (it->second.handle)
        CloseHandle(HANDLE(it->second.handle));
    /* an un-committed temp file is removed (atomic semantics: never a
     * half-written final file) */
    if (!it->second.temp_path.empty() && it->second.handle) {
        DeleteFileW(reinterpret_cast<const wchar_t*>(it->second.temp_path.c_str()));
    }
    open_.erase(it);
}

bool WinDestIo::exists(const std::u16string& path_utf16) {
    return GetFileAttributesW(reinterpret_cast<const wchar_t*>(path_utf16.c_str())) !=
           INVALID_FILE_ATTRIBUTES;
}

u32 WinDestIo::mkdir(const std::u16string& path_utf16) {
    /* G-08: create every component of the chain; ERROR_ALREADY_EXISTS is
     * success (idempotent reconstruction). Trailing separators stripped. */
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
        const DWORD e = GetLastError();
        if (e == ERROR_ALREADY_EXISTS) {
            /* a FILE in the way is not a directory: surface it */
            DWORD attr = GetFileAttributesW(reinterpret_cast<const wchar_t*>(prefix.c_str()));
            if (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY))
                continue;
            return ERR_DEST_UNWRITABLE;
        }
        return ERR_DEST_UNWRITABLE;
    }
    return OK;
}

} // namespace win
} // namespace cr
