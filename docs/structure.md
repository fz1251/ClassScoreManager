# 代码结构与数据流

## 1. 库的作用

| 库/模块 | 用途 |
|---|---|
| **Qt Core / Gui / Widgets** | 应用骨架、窗口、布局、信号槽、Model/View、拖放、对话框 |
| **FluentUI3Style** | 全局 Fluent 风格，`main.cpp` 直接 `a.setStyle(new FluentUI3Style)`，不依赖样式插件 |
| **ExWidgets** | ExWidgets 控件库；当前主要使用 `ExExpander` 实现侧栏折叠块 |
| **CMake + Ninja + MSVC x64** | 构建系统，使用 Qt 5.15.19 预设 |

## 2. 代码结构

```
src/
├── main.cpp                    # 启动入口：DPI 设置、FluentUI3Style、字体、MainWindow
├── core/                       # 数据与业务核心
│   ├── DataTypes.h             # 文件头、学生/模板/记录、排序设置及序列化
│   ├── ClassDataManager.h/.cpp # 唯一数据源：学生与分组/模板/记录/草稿/排序设置/字节序列化
│   ├── FileIoService.h/.cpp    # 文件 I/O 门面：GUI 线程请求，worker 线程执行
│   ├── FileIoWorker.h/.cpp     # worker：用 QFile/QSaveFile 整文件读写
│   └── ScoreSumReport.h/.cpp   # 按排序设置生成积分求和 HTML 报告
├── models/                     # Qt Model/View 模型
│   ├── ClassTableModel.h/.cpp   # 学生表格：列显隐、姓名编辑开关、查看索引、拖拽排序、只读的小组号列
│   ├── RecordListModel.h/.cpp   # 积分记录列表模型：recordsChanged 后重置
│   ├── StudentSearchModel.h/.cpp# 学生搜索列表模型：按姓名匹配，结果按表格显示顺序排列
│   └── TemplateListModel.h/.cpp # 积分模板列表模型：templatesChanged 后重置
├── pages/                      # 堆叠窗口页面（只建 UI、发信号，不含业务）
│   ├── WelcomePage.h/.cpp      # 欢迎页：打开/新建两个入口卡片，使用 resources/icons 矢量图标
│   ├── StudentTablePage.h/.cpp # 班级表格页：左表格 + 右侧 StudentTableSidebar；拖动排序时贴边自动滚动，右侧滚动条不显示
│   ├── GroupBoardPage.h/.cpp   # 小组管理页：左未分组面板 + 右小组容器，拖放完成分组与组内排序，右栏拖动时贴边自动滚动
│   ├── StudentTableSidebar.h/.cpp # 侧栏：5 个 ExExpander；学生块内提供姓名搜索与结果跳转，记录/模板块支持查看与上下项导航
│   ├── RecordListPage.h/.cpp   # 记录列表页：顶部工具行 + 多选列表
│   └── TemplateListPage.h/.cpp # 模板列表页：顶部工具行 + 多选列表
├── delegates/                  # 表格单元格委托
│   ├── ScoreExprDelegate.h/.cpp# 积分表达式委托
│   └── TemplateSpinBoxDelegate.h/.cpp # 模板分值 SpinBox 委托
├── widgets/                    # 自绘/复用控件
│   ├── CardHost.h              # 卡片容器统一接口：插入/移除卡片、刷新标题、提供卡片底色
│   ├── CardStyle.h/.cpp        # 小组卡片的尺寸与配色：按组号在 OKLCH 上取点生成组色
│   ├── DragAutoScroller.h/.cpp # 拖动时贴近滚动区边缘自动滚动：班级表格与小组容器列表共用
│   ├── FlowLayout.h/.cpp       # 流式布局：换行、隐藏项补位、重排弹簧动画、单列模式
│   ├── GroupCard.h/.cpp        # 小组容器：标题、落点判定与高亮、插入占位槽
│   ├── HomeActionCard.h/.cpp   # 欢迎页操作卡片
│   ├── LineSpacedLabel.h/.cpp  # 可调节行距标签
│   ├── OkCancelBox.h/.cpp      # 对话框底部的确定/取消按钮组
│   ├── ScoreTableView.h/.cpp   # 学生表格视图：积分列的输入即编辑、回车进入编辑、提交后移到下一行同列
│   ├── ShineLabel.h/.cpp       # 发光标题标签
│   ├── StudentCard.h/.cpp      # 学生胶囊卡片：悬停动效、拖动（略放大的缩略图）、单击改组号
│   └── UngroupedPanel.h/.cpp   # 未分组面板：单列卡片与毛玻璃取消分组蒙版
└── windows/                    # 主窗口与对话框（全部纯代码）
    ├── MainWindow.h/.cpp       # 窗口层协调器：文件/关于菜单，页面、侧栏与模型的信号连接
    ├── SumExportDialog.h/.cpp  # 积分导出对话框：排序配置 + OkCancelBox
    ├── GroupNumberDialog.h/.cpp # 设置小组号对话框：SpinBox + OkCancelBox
    ├── ItemSelectionDialog.h/.cpp # 列表选择入口：静态方法返回选中索引，基于 ExContentDialog
    └── AboutInfoDialog.h/.cpp  # 关于对话框
```

## 3. 架构关系图

```mermaid
flowchart TD
    MainWindow[MainWindow 窗口层协调器]
    Manager[ClassDataManager 数据源]
    TableModel[ClassTableModel 学生表格模型]
    SearchModel[StudentSearchModel 学生搜索模型]
    RecordModel[RecordListModel 记录列表模型]
    TemplateModel[TemplateListModel 模板列表模型]
    StudentPage[StudentTablePage]
    TableView[ScoreTableView 学生表格]
    Sidebar[StudentTableSidebar 侧栏]
    GroupPage[GroupBoardPage 小组管理页]
    Ungrouped[UngroupedPanel 未分组面板]
    GroupCard[GroupCard 小组容器]
    FlowLayout[FlowLayout 流式布局]
    StudentCard[StudentCard 学生卡片]
    AutoScroll[DragAutoScroller 拖动自动滚动]
    GroupNumber[GroupNumberDialog 设置小组号]
    RecordPage[RecordListPage]
    TemplatePage[TemplateListPage]
    SumExport[SumExportDialog]
    ItemSelect[ItemSelectionDialog]
    ScoreReport[ScoreSumReport]
    FileIo[FileIoService 文件 I/O 门面]
    Worker[FileIoWorker 工作线程]

    MainWindow -->|持有并提交数据| Manager
    MainWindow -->|读写文件| FileIo
    FileIo -->|队列请求| Worker
    MainWindow -->|持有并设置| TableModel
    MainWindow -->|持有| SearchModel
    MainWindow -->|持有| StudentPage
    MainWindow -->|持有| GroupPage
    MainWindow -->|持有| RecordPage
    MainWindow -->|持有| TemplatePage
    MainWindow -->|创建并读取配置| SumExport
    MainWindow -->|调用| ItemSelect
    MainWindow -->|生成 HTML| ScoreReport

    StudentPage --> TableView
    StudentPage --> Sidebar
    StudentPage -->|拖动排序时| AutoScroll
    GroupPage -->|拖动卡片时| AutoScroll
    GroupPage --> Ungrouped
    GroupPage --> GroupCard
    Ungrouped --> FlowLayout
    GroupCard --> FlowLayout
    FlowLayout --> StudentCard
    MainWindow -->|双击小组号列| GroupNumber
    GroupPage -->|单击学生卡片| GroupNumber
    GroupPage -->|分组写入| Manager
    TableModel -->|读取| Manager
    SearchModel -->|读取| Manager
    RecordModel -->|读取| Manager
    TemplateModel -->|读取| Manager
    TableView -->|setModel| TableModel
    Sidebar -->|setModel| SearchModel
    TableModel -->|行序变化后重新匹配| SearchModel
    TableModel -->|行序变化后重建卡片| GroupPage
    RecordPage -->|setListModel| RecordModel
    TemplatePage -->|setListModel| TemplateModel

    Sidebar -->|信号| MainWindow
    RecordPage -->|信号| MainWindow
    TemplatePage -->|信号| MainWindow
    Manager -->|recordsChanged / templatesChanged / dirtyStateChanged| RecordModel
    Manager -->|recordsChanged / templatesChanged / dirtyStateChanged| TemplateModel
    Manager -->|templatesChanged / dirtyStateChanged / recordsChanged| MainWindow
    Manager -->|studentsChanged| TableModel
    TableModel -->|groupAssignmentEdited| GroupPage
    TableModel -->|show/hideColumnRequested| TableView
```

## 4. 数据流向图

### 4.1 打开 / 新建文件

打开文件时，设备读取在 worker 线程完成；字节回到 GUI 线程后才解析并提交数据。

```mermaid
flowchart LR
    User[用户] -->|打开 .cls| Open[MainWindow::openFile]
    Open --> Read[FileIoService::readAll 工作线程读取]
    Read --> Done[MainWindow::onFileReadFinished]
    Done --> Parse[ClassDataManager::loadFromBytes]
    Parse --> Result{解析结果}
    Result -->|失败| Fail[提示错误并中止]
    Result -->|成功| Repair[字段归一化]
    Repair --> Warn[有修正则提示并置脏]
    Repair --> Emit[发出 recordsChanged / templatesChanged]
    Warn --> Emit
    Emit --> RecordModel[RecordListModel 重置]
    Emit --> TemplateModel[TemplateListModel 重置]
    Emit --> Preview[MainWindow::updateFilePreview]
    Preview --> Sidebar[侧栏文件信息块]
    Parse -->|成功| Reset[MainWindow::resetTable]
    Reset --> TableModel[ClassTableModel::resetTable]
    User -->|新建| NewFile[ClassDataManager::newFile]
```

### 4.2 新建 / 保存积分记录

```mermaid
flowchart LR
    SideNew[侧栏或列表页 新建记录] --> Draft[MainWindow::createScoreRecordDraft]
    Draft --> TableNew[ClassTableModel::newScoreRecord]
    TableNew --> ManagerNew[ClassDataManager::newRecord 建立空草稿]
    Draft --> State[updateRecordEditingActions 更新按钮状态]
    State --> SidebarState[侧栏：新建禁用，保存/应用模板启用]

    SideSave[侧栏 保存记录] --> Validate[MainWindow::saveScoreRecord]
    Validate --> Check{名称非空且不重复}
    Check -->|否| Warn[弹窗并阻止]
    Check -->|是| SetName[ClassDataManager::setRecordDraftDescription]
    SetName --> TableSave[ClassTableModel::saveScoreRecord]
    TableSave --> ManagerSave[ClassDataManager::saveRecord 写入记录]
    ManagerSave --> EmitRecords[发出 recordsChanged]
    EmitRecords --> RecordModel[RecordListModel 重置]
    EmitRecords --> FilePreview[updateFilePreview 更新数量]
```

### 4.3 记录 / 模板查看

```mermaid
flowchart LR
    Double[列表双击] --> Show[MainWindow::showRecord / showTemplate]
    View[侧栏 查看记录/模板] --> Select[ItemSelectionDialog 选择条目]
    Select --> Show
    Nav[侧栏 上一项/下一项] --> Show
    Exit[侧栏 退出查看] --> Hide[MainWindow::hideRecord / hideTemplate]
    Show --> SetPreview[tableModel->setRecordPreviewIndex / setTemplatePreviewIndex]
    Hide --> SetPreview
    SetPreview --> Column[表格显示对应查看列]
    Show --> SidebarSet[侧栏 setRecordPreview / setTemplatePreview]
    Hide --> SidebarSet
    SidebarSet --> Pos[更新查看标题与 x/x]
    Enter[切到记录/模板列表页] --> Sync[选中当前正在查看的那一行]
```

### 4.4 新建 / 改名积分模板

```mermaid
flowchart LR
    NewBtn[侧栏或列表页 新建模板] --> New[MainWindow::newTemplate]
    New --> Add[ClassDataManager::addTemplate 立即入库]
    Add --> EmitT[发出 templatesChanged]
    EmitT --> TModel[TemplateListModel 重置]
    New --> Show[MainWindow::showTemplate 选中新模板]
    Show --> Focus[侧栏名称输入框聚焦并全选]

    Rename[侧栏 更新名称] --> Update[MainWindow::updateTemplateName]
    Update --> Check{名称非空且不重复}
    Check -->|否| Warn[弹窗并恢复原名称]
    Check -->|是| SetDesc[ClassDataManager::setTemplateDescription]
    SetDesc --> EmitT
```

### 4.5 导出选中积分记录

```mermaid
flowchart LR
    Select[记录页多选行] --> Selected[MainWindow::exportSelectedRecords 收集行号]
    Selected --> Dialog[SumExportDialog 选择排序配置]
    Dialog --> Sum[MainWindow::exportScoreSum 按选中记录累加每个学生分数]
    Sum --> Report[ScoreSumReport::generateHtmlString]
    Report --> Write[FileIoService::writeAll 工作线程写入]
    Write --> Finish[MainWindow::onFileWriteFinished]
    Finish -->|成功| Settings[ClassDataManager::setPreviousSettings 记住配置]
    Finish -->|失败| Error[提示错误，不修改设置]
```

文件任务在独立线程执行，GUI 线程始终处理事件。任务超过 150ms 仍未完成时，右下角显示不可关闭的 `ExInfoBar`，标题为“正在读取/保存/导出文件”，操作区显示不确定态 `ExProgressRing`；完成或失败后关闭提示再弹 Toast。写出使用 `QSaveFile`，先写同目录临时文件、提交成功才替换目标文件，失败时磁盘上的旧文件保持原样。

### 4.6 搜索学生并跳转表格选区

```mermaid
flowchart LR
    Input[侧栏搜索框输入] --> Model[StudentSearchModel::setFilter 按姓名匹配]
    Model --> List[侧栏结果列表]
    Click[点击结果项] --> Map[MainWindow::selectStudentFromSearch 换算学生索引与表格行]
    Map --> Select[表格选中该行并滚动到可见位置]
    Change[学生增删 / 拖动排序 / 改名] --> Model
```

### 4.7 拖放分组与组内排序

```mermaid
flowchart LR
    Drag[拖动学生卡片] --> Drop{落点}
    Drop -->|小组容器| Position[按插入位置落位]
    Drop -->|左侧未分组面板| Cancel[取消该学生分组]
    Position --> Move[ClassDataManager::moveStudentToGroup 更新组号与组内顺序]
    Cancel --> Move
    Move --> Same{分组与位置是否变化}
    Same -->|没有变化| Keep[卡片放回原位，不置脏]
    Same -->|有变化| Emit[发出 studentsChanged]
    Emit --> Column[表格小组号列刷新]
    Same -->|有变化| Animate[搬移卡片并播放让位动画]
```

### 4.8 在表格里设置学生小组号

```mermaid
flowchart LR
    Double[双击小组号单元格] --> Switch{小组信息编辑开关}
    Switch -->|关闭| Ignore[不响应]
    Switch -->|开启| Dialog[GroupNumberDialog 输入组号]
    Dialog -->|取消| Ignore
    Dialog -->|确定| Assign[ClassTableModel::assignGroupNumber]
    Assign --> Move[ClassDataManager::moveStudentToGroup]
    Move --> Emit[发出 groupAssignmentEdited]
    Emit --> Reload[小组管理页按数据重建]
```

### 4.9 保存班级信息

```mermaid
flowchart LR
    Save[菜单保存或关闭确认] --> Guard{已有文件任务}
    Guard -->|是| Ignore[直接返回]
    Guard -->|否| Serialize[ClassDataManager::serializeTo 生成快照]
    Serialize --> Write[FileIoService::writeAll 工作线程写入]
    Write --> Finish[MainWindow::onFileWriteFinished]
    Finish -->|成功| Commit[markFileSaved 比较快照并提交路径/脏状态]
    Finish -->|失败| Error[保留原路径与脏状态，提示错误]
    Commit -->|完全一致| Toast[成功 Toast 或关闭续作]
    Commit -->|保存期间有新修改| KeepDirty[保持脏状态并提示]
```

保存开始时序列化一份字节快照交给 worker；写成功后重新序列化当前内存数据并与快照比较，只有完全一致且没有未保存的记录草稿时才清除脏标记。这样不必在保存期间锁住编辑，也不会把写盘开始后的新修改误判为已保存。

## 5. 状态归属

| 状态 | 归属 |
|---|---|
| 学生（含小组号与组内顺序）、模板、记录、记录草稿、排序设置、脏标记 | `ClassDataManager` |
| 当前文件名、记录/模板查看索引、小组信息编辑开关、页面会话状态 | `MainWindow` |
| 当前文件任务、保存快照、延迟提示、关闭续作 | `MainWindow` |
| 设备 I/O 与 worker 线程 | `FileIoService` / `FileIoWorker` |
| 表格列显隐、行排序映射、姓名编辑开关、查看列渲染状态 | `ClassTableModel` |
| 小组容器与学生卡片的归属、拖放落点与占位状态 | `GroupBoardPage` / `GroupCard` / `UngroupedPanel` |
| 学生搜索关键字与匹配结果 | `StudentSearchModel` |
| 按钮可用性、折叠块展开状态、列表选中状态 | 各页面 / `StudentTableSidebar` |
