# OBS 高级美颜滤镜

一个面向 OBS Studio 31+ 的开源本地实时美颜视频滤镜。支持 Windows x64 与 Apple Silicon Mac（arm64）；首版仅提供中文界面。

构建系统会拒绝 Intel Mac、Linux 和 Windows 非 x64 配置，避免产生超出支持范围的安装包。

## 当前状态

P0 已实现：可加载的 OBS 视频滤镜、中文配置界面、跨图形后端的 Shader 磨皮/提亮/红润/锐化链路，以及自动、兼容、高质量三个质量模式。

P1 已完成 MediaPipe Face Landmarker 的原生运行时、模型加载、视频帧推理适配层、关键点转换、有界后台推理工作器、OBS 纹理帧桥和 GPU 局部美颜 mask；Apple Silicon 的 CPU/XNNPACK 路径已通过真实模型推理及工作器测试，并已在隔离插件路径下通过 OBS 31.0.2 的启动/卸载 smoke test。构建还会运行 `obs-module-smoke`：由 OBS `libobs` 直接加载 bundle，确认滤镜注册和中文属性资源可用。帧桥使用双 staging surface，工作器只保留最新帧，渲染端不会等待 GPU 读回或模型推理。局部 mask 使用脸部椭圆轮廓并保护眼睛、嘴部，超过 500ms 未更新的轨迹自动失效；它不是像素级皮肤分割。质量模式会调整采样档：自动为 256px/10fps、兼容为 192px/6fps、高质量为 384px/15fps。现已提供可调的 mask 羽化与诊断预览；下一步是含滤镜实例的 OBS 场景验证、遮挡处理和边缘调校。第三方依赖和模型的许可证台账见 [THIRD_PARTY.md](docs/THIRD_PARTY.md)。

## 支持范围

- Windows：x64，Windows 10 22H2+。
- macOS：Apple Silicon（arm64），macOS 12+。
- 不支持 Intel Mac、Linux 或 Windows ARM。

## 本机构建（macOS）

构建需要与目标 OBS 版本相匹配的 OBS Studio 源码，以及本机 OBS 安装中的 `libobs`。以下命令以 OBS 31.0.2 为例：

```sh
cmake --preset macos-arm64 \
  -DOBS_SOURCE_DIR=/path/to/obs-studio \
  -DOBS_LIBRARY=/Applications/OBS.app/Contents/Frameworks/libobs.framework/libobs
cmake --build --preset macos-arm64
```

输出为 `build/macos-arm64/RelWithDebInfo/obs-beauty-filter.plugin`。将其复制到 `~/Library/Application Support/obs-studio/plugins/` 后重启 OBS，在摄像头源的“滤镜”中添加“高级美颜”。正式分发前必须使用 Apple Developer ID 签名并公证。

构建后的 P1 安装副本可用下列命令验证；它会安装到全新临时目录，并检查 bundle 资源、运行时、`libobs` 模块加载和 ad-hoc 签名完整性：

```zsh
scripts/verify-macos-p1-install.zsh
```

这不是 Developer ID/notarization 或真实摄像头场景验收的替代。

要构建带 P1 人脸运行时的 bundle，先下载模型并生成 Apple Silicon 原生依赖：

```sh
scripts/fetch-models.zsh
scripts/build-mediapipe-macos-arm64.zsh
```

再使用脚本输出的 `MEDIAPIPE_*` 参数配置 CMake。该构建会将 Face Landmarker、最小 OpenCV Core/Imgproc 运行时和模型一起打入 `.plugin`；目前只在 macOS arm64 实测通过。

## Windows 构建

在 Windows 10 22H2+ 的 Visual Studio 2022 x64 开发者 PowerShell 中，先准备 Bazelisk 与 OpenCV 3.4.10 Windows x64 包（默认展开至 `C:\opencv\build`），再下载模型并构建运行时：

```powershell
scripts\fetch-models.ps1
scripts\build-mediapipe-windows-x64.ps1
```

脚本会生成 `third_party\prebuilt\windows-x64` 下的 `face_landmarker.dll`、导入库和 `opencv_world3410.dll`。随后提供目标 OBS 版本的源码目录与 `libobs` 库路径：

```powershell
cmake --preset windows-x64 -DOBS_SOURCE_DIR=C:/src/obs-studio -DOBS_LIBRARY=C:/path/to/obs.lib -DOBS_BEAUTY_ENABLE_MEDIAPIPE=ON -DMEDIAPIPE_ROOT=$env:TEMP/obs-beauty-third-party/mediapipe-0.10.35 -DMEDIAPIPE_FACE_LANDMARKER_LIBRARY=third_party/prebuilt/windows-x64/face_landmarker.lib -DMEDIAPIPE_FACE_LANDMARKER_RUNTIME=third_party/prebuilt/windows-x64/face_landmarker.dll -DMEDIAPIPE_RUNTIME_LIBRARIES=third_party/prebuilt/windows-x64/opencv_world3410.dll
cmake --build --preset windows-x64 --config RelWithDebInfo
ctest --test-dir build/windows-x64 --config RelWithDebInfo --output-on-failure
cmake --install build/windows-x64 --config RelWithDebInfo --prefix dist
```

其中 `obs-module-smoke` 会直接加载构建出的 DLL，确认 MediaPipe/OpenCV 运行时、滤镜注册和中文资源可用；安装布局遵循 OBS 的 `obs-beauty-filter/bin/64bit` 与 `obs-beauty-filter/data` 目录结构。该脚本和打包参数已完成代码审查，尚待 Windows x64 原生构建机和 OBS 实机验证。

## 开发文档

完整范围、架构、性能分级与待确认需求见 [开发说明](docs/OBS_BEAUTY_PLUGIN_DEV_SPEC.md)。
阶段目标、当前进度与验收门槛见 [开发计划](docs/DEVELOPMENT_PLAN.md)。
人工 OBS 场景验收步骤见 [场景验收清单](docs/OBS_SCENE_VALIDATION.md)。
安装、卸载、故障排查与性能建议见 [用户指南](docs/USER_GUIDE.md)。
支持矩阵、硬件基线与已知限制见 [COMPATIBILITY.md](docs/COMPATIBILITY.md)，版本变更见 [CHANGELOG.md](CHANGELOG.md)。
macOS 与 Windows 的签名/打包入口位于 `scripts/package-macos-arm64.zsh` 和 `scripts/package-windows-x64.ps1`。

## 许可证

GPL-2.0-or-later。MediaPipe、OpenCV 及模型的审查与固定版本见 [THIRD_PARTY.md](docs/THIRD_PARTY.md)。
