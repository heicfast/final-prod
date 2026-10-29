/*
 * stdlib.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: tests/unit/test_destio_win.cpp:202
 * calls _wsystem (the MSVC wide-command shell spawn; the junction
 * staging probe). The real glibc stdlib.h rides through include_next;
 * the stub only adds the MSVC surface. The stubs declare, never link or
 * run.
 */

#ifndef CR_WINSTUB_STDLIB_H_
#define CR_WINSTUB_STDLIB_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#include_next <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int _wsystem(const wchar_t* command);
int _wremove(const wchar_t* path);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_STDLIB_H_ */
