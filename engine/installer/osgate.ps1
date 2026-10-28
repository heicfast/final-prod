# osgate.ps1 - the OS-gate decision function (PACKAGING.md section 6).
#
# The REFERENCE's gate (SetupUE's 16-entry table, Win2000 -> Server 2016,
# GetNativeSystemInfo + RtlGetNtVersionNumbers) is modernized to the merged
# product's documented target envelope (REGISTERED divergence, by evidence):
# Win7 SP1 -> Win11 + Server 2025, x64-only. The mechanism stays the
# reference's: native version probing (WOW64-aware), a table of supported
# major.minor.build ranges, and a REFUSAL (never a warning) outside the
# envelope.
#
# This module is the shared decision truth: installer/core-recovery.iss
# embeds the SAME table (the OSGATE-V1 marker line is the sync pin
# tools/test_installer_osgate.py enforces), and the matrix gate drives this
# function with injected version tuples so the refusal behavior is provable
# on any runner OS (the real setup.exe runs on this runner's in-envelope OS
# as part of the round-trip gate).
#
# Usage:
#   ./osgate.ps1 -Live                     # decide for THIS machine
#   ./osgate.ps1 -Major 6 -Minor 1 -Build 7600 -Arch x64   # probe a tuple
#
# Output: JSON @{ admit = $true/$false; reason = '...' } on stdout; exit 0.

# OSGATE-V1: admit (6.1.7601+ | 6.2 | 6.3 | 10.0+ on x64); refuse (below 6.1.7601, any x86/arm64)
param(
    [int] $Major = -1,
    [int] $Minor = -1,
    [int] $Build = -1,
    [string] $Arch = '',
    [switch] $Live
)

$ErrorActionPreference = 'Stop'

if ($Live) {
    # native truth: [Environment]::OSVersion.Version is not subject to the
    # unmanifested GetVersion lie; PROCESSOR_ARCHITECTURE under 64-bit pwsh
    # reports the native arch (WOW64-aware)
    $os = [Environment]::OSVersion.Version
    $Major = $os.Major; $Minor = $os.Minor; $Build = $os.Build
    $Arch = $env:PROCESSOR_ARCHITECTURE
}

function Test-OsGate {
    param([int] $Major, [int] $Minor, [int] $Build, [string] $Arch)

    # x64-only: the driver's PE contract gates machine AMD64; any other
    # architecture is outside the envelope (the reference's Itanium /
    # Compute-Cluster rows have no successor meaning)
    if ($Arch -ne 'x64' -and $Arch -ne 'AMD64') {
        return @{ admit = $false
                  reason = "architecture '$Arch' is not x64 (x64-only envelope)" }
    }

    # the modernized envelope: 6.1.7601 (Win7 SP1) -> 10.0+ (Win10 / Win11 /
    # Server 2022 / Server 2025). Win7 RTM/SP0 (build < 7601) REFUSES - SP1
    # is the documented floor (the same floor the core's _WIN32_WINNT=0x0601
    # + PE import gates enforce on the binary)
    $inEnvelope = (
        ($Major -eq 6 -and $Minor -eq 1 -and $Build -ge 7601) -or
        ($Major -eq 6 -and $Minor -eq 2) -or
        ($Major -eq 6 -and $Minor -eq 3) -or
        ($Major -ge 10)
    )
    if (-not $inEnvelope) {
        return @{ admit = $false
                  reason = "version $Major.$Minor.$Build is below the Windows 7 SP1 floor (6.1.7601)" }
    }

    return @{ admit = $true
              reason = "version $Major.$Minor.$Build on $Arch is inside the Win7 SP1 -> Win11/Server 2025 envelope" }
}

$decision = Test-OsGate -Major $Major -Minor $Minor -Build $Build -Arch $Arch
$decision | ConvertTo-Json -Compress | Write-Output
if (-not $decision.admit) { exit 3 } else { exit 0 }
