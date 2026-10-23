# og-driver.ps1 - UI automation library for the og EaseUS DRW free build.
#
# Adapted from the PROVEN driver in easeus-drw-research/.github/workflows/
# ui-screenshot-atlas.yml (pixel-verified coordinates, restore-resilient clicks,
# gate-aware dismissal, browser killer). The og is driven through its real UI:
#   launch via DRW.exe (the loader - DRWUI.exe never launched directly),
#   window class Qt5QWindowIcon, title "EaseUS Data Recovery Wizard".
#
# Every interaction is logged and screenshotted. Nothing is claimed without a
# screenshot artifact. See corpus artifacts/UIUX/35-redesign-reference-atlas.md
# for the coordinate provenance.
#
# Licensed under the MIT license. See LICENSE.

$ErrorActionPreference = 'Stop'

if (-not ('OgAH' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class OgAH {
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint f, uint dx, uint dy, uint dd, UIntPtr e);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int cmd);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    public static void Click(int x, int y) {
        SetCursorPos(x, y);
        mouse_event(0x0002, 0, 0, 0, UIntPtr.Zero);
        mouse_event(0x0004, 0, 0, 0, UIntPtr.Zero);
    }
    public static void RClick(int x, int y) {
        SetCursorPos(x, y);
        mouse_event(0x0008, 0, 0, 0, UIntPtr.Zero);
        mouse_event(0x0010, 0, 0, 0, UIntPtr.Zero);
    }
    public static void Esc() {
        keybd_event(0x1B, 0, 0, UIntPtr.Zero);
        keybd_event(0x1B, 0, 2, UIntPtr.Zero);
    }
    public static void Type(string s) {
        foreach (char c in s) {
            byte vk = (byte)(c & 0xFF);
            if (c >= 'a' && c <= 'z') vk = (byte)((int)c - 32);
            keybd_event(vk, 0, 0, UIntPtr.Zero);
            keybd_event(vk, 0, 2, UIntPtr.Zero);
        }
    }
}
"@
}

# ---- window helpers ---------------------------------------------------------------

function Get-OgMainRect {
    $drw = Get-Process DRWUI -ErrorAction SilentlyContinue |
        Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if (-not $drw) { return $null }
    $r = New-Object OgAH+RECT
    [OgAH]::GetWindowRect($drw.MainWindowHandle, [ref]$r) | Out-Null
    return $r
}

function Restore-OgWin {
    $drw = Get-Process DRWUI -ErrorAction SilentlyContinue |
        Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
    if ($drw) {
        $h = $drw.MainWindowHandle
        if ([OgAH]::IsIconic($h)) { [OgAH]::ShowWindow($h, 9) | Out-Null }
        [OgAH]::SetForegroundWindow($h) | Out-Null
        Start-Sleep -Milliseconds 400
    }
}

function Invoke-OgClickRel {
    param([int] $X, [int] $Y, [string] $Note = '')
    Restore-OgWin
    $r = Get-OgMainRect
    if ($r) {
        [OgAH]::Click($r.L + $X, $r.T + $Y)
        Write-Host "click ($X,$Y) $Note"
        return $true
    }
    Write-Host "click ($X,$Y) SKIPPED - no og window $Note"
    return $false
}

function Stop-OgBrowsers {
    Stop-Process -Name msedge, iexplore, microsoftedge -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 400
}

# the retention nag gate (X at window-relative 809,135) + single ESC + browser kill:
# the proven gate-dismissal sequence from the atlas driver
function Invoke-OgGateDismiss {
    Invoke-OgClickRel 809 135 '(gate X)' | Out-Null
    Start-Sleep -Milliseconds 600
    [OgAH]::Esc()
    Start-Sleep -Milliseconds 400
    Stop-OgBrowsers
    Restore-OgWin
}

function New-OgShot {
    param([string] $Name, [string] $Dir)
    Restore-OgWin
    try {
        Add-Type -AssemblyName System.Windows.Forms, System.Drawing
        $b = [System.Windows.Forms.SystemInformation]::VirtualScreen
        $bmp = New-Object System.Drawing.Bitmap($b.Width, $b.Height)
        $g = [System.Drawing.Graphics]::FromImage($bmp)
        $g.CopyFromScreen($b.X, $b.Y, 0, 0, $bmp.Size)
        $safe = $Name -replace '[^A-Za-z0-9_.-]', '_'
        $bmp.Save((Join-Path $Dir "$safe.png"), [System.Drawing.Imaging.ImageFormat]::Png)
        $g.Dispose(); $bmp.Dispose()
        return $true
    } catch {
        Write-Host "shot $Name FAILED: $($_.Exception.Message)"
        return $false
    }
}

if (-not ('OgWE' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class OgWE {
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc cb, IntPtr lp);
    [DllImport("user32.dll")] static extern int GetWindowText(IntPtr h, StringBuilder sb, int n);
    [DllImport("user32.dll")] static extern int GetClassName(IntPtr h, StringBuilder sb, int n);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    delegate bool EnumWindowsProc(IntPtr h, IntPtr lp);
    public static List<string> Results = new List<string>();
    static bool Cb(IntPtr h, IntPtr lp) {
        uint pid; GetWindowThreadProcessId(h, out pid);
        if (IsWindowVisible(h)) {
            var t = new StringBuilder(256); GetWindowText(h, t, 256);
            var c = new StringBuilder(256); GetClassName(h, c, 256);
            RECT r; GetWindowRect(h, out r);
            if (t.Length > 0)
                Results.Add(string.Format("pid={0} hwnd=0x{1:X} class={2} title={3} rect=({4},{5})-({6},{7})",
                    pid, h.ToInt64(), c, t, r.L, r.T, r.R, r.B));
        }
        return true;
    }
    public static List<string> Go() { Results.Clear(); EnumWindows(Cb, IntPtr.Zero); return Results; }
}
"@
}

function Get-OgWindowCensus {
    try { return @([OgWE]::Go()) } catch { return @("census-failed: $($_.Exception.Message)") }
}

# ---- app lifecycle ----------------------------------------------------------------

function Start-OgApp {
    param([string] $WorkDir)
    $proc = Start-Process -FilePath (Join-Path $WorkDir 'DRW.exe') -WorkingDirectory $WorkDir -PassThru
    Write-Host "launched DRW.exe pid=$($proc.Id)"
    return $proc
}

function Stop-OgApp {
    Stop-Process -Name DRWUI -Force -ErrorAction SilentlyContinue
    Stop-Process -Name DRW -Force -ErrorAction SilentlyContinue
    Stop-Process -Name AliyunWrapExe -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
}

function Wait-OgLanding {
    param([int] $TimeoutSec = 150, [string] $ShotDir)
    $t0 = Get-Date
    $i = 0
    while (((Get-Date) - $t0).TotalSeconds -lt $TimeoutSec) {
        $drw = Get-Process DRWUI -ErrorAction SilentlyContinue |
            Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
        if ($drw -and $drw.MainWindowTitle -match 'EaseUS Data Recovery') {
            Write-Host "og landing ready after $([int]((Get-Date) - $t0).TotalSeconds)s (title: $($drw.MainWindowTitle))"
            Start-Sleep -Seconds 3
            if ($ShotDir) { New-OgShot "01_landing" $ShotDir | Out-Null }
            return $true
        }
        Start-Sleep -Seconds 5
        $i++
        if ($ShotDir -and ($i % 3 -eq 0)) { New-OgShot ("00_wait_{0:d2}" -f $i) $ShotDir | Out-Null }
    }
    return $false
}

# ---- free-cap state (client-side, corpus-documented) -------------------------------

function Get-OgQuota {
    $q = [ordered]@{}
    try {
        $k = 'HKLM:\SOFTWARE\DRWNewFree\Metadata'
        if (Test-Path $k) {
            $q.enable_total_size = (Get-ItemProperty -Path $k -ErrorAction SilentlyContinue).EnableTotalSize
            $q.enable_total_file_count = (Get-ItemProperty -Path $k -ErrorAction SilentlyContinue).EnableTotalFileCount
            $q.recovered_total_size_before = (Get-ItemProperty -Path $k -ErrorAction SilentlyContinue).RecoveredTotalSize
            $q.recovered_total_file_count_before = (Get-ItemProperty -Path $k -ErrorAction SilentlyContinue).RecoveredTotalFileCount
        } else { $q.key_missing = $true }
    } catch { $q.error = $_.Exception.Message }
    return $q
}

function Reset-OgState {
    param([string] $CorpusDir, [string] $PristineIni)
    Stop-OgApp
    # client-side quota state: registry + recoveryFree.ini
    if (Test-Path 'HKLM:\SOFTWARE\DRWNewFree') {
        Remove-Item 'HKLM:\SOFTWARE\DRWNewFree' -Recurse -Force -ErrorAction SilentlyContinue
    }
    if ($PristineIni -and (Test-Path $PristineIni)) {
        Copy-Item $PristineIni (Join-Path $CorpusDir 'recoveryFree.ini') -Force
    }
    # remove any stale scan sessions/logs from prior scenarios (fresh-launch semantics)
    Get-ChildItem (Join-Path $CorpusDir 'SaveScan') -Filter '*.rsf' -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
    Write-Host 'og state reset: DRWNewFree registry removed, recoveryFree.ini restored, SaveScan cleared'
}

function Get-OgRsfBaseline {
    param([string] $CorpusDir)
    $ss = Join-Path $CorpusDir 'SaveScan'
    if (-not (Test-Path $ss)) { return @() }
    return @(Get-ChildItem $ss -Filter '*.rsf' -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
}

function Wait-OgRsf {
    param([array] $Baseline, [string] $CorpusDir, [int] $TimeoutSec, [string] $ShotDir, [string] $SamplesCsv)
    $ss = Join-Path $CorpusDir 'SaveScan'
    $t0 = Get-Date
    $lastTitle = ''
    $i = 0
    $samples = New-Object System.Collections.Generic.List[string]
    $samples.Add('t_ms,working_set_bytes,cpu_seconds')
    while (((Get-Date) - $t0).TotalSeconds -lt $TimeoutSec) {
        # completion signal: a NEW auto_*.rsf appears (auto-saved at scan completion)
        $now = @(Get-ChildItem $ss -Filter '*.rsf' -ErrorAction SilentlyContinue | ForEach-Object { $_.FullName })
        $new = @($now | Where-Object { $Baseline -notcontains $_ })
        if ($new.Count -gt 0) {
            Start-Sleep -Seconds 5   # let the completion modal settle
            if ($ShotDir) { New-OgShot '10_scan_complete_modal' $ShotDir | Out-Null }
            $samples | Set-Content -Path $SamplesCsv -Encoding utf8
            return @{ rsf = $new[0]; all_new = $new; seconds = [math]::Round(((Get-Date) - $t0).TotalSeconds, 1) }
        }
        # sampling + gate dismissal + evidence
        try {
            $procs = @(Get-Process DRWUI -ErrorAction SilentlyContinue)
            if ($procs.Count -gt 0) {
                $age = [int]((Get-Date) - $t0).TotalMilliseconds
                $ws = 0; $cpu = 0.0
                foreach ($p in $procs) { $ws += $p.WorkingSet64; $cpu += $p.CPU }
                $samples.Add(("{0},{1},{2:F3}" -f $age, $ws, $cpu))
            }
        } catch { }
        $p1 = Get-Process DRWUI -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($p1 -and $p1.MainWindowTitle -and $p1.MainWindowTitle -ne $lastTitle) {
            Write-Host "og title: $($p1.MainWindowTitle)"
            $lastTitle = $p1.MainWindowTitle
        }
        Invoke-OgGateDismiss
        $i++
        if ($ShotDir -and ($i % 4 -eq 0)) { New-OgShot ("05_scan_{0:d2}" -f $i) $ShotDir | Out-Null }
        Start-Sleep -Seconds 15
    }
    $samples | Set-Content -Path $SamplesCsv -Encoding utf8
    return $null
}

function Get-OgLogs {
    param([string] $CorpusDir, [string] $OutDir)
    $copied = @()
    foreach ($rel in @('log\DRWUI.log', 'log\CalcPartition.log', 'log\DefaultLog.txt', 'log\DRWLog.log',
                       'EPMLOG.log', 'Eaolog.log')) {
        $src = Join-Path $CorpusDir $rel
        if (Test-Path $src) {
            $dst = Join-Path $OutDir ($rel -replace '\\', '_')
            Copy-Item $src $dst -Force -ErrorAction SilentlyContinue
            $copied += $rel
        }
    }
    return $copied
}
