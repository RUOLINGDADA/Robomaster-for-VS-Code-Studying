# 项目技能

从 `C:/Users/zhoujinyuan/Desktop/skills/skills/` 安装全部 37 项包含 `SKILL.md` 的技能；每项技能的脚本、参考文档和 `agents/openai.yaml` 一并保留。安装为独立副本，之后更新源目录不会自动更新已安装副本。

## 安装范围

- 用户级：`C:/Users/zhoujinyuan/.agents/skills/`，用于该用户的各个项目。
- 当前工作区：`.agents/skills/`，可从本工作区及下层的 `yuntai/` 工程发现。
- 来源和安装清单：`matt-pocock-skills-install.json`。
- 原始许可证：`matt-pocock-skills.LICENSE`。

Codex 按任务描述匹配允许自动调用的技能，也可通过 `$技能名` 显式选择，例如 `$code-review`、`$diagnosing-bugs`、`$teach`。要求手动调用的技能仍须显式选择。同名技能同时存在于用户级和项目级时，技能选择器可能显示两个入口。

## 技能分类

- 工程（20 项）：ask-matt、code-review、codebase-design、diagnosing-bugs、domain-modeling、grill-with-docs、implement、implement-spec、improve-codebase-architecture、pr、prototype、research、retro、setup-matt-pocock-skills、tdd、to-spec、to-tickets、triage、wayfinder、wizard。
- 学习与生产力（7 项）：grill-me、grilling、handoff、teach、to-questionnaire、wait-what、writing-for-agents。
- 开发中的技能（6 项）：claude-handoff、loop-me、setup-ts-deep-modules、writing-beats、writing-fragments、writing-shape。
- 其它工具（4 项）：git-guardrails-claude-code、migrate-to-shoehorn、scaffold-exercises、setup-pre-commit。

部分技能用于 Claude Code 或 TypeScript 场景，是否使用取决于具体任务；安装时没有执行它们的脚本或改变工程工具链。

## 需要项目配置的工作流

`to-spec`、`to-tickets`、`triage` 等依赖项目的 Issue 跟踪与文档配置。需要这些工作流时，显式调用 `$setup-matt-pocock-skills`，由其引导选择跟踪位置、标签和文档布局。此次安装没有预设这些选择。

官方技能发现说明：https://developers.openai.com/codex/skills/
