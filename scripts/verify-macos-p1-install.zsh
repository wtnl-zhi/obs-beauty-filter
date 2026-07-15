#!/bin/zsh
# Installs the P1 bundle into a fresh temporary prefix and verifies that
# libobs can load the installed copy with all required resources present.

set -euo pipefail

repo_root="${0:A:h:h}"
build_dir="${OBS_BEAUTY_BUILD_DIR:-$repo_root/build/macos-arm64}"
configuration="${OBS_BEAUTY_CONFIGURATION:-RelWithDebInfo}"
smoke_test="$build_dir/$configuration/obs-module-smoke-test"

[[ "$(uname -m)" == "arm64" ]] || { print -u2 "仅能在 Apple Silicon 上验证"; exit 1; }
[[ -x "$smoke_test" ]] || { print -u2 "未找到 smoke test：$smoke_test；请先构建"; exit 1; }

stage="$(mktemp -d "${TMPDIR:-/tmp}/obs-beauty-installed-p1.XXXXXX")"
if [[ "${OBS_BEAUTY_KEEP_INSTALL_STAGE:-0}" != "1" ]]; then
  trap 'rm -rf "$stage"' EXIT
fi

cmake --install "$build_dir" --config "$configuration" --prefix "$stage"
plugin="$stage/obs-beauty-filter.plugin"
[[ -d "$plugin" ]] || { print -u2 "未安装 plugin bundle"; exit 1; }

for required_file in \
  "$plugin/Contents/Resources/beauty.effect" \
  "$plugin/Contents/Resources/locale/zh-CN.ini" \
  "$plugin/Contents/Resources/face_landmarker.task" \
  "$plugin/Contents/Frameworks/libface_landmarker.dylib" \
  "$plugin/Contents/Frameworks/libopencv_core.3.4.dylib" \
  "$plugin/Contents/Frameworks/libopencv_imgproc.3.4.dylib"; do
  [[ -f "$required_file" ]] || { print -u2 "安装副本缺少文件：$required_file"; exit 1; }
done

"$smoke_test" "$plugin/Contents/MacOS/obs-beauty-filter" "$plugin/Contents/Resources"
codesign --verify --deep --strict --verbose=2 "$plugin"
print "P1 安装副本验证通过：$plugin"
if [[ "${OBS_BEAUTY_KEEP_INSTALL_STAGE:-0}" == "1" ]]; then
  print "保留临时安装目录：$stage"
fi
