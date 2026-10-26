/* cr_elevate.cpp - definitive process-elevation probe (GAP-P1-5).
 *
 * Licensed under the MIT license. See LICENSE.
 */

#include "cr/win/cr_elevate.h"
#include <windows.h>

namespace cr {
namespace win {

bool is_elevated() {
    HANDLE tok = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok))
        return false;
    TOKEN_ELEVATION elev = {};
    DWORD got = 0;
    const BOOL ok = GetTokenInformation(tok, TokenElevation, &elev, sizeof(elev), &got);
    CloseHandle(tok);
    return ok != FALSE && got >= sizeof(elev) && elev.TokenIsElevated != 0;
}

} // namespace win
} // namespace cr
