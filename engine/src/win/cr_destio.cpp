/* cr_destio.cpp - Win32 DestIo (temp file + atomic rename).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_destio.h"
#include <windows.h>
#include <map>

namespace cr {
namespace win {

u32 WinDestIo::create_temp(u64 item_id, const std::u16string& final_path) {
    std::u16string temp = final_path + u".part";
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

} // namespace win
} // namespace cr
