# macOS Apple Silicon 验证记录

验证日期：2026-07-20  
目标环境：Apple Silicon、macOS 26.3、OBS Studio 31.0.2（非 Rosetta）

## 已完成的实机验证

- 将 `obs-beauty-filter.plugin` 安装至用户级 OBS 插件目录后，OBS 真实启动日志列出 `obs-beauty-filter` 模块。
- 在独立的“美颜验收”场景集合中，以无敏感数据的颜色源创建 `高级美颜` 滤镜实例；日志确认 `MediaPipe face region enabled`。
- OBS 预览持续渲染该测试源，状态栏为 60 FPS；本次观察 CPU 约为 1.2%。
- 退出 OBS 后日志确认滤镜实例销毁和模块卸载；场景集合保存了 `obs_beauty_filter_v2`、预设和 `MediaPipe` 运行状态。
- 重启 OBS 后，保存的测试源与滤镜均恢复，日志再次确认模块加载与 MediaPipe 滤镜实例创建。
- 重新构建并运行 CTest：8/8 通过，覆盖模块注册、中文属性、设置迁移、关键点转换、真实模型空帧推理和推理工作器。
- 对构建 bundle 与安装副本均执行了深度签名结构校验；当前为本地 ad-hoc 签名，不是发布签名。

## 测试运行库修复

MediaPipe 推理测试此前依赖开发机临时 OpenCV 目录。CTest 现在显式使用构建 bundle 内的 `Frameworks` 目录，因此临时第三方构建目录被清理后，测试仍加载与待安装插件相同的 OpenCV 运行库。

## 仍未完成的 macOS 发布验收

- 接入真实人脸视频后，对单人、多人、转头、遮挡、逆光和弱光的画面效果与边缘质量进行人工验收。
- 720p30、1080p30、1080p60 的量化性能记录，以及至少两小时稳定性运行。
- Developer ID 签名、公证、全新环境安装和 Gatekeeper 验证。

本记录不把颜色源验证等同于真实人脸画面验收；它仅证明真实 OBS 中的模块加载、滤镜生命周期、MediaPipe 初始化、配置保存和重启恢复链路已经打通。
