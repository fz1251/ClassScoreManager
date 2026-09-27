# 待办事项

> 记录已明确、尚未动手的事项；完成后直接删除对应条目。主索引见 [README.md](README.md)。

1. 把项目目录迁移到 `D:\projects` 下的干净路径。
   现路径 `C:\Users\dell\Desktop\C++\projects\ClassScoreManager` 位于桌面目录下；迁移后需同步 VS Code 配置、构建目录与脚本里的绝对路径。

2. `AboutInfoDialog` 从手写 DWM 无边框逻辑迁移到 `Frameless::Frameless`；该对话框内部的小问题一并处理：`BLUR_RADIUS` 命名与用途不符、`paintEvent` 中 QPainter 的作用域、AccentPolicy 等 Win32 魔法值缺注释、连按方向键时窗口可能偏移、函数内 static 承担实例级状态。

3. 软件图标等视觉资源在开发最后阶段统一处理。

4. 打开文件时校验 displayOrder 是否为 1..n 的排列。
   `ClassDataManager::getStudentList` 用 `std::map<displayOrder, name>` 排序，重复的 displayOrder 会让对应学生在“删除学生”选择框里静默消失。
