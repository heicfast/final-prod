/*
 * stdio.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: the Windows-only TEST files pull
 * <gtest/gtest.h>, whose gtest-port.h calls the MSVC-only underscore
 * CRT spellings (_fileno :2042, _wfopen :2143). The real glibc stdio.h
 * rides through include_next; the stub only adds the MSVC surface.
 * The stubs declare, never link or run.
 */

#ifndef CR_WINSTUB_STDIO_H_
#define CR_WINSTUB_STDIO_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#include_next <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int _fileno(FILE* stream);
FILE* _wfopen(const wchar_t* filename, const wchar_t* mode);
int _wremove(const wchar_t* filename);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_STDIO_H_ */
