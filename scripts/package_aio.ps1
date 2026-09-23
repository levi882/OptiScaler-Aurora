param(
    [Parameter(Mandatory=$true)][string]$BasePackage,
    [Parameter(Mandatory=$true)][string]$Sm86Source,
    [string]$RhiCache = (Join-Path (Split-Path -Parent $PSScriptRoot) 'build_out/rhi'),
    [string]$ArchiveTool = 'C:/Program Files/Bandizip/bz.exe',
    [string]$Version = 'aurora-aio-preview.1-rhi2'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $RhiCache) { throw 'RHI cache is required: run scripts/fetch_rhi_runtimes.ps1 first' }
$runtimeLock = Join-Path $PSScriptRoot 'rhi-runtimes.lock.json'
$rhi = Get-Content -LiteralPath $runtimeLock -Raw | ConvertFrom-Json
$nrPackages = @($rhi.packages | Where-Object { $_.files.name -contains 'nvngx_dlssnr.dll' })
if ($nrPackages.Count -ne 1 -or $nrPackages[0].tag -ne 'dlssnr-310.8.SF-v2' -or $nrPackages[0].optional) {
    throw 'AIO must contain only ShortFuse SF-v2 as the default NR runtime'
}
if ($Version -notmatch '^[a-zA-Z0-9._-]+$') { throw 'Invalid version filename' }
$output = Join-Path $root "release/$Version"
$archive = Join-Path $root "release/OptiScaler_$Version.7z"
if ((Test-Path -LiteralPath $output) -or (Test-Path -LiteralPath $archive)) { throw 'Use a new version/output name; existing releases are not overwritten' }
$expectedBase = '02c168f15468e193a17476583a22e6bbd919761dd7e940e686c96937423c78bc'
$expectedSm86 = 'c3934a09399f022504227c72df0bf8c0de55f9a08880dddde898c5262cefa838'
if ((Get-FileHash -LiteralPath $BasePackage -Algorithm SHA256).Hash -ine $expectedBase) { throw 'Unexpected Aurora base archive' }
if ((Get-FileHash -LiteralPath (Join-Path $Sm86Source 'version.dll') -Algorithm SHA256).Hash -ine $expectedSm86) { throw 'Unexpected SM86 binary; revalidate before updating the pinned version' }
$mainDll = Join-Path $root 'x64/Release/a/OptiScaler.dll'
if ((Get-Item -LiteralPath $mainDll).VersionInfo.ProductVersion -notmatch 'aio-preview') { throw 'Build the AIO configuration before packaging' }
New-Item -ItemType Directory -Path $output | Out-Null
& $ArchiveTool x -y "-o:$output" $BasePackage
if ($LASTEXITCODE -ne 0) { throw 'Base extraction failed' }
Copy-Item -LiteralPath $mainDll,(Join-Path $root 'x64/Release/a/nvngx.dll_dlssnr.dll') -Destination $output -Force
foreach ($name in @('nvngx.dll_dlssnr.exp','nvngx.dll_dlssnr.lib','nvngx.dll_dlssnr.pdb')) {
    $path = Join-Path $output $name
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
}
$component = Join-Path $output 'OptiScaler/SM86'
New-Item -ItemType Directory -Path $component | Out-Null
foreach ($name in @('version.dll','README.md','README.en.md','THIRD_PARTY_NOTICES.txt')) {
    Copy-Item -LiteralPath (Join-Path $Sm86Source $name) -Destination $component
}
Copy-Item -LiteralPath (Join-Path $Sm86Source 'docs') -Destination (Join-Path $component 'docs') -Recurse
$ini = Get-Content -LiteralPath (Join-Path $Sm86Source 'dlssg_sm86.ini') -Raw -Encoding utf8
if ([regex]::Matches($ini, '(?m)^Enabled=1\r?$').Count -ne 1) { throw 'Unexpected SM86 factory config' }
$ini = $ini -replace '(?m)^Enabled=1\r?$', 'Enabled=0'
$ini = "; Aurora AIO: opt-in via the RTX 20/30 panel; save and restart the game.`r`n" + $ini
[IO.File]::WriteAllText((Join-Path $component 'dlssg_sm86.ini'), $ini, [Text.UTF8Encoding]::new($true))
Copy-Item -LiteralPath (Join-Path $root 'docs/AIO_SM86.md') -Destination (Join-Path $output 'README_AIO.md')
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination $output
if ($RhiCache) {
    $optional = Join-Path $output 'Optional/Runtimes'
    New-Item -ItemType Directory -Path $optional | Out-Null
    foreach ($package in $rhi.packages) {
        $folder = Join-Path $RhiCache $package.tag
        $zip = Join-Path $folder $package.asset
        if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ine $package.sha256) {
            throw "RHI archive checksum mismatch: $zip"
        }
        if ($package.optional) {
            # Keep alternatives compressed so recursive DLL discovery cannot select them.
            Copy-Item -LiteralPath $zip -Destination $optional
        } else {
            foreach ($file in $package.files) {
                $path = Join-Path (Join-Path $folder 'extracted') $file.name
                if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $file.sha256) {
                    throw "RHI runtime checksum mismatch: $path"
                }
                Copy-Item -LiteralPath $path -Destination (Join-Path $output 'OptiScaler') -Force
            }
        }
    }
    Copy-Item -LiteralPath $runtimeLock -Destination (Join-Path $output 'RHI_RUNTIMES.json')
    Copy-Item -LiteralPath (Join-Path $root 'docs/RHI_RUNTIMES.md') -Destination (Join-Path $output 'README_RHI_RUNTIMES.md')
}
$commit = git -C $root rev-parse HEAD
$dirty = @(git -C $root status --porcelain --untracked-files=no).Count -gt 0
$build = @{
    version=$Version; sourceCommit=$commit; sourceHasLocalChanges=$dirty
    dllProductVersion=(Get-Item -LiteralPath $mainDll).VersionInfo.ProductVersion
    sm86Version='0.3.5'; sm86Source='https://github.com/sdli1995/dlssg_for_sm86'
    sm86Commit='9621db573e07ed54f50c15bbb585ed9a7bdfac28'; sm86SHA256=$expectedSm86
    baseSHA256=$expectedBase; inGameValidated=$false
    rhiRuntimesIncluded=[bool]$RhiCache
    nrRuntime='ShortFuse 310.8.SF-v2'
}
$build | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'AIO_BUILD.json') -Encoding utf8
$hashes = @(Get-ChildItem -LiteralPath $output -Recurse -File | Sort-Object FullName | ForEach-Object {
    (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() + '  ' + $_.FullName.Substring($output.Length + 1).Replace('\','/')
})
$hashes | Set-Content -LiteralPath (Join-Path $output 'SHA256SUMS.txt') -Encoding ascii
& $ArchiveTool c -fmt:7z -l:5 -r $archive "$output/*"
if ($LASTEXITCODE -ne 0) { throw 'Compression failed' }
& $ArchiveTool t $archive
if ($LASTEXITCODE -ne 0) { throw 'Archive integrity check failed' }
Get-FileHash -LiteralPath $archive -Algorithm SHA256 | Format-List
Write-Output "AIO archive: $archive"
