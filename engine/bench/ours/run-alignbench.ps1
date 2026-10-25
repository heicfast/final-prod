# run-alignbench.ps1 - the API/transport benchmark harness (core-api session,
# mandate section 3 + docs/PERFORMANCE-BASELINE.md section 5/6).
#
# Measures, on real Windows storage stacks, with every number recorded to JSON:
#   1. THE 4-KiB-vs-64-KiB slow-path demonstration (the mandate's ~3.5x claim,
#      ~830 vs ~2900 MB/s on mid-range NVMe): sequential NO_BUFFERING reads at
#      a 64 KiB-multiple chunk (128 KiB) vs a 4-KiB-multiple-but-not-64-KiB
#      chunk (132 KiB) over the same span. The ratio is RECORDED, not gated -
#      virtualized runner storage may attenuate it; the transport's 64 KiB
#      envelope is enforced by construction and unit-tested (test_directalign).
#   2. Read-size sweep x queue depth: [64 KiB, 256 KiB, 1 MiB, 4 MiB] x
#      QD [1, 2, 4, 8] (FILE_FLAG_OVERLAPPED ring for QD>1; QD1 = the
#      production shape, matching the reference's synchronous model).
#   3. RDA driver round-trip (optional): grammar-path open + ReadFile with the
#      F11 Information x512 unscale verified, same sweep subset.
#   4. cr_ui cheap-op API latency (optional): late-bound DllImport loop over
#      abi_version / last_error / carve_format_count / carve_format_info.
#
# Hard gates (exit non-zero): open failures, any failed read, checksum
# mismatch between the first and last pass over a config, missing results.
# NO continue-on-error semantics.
#
# Licensed under the MIT license. See LICENSE.

param(
    [string] $Target = '\\.\C:',      # direct raw target (volume or disk path)
    [string] $RdaService = 'rda',     # empty string = skip the RDA leg
    [int] $RdaDisk = 0,               # disk number for the grammar path
    [string] $CrUiDll = '',           # path to cr_ui.dll (empty = skip API latency)
    [int] $SpanMiB = 512,             # bytes read per measurement
    [string] $OutJson = 'alignbench.json'
)

$ErrorActionPreference = 'Stop'

if (-not ('AlignBench' -as [type])) {
    Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

public class BenchResult {
    public string config;
    public double mib_per_s;
    public ulong checksum;
    public ulong bytes;
    public double ms;
    public int reads;
}

public static class AlignBench {
    const uint GENERIC_READ = 0x80000000;
    const uint OPEN_EXISTING = 3;
    const uint FILE_FLAG_NO_BUFFERING = 0x20000000;
    const uint FILE_FLAG_OVERLAPPED = 0x40000000;

    [StructLayout(LayoutKind.Sequential)]
    public struct OVERLAPPED {
        public IntPtr internalH;
        public IntPtr internalHigh;
        public uint offsetLo;
        public uint offsetHi;
        public IntPtr hEvent;
    }

    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr CreateFileW(string name, uint access, uint share, IntPtr sec,
        uint disp, uint flags, IntPtr tmpl);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool ReadFile(IntPtr h, byte[] buf, uint len, out uint got, ref OVERLAPPED ov);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool GetOverlappedResult(IntPtr h, ref OVERLAPPED ov, out uint got, bool wait);
    [DllImport("kernel32.dll", SetLastError = true)]
    static extern bool CloseHandle(IntPtr h);
    [DllImport("kernel32.dll")]
    static extern IntPtr CreateEvent(IntPtr attrs, bool manual, bool init, string name);
    [DllImport("kernel32.dll")]
    static extern bool ResetEvent(IntPtr ev);

    // The reference PID-token transform (corpus Device_decompiled.c:342-351 /
    // driver rda_compat.h RdaComputePidToken): 8-bit wrapping byte sums.
    public static uint TokenFromPid(uint pid) {
        uint b0 = pid & 0xFF, b1 = (pid >> 8) & 0xFF, b2 = (pid >> 16) & 0xFF, b3 = (pid >> 24) & 0xFF;
        return b0 | (((b0 + b1) & 0xFF) << 8) | (((b1 + b2) & 0xFF) << 16) | (((b2 + b3) & 0xFF) << 24);
    }

    static IntPtr Open(string path, bool noBuffering, bool overlapped) {
        uint flags = 0x80; // FILE_ATTRIBUTE_NORMAL
        if (noBuffering) flags |= FILE_FLAG_NO_BUFFERING;
        if (overlapped) flags |= FILE_FLAG_OVERLAPPED;
        IntPtr h = CreateFileW(path, GENERIC_READ, 1 | 2 /*READ|WRITE*/, IntPtr.Zero,
            OPEN_EXISTING, flags, IntPtr.Zero);
        if (h == new IntPtr(-1)) {
            throw new IOException("CreateFileW(" + path + ") failed GLE=" + Marshal.GetLastWin32Error());
        }
        return h;
    }

    // One sequential-read measurement. QD1: plain synchronous reads with the
    // 64-bit offset in OVERLAPPED (the production transport's shape). QD>1:
    // an overlapped ring (issue QD reads, harvest the oldest, re-issue) on
    // FILE_FLAG_OVERLAPPED handles. F11: RDA handles report Information =
    // len * 512 - unscale and verify.
    public static BenchResult SeqRead(string path, uint chunk, int qd, bool noBuffering,
                                       bool rdaScaled, long spanBytes, int passes) {
        var r = new BenchResult();
        r.config = string.Format("{0} chunk={1} qd={2} nobuf={3} rda={4}",
            path, chunk, qd, noBuffering ? 1 : 0, rdaScaled ? 1 : 0);
        bool ovh = qd > 1;
        IntPtr h = Open(path, noBuffering, ovh);
        var sw = Stopwatch.StartNew();
        ulong check = 1469598103934665603UL; // FNV-1a basis - verifies both passes read the same bytes
        ulong bytesTotal = 0; int reads = 0;
        byte[] buf = new byte[chunk];
        try {
            for (int p = 0; p < passes; ++p) {
                if (p == 1) { r.checksum = check; } // first-pass checksum snapshot
                if (p == 1) { check = 1469598103934665603UL; bytesTotal = 0; reads = 0; sw.Restart(); }
                if (!ovh) {
                    for (long off = 0; off < spanBytes; off += chunk) {
                        var ov = new OVERLAPPED();
                        ov.offsetLo = (uint)((ulong)off & 0xFFFFFFFF);
                        ov.offsetHi = (uint)((ulong)off >> 32);
                        uint got;
                        if (!ReadFile(h, buf, chunk, out got, ref ov)) {
                            uint gle = (uint)Marshal.GetLastWin32Error();
                            if (gle != 997 && !GetOverlappedResult(h, ref ov, out got, true))
                                throw new IOException("ReadFile failed GLE=" + Marshal.GetLastWin32Error()
                                    + " off=" + off);
                        }
                        uint want = rdaScaled ? chunk * 512 : chunk;
                        if (got != want)
                            throw new IOException("short read: got=" + got + " want=" + want);
                        for (int i = 0; i < buf.Length; ++i) { check = (check ^ buf[i]) * 1099511628211UL; }
                        bytesTotal += chunk; ++reads;
                    }
                } else {
                    // overlapped ring: manual-reset events are Reset before EVERY
                    // re-issue (a stale signaled event would let the next
                    // GetOverlappedResult pass through before the read lands)
                    int n = (int)(spanBytes / chunk);
                    if (n < qd) qd = n;
                    var ovs = new OVERLAPPED[qd];
                    var bufs = new byte[qd][];
                    for (int i = 0; i < qd; ++i) {
                        bufs[i] = new byte[chunk];
                        ovs[i].hEvent = CreateEvent(IntPtr.Zero, true, false, null);
                    }
                    int issued = 0, done = 0;
                    while (done < n) {
                        while (issued < n && issued - done < qd) {
                            int slot = issued % qd;
                            ResetEvent(ovs[slot].hEvent);
                            ovs[slot].offsetLo = (uint)((ulong)(issued * (long)chunk) & 0xFFFFFFFF);
                            ovs[slot].offsetHi = (uint)((ulong)(issued * (long)chunk) >> 32);
                            uint got;
                            if (!ReadFile(h, bufs[slot], chunk, out got, ref ovs[slot])) {
                                uint gle = (uint)Marshal.GetLastWin32Error();
                                if (gle != 997)
                                    throw new IOException("ReadFile(ov) failed GLE=" + gle);
                            }
                            ++issued;
                        }
                        int ds = done % qd;
                        uint got2;
                        if (!GetOverlappedResult(h, ref ovs[ds], out got2, true))
                            throw new IOException("GetOverlappedResult failed GLE=" + Marshal.GetLastWin32Error());
                        uint want = rdaScaled ? chunk * 512 : chunk;
                        if (got2 != want)
                            throw new IOException("short ov read: got=" + got2 + " want=" + want);
                        for (int i = 0; i < chunk; ++i) { check = (check ^ bufs[ds][i]) * 1099511628211UL; }
                        bytesTotal += chunk; ++reads; ++done;
                    }
                    for (int i = 0; i < qd; ++i) if (ovs[i].hEvent != IntPtr.Zero) CloseHandle(ovs[i].hEvent);
                }
            }
            if (r.checksum != check)
                throw new IOException("checksum mismatch between pass 1 and 2: "
                    + r.checksum + " vs " + check + " (data changed under the benchmark)");
        } finally {
            CloseHandle(h);
        }
        sw.Stop();
        r.bytes = bytesTotal; r.reads = reads; r.ms = sw.Elapsed.TotalMilliseconds;
        r.mib_per_s = bytesTotal / (1024.0 * 1024.0) / (sw.Elapsed.TotalSeconds);
        return r;
    }

    // ---- late-bound cr_ui cheap-op latency ----
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    static extern IntPtr LoadLibraryW(string path);
    [DllImport("kernel32.dll", CharSet = CharSet.Ansi)]
    static extern IntPtr GetProcAddress(IntPtr h, string name);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate uint U32Fn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate IntPtr CharPtrFn();
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    delegate uint InfoFn(uint idx, byte[] outBuf);
    // cr_ui_carve_format C layout (natural alignment): layer u32@0,
    // ext[24]@4, min_size u64@32, max_size u64@40, base_confidence u16@48,
    // magic_len u32@52, magic_offset u32@56, magic[16]@60 = 76 -> 80 bytes.
    // The benchmark only times the call; the buffer is sized 256 so ABI
    // appends (up to 256 bytes) cannot overrun the managed side.

    public static Dictionary<string, double> ApiLatency(string dllPath, int iters) {
        var outMap = new Dictionary<string, double>();
        IntPtr h = LoadLibraryW(dllPath);
        if (h == IntPtr.Zero) throw new IOException("LoadLibraryW(" + dllPath + ") failed");
        var sw = new Stopwatch();
        IntPtr p;

        p = GetProcAddress(h, "cr_ui_abi_version");
        if (p != IntPtr.Zero) {
            var f = (U32Fn)Marshal.GetDelegateForFunctionPointer(p, typeof(U32Fn));
            f(); sw.Restart();
            for (int i = 0; i < iters; ++i) f();
            sw.Stop();
            outMap["cr_ui_abi_version_ns"] = sw.Elapsed.TotalMilliseconds * 1e6 / iters;
        }
        p = GetProcAddress(h, "cr_ui_last_error");
        if (p != IntPtr.Zero) {
            var f = (CharPtrFn)Marshal.GetDelegateForFunctionPointer(p, typeof(CharPtrFn));
            f(); sw.Restart();
            for (int i = 0; i < iters; ++i) f();
            sw.Stop();
            outMap["cr_ui_last_error_ns"] = sw.Elapsed.TotalMilliseconds * 1e6 / iters;
        }
        p = GetProcAddress(h, "cr_ui_carve_format_count");
        if (p != IntPtr.Zero) {
            var f = (U32Fn)Marshal.GetDelegateForFunctionPointer(p, typeof(U32Fn));
            uint n = f(); sw.Restart();
            for (int i = 0; i < iters; ++i) f();
            sw.Stop();
            outMap["cr_ui_carve_format_count_ns"] = sw.Elapsed.TotalMilliseconds * 1e6 / iters;
            outMap["carve_format_count"] = n;

            p = GetProcAddress(h, "cr_ui_carve_format_info");
            if (p != IntPtr.Zero && n > 0) {
                var fi = (InfoFn)Marshal.GetDelegateForFunctionPointer(p, typeof(InfoFn));
                var ob = new byte[256];
                fi(0, ob); sw.Restart();
                for (int i = 0; i < iters; ++i) fi((uint)(i % n), ob);
                sw.Stop();
                outMap["cr_ui_carve_format_info_ns"] = sw.Elapsed.TotalMilliseconds * 1e6 / iters;
            }
        }
        return outMap;
    }
}
"@
}

$results = New-Object System.Collections.Generic.List[object]
$envInfo = [ordered]@{
    timestamp = (Get-Date -AsUTC -Format 'yyyy-MM-ddTHH:mm:ssZ')
    span_mib  = $SpanMiB
    target    = $Target
    runner    = $env:RUNNER_NAME
    image     = $env:ImageOS
}
$apiLatency = $null

function Add-Result($r) {
    $results.Add([ordered]@{
        config    = $r.config
        mib_per_s = [math]::Round($r.mib_per_s, 2)
        ms        = [math]::Round($r.ms, 1)
        reads     = $r.reads
        bytes     = $r.bytes
        checksum  = $r.checksum
    })
    Write-Host ("  {0}  ->  {1} MiB/s ({2} reads, {3} ms)" -f $r.config, [math]::Round($r.mib_per_s, 2), $r.reads, [math]::Round($r.ms, 1))
}

$span = [long]$SpanMiB * 1024 * 1024

Write-Host "== 1. the 4-KiB-vs-64-KiB slow-path demonstration (NO_BUFFERING, QD1) =="
# 128 KiB = 64 KiB multiple (fast-path shape); 132 KiB = 4 KiB multiple, NOT a
# 64 KiB multiple (the documented slow-path shape). Same span, same target.
foreach ($chunk in @(128KB, 132KB)) {
    $r = [AlignBench]::SeqRead($Target, [uint32]$chunk, 1, $true, $false, $span, 2)
    Add-Result $r
}

Write-Host "== 2. read-size x queue-depth sweep (production flag shape, QD1 sync + overlapped QD>1) =="
foreach ($chunk in @(64KB, 256KB, 1MB, 4MB)) {
    foreach ($qd in @(1, 2, 4, 8)) {
        $r = [AlignBench]::SeqRead($Target, [uint32]$chunk, $qd, $false, $false, $span, 2)
        Add-Result $r
    }
}

if ($RdaService -and $RdaService -ne '') {
    Write-Host "== 3. RDA driver grammar-path round-trip (F11 unscale verified) =="
    $tok = [AlignBench]::TokenFromPid([uint32]$PID)
    $rdaPath = "\\.\$RdaService\$tok\$RdaDisk\0"
    Write-Host "  grammar path: $rdaPath"
    foreach ($chunk in @(64KB, 256KB, 1MB)) {
        $r = [AlignBench]::SeqRead($rdaPath, [uint32]$chunk, 1, $false, $true, $span, 2)
        Add-Result $r
    }
    # direct-vs-driver comparison at the same chunk
    foreach ($chunk in @(256KB)) {
        $r = [AlignBench]::SeqRead("\\.\PhysicalDrive$RdaDisk", [uint32]$chunk, 1, $false, $false, $span, 2)
        Add-Result $r
    }
}

if ($CrUiDll -and (Test-Path $CrUiDll)) {
    Write-Host "== 4. cr_ui cheap-op API latency (late-bound, 2M iterations each) =="
    $apiLatency = [AlignBench]::ApiLatency($CrUiDll, 2000000)
    foreach ($k in $apiLatency.Keys) {
        Write-Host ("  {0} = {1}" -f $k, $apiLatency[$k])
    }
}

# ---- the alignment comparison is RECORDED (the ratio is hardware-dependent;
# virtualized runner storage may attenuate the documented ~3.5x) ----
$c128 = $results | Where-Object { $_.config -match 'chunk=131072 qd=1 nobuf=1' }
$c132 = $results | Where-Object { $_.config -match 'chunk=135168 qd=1 nobuf=1' }
$ratio = $null
if ($c128 -and $c132 -and $c132.mib_per_s -gt 0) {
    $ratio = [math]::Round($c128.mib_per_s / $c132.mib_per_s, 3)
    Write-Host ("ALIGNMENT RATIO (128KiB / 132KiB, NO_BUFFERING, QD1): {0}x  [documented: ~3.5x on mid-range NVMe]" -f $ratio)
}

if ($results.Count -eq 0) { Write-Error 'no benchmark results'; exit 1 }

$out = [ordered]@{
    environment = $envInfo
    alignment_ratio_64k_over_4k = $ratio
    api_latency_ns = $apiLatency
    results = $results
}
$out | ConvertTo-Json -Depth 5 | Out-File $OutJson -Encoding utf8
Write-Host "wrote $OutJson ($($results.Count) measurements)"
exit 0
