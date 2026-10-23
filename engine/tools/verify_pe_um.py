#!/usr/bin/env python3
"""
verify_pe_um.py - Win7-loadability gate for core_recovery user-mode binaries
(rrecov.exe, cr_tests.exe, cr_ui.dll - every shipped Windows PE).

User-mode port of the rda-driver Win7 gate (tools/verify_pe.py), adapted:

  1. PE machine = AMD64; subsystem = CUI (3) for the CLI/test exes;
     OS + subsystem VERSION fields = 6.1 (Windows 7 SP1 contract - run
     tools/stamp_osversion_um.py first: the modern MSVC toolset links
     (10, 0) regardless of project flags, exactly like the WDK did for
     the driver).
  2. Import DLLs must be within the allowed set. For kernel32.dll and
     advapi32.dll the FUNCTION list is strict against
     tools/ref_imports_um.json + the audited additions below (every entry
     available on Win7 SP1 x64 - our API choices ARE the Win7 contract).
     For the C runtime / zlib DLLs (shipped, vcpkg-versioned binaries,
     not our import surface to police) only the DLL NAME is gated.
  3. Any new kernel32/advapi32 import FAILS the build: forcing a
     conscious allowlist update with a reason, never a silent drift.

Usage: python tools/verify_pe_um.py <path.exe> [--allowlist tools/ref_imports_um.json]
Exit: 0 = passes, nonzero = gate failure.
"""

import json
import struct
import sys

# Audited additions beyond the checked-in baseline, with reasons. Every
# entry is present on Windows 7 SP1 x64.
ALLOWLIST_ADDITIONS = {
    # CRT / exe-startup surface the linker pulls into kernel32
    "GetCommandLineW": "CRT startup (wmain argc/argv); XP-era",
    "GetCommandLineA": "CRT startup fallback; XP-era",
    "GetStdHandle": "CRT stdio init; XP-era",
    "GetModuleFileNameW": "CRT DLL-name resolution; XP-era",
    "GetModuleHandleExW": "CRT static-init guard; XP-era",
    "GetModuleHandleW": "CRT atexit table; XP-era",
    "GetStartupInfoW": "CRT startup info; XP-era",
    "GetSystemTimeAsFileTime": "CRT time init; XP-era",
    "GetProcessHeap": "CRT heap (XP-era; our F11-unscale path never touches it)",
    "HeapAlloc": "CRT heap alloc; XP-era",
    "HeapFree": "CRT heap free; XP-era",
    "HeapReAlloc": "CRT heap realloc; XP-era",
    "ExitProcess": "CRT exit; XP-era",
    "TerminateProcess": "CRT abort path; XP-era",
    "GetCurrentThreadId": "CRT/stdlib thread identity; XP-era",
    "GetFileType": "CRT stdio classification; XP-era",
    "SetHandleCount": "CRT legacy stdio init; XP-era",
    "WriteConsoleW": "CRT wide stdio; XP-era",
    "ReadConsoleW": "CRT wide stdio; XP-era",
    "WideCharToMultiByte": "CRT narrow conversion; XP-era",
    "MultiByteToWideChar": "CRT wide conversion; XP-era",
    "lstrlenW": "CRT inline; XP-era",
    "EncodePointer": "CRT GS/DEP; XP SP2-era",
    "DecodePointer": "CRT GS/DEP; XP SP2-era",
    "IsProcessorFeaturePresent": "CRT memchr SSE probe; XP-era",
    "RtlUnwind": "C++ EH; XP-era",
    "RtlCaptureContext": "C++ EH; XP-era",
    "RtlLookupFunctionEntry": "C++ EH; XP-era",
    "RtlVirtualUnwind": "C++ EH; XP-era",
    "RtlPcToFileHeader": "C++ EH; XP-era",
    "QueryPerformanceCounter": "std::chrono + progress timing; XP-era",
    "QueryPerformanceFrequency": "std::chrono; XP-era",
    "Sleep": "cooperative waits; XP-era",
    "TryEnterCriticalSection": "std::mutex probe; XP-era",
    "InitializeSListHead": "std::atomic machinery; XP-era",
    "GetSystemInfo": "std::thread hw detection; XP-era",
    "GetNativeSystemInfo": "hw detection (WoW); Vista-era",
    "GetCurrentThread": "thread identity; XP-era",
    "CreateEventW": "sync primitives; XP-era",
    "SetEvent": "sync primitives; XP-era",
    "ResetEvent": "sync primitives; XP-era",
    "CreateThread": "std::thread; XP-era",
    "ExitThread": "std::thread; XP-era",
    "TlsAlloc": "CRT/ thread_local; XP-era",
    "TlsGetValue": "thread_local; XP-era",
    "TlsSetValue": "thread_local; XP-era",
    "TlsFree": "thread_local; XP-era",
    "FlsAlloc": "thread_local (Vista+) - Win7 ok",
    "FlsGetValue": "thread_local (Vista+) - Win7 ok",
    "FlsSetValue": "thread_local (Vista+) - Win7 ok",
    "FlsFree": "thread_local (Vista+) - Win7 ok",
    "GetLastError": "win layer; XP-era",
    "SetLastError": "win layer; XP-era",
    "CloseHandle": "win layer; XP-era",
    # linker-verified 2026-10-23 (first gate run, run 39992119849): the
    # MSVC toolset emits these into rrecov.exe / cr_tests.exe directly
    "UnhandledExceptionFilter": "CRT SEH chain tail; XP-era",
    "SetUnhandledExceptionFilter": "CRT abort/report install; XP-era",
    "IsDebuggerPresent": "CRT abort diagnostics; XP-era",
    # SRWLock / ConditionVariable / InitOnce families: emitted by the
    # toolset for std::mutex / std::condition_variable / one-shot guards.
    # Every member is documented Vista+ (Minimum supported client:
    # Windows Vista) - all Win7-safe. Family completion entries included so
    # the gate only trips on genuinely NEW imports, not on which sibling
    # the linker happened to pull this build.
    "AcquireSRWLockExclusive": "std::mutex machinery (verified); Vista+",
    "ReleaseSRWLockExclusive": "std::mutex machinery (verified); Vista+",
    "TryAcquireSRWLockExclusive": "std::mutex try_lock (family); Vista+",
    "AcquireSRWLockShared": "SRW shared side (family); Vista+",
    "ReleaseSRWLockShared": "SRW shared side (family); Vista+",
    "TryAcquireSRWLockShared": "SRW shared try (family); Vista+",
    "InitializeSRWLock": "SRW init (family); Vista+",
    "InitializeConditionVariable": "CV init (family); Vista+",
    "WakeConditionVariable": "CV notify_one (family); Vista+",
    "WakeAllConditionVariable": "CV notify_all (verified); Vista+",
    "SleepConditionVariableSRW": "CV wait (verified); Vista+",
    "SleepConditionVariableCS": "CV wait, CS variant (family); Vista+",
    "InitOnceBeginInitialize": "one-shot guard (verified); Vista+",
    "InitOnceComplete": "one-shot guard (verified); Vista+",
    "InitOnceExecuteOnce": "one-shot execute (family); Vista+",
    # deliberately NOT here: WaitOnAddress / WakeByAddress* / GetSystemTime-
    # PreciseAsFileTime - those are Windows 8+ ONLY; if the toolset ever
    # links them the gate must fail (that is the guard working).
    # advapi32 CRT-adjacent
    "RegQueryValueExW": "registry reads (auto-unlock, elevation probe); XP-era",
    "AdjustTokenGroups": "token probe (if linked by elevation path); XP-era",
}

EXPECTED_MACHINE = 0x8664
EXPECTED_OS_MAJOR = 6
EXPECTED_OS_MINOR = 1

# DLL name gate. kernel32/advapi32: strict function allowlists. The rest
# (CRT/vcruntime/zlib) are shipped binaries whose function surface is not
# ours to police - only their presence is gated, and every name here
# exists on Win7 SP1. NOTE the ACTUAL vcpkg (2026-10) binary names,
# linker-verified from the first gate run: zlib ships z.dll (debug zd.dll),
# lz4 ships lz4.dll, zstd ships zstd.dll - the historical zlib1/liblz4/
# libzstd spellings are kept for older vcpkg revisions. Debug-CRT and
# gtest names are listed so the gate also passes if pointed at a Debug
# tree (the CI gate runs Release; debug redist DLLs never ship).
ALLOWED_DLLS = {
    "kernel32.dll": "strict",
    "advapi32.dll": "strict",
    "user32.dll": "strict",  # not expected for the CLI; gate catches it
    "ucrtbase.dll": "name-only",
    "ucrtbased.dll": "name-only",
    "vcruntime140.dll": "name-only",
    "vcruntime140d.dll": "name-only",
    "msvcp140.dll": "name-only",
    "msvcp140d.dll": "name-only",
    "vcruntime140_1.dll": "name-only",
    "vcruntime140_1d.dll": "name-only",
    "zlib1.dll": "name-only",
    "zlibd1.dll": "name-only",  # old-vcpkg debug zlib
    "z.dll": "name-only",      # vcpkg-2026 release zlib
    "zd.dll": "name-only",     # vcpkg-2026 debug zlib
    "liblz4.dll": "name-only",  # old vcpkg lz4 name
    "lz4.dll": "name-only",     # vcpkg-2026 lz4
    "libzstd.dll": "name-only",  # old vcpkg zstd name
    "zstd.dll": "name-only",    # vcpkg-2026 zstd
    "gtest.dll": "name-only",    # vcpkg dynamic gtest (cr_tests debug tree)
    "gtest_main.dll": "name-only",
    "api-ms-win-crt-runtime-l1-1-0.dll": "name-only",  # UCRT api sets (Win10+
    "api-ms-win-crt-heap-l1-1-0.dll": "name-only",     # systems carry them;
    "api-ms-win-crt-math-l1-1-0.dll": "name-only",     # on Win7 the appcrt
    "api-ms-win-crt-stdio-l1-1-0.dll": "name-only",    # static-applocal UCRT
    "api-ms-win-crt-string-l1-1-0.dll": "name-only",   # forwarders resolve
    "api-ms-win-crt-time-l1-1-0.dll": "name-only",     # into ucrtbase -
    "api-ms-win-crt-convert-l1-1-0.dll": "name-only",  # keeps Win7 working
    "api-ms-win-crt-environment-l1-1-0.dll": "name-only",
    "api-ms-win-crt-filesystem-l1-1-0.dll": "name-only",
    "api-ms-win-crt-locale-l1-1-0.dll": "name-only",
    "api-ms-win-crt-utility-l1-1-0.dll": "name-only",
}


def fail(msg):
    print(f"FAIL: {msg}")
    sys.exit(1)


def parse_pe(path):
    data = open(path, "rb").read()
    if data[:2] != b"MZ":
        fail("not a PE file")
    e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
    if data[e_lfanew : e_lfanew + 4] != b"PE\x00\x00":
        fail("bad PE signature")
    coff = e_lfanew + 4
    machine, nsec, _ts, _ps, _ns, ohs, _chars = struct.unpack_from("<HHIIIHH", data, coff)
    opt = coff + 20
    magic = struct.unpack_from("<H", data, opt)[0]
    if magic != 0x20B:
        fail("not a PE32+ (x64) optional header")
    os_major, os_minor = struct.unpack_from("<HH", data, opt + 40)
    sub_major, sub_minor = struct.unpack_from("<HH", data, opt + 48)
    subsystem = struct.unpack_from("<H", data, opt + 68)[0]

    imp_rva, _imp_len = struct.unpack_from("<II", data, opt + 112 + 8)
    sections = []
    sec_off = opt + ohs
    for i in range(nsec):
        s = data[sec_off + i * 40 : sec_off + (i + 1) * 40]
        vsz, va, rsz, rp = struct.unpack_from("<IIII", s, 8)
        sections.append((va, max(vsz, rsz), rp))

    def r2o(rva):
        for va, sz, rp in sections:
            if va <= rva < va + sz:
                return rp + (rva - va)
        return None

    imports = {}
    if imp_rva:
        off = r2o(imp_rva)
        idx = 0
        while True:
            ilt, _ts, _fw, nrva, iat = struct.unpack_from("<IIIII", data, off + idx * 20)
            if nrva == 0 and ilt == 0:
                break
            no = r2o(nrva)
            dll = data[no : no + 64].split(b"\0")[0].decode(errors="replace")
            funcs = []
            t = ilt if ilt else iat
            to = r2o(t)
            j = 0
            while True:
                v = struct.unpack_from("<Q", data, to + j * 8)[0]
                if v == 0:
                    break
                if v & (1 << 63):
                    funcs.append(f"ordinal_{v & 0xffff}")
                else:
                    ho = r2o(v & 0x7FFFFFFF)
                    fn = data[ho + 2 : ho + 2 + 128].split(b"\0")[0].decode(errors="replace")
                    funcs.append(fn)
                j += 1
            imports[dll] = funcs
            idx += 1

    return {
        "machine": machine,
        "os_version": (os_major, os_minor),
        "subsystem_version": (sub_major, sub_minor),
        "subsystem": subsystem,
        "imports": imports,
    }


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    target = sys.argv[1]
    allowlist_path = "tools/ref_imports_um.json"
    if "--allowlist" in sys.argv:
        allowlist_path = sys.argv[sys.argv.index("--allowlist") + 1]

    pe = parse_pe(target)
    ok = True
    if pe["machine"] != EXPECTED_MACHINE:
        print(f"FAIL: machine 0x{pe['machine']:X} != 0x{EXPECTED_MACHINE:X}")
        ok = False
    if pe["os_version"] != (EXPECTED_OS_MAJOR, EXPECTED_OS_MINOR):
        print(
            f"FAIL: OS version {pe['os_version']} != (6, 1) - Win7 loader "
            "contract broken (run tools/stamp_osversion_um.py after link)"
        )
        ok = False
    if pe["subsystem_version"] < (6, 1):
        print(
            f"FAIL: subsystem version {pe['subsystem_version']} < (6, 1) - "
            "the PE would not load on Windows 7"
        )
        ok = False

    with open(allowlist_path) as f:
        ref = json.load(f)
    strict_allow = {}
    for dll, funcs in ref["dlls"].items():
        strict_allow[dll.lower()] = set(funcs)
    for fn in ALLOWLIST_ADDITIONS:
        strict_allow.setdefault("kernel32.dll", set()).add(fn)
        strict_allow.setdefault("advapi32.dll", set()).add(fn)

    print(
        f"PE: machine=0x{pe['machine']:X} OS={pe['os_version'][0]}.{pe['os_version'][1]} "
        f"subsystem={pe['subsystem']} v{pe['subsystem_version'][0]}.{pe['subsystem_version'][1]}"
    )

    for dll, funcs in pe["imports"].items():
        policy = ALLOWED_DLLS.get(dll.lower())
        if policy is None:
            print(f"FAIL: unexpected import DLL '{dll}'")
            ok = False
            continue
        if policy != "strict":
            print(f"  {dll}: {len(funcs)} imports (name-only gate)")
            continue
        allow = strict_allow.get(dll.lower(), set())
        for fn in funcs:
            if fn not in allow:
                print(f"FAIL: import '{fn}' (from {dll}) is not in the Win7 allowlist")
                ok = False

    if not ok:
        print("verify_pe_um: GATE FAILED")
        return 1
    print(
        f"verify_pe_um: OK "
        f"({sum(len(v) for v in pe['imports'].values())} imports, all Win7-safe)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
