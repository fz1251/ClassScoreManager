# 项目上下文

> 本文档是 `CLAUDE.md` 的补充，记录项目相关的详细上下文；内容以当前仓库代码为准，修改后同步更新。

## 1. 项目与技术栈

- 项目：ClassScoreManager，班级积分管理系统，面向班级日常积分管理。
- 技术栈：C++17 + Qt Widgets（纯 Widgets，禁用 QML）。
- 构建：CMake + Ninja，开发环境为 VS Code（CMake Tools + clangd）。
- 代码同时保留 Qt5/Qt6 条件编译兼容。

## 2. Qt 套件

- 当前套件：自编译 Qt 5.15.19（qtbase + qtsvg），MSVC 2022 x64，debug-and-release。
- 安装前缀：`D:/Projects/Qt5.15.19/msvc2022_64`。
- 重建脚本：`D:\Projects\Qt5.15.19\build_qt.ps1`（qtbase）、`D:\Projects\Qt5.15.19\build_qtsvg.ps1`（qtsvg）。
  - 必须在 x64 VS2022 环境中运行；脚本会先验证 cl 为 x64，并清空旧构建再 configure/build/install。
  - 该 Qt 曾整库被误建成 i386（`qsizetype` 4 字节）导致 `.cls` 文件兼容问题；凡涉及 Qt 重建或新装，必须确认编译器和产物都是 x64，不能只依赖目录名。
- 旧 Qt 5.15.2 暂保留，等新套件稳定后再清理。

## 3. CMake 构建

- 预设：
  - `qt5-msvc2022-debug` → `build/vscode-qt5.15.19-debug`
  - `qt5-msvc2022-release` → `build/vscode-qt5.15.19-release`
  - 两个预设的 `CMAKE_PREFIX_PATH` 均指向当前 Qt，并开启 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`。
- 两个预设继承隐藏预设 `msvc2022-x64-base`：
  - 固定 `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` 为 `cl`。
  - 在 PATH/INCLUDE/LIB 中内置 VS2022 x64 工具链与 Windows SDK 路径。
  - 普通终端和 VS Code 构建都无需先进入 VS 开发环境，也不能让 CMake 自行选中 Strawberry Perl 的 GCC。
- VS 或 Windows SDK 版本升级后，需同步更新 `msvc2022-x64-base` 与根目录 `.clangd` 里写死的 MSVC / Windows Kits 版本路径。
- 产物统一输出到对应构建目录的 `bin/`（exe 与所有 DLL）和 `lib/`。
- clangd：
  - 使用 Qt Creator 自带版本（VS Code 中由 `clangd.path` 指定）。
  - 读取 debug 的 `compile_commands.json`。
  - 系统头目录由根目录 `.clangd` 补充。
- 构建信息：
  - 每次构建都会运行 `cmake/generate_build_info.cmake`。
  - 生成 `${CMAKE_BINARY_DIR}/generated/app_build_info.h`，仅在日期变化时重写。
  - 界面通过其中的 `APP_BUILD_DATE` 宏显示构建日期。
- 排查提示：
  - 若构建错误显示编译器是 `C:/dev/runtimes/StrawberryPerl/c/bin/c++.exe` / `gcc.exe`，说明该 build 目录的 CMakeCache 被错误环境污染；删除对应 build 目录后用预设重新配置。

## 4. CMakeLists.txt 关键点

- 版本无关 `find_package(QT NAMES Qt6 Qt5 ...)`；当前实际走 Qt 5.15.19。
- 开启 AUTOMOC/AUTORCC；不使用 `.ui`，未启用 AUTOUIC。
- `target_include_directories` 只包含 `generated` 目录；项目根目录 include 与 customwidget 相对头路径支持均不再需要。
- rc.exe 的字符串中 `\` 是转义符，图标路径必须把反斜杠翻倍后再写入 RC 文件；MSVC 加 `/utf-8`。
- FluentUIStyle 以子目录构建：
  - `BUILD_LIBRARY ON`、`BUILD_PLUGIN OFF`、`BUILD_EXAMPLES OFF`、`EXWIDGETS_BUILD_FRAMELESS ON`。
  - 其依赖 Qt5Svg。
- QWindowKit：
  - 必须预先 `add_subdirectory` 到浅目录 `${CMAKE_BINARY_DIR}/qwindowkit`，避免深路径生成的私有 forwarding header 相对路径过长。
  - `QWINDOWKIT_INSTALL ON` 才能通过 CMake export 校验。
- 主程序链接：Qt Core/Gui/Widgets、FluentUI3Style、ExWidgets::ExWidgets、ExWidgets::Frameless、dwmapi。

## 5. 样式

- `src/main.cpp` 直接 `a.setStyle(new FluentUI3Style)`；样式库随项目构建，不再使用或部署 Qt 样式插件。
- 需要 Fluent 属性头时写作 `#include "fluentui3styleproperties.h"`（不带 `FluentUI3Style/` 前缀）。
- 全局视觉统一走 FluentUI3Style。
- `AboutInfoDialog` 是整体独立定制的对话框，其局部 QSS 属该对话框自身的完整定制，与主界面样式不冲突。
- `AboutInfoDialog` 当前保留手写 DWM/无边框标题栏逻辑；已链接 ExWidgets::Frameless 但尚未切换，迁移到 `FluentTitleBar`/`FluentWindowFrame` 是单独待办。

## 6. UI 现状

- 纯代码 UI 编写规范见根目录 `CLAUDE.md` 2.8。
- MainWindow 为纯代码构建，四个页面独立为类：
  - `src/pages/WelcomePage`
  - `src/pages/StudentTablePage`
  - `src/pages/RecordListPage`
  - `src/pages/TemplateListPage`
- `StudentTablePage` 右侧为 `StudentTableSidebar`，用 ExWidgets 的 `ExExpander` 组织文件信息、表格选项、积分记录、积分模板四个折叠块；积分记录块包含预览导航、新建/应用模板、名称输入与保存（新建直接开草稿，保存时校验并填写名称），积分模板块包含预览导航、新建模板与改名编辑。
- `RecordListPage`、`TemplateListPage` 支持多选，顶部有新建、导出/删除选中、清空工具按钮。
- `src/windows` 的 `AboutInfoDialog`、`ItemSelectionDialog`、`MainWindow`、`SumExportDialog` 均为纯代码实现。
- 学生添加与批量导入合并为同一个多行输入入口。

## 7. 第三方库

- 第三方库细节见 `docs/third-party.md`。
- FluentUIStyle：git 子模块 `third_party/FluentUIStyle`，随主项目一起构建。

## 8. 部署方式

- 打包后拷入 U 盘，免安装直接运行；保持体积小、启动快。
- 运行时依赖 Qt bin（Qt DLL）与构建输出 `bin/`（FluentUI3Style、ExWidgets、QWK 等 DLL），打包时按需收集。

## 9. 主要模块

- `src/core`：数据管理、报表。
- `src/models`：表格模型、列表模型。
- `src/widgets`：自绘、可复用控件。
- `src/pages`：堆叠窗口中的页面。
- `src/windows`：主窗口与对话框。
- `src/delegates`：表格单元格委托。

## 10. 命名约定

- 业务命名不使用 `Score` 前缀。
- 积分记录、积分模板分别用 `Record`、`Template` 命名，与 C++ 关键字冲突时视情况加修饰或复数。

## 11. 应用图标

- 图标文件：`resources/icon.ico`。
- 通过 CMake 生成的 RC 文件设置。

## 12. 待办

1. `AboutInfoDialog` 从手写 DWM/QWindowKit 逻辑迁移到 `ExWidgets::Frameless`。
2. 编写 README（项目说明、构建与使用方式）。
3. 配置 clang-tidy 作为代码格式化工具；astyle 对复杂 lambda 表达式处理不佳，暂不作为标准。
4. 新 Qt 5.15.19 稳定后清理旧 Qt 5.15.2。
5. 软件图标等视觉资源在开发最后阶段统一处理。
