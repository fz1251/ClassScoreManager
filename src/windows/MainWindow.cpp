#include "MainWindow.h"

#include "../core/ClassDataManager.h"
#include "../core/ScoreSumReport.h"
#include "../delegates/ScoreExprDelegate.h"
#include "../delegates/TemplateSpinBoxDelegate.h"
#include "../models/ClassTableModel.h"
#include "../models/RecordListModel.h"
#include "../models/StudentSearchModel.h"
#include "../models/TemplateListModel.h"
#include "../pages/GroupBoardPage.h"
#include "../pages/RecordListPage.h"
#include "../pages/StudentTablePage.h"
#include "../pages/StudentTableSidebar.h"
#include "../pages/TemplateListPage.h"
#include "../pages/WelcomePage.h"
#include "../widgets/ScoreTableView.h"
#include "AboutInfoDialog.h"
#include "GroupNumberDialog.h"
#include "ItemSelectionDialog.h"
#include "SumExportDialog.h"
#include "excontentdialog.h"
#include "exinfobarhost.h"
#include "fluentui3styleproperties.h"

#include <QAbstractItemView>
#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeySequence>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTabBar>
#include <QTableView>
#include <QTextStream>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , manager(new ClassDataManager(this))
    , tableModel(new ClassTableModel(manager, this))
{
    resize(800, 600);
    setMinimumSize(800, 600);
    setWindowTitle(tr("班级积分管理系统"));

    // 中央区域：导航栏在左，五个页面由堆叠窗口承载
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget = new QStackedWidget(centralWidget);
    mainLayout->addWidget(stackedWidget);

    auto* welcomePage = new ::WelcomePage(stackedWidget);
    studentTablePage = new ::StudentTablePage(stackedWidget);
    groupBoardPage = new ::GroupBoardPage(manager, stackedWidget);
    recordListPage = new RecordListPage(stackedWidget);
    templateListPage = new TemplateListPage(stackedWidget);
    stackedWidget->addWidget(welcomePage);
    stackedWidget->addWidget(studentTablePage);
    stackedWidget->addWidget(groupBoardPage);
    stackedWidget->addWidget(recordListPage);
    stackedWidget->addWidget(templateListPage);

    // 导航栏：文件打开后显示，切换班级信息、小组管理、积分记录、积分模板页
    navigationTabBar = new QTabBar(centralWidget);
    navigationTabBar->setShape(QTabBar::RoundedWest);
    navigationTabBar->setDrawBase(false);
    navigationTabBar->setExpanding(false);
    navigationTabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    navigationTabBar->setProperty(TabBarStyleProperty, TabBarStyle::Navigation);
    navigationTabBar->addTab(tr("班级信息"));
    navigationTabBar->addTab(tr("小组管理"));
    navigationTabBar->addTab(tr("积分记录"));
    navigationTabBar->addTab(tr("积分模板"));
    mainLayout->insertWidget(0, navigationTabBar);

    setCentralWidget(centralWidget);
    // 通知层挂在中央区，窗口级通知不会压住菜单栏
    ExInfoBarHost::setDefaultTarget(centralWidget);
    statusBar()->setVisible(false);

    createActions();
    createMenus();

    // 列表模型只由视图持有，MainWindow 后续不直接访问
    auto* recordModel = new RecordListModel(manager, this);
    auto* templateModel = new TemplateListModel(manager, this);
    auto* tableView = studentTablePage->tableView();
    tableView->setModel(tableModel);
    recordListPage->setListModel(recordModel);
    templateListPage->setListModel(templateModel);

    // 学生搜索模型：侧栏展示匹配结果，MainWindow 负责把结果行换算成表格行
    searchModel = new StudentSearchModel(manager, this);
    studentTablePage->sidebar()->setStudentSearchModel(searchModel);

    // 表格单元格委托：两个积分列支持输入即编辑与回车进入编辑
    tableView->setItemDelegateForColumn(ClassTableModel::CurrentRecord, new ScoreExprDelegate(this));
    tableView->setItemDelegateForColumn(ClassTableModel::ScoreTemplatePreview, new TemplateSpinBoxDelegate(this));
    tableView->setInlineEditColumns({ClassTableModel::CurrentRecord, ClassTableModel::ScoreTemplatePreview});

    // 小组号列不可原地编辑，双击弹出设置小组对话框，与小组管理页共用同一个入口
    connect(tableView, &QTableView::doubleClicked, this, &MainWindow::editStudentGroup);

    // 表格模型与视图的列显隐联动
    connect(tableModel, &ClassTableModel::showColumnRequested, tableView, &QTableView::showColumn);
    connect(tableModel, &ClassTableModel::hideColumnRequested, tableView, &QTableView::hideColumn);

    // 小组分配：小组管理页与表格页共用同一份数据，任一侧改动都刷新另一侧
    connect(manager, &ClassDataManager::studentsChanged, tableModel, &ClassTableModel::refreshGroupColumn);
    connect(tableModel, &ClassTableModel::groupAssignmentEdited, groupBoardPage, &::GroupBoardPage::reload);

    // 欢迎页卡片请求
    connect(welcomePage, &::WelcomePage::openFileRequested, this, &MainWindow::openFileDialog);
    connect(welcomePage, &::WelcomePage::newFileRequested, this, &MainWindow::createNewFile);

    // 学生表页开关面板
    connect(studentTablePage->sidebar(), &StudentTableSidebar::dragSortChanged, this,
            [tableView](bool enabled)
            {
                if(enabled)
                {
                    tableView->setDragDropMode(QAbstractItemView::InternalMove);
                    tableView->setDefaultDropAction(Qt::MoveAction);
                }
                else
                {
                    tableView->setDragDropMode(QAbstractItemView::NoDragDrop);
                }
            });
    connect(studentTablePage->sidebar(), &StudentTableSidebar::nameEditChanged, tableModel, &ClassTableModel::setStudentEditable);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::groupEditChanged, this,
            [this](bool enabled)
            {
                groupEditEnabled = enabled;
            });

    // 侧栏学生操作块：添加、删除学生
    connect(studentTablePage->sidebar(), &StudentTableSidebar::addStudentsRequested, this, &MainWindow::addStudentsDialog);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::deleteStudentRequested, this, &MainWindow::deleteStudentDialog);

    // 侧栏学生搜索：输入即时过滤，点击结果选中对应学生；学生集合与显示顺序变化后重新匹配
    connect(studentTablePage->sidebar(), &StudentTableSidebar::studentSearchTextChanged, searchModel, &StudentSearchModel::setFilter);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::studentSearchResultActivated, this, &MainWindow::selectStudentFromSearch);
    connect(tableModel, &QAbstractItemModel::modelReset, searchModel, &StudentSearchModel::refresh);
    connect(tableModel, &QAbstractItemModel::rowsInserted, searchModel, &StudentSearchModel::refresh);
    connect(tableModel, &QAbstractItemModel::rowsRemoved, searchModel, &StudentSearchModel::refresh);
    connect(tableModel, &QAbstractItemModel::rowsMoved, searchModel, &StudentSearchModel::refresh);
    // 表格行序变化后，小组管理页的未分组面板顺序也要跟着变
    connect(tableModel, &QAbstractItemModel::rowsMoved, groupBoardPage, &::GroupBoardPage::reload);
    connect(tableModel, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&)
            {
                if(topLeft.column() == ClassTableModel::StudentName)
                    searchModel->refresh();
            });

    // 侧栏积分记录块：新建、保存、应用模板
    connect(studentTablePage->sidebar(), &StudentTableSidebar::newScoreRecordRequested, this, &MainWindow::createScoreRecordDraft);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::saveScoreRecordRequested, this, &MainWindow::saveScoreRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::applyTemplateRequested, this, &MainWindow::applyTemplateDialog);

    // 侧栏记录与模板预览块
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordViewRequested, this, &MainWindow::selectRecordToView);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordPreviousRequested, this, &MainWindow::showPreviousRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordNextRequested, this, &MainWindow::showNextRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordPreviewCloseRequested, this, &MainWindow::hideRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::newTemplateRequested, this, &MainWindow::newTemplateFromSidebar);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateViewRequested, this, &MainWindow::selectTemplateToView);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templatePreviousRequested, this, &MainWindow::showPreviousTemplate);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateNextRequested, this, &MainWindow::showNextTemplate);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templatePreviewCloseRequested, this, &MainWindow::hideTemplate);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateNameUpdateRequested, this, &MainWindow::updateTemplateName);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateNameEditCancelled, this, &MainWindow::discardTemplateNameEdit);

    // 双击列表项：预览对应记录/模板并切回表格页
    connect(recordListPage->listView(), &QListView::doubleClicked, this,
            [this](const QModelIndex& index)
            {
                showRecord(index.row());
                navigationTabBar->setCurrentIndex(0);
                stackedWidget->setCurrentIndex(StudentTablePage);
            });
    connect(templateListPage->listView(), &QListView::doubleClicked, this,
            [this](const QModelIndex& index)
            {
                showTemplate(index.row());
                navigationTabBar->setCurrentIndex(0);
                stackedWidget->setCurrentIndex(StudentTablePage);
            });

    // 记录列表页工具按钮
    connect(recordListPage, &RecordListPage::createRequested, this, &MainWindow::newScoreRecordFromList);
    connect(recordListPage, &RecordListPage::deleteSelectedRequested, this, &MainWindow::deleteSelectedRecords);
    connect(recordListPage, &RecordListPage::clearRequested, this, &MainWindow::clearRecordsWithConfirm);

    // 模板列表页工具按钮
    connect(recordListPage, &RecordListPage::exportRequested, this, &MainWindow::exportSelectedRecords);

    connect(templateListPage, &TemplateListPage::createRequested, this, &MainWindow::newTemplateFromList);
    connect(templateListPage, &TemplateListPage::deleteSelectedRequested, this, &MainWindow::deleteSelectedTemplates);
    connect(templateListPage, &TemplateListPage::clearRequested, this, &MainWindow::clearTemplatesWithConfirm);

    // 导航栏切换页面
    connect(navigationTabBar, &QTabBar::currentChanged, this,
            [this](int index)
            {
                stackedWidget->setCurrentIndex(index + StudentTablePage);
                syncListPageSelection();
            });

    connect(manager, &ClassDataManager::recordsChanged, this, &MainWindow::updateFilePreview);
    connect(manager, &ClassDataManager::templatesChanged, this, &MainWindow::updateFilePreview);
    connect(manager, &ClassDataManager::dirtyStateChanged, this, &MainWindow::updateFilePreview);

    onFileClosed();
}

void MainWindow::createActions()
{
    /*文件菜单动作*/
    action_OpenFile = new QAction(tr("打开"), this);
    action_OpenFile->setShortcut(QKeySequence::Open);
    action_NewFile = new QAction(tr("新建"), this);
    action_NewFile->setShortcut(QKeySequence::New);
    action_SaveFile = new QAction(tr("保存"), this);
    action_SaveFile->setShortcut(QKeySequence::Save);
    action_CloseFile = new QAction(tr("关闭"), this);
    action_CloseFile->setShortcut(QKeySequence::Close);
    connect(action_OpenFile, &QAction::triggered, this, &MainWindow::openFileDialog);
    connect(action_NewFile, &QAction::triggered, this, &MainWindow::createNewFile);
    connect(action_SaveFile, &QAction::triggered, this, &MainWindow::saveFile);
    connect(action_CloseFile, &QAction::triggered, this, &MainWindow::closeFileWithConfirm);

    /*关于菜单动作*/
    action_AboutApp = new QAction(tr("关于本程序"), this);
    action_AboutQt = new QAction(tr("关于Qt"), this);
    connect(action_AboutApp, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(action_AboutQt, &QAction::triggered, this, &MainWindow::showAboutQt);
}

void MainWindow::createMenus()
{
    QMenuBar* menubar = menuBar();

    QMenu* menu_File = menubar->addMenu(tr("文件"));
    menu_File->addAction(action_OpenFile);
    menu_File->addAction(action_NewFile);
    menu_File->addAction(action_SaveFile);
    menu_File->addAction(action_CloseFile);

    QMenu* menu_About = menubar->addMenu(tr("关于"));
    menu_About->addAction(action_AboutApp);
    menu_About->addAction(action_AboutQt);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if(!confirmUnsavedChanges())
    {
        event->ignore();
        return;
    }
    event->accept();
}

/*文件操作入口*/
void MainWindow::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开班级信息文件"), QDir::currentPath(), tr("班级信息文件(*.cls)"));
    if(fileName.isEmpty())
        return;
    openFile(fileName);
}

void MainWindow::createNewFile()
{
    currentFileName = tr("新建班级");
    newFile();
    onFileLoaded();
}

void MainWindow::closeFileWithConfirm()
{
    if(confirmUnsavedChanges())
        closeFile();
}

/*学生操作入口*/
void MainWindow::addStudentsDialog()
{
    bool ok = false;
    const QString inputText =
        QInputDialog::getMultiLineText(this, tr("添加学生对话框"), tr("输入学生姓名，可用换行分隔多个学生"), QString(), &ok);
    if(!ok)
        return;

    QStringList newNames;
    const QStringList rawList = inputText.split('\n', Qt::SkipEmptyParts);
    for(const QString& rawName : rawList)
    {
        const QString studentName = rawName.simplified();
        if(!studentName.isEmpty())
            newNames.append(studentName);
    }
    if(newNames.isEmpty())
        return;
    importStudents(newNames);
}

void MainWindow::deleteStudentDialog()
{
    // 表格当前选中行的行号即学生列表中的显示序号，作为对话框的初始选中项
    const QTableView* tableView = studentTablePage->tableView();
    const QList<QModelIndex> selectedRows = tableView->selectionModel()->selectedRows();
    const int currentStudent = selectedRows.isEmpty() ? -1 : selectedRows.first().row();
    const int index = ItemSelectionDialog::selectIndex(this, tr("选择要删除的学生"), manager->getStudentList(), currentStudent);
    if(index == -1)
        return;

    // 删除学生同时会移除所有积分记录与模板中的对应分数，与其他删除操作一致先二次确认
    const QString studentName = tableModel->index(index, ClassTableModel::StudentName).data().toString();
    if(!confirmDestructiveOperation(tr("删除"), tr("学生「%1」").arg(studentName)))
        return;
    removeStudent(index, studentName);
}

/*积分记录操作入口*/
void MainWindow::createScoreRecordDraft()
{
    tableModel->newScoreRecord(QString());
    updateRecordEditingActions();
    studentTablePage->sidebar()->focusRecordNameEdit();
}

void MainWindow::newScoreRecordFromList()
{
    createScoreRecordDraft();
    navigationTabBar->setCurrentIndex(0);
    stackedWidget->setCurrentIndex(StudentTablePage);
    studentTablePage->sidebar()->focusRecordNameEdit();
}

void MainWindow::applyTemplateDialog()
{
    const auto& templates = manager->getTemplates();
    if(templates.isEmpty())
    {
        showNotification(ExInfoBar::Informational, tr("提示"), tr("当前还没有可用的积分模板"));
        return;
    }
    QStringList templateNames;
    templateNames.reserve(templates.size());
    for(const ScoreTemplate& item : templates)
        templateNames.append(item.description);

    // 初始选中当前正在查看的模板，没有查看中的模板时选最后一条
    const bool hasPreview = templatePreviewIndex >= 0 && templatePreviewIndex < templateNames.size();
    const int currentTemplate = hasPreview ? templatePreviewIndex : templateNames.size() - 1;
    const int index = ItemSelectionDialog::selectIndex(this, tr("选择要使用的积分模板"), templateNames, currentTemplate);
    if(index != -1)
        activateTemplate(index);
}

/*关于*/
void MainWindow::showAboutDialog()
{
    AboutInfoDialog* dialog = new AboutInfoDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::showAboutQt()
{
    QMessageBox::aboutQt(this);
}

/*界面状态维护*/
void MainWindow::onFileLoaded()
{
    stackedWidget->setCurrentIndex(StudentTablePage);
    navigationTabBar->setCurrentIndex(0);
    navigationTabBar->setVisible(true);

    action_OpenFile->setDisabled(true);
    action_NewFile->setDisabled(true);
    action_SaveFile->setDisabled(false);
    action_CloseFile->setDisabled(false);

    studentTablePage->sidebar()->setStudentActionsEnabled(true);

    updateRecordEditingActions();
    updateFilePreview();
}

void MainWindow::onFileClosed()
{
    stackedWidget->setCurrentIndex(WelcomePage);
    navigationTabBar->setVisible(false);

    action_OpenFile->setDisabled(false);
    action_NewFile->setDisabled(false);
    action_SaveFile->setDisabled(true);
    action_CloseFile->setDisabled(true);

    studentTablePage->sidebar()->setStudentActionsEnabled(false);

    updateRecordEditingActions();
    updateFilePreview();
}

void MainWindow::updateFilePreview()
{
    studentTablePage->sidebar()->setFileInfo(currentFileName, manager->isDirty(), manager->getStudents().size(),
                                             manager->getRecords().size(), manager->getTemplates().size());
}

void MainWindow::updateRecordEditingActions()
{
    const bool isEditing = manager->isRecordUnsaved();
    studentTablePage->sidebar()->setRecordEditingInProgress(isEditing);
    recordListPage->setNewRecordEnabled(!isEditing);
}

void MainWindow::resetTable()
{
    recordPreviewIndex = -1;
    templatePreviewIndex = -1;
    studentTablePage->sidebar()->setRecordNameText(QString());
    tableModel->resetTable();
    groupBoardPage->reload();
    updateRecordPreviewPanel();
    updateTemplatePreviewPanel();
}

void MainWindow::updateRecordPreviewPanel()
{
    const int recordCount = manager->getRecords().size();
    const bool hasPreview = recordPreviewIndex >= 0 && recordPreviewIndex < recordCount;
    QString displayText;
    QString positionText;
    if(hasPreview)
    {
        displayText = manager->getRecordDisplayName(recordPreviewIndex);
        positionText = tr("%1/%2").arg(recordPreviewIndex + 1).arg(recordCount);
    }
    studentTablePage->sidebar()->setRecordPreview(displayText, positionText, hasPreview, recordCount > 0,
                                                  hasPreview && recordPreviewIndex > 0, hasPreview && recordPreviewIndex < recordCount - 1);
}

void MainWindow::updateTemplatePreviewPanel()
{
    const int templateCount = manager->getTemplates().size();
    const bool hasPreview = templatePreviewIndex >= 0 && templatePreviewIndex < templateCount;
    QString displayText;
    QString positionText;
    if(hasPreview)
    {
        displayText = manager->getTemplateDisplayName(templatePreviewIndex);
        positionText = tr("%1/%2").arg(templatePreviewIndex + 1).arg(templateCount);
    }
    studentTablePage->sidebar()->setTemplatePreview(displayText, positionText, hasPreview, templateCount > 0,
                                                    hasPreview && templatePreviewIndex > 0,
                                                    hasPreview && templatePreviewIndex < templateCount - 1);
    if(hasPreview)
        studentTablePage->sidebar()->setTemplateNameDraft(manager->getTemplates().at(templatePreviewIndex).description);
}

void MainWindow::syncListPageSelection()
{
    // 切到列表页时选中当前正在查看的那一条，便于直接接着操作
    const auto selectRow = [](QListView* view, int row)
    {
        if(!view->model() || row < 0 || row >= view->model()->rowCount())
            return;
        const QModelIndex index = view->model()->index(row, 0);
        view->setCurrentIndex(index);
        view->scrollTo(index, QAbstractItemView::PositionAtCenter);
    };
    const int pageIndex = stackedWidget->currentIndex();
    if(pageIndex == ScoreRecordPage)
        selectRow(recordListPage->listView(), recordPreviewIndex);
    else if(pageIndex == ScoreTemplatePage)
        selectRow(templateListPage->listView(), templatePreviewIndex);
}

/*文件操作*/
void MainWindow::openFile(const QString& filePath)
{
    const ClassDataManager::OpenResult result = manager->openFile(filePath);
    if(!result.succeeded)
    {
        showNotification(ExInfoBar::Error, tr("打开文件失败"), result.message);
        return;
    }
    currentFileName = QFileInfo(filePath).fileName();
    //先重建表格的行映射再切页面，否则页面切换过程中的取数可能用到旧映射
    resetTable();
    onFileLoaded();
    //修正过的数据与磁盘上的文件不同，按未保存处理，并把修正内容告知用户
    if(!result.message.isEmpty())
        showNotification(ExInfoBar::Warning, tr("文件已自动修正"), result.message);
}

void MainWindow::newFile()
{
    manager->newFile();
    resetTable();
}

bool MainWindow::saveFile()
{
    if(manager->isRecordUnsaved())
    {
        ExContentDialog dialog(this);
        dialog.setTitle(tr("询问"));
        dialog.setContent(tr("当前有未完成编辑的积分记录，要保存并关闭它吗？"));
        dialog.setPrimaryButtonText(tr("保存并关闭"));
        dialog.setSecondaryButtonText(tr("只保存文件"));
        dialog.setCloseButtonText(tr("取消"));
        dialog.setDefaultButton(ExContentDialog::PrimaryBtn);

        const ExContentDialog::ContentDialogResult result = dialog.showDialog();
        if(result == ExContentDialog::None)
            return false;
        if(result == ExContentDialog::Primary && !saveScoreRecord())
            return false;
    }
    bool saved = false;
    if(manager->isNewFile())
    {
        const QString fileName = QFileDialog::getSaveFileName(this, tr("保存班级信息文件"), QDir::currentPath(), tr("班级信息文件(*.cls)"));
        if(fileName.isEmpty())
            return false;
        saved = manager->saveNewFile(fileName);
        if(saved)
        {
            currentFileName = QFileInfo(fileName).fileName();
            updateFilePreview();
        }
    }
    else
    {
        saved = manager->saveFile();
    }
    if(!saved)
    {
        showNotification(ExInfoBar::Error, tr("错误"), tr("无法写入文件，请检查路径是否可写"));
        return false;
    }
    showNotification(ExInfoBar::Success, tr("保存成功"), tr("班级信息已保存到 %1").arg(currentFileName));
    return true;
}

void MainWindow::closeFile()
{
    manager->closeFile();
    currentFileName.clear();
    onFileClosed();
    resetTable();
}

/*学生操作*/
void MainWindow::importStudents(const QStringList& newNames)
{
    tableModel->importStudents(newNames);
    groupBoardPage->reload();
    updateFilePreview();
    showNotification(ExInfoBar::Success, tr("添加学生"), tr("已添加%1名学生").arg(newNames.size()));
}

void MainWindow::removeStudent(int displayIndex, const QString& studentName)
{
    tableModel->removeStudent(displayIndex);
    groupBoardPage->reload();
    updateFilePreview();
    showNotification(ExInfoBar::Informational, tr("删除学生"), tr("已删除学生「%1」").arg(studentName));
}

void MainWindow::editStudentGroup(const QModelIndex& index)
{
    if(index.column() != ClassTableModel::GroupNumber || !groupEditEnabled)
        return;
    const QString studentName = tableModel->index(index.row(), ClassTableModel::StudentName).data().toString();
    const qint32 currentGroup = tableModel->groupNumberAt(index.row());
    GroupNumberDialog dialog(studentName, currentGroup == StudentInfo::NoGroup ? 0 : currentGroup, this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    tableModel->assignGroupNumber(index.row(), dialog.getGroupNumber());
}

void MainWindow::selectStudentFromSearch(int resultRow)
{
    const int studentIndex = searchModel->studentIndexAt(resultRow);
    if(studentIndex < 0)
        return;
    const int displayRow = tableModel->displayRowForStudent(studentIndex);
    if(displayRow < 0)
        return;
    QTableView* tableView = studentTablePage->tableView();
    tableView->selectRow(displayRow);
    tableView->scrollTo(tableModel->index(displayRow, ClassTableModel::StudentName), QAbstractItemView::PositionAtCenter);
}

/*积分记录操作*/
bool MainWindow::saveScoreRecord()
{
    const QString recordName = studentTablePage->sidebar()->recordNameText();
    if(!manager->verifyRecordName(recordName))
    {
        showNotification(ExInfoBar::Warning, tr("警告"), tr("描述不能为空或与之前重复"));
        return false;
    }
    manager->setRecordDraftDescription(recordName);
    tableModel->saveScoreRecord();
    studentTablePage->sidebar()->setRecordNameText(QString());
    updateRecordEditingActions();
    updateRecordPreviewPanel();
    showNotification(ExInfoBar::Success, tr("保存成功"), tr("积分记录已保存"));
    return true;
}

void MainWindow::activateTemplate(int index)
{
    if(index != -1)
        tableModel->activateTemplate(index);
}

void MainWindow::exportScoreSum(const QVector<int>& recordIndices)
{
    if(recordIndices.isEmpty())
        return;
    SumExportDialog dialog(this, manager->getSettings());
    if(dialog.exec() != QDialog::Accepted)
        return;

    const int studentCount = manager->getStudents().count();
    QVector<ScoreSumReport::SummaryData> dataList;
    dataList.reserve(studentCount);
    for(const StudentInfo& stu : manager->getStudents())
        dataList.append({stu.name, stu.displayOrder, 0, stu.groupNumber, stu.groupOrder});

    for(int recordIndex : recordIndices)
        for(int i = 0; i < studentCount; i++)
            dataList[i].totalScore += manager->recordAt(recordIndex).scores.at(i);

    QString fileName = QFileDialog::getSaveFileName(this, tr("保存生成的文档"), QDir::currentPath(), tr("HTML Document (*.html)"));
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly))
    {
        showNotification(ExInfoBar::Error, tr("错误"), tr("无法写入文件"));
        return;
    }
    SortSettings curMode = dialog.getCurrentConfig();
    QTextStream stream(&file);
    // 显式指定 UTF-8 编码，避免中文按本地代码页写出
    stream.setCodec("UTF-8");
    stream << ScoreSumReport::generateHtmlString(dataList, curMode);
    file.close();
    if(file.error() != QFile::NoError || stream.status() != QTextStream::Ok)
    {
        showNotification(ExInfoBar::Error, tr("错误"), tr("无法写入文件，请检查磁盘空间与路径"));
        return;
    }
    manager->setPreviousSettings(curMode);
}

void MainWindow::exportSelectedRecords()
{
    const QList<QModelIndex> selectedRows = recordListPage->selectedRows();
    if(selectedRows.isEmpty())
        return;
    QVector<int> recordIndices;
    recordIndices.reserve(selectedRows.size());
    for(const QModelIndex& selectedRow : selectedRows)
        recordIndices.append(selectedRow.row());
    std::sort(recordIndices.begin(), recordIndices.end());
    exportScoreSum(recordIndices);
}

void MainWindow::selectRecordToView()
{
    const auto& records = manager->getRecords();
    if(records.isEmpty())
        return;

    QStringList recordNames;
    recordNames.reserve(records.size());
    for(int i = 0; i < records.size(); ++i)
        recordNames.append(manager->getRecordDisplayName(i));

    const int index = ItemSelectionDialog::selectIndex(this, tr("选择要查看的积分记录"), recordNames, recordPreviewIndex);
    if(index != -1)
        showRecord(index);
}

void MainWindow::showRecord(int index)
{
    if(index < 0 || index >= manager->getRecords().size())
        return;
    recordPreviewIndex = index;
    tableModel->setRecordPreviewIndex(index);
    updateRecordPreviewPanel();
}

void MainWindow::showPreviousRecord()
{
    if(recordPreviewIndex > 0)
        showRecord(recordPreviewIndex - 1);
}

void MainWindow::showNextRecord()
{
    if(recordPreviewIndex >= 0 && recordPreviewIndex < manager->getRecords().size() - 1)
        showRecord(recordPreviewIndex + 1);
}

void MainWindow::hideRecord()
{
    recordPreviewIndex = -1;
    tableModel->setRecordPreviewIndex(-1);
    updateRecordPreviewPanel();
}

void MainWindow::deleteSelectedRecords()
{
    const QList<QModelIndex> selectedRows = recordListPage->selectedRows();
    if(selectedRows.isEmpty())
        return;
    QVector<int> indices;
    indices.reserve(selectedRows.size());
    for(const QModelIndex& selectedRow : selectedRows)
        indices.append(selectedRow.row());

    if(!confirmDestructiveOperation(tr("删除"), tr("选中的%1条积分记录").arg(indices.size())))
        return;

    if(recordPreviewIndex >= 0)
    {
        if(indices.contains(recordPreviewIndex))
        {
            recordPreviewIndex = -1;
        }
        else
        {
            int removedBefore = 0;
            for(int row : indices)
                if(row < recordPreviewIndex)
                    ++removedBefore;
            recordPreviewIndex -= removedBefore;
        }
    }
    std::sort(indices.begin(), indices.end(), std::greater<int>());
    for(int row : indices)
        manager->removeRecord(row);

    tableModel->setRecordPreviewIndex(recordPreviewIndex);
    updateRecordPreviewPanel();
    showNotification(ExInfoBar::Informational, tr("删除积分记录"), tr("已删除%1条积分记录").arg(indices.size()));
}

void MainWindow::clearRecordsWithConfirm()
{
    if(confirmDestructiveOperation(tr("清空"), tr("全部%1条积分记录").arg(manager->getRecords().size())))
        clearRecords();
}

void MainWindow::clearRecords()
{
    const int recordCount = manager->getRecords().size();
    manager->clearRecords();
    recordPreviewIndex = -1;
    tableModel->setRecordPreviewIndex(-1);
    updateRecordPreviewPanel();
    showNotification(ExInfoBar::Informational, tr("清空积分记录"), tr("已清空%1条积分记录").arg(recordCount));
}

/*积分模板操作*/
void MainWindow::newTemplate()
{
    ScoreTemplate newTemplate;
    newTemplate.description = tr("未命名-%1").arg(QDateTime::currentDateTime().toString("yy/MM/dd hh:mm:ss:zzz"));
    newTemplate.values = QVector<qint32>(manager->getStudents().count());
    manager->addTemplate(newTemplate);
    showTemplate(manager->getTemplates().count() - 1);
}

void MainWindow::newTemplateFromList()
{
    newTemplate();
    navigationTabBar->setCurrentIndex(0);
    stackedWidget->setCurrentIndex(StudentTablePage);
    studentTablePage->sidebar()->focusTemplateNameEdit();
}

void MainWindow::newTemplateFromSidebar()
{
    newTemplate();
    studentTablePage->sidebar()->focusTemplateNameEdit();
}

void MainWindow::selectTemplateToView()
{
    const auto& templates = manager->getTemplates();
    if(templates.isEmpty())
        return;

    QStringList templateNames;
    templateNames.reserve(templates.size());
    for(int i = 0; i < templates.size(); ++i)
        templateNames.append(manager->getTemplateDisplayName(i));

    const int index = ItemSelectionDialog::selectIndex(this, tr("选择要查看的积分模板"), templateNames, templatePreviewIndex);
    if(index != -1)
        showTemplate(index);
}

void MainWindow::showTemplate(int index)
{
    if(index < 0 || index >= manager->getTemplates().size())
        return;
    templatePreviewIndex = index;
    tableModel->setTemplatePreviewIndex(index);
    updateTemplatePreviewPanel();
}

void MainWindow::showPreviousTemplate()
{
    if(templatePreviewIndex > 0)
        showTemplate(templatePreviewIndex - 1);
}

void MainWindow::showNextTemplate()
{
    if(templatePreviewIndex >= 0 && templatePreviewIndex < manager->getTemplates().size() - 1)
        showTemplate(templatePreviewIndex + 1);
}

void MainWindow::updateTemplateName(const QString& newDescription)
{
    if(templatePreviewIndex < 0 || templatePreviewIndex >= manager->getTemplates().size())
        return;
    const QString currentName = manager->getTemplates().at(templatePreviewIndex).description;
    if(newDescription == currentName)
    {
        studentTablePage->sidebar()->setTemplateNameDraft(currentName);
        return;
    }
    if(!manager->verifyTemplateName(newDescription))
    {
        showNotification(ExInfoBar::Warning, tr("警告"), tr("描述不能为空或与之前重复"));
        studentTablePage->sidebar()->setTemplateNameDraft(currentName);
        return;
    }
    manager->setTemplateDescription(templatePreviewIndex, newDescription);
    updateTemplatePreviewPanel();
}

void MainWindow::discardTemplateNameEdit()
{
    if(templatePreviewIndex < 0 || templatePreviewIndex >= manager->getTemplates().size())
        return;
    studentTablePage->sidebar()->setTemplateNameDraft(manager->getTemplates().at(templatePreviewIndex).description);
}

void MainWindow::hideTemplate()
{
    templatePreviewIndex = -1;
    tableModel->setTemplatePreviewIndex(-1);
    updateTemplatePreviewPanel();
}

void MainWindow::deleteSelectedTemplates()
{
    const QList<QModelIndex> selectedRows = templateListPage->selectedRows();
    if(selectedRows.isEmpty())
        return;
    QVector<int> indices;
    indices.reserve(selectedRows.size());
    for(const QModelIndex& selectedRow : selectedRows)
        indices.append(selectedRow.row());

    if(!confirmDestructiveOperation(tr("删除"), tr("选中的%1个积分模板").arg(indices.size())))
        return;

    if(templatePreviewIndex >= 0)
    {
        if(indices.contains(templatePreviewIndex))
        {
            templatePreviewIndex = -1;
        }
        else
        {
            int removedBefore = 0;
            for(int row : indices)
                if(row < templatePreviewIndex)
                    ++removedBefore;
            templatePreviewIndex -= removedBefore;
        }
    }
    std::sort(indices.begin(), indices.end(), std::greater<int>());
    for(int row : indices)
        manager->removeTemplate(row);

    tableModel->setTemplatePreviewIndex(templatePreviewIndex);
    updateTemplatePreviewPanel();
    showNotification(ExInfoBar::Informational, tr("删除积分模板"), tr("已删除%1个积分模板").arg(indices.size()));
}

void MainWindow::clearTemplatesWithConfirm()
{
    if(confirmDestructiveOperation(tr("清空"), tr("全部%1个积分模板").arg(manager->getTemplates().size())))
        clearTemplates();
}

void MainWindow::clearTemplates()
{
    const int templateCount = manager->getTemplates().size();
    manager->clearTemplates();
    templatePreviewIndex = -1;
    tableModel->setTemplatePreviewIndex(-1);
    updateTemplatePreviewPanel();
    showNotification(ExInfoBar::Informational, tr("清空积分模板"), tr("已清空%1个积分模板").arg(templateCount));
}

/*通用确认：返回是否可以继续后续操作*/
bool MainWindow::confirmUnsavedChanges()
{
    if(!manager->hasData() || !manager->isDirty() || manager->isEmptyNewFile())
        return true;

    ExContentDialog dialog(this);
    dialog.setTitle(tr("询问"));
    dialog.setContent(tr("要保存更改吗？"));
    dialog.setPrimaryButtonText(tr("保存"));
    dialog.setSecondaryButtonText(tr("不保存"));
    dialog.setCloseButtonText(tr("取消"));
    dialog.setDefaultButton(ExContentDialog::PrimaryBtn);

    const ExContentDialog::ContentDialogResult result = dialog.showDialog();
    if(result == ExContentDialog::None)
        return false;
    if(result == ExContentDialog::Primary)
    {
        if(manager->isRecordUnsaved() && !saveScoreRecord())
            return false;
        if(!saveFile())
            return false;
    }
    return true;
}

bool MainWindow::confirmDestructiveOperation(const QString& action, const QString& description)
{
    ExContentDialog dialog(this);
    dialog.setTitle(tr("警告"));
    dialog.setContent(tr("确定要%1%2吗？\n此操作无法恢复。").arg(action, description));
    dialog.setPrimaryButtonText(action);
    dialog.setCloseButtonText(tr("取消"));
    dialog.setDefaultButton(ExContentDialog::CloseBtn);
    return dialog.showDialog() == ExContentDialog::Primary;
}

/*提示通知*/
void MainWindow::showNotification(ExInfoBar::Severity severity, const QString& title, const QString& message)
{
    ExInfoBarHost::defaultHost()->showInfoBar(severity, title, message, ExInfoBarHost::BottomRight, 3000);
}
