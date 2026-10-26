/* psapi.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale. The real
 * psapi.h is a Windows SDK header; this stub re-declares exactly the
 * surface src/ui consumes (N-062: the working-set sample) with
 * SDK-faithful shapes.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include "windows.h"

/* SDK shape (psapi.h): the counters struct the sample reads. Only the
 * fields the consumer touches are laid out; the cb prefix and the
 * DWORD-sized fields follow the real header's widths. */
typedef struct _PROCESS_MEMORY_COUNTERS {
    DWORD cb;
    DWORD PageFaultCount;
    size_t PeakWorkingSetSize;
    size_t WorkingSetSize;
    size_t QuotaPeakPagedPoolUsage;
    size_t QuotaPagedPoolUsage;
    size_t QuotaPeakNonPagedPoolUsage;
    size_t QuotaNonPagedPoolUsage;
    size_t PagefileUsage;
    size_t PeakPagefileUsage;
} PROCESS_MEMORY_COUNTERS;

/* psapi.h's WINAPI (stdlib) import; the parse gate never links. */
inline BOOL GetProcessMemoryInfo(HANDLE /*process*/, PROCESS_MEMORY_COUNTERS* /*counters*/,
                                 DWORD /*cb*/) {
    return FALSE;
}
