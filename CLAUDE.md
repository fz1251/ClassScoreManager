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

1. 需要语法/编译验证时，直接构建完整项目进行测试；不要为了验证而单独编译中间文件或子目标。不需要验证时，把构建交给用户，用户点击运行时 IDE 会顺带构建。
2. 不执行 git commit。每完成一整部分内容、且用户确认测试无误后，给出建议的提交文本，用 markdown 代码块包裹便于用户复制；由用户修改并自行执行提交。
3. 编写代码时：修改完完整代码后，简单通读一遍，确认没问题就停止；确需验证时按第 1 条直接构建完整项目，不做中间编译或脚本静态检查，以免浪费时间和上下文。构建部署时：完成必要步骤后只做最小范围检查（如目标文件是否存在），非必要不运行产物测试。

### 2.3 设计与代码

1. 遵循 KISS 原则，拒绝过度设计。
2. 代码是代码，文档是文档：代码注释只标关键技术实现（特殊外部 API/方法、与逻辑强相关的算法），其余靠清晰命名自解释。
3. 遇到问题追根因，不打补丁；每个决策都要能回答“为什么”。
4. 不要过度考虑边界情况；当前项目中不可能出现的边界情况不需要处理。
5. 代码行宽最多容忍到约 140 个字符；声明、调用、短 lambda 不要因为 60 几个字符就折行，过早换行会影响屏幕观感。
6. 纯代码 UI 的控件指针：只在构造函数之外还需要访问的控件才声明为类成员，否则用构造函数局部变量；成员指针集中放一个区块，区块上下各空一行，顶部用一行统一注释说明该区块，指针本身不再逐项注释；构造函数中每个控件创建块单独注释。

### 2.4 文件与记忆

1. 不主动创建无关文档文件（*.md）；用户要求或为协作理解架构/实现而需要时除外。
2. 项目记忆（规则与上下文）只维护本文件；详细技术知识统一放入 `docs/`（主索引 `docs/README.md`），不要在其他位置另建重复说明文件。

### 2.5 工具与环境

1. 如认为某个目录为空，**必须**再用 `dir` 命令验证一次。
2. 如需调用 PowerShell 命令（如 `Select-String`、`Get-Content` 等），必须显式使用 PowerShell 7，即 `pwsh -NoProfile -ExecutionPolicy Bypass -Command "[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; ..."` 调用并指定 UTF-8 编码；不要再使用 PowerShell 5.1（`powershell.exe`）。
3. 如需临时写代码验证 Qt 行为，把验证程序放进 `C:\Users\dell\Desktop\C++\projects\widget_demo`，由用户选择 Qt 5.15.19（项目当前套件）、5.15.2 或 6.8.3 编译；助手没有视觉能力，交付验证程序时必须明确写出需要用户观察什么现象。
4. 不要做无意义的静态验证（括号配对、残留字符串搜索、`git diff --check` 等）；编译和用户测试足以发现全部问题，把算力和上下文留给实际修改。
5. 用 Python 处理 Windows 路径时，普通字符串里的 `\` 会被当成转义符并吃掉字符；必须使用 raw string（`r'C:\path'`）、正斜杠或双反斜杠。写临时 Python 脚本时优先用单引号 heredoc（`<<'PY'`）。

### 2.6 交付物与变更表达

1. 每次修改完成后，只产出面向当前最终状态的独立可交付物；注释、文档、提交信息等只描述“现在是什么”，不写“由 A 改为 B”“V2（已移除 X）”这类变更过程描述。
2. 删除某个功能或内容后，不要把它沉淀成“原则上不应包含 X”的新规则或文档章节（例如移除游泳池后新增“卫生间原则上不应包含游泳池”）；文档只保留当前有效的内容，避免充斥已经不存在的“现状”。

### 2.7 FluentUIStyle 子模块更新流程

1. 触发条件：用户要求更新 FluentUIStyle 时执行；它现在是 `third_party/FluentUIStyle` 的 git 子模块，随主项目一起构建，不再单独构建或部署 Qt 样式插件。
2. 更新前：如需访问 GitHub 先设置代理 `export http_proxy=http://127.0.0.1:10808 https_proxy=http://127.0.0.1:10808`；用 `git submodule status` 和 `git -C third_party/FluentUIStyle status` 确认子模块工作区干净，并和用户确认目标分支/提交。工作区不干净时停下询问，不要直接 reset。
3. 更新：`git -C third_party/FluentUIStyle fetch origin --prune`，然后 checkout/merge 到目标提交；同步检查 FluentUIStyle 及其 QWindowKit 的构建选项、目标名、接口是否有变化，必要时调整主项目 `CMakeLists.txt`。
4. 验证：按 2.2 构建完整项目；构建通过后由用户运行程序做视觉验证。
5. 收尾：不执行 git commit；用户验证通过后给出建议提交文本（子模块指针更新 + 必要适配说明）。

## 3. 项目上下文

- 项目：ClassScoreManager，班级积分管理系统，面向班级日常积分管理。
- 技术栈：C++17 + Qt Widgets（纯 Widgets，禁用 QML）；构建用 CMake + Ninja，开发环境为 VS Code（CMake Tools + clangd）；代码同时保留 Qt5/Qt6 条件编译兼容。
- 当前 Qt 套件：自编译 Qt 5.15.19（qtbase + qtsvg），MSVC 2022 x64，debug-and-release，prefix `D:/Projects/Qt5.15.19/msvc2022_64`。
  - 重建脚本：`D:\Projects\Qt5.15.19\build_qt.ps1`（qtbase）、`D:\Projects\Qt5.15.19\build_qtsvg.ps1`（qtsvg）。必须在 x64 VS2022 环境中运行；脚本会先验证 cl 为 x64 并清空旧构建再 configure/build/install。
  - 该 Qt 曾整库被误建成 i386（`qsizetype` 4 字节）导致 `.cls` 文件兼容问题；凡涉及 Qt 重建或新装，必须确认编译器和产物都是 x64，不能只依赖目录名。
  - 旧 Qt 5.15.2 暂保留，等新套件稳定后再清理。
- CMake 构建：
  - 预设：`qt5-msvc2022-debug` → `build/vscode-qt5.15.19-debug`；`qt5-msvc2022-release` → `build/vscode-qt5.15.19-release`。两个预设的 `CMAKE_PREFIX_PATH` 均指向上述 Qt，并开启 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`。
  - 两个预设继承隐藏预设 `msvc2022-x64-base`：固定 `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` 为 `cl`，并在 PATH/INCLUDE/LIB 中内置 VS2022 x64 工具链与 Windows SDK 路径；普通终端和 VS Code 构建都无需先进入 VS 开发环境，也不能让 CMake 自行选中 Strawberry Perl 的 GCC。
  - VS 或 Windows SDK 版本升级后，需同步更新 `msvc2022-x64-base` 与根目录 `.clangd` 里写死的 MSVC / Windows Kits 版本路径。
  - 产物统一输出到对应构建目录的 `bin/`（exe 与所有 DLL）和 `lib/`。
  - 当前状态：Debug 与 Release 均已用上述预设构建成功；clangd 使用 Qt Creator 自带版本（VS Code 中由 `clangd.path` 指定），读取 debug 的 `compile_commands.json`，系统头目录由根目录 `.clangd` 补充。
  - 排查提示：若构建错误显示编译器是 `C:/dev/runtimes/StrawberryPerl/c/bin/c++.exe` / `gcc.exe`，说明该 build 目录的 CMakeCache 被错误环境污染；删除对应 build 目录后用预设重新配置即可。
  - 构建信息：每次构建都会运行 `cmake/generate_build_info.cmake`，生成 `${CMAKE_BINARY_DIR}/generated/app_build_info.h`（仅在日期变化时重写，避免无谓重编译）；界面通过其中的 `APP_BUILD_DATE` 宏显示构建日期。
- `CMakeLists.txt` 关键点：
  - 版本无关 `find_package(QT NAMES Qt6 Qt5 ...)`；当前实际走 Qt 5.15.19。
  - 开启 AUTOMOC/AUTOUIC/AUTORCC；`.ui` 中 customwidget 头写作 `.\src\ui\*.h`，因此必须 `target_include_directories` 项目根目录，否则 AUTOUIC 找不到头。
  - rc.exe 的字符串中 `\` 是转义符，图标路径必须把反斜杠翻倍后再写入 RC 文件；MSVC 加 `/utf-8`。
  - FluentUIStyle 以子目录构建：`BUILD_LIBRARY ON`、`BUILD_PLUGIN OFF`、`BUILD_EXAMPLES OFF`、`EXWIDGETS_BUILD_FRAMELESS ON`；其依赖 Qt5Svg。
  - QWindowKit 必须预先 `add_subdirectory` 到浅目录 `${CMAKE_BINARY_DIR}/qwindowkit`，避免深路径生成的私有 forwarding header 相对路径过长；`QWINDOWKIT_INSTALL ON` 才能通过 CMake export 校验。
  - 主程序链接：Qt Core/Gui/Widgets、FluentUI3Style、ExWidgets::ExWidgets、ExWidgets::Frameless、dwmapi。
- 样式：
  - `src/main.cpp` 直接 `a.setStyle(new FluentUI3Style)`；样式库随项目构建，不再使用或部署 Qt 样式插件。
  - 需要 Fluent 属性头时写作 `#include "fluentui3styleproperties.h"`（不带 `FluentUI3Style/` 前缀）。
  - 全局视觉统一走 FluentUI3Style；AboutInfoDialog 是整体独立定制的对话框，其局部 QSS 属该对话框自身的完整定制，与主界面样式不冲突。
  - AboutInfoDialog 目前保留手写 DWM/无边框标题栏逻辑；已链接 ExWidgets::Frameless 但尚未切换，迁移到 `FluentTitleBar`/`FluentWindowFrame` 是单独待办。
- 第三方库：
  - FluentUIStyle：git 子模块 `third_party/FluentUIStyle`，上游 https://github.com/XHY-ChuJian/FluentUIStyle.git，当前固定在 `8d48defc9b8cb8b21cb86eca355cb4c47cb8e714`；QWindowKit 随其在 `3rd/qwindowkit` 提供。
- 部署方式：打包后拷入 U 盘，免安装直接运行；保持体积小、启动快。运行时依赖 Qt bin（Qt DLL）与构建输出 `bin/`（FluentUI3Style、ExWidgets、QWK 等 DLL），打包时按需收集。
- 主要模块：`src/core`（数据管理、报表）、`src/models`（表格模型）、`src/widgets`（自绘控件）、`src/pages`（页面）、`src/windows`（主窗口与对话框）、`src/delegates`（委托）。
- 命名约定：业务命名不使用 `Score` 前缀；积分记录、积分模板分别用 `Record`、`Template` 命名，与 C++ 关键字冲突时视情况加修饰或复数。
- 应用图标：`resources/icon.ico`，通过 CMake 生成的 RC 文件设置。
- 技术文档定位：详细技术知识（第三方库配置、项目结构等）放在 `docs/`，主索引 `docs/README.md`；CLAUDE.md 只保存项目记忆与协作规则。代码与文档分离，代码注释只保留关键技术标注。
- 待办：
  1. AboutInfoDialog 从手写 DWM/QWindowKit 逻辑迁移到 ExWidgets::Frameless。
  2. 编写 README（项目说明、构建与使用方式）。
  3. 配置 clang-tidy 作为代码格式化工具；astyle 对复杂 lambda 表达式处理不佳，暂不作为标准。
  4. 新 Qt 5.15.19 稳定后清理旧 Qt 5.15.2。
  5. 软件图标等视觉资源在开发最后阶段统一处理。

## 4. 参考资料（按需查阅，不要整体读入上下文）

- `docs/README.md` — 技术文档主索引；第三方库配置、项目代码结构等详细知识见 `docs/`。
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
