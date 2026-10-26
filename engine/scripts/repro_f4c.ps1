# repro_f4c.ps1 - reproduce the op_Multiply failure shape from run 40077233920
# (bench-og ntfs/partial_overwrite): simulate the manifest extents round-trip
# exactly as generate_seed.ps1 builds it and apply-scenario.ps1 reads it.

# --- shape 1: what generate_seed builds (post F-3 fix), JSON round-tripped ---
$manifestLine = '{"rel":"seed/tiny/t000.jpg","name":"t000.jpg","size":2849,"extents":[[5953,1]],"extents_error":null}'
$o = $manifestLine | ConvertFrom-Json
$e = $o
Write-Host "shape1: extents type=$($e.extents.GetType().Name) count=$($e.extents.Count)"
Write-Host "  extents[0] type=$($e.extents[0].GetType().Name) value=$($e.extents[0] -join ',')"

# --- the partial_overwrite math ---
$deletable = @($e)
$bpc = [long]4096
try {
    foreach ($d in $deletable) {
        if (-not $d.extents -or $d.extents.Count -eq 0) { continue }
        $first = $d.extents[0]
        Write-Host "  first type=$($first.GetType().Name); first[0] type=$($first[0].GetType().Name) val=$($first[0]); first[1] type=$($first[1].GetType().Name) val=$($first[1])"
        $r = ,@([long]$first[0] * [long]$bpc, [long]$first[1] * [long]$bpc)
        Write-Host "  OK: $($r -join ',')"
    }
} catch {
    Write-Host "  FAILED: $($_.Exception.Message)"
}

# --- shape 2: SINGLE-extent JSON where ConvertFrom-Json may unwrap ---
$single = '{"extents":[[5953,1]]}'
$o2 = $single | ConvertFrom-Json
Write-Host "shape2: type=$($o2.extents.GetType().Name) count=$($o2.extents.Count) inner=$($o2.extents[0].GetType().Name)"

# --- shape 3: the KEEEP entries (F-3 fix newly enriches them) - ads field ---
$keepLine = '{"rel":"keep/k00.jpg","name":"k00.jpg","size":17191,"extents":[[1055731712,5]],"ads":[]}'
$o3 = $keepLine | ConvertFrom-Json
Write-Host "shape3 keep: extents ok=$($null -ne $o3.extents)"

# --- shape 4: the ADS entries carry ads arrays - does anything multiply there? ---
# --- shape 5: what if extents contains a SINGLE NUMBER (not array) - e.g. JSON
# --- where ConvertTo-Json serialized @(x) single-element as scalar? ---
$weird = '{"extents":[5953]}'
$o5 = $weird | ConvertFrom-Json
Write-Host "shape5: extents[0] type=$($o5.extents[0].GetType().Name)"
try {
    $first = $o5.extents[0]
    $r2 = [long]$first[0] * [long]$bpc
    Write-Host "  scalar-index OK: $r2"
} catch {
    Write-Host "  scalar-index FAILED: $($_.Exception.Message)"
}

# --- shape 6: flat array of numbers (flattening bug) ---
$flat = '{"extents":[5953, 1, 60000, 2]}'
$o6 = $flat | ConvertFrom-Json
try {
    $first = $o6.extents[0]   # scalar 5953
    $r3 = [long]$first[0] * [long]$bpc   # scalar[0] -> scalar itself
    $r4 = [long]$first[1] * [long]$bpc   # scalar[1] -> $null -> [long]$null = 0
    Write-Host "shape6 flat: r3=$r3 r4=$r4 (silent wrong math, no error)"
} catch {
    Write-Host "shape6 flat FAILED: $($_.Exception.Message)"
}

# --- shape 7: nested deeper: [[ [5953,1] ]] ---
$deep = '{"extents":[[[5953,1]]]}'
$o7 = $deep | ConvertFrom-Json
try {
    $first = $o7.extents[0]
    Write-Host "shape7: first type=$($first.GetType().Name); first[0] type=$($first[0].GetType().Name)"
    $r5 = [long]$first[0] * [long]$bpc
    Write-Host "  OK $r5"
} catch {
    Write-Host "shape7 FAILED: $($_.Exception.Message)"
}
