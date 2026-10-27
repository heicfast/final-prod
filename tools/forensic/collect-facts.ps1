# collect-facts.ps1 - snapshot machine + product state (services, driver file,
# install dir, registry, cert stores, rdacli, secure boot / testsigning) to
# facts-<label>.json. Used before install, after install, and after the run.
#
# Usage: collect-facts.ps1 -OutDir <dir> -Label <preinstall|postinstall|postrun>

param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $true)][string]$Label
)

$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$instDir = 'C:\Program Files\HeicFast\Recovery Wizard'

function Safe([scriptblock]$b) {
    try { & $b } catch { "ERROR: $($_.Exception.Message)" }
}

$f = [ordered]@{
    label        = $Label
    capturedUtc  = (Get-Date).ToUniversalTime().ToString('o')
    os           = Safe { (Get-CimInstance Win32_OperatingSystem).Caption + ' ' + (Get-CimInstance Win32_OperatingSystem).Version }
    physDisks    = Safe { @(Get-Disk | Sort-Object Number | ForEach-Object { [ordered]@{ n = $_.Number; model = $_.Model; sizeGB = [math]::Round($_.Size/1GB,1); bus = $_.BusType; partitions = $_.NumberOfPartitions } }) }
    volumes      = Safe { @(Get-Volume | Where-Object DriveLetter | Sort-Object DriveLetter | ForEach-Object { [ordered]@{ letter = $_.DriveLetter; fs = $_.FileSystem; label = $_.FileSystemLabel; sizeGB = [math]::Round($_.Size/1GB,1) } }) }
    secureBoot   = Safe { try { Confirm-SecureBootUEFI } catch { "unavailable: $($_.Exception.Message)" } }
    testSigning  = Safe { (bcdedit | Select-String 'testsigning') -join '; ' }
    rdaService   = Safe { $s = Get-Service -Name RDA -ErrorAction SilentlyContinue; if ($s) { [ordered]@{ status = $s.Status.ToString(); startType = $s.StartType.ToString() } } else { 'missing' } }
    rdaServiceReg= Safe { if (Test-Path 'HKLM:\SYSTEM\CurrentControlSet\Services\RDA') { Get-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Services\RDA' | Select-Object * | Out-String } else { 'missing' } }
    driverFile   = Safe { if (Test-Path "$env:SystemRoot\System32\drivers\RDA.sys") { Get-Item "$env:SystemRoot\System32\drivers\RDA.sys" | Select-Object FullName, Length, LastWriteTime | Out-String } else { 'missing' } }
    installDir   = Safe { if (Test-Path $instDir) { Get-ChildItem $instDir -Recurse -File | Select-Object @{n='path';e={$_.FullName.Substring($instDir.Length+1)}}, Length | Out-String } else { 'missing' } }
    installDirCount = Safe { if (Test-Path $instDir) { (Get-ChildItem $instDir -Recurse -File).Count } else { 0 } }
    rootCert     = Safe { @(Get-ChildItem Cert:\LocalMachine\Root | Where-Object { $_.Subject -match 'HeicFast' } | ForEach-Object { $_.Thumbprint }) }
    tpCert       = Safe { @(Get-ChildItem Cert:\LocalMachine\TrustedPublisher | Where-Object { $_.Subject -match 'HeicFast' } | ForEach-Object { $_.Thumbprint }) }
    rdacli       = Safe { $exe = Join-Path $instDir 'driver\rdacli.exe'; if (Test-Path $exe) { & $exe info 2>&1 | Out-String } else { 'rdacli not installed' } }
    productKey   = Safe { if (Test-Path 'HKLM:\SOFTWARE\HeicFast') { Get-ChildItem 'HKLM:\SOFTWARE\HeicFast' -Recurse | Out-String } else { 'missing' } }
    uninstallKey = Safe { Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*' -ErrorAction SilentlyContinue | Where-Object { $_.DisplayName -match 'Recovery Wizard' } | Select-Object DisplayName, DisplayVersion, UninstallString | Out-String }
    systemEvents = Safe { Get-WinEvent -FilterHashtable @{LogName='System'; StartTime=(Get-Date).AddHours(-2)} -ErrorAction SilentlyContinue | Where-Object { $_.Message -match 'RDA|rda\.sys' } | Select-Object -First 10 TimeCreated, Id, ProviderName, Message | Out-String }
}

$dest = Join-Path $OutDir "facts-$Label.json"
$f | ConvertTo-Json -Depth 6 | Set-Content $dest -Encoding UTF8
Write-Host "[collect-facts] $Label -> $dest"
