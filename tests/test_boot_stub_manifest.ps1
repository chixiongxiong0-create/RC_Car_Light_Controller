$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Require-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Read-IntelHex([string]$Path) {
    $bytes = @{}
    [uint32]$upper = 0
    $eofSeen = $false

    foreach ($line in Get-Content -Encoding ASCII $Path) {
        if ($line.Length -lt 11 -or $line[0] -ne ':') {
            throw "Invalid Intel HEX record: $line"
        }
        $record = for ($i = 1; $i -lt $line.Length; $i += 2) {
            [Convert]::ToByte($line.Substring($i, 2), 16)
        }
        $sum = 0
        foreach ($value in $record) {
            $sum = ($sum + $value) -band 0xff
        }
        if ($sum -ne 0) {
            throw "Intel HEX checksum mismatch: $line"
        }

        $count = $record[0]
        $offset = ([uint32]$record[1] -shl 8) -bor $record[2]
        $type = $record[3]
        if ($record.Count -ne $count + 5) {
            throw "Intel HEX byte count mismatch: $line"
        }

        if ($type -eq 0) {
            for ($i = 0; $i -lt $count; ++$i) {
                $address = [uint32]($upper + $offset + $i)
                if ($bytes.ContainsKey($address)) {
                    throw ('Intel HEX contains duplicate data at 0x{0:X8}' -f $address)
                }
                $bytes[$address] = $record[4 + $i]
            }
        } elseif ($type -eq 4) {
            if ($count -ne 2) {
                throw "Invalid extended-address record: $line"
            }
            $upper = (([uint32]$record[4] -shl 8) -bor $record[5]) -shl 16
        } elseif ($type -eq 1) {
            $eofSeen = $true
            break
        }
    }
    if (!$eofSeen) {
        throw 'Intel HEX is missing its EOF record'
    }
    return $bytes
}

function Read-U32($Bytes, [uint32]$Address) {
    for ($i = 0; $i -lt 4; ++$i) {
        if (!$Bytes.ContainsKey([uint32]($Address + $i))) {
            throw ('Intel HEX has no byte at 0x{0:X8}' -f ($Address + $i))
        }
    }
    return [uint32](([uint32]$Bytes[$Address]) -bor
        (([uint32]$Bytes[[uint32]($Address + 1)]) -shl 8) -bor
        (([uint32]$Bytes[[uint32]($Address + 2)]) -shl 16) -bor
        (([uint32]$Bytes[[uint32]($Address + 3)]) -shl 24))
}

$required = @(
    'boot_stub/boot_stub.S',
    'boot_stub/STM32H725AEIX_BOOT.ld',
    'boot_stub/CMakeLists.txt',
    'boot_stub/Makefile',
    'boot_stub/wio_ai_boot_stub.hex',
    'scripts/inspect_boot_stub.ps1'
)
foreach ($relative in $required) {
    if (!(Test-Path -LiteralPath (Join-Path $root $relative))) {
        throw "Missing stable boot-stub file: $relative"
    }
}

$rootCMake = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'CMakeLists.txt')
$stProject = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'cmake/st-project.cmake')
$makefile = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'Makefile')
$bootSource = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'boot_stub/boot_stub.S')
$bootLinker = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'boot_stub/STM32H725AEIX_BOOT.ld')
$bootCMake = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'boot_stub/CMakeLists.txt')
$bootMake = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'boot_stub/Makefile')
$inspector = Get-Content -Raw -Encoding UTF8 (Join-Path $root 'scripts/inspect_boot_stub.ps1')

Require-Match $rootCMake 'WIO_AI_APP_LINKER_SCRIPT[\s\S]*STM32H725AEIX_PSRAM\.ld' `
    'Root CMake must explicitly select the PSRAM application linker script'
if ($rootCMake -match 'WIO_AI_APP_LINKER_SCRIPT[\s\S]{0,240}CACHE\s+FILEPATH') {
    throw 'Root CMake must not allow overriding the fixed 0x08020000 application layout'
}
Require-Match $rootCMake 'target_link_libraries\s*\(\s*\$\{PROJECT_NAME\}\s+PRIVATE\s+m\s*\)' `
    'Root ARM CMake target must link libm'
Require-Match $rootCMake 'add_subdirectory\s*\(\s*boot_stub\s*\)' `
    'Root CMake must build the stable boot stub'
Require-Match $stProject '-T\$\{WIO_AI_APP_LINKER_SCRIPT\}' `
    'Generated target properties must consume the explicit application linker script'
if ($stProject -match 'STM32H725AEIX_FLASH\.ld') {
    throw 'Root ARM CMake must not select the 0x08000000 application layout in any configuration'
}
Require-Match $makefile 'LDSCRIPT\s*=\s*STM32H725AEIX_PSRAM\.ld' `
    'SeedStudio Make build must retain the PSRAM linker script'
Require-Match $makefile 'LIBS\s*=.*-lm' `
    'SeedStudio Make build must retain libm'

Require-Match $bootSource '\.equ\s+APP_VECTOR_TABLE\s*,\s*0x08020000' `
    'Boot stub must name the application vector table at 0x08020000'
Require-Match $bootSource 'ldr\s+r1\s*,\s*\[r0\s*\]' `
    'Boot stub must dynamically load the application MSP from vector word 0'
Require-Match $bootSource 'ldr\s+r2\s*,\s*\[r0\s*,\s*#4\s*\]' `
    'Boot stub must dynamically load the application Reset_Handler from vector word 1'
Require-Match $bootSource 'msr\s+msp\s*,\s*r1' `
    'Boot stub must install the application MSP'
Require-Match $bootSource 'bx\s+r2' `
    'Boot stub must branch through the dynamically loaded application reset vector'
foreach ($literalMatch in [regex]::Matches($bootSource, '0x[0-9A-Fa-f]+')) {
    $literal = [Convert]::ToUInt32($literalMatch.Value.Substring(2), 16)
    if ($literal -ge 0x08020000 -and $literal -le 0x0807ffff -and
        $literal -ne 0x08020000) {
        throw ('Boot stub source embeds an application code address: 0x{0:X8}' -f $literal)
    }
}

Require-Match $bootLinker 'ORIGIN\s*=\s*0x08000000\s*,\s*LENGTH\s*=\s*128K' `
    'Boot stub must fit wholly within flash sector 0'
Require-Match $bootLinker 'ASSERT\s*\(\s*BootStub_Reset_Handler\s*==\s*0x08000040' `
    'Boot stub reset entry must be fixed at 0x08000040'
Require-Match $bootCMake 'STM32H725AEIX_BOOT\.ld' `
    'Boot-stub CMake target must use its sector-0 linker script'
Require-Match $bootMake 'STM32H725AEIX_BOOT\.ld' `
    'Boot-stub Make build must use its sector-0 linker script'
Require-Match $inspector '\$appSections\s*=\s*Run\s+\$objdump\s+@\(\s*''-h''\s*,\s*\$ApplicationElf\s*\)' `
    'Artifact inspector must inspect the application section layout'
Require-Match $inspector 'Application \.isr_vector is not at 0x08020000' `
    'Artifact inspector must reject an application vector outside 0x08020000'

$hexBytes = Read-IntelHex (Join-Path $root 'boot_stub/wio_ai_boot_stub.hex')
foreach ($address in $hexBytes.Keys) {
    if ($address -lt 0x08000000 -or $address -gt 0x0801ffff) {
        throw ('Tracked boot-stub HEX contains data outside sector 0 at 0x{0:X8}' -f $address)
    }
}
if ((Read-U32 $hexBytes 0x08000000) -ne 0x24050000) {
    throw 'Tracked boot-stub HEX has the wrong initial MSP'
}
if ((Read-U32 $hexBytes 0x08000004) -ne 0x08000041) {
    throw 'Tracked boot-stub HEX does not point at the fixed Thumb reset entry'
}

$appBaseLiteralCount = 0
foreach ($address in @($hexBytes.Keys | Sort-Object)) {
    if (($address -band 3) -ne 0) {
        continue
    }
    $hasWord = $true
    for ($i = 0; $i -lt 4; ++$i) {
        $hasWord = $hasWord -and $hexBytes.ContainsKey([uint32]($address + $i))
    }
    if (!$hasWord) {
        continue
    }
    $word = Read-U32 $hexBytes $address
    if ($word -eq 0x08020000) {
        ++$appBaseLiteralCount
    } elseif ($word -ge 0x08020000 -and $word -le 0x0807ffff) {
        throw ('Tracked boot-stub HEX embeds an application code address: 0x{0:X8}' -f $word)
    }
}
if ($appBaseLiteralCount -lt 1) {
    throw 'Tracked boot-stub HEX does not contain the dynamic application vector-table base'
}
