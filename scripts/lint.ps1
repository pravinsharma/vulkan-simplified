param(
    [string]$Config = "Release"
)

if (-not (Test-Path build)) {
    Write-Host "Build directory not found. Run .\scripts\build.ps1 first."
    exit 1
}

cmake --build build --config $Config --target vks_lint
