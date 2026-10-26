/* share.h - DECLARATION-ONLY stub for the Linux-side win-layer parse
 * gate (M-055). See tests/win-stub/windows.h for the rationale. The real
 * share.h is a UCRT header carrying the _SH_* share-mode flags used by
 * _fsopen/_wfsopen; this stub re-declares exactly the surface src/ui
 * consumes (S-008: the shared-read image opens).
 *
 * Licensed under the MIT license. See LICENSE.
 */
#pragma once

#include "windows.h"
#include <cstdio>

/* SDK shapes (share.h): the share-mode constants. _SH_DENYNO = 0x40. */
#define _SH_DENYWR 0x20
#define _SH_DENYNO 0x40

/* CRT (stdio) import; the parse gate never links. */
inline FILE* _wfsopen(const wchar_t* /*filename*/, const wchar_t* /*mode*/, int /*shflag*/) {
    return nullptr;
}
