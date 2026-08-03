$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$irqSource = Get-Content -Raw (Join-Path $root 'Core/Src/stm32h7xx_it.c')
$irqHeader = Get-Content -Raw (Join-Path $root 'Core/Inc/stm32h7xx_it.h')
$startup = Get-Content -Raw (Join-Path $root 'Core/Startup/startup_stm32h725aeix.s')
$main = Get-Content -Raw (Join-Path $root 'Core/Src/main.c')
$makefile = Get-Content -Raw (Join-Path $root 'Makefile')
$cmakeLists = Get-Content -Raw (Join-Path $root 'CMakeLists.txt')
$app = Get-Content -Raw (Join-Path $root 'App/Src/app.c')

if ($irqSource -match 'EXTI1_IRQHandler|hpb_exti\s*\[') {
    throw 'Polling mode must not retain the EXTI1 button handler or handle access'
}
if ($irqHeader -match 'EXTI1_IRQHandler') {
    throw 'Polling mode must not declare EXTI1_IRQHandler'
}
if ($startup -notmatch '\.weak\s+EXTI1_IRQHandler' -or
    $startup -notmatch '\.thumb_set\s+EXTI1_IRQHandler,Default_Handler') {
    throw 'Startup must provide a weak EXTI1 Default_Handler alias'
}
if ($main -notmatch 'BSP_PB_Init\s*\(\s*BUTTON_USER1\s*,\s*BUTTON_MODE_GPIO\s*\)') {
    throw 'BUTTON_USER1 must remain configured for GPIO polling'
}
foreach ($source in @('App/Src/demo_vehicle_state.c', 'App/Src/vehicle_state_source.c')) {
    if ($makefile -notmatch [regex]::Escape($source)) {
        throw "Makefile must compile $source"
    }
    if ($cmakeLists -notmatch [regex]::Escape($source)) {
        throw "CMakeLists.txt must compile $source"
    }
}
if ($app -notmatch 'if\s*\(\s*vehicle_state_on_msp\s*\(\s*frame\s*,\s*now_ms\s*\)\s*\)\s*\{\s*vehicle_state_source_note_real\s*\(') {
    throw 'A valid MSP frame must be required before real takeover is noted'
}
