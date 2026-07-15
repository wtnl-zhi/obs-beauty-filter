#!/bin/zsh
# Produces a signed, checksummed Apple Silicon release archive from an installed bundle.

set -euo pipefail

repo_root="${0:A:h:h}"
build_dir="${OBS_BEAUTY_BUILD_DIR:-$repo_root/build/macos-arm64}"
configuration="${OBS_BEAUTY_CONFIGURATION:-RelWithDebInfo}"
version="${OBS_BEAUTY_VERSION:-$(sed -n 's/^project(obs-beauty-filter VERSION \([^ ]*\).*/\1/p' "$repo_root/CMakeLists.txt")}"
output_dir="${OBS_BEAUTY_RELEASE_DIR:-$repo_root/dist}"
identity="${OBS_BEAUTY_CODESIGN_IDENTITY:-}"

[[ "$(uname -m)" == "arm64" ]] || { print -u2 "仅能在 Apple Silicon 上打包"; exit 1; }
[[ -n "$version" ]] || { print -u2 "无法确定版本号"; exit 1; }
[[ -n "$identity" ]] || { print -u2 "设置 OBS_BEAUTY_CODESIGN_IDENTITY 为 Developer ID Application 证书名"; exit 1; }
[[ "$identity" == Developer\ ID\ Application:* ]] || {
  print -u2 "必须使用 Developer ID Application 证书，不能使用 Apple Development 或 ad-hoc 签名"
  exit 1
}
security find-identity -v -p codesigning | grep -Fq "\"$identity\"" || {
  print -u2 "钥匙串中未找到指定的 Developer ID Application 证书：$identity"
  exit 1
}

stage="$(mktemp -d "${TMPDIR:-/tmp}/obs-beauty-release.XXXXXX")"
payload="$stage/obs-beauty-filter-${version}-macos-arm64"
trap 'rm -rf "$stage"' EXIT
cmake --install "$build_dir" --config "$configuration" --prefix "$payload"

plugin="$payload/obs-beauty-filter.plugin"
[[ -d "$plugin" ]] || { print -u2 "未找到插件 bundle：$plugin"; exit 1; }
[[ -f "$plugin/Contents/Resources/beauty.effect" ]] || exit 1
[[ -f "$plugin/Contents/Resources/locale/zh-CN.ini" ]] || exit 1
[[ -f "$plugin/Contents/Resources/face_landmarker.task" ]] || exit 1
[[ -f "$repo_root/third_party/licenses/Apache-2.0.txt" ]] || { print -u2 "缺少 MediaPipe / 模型许可文本"; exit 1; }
[[ -f "$repo_root/third_party/licenses/OpenCV-BSD-3-Clause.txt" ]] || { print -u2 "缺少 OpenCV 许可文本"; exit 1; }

codesign --force --deep --options runtime --timestamp --sign "$identity" "$plugin"
codesign --verify --deep --strict --verbose=2 "$plugin"

cp "$repo_root/LICENSE" "$payload/LICENSE"
cp "$repo_root/docs/THIRD_PARTY.md" "$payload/THIRD_PARTY.md"
cp -R "$repo_root/third_party/licenses" "$payload/THIRD_PARTY_LICENSES"

mkdir -p "$output_dir"
archive="$output_dir/obs-beauty-filter-${version}-macos-arm64.zip"
rm -f "$archive" "$archive.sha256"
(cd "$stage" && ditto -c -k --sequesterRsrc --keepParent "$(basename "$payload")" "$archive")
shasum -a 256 "$archive" > "$archive.sha256"
print "已生成：$archive"
print "下一步：使用 xcrun notarytool submit 并在 stapler 后对该归档进行干净环境安装复测。"
