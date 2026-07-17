$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$generator = Join-Path $repo 'scripts\lvgl_sources.ps1'
$first = Join-Path $env:TEMP 'wio-lvgl-manifest-1.mk'
$second = Join-Path $env:TEMP 'wio-lvgl-manifest-2.mk'

& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $generator -OutputPath $first
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $generator -OutputPath $second

$a = [IO.File]::ReadAllBytes($first)
$b = [IO.File]::ReadAllBytes($second)
if ([Convert]::ToBase64String($a) -ne [Convert]::ToBase64String($b)) {
    throw 'LVGL manifests are not byte-identical'
}

$text = [IO.File]::ReadAllText($first)
$matrixSources = [regex]::Matches($text, '(?m)^Middlewares/Third_Party/lvgl/src/.*/vg_lite_matrix\.c(?: \\)?$')
$matrixObjects = [regex]::Matches($text, '(?m)^\$\(BUILD_DIR\)/lvgl/.*/vg_lite_matrix\.o(?: \\)?$')
if ($matrixSources.Count -ne 2) { throw 'Expected two colliding vg_lite_matrix.c basenames' }
if ($matrixObjects.Count -ne 2) { throw 'Expected two path-unique vg_lite_matrix.o objects' }
if ($matrixObjects[0].Value -eq $matrixObjects[1].Value) { throw 'LVGL object paths collide' }

Remove-Item -LiteralPath $first, $second -Force
