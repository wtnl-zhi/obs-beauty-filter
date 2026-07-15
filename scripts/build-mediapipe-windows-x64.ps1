# Builds the pinned native MediaPipe C API runtime for Windows x64.
# Run from a Visual Studio 2022 x64 Developer PowerShell with Bazelisk installed.
[CmdletBinding()]
param(
  [string]$WorkRoot = $(if ($env:OBS_BEAUTY_THIRD_PARTY_WORK_ROOT) { $env:OBS_BEAUTY_THIRD_PARTY_WORK_ROOT } else { Join-Path $env:TEMP 'obs-beauty-third-party' }),
  [string]$OpenCvRoot = 'C:\opencv\build',
  [string]$ArtifactDir = $(if ($env:OBS_BEAUTY_MEDIAPIPE_ARTIFACT_DIR) { $env:OBS_BEAUTY_MEDIAPIPE_ARTIFACT_DIR } else { Join-Path (Split-Path -Parent $PSScriptRoot) 'third_party/prebuilt/windows-x64' })
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$mediapipeRoot = Join-Path $WorkRoot 'mediapipe-0.10.35'
$mediapipeCommit = 'f8ef212d5c962c0e853db7e59d217056b187084b'
$faceTarget = '//mediapipe/tasks/c/vision/face_landmarker:face_landmarker.dll'
$opencvDll = Join-Path $OpenCvRoot 'x64/vc15/bin/opencv_world3410.dll'

foreach ($command in @('git', 'bazel', 'cl')) {
  if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
    throw "需要在 VS2022 x64 开发者 PowerShell 中运行，并安装 Bazelisk：缺少 $command"
  }
}
if (-not (Test-Path $opencvDll)) {
  throw "未找到 OpenCV 3.4.10 Windows x64 运行时：$opencvDll"
}

New-Item -ItemType Directory -Force -Path $WorkRoot, $ArtifactDir | Out-Null
if (-not (Test-Path (Join-Path $mediapipeRoot '.git'))) {
  git clone https://github.com/google-ai-edge/mediapipe.git $mediapipeRoot
  git -C $mediapipeRoot checkout $mediapipeCommit
}
git -C $mediapipeRoot diff --quiet
if ($LASTEXITCODE -eq 0) {
  Get-Content (Join-Path $repoRoot 'third_party/patches/mediapipe-0.10.35-native.patch') -Raw |
    git -C $mediapipeRoot apply
  $workspace = Join-Path $mediapipeRoot 'WORKSPACE'
  $escapedOpenCvRoot = $OpenCvRoot.Replace('\', '\\')
  $content = [IO.File]::ReadAllText($workspace).Replace('__OBS_BEAUTY_WINDOWS_OPENCV_PREFIX__', $escapedOpenCvRoot)
  [IO.File]::WriteAllText($workspace, $content)
}

Push-Location $mediapipeRoot
try {
  bazel build --config windows -c opt --strip always --define MEDIAPIPE_DISABLE_GPU=1 $faceTarget
} finally {
  Pop-Location
}

$faceDll = Join-Path $mediapipeRoot 'bazel-bin/mediapipe/tasks/c/vision/face_landmarker/face_landmarker.dll'
if (-not (Test-Path $faceDll)) {
  throw "未找到 MediaPipe DLL：$faceDll"
}
$faceImportLibrary = Get-ChildItem (Join-Path $mediapipeRoot 'bazel-bin/mediapipe/tasks/c/vision/face_landmarker') -Filter 'face_landmarker*.lib' |
  Select-Object -First 1
if (-not $faceImportLibrary) {
  throw '未找到 MediaPipe DLL 对应的导入库（.lib）'
}

Copy-Item $faceDll (Join-Path $ArtifactDir 'face_landmarker.dll') -Force
Copy-Item $faceImportLibrary.FullName (Join-Path $ArtifactDir 'face_landmarker.lib') -Force
Copy-Item $opencvDll (Join-Path $ArtifactDir 'opencv_world3410.dll') -Force

Write-Host "已生成运行时：$ArtifactDir"
Write-Host 'CMake 参数：'
Write-Host "  -DMEDIAPIPE_ROOT=$mediapipeRoot"
Write-Host "  -DMEDIAPIPE_FACE_LANDMARKER_LIBRARY=$(Join-Path $ArtifactDir 'face_landmarker.lib')"
Write-Host "  -DMEDIAPIPE_FACE_LANDMARKER_RUNTIME=$(Join-Path $ArtifactDir 'face_landmarker.dll')"
Write-Host "  -DMEDIAPIPE_RUNTIME_LIBRARIES=$(Join-Path $ArtifactDir 'opencv_world3410.dll')"
