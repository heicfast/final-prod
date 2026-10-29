/*
 * sys/stat.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: gtest-port.h:2035-2046 declares
 * StatStruct as MSVC's struct _stat and reads st_mode via _S_IFDIR; the
 * glibc sys/stat.h (include_next) provides the POSIX shapes this stub
 * aliases. The stubs declare, never link or run.
 */

#ifndef CR_WINSTUB_SYS_STAT_H_
#define CR_WINSTUB_SYS_STAT_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#include_next <sys/stat.h>
/* glibc's sys/stat.h defines time_t only under __USE_XOPEN guards, and
 * gtest's Windows include branch never pulls it - the real <time.h>
 * defines it unconditionally (MSVC's _stat members are time_t). */
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MSVC's underscore spellings alias the POSIX masks the glibc header
 * just defined - one source of truth, the SDK-faithful values. */
#ifndef _S_IFDIR
#define _S_IFDIR S_IFDIR
#endif
#ifndef _S_IFMT
#define _S_IFMT S_IFMT
#endif
#ifndef _S_IFCHR
#define _S_IFCHR S_IFCHR
#endif
#ifndef _S_IFREG
#define _S_IFREG S_IFREG
#endif

/* glibc's POSIX.1-2008 accessors (#define st_atime st_atim.tv_sec and
 * friends) collide with MSVC's _stat member names - drop them for the
 * struct declaration, restore the glibc surface after (the gate never
 * touches the fields; the shapes stay SDK-faithful on both sides). */
#undef st_atime
#undef st_mtime
#undef st_ctime

/* The MSVC _stat layout (UCRT corecrt_struct_stat.h shape; gtest reads
 * st_mode only, the parse gate wants the full SDK-faithful shape). */
struct _stat {
    unsigned int st_dev;
    unsigned short st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    unsigned int st_rdev;
    long st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
};

/* restore glibc's own accessor macros (the pre-stub state) */
#define st_atime st_atim.tv_sec
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec

int _stat(const char* path, struct _stat* buffer);
int _wstat(const wchar_t* path, struct _stat* buffer);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_SYS_STAT_H_ */
