# 支持矩阵与已知限制

## 支持矩阵

| 项目 | 支持范围 | 当前验证状态 |
| --- | --- | --- |
| OBS Studio | 31+ | macOS 使用 OBS 31.0.2 完成插件加载/卸载 smoke test；完整场景测试待完成。 |
| macOS | 12+，Apple Silicon arm64 | 原生构建、模型推理、工作器、bundle 资源与 ad-hoc 签名已验证；正式签名、公证和真实场景验收待完成。 |
| Windows | Windows 10 22H2+，x64 | 已提供 VS2022 x64 构建与发布脚本；原生构建、安装和 OBS 实机验证待完成。 |
| Intel Mac / Linux / Windows ARM | 不支持 | CMake 配置会拒绝这些目标。 |

## 可复现构建证据

- 2026-07-15：从 GitHub `main` 的全新浅克隆，在 Apple Silicon 上按 README 指定 OBS 31 开发依赖完成 P0 构建与 4/4 测试。
- 2026-07-15：同一全新浅克隆通过 `scripts/fetch-models.zsh` 重新下载并校验 `face_landmarker.task`，以固定的 Apple Silicon MediaPipe/OpenCV 原生运行时完成 P1 配置、构建和 6/6 测试。这证明源码、模型和已构建运行时能完整装配；运行时本身仍复用了开发机已构建产物。
- 2026-07-15：Apple Silicon P1 构建新增 `obs-module-smoke` 自动测试；它通过 OBS `libobs` 直接加载已构建 bundle，验证版本化滤镜类型 `obs_beauty_filter_v2` 已注册，并能读取中文“预设”等属性。P1 构建共 7/7 测试通过。该测试不创建摄像头场景，不能替代下面的实机验收。
- 2026-07-15：设置迁移逻辑已抽为 `beauty-settings-migration` 单元测试，覆盖 P0 滑杆设置转为“自定义”预设、v1 新增独立开关的默认迁移和当前 v2 设置不被改写。Apple Silicon P1 构建共 8/8 测试通过。
- 2026-07-15：Apple Silicon P1 bundle 通过 `cmake --install` 安装到新的临时前缀后，已用 `libobs` 重新加载；模型、中文资源、MediaPipe 与 OpenCV 运行时都在安装副本中，ad-hoc 签名校验通过。该流程由 `scripts/verify-macos-p1-install.zsh` 固化，仍不等同于正式签名、公证或真实场景验收。
- Windows x64 的同名 CTest 入口已配置：P1 构建会将 MediaPipe/OpenCV DLL 放到模块旁，并以 `data` 资源目录加载模块。它尚未在 Windows 主机执行，不能作为 Windows 兼容结论。
- P1 的完整从零复现仍要求在独立干净机器重新构建固定版本的 MediaPipe/OpenCV，并完成端到端复测。

## 硬件基线

| 档位 | 建议基线 | 目标 | 验证状态 |
| --- | --- | --- | --- |
| 兼容 | Windows 4 核 CPU / 8GB；Apple Silicon Mac | 720p30 | 待实测。 |
| 推荐 | Windows 6 核+ / 16GB；M1+ / 16GB | 1080p30 | 待实测。 |
| 高性能 | 现代独显 4GB+；M1 Pro/M2+ / 16GB+ | 1080p60 | 待实测。 |

## 已知限制

- 当前是关键点驱动的脸部区域与五官保护，不是像素级皮肤分割；头发、胡须、衣物和复杂遮挡的边缘仍须实机调校。
- 首版不包含瘦脸、大眼、妆容、祛痘或瑕疵修复。
- 自动模式根据最近推理耗时在 256px/10fps 与 192px/6fps 之间带滞回调整；实际性能依赖 OBS 场景、摄像头、编码器和机器负载。
- “模型不可用”或运行期推理错误会回退到全画面兼容 Shader，保证不阻塞视频输出。
