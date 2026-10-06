# 第三方库与携带依赖

> 本文档记录项目引入的第三方美化库及其随附依赖。主索引见 [README.md](README.md)。

## 1. 总览

| 库 | 位置 | 用途 | 构建方式 |
| --- | --- | --- | --- |
| FluentUIStyle | `third_party/FluentUIStyle`（git 子模块） | FluentUI3 全局样式、ExWidgets 扩展控件、Frameless 无边框组件 | 随主项目一起编译 |
| QWindowKit | `third_party/FluentUIStyle/3rd/qwindowkit` | `Frameless` 依赖的无边框/标题栏基础设施 | 随 FluentUIStyle 一起编译 |
| kissfft | `third_party/FluentUIStyle/3rd/kissfft` | ExWidgets 中频谱控件的 FFT 后端 | 编译为静态库 |
| qmsetup | `third_party/FluentUIStyle/3rd/qwindowkit/qmsetup` | QWindowKit 的构建期辅助工具 | 主项目配置阶段自动构建并安装到构建目录 |

## 2. FluentUIStyle

### 基本信息

- 子模块路径：`third_party/FluentUIStyle`
- 上游仓库：https://github.com/XHY-ChuJian/FluentUIStyle.git
- 当前固定提交：`b8a7ce5b36cbc022b86227590deaa033fcc55534`（tag `4.0` 之后的第 4 个提交）
- 用途：
  - `fluentui3style/`：`FluentUI3Style` 全局 QStyle，随程序直接实例化使用。
  - `ExWidgets/`：`ExWidgets::ExWidgets` 扩展控件库。
  - `frameless/`：独立静态库 `Frameless`，提供无边框窗口组件（`FluentTitleBar` / `FluentWindowFrame`）。
  - `ExWidgets/dialogs/`：`ExContentDialog`（WinUI3 风格模态对话框）、`ExInfoBar` / `ExInfoBarHost`（窗口级非阻塞通知条）、`ExMessageBox` 等对话框组件。
  - `3rd/`：携带的 QWindowKit 与 kissfft 依赖。

### 构建方式

FluentUIStyle 以源码子目录方式加入主项目构建，不构建、不部署 Qt 样式插件。主项目在 `add_subdirectory` 前设置以下选项：

| 选项 | 值 | 作用 |
| --- | --- | --- |
| `BUILD_LIBRARY` | ON | 构建 `FluentUI3Style` 样式库 |
| `BUILD_PLUGIN` | OFF | 不构建样式插件 |
| `BUILD_EXAMPLES` | OFF | 不构建示例程序 |
| `BUILD_FRAMELESS` | ON | 构建 QWindowKit 支撑的无边框静态库 `Frameless` |

FluentUIStyle 的 CMake 需要 `Qt5Svg`（或 Qt6 的 Svg 组件）。当前自编译 Qt 5.15.19 已包含 qtsvg。

### 项目中的使用方式

- `src/main.cpp` 直接实例化样式：
  ```cpp
  a.setStyle(new FluentUI3Style);
  ```
  因此运行时不需要把样式插件 DLL 部署到 Qt 的 `plugins/styles`。
- 需要 Fluent 属性枚举/常量时，头文件写作：
  ```cpp
  #include "fluentui3styleproperties.h"
  ```
  （不带 `FluentUI3Style/` 前缀，include 目录由 FluentUIStyle 子项目提供。）
- 主程序链接的目标：
  ```cmake
  target_link_libraries(ClassScoreManager PRIVATE
      FluentUI3Style
      ExWidgets::ExWidgets
      Frameless::Frameless
      QWindowKit::Widgets
  )
  ```
- 主程序直接使用 QWindowKit 的 `QWK::WidgetWindowAgent`（`#include <QWKWidgets/widgetwindowagent.h>`）：`AboutInfoDialog` 在 Win10 及以上以它实现无边框窗口，整窗拖动、边缘缩放、DWM 帧扩展与窗口材质全部由它接管，因此 CMake 中除 `Frameless::Frameless` 外还显式链接 `QWindowKit::Widgets`。
- `Frameless::Frameless` 的 `FluentTitleBar` / `FluentWindowFrame` 只接受 `QMainWindow*`，对话框类窗口不能使用该门面。
- 对话框统一走 `ExContentDialog`：确认框用 `setContent` + 主副按钮，列表选择框用 `setContentWidget` 承载下拉框。头文件写作 `#include "excontentdialog.h"`。
- 提示信息统一走 `ExInfoBar`：`MainWindow` 构造函数用 `ExInfoBarHost::setDefaultTarget(centralWidget)` 设定通知宿主，各处通过 `MainWindow::showNotification()` 弹出窗口级通知条；严重级别按「文件读写等操作失败 → `Error`、输入校验不通过 → `Warning`、操作完成 → `Success`、状态说明 → `Informational`」选取。头文件写作 `#include "exinfobarhost.h"`。

### 构建产物位置

所有动态库与主程序输出到当前构建目录的 `bin/`，导入库输出到 `lib/`：

| 配置 | `build/.../bin` 中的相关文件 |
| --- | --- |
| Debug | `FluentUI3Styled.dll`、`ExWidgetsd.dll`、`QWKCored.dll`、`QWKWidgetsd.dll`、`ClassScoreManager.exe` |
| Release | `FluentUI3Style.dll`、`ExWidgets.dll`、`QWKCore.dll`、`QWKWidgets.dll`、`ClassScoreManager.exe` |

Debug 配置下 FluentUIStyle 相关目标统一带 `d` 后缀。
`Frameless` 是静态库，只输出到 `lib/`（Debug 为 `Framelessd.lib`），不再产出独立 DLL。

## 3. QWindowKit

- 位置：`third_party/FluentUIStyle/3rd/qwindowkit`
- 目标：`QWKCore`、`QWKWidgets`；`Frameless` 公开链接 `QWindowKit::Widgets`。
- 构建选项由主项目在预添加时固定：

  | 选项 | 值 |
  | --- | --- |
  | `QWINDOWKIT_BUILD_EXAMPLES` | OFF |
  | `QWINDOWKIT_BUILD_QUICK` | OFF |
  | `QWINDOWKIT_BUILD_WIDGETS` | ON |
  | `QWINDOWKIT_BUILD_DOCUMENTATIONS` | OFF |
  | `QWINDOWKIT_INSTALL` | ON |

### 主项目必须预添加 QWindowKit

主项目 `CMakeLists.txt` 在添加 FluentUIStyle 之前，先单独执行：

```cmake
add_subdirectory(third_party/FluentUIStyle/3rd/qwindowkit "${CMAKE_BINARY_DIR}/qwindowkit")
```

原因：

- 若让 ExWidgets 在其默认的深目录 `ExWidgets/CMakeFiles/...` 下添加 QWindowKit，QWindowKit 生成的私有转发头相对路径过长，MSVC 编译时报 C1083。
- 预先添加到浅目录 `${CMAKE_BINARY_DIR}/qwindowkit` 可让相对路径保持可接受长度。
- 预添加后 `QWindowKit::Widgets` 目标已存在，ExWidgets 会直接复用，不再重复添加。

### `QWINDOWKIT_INSTALL ON`

该选项必须为 ON：FluentUIStyle 安装时导出的 `ExWidgetsTargets` 引用了 `QWKWidgets`，关闭安装会让 CMake 的 export 校验报 `requires target "QWKWidgets" that is not in any export set`。

## 4. kissfft

- 位置：`third_party/FluentUIStyle/3rd/kissfft`
- 目标：静态库 `kissfft`，由 ExWidgets 私有链接，主项目不直接使用。
- 无需额外配置；ExWidgets 的 CMake 会自动添加并启用 C 语言。

## 5. qmsetup（构建期辅助工具）

- 位置：`third_party/FluentUIStyle/3rd/qwindowkit/qmsetup`
- QWindowKit 配置阶段检测不到已安装的 qmsetup 时，会通过其私有模块自动构建并安装一份本地副本。
- 本地副本的安装根目录为 `${CMAKE_BINARY_DIR}/_install`，配置/构建日志位于 `${CMAKE_BINARY_DIR}/_build/qmsetup_*.log`。
- 主项目不需要直接链接或部署 qmsetup。

## 6. 构建环境要求

- FluentUIStyle、QWindowKit 和主项目统一使用 MSVC 2022 x64（`cl`），不得用 Strawberry Perl 自带的 GCC 编译。
- 该环境由 `CMakePresets.json` 的隐藏预设 `msvc2022-x64-base` 提供（PATH/INCLUDE/LIB 已内置），普通终端与 VS Code 构建均直接使用预设，无需先进入 VS 开发环境。
- 如果构建日志出现 `C:/dev/runtimes/StrawberryPerl/c/bin/c++.exe` 或 `gcc.exe`，说明构建目录被错误环境污染，删除该 build 目录后用预设重新配置。

## 7. 更新 FluentUIStyle 子模块

1. 确认子模块工作区干净，并与用户确认目标提交：
   ```bash
   git submodule status
   git -C third_party/FluentUIStyle status
   ```
2. 拉取并切换到目标提交：
   ```bash
   git -C third_party/FluentUIStyle fetch origin --prune
   git -C third_party/FluentUIStyle checkout <目标提交>
   ```
3. 检查 FluentUIStyle / QWindowKit 的构建选项、目标名、接口是否有变化，必要时调整主项目 `CMakeLists.txt`。
4. 完整构建 Debug/Release 两个预设验证；构建通过后由用户运行程序做视觉验证。
5. 用户验证通过后，提交主仓库中的子模块指针变更；助手不执行 git commit。

## 8. 打包注意事项

- 分发时把对应配置的 `bin/` 中全部 DLL 与 `ClassScoreManager.exe` 一起打包。
- FluentUIStyle 的许可证与第三方声明分别在其根目录 `LICENSE`、`THIRD_PARTY_NOTICES.md`；正式分发时随包携带或按其要求署名。
