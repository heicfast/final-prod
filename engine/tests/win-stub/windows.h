/* windows.h - DECLARATION-ONLY stub of the Win32 SDK surface the
 * src/win platform layer consumes.
 *
 * Purpose (M-055 gate): the Windows-only layer (src/win/*.cpp) has no
 * MSVC lane executing in this environment, and a build-breaking edit
 * there is otherwise invisible until a Windows CI run executes. This
 * header re-declares the exact SDK surface those files consume so the
 * Linux lane can run `g++ -fsyntax-only` over the whole win layer as
 * a parse gate (tests/win_syntax_check).
 *
 * These are DECLARATIONS ONLY - inline bodies exist solely so the
 * translation units parse; the gate never links or runs them.
 * Signature fidelity follows the SDK (ULONG_PTR width on x64,
 * wchar_t 16-bit via -fshort-wchar, SAL stripped). When src/win starts
 * consuming a new SDK symbol, the gate fails here until the stub grows
 * - that failure is the gate working (it is exactly how M-055 was
 * caught).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>

typedef unsigned long DWORD;
typedef unsigned long ULONG;
typedef unsigned long long ULONG_PTR;
typedef long LONG;
typedef int BOOL;
typedef unsigned char BOOLEAN;
typedef unsigned int UINT;
typedef void* HANDLE;
typedef void* HKEY;
typedef const wchar_t* LPCWSTR;
typedef wchar_t* LPWSTR;
typedef char* LPTSTR;
typedef const char* LPCSTR;
typedef char* LPSTR;
typedef DWORD* LPDWORD;
typedef ULONG* PULONG;
typedef void* LPVOID;

struct LARGE_INTEGER {
    union {
        struct {
            DWORD LowPart;
            long HighPart;
        };
        long long QuadPart;
    };
};
typedef unsigned short USHORT;
struct ULARGE_INTEGER {
    unsigned long long QuadPart;
};
struct OVERLAPPED {
    unsigned long Internal;
    unsigned long InternalHigh;
    union {
        struct {
            unsigned long Offset;
            unsigned long OffsetHigh;
        };
        void* Pointer;
    };
    HANDLE hEvent;
};
struct SECURITY_ATTRIBUTES {
    DWORD nLength;
    void* lpSecurityDescriptor;
    BOOL bInheritHandle;
};
struct CRITICAL_SECTION {
    void* DebugInfo;
    long LockCount;
    long RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    unsigned long SpinCount;
};
struct GUID {
    unsigned long Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char Data4[8];
};
struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

#define FALSE 0
#define TRUE 1
#define MAX_PATH 260
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)0xFFFFFFFF)

/* calling convention + console control (app layer: rrecov's Ctrl-C).
 * WINAPI: on x64 the convention is ignored by MSVC too, so an empty
 * definition is ABI-faithful for the parse gate. (__declspec is neutral-
 * ized in the gate's CXXFLAGS - see win_syntax_check.sh - so it applies
 * regardless of include order.) */
#define WINAPI
#define CTRL_C_EVENT 0
#define CTRL_BREAK_EVENT 1

/* create/share/flags (values as the SDK defines them) */
#define ERROR_ACCESS_DENIED 5L
#define ERROR_SHARING_VIOLATION 32L
#define GENERIC_READ 0x80000000UL
#define GENERIC_WRITE 0x40000000UL
#define FILE_SHARE_READ 0x00000001UL
#define FILE_SHARE_WRITE 0x00000002UL
#define FILE_SHARE_DELETE 0x00000004UL
#define CREATE_ALWAYS 2
#define CREATE_NO_WINDOW 0x08000000UL
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x00000080UL
#define NO_ERROR 0UL
#define INVALID_SET_FILE_POINTER 0xFFFFFFFFUL
#define FILE_BEGIN 0UL
#define FILE_CURRENT 1UL
#define FILE_END 2UL
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010UL
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100UL
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000UL
#define FILE_FLAG_OVERLAPPED 0x40000000UL

/* error codes */
#define ERROR_SUCCESS 0UL
#define ERROR_INVALID_FUNCTION 1UL
#define ERROR_FILE_NOT_FOUND 2UL
#define ERROR_INVALID_HANDLE 6UL
#define ERROR_CANCELLED 1223UL
#define ERROR_MORE_DATA 234UL
#define ERROR_ALREADY_EXISTS 183UL
#define ERROR_DISK_FULL 112UL
#define ERROR_HANDLE_DISK_FULL 39UL
#define ERROR_IO_PENDING 998UL
#define ERROR_INSUFFICIENT_BUFFER 122UL

/* wait codes */
#define WAIT_OBJECT_0 0UL
#define WAIT_TIMEOUT 258UL
#define INFINITE 0xFFFFFFFFUL

/* token / elevation */
#define TOKEN_QUERY 0x0008UL
struct TOKEN_ELEVATION {
    DWORD TokenIsElevated;
};
#define TokenElevation 20

/* registry */
#define HKEY_LOCAL_MACHINE ((HKEY)(long long)0x80000002)
#define HKEY_CURRENT_USER ((HKEY)(long long)0x80000001)
#define KEY_READ 0x20019UL
#define REG_SZ 1UL
#define REG_BINARY 3UL
#define REG_EXPAND_SZ 2UL
#define RRF_RT_REG_SZ 0x00000002UL

#define MOVEFILE_REPLACE_EXISTING 1
#define DRIVE_UNKNOWN 0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE 2
#define DRIVE_FIXED 3
#define DRIVE_REMOTE 4
#define DRIVE_CDROM 5
#define DRIVE_RAMDISK 6

/* ---- file/device I/O ------------------------------------------------- */
inline DWORD GetLastError() {
    return 0;
}
inline void SetLastError(DWORD) {}
inline HANDLE CreateFileW(const wchar_t*, DWORD, DWORD, SECURITY_ATTRIBUTES*, DWORD, DWORD,
                          HANDLE) {
    return (HANDLE)0;
}
inline BOOL CloseHandle(HANDLE) {
    return 1;
}
inline BOOL ReadFile(HANDLE, void*, DWORD, DWORD*, OVERLAPPED*) {
    return 1;
}
inline BOOL WriteFile(HANDLE, const void*, DWORD, DWORD*, OVERLAPPED*) {
    return 1;
}
inline BOOL GetOverlappedResult(HANDLE, OVERLAPPED*, DWORD*, BOOL) {
    return 1;
}
inline BOOL FlushFileBuffers(HANDLE) {
    return 1;
}
inline BOOL DeleteFileW(const wchar_t*) {
    return 1;
}
inline DWORD GetFileAttributesW(const wchar_t*) {
    return 0;
}
inline BOOL CreateDirectoryW(const wchar_t*, SECURITY_ATTRIBUTES*) {
    return 1;
}
inline BOOL MoveFileExW(const wchar_t*, const wchar_t*, DWORD) {
    return 1;
}
inline BOOL GetFileSizeEx(HANDLE, LARGE_INTEGER*) {
    return 1;
}
inline DWORD SetFilePointer(HANDLE, LONG, LONG*, DWORD) {
    return 0;
}
typedef void* FARPROC;
typedef void* HMODULE;
typedef void* HINSTANCE;
inline HMODULE GetModuleHandleW(const wchar_t*) {
    return (HMODULE)0;
}
inline FARPROC GetProcAddress(HMODULE, const char*) {
    return (FARPROC)0;
}
inline BOOL GetDiskFreeSpaceExW(const wchar_t*, ULARGE_INTEGER*, ULARGE_INTEGER*, ULARGE_INTEGER*) {
    return 1;
}
inline HANDLE GetCurrentProcess() {
    return (HANDLE)0;
}
inline DWORD GetCurrentProcessId() {
    return 0;
}
inline void Sleep(DWORD) {}
inline unsigned long long GetTickCount64() {
    return 0;
}
inline DWORD GetTickCount() {
    return 0;
}
inline BOOL QueryPerformanceCounter(LARGE_INTEGER*) {
    return 1;
}
inline BOOL QueryPerformanceFrequency(LARGE_INTEGER*) {
    return 1;
}

/* ---- CRT-adjacent formatting (MSVC secure CRT) ----------------------- */
#define _TRUNCATE ((size_t) - 1)
inline int _snwprintf_s(wchar_t*, size_t, size_t, const wchar_t*, ...) {
    return 0;
}
inline int swprintf_s(wchar_t*, size_t, const wchar_t*, ...) {
    return 0;
}

/* ---- volumes / mount points ------------------------------------------ */
inline DWORD GetLogicalDriveStringsW(DWORD, wchar_t*) {
    return 0;
}
inline UINT GetDriveTypeW(const wchar_t*) {
    return DRIVE_FIXED;
}
inline BOOL GetVolumeInformationW(const wchar_t*, wchar_t*, DWORD, DWORD*, DWORD*, DWORD*, wchar_t*,
                                  DWORD) {
    return 1;
}
inline BOOL GetVolumeNameForVolumeMountPointW(const wchar_t*, wchar_t*, DWORD) {
    return 1;
}
inline BOOL SetVolumeMountPointW(const wchar_t*, const wchar_t*) {
    return 1;
}
inline UINT GetSystemDirectoryA(char*, UINT) {
    return 0;
}
inline BOOL GetVolumePathNameW(const wchar_t*, wchar_t*, DWORD) {
    return 1;
}
inline BOOL DeleteVolumeMountPointW(const wchar_t*) {
    return 1;
}
inline DWORD GetTempPathW(DWORD, wchar_t*) {
    return 0;
}
inline UINT GetWindowsDirectoryW(wchar_t*, UINT) {
    return 0;
}
inline DWORD GetFinalPathNameByHandleW(HANDLE, wchar_t*, DWORD, DWORD) {
    return 0;
}
inline BOOL GetVolumePathNamesForVolumeNameW(const wchar_t*, wchar_t*, DWORD, DWORD*) {
    return 1;
}

/* ---- process / token ------------------------------------------------- */
inline BOOL OpenProcessToken(HANDLE, DWORD, HANDLE*) {
    return 1;
}
#define TOKEN_ADJUST_PRIVILEGES 0x0020UL
#define SE_PRIVILEGE_ENABLED 0x00000002UL
struct LUID {
    DWORD LowPart;
    long HighPart;
};
struct LUID_AND_ATTRIBUTES {
    LUID Luid;
    DWORD Attributes;
};
struct TOKEN_PRIVILEGES {
    DWORD PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[1];
};
#define SE_SYSTEM_ENVIRONMENT_NAME L"SeSystemEnvironmentPrivilege"
#define SE_BACKUP_NAME L"SeBackupPrivilege"
#define SE_RESTORE_NAME L"SeRestorePrivilege"
inline BOOL LookupPrivilegeValueW(const wchar_t*, const wchar_t*, LUID*) {
    return 1;
}
inline BOOL AdjustTokenPrivileges(HANDLE, BOOL, TOKEN_PRIVILEGES*, DWORD, void*, void*) {
    return 1;
}
inline BOOL GetTokenInformation(HANDLE, int, void*, DWORD, DWORD*) {
    return 1;
}
struct STARTUPINFOW {
    DWORD cb;
    wchar_t* lpReserved;
    wchar_t* lpDesktop;
    wchar_t* lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    unsigned short wShowWindow;
    unsigned short cbReserved2;
    unsigned char* lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
};
struct PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
};
inline BOOL CreateProcessW(const wchar_t*, wchar_t*, SECURITY_ATTRIBUTES*, SECURITY_ATTRIBUTES*,
                           BOOL, DWORD, void*, const wchar_t*, STARTUPINFOW*,
                           PROCESS_INFORMATION*) {
    return 1;
}
inline DWORD WaitForSingleObject(HANDLE, DWORD) {
    return WAIT_OBJECT_0;
}
inline BOOL GetExitCodeProcess(HANDLE, DWORD*) {
    return 1;
}

/* ---- critical sections ------------------------------------------------ */
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline void EnterCriticalSection(CRITICAL_SECTION*) {}
inline void LeaveCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}

/* ---- registry --------------------------------------------------------- */
inline LONG RegOpenKeyExW(HKEY, const wchar_t*, DWORD, DWORD, HKEY*) {
    return 0;
}
inline LONG RegEnumKeyExW(HKEY, DWORD, wchar_t*, DWORD*, DWORD*, wchar_t*, DWORD*, FILETIME*) {
    return 0;
}
inline LONG RegQueryInfoKeyW(HKEY, wchar_t*, DWORD*, DWORD*, DWORD*, DWORD*, DWORD*, DWORD*, DWORD*,
                             DWORD*, DWORD*, FILETIME*) {
    return 0;
}
inline LONG RegQueryValueExW(HKEY, const wchar_t*, DWORD*, DWORD*, unsigned char*, DWORD*) {
    return 0;
}
inline LONG RegCloseKey(HKEY) {
    return 0;
}
inline LONG RegGetValueW(HKEY, const wchar_t*, const wchar_t*, DWORD, DWORD*, void*, DWORD*) {
    return 0;
}

/* ---- ntstatus mapping -------------------------------------------------- */
inline ULONG RtlNtStatusToDosError(long) {
    return 0;
}

/* ---- MSVC CRT surface (parse-gate only) ------------------------------- */
typedef int errno_t;
#define __int64 long long /* MSVC builtin 64-bit type; width-faithful */
inline errno_t _wfopen_s(FILE**, const wchar_t*, const wchar_t*) {
    return 0;
}
inline int _fseeki64(FILE*, __int64, int) {
    return 0;
}
inline __int64 _ftelli64(FILE*) {
    return 0;
}
inline BOOL SetConsoleCtrlHandler(BOOL(WINAPI*)(DWORD), BOOL) {
    return 1;
}
inline wchar_t* _wcsdup(const wchar_t*) {
    return nullptr;
}
inline errno_t wcsncpy_s(wchar_t*, size_t, const wchar_t*, size_t) {
    return 0;
}
inline errno_t wcscpy_s(wchar_t*, size_t, const wchar_t*) {
    return 0;
}
inline errno_t strncpy_s(char*, size_t, const char*, size_t) {
    return 0;
}
inline void* _aligned_malloc(size_t, size_t) {
    return nullptr;
}
inline void _aligned_free(void*) {}
inline char* _strupr_s(char*, size_t) {
    return nullptr;
}
inline FILE* _popen(const char*, const char*) {
    return nullptr;
}
inline int _pclose(FILE*) {
    return 0;
}
#define STILL_ACTIVE 259UL
#define VOLUME_NAME_DOS 0

/* ---- narrow/wide conversion (N-064 password-dictionary unlock aid) ---- */
#define CP_UTF8 65001U
inline int MultiByteToWideChar(UINT, DWORD, const char*, int, wchar_t*, int) {
    return 0;
}
