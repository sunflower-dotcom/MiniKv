# OpenCode 学习计划

> 目标：系统掌握 OpenCode 终端 AI 编码代理，以 MiniKV 项目为实战载体。

---

## 一、OpenCode 简介

OpenCode 是 SST/Anomaly 团队开发的开源 AI 编程代理（160K+ GitHub Stars，750 万月活开发者）。以终端 TUI 为主要交互界面，同时支持桌面应用和 IDE 扩展。

**核心价值**：在终端里用自然语言驱动编码，支持 Plan（规划）和 Build（执行）双模式。

---

## 二、第一阶段：安装与配置（~30 分钟）

### Windows 安装

```powershell
# 推荐：Scoop（更新方便）
scoop install opencode

# 备选：npm（已装有 Node.js v24）
npm install -g opencode-ai

# 备选：Chocolatey
choco install opencode
```

### 配置 LLM Provider

首次运行 `opencode` 后，在 TUI 中输入 `/connect`，选择 provider：

| 方式 | 说明 |
|------|------|
| **OpenCode Zen** | 官方精选模型，去 opencode.ai/auth 获取 API key |
| **自带 API key** | Anthropic / OpenAI / Gemini 等 75+ providers |
| **GitHub Copilot** | 有 Copilot 订阅可直接用 |

---

## 三、第二阶段：核心概念（2~3 天，边用边学）

| 优先级 | 概念 | 操作 | 说明 |
|--------|------|------|------|
| ⭐⭐⭐ | **Plan / Build 模式** | `Tab` 切换 | Plan 只出方案不改代码；Build 直接动手 |
| ⭐⭐⭐ | **@ 文件引用** | `@src/foo.ts` | 把文件内容注入上下文 |
| ⭐⭐⭐ | **/init** | 项目根执行 | 自动分析项目，生成 AGENTS.md |
| ⭐⭐ | **/undo / /redo** | 撤销/重做 | 改错了随时回滚 |
| ⭐⭐ | **/share** | 分享会话链接 | 把对话发给同事 review |
| ⭐⭐ | **自定义命令** | `.opencode/commands/*.md` | 高频操作模板化 |
| ⭐ | **Subagents** | 并行子任务 | 多个 agent 同时干不同模块 |
| ⭐ | **图片拖入** | 拖截图到终端 | 用设计稿/截图补充需求描述 |

---

## 四、第三阶段：实战练习（4~6 天）

练习项目：**MiniKV** — LSM-Tree 嵌入式 KV 存储引擎（`d:\Code\minikv`）

### 练习 1：让 OpenCode 解释代码

```
这段代码的认证逻辑是怎么实现的？@src/auth/middleware.ts
```

用 `@` 引用文件提问，建立「把 AI 当结对同事」的习惯。

### 练习 2：Plan → 迭代 → Build

1. `Tab` 切 Plan 模式
2. 描述需求
3. 审核方案，补充细节或纠正
4. `Tab` 切 Build 模式执行

### 练习 3：创建自定义命令

在 `.opencode/commands/` 下创建 markdown 文件，一键执行测试、格式化等操作。

### 练习 4：回滚与重做

- 故意让 OpenCode 做一个不满意的改动
- `/undo` 回滚 → 改 prompt → 重来
- 误回滚时 `/redo` 恢复

---

## 五、第四阶段：进阶配置（第 2 周起）

### 项目级配置 `opencode.json`

```json
{
  "$schema": "https://opencode.ai/config.json",
  "model": "anthropic/claude-sonnet-4-5",
  "rules": {
    "project": [
      "使用 C++20 标准",
      "遵循 RAII 原则",
      "所有公共接口需文档注释"
    ]
  }
}
```

### 全局配置 `~/.config/opencode/opencode.json`

设置默认 model、auto-update、主题等偏好。

### 打磨 AGENTS.md

`/init` 自动生成初稿后，手动补充：
- 代码风格约定
- 命名规范
- 架构决策记录
- 禁止的做法

---

## 六、MiniKV 实战路线

项目位置：`d:\Code\minikv`

| 阶段 | 模块 | 技术点 | 练习的 OpenCode 技能 |
|------|------|--------|---------------------|
| 1 | 项目骨架 | CMake, Catch2, 接口设计 | `/init`, @ 引用 |
| 2 | MemTable 跳表 | Skip List, 线程安全 | Plan→Build, `/undo` |
| 3 | WAL 预写日志 | 顺序写, Checksum, 崩溃恢复 | 自定义 `/test` 命令 |
| 4 | SSTable 磁盘文件 | 文件格式, 二分查找, mmap | 多文件协调 |
| 5 | Compaction 归并 | 多路归并, 后台线程 | Subagents |
| 6 | Bloom Filter | 位数组, 哈希函数 | 集成第三方库 |
| 7 | Benchmark 性能 | 对比 std::map / SQLite | 完整 Plan→Build 闭环 |

---

## 七、关键心态

1. **当 junior developer 带** — 描述越具体，产出越好
2. **Plan 模式先问再动** — 改代码前先让它说方案
3. **小步快跑** — 一次一个功能，别一口气塞太多需求
4. **AGENTS.md 是项目宪章** — 花时间打磨，ROI 很高
5. **大胆试，不满意就 `/undo`** — 零成本回滚

---

## 八、参考资源

| 资源 | 地址 |
|------|------|
| 官方文档 | https://opencode.ai/docs |
| 配置参考 | https://opencode.ai/docs/config |
| 命令参考 | https://opencode.ai/docs/commands |
| 示例会话 | https://opencode.ai/s/4XP1fce5 |
| GitHub | https://github.com/sst/opencode |

---

## 九、环境信息

- OS: Windows 11 (x64)
- 编译器: GCC 13.2 (Strawberry Perl)
- 构建: CMake 3.29 + Ninja
- 测试: Catch2 v3.7.1
- 标准: C++20

---

> 最后更新：2026-06-03
