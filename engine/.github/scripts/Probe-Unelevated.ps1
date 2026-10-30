# Probe-Unelevated.ps1 - the unelevated-refusal probe (session-96: the
# direct-access elevation contract).
#
# The runner shell is elevated by construction, so the refusal state is
# created the way the SHELL itself does it: CreateRestrictedToken(LUA_TOKEN)
# on our own process token (the UAC-filtered view - TokenElevation FALSE,
# exactly what is_elevated() probes), launched through
# CreateProcessWithTokenW (needs only SeImpersonatePrivilege, which the
# runner's service context holds; CreateProcessAsUserW needs
# SeAssignPrimaryTokenPrivilege - the GLE=5 class - and the scheduled-task
# route never launches a child in this session-0 runner context).
#
# The child (rrecov scan --disk N) must answer with the exit-4
# ERR_ELEVATION_REQUIRED refusal as its WHOLE response (no scan output may
# leak past the gate). Its stdout/stderr/exit code are captured through
# redirected pipes into <OutFile> (the evidence artifact).
#
# Exit: 0 = the refusal contract held; 1 = it did not (loud, with output).

param(
    [Parameter(Mandatory = $true)] [string] $RrecovExe,
    [Parameter(Mandatory = $true)] [string] $OutFile,
    [int] $Disk = 0,
    [int] $TimeoutSec = 60
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class Unelev {
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern bool OpenProcessToken(IntPtr p, uint acc, out IntPtr tok);
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern bool CreateRestrictedToken(IntPtr tok, uint flags,
        uint disSidCount, IntPtr disSids, uint delPrivCount, IntPtr delPrivs,
        uint resSidCount, IntPtr resSids, out IntPtr lua);
    [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    static extern bool CreateProcessWithTokenW(IntPtr tok, uint logonFlags,
        string app, string cmd, uint flags, IntPtr env, string cwd,
        ref STARTUPINFO si, out PROCESS_INFORMATION pi);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CreatePipe(out IntPtr read, out IntPtr write, IntPtr sa, uint size);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool SetHandleInformation(IntPtr h, uint mask, uint flags);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool ReadFile(IntPtr h, byte[] buf, uint toRead, out uint read, IntPtr ov);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern uint WaitForSingleObject(IntPtr h, uint ms);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetExitCodeProcess(IntPtr h, out uint code);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(IntPtr h);
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct STARTUPINFO {
        public int cb; public string lpReserved, lpDesktop, lpTitle;
        public int dwX, dwY, dwXSize, dwYSize, dwXCountChars, dwYCountChars,
                   dwFillAttribute, dwFlags;
        public short wShowWindow, cbReserved2;
        public IntPtr lpReserved2, hStdInput, hStdOutput, hStdError;
    }
    [StructLayout(LayoutKind.Sequential)]
    public struct PROCESS_INFORMATION {
        public IntPtr hProcess, hThread; public int dwProcessId, dwThreadId;
    }
    public static string Run(string app, string args) {
        IntPtr tok;
        if (!OpenProcessToken(System.Diagnostics.Process.GetCurrentProcess().Handle,
                0x0002 | 0x0008 | 0x0020, out tok))
            throw new Exception("OpenProcessToken GLE=" + Marshal.GetLastWin32Error());
        IntPtr lua;
        if (!CreateRestrictedToken(tok, 0x00000001 /* LUA_TOKEN */,
                0, IntPtr.Zero, 0, IntPtr.Zero, 0, IntPtr.Zero, out lua))
            throw new Exception("CreateRestrictedToken GLE=" + Marshal.GetLastWin32Error());
        IntPtr outR, outW;
        if (!CreatePipe(out outR, out outW, IntPtr.Zero, 0))
            throw new Exception("CreatePipe GLE=" + Marshal.GetLastWin32Error());
        SetHandleInformation(outR, 0x00000001 /* HANDLE_FLAG_INHERIT */, 0);
        STARTUPINFO si = new STARTUPINFO();
        si.cb = Marshal.SizeOf(typeof(STARTUPINFO));
        si.dwFlags = 0x00000100; /* STARTF_USESTDHANDLES */
        si.hStdOutput = outW; si.hStdError = outW;
        PROCESS_INFORMATION pi;
        string cmd = "\"" + app + "\" " + args;
        if (!CreateProcessWithTokenW(lua, 0, null, cmd,
                0x08000000 /* CREATE_NO_WINDOW */, IntPtr.Zero, null, ref si, out pi))
            throw new Exception("CreateProcessWithTokenW GLE=" + Marshal.GetLastWin32Error());
        CloseHandle(outW); CloseHandle(lua); CloseHandle(tok);
        var buf = new byte[65536]; var text = new System.Text.StringBuilder();
        uint read;
        while (ReadFile(outR, buf, (uint)buf.Length, out read, IntPtr.Zero) && read > 0)
            text.Append(System.Text.Encoding.UTF8.GetString(buf, 0, (int)read));
        CloseHandle(outR);
        WaitForSingleObject(pi.hProcess, 60000);
        uint code;
        GetExitCodeProcess(pi.hProcess, out code);
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        return "EXITCODE=" + code + "\r\n" + text.ToString();
    }
}
'@

try {
    $out = [Unelev]::Run($RrecovExe, "scan --disk $Disk")
} catch {
    Write-Host "::error::the unelevated launch failed: $($_.Exception.Message)"
    exit 1
}
$out | Set-Content -LiteralPath $OutFile -Encoding utf8

$failed = $false
if ($out -match 'EXITCODE=4' -and $out -match 'administrator privilege required') {
    Write-Host "PASS unelevated scan --disk ${Disk}: exit 4, the ERR_ELEVATION_REQUIRED refusal"
} else {
    Write-Host "::error::unelevated scan --disk ${Disk} did not refuse; output: $out"
    $failed = $true
}
if ($out -match 'starting scan|\[transport\]|\[stage') {
    Write-Host '::error::the refusal must be the whole response - scan output leaked past the privilege gate'
    $failed = $true
}
if ($failed) { exit 1 }
exit 0
