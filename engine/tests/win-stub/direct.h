/*
 * direct.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: win_syntax_check.sh now parses the
 * Windows-only TEST files too, and <gtest/gtest.h> pulls <direct.h> under
 * GTEST_OS_WINDOWS (gtest-port.h:400). The stubs declare, never link or
 * run; shapes are SDK-faithful (MSVC UCRT direct.h).
 */

#ifndef CR_WINSTUB_DIRECT_H_
#define CR_WINSTUB_DIRECT_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#ifdef __cplusplus
extern "C" {
#endif

char* _getcwd(char* buffer, unsigned int length);
int _chdir(const char* path);
int _mkdir(const char* path);
int _rmdir(const char* path);
unsigned int _getdiskfree(unsigned int drive, void* info);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_DIRECT_H_ */
