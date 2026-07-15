# 用户指南

## 安装

macOS Apple Silicon：解压发布包，将 `obs-beauty-filter.plugin` 复制到 `~/Library/Application Support/obs-studio/plugins/`，重启 OBS。在摄像头源的“滤镜”中添加“高级美颜”。

Windows x64：解压发布包到 OBS 安装目录或用户插件目录，保持 `obs-beauty-filter/bin/64bit` 与 `obs-beauty-filter/data` 的目录结构不变，重启 OBS。

## 卸载

先关闭 OBS，再删除对应的 `obs-beauty-filter.plugin`（macOS）或 `obs-beauty-filter` 插件目录（Windows）。删除滤镜不会修改摄像头原始画面；如需清除保存的滤镜设置，可在 OBS 中先移除该滤镜。

## 故障排查

- 未看到“高级美颜”：确认 OBS 为 31+、架构受支持，并检查 OBS 日志中 `obs-beauty-filter` 的加载记录。
- 显示“模型不可用”：重新安装完整发布包；模型和运行库必须与插件一同存在。此时会自动回退为兼容 Shader。
- 画面卡顿：将质量模式改为“兼容模式”，或在“自动”模式查看性能状态；减少同场景摄像头源、输出分辨率或编码负载。
- 效果区域不对：暂时打开“显示人脸区域（诊断）”核对；诊断结束后关闭。

## 性能建议

兼容模式适合 720p30 或 CPU 负载较高的场景；自动模式会根据推理耗时降级；高质量模式适合有余量的 1080p 场景。推理全在本机完成，不上传摄像头画面。
