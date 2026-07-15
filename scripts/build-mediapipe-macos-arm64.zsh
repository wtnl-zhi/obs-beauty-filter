#!/bin/zsh
# Builds the pinned native MediaPipe runtime used by the Apple Silicon plugin.
# Output is intentionally kept outside the source tree by default.

set -euo pipefail

repo_root="${0:A:h:h}"
work_root="${OBS_BEAUTY_THIRD_PARTY_WORK_ROOT:-${TMPDIR:-/tmp}/obs-beauty-third-party}"
mediapipe_root="$work_root/mediapipe-0.10.35"
opencv_root="$work_root/opencv-3.4.11"
opencv_build="$work_root/opencv-3.4.11-build"
opencv_prefix="$work_root/opencv-3.4.11-install"
artifact_dir="${OBS_BEAUTY_MEDIAPIPE_ARTIFACT_DIR:-$repo_root/third_party/prebuilt/macos-arm64}"

mediapipe_commit="f8ef212d5c962c0e853db7e59d217056b187084b"
face_target="//mediapipe/tasks/c/vision/face_landmarker:libface_landmarker.dylib"

command -v bazel >/dev/null || { print -u2 "需要 Bazelisk 或 Bazel 7.4.1"; exit 1; }
command -v cmake >/dev/null || { print -u2 "需要 CMake"; exit 1; }

mkdir -p "$work_root" "$artifact_dir"

if [[ ! -d "$opencv_root/.git" ]]; then
  git clone --depth 1 --branch 3.4.11 https://github.com/opencv/opencv.git "$opencv_root"
fi
if ! git -C "$opencv_root" diff --quiet -- 3rdparty/zlib/zutil.h; then
  :
else
  git -C "$opencv_root" apply "$repo_root/third_party/patches/opencv-3.4.11-xcode17-zlib.patch"
fi

cmake -S "$opencv_root" -B "$opencv_build" -G "Unix Makefiles" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$opencv_prefix" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
  -DBUILD_LIST=core,imgproc -DBUILD_SHARED_LIBS=ON \
  -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_opencv_apps=OFF \
  -DOPENCV_GENERATE_PKGCONFIG=OFF -DOPENCV_GENERATE_SETUPVARS=OFF \
  -DWITH_FFMPEG=OFF -DWITH_OPENCL=OFF -DWITH_OPENGL=OFF -DWITH_GTK=OFF -DWITH_QT=OFF \
  -DWITH_JPEG=OFF -DWITH_PNG=OFF -DWITH_TIFF=OFF -DWITH_WEBP=OFF -DWITH_OPENEXR=OFF \
  -DWITH_IPP=OFF -DWITH_ITT=OFF -DWITH_LAPACK=OFF -DWITH_EIGEN=OFF -DWITH_V4L=OFF \
  -DWITH_AVFOUNDATION=OFF -DWITH_CARBON=OFF
cmake --build "$opencv_build" --parallel 4
cmake --install "$opencv_build"

if [[ ! -d "$mediapipe_root/.git" ]]; then
  git clone https://github.com/google-ai-edge/mediapipe.git "$mediapipe_root"
  git -C "$mediapipe_root" checkout "$mediapipe_commit"
fi
if git -C "$mediapipe_root" diff --quiet; then
  git -C "$mediapipe_root" apply "$repo_root/third_party/patches/mediapipe-0.10.35-native.patch"
  perl -0pi -e "s|__OBS_BEAUTY_OPENCV_PREFIX__|$opencv_prefix|g" "$mediapipe_root/WORKSPACE"
fi

build_args=(
  --repo_env=HERMETIC_PYTHON_VERSION=3.12
  --config darwin_arm64
  -c opt
  --strip always
  --define MEDIAPIPE_DISABLE_GPU=1
)

# Xcode 17 exposes a legacy zlib fdopen macro conflict. Fetch first so the
# exact external checkout exists, then apply the narrowly scoped compatibility patch.
(cd "$mediapipe_root" && bazel fetch "${build_args[@]}" "$face_target")
output_base="$(cd "$mediapipe_root" && bazel info "${build_args[@]}" output_base)"
patch --forward --batch -d "$output_base/external/zlib" -p1 \
  < "$repo_root/third_party/patches/mediapipe-zlib-xcode17.patch" || true
grep -q '#      if 0' "$output_base/external/zlib/zutil.h" || {
  print -u2 "无法应用 MediaPipe zlib 的 Xcode 17 兼容补丁"
  exit 1
}
(cd "$mediapipe_root" && bazel build "${build_args[@]}" "$face_target")

face_library="$mediapipe_root/bazel-bin/mediapipe/tasks/c/vision/face_landmarker/libface_landmarker.dylib"
core_library="$opencv_prefix/lib/libopencv_core.3.4.dylib"
imgproc_library="$opencv_prefix/lib/libopencv_imgproc.3.4.dylib"

install_name_tool -id @rpath/libface_landmarker.dylib "$face_library"
cp -L "$face_library" "$artifact_dir/libface_landmarker.dylib"
cp -L "$core_library" "$artifact_dir/libopencv_core.3.4.dylib"
cp -L "$imgproc_library" "$artifact_dir/libopencv_imgproc.3.4.dylib"

print "已生成运行时：$artifact_dir"
print "CMake 参数："
print "  -DMEDIAPIPE_ROOT=$mediapipe_root"
print "  -DMEDIAPIPE_FACE_LANDMARKER_LIBRARY=$artifact_dir/libface_landmarker.dylib"
print "  -DMEDIAPIPE_RUNTIME_LIBRARIES=$artifact_dir/libopencv_core.3.4.dylib;$artifact_dir/libopencv_imgproc.3.4.dylib"
