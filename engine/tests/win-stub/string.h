/*
 * string.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: gtest-port.h:2099 calls the
 * MSVC-only _stricmp. The real glibc string.h rides through
 * include_next; the stub only adds the MSVC surface. The stubs declare,
 * never link or run.
 */

#ifndef CR_WINSTUB_STRING_H_
#define CR_WINSTUB_STRING_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int _stricmp(const char* lhs, const char* rhs);
int _strnicmp(const char* lhs, const char* rhs, size_t count);
char* _strdup(const char* source);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_STRING_H_ */
