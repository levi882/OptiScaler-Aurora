param([string]$Destination)
$ErrorActionPreference = 'Stop'
if (-not $Destination) { $Destination = Join-Path (Split-Path -Parent $PSScriptRoot) 'build_out/rhi' }
$lock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'rhi-runtimes.lock.json') -Raw | ConvertFrom-Json
foreach ($package in $lock.packages) {
    $folder = Join-Path $Destination $package.tag
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    $zip = Join-Path $folder $package.asset
    if (-not (Test-Path -LiteralPath $zip)) {
        & gh release download $package.tag --repo $lock.repository --dir $folder --pattern $package.asset
        if ($LASTEXITCODE -ne 0) { throw "Download failed: $($package.tag)" }
    }
    if ((Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash -ine $package.sha256) {
        throw "Archive checksum mismatch: $zip"
    }
    $extracted = Join-Path $folder 'extracted'
    if (-not (Test-Path -LiteralPath $extracted)) { Expand-Archive -LiteralPath $zip -DestinationPath $extracted }
    foreach ($file in $package.files) {
        $path = Join-Path $extracted $file.name
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $file.sha256) {
            throw "Runtime checksum mismatch: $path"
        }
    }
    Write-Output "Verified: $($package.tag)"
}
