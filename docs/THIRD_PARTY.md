# 第三方组件与模型审查台账

本文件在引入可执行依赖、模型文件或模型权重前更新。所有项目均需保留其许可证和必要的 NOTICE 文件。

| 组件 | 用途 | 暂定版本/来源 | 许可证状态 | 接入状态 |
| --- | --- | --- | --- | --- |
| OBS Studio SDK / libobs | 插件 API 与渲染接口 | OBS Studio 31+ | GPL-2.0-or-later | P0 使用 API；不打包 libobs。 |
| MediaPipe | 人脸关键点推理框架 | `google-ai-edge/mediapipe` | Apache-2.0；待锁定具体发布版本和 NOTICE | P1 候选，未引入。 |
| MediaPipe Face Landmarker / Face Mesh V2 | 人脸轮廓、眼睛、嘴唇保护区域 | `face_landmarker.task`，float16/1，SHA-256 `64184e…bc9ff` | Face Mesh V2 模型卡标注 Apache-2.0；具体来源和校验记录在 `models/face_landmarker.json` | P1 暂定模型；二进制不提交，按校验脚本获取。 |
| ONNX Runtime / Core ML | 备用推理后端评估 | 待定 | 待定 | 不在 P1 首个集成范围。 |

## P1 暂定技术决定

P1 先采用 **MediaPipe Face Landmarker**，而非直接引入人脸解析（face parsing）模型：

- 它可在实时视频中异步返回人脸关键点，适合生成脸部轮廓 mask，并可保护眼睛、眉毛、嘴唇等区域。
- MediaPipe 是跨平台项目，代码为 Apache-2.0；官方 Face Mesh V2 模型卡也标注 Apache-2.0。
- 首次集成前必须记录模型下载 URL、SHA-256、模型版本、许可证文本与 NOTICE；若模型包的条款与此台账不符，则停止打包并重新选型。

该方案的 mask 是“脸部区域 + 五官保护区”，不等同于像素级皮肤解析。祛痘、口红和精细妆容需要后续另行选择并审查 face parsing 模型。

## 可复现获取

运行 `scripts/fetch-models.zsh` 下载模型。脚本会校验 SHA-256；模型二进制由 `.gitignore` 排除，避免未经发布流程的模型资产进入源码提交。
