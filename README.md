# OBS 高级美颜滤镜

一个面向 OBS Studio 31+ 的开源本地实时美颜视频滤镜。支持 Windows x64 与 Apple Silicon Mac（arm64）；首版仅提供中文界面。

## 当前状态

P0 已实现：可加载的 OBS 视频滤镜、中文配置界面、跨图形后端的 Shader 磨皮/提亮/红润/锐化链路，以及自动、兼容、高质量三个质量模式。

P1 将接入 MediaPipe Face Landmarker 的异步关键点结果，生成脸部 mask 并保护眼睛、眉毛、嘴唇区域。当前 P0 的皮肤区域仅使用保守的像素颜色启发式，因此不应将其视为高阶 AI 美颜的最终效果。第三方依赖和模型的许可证台账见 [THIRD_PARTY.md](docs/THIRD_PARTY.md)。

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

## Windows 构建

在 Windows 的 Visual Studio 2022 开发者终端中，提供目标 OBS 版本的源码目录与 `libobs` 库路径：

```powershell
cmake --preset windows-x64 -DOBS_SOURCE_DIR=C:/src/obs-studio -DOBS_LIBRARY=C:/path/to/obs.lib
cmake --build --preset windows-x64 --config RelWithDebInfo
cmake --install build/windows-x64 --config RelWithDebInfo --prefix dist
```

安装布局遵循 OBS 的 `obs-beauty-filter/bin/64bit` 与 `obs-beauty-filter/data` 目录结构。

## 开发文档

完整范围、架构、性能分级与待确认需求见 [开发说明](docs/OBS_BEAUTY_PLUGIN_DEV_SPEC.md)。
阶段目标、当前进度与验收门槛见 [开发计划](docs/DEVELOPMENT_PLAN.md)。

## 许可证

GPL-2.0-or-later。模型、模型权重和第三方运行时将在引入前单独完成许可证审查。
