$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$app = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'App/Src/app.c')

if ($app -notmatch 'low_battery_policy_update_from_vehicle\s*\(\s*&battery_policy\s*,\s*vehicle_state_source_is_demo\s*\(\s*&vehicle_source\s*\)\s*,\s*real_state\s*\)') {
    throw 'App_Tick must evaluate low battery from raw real state and demo ownership'
}
if ($app -match 'low_battery_policy_update\s*\([^;]*presented->battery_v') {
    throw 'App_Tick must not evaluate low battery from blended presentation voltage'
}
