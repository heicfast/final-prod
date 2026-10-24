# isolate.ps1 - pin down which sub-expression throws op_Multiply
$first = [object[]]@(5953L, 1L)
$bpc = [long]4096

Write-Host "A: cast+index alone"
$a1 = [long]$first[0]
Write-Host "  a1=$a1 type=$($a1.GetType().Name)"

Write-Host "B: multiply outside array literal"
try { $b1 = [long]$first[0] * [long]$bpc; Write-Host "  b1=$b1" } catch { Write-Host "  B FAILED: $($_.Exception.Message)" }

Write-Host "C: multiply inside array literal @()"
try { $c1 = @([long]$first[0] * [long]$bpc, [long]$first[1] * [long]$bpc); Write-Host "  c1=$($c1 -join ',')" } catch { Write-Host "  C FAILED: $($_.Exception.Message)" }

Write-Host "D: parenthesized inside array literal"
try { $d1 = @(([long]$first[0]) * ([long]$bpc), ([long]$first[1]) * ([long]$bpc)); Write-Host "  d1=$($d1 -join ',')" } catch { Write-Host "  D FAILED: $($_.Exception.Message)" }

Write-Host "E: comma-first parse check (is it parsed as [long]($first[0] * $bpc, ...) ??)"
try { $e1 = @([long]($first[0] * $bpc), [long]($first[1] * $bpc)); Write-Host "  e1=$($e1 -join ',')" } catch { Write-Host "  E FAILED: $($_.Exception.Message)" }

Write-Host "F: the ,@( ) wrapper exactly as apply-scenario writes it"
try { $f1 = ,@([long]$first[0] * [long]$bpc, [long]$first[1] * [long]$bpc); Write-Host "  f1=$($f1[0] -join ',')" } catch { Write-Host "  F FAILED: $($_.Exception.Message)" }

Write-Host "G: single element, no comma"
try { $g1 = @([long]$first[0] * [long]$bpc); Write-Host "  g1=$g1" } catch { Write-Host "  G FAILED: $($_.Exception.Message)" }
