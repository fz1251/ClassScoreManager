# ClassScoreManager 技术文档

> 本目录保存协作开发需要的技术知识，按需查阅，不整体读入上下文。
> 项目记忆、协作规则与纯代码 UI 编写规范唯一保存在根目录 `CLAUDE.md`；本目录是它的知识补充，不与它重复。

## 文档索引

| 文档 | 内容 |
| --- | --- |
| [structure.md](structure.md) | 代码结构、模块关系图与数据流向图 |
| [project-context.md](project-context.md) | 项目技术栈、Qt 套件、CMake 构建、样式、部署、模块结构 |
| [coding-conventions.md](coding-conventions.md) | 编码约定：整数类型选择（定长类型、int 与 qsizetype 的边界） |
| [group-board.md](group-board.md) | 小组管理页：组色取点、落点判定、重排弹簧与拖动周边手感 |
| [third-party.md](third-party.md) | 第三方美化库 FluentUIStyle 及其携带依赖（QWindowKit、kissfft、qmsetup）的配置与使用 |
| [qt-patches.md](qt-patches.md) | 自编译 Qt 源码树的项目改动：拖动指示线的覆盖范围与绘制、增量重建方式 |
| [font-rendering.md](font-rendering.md) | Windows 字体渲染：光栅器选择、hinting 与字号的实测关系、拖动缩略图 DPR、字体族坑位 |
| [todo.md](todo.md) | 待办事项清单 |

## 维护约定

- 每份文档只描述当前状态，不记录“由 A 改为 B”这类变更过程。
- 文档内容以当前仓库代码为准；代码变化后同步更新对应文档。
- 临时计划、对话摘要等不放入本目录。
