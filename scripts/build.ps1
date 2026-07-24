param(
    [string]$Preset = "ciual",
    [string]$Config = "Release",
    [switch]$Clean
)

if ($Clean) {
    if (Test-Path build) { Remove-Item build -Recurse -Force }
    Write-Host "Cleaned build directory."
    exit 0
}

if (-not (Test-Path build)) {
    cmake --preset $Preset -B build
}

cmake --build build --config $Config
Write-Host "Build complete: $Config"
