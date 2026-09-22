param([string]$VsDevCmd)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $VsDevCmd) {
    $VsDevCmd = Join-Path $root 'build-tools/VS2022/Common7/Tools/VsDevCmd.bat'
    if (-not (Test-Path -LiteralPath $VsDevCmd)) {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
        $installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        $VsDevCmd = Join-Path $installation 'Common7/Tools/VsDevCmd.bat'
    }
}
$output = Join-Path $root 'build_out/sm86-tests'
New-Item -ItemType Directory -Force -Path $output | Out-Null
$commands = @"
@echo off
call "$VsDevCmd" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /utf-8 /I"$PSScriptRoot/support" /I"$root/OptiScaler" /I"$root/external/simpleini" "$PSScriptRoot/Sm86SettingsTests.cpp" "$root/OptiScaler/framegen/dlssg/Sm86Settings.cpp" /Fe:"$output/Sm86SettingsTests.exe" /Fo:"$output/"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /utf-8 /I"$PSScriptRoot/support" "$PSScriptRoot/Sm86ProxySmoke.cpp" /Fe:"$output/Sm86ProxySmoke.exe" /Fo:"$output/"
if errorlevel 1 exit /b 1
cl /nologo /std:c++20 /EHsc /MD /utf-8 /I"$PSScriptRoot/support" "$PSScriptRoot/AioStartupSmoke.cpp" /Fe:"$output/AioStartupSmoke.exe" /Fo:"$output/"
exit /b %errorlevel%
"@
$commandFile = Join-Path $output 'build-tests.cmd'
Set-Content -LiteralPath $commandFile -Value $commands -Encoding ascii
& $env:ComSpec /d /c $commandFile
if ($LASTEXITCODE -ne 0) { throw 'Test build failed' }
& "$output/Sm86SettingsTests.exe" $output
if ($LASTEXITCODE -ne 0) { throw 'Settings tests failed' }
