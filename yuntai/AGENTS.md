# yuntai 工作区与技能入口

本文件适用于当前工作区及其子目录。实际 STM32 工程位于 `yuntai/`；修改工程前必须阅读 `yuntai/AGENTS.md` 和 `yuntai/doc/README.md`，并遵守其中的模块、硬件、注释和验证规则。

## 本地技能

已将 `C:/Users/zhoujinyuan/Desktop/skills` 中的 37 项技能安装到本工作区的 `.agents/skills/`，并同时安装到用户目录 `C:/Users/zhoujinyuan/.agents/skills/`。完整来源和安装目录见 `.agents/matt-pocock-skills-install.json`；使用说明见 `.agents/README.md`。

- 用户指定技能，或任务符合允许自动调用的技能描述时，先读取对应的 `SKILL.md`，再按需读取其附属文件。
- 保留技能的调用限制：标记 `disable-model-invocation: true` 或要求显式调用的技能，由用户明确调用后使用。安装技能不等于执行其工作流。
- 常用入口：代码审查使用 `code-review`，复杂故障诊断使用 `diagnosing-bugs`，架构设计使用 `codebase-design`，术语建模使用 `domain-modeling`，学习讲解可显式调用 `$teach`。
- 技能使用必须结合本项目的 STM32/HAL/FreeRTOS 环境；生成区修改、测试代码保存和硬件验证遵循工程规则。
- 技能安装不自动创建外部 Issue、PR、Git hook 或新增工具依赖；这些动作随具体任务的授权执行。
