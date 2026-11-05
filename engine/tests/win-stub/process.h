/*
 * process.h - declaration-only Win32 CRT stub (tests/win-stub).
 *
 * Grown for the N-228 gate extension: tests/unit/test_tmpdir.hpp:23
 * includes <process.h> for _getpid on the Windows lane (POSIX getpid is
 * C4996-deprecated under MSVC). The stubs declare, never link or run.
 */

#ifndef CR_WINSTUB_PROCESS_H_
#define CR_WINSTUB_PROCESS_H_

#ifndef _WIN32
#error "win-stub headers are for the -D_WIN32 parse gate only"
#endif

#ifdef __cplusplus
extern "C" {
#endif

int _getpid(void);
/* __stdcall deliberately omitted: ELF g++ rejects the MSVC keyword and
 * it is a no-op on x64 (one calling convention); the MSVC lane compiles
 * the real SDK header. Parse-only, SDK-shaped otherwise. */
unsigned long _beginthreadex(void* security, unsigned int stack_size,
                             unsigned int (*start_address)(void*), void* arglist,
                             unsigned int initflag, unsigned int* thrdaddr);
void _endthreadex(unsigned int retval);

#ifdef __cplusplus
}
#endif

#endif /* CR_WINSTUB_PROCESS_H_ */
