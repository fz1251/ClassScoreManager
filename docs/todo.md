# 待办事项

> 记录已明确、尚未动手的事项；完成后直接删除对应条目。主索引见 [README.md](README.md)。

1. 把项目目录迁移到 `D:\projects` 下的干净路径。
   现路径 `C:\Users\dell\Desktop\C++\projects\ClassScoreManager` 含正则元字符 `+`：FluentUIStyle 的 `ExWidgets/CMakeLists.txt` 把绝对路径拼进正则，遇到 `+` 会让 CMake 配置阶段直接失败。迁移后需同步 VS Code 配置、构建目录与脚本里的绝对路径。

2. 撤销 FluentUIStyle 的本地补丁并重新更新子模块。
   `third_party/FluentUIStyle/ExWidgets/CMakeLists.txt` 带一处本地修改：正则 pattern 由 `"^${CMAKE_CURRENT_SOURCE_DIR}/frameless/"` 改为 `"/frameless/"`。等上游修掉同源问题后撤销该修改、更新子模块到修复版本；在此之前不拉取 FluentUIStyle 的更新。

3. `AboutInfoDialog` 从手写 DWM 无边框逻辑迁移到 `ExWidgets::Frameless`。

4. 编写 README（项目说明、构建与使用方式）。

5. 配置 clang-tidy 作为代码格式化工具；astyle 对复杂 lambda 表达式处理不佳，暂不作为标准。

6. 新 Qt 5.15.19 稳定后清理旧 Qt 5.15.2。

7. 软件图标等视觉资源在开发最后阶段统一处理。
