/* shlwapi.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale.
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include "windows.h"

inline BOOL PathIsNetworkPathW(const wchar_t* /*pszPath*/) {
    return FALSE;
}
