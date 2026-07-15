# Downloads and verifies the pinned Face Landmarker model on Windows.
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$metadata = Get-Content (Join-Path $repoRoot 'models/face_landmarker.json') -Raw | ConvertFrom-Json
$destination = Join-Path $repoRoot 'models/face_landmarker.task'

if (Test-Path $destination) {
  $actual = (Get-FileHash $destination -Algorithm SHA256).Hash.ToLowerInvariant()
  if ($actual -eq $metadata.sha256.ToLowerInvariant()) {
    Write-Host "模型已存在且校验通过：$destination"
    exit 0
  }
  throw "现有模型的 SHA-256 不匹配；请先手动移除：$destination"
}

Invoke-WebRequest -Uri $metadata.url -OutFile $destination
$actual = (Get-FileHash $destination -Algorithm SHA256).Hash.ToLowerInvariant()
if ($actual -ne $metadata.sha256.ToLowerInvariant()) {
  throw "下载模型的 SHA-256 不匹配：$actual"
}
Write-Host "模型下载并校验通过：$destination"
