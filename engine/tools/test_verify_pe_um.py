#!/usr/bin/env python3
"""
test_verify_pe_um.py - regression tests for the Win7 user-mode PE gate.

The gate only ever runs on the Windows CI lane against real MSVC output -
which means a broken gate script would surface as a confusing windows-lane
failure, and a too-permissive gate would surface NEVER. This self-test
synthesizes minimal-but-valid PE32+ images (the same structures
verify_pe_um.py parses: DOS header, COFF, PE32+ optional header, one
section, import descriptors/thunks/hint-names) and exercises both sides:

  PASS path  - the exact import surface of the 2026-10-23 MSVC link (the
               first real gate run, run 39992119849): census + additions
               incl. the SRWLock / ConditionVariable / InitOnce families
               and the actual vcpkg DLL names (z.dll / lz4.dll / zstd.dll).
  STAMP path - an unstamped (10, 0) PE FAILS the loader-contract check,
               then passes after tools/stamp_osversion_um.py runs (which
               also regression-tests the stamper's checksum math).
  FAIL paths - a Windows-8-only kernel32 import (WaitOnAddress) must FAIL;
               an un-allowlisted DLL (gdi32.dll) must FAIL; a user32.dll
               function import must FAIL (user32 is strict, not name-only).

Run:  python tools/test_verify_pe_um.py     (from the repo root)
Exit:  0 = all cases behave, 1 = a gate regression.
"""

import os
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
VERIFY = os.path.join(HERE, "verify_pe_um.py")
STAMP = os.path.join(HERE, "stamp_osversion_um.py")
ALLOWLIST = os.path.join(HERE, "ref_imports_um.json")

# The real 2026-10-23 MSVC import surface (run 39992119849 gate evidence +
# ref_imports_um.json census + the linker-verified additions).
REAL_KERNEL32 = [
    "CloseHandle", "CreateFileW", "DeviceIoControl", "GetOverlappedResult",
    "ReadFile", "WriteFile", "FlushFileBuffers", "GetFileSizeEx",
    "DeleteFileW", "CreateDirectoryW", "MoveFileExW", "GetFileAttributesW",
    "GetVolumeInformationW", "GetLogicalDriveStringsW", "GetDriveTypeW",
    "GetCurrentProcess", "GetCurrentProcessId", "GetTickCount64",
    "GetTempPathW", "SetConsoleCtrlHandler", "CreateProcessW",
    "GetExitCodeProcess", "WaitForSingleObject", "InitializeCriticalSection",
    "EnterCriticalSection", "LeaveCriticalSection", "DeleteCriticalSection",
    # CRT/startup + toolset pulls (ALLOWLIST_ADDITIONS)
    "GetCommandLineW", "GetCommandLineA", "GetStdHandle", "GetModuleFileNameW",
    "GetModuleHandleExW", "GetModuleHandleW", "GetStartupInfoW",
    "GetSystemTimeAsFileTime", "GetProcessHeap", "HeapAlloc", "HeapFree",
    "HeapReAlloc", "ExitProcess", "TerminateProcess", "GetCurrentThreadId",
    "GetFileType", "SetHandleCount", "WriteConsoleW", "ReadConsoleW",
    "WideCharToMultiByte", "MultiByteToWideChar", "lstrlenW", "EncodePointer",
    "DecodePointer", "IsProcessorFeaturePresent", "RtlUnwind",
    "RtlCaptureContext", "RtlLookupFunctionEntry", "RtlVirtualUnwind",
    "RtlPcToFileHeader", "QueryPerformanceCounter", "QueryPerformanceFrequency",
    "Sleep", "TryEnterCriticalSection", "InitializeSListHead", "GetSystemInfo",
    "GetNativeSystemInfo", "GetCurrentThread", "CreateEventW", "SetEvent",
    "ResetEvent", "CreateThread", "ExitThread", "TlsAlloc", "TlsGetValue",
    "TlsSetValue", "TlsFree", "FlsAlloc", "FlsGetValue", "FlsSetValue",
    "FlsFree", "GetLastError", "SetLastError",
    "UnhandledExceptionFilter", "SetUnhandledExceptionFilter",
    "IsDebuggerPresent", "AcquireSRWLockExclusive", "ReleaseSRWLockExclusive",
    "TryAcquireSRWLockExclusive", "AcquireSRWLockShared",
    "ReleaseSRWLockShared", "TryAcquireSRWLockShared", "InitializeSRWLock",
    "InitializeConditionVariable", "WakeConditionVariable",
    "WakeAllConditionVariable", "SleepConditionVariableSRW",
    "SleepConditionVariableCS", "InitOnceBeginInitialize", "InitOnceComplete",
    "InitOnceExecuteOnce",
]
REAL_ADVAPI32 = [
    "OpenProcessToken", "GetTokenInformation", "RegOpenKeyExW",
    "RegEnumValueW", "RegEnumKeyExW", "RegCloseKey", "RegQueryValueExW",
]
REAL_NAME_ONLY_DLLS = [
    "z.dll", "lz4.dll", "zstd.dll", "vcruntime140.dll", "vcruntime140_1.dll",
    "msvcp140.dll", "ucrtbase.dll", "api-ms-win-crt-runtime-l1-1-0.dll",
    "api-ms-win-crt-heap-l1-1-0.dll", "api-ms-win-crt-stdio-l1-1-0.dll",
]


def align(v, a):
    return (v + a - 1) & ~(a - 1)


def build_pe(imports, os_major=6, os_minor=1, subsystem=3,
             subsys_major=6, subsys_minor=1):
    """Synthesize a minimal valid PE32+ with the given import surface.

    imports: list of (dll_name, [function_names]) - name imports only.
    Layout: 0x200 file headers, section RVA 0x1000 at file offset 0x200.
    """
    sec_va = 0x1000
    sec_off = 0x200
    blob = bytearray()

    def rva():
        return sec_va + len(blob)

    # descriptors (filled after we know the layout) + thunks + names
    ndll = len(imports)
    desc_rva = rva()
    blob += b"\0" * (20 * (ndll + 1))
    entries = []
    for dll, funcs in imports:
        ilt_rva = rva()
        for _ in funcs:
            blob += b"\0" * 8
        blob += struct.pack("<Q", 0)  # null terminator
        iat_rva = rva()
        for _ in funcs:
            blob += b"\0" * 8
        blob += struct.pack("<Q", 0)
        name_rva = rva()
        blob += dll.encode("ascii") + b"\0"
        entries.append((ilt_rva, iat_rva, name_rva, funcs))

    # hint/name strings + patch the thunks with their RVAs
    for ilt_rva, iat_rva, name_rva, funcs in entries:
        thunk_rvas = []
        for fn in funcs:
            hn_rva = rva()
            blob += struct.pack("<H", 0) + fn.encode("ascii") + b"\0"
            if len(blob) % 2:  # hint/name entries are 2-aligned in real PEs
                blob += b"\0"
            thunk_rvas.append(hn_rva)
        for base in (ilt_rva, iat_rva):
            for i, tr in enumerate(thunk_rvas):
                off = base - sec_va + i * 8
                struct.pack_into("<Q", blob, off, tr)

    # patch descriptors: OriginalFirstThunk, TimeDateStamp, ForwarderChain,
    # Name, FirstThunk
    for i, (ilt_rva, iat_rva, name_rva, funcs) in enumerate(entries):
        struct.pack_into("<IIIII", blob, i * 20, ilt_rva, 0, 0, name_rva,
                         iat_rva)

    # --- headers ---
    hdr = bytearray(b"\0" * sec_off)
    hdr[0:2] = b"MZ"
    struct.pack_into("<I", hdr, 0x3C, 0x80)
    hdr[0x80:0x84] = b"PE\x00\x00"
    coff = 0x84
    ohs = 0xF0  # PE32+ optional header size
    struct.pack_into("<HHIIIHH", hdr, coff, 0x8664, 1, 0, 0, 0, ohs, 0)
    opt = coff + 20
    struct.pack_into("<H", hdr, opt, 0x20B)             # magic PE32+
    struct.pack_into("<HH", hdr, opt + 40, os_major, os_minor)
    struct.pack_into("<HH", hdr, opt + 48, subsys_major, subsys_minor)
    struct.pack_into("<H", hdr, opt + 68, subsystem)    # 3 = CUI
    struct.pack_into("<II", hdr, opt + 120, sec_va, len(blob))  # import dir
    # section table
    sec = opt + ohs
    hdr[sec:sec + 8] = b".idata\x00\x00"
    struct.pack_into("<IIII", hdr, sec + 8, len(blob), sec_va, len(blob),
                     sec_off)
    struct.pack_into("<I", hdr, sec + 16, 0x40000040)  # flags: r, init-data

    return bytes(hdr) + bytes(blob)


def run_gate(path):
    p = subprocess.run([sys.executable, VERIFY, path, "--allowlist", ALLOWLIST],
                       capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def run_stamp(path):
    p = subprocess.run([sys.executable, STAMP, path],
                       capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def main():
    failures = []

    def expect(cond, label, detail=""):
        if cond:
            print(f"  PASS: {label}")
        else:
            failures.append(label)
            print(f"  FAIL: {label}\n{detail}")

    real_imports = ([("KERNEL32.dll", REAL_KERNEL32), ("ADVAPI32.dll", REAL_ADVAPI32)]
                    + [(d, ["AnyFunction"]) for d in REAL_NAME_ONLY_DLLS])

    with tempfile.TemporaryDirectory() as td:
        # 1. the real 2026-10 MSVC surface passes as-linked (6,1)
        good = os.path.join(td, "real_surface.dll")
        with open(good, "wb") as f:
            f.write(build_pe(real_imports))
        rc, out = run_gate(good)
        expect(rc == 0, "real MSVC import surface passes the gate", out)

        # 2. unstamped (10,0) fails the loader contract, passes after stamp
        unstamped = os.path.join(td, "unstamped.dll")
        with open(unstamped, "wb") as f:
            f.write(build_pe(real_imports, os_major=10, os_minor=0,
                             subsys_major=10, subsys_minor=0))
        rc, out = run_gate(unstamped)
        expect(rc != 0 and "OS version (10, 0)" in out,
               "unstamped (10,0) PE is rejected", out)
        src, s_out = run_stamp(unstamped)
        rc, out = run_gate(unstamped)
        expect(src == 0 and rc == 0,
               "stamp_osversion_um.py produces a gate-passing PE", s_out + out)

        # 3. a Windows-8-only import must still FAIL (the guard bites)
        win8 = os.path.join(td, "win8_only.dll")
        with open(win8, "wb") as f:
            f.write(build_pe([("KERNEL32.dll", ["WaitOnAddress"])]))
        rc, out = run_gate(win8)
        expect(rc != 0 and "WaitOnAddress" in out,
               "Win8-only import (WaitOnAddress) is rejected", out)

        # 4. an unexpected DLL must FAIL
        rogue = os.path.join(td, "rogue_dll.dll")
        with open(rogue, "wb") as f:
            f.write(build_pe([("gdi32.dll", ["BitBlt"])]))
        rc, out = run_gate(rogue)
        expect(rc != 0 and "gdi32.dll" in out,
               "un-allowlisted DLL (gdi32.dll) is rejected", out)

        # 5. user32 is strict (function list), not name-only
        u32 = os.path.join(td, "user32_strict.dll")
        with open(u32, "wb") as f:
            f.write(build_pe([("USER32.dll", ["MessageBoxW"])]))
        rc, out = run_gate(u32)
        expect(rc != 0 and "MessageBoxW" in out,
               "user32.dll functions are gated strictly", out)

    print()
    if failures:
        print(f"test_verify_pe_um: {len(failures)} REGRESSION(S): {failures}")
        return 1
    print("test_verify_pe_um: all gate behaviors verified")
    return 0


if __name__ == "__main__":
    sys.exit(main())
