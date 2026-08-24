# ClassScoreManager 项目记忆

> 本文件是当前项目**唯一权威**的记忆文件，由早期分散的记忆条目合并整理而来。
> 其他位置不再维护，也不要在多处重复存放记忆。

## 1. 用户画像

- 用户是初中生，有 C++ 和 Qt 基础，但倾向于由 AI 助手执行具体编码工作。
- 用户负责：运行程序、查看 UI、发现问题并反馈（视觉验证）；AI 助手负责：代码修改。
- 沟通偏好：表达清晰、结构化、简洁直接。
- 用户基本使用语音输入发送信息；遇到识别错误时先尝试推测原意，无法确认或关键信息缺失时停下请用户澄清，不得自行猜测。

## 2. 协作规则（必须遵守）

### 2.1 沟通与决策

1. 始终以简体中文输出计划和回复；表达清晰、结构化、简洁直接。
2. 不要假设用户清楚自己想要什么；动机或目标不清晰时，停下来与用户讨论，不要猜测后直接执行。
3. 用户指令目标清晰但方案不是最佳时，直接指出并建议更好的办法。
4. 大规模代码改动前必须先给 plan（说明要改什么、影响范围、实施顺序），待用户确认后再动手；用户明确指定的小修复可直接执行。

### 2.2 构建与提交

1. 不要运行 qmake 或构建项目，除非用户明确要求；构建由用户在自己环境中完成。
2. 不执行 git commit。每完成一整部分内容、且用户确认测试无误后，给出建议的提交文本，用 markdown 代码块包裹便于用户复制；由用户修改并自行执行提交。
3. 编写代码时：修改完完整代码后，简单通读一遍，确认没问题就停止；不需要构建运行测试，也不要手动或写脚本做静态检查，以免浪费时间和上下文。构建部署时：完成必要步骤后只做最小范围检查（如目标文件是否存在），非必要不运行产物测试。

### 2.3 设计与代码

1. 遵循 KISS 原则，拒绝过度设计。
2. 代码是代码，文档是文档：代码注释只标关键技术实现（特殊外部 API/方法、与逻辑强相关的算法），其余靠清晰命名自解释。
3. 遇到问题追根因，不打补丁；每个决策都要能回答“为什么”。
4. 不要过度考虑边界情况；当前项目中不可能出现的边界情况不需要处理。
5. 代码行宽最多容忍到约 140 个字符；声明、调用、短 lambda 不要因为 60 几个字符就折行，过早换行会影响屏幕观感。

### 2.4 文件与记忆

1. 不主动创建无关文档文件（*.md）；用户要求或为协作理解架构/实现而需要时除外。
2. 项目记忆只维护本文件，不再使用其他分散的记忆文件或位置，也不要在项目内另建重复说明文件。

### 2.5 工具与环境

1. 如认为某个目录为空，**必须**再用 `dir` 命令验证一次。
2. 如需调用 PowerShell 命令（如 `Select-String`、`Get-Content` 等），必须显式使用 PowerShell 7，即 `pwsh -NoProfile -ExecutionPolicy Bypass -Command "[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; ..."` 调用并指定 UTF-8 编码；不要再使用 PowerShell 5.1（`powershell.exe`）。
3. 如需临时写代码验证 Qt 行为，把验证程序放进 `C:\Users\dell\Desktop\C++\projects\widget_demo`，由用户选择 Qt 5.15.2 或 6.8.3 编译；助手没有视觉能力，交付验证程序时必须明确写出需要用户观察什么现象。
4. 不要做无意义的静态验证（括号配对、残留字符串搜索、`git diff --check` 等）；编译和用户测试足以发现全部问题，把算力和上下文留给实际修改。
5. 用 Python 处理 Windows 路径时，普通字符串里的 `\` 会被当成转义符并吃掉字符；必须使用 raw string（`r'C:\path'`）、正斜杠或双反斜杠。写临时 Python 脚本时优先用单引号 heredoc（`<<'PY'`）。

### 2.6 交付物与变更表达

1. 每次修改完成后，只产出面向当前最终状态的独立可交付物；注释、文档、提交信息等只描述“现在是什么”，不写“由 A 改为 B”“V2（已移除 X）”这类变更过程描述。
2. 删除某个功能或内容后，不要把它沉淀成“原则上不应包含 X”的新规则或文档章节（例如移除游泳池后新增“卫生间原则上不应包含游泳池”）；文档只保留当前有效的内容，避免充斥已经不存在的“现状”。

### 2.7 样式库更新流程

1. 触发条件：用户要求更新项目使用的 FluentUIStyle 样式库时，按本节流程执行；目标是更新 Qt 5.15.2 的样式插件，不构建 ClassScoreManager。
2. Git 同步：先在当前 shell 设置代理 `export http_proxy=http://127.0.0.1:10808 https_proxy=http://127.0.0.1:10808`；确认 `D:\projects\FluentUIStyle` 工作区干净后 `git fetch origin --prune`，再 `git reset --hard origin/master` 让 `master` 对齐上游；旧 QMake 状态保留在 `backup/qmake-support` 分支。工作区不干净时停下询问，不要直接 reset。
3. CMake 构建：用 VS2022 Developer PowerShell（`pwsh` 中执行 `Launch-VsDevShell.ps1 -Arch amd64 -HostArch amd64 -SkipAutomaticLocation`）后执行：
```
cmake -S . -B build-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:\Qt\5.15.2\msvc2019_64" -DBUILD_LIBRARY=OFF -DBUILD_PLUGIN=ON -DBUILD_EXAMPLES=OFF -DEXWIDGETS_BUILD_FRAMELESS=OFF -DFLUENTUI3STYLE_COPY_TO_QT_DIR=ON
cmake --build build-cmake --config Release
```
4. 校验：构建后插件自动复制到 `C:\Qt\5.15.2\msvc2019_64\plugins\styles\FluentUI3StylePlugin.dll`；确认其与 `D:\projects\FluentUIStyle\build-cmake\plugins\styles\FluentUI3StylePlugin.dll` 均存在且哈希一致即可。
5. 收尾：不执行 git commit；用户编译验证通过后，如要求同步 fork，再执行 `git push --force-with-lease myfork master`。

## 3. 项目上下文

- 项目：ClassScoreManager，班级积分管理系统，面向班级日常积分管理。
- 技术栈：C++17 + Qt Widgets（纯 Widgets，禁用 QML）。
- 当前构建套件：Qt 5.15.2 / MSVC 2019 64bit（以 `ClassScoreManager.pro.user` 为准）。
- 代码同时保留 Qt5/Qt6 条件编译兼容（`QT_VERSION` / `QT_VERSION_CHECK`）。
- 部署方式：打包后拷入 U 盘，免安装直接运行；保持体积小、启动快。
- 约束：核心功能阶段不引入大型美化/主题库；Fluent 风格美化作为后续独立阶段统一实施。
- 界面基线：目前所有控件使用 Qt 默认皮肤；美化集中进行，不做分散样式改动。
- 美化约束：QSS 与后续 QProxyStyle 美化库互斥（启用 QProxyStyle 后 QSS 失效）；视觉指示一律使用 QStyle / QStyledItemDelegate / QPalette / 模型角色，不引入 QSS。
- 当前阶段范围：仅把 MainWindow 中调用底层处迁移为私有操作函数；QListView 选中行跟随预览的高亮暂缓，相关选中同步逻辑删除。
- 主要模块：`src/core`（数据管理、报表）、`src/models`（表格模型）、`src/ui`（界面）、`src/delegates`（委托）。
- 命名约定：业务命名不使用 `Score` 前缀；积分记录、积分模板分别用 `Record`、`Template` 命名，与 C++ 关键字冲突时视情况加修饰或复数。
- 应用图标：`resources/icon.ico`（`RC_ICONS`）。
- 技术文档定位：服务于协作开发，帮助快速了解项目架构与模块实现，避免每次通读代码；代码与文档分离，代码注释只保留关键技术标注。
- 第三方 Fluent 风格库：FluentUIStyle，源仓库 https://github.com/XHY-ChuJian/FluentUIStyle，本地目录 `D:\projects\FluentUIStyle`；使用 CMake 构建，插件构建后自动复制到当前 Qt 的 `plugins/styles`。
- 待办：当前美化阶段完成后配置 clang-tidy 作为代码格式化工具；astyle 对复杂 lambda 表达式处理不佳，暂不作为格式化标准。

## 4. 参考资料（按需查阅，不要整体读入上下文）

- `references/qss/qt5_qss_reference.md` — Qt 5.15 QSS 属性/伪状态/子控件速查表。
- `references/qss/qt6_qss_reference.md` — Qt 6.11 QSS 属性/伪状态/子控件速查表。

## 5. 提交信息规范（C++/Qt 项目版）

- 写 commit message 前先评估改动量：
  - 单一独立改动点 → 单行写完；
  - 2 个以上独立改动点 → 首行简介 + 空行 + 分点。
- 语言与粒度：
  - 只写“做了什么”，不写原因，不写技术细节。
  - 尽可能简短；尽量不暴露内部类名、方法名，只表现改动在宏观上的效果。
- 长格式：
  - 首行：`type: 简短概括主要内容`，只列主要变更；多个大主题用顿号分隔（如 `feat: 新增成绩导出、支持学生批量导入、优化表格交互`）。
  - 小改动不放首行，放底下分点。
  - 空一行后按“功能/模块：中文说明”分点列举，说明做了什么。
- 短格式：单一改动单行写完。
- 内部接口原则（个人工具项目，不向代码库外暴露 API）：
  - 修改内部模型、函数签名和调用约定时可放心引入 breaking changes，不需要为假设中的外部调用者维持兼容。
  - Why: 为不存在的外部兼容需求保留旧参数位置等约束，会牺牲代码结构和可读性。
  - How to apply: 优先语义分组、一致性和简洁性；同步修改仓库内调用点即可；不要添加兼容层，也不要把新参数机械追加到末尾。
