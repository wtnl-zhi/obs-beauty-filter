# Produces a checksummed Windows x64 release archive from an installed plugin tree.
# Run in PowerShell after a successful Visual Studio 2022 x64 build and install.
[CmdletBinding()]
param(
  [string]$InstallRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) 'dist'),
  [string]$Version = '0.1.0',
  [string]$ReleaseDir = (Join-Path (Split-Path -Parent $PSScriptRoot) 'dist/release')
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$pluginRoot = Join-Path $InstallRoot 'obs-beauty-filter'
$requiredFiles = @(
  'bin/64bit/obs-beauty-filter.dll',
  'bin/64bit/face_landmarker.dll',
  'bin/64bit/opencv_world3410.dll',
  'data/obs-plugins/obs-beauty-filter/beauty.effect',
  'data/obs-plugins/obs-beauty-filter/face_landmarker.task',
  'data/obs-plugins/obs-beauty-filter/locale/zh-CN.ini'
)

if (-not [Environment]::Is64BitProcess) {
  throw '仅能在 Windows x64 PowerShell 中打包'
}
foreach ($relativePath in $requiredFiles) {
  if (-not (Test-Path (Join-Path $pluginRoot $relativePath))) {
    throw "发布包缺少必需文件：$relativePath"
  }
}

$stage = Join-Path ([IO.Path]::GetTempPath()) ("obs-beauty-release-" + [guid]::NewGuid())
$payload = Join-Path $stage "obs-beauty-filter-$Version-windows-x64"
try {
  New-Item -ItemType Directory -Force -Path $payload | Out-Null
  Copy-Item $pluginRoot (Join-Path $payload 'obs-beauty-filter') -Recurse -Force
  Copy-Item (Join-Path $repoRoot 'LICENSE') (Join-Path $payload 'LICENSE') -Force
  Copy-Item (Join-Path $repoRoot 'docs/THIRD_PARTY.md') (Join-Path $payload 'THIRD_PARTY.md') -Force

  New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null
  $archive = Join-Path $ReleaseDir "obs-beauty-filter-$Version-windows-x64.zip"
  Remove-Item $archive -Force -ErrorAction SilentlyContinue
  Remove-Item "$archive.sha256" -Force -ErrorAction SilentlyContinue
  Compress-Archive -Path $payload -DestinationPath $archive -CompressionLevel Optimal
  $hash = (Get-FileHash $archive -Algorithm SHA256).Hash.ToLowerInvariant()
  Set-Content -NoNewline -Path "$archive.sha256" -Value "$hash  $([IO.Path]::GetFileName($archive))"
  Write-Host "已生成：$archive"
} finally {
  Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
}
