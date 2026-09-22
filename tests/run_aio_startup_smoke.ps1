param([Parameter(Mandatory=$true)][string]$Sm86Source, [string]$RuntimeDll)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$testRoot = Join-Path $root ('build_out/aio-smoke-' + [Guid]::NewGuid().ToString('N'))
$exe = Join-Path $root 'build_out/sm86-tests/AioStartupSmoke.exe'
foreach ($mode in @('absent', 'disabled', 'enabled')) {
    $stage = Join-Path $testRoot $mode
    New-Item -ItemType Directory -Path $stage | Out-Null
    $aurora = Join-Path $stage 'dxgi.dll'
    $stageExe = Join-Path $stage 'AioStartupSmoke.exe'
    Copy-Item -LiteralPath $exe -Destination $stageExe
    Copy-Item -LiteralPath (Join-Path $root 'x64/Release/a/OptiScaler.dll') -Destination $aurora
    $config = Get-Content -LiteralPath (Join-Path $root 'OptiScaler.ini') -Raw
    $config = $config -replace '(?m)^CheckForUpdate=.*$', 'CheckForUpdate=false'
    Set-Content -LiteralPath (Join-Path $stage 'OptiScaler.ini') -Value $config -Encoding utf8
    $component = Join-Path $stage 'OptiScaler/SM86'
    $proxy = Join-Path $component 'version.dll'
    $enabled = if ($mode -eq 'enabled') { 1 } else { 0 }
    if ($mode -ne 'absent') {
        New-Item -ItemType Directory -Path $component | Out-Null
        Copy-Item -LiteralPath (Join-Path $Sm86Source 'version.dll') -Destination $proxy
        $ini = Get-Content -LiteralPath (Join-Path $Sm86Source 'dlssg_sm86.ini') -Raw -Encoding utf8
        $ini = $ini -replace '(?m)^Enabled=1\r?$', "Enabled=$enabled"
        $cache = Join-Path $testRoot 'sm86-test-cache'
        $ini = $ini -replace '(?m)^CacheDirectory=.*$', "CacheDirectory=$cache"
        [IO.File]::WriteAllText((Join-Path $component 'dlssg_sm86.ini'), $ini, [Text.UTF8Encoding]::new($true))
    }
    $runtime = if ($RuntimeDll) { (Resolve-Path -LiteralPath $RuntimeDll).Path } else { Join-Path $root 'dist/nvngx/nvngx_dlssg.dll' }
    $process = New-Object Diagnostics.Process
    $process.StartInfo.FileName = $stageExe
    $process.StartInfo.Arguments = '"' + $aurora + '" "' + $proxy + '" ' + $enabled + ' "' + $runtime + '"'
    $process.StartInfo.WorkingDirectory = $stage
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit(20000)) {
        $process.Kill()
        throw "AIO startup timed out: $mode"
    }
    $stdout.Result | Set-Content -LiteralPath (Join-Path $stage 'stdout.txt')
    $stderr.Result | Set-Content -LiteralPath (Join-Path $stage 'stderr.txt')
    Write-Output "$mode :"
    $stdout.Result -split "`r?`n" | Where-Object { $_ -match '^(PASS:|Runtime path:)' } | Write-Output
    if ($stderr.Result) { Write-Output $stderr.Result }
    $code = $process.ExitCode
    $process.Dispose()
    if ($code -ne 0) { throw "AIO startup failed: $mode, exit $code (logs: $stage)" }
}
Write-Output "PASS: all AIO startup modes (logs: $testRoot)"
