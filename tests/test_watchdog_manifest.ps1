$ErrorActionPreference = 'Stop'
$source = Get-Content -Raw (Join-Path $PSScriptRoot '..\Core\Src\iwdg.c')
if ($source -notmatch '#if defined\(DEBUG\) \|\| !defined\(NDEBUG\)') {
    throw 'IWDG debug-freeze compile guard is missing'
}
if ($source -notmatch '__HAL_DBGMCU_FREEZE_IWDG1\(\)') {
    throw 'IWDG1 is not frozen while debugging'
}
if ($source -notmatch 'LSI_READY_TIMEOUT_MS = 100u' -or
    $source -notmatch 'IWDG_UPDATE_TIMEOUT_MS = 6145u' -or
    $source -notmatch 'lsi_started_ms' -or
    $source -notmatch 'update_started_ms') {
    throw 'LSI-ready and IWDG-update waits do not have independent deadlines'
}
$start = $source.IndexOf('0xCCCCu')
$notStartedFailure = $source.LastIndexOf('return WATCHDOG_NOT_STARTED_FAIL')
$startedFailure = $source.IndexOf('return WATCHDOG_STARTED_CONFIG_FAIL')
if ($start -lt 0 -or $notStartedFailure -gt $start) {
    throw 'A NOT_STARTED failure is reported after IWDG start'
}
if ($startedFailure -lt $start) {
    throw 'Post-start configuration timeout is not classified as STARTED_CONFIG_FAIL'
}
$ioc = Get-Content -Raw (Join-Path $PSScriptRoot '..\wio_ai.ioc')
if ($ioc -match '(?m)^RCC\.LSIState=') {
    throw 'IOC claims SystemClock ownership of LSI while IWDG runtime owns it'
}
