# Probe-Unelevated.ps1 - the unelevated-refusal probe (session-96: the
# direct-access elevation contract).
#
# The runner shell is elevated by construction, and the runner's job
# process holds NEITHER SeAssignPrimaryTokenPrivilege (CreateProcessAsUserW
# -> GLE=5) NOR SeImpersonatePrivilege (CreateProcessWithTokenW -> GLE=5),
# and the scheduled-task route never launches a child in this session-0
# context. The privilege-free primitive that DOES work:
# CreateProcessWithLogonW - it needs only credentials, and a freshly
# created LOCAL STANDARD USER is non-elevated by definition (TokenElevation
# FALSE - exactly what is_elevated() probes).
#
# The probe: create the local user, launch rrecov scan --disk N as that
# user with redirected stdout/stderr, capture the exit code + output into
# <OutFile> (the evidence artifact), delete the user. The child must
# answer with the exit-4 ERR_ELEVATION_REQUIRED refusal as its WHOLE
# response (no scan output may leak past the gate).
#
# Exit: 0 = the refusal contract held; 1 = it did not (loud, with output).

param(
    [Parameter(Mandatory = $true)] [string] $RrecovExe,
    [Parameter(Mandatory = $true)] [string] $OutFile,
    [int] $Disk = 0
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class AsUser {
    [DllImport("advapi32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    static extern bool CreateProcessWithLogonW(string username, string domain, string password,
        uint logonFlags, string app, string cmd, uint flags, IntPtr env, string cwd,
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
    public static string Run(string user, string pass, string app, string args) {
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
        if (!CreateProcessWithLogonW(user, ".", pass, 0, null, cmd,
                0, IntPtr.Zero, null, ref si, out pi))
            throw new Exception("CreateProcessWithLogonW GLE=" + Marshal.GetLastWin32Error());
        CloseHandle(outW);
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

# ---- the local standard user (non-elevated by definition) -------------------
$user = 'cr_unelev_probe'
$pass = 'Cr-' + (-join ((65..90) + (97..122) | Get-Random -Count 12 | ForEach-Object { [char]$_ }))
& net user $user $pass /add /y 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "::error::could not create the probe user (net user exit $LASTEXITCODE)"
    exit 1
}
& net localgroup Users $user /add 2>&1 | Out-Null
try {
    # the evidence path must be writable by the standard user: a dedicated
    # temp dir with an explicit Everyone grant
    $probeDir = Join-Path $env:RUNNER_TEMP 'cr-unelev-dir'
    New-Item -ItemType Directory -Force -Path $probeDir | Out-Null
    $evidence = Join-Path $probeDir 'probe-output.txt'
    & icacls $probeDir /grant 'Everyone:(OI)(CI)F' 2>&1 | Out-Null

    $out = [AsUser]::Run($user, $pass, $RrecovExe, "scan --disk $Disk")

    $out | Set-Content -LiteralPath $evidence -Encoding utf8
    try { Copy-Item -LiteralPath $evidence -Destination $OutFile -Force -ErrorAction Stop } catch { $out | Set-Content -LiteralPath $OutFile -Encoding utf8 }
} finally {
    & net user $user /delete /y 2>&1 | Out-Null
}

$failed = $false
if ($out -match 'EXITCODE=4' -and $out -match 'administrator privilege required') {
    Write-Host "PASS unelevated scan --disk ${Disk}: exit 4, the ERR_ELEVATION_REQUIRED refusal (local standard user)"
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
