# 待办事项

> 记录已明确、尚未动手的事项；完成后直接删除对应条目。主索引见 [README.md](README.md)。

1. 实机验证关于对话框的窗口效果：Win7 的整窗拖动与彩蛋 Aero 玻璃、Win10 的窗口顶部边框观感（QWindowKit 自绘 1px 系统边框且默认开启深色模式）、Win11 的四种窗口材质（Acrylic / Mica / MicaAlt / Blur）。

2. 打开文件时校验 displayOrder 是否为 1..n 的排列。
   `ClassDataManager::getStudentList` 用 `std::map<displayOrder, name>` 排序，重复的 displayOrder 会让对应学生在“删除学生”选择框里静默消失。
