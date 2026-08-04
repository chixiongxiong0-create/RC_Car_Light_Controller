param(
    [Parameter(Mandatory = $true)]
    [string]$BootStubElf,
    [Parameter(Mandatory = $true)]
    [string]$BootStubBin,
    [Parameter(Mandatory = $true)]
    [string]$ApplicationElf,
    [string]$BootStubHex,
    [string]$TrackedHex,
    [string]$ToolchainBin = ''
)

$ErrorActionPreference = 'Stop'

function Tool([string]$Name) {
    $fileName = "$Name.exe"
    if ($ToolchainBin) {
        $candidate = Join-Path $ToolchainBin $fileName
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }
    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($null -eq $command) {
        throw "Cannot find $Name; pass -ToolchainBin"
    }
    return $command.Source
}

function Run([string]$Program, [string[]]$Arguments) {
    $output = & $Program @Arguments 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE`n$output"
    }
    return ($output -join "`n")
}

function Read-U32([byte[]]$Bytes, [int]$Offset) {
    return [BitConverter]::ToUInt32($Bytes, $Offset)
}

$objdump = Tool 'arm-none-eabi-objdump'
$nm = Tool 'arm-none-eabi-nm'
$sections = Run $objdump @('-h', $BootStubElf)
$appSections = Run $objdump @('-h', $ApplicationElf)
$bootSymbols = Run $nm @('-n', $BootStubElf)
$appSymbols = Run $nm @('-n', $ApplicationElf)
$disassembly = Run $objdump @('-d', $BootStubElf)

if ($sections -notmatch '(?m)^\s*\d+\s+\.isr_vector\s+[0-9a-fA-F]+\s+08000000\s+08000000') {
    throw 'Boot-stub .isr_vector is not at 0x08000000'
}
if ($appSections -notmatch '(?m)^\s*\d+\s+\.isr_vector\s+[0-9a-fA-F]+\s+08020000\s+08020000') {
    throw 'Application .isr_vector is not at 0x08020000'
}
if ($bootSymbols -notmatch '(?mi)^08000040\s+\w\s+BootStub_Reset_Handler$') {
    throw 'BootStub_Reset_Handler is not fixed at 0x08000040'
}
if ($appSymbols -notmatch '(?mi)^([0-9a-f]{8})\s+\w\s+Reset_Handler$') {
    throw 'Could not read the application Reset_Handler symbol'
}
$applicationReset = [Convert]::ToUInt32($Matches[1], 16)

if ($disassembly -notmatch '(?i)ldr(?:\.w)?\s+r1,\s*\[r0(?:,\s*#0)?\]' -or
    $disassembly -notmatch '(?i)ldr(?:\.w)?\s+r2,\s*\[r0,\s*#4\]' -or
    $disassembly -notmatch '(?i)str(?:\.w)?\s+r0,\s*\[r3(?:,\s*#0)?\]' -or
    $disassembly -notmatch '(?i)dsb(?:\s+sy)?' -or
    $disassembly -notmatch '(?i)isb(?:\s+sy)?' -or
    $disassembly -notmatch '(?i)msr\s+MSP,\s*r1' -or
    $disassembly -notmatch '(?i)bx\s+r2') {
    throw 'Boot-stub disassembly does not perform the required dynamic vector handoff'
}

[byte[]]$bytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $BootStubBin))
if ($bytes.Length -lt 8) {
    throw 'Boot-stub BIN is too short'
}
if ((Read-U32 $bytes 0) -ne 0x24050000) {
    throw 'Boot-stub BIN has the wrong initial MSP'
}
if ((Read-U32 $bytes 4) -ne 0x08000041) {
    throw 'Boot-stub BIN does not point at its fixed Thumb reset entry'
}

$appBaseCount = 0
for ($offset = 0; $offset + 4 -le $bytes.Length; $offset += 4) {
    $word = Read-U32 $bytes $offset
    if ($word -eq 0x08020000) {
        ++$appBaseCount
    } elseif ($word -ge 0x08020000 -and $word -le 0x0807ffff) {
        throw ('Boot-stub BIN embeds application code address 0x{0:X8}' -f $word)
    }
    if ($word -eq $applicationReset -or
        $word -eq ($applicationReset -bor [uint32]1)) {
        throw ('Boot-stub BIN embeds the current application Reset_Handler 0x{0:X8}' -f $word)
    }
}
if ($appBaseCount -lt 1) {
    throw 'Boot-stub BIN does not contain the application vector-table base'
}

if ($BootStubHex -and $TrackedHex) {
    $fresh = (Get-Content -Encoding ASCII $BootStubHex) -join "`n"
    $tracked = (Get-Content -Encoding ASCII $TrackedHex) -join "`n"
    if ($fresh -cne $tracked) {
        throw 'Fresh boot-stub HEX does not match the tracked programming artifact'
    }
}

Write-Host ('boot stub PASS: vector=0x08000000 reset=0x08000040 ' +
            'app_vector=0x08020000 current_app_reset=0x{0:X8} not embedded' -f
            $applicationReset)
