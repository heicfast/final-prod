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
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool IsWindow(IntPtr h);
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

# ---- nag/gate dismissal (corpus-evidenced) -----------------------------------------
# The Free build raises PERIODIC retention nags (RetentionActiveDlg, every
# 30-60 s of active use on ANY page - corpus atlas 35 #15) as separate
# top-level Qt windows (class Qt5QWindowIcon, title 'DRWUI', ~540x380,
# position VARIES run to run). Run 40048192077 evidence: the nag at
# (232,134)-(772,514) swallowed every landing-tile click; the old fixed
# (809,135) X-click missed it by 37 px, and the blind ESC provokes the
# EXIT-retention gate on landing (corpus atlas: exit-attempt dialog).
# Robust dismissal: enumerate the og pid's non-main Qt windows and click
# EACH dialog's own X (atlas-verified offset r-11, t+24 from its rect),
# verify it is gone, fall back to WM_CLOSE.
function Get-OgNagWindows {
    $nags = @()
    $pids = @(Get-Process DRWUI -ErrorAction SilentlyContinue | ForEach-Object { $_.Id })
    if ($pids.Count -eq 0) { return $nags }
    foreach ($line in (Get-OgWindowCensus)) {
        if ($line -match 'pid=(\d+) hwnd=(0x[0-9A-F]+) class=(Qt5QWindowIcon) title=(.+?) rect=\((-?\d+),(-?\d+)\)-\((-?\d+),(-?\d+)\)') {
            $wpid = [int]$Matches[1]
            if ($pids -notcontains $wpid) { continue }
            $title = $Matches[4].Trim()
            if ($title -ne 'DRWUI') { continue }   # main window is 'EaseUS Data Recovery Wizard'
            $nags += [pscustomobject]@{
                hwnd = [IntPtr][Convert]::ToInt64($Matches[2].Substring(2), 16)
                l = [int]$Matches[5]; t = [int]$Matches[6]; r = [int]$Matches[7]; b = [int]$Matches[8]
                line = $line
            }
        }
    }
    return $nags
}

function Invoke-OgNagDismiss {
    param([int] $MaxRounds = 3)
    $dismissed = @()
    for ($round = 1; $round -le $MaxRounds; $round++) {
        $nags = @(Get-OgNagWindows)
        if ($nags.Count -eq 0) { break }
        foreach ($n in $nags) {
            # X close at the dialog's own top-right (atlas: X at rect.r-11, rect.t+24)
            Restore-OgWin
            [OgAH]::Click(($n.r - 11), ($n.t + 24))
            Start-Sleep -Milliseconds 900
            # still there? send WM_CLOSE to that exact hwnd
            $still = @(Get-OgNagWindows | Where-Object { $_.line -eq $n.line })
            if ($still.Count -gt 0 -and [OgAH]::IsWindow($n.hwnd)) {
                [OgAH]::PostMessage($n.hwnd, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
                Start-Sleep -Milliseconds 900
            }
            $dismissed += $n.line
        }
    }
    if ($dismissed.Count -gt 0) {
        Start-Sleep -Milliseconds 500
        Restore-OgWin
        Write-Host "nag-dismiss: $($dismissed.Count) dialog(s): $($dismissed -join ' | ')"
    }
    return $dismissed.Count
}

# the retention nag gate + browser killer. NO blind ESC: on the landing page
# ESC is an exit attempt and PROVOKES the exit-retention gate (corpus atlas
# 35 #15; run 40048192077 screenshots 02_select_attempt_01..06).
function Invoke-OgGateDismiss {
    Invoke-OgNagDismiss | Out-Null
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

# ---- titled-window lookup (BENCH-OG-F1) -------------------------------------------
# Run 40073093997 evidence (window-census-final.txt): the destination dialog
# is its own top-level window class=Qt5QWindowIcon title=ChooseDestinationPathDialog,
# and the 500 MB free-cap gate is class=Qt5QWindowIcon title=Dialog. The old
# cap-gate detector ('census.Count -gt 2') FALSE-FIRED on the destination
# dialog when the Save click missed -> 'Give Up' clicked into the destination
# dialog -> the flow deadlocked 1200 s and recovered nothing (og permanent_
# delete 0.0% was an automation defect, not og behavior).
function Get-OgWindowByTitle {
    param([string] $Title)
    $pids = @(Get-Process DRWUI -ErrorAction SilentlyContinue | ForEach-Object { $_.Id })
    if ($pids.Count -eq 0) { return $null }
    foreach ($line in (Get-OgWindowCensus)) {
        if ($line -match 'pid=(\d+) hwnd=(0x[0-9A-F]+) class=(Qt5QWindow\w*) title=(.+?) rect=\((-?\d+),(-?\d+)\)-\((-?\d+),(-?\d+)\)') {
            if ($pids -notcontains [int]$Matches[1]) { continue }
            if ($Matches[4].Trim() -ne $Title) { continue }
            return [pscustomobject]@{
                hwnd = [IntPtr][Convert]::ToInt64($Matches[2].Substring(2), 16)
                l = [int]$Matches[5]; t = [int]$Matches[6]; r = [int]$Matches[7]; b = [int]$Matches[8]
                line = $line
            }
        }
    }
    return $null
}

function Wait-OgWindowGone {
    param([string] $Title, [int] $TimeoutSec = 30)
    $t0 = Get-Date
    while (((Get-Date) - $t0).TotalSeconds -lt $TimeoutSec) {
        if (-not (Get-OgWindowByTitle $Title)) { return $true }
        Start-Sleep -Milliseconds 500
    }
    return $null -eq (Get-OgWindowByTitle $Title)
}

function Wait-OgWindowAppear {
    param([string] $Title, [int] $TimeoutSec = 30)
    $t0 = Get-Date
    while (((Get-Date) - $t0).TotalSeconds -lt $TimeoutSec) {
        $w = Get-OgWindowByTitle $Title
        if ($w) { return $w }
        Start-Sleep -Milliseconds 500
    }
    return $null
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
    # The DRWNewFree\Metadata values are REG_BINARY (u64 little-endian); the
    # first draft returned the raw byte arrays (evidence: run 40073093997
    # og-quota.json - "enable_total_size": [0,0,0,32,...] = 0x20000000 =
    # 512 MiB, the actual free limit; the cap gate fired at 520.29 MB).
    $q = [ordered]@{}
    try {
        $k = 'HKLM:\SOFTWARE\DRWNewFree\Metadata'
        if (Test-Path $k) {
            $props = Get-ItemProperty -Path $k -ErrorAction SilentlyContinue
            function Convert-OgQuotaValue($v) {
                if ($null -eq $v) { return $null }
                if ($v -is [byte[]]) {
                    if ($v.Length -ge 8) { return [BitConverter]::ToUInt64($v, 0) }
                    if ($v.Length -gt 0) { $padded = $v + (New-Object byte[] (8 - $v.Length)); return [BitConverter]::ToUInt64($padded, 0) }
                    return 0
                }
                return $v
            }
            $q.enable_total_size = Convert-OgQuotaValue $props.EnableTotalSize
            $q.enable_total_file_count = Convert-OgQuotaValue $props.EnableTotalFileCount
            $q.recovered_total_size_before = Convert-OgQuotaValue $props.RecoveredTotalSize
            $q.recovered_total_file_count_before = Convert-OgQuotaValue $props.RecoveredTotalFileCount
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
