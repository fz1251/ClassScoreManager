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
│   ├── ClassDataManager.h/.cpp # 唯一数据源：学生/模板/记录/草稿/排序设置/文件读写
│   └── ScoreSumReport.h/.cpp   # 按排序设置生成积分求和 HTML 报告
├── models/                     # Qt Model/View 模型
│   ├── ClassTableModel.h/.cpp  # 学生表格：列显隐、编辑开关、预览索引、拖拽排序
│   ├── RecordListModel.h/.cpp  # 积分记录列表模型：recordsChanged 后重置
│   └── TemplateListModel.h/.cpp# 积分模板列表模型：templatesChanged 后重置
├── pages/                      # 堆叠窗口页面（只建 UI、发信号，不含业务）
│   ├── WelcomePage.h/.cpp      # 欢迎页：打开/新建两个入口卡片
│   ├── StudentTablePage.h/.cpp # 班级表格页：左表格 + 右侧 StudentTableSidebar
│   ├── StudentTableSidebar.h/.cpp # 侧栏：文件信息/表格选项/积分记录/积分模板 4 个 ExExpander
│   ├── RecordListPage.h/.cpp   # 记录列表页：顶部工具行 + 多选列表
│   └── TemplateListPage.h/.cpp # 模板列表页：顶部工具行 + 多选列表
├── delegates/                  # 表格单元格委托
│   ├── ScoreExprDelegate.h/.cpp# 积分表达式委托
│   ├── GroupSpinBoxDelegate.h/.cpp # 小组号 SpinBox 委托
│   └── TemplateSpinBoxDelegate.h/.cpp # 模板分值 SpinBox 委托
├── widgets/                    # 自绘/复用控件
│   ├── HomeActionCard.h/.cpp   # 欢迎页操作卡片
│   ├── LineSpacedLabel.h/.cpp  # 可调节行距标签
│   └── ShineLabel.h/.cpp       # 发光标题标签
└── windows/                    # 主窗口与对话框（全部纯代码）
    ├── MainWindow.h/.cpp       # 窗口层协调器：文件/学生/关于菜单，所有页面与模型的连接
    ├── SumExportDialog.h/.cpp  # 积分导出对话框：只保留排序配置，确定/取消
    ├── ItemSelectionDialog.h/.cpp # 下拉选择对话框：删除学生、应用模板
    └── AboutInfoDialog.h/.cpp  # 关于对话框
```

## 3. 架构关系图

```mermaid
flowchart TD
    MainWindow[MainWindow 窗口层协调器]
    Manager[ClassDataManager 数据源]
    TableModel[ClassTableModel 学生表格模型]
    RecordModel[RecordListModel 记录列表模型]
    TemplateModel[TemplateListModel 模板列表模型]
    StudentPage[StudentTablePage]
    TableView[QTableView 学生表格]
    Sidebar[StudentTableSidebar 侧栏]
    RecordPage[RecordListPage]
    TemplatePage[TemplateListPage]
    SumExport[SumExportDialog]
    ItemSelect[ItemSelectionDialog]
    ScoreReport[ScoreSumReport]

    MainWindow -->|持有并读写| Manager
    MainWindow -->|持有并设置| TableModel
    MainWindow -->|持有| StudentPage
    MainWindow -->|持有| RecordPage
    MainWindow -->|持有| TemplatePage
    MainWindow -->|创建并读取配置| SumExport
    MainWindow -->|创建| ItemSelect
    MainWindow -->|生成 HTML| ScoreReport

    StudentPage --> TableView
    StudentPage --> Sidebar
    TableModel -->|读取| Manager
    RecordModel -->|读取| Manager
    TemplateModel -->|读取| Manager
    TableView -->|setModel| TableModel
    RecordPage -->|setListModel| RecordModel
    TemplatePage -->|setListModel| TemplateModel

    Sidebar -->|信号| MainWindow
    RecordPage -->|信号| MainWindow
    TemplatePage -->|信号| MainWindow
    Manager -->|recordsChanged / templatesChanged / dirtyStateChanged| RecordModel
    Manager -->|recordsChanged / templatesChanged / dirtyStateChanged| TemplateModel
    Manager -->|templatesChanged / dirtyStateChanged / recordsChanged| MainWindow
    TableModel -->|show/hideColumnRequested| TableView
```

## 4. 数据流向图

### 4.1 打开 / 新建文件

```mermaid
flowchart LR
    User[用户] -->|打开 .cls| Open[MainWindow::openFile]
    Open --> Parse[ClassDataManager::openFile / newFile]
    Parse --> Emit[发出 recordsChanged / templatesChanged]
    Emit --> RecordModel[RecordListModel 重置]
    Emit --> TemplateModel[TemplateListModel 重置]
    Emit --> Preview[MainWindow::updateFilePreview]
    Preview --> Sidebar[侧栏文件信息块]
    Open --> Reset[MainWindow::resetTable]
    Reset --> TableModel[ClassTableModel::resetTable]
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

### 4.3 记录 / 模板预览

```mermaid
flowchart LR
    Double[列表双击] --> Show[MainWindow::showRecord / showTemplate]
    Nav[侧栏 前 / 后] --> Show
    Show --> SetPreview[tableModel->setRecordPreviewIndex / setTemplatePreviewIndex]
    SetPreview --> Column[表格显示对应预览列]
    Show --> SidebarSet[侧栏 setRecordPreview / setTemplatePreview]
    SidebarSet --> Pos[更新描述文字与 x/x]
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
    Report --> File[写出 HTML 文件]
    Sum --> Settings[ClassDataManager::setPreviousSettings 记住配置]
```

## 5. 状态归属

| 状态 | 归属 |
|---|---|
| 学生、模板、记录、记录草稿、排序设置、脏标记 | `ClassDataManager` |
| 当前文件名、记录/模板预览索引、页面会话状态 | `MainWindow` |
| 表格列显隐、行排序映射、编辑开关、预览渲染状态 | `ClassTableModel` |
| 按钮可用性、折叠块展开状态、列表选中状态 | 各页面 / `StudentTableSidebar` |
