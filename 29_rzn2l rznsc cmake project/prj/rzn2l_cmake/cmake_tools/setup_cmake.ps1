param(
    [Parameter(Mandatory = $true)]
    [string]$Root
)

$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path $Root).Path
$cmakeDir = Join-Path $Root 'cmake'
$configFile = Join-Path $Root 'Config.cmake'
$gccFile = Join-Path $cmakeDir 'gcc.cmake'
$linkerFile = Join-Path $Root 'script\fsp_xspi0_boot.ld'
$defaultToolchain = 'D:/Program Files/GCC/arm-gnu-toolchain-13.3.rel1-mingw-w64-i686-arm-none-eabi/bin'

if (-not (Test-Path $configFile)) { throw "Config.cmake not found: $configFile" }
if (-not (Test-Path $gccFile)) { throw "cmake/gcc.cmake not found: $gccFile" }
if (-not (Test-Path $linkerFile)) { throw "Linker script not found: $linkerFile" }

$config = Get-Content -LiteralPath $configFile -Raw
$match = [regex]::Match($config, '(?m)^\s*set\(CMAKE_FIND_ROOT_PATH\s+"([^"]+)"\)')
$toolchainBin = if ($match.Success) { $match.Groups[1].Value } else { $defaultToolchain }
$gcc = Join-Path ($toolchainBin -replace '/', '\') 'arm-none-eabi-gcc.exe'

if (-not (Test-Path $gcc)) {
    if (Test-Path (Join-Path ($defaultToolchain -replace '/', '\') 'arm-none-eabi-gcc.exe')) {
        $toolchainBin = $defaultToolchain
    } else {
        throw "GNU Arm GCC was not found. Set CMAKE_FIND_ROOT_PATH in Config.cmake."
    }
}
$newLine = "set(CMAKE_FIND_ROOT_PATH `"$toolchainBin`")"
if ($match.Success) {
    $config = [regex]::Replace($config, '(?m)^\s*set\(CMAKE_FIND_ROOT_PATH\s+"[^"]+"\)', $newLine, 1)
} else {
    $config = $config.TrimEnd() + "`r`n`r`n# GNU Arm toolchain bin directory`r`n$newLine`r`n"
}
Set-Content -LiteralPath $configFile -Value $config -Encoding ASCII
Write-Host "Configured GNU Arm toolchain: $toolchainBin"

$make = Get-Command mingw32-make.exe -ErrorAction SilentlyContinue
if (-not $make) { throw 'mingw32-make.exe was not found in PATH.' }
Write-Host "MinGW make: $($make.Source)"

$gccText = Get-Content -LiteralPath $gccFile -Raw
$gccText = [regex]::Replace($gccText, '(?ms)\r?\n# BEGIN RZN2L CMAKE TOOLCHAIN OVERRIDES.*?# END RZN2L CMAKE TOOLCHAIN OVERRIDES\r?\n?', "`r`n")
$overrideBlock = @'
# BEGIN RZN2L CMAKE TOOLCHAIN OVERRIDES
SET(CMAKE_CXX_FLAGS "${RASC_CMAKE_CXX_FLAGS} -Og -mfloat-abi=hard -mfpu=neon-fp-armv8")
SET(CMAKE_ASM_FLAGS "${RASC_CMAKE_ASM_FLAGS} -mfloat-abi=hard -mfpu=neon-fp-armv8")
SET(CMAKE_C_FLAGS "${RASC_CMAKE_C_FLAGS} -Og -mfloat-abi=hard -mfpu=neon-fp-armv8")
SET(CMAKE_EXE_LINKER_FLAGS "${RASC_CMAKE_EXE_LINKER_FLAGS} -Og -mfloat-abi=hard -mfpu=neon-fp-armv8 -nostartfiles -u _printf_float --specs=nosys.specs -Wl,-e,system_init")
# END RZN2L CMAKE TOOLCHAIN OVERRIDES
'@
$gccText = $gccText.TrimEnd() + "`r`n" + $overrideBlock
Set-Content -LiteralPath $gccFile -Value $gccText -Encoding ASCII
Write-Host 'Configured cmake/gcc.cmake.'

$ld = Get-Content -LiteralPath $linkerFile
$ldLines = [System.Collections.Generic.List[string]]::new()
$inLoader = $false
foreach ($line in $ld) {
    if ($line -match '^\s*\.loader_text\b') { $inLoader = $true }
    if ($inLoader -and $line -match '\.o\(' -and $line -notmatch 'crtbegin') {
        $ldLines.Add($line)
        $converted = $line -replace '\.o\(', '.c.obj('
        if ($converted -ne $line -and -not ($ld -contains $converted)) { $ldLines.Add($converted) }
    } else {
        $ldLines.Add($line)
    }
    if ($inLoader -and $line -match '^\s*\.intvec\b') { $inLoader = $false }
}
Set-Content -LiteralPath $linkerFile -Value $ldLines -Encoding ASCII
Write-Host 'Configured loader object matching in script/fsp_xspi0_boot.ld.'
Write-Host 'CMake project setup completed.'
