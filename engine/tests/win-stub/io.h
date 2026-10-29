/*
 * io.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: win_syntax_check.sh now parses the
 * Windows-only TEST files too, and <gtest/gtest.h> pulls <io.h> under
 * GTEST_OS_WINDOWS (gtest-port.h:401 - the testing::internal::posix
 * wrappers read/write/close/isatty). The stubs declare, never link or
 * run; shapes are SDK-faithful (MSVC UCRT io.h).
 */

#ifndef CR_WINSTUB_IO_H_
#define CR_WINSTUB_IO_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int _access(const char* path, int mode);
int _close(int fd);
int _isatty(int fd);
int _open(const char* path, int oflag, ...);
int _read(int fd, void* buffer, unsigned int count);
int _write(int fd, const void* buffer, unsigned int count);
long _lseek(int fd, long offset, int origin);
int _unlink(const char* path);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_IO_H_ */
