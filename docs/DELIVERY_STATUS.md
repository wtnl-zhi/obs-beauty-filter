# 交付状态审计

更新时间：2026-07-15。此表只记录已有证据；“已准备”不等同于已在目标机器完成验收。

| 交付要求 | 状态 | 当前证据 / 下一步 |
| --- | --- | --- |
| Apple Silicon P1 原生构建与模型推理 | 已验证 | `ctest` 8/8：关键点、预设、采样、设置迁移、模块加载、真实模型和工作器。 |
| macOS 安装后的 bundle 可加载 | 已验证（自动） | `scripts/verify-macos-p1-install.zsh` 将 P1 bundle 安装到新前缀，验证资源、运行时、`libobs` 加载与 ad-hoc 签名。 |
| macOS OBS 31+ 添加滤镜、保存、重启恢复 | 待实机 | 需要在独立 OBS 场景中按 [场景验收清单](OBS_SCENE_VALIDATION.md) 执行并填写记录。 |
| macOS 单人/多人/遮挡/光照/场景切换 | 待实机 | 需保存匿名日志与 [验收记录](ACCEPTANCE_EVIDENCE_TEMPLATE.md)。 |
| macOS 720p30、1080p30、1080p60 与 2 小时稳定性 | 待实机 | 需在目标输入和 OBS 统计窗口记录性能，不由单元测试推断。 |
| macOS Developer ID 签名、公证、干净环境复测 | 待外部凭据与环境 | 打包脚本已强制 Developer ID；本机仅有 Apple Development 证书。 |
| Windows x64 构建/模块运行时检查 | 已准备，未验证 | VS2022 构建、模型获取、打包与 `obs-module-smoke` CTest 入口已提供；尚未在 Windows 主机执行。 |
| Windows 安装、OBS 场景、性能与恢复 | 待 Windows 实机 | 需使用 Windows 10 22H2+、OBS 31+ 完成与 macOS 对等记录。 |
| 模型缺失与设置迁移回归 | 已验证（自动） | 缺失模型会返回错误；P0/v1/v2 设置迁移已单测。实际 OBS 画面回退仍待实机。 |
| 支持矩阵与限制 | 已记录 | 见 [COMPATIBILITY.md](COMPATIBILITY.md)，未测硬件指标保持“待实测”。 |
| GPL、第三方许可证和模型校验 | 已准备 | 项目 GPL-2.0-or-later；发布打包强制带入许可证文本；模型下载脚本校验 SHA-256。发布前仍须复核上游模型条款。 |
| README、构建、安装、卸载与排障 | 已提供 | 见 [README](../README.md) 与 [USER_GUIDE.md](USER_GUIDE.md)。 |
| 版本发布包、校验值、GitHub Release | 待发布门禁 | 必须在双端实机、macOS 公证和许可证终审之后创建。 |
| 从零克隆构建 | 部分验证 | Apple Silicon P0 新浅克隆 4/4；P1 已在新浅克隆用新下载模型和开发机运行时 6/6。独立干净机全 P1 重建待完成。 |

## 发布判定

当前状态：**不可发布**。在所有“待实机”“待外部凭据与环境”与“待发布门禁”项目转为已验证前，不创建 GitHub Release，不生成对用户承诺可用的正式安装包。
