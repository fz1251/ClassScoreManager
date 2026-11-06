#include "MainWindow.h"

#include "../core/ClassDataManager.h"
#include "../core/FileIoService.h"
#include "../core/ScoreSumReport.h"
#include "../delegates/ScoreExprDelegate.h"
#include "../delegates/TemplateSpinBoxDelegate.h"
#include "../models/ClassTableModel.h"
#include "../models/RecordListModel.h"
#include "../models/SearchListModel.h"
#include "../models/TemplateListModel.h"
#include "../pages/GroupBoardPage.h"
#include "../pages/RecordListPage.h"
#include "../pages/StudentTablePage.h"
#include "../pages/StudentTableSidebar.h"
#include "../pages/TemplateListPage.h"
#include "../pages/WelcomePage.h"
#include "../widgets/SearchPane.h"
#include "../widgets/ScoreTableView.h"
#include "AboutInfoDialog.h"
#include "GroupNumberDialog.h"
#include "ItemSelectionDialog.h"
#include "SumExportDialog.h"
#include "excontentdialog.h"
#include "exinfobarhost.h"
#include "exprogressring.h"
#include "fluentui3styleproperties.h"

#include <QAbstractItemView>
#include <QAction>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QKeySequence>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStringList>
#include <QTabBar>
#include <QTableView>
#include <QTimer>
#include <algorithm>

namespace
{
//文件操作超过该时间仍未完成时才显示常驻提示，避免快速操作一闪而过
constexpr int kFileNoticeDelayMs = 150;

//提示条内容区约 46 逻辑像素，扣除上下内边距后实测 32 逻辑像素的进度环观感最佳
constexpr int kBusyRingSide = 32;

//提示条中的不确定进度环：包一层固定尺寸控件，避免 QProgressBar 默认的 120×120 sizeHint 撑高提示条
class BusyRing final : public QWidget
{
public:
    explicit BusyRing(QWidget* parent = nullptr)
        : QWidget(parent)
    {
        auto* ring = new ExProgressRing(this);
        ring->setRange(0, 0);
        ring->setTextVisible(false);
        ring->setFixedSize(kBusyRingSide, kBusyRingSide);
        ring->setProperty(ProgressBarThicknessProperty, 3.0);
        setFixedSize(kBusyRingSide, kBusyRingSide);
    }

    QSize sizeHint() const override
    {
        return QSize(kBusyRingSide, kBusyRingSide);
    }
};

// 列表页搜索结果定位：清空原多选，选中目标行并放到视口中央
void selectListRow(QListView* view, int row)
{
    if(!view || !view->model() || row < 0 || row >= view->model()->rowCount())
        return;
    const QModelIndex index = view->model()->index(row, 0);
    view->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    view->scrollTo(index, QAbstractItemView::PositionAtCenter);
}
} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , fileIoService(new FileIoService(this))
    , manager(new ClassDataManager(this))
    , tableModel(new ClassTableModel(manager, this))
{
    resize(960, 640);
    setMinimumSize(900, 600);
    setWindowTitle(tr("班级积分管理系统"));

    // 中央区域：左侧导航栏、搜索窗格，右侧五个页面由堆叠窗口承载
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

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
    mainLayout->addWidget(navigationTabBar);

    // 搜索窗格与页面堆叠是唯一可拖动的分栏；窗格默认关闭
    contentSplitter = new QSplitter(Qt::Horizontal, centralWidget);
    contentSplitter->setChildrenCollapsible(false);
    searchPane = new SearchPane(contentSplitter);
    stackedWidget = new QStackedWidget(contentSplitter);
    contentSplitter->addWidget(searchPane);
    contentSplitter->addWidget(stackedWidget);
    contentSplitter->setStretchFactor(0, 0);
    contentSplitter->setStretchFactor(1, 1);
    mainLayout->addWidget(contentSplitter, 1);
    searchPane->hide();

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

    // 搜索模型：一个实例承载学生/记录/模板三种范围，窗格负责展示
    searchModel = new SearchListModel(manager, this);
    searchPane->setModel(searchModel);

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

    // 侧栏学生操作块：添加、删除、搜索学生
    connect(studentTablePage->sidebar(), &StudentTableSidebar::addStudentsRequested, this, &MainWindow::addStudentsDialog);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::deleteStudentRequested, this, &MainWindow::deleteStudentDialog);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::searchStudentsRequested, this, &MainWindow::openSearchPane);

    // 搜索窗格：范围由当前页决定，结果按范围跳转到表格、小组卡片或列表
    connect(searchPane, &SearchPane::searchTextChanged, searchModel, &SearchListModel::setFilter);
    connect(searchPane, &SearchPane::resultActivated, this, &MainWindow::activateSearchResult);
    connect(searchPane, &SearchPane::closeRequested, this,
            [this]()
            {
                setSearchPaneVisible(false);
            });
    connect(stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::updateSearchScope);

    // Ctrl+F 在任意页面打开搜索窗格；窗格已打开时只聚焦搜索框
    auto* searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, this, &MainWindow::openSearchPane);

    // 学生集合、显示顺序与姓名变化后重新匹配；只在学生范围刷新
    connect(tableModel, &QAbstractItemModel::modelReset, this,
            [this]()
            {
                if(searchModel->scope() == SearchScope::Student)
                    searchModel->refresh();
            });
    connect(tableModel, &QAbstractItemModel::rowsInserted, this,
            [this]()
            {
                if(searchModel->scope() == SearchScope::Student)
                    searchModel->refresh();
            });
    connect(tableModel, &QAbstractItemModel::rowsRemoved, this,
            [this]()
            {
                if(searchModel->scope() == SearchScope::Student)
                    searchModel->refresh();
            });
    connect(tableModel, &QAbstractItemModel::rowsMoved, this,
            [this]()
            {
                if(searchModel->scope() == SearchScope::Student)
                    searchModel->refresh();
            });
    // 表格行序变化后，小组管理页的未分组面板顺序也要跟着变
    connect(tableModel, &QAbstractItemModel::rowsMoved, groupBoardPage, &::GroupBoardPage::reload);
    connect(tableModel, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex& topLeft, const QModelIndex&, const QVector<int>&)
            {
                if(topLeft.column() == ClassTableModel::StudentName && searchModel->scope() == SearchScope::Student)
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
    connect(recordListPage, &RecordListPage::searchRequested, this, &MainWindow::openSearchPane);
    connect(recordListPage, &RecordListPage::deleteSelectedRequested, this, &MainWindow::deleteSelectedRecords);
    connect(recordListPage, &RecordListPage::clearRequested, this, &MainWindow::clearRecordsWithConfirm);

    // 模板列表页工具按钮
    connect(recordListPage, &RecordListPage::exportRequested, this, &MainWindow::exportSelectedRecords);

    connect(templateListPage, &TemplateListPage::createRequested, this, &MainWindow::newTemplateFromList);
    connect(templateListPage, &TemplateListPage::searchRequested, this, &MainWindow::openSearchPane);
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

    // 文件 I/O 在独立线程执行，结果回到 GUI 线程后再提交数据与界面
    connect(fileIoService, &FileIoService::readFinished, this, &MainWindow::onFileReadFinished);
    connect(fileIoService, &FileIoService::writeFinished, this, &MainWindow::onFileWriteFinished);

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
    if(isFileTaskBusy())
    {
        event->ignore();
        return;
    }
    if(!hasUnsavedChanges())
    {
        event->accept();
        return;
    }

    switch(askSaveChanges())
    {
        case SaveChoice::Discard:
            event->accept();
            return;
        case SaveChoice::Cancel:
            event->ignore();
            return;
        case SaveChoice::Save:
            event->ignore();
            startSave(AfterSave::CloseWindow);
            return;
    }
}

/*文件操作入口*/
void MainWindow::openFileDialog()
{
    if(isFileTaskBusy())
        return;

    QString fileName = QFileDialog::getOpenFileName(this, tr("打开班级信息文件"), QDir::currentPath(), tr("班级信息文件(*.cls)"));
    if(fileName.isEmpty())
        return;
    openFile(fileName);
}

void MainWindow::createNewFile()
{
    if(isFileTaskBusy())
        return;

    currentFileName = tr("新建班级");
    newFile();
    onFileLoaded();
}

void MainWindow::closeFileWithConfirm()
{
    if(isFileTaskBusy())
        return;
    if(!hasUnsavedChanges())
    {
        closeFile();
        return;
    }

    switch(askSaveChanges())
    {
        case SaveChoice::Discard:
            closeFile();
            return;
        case SaveChoice::Cancel:
            return;
        case SaveChoice::Save:
            startSave(AfterSave::CloseFile);
            return;
    }
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
    setSearchPaneVisible(false);
    searchPane->clearSearch();

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
    searchModel->refresh();
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

void MainWindow::setSearchPaneVisible(bool visible)
{
    // 没有任何文件或停在欢迎页时窗格不能显示
    const bool canShow = manager->hasData() && stackedWidget->currentIndex() != WelcomePage;
    if(visible && !canShow)
        visible = false;

    if(searchPane->isVisible() != visible)
    {
        if(visible)
        {
            searchPane->show();
            const int totalWidth = contentSplitter->width();
            const int paneWidth = qBound(searchPane->minimumWidth(), searchPaneWidth, searchPane->maximumWidth());
            contentSplitter->setSizes({paneWidth, qMax(0, totalWidth - paneWidth)});
            updateSearchScope();
            searchPane->focusSearch();
        }
        else
        {
            const QList<int> sizes = contentSplitter->sizes();
            if(!sizes.isEmpty() && sizes.at(0) > 0)
                searchPaneWidth = sizes.at(0);
            searchPane->hide();
        }
    }
}

void MainWindow::openSearchPane()
{
    // 已经打开时只把焦点交给搜索框，保留当前查询
    const bool wasVisible = searchPane->isVisible();
    setSearchPaneVisible(true);
    if(wasVisible)
        searchPane->focusSearch();
}

void MainWindow::updateSearchScope()
{
    const int pageIndex = stackedWidget->currentIndex();
    if(pageIndex == WelcomePage)
    {
        setSearchPaneVisible(false);
        return;
    }
    if(!searchPane->isVisible())
        return;

    SearchScope scope = SearchScope::Student;
    switch(pageIndex)
    {
        case StudentTablePage:
        case GroupBoardPage:
            scope = SearchScope::Student;
            break;
        case ScoreRecordPage:
            scope = SearchScope::Record;
            break;
        case ScoreTemplatePage:
            scope = SearchScope::Template;
            break;
        default:
            return;
    }

    if(searchModel->scope() != scope)
    {
        searchPane->clearSearch();
        searchModel->setScope(scope);
    }

    switch(scope)
    {
        case SearchScope::Student:
            searchPane->setSearchContext(tr("搜索学生"), tr("暂无学生"), tr("未找到匹配的学生"));
            break;
        case SearchScope::Record:
            searchPane->setSearchContext(tr("搜索积分记录"), tr("暂无记录"), tr("未找到匹配的记录"));
            break;
        case SearchScope::Template:
            searchPane->setSearchContext(tr("搜索积分模板"), tr("暂无模板"), tr("未找到匹配的模板"));
            break;
    }
}

/*文件操作*/
void MainWindow::openFile(const QString& filePath)
{
    if(isFileTaskBusy())
        return;
    if(!beginFileTask(FileTask::OpenClass, filePath))
        return;
    if(!fileIoService->readAll(filePath))
        finishFileTask();
}

void MainWindow::newFile()
{
    manager->newFile();
    resetTable();
}

bool MainWindow::saveFile()
{
    return startSave(AfterSave::None);
}

bool MainWindow::startSave(AfterSave after)
{
    if(isFileTaskBusy())
        return false;

    if(manager->isRecordUnsaved())
    {
        ExContentDialog dialog(this);
        dialog.setTitle(tr("询问"));
        dialog.setContent(tr("当前有未完成编辑的积分记录，要保存并关闭它吗？"));
        dialog.setPrimaryButtonText(tr("保存并关闭"));
        if(after == AfterSave::None)
            dialog.setSecondaryButtonText(tr("只保存文件"));
        dialog.setCloseButtonText(tr("取消"));
        dialog.setDefaultButton(ExContentDialog::PrimaryBtn);

        const ExContentDialog::ContentDialogResult result = dialog.showDialog();
        if(result == ExContentDialog::None)
            return false;
        if(result == ExContentDialog::Primary && !saveScoreRecord())
            return false;
    }

    QString filePath;
    if(manager->isNewFile())
    {
        filePath = QFileDialog::getSaveFileName(this, tr("保存班级信息文件"), QDir::currentPath(), tr("班级信息文件(*.cls)"));
        if(filePath.isEmpty())
            return false;
    }
    else
    {
        filePath = manager->currentFilePath();
        if(filePath.isEmpty())
        {
            showNotification(ExInfoBar::Error, tr("错误"), tr("当前文件路径为空，无法保存"));
            return false;
        }
    }

    QByteArray snapshot;
    if(!manager->serializeTo(snapshot))
    {
        showNotification(ExInfoBar::Error, tr("错误"), tr("无法生成文件数据"));
        return false;
    }

    if(!beginFileTask(FileTask::SaveClass, filePath))
        return false;

    pendingSaveSnapshot = snapshot;
    afterSave = after;
    if(!fileIoService->writeAll(filePath, snapshot))
    {
        pendingSaveSnapshot.clear();
        afterSave = AfterSave::None;
        finishFileTask();
        return false;
    }
    return true;
}

void MainWindow::closeFile()
{
    if(isFileTaskBusy())
        return;
    manager->closeFile();
    currentFileName.clear();
    onFileClosed();
    resetTable();
}

/*文件任务与提示*/
bool MainWindow::isFileTaskBusy() const
{
    return fileTask != FileTask::None;
}

bool MainWindow::beginFileTask(FileTask task, const QString& path)
{
    if(isFileTaskBusy())
        return false;

    fileTask = task;
    taskPath = path;
    const quint64 id = ++fileTaskId;
    QTimer::singleShot(kFileNoticeDelayMs, this,
                       [this, id]
                       {
                           if(fileTask == FileTask::None || fileTaskId != id)
                               return;
                           showFileNotice();
                       });
    return true;
}

void MainWindow::finishFileTask()
{
    fileTask = FileTask::None;
    taskPath.clear();
    hideFileNotice();
}

void MainWindow::showFileNotice()
{
    if(fileNoticeBar)
        return;

    fileNoticeBar =
        ExInfoBarHost::defaultHost()->showInfoBar(ExInfoBar::Informational, fileNoticeTitle(), QString(), ExInfoBarHost::BottomRight, 0);
    if(!fileNoticeBar)
        return;
    fileNoticeBar->setClosable(false);

    //文件任务未在阈值时间内完成时，用不确定进度环表示仍在进行
    fileNoticeBar->setActionWidget(new BusyRing);
}

void MainWindow::hideFileNotice()
{
    if(!fileNoticeBar)
        return;
    fileNoticeBar->dismiss();
    fileNoticeBar = nullptr;
}

QString MainWindow::fileNoticeTitle() const
{
    switch(fileTask)
    {
        case FileTask::OpenClass:
            return tr("正在读取文件…");
        case FileTask::SaveClass:
            return tr("正在保存文件…");
        case FileTask::ExportReport:
            return tr("正在导出文件…");
        case FileTask::None:
            break;
    }
    return QString();
}

void MainWindow::onFileReadFinished(const QString& path, const QByteArray& data, const QString& error)
{
    if(fileTask != FileTask::OpenClass || path != taskPath)
        return;

    if(!error.isEmpty())
    {
        finishFileTask();
        showNotification(ExInfoBar::Error, tr("打开文件失败"), tr("无法打开名为%1的文件：%2").arg(path, error));
        return;
    }

    const ClassDataManager::OpenResult result = manager->loadFromBytes(data, path);
    finishFileTask();
    if(!result.succeeded)
    {
        showNotification(ExInfoBar::Error, tr("打开文件失败"), result.message);
        return;
    }

    currentFileName = QFileInfo(path).fileName();
    //先重建表格的行映射再切页面，否则页面切换过程中的取数可能用到旧映射
    resetTable();
    onFileLoaded();
    //修正过的数据与磁盘上的文件不同，按未保存处理，并把修正内容告知用户
    if(!result.message.isEmpty())
        showNotification(ExInfoBar::Warning, tr("文件已自动修正"), result.message);
}

void MainWindow::onFileWriteFinished(const QString& path, const QString& error)
{
    if(fileTask == FileTask::None || path != taskPath)
        return;

    if(fileTask == FileTask::SaveClass)
    {
        if(!error.isEmpty())
        {
            const QString message = error;
            pendingSaveSnapshot.clear();
            afterSave = AfterSave::None;
            finishFileTask();
            showNotification(ExInfoBar::Error, tr("保存失败"), tr("无法写入文件：%1").arg(message));
            return;
        }

        const bool fullySaved = manager->markFileSaved(path, pendingSaveSnapshot);
        currentFileName = QFileInfo(path).fileName();
        updateFilePreview();

        const AfterSave action = afterSave;
        afterSave = AfterSave::None;
        pendingSaveSnapshot.clear();
        finishFileTask();

        if(!fullySaved)
        {
            const QString message = manager->isRecordUnsaved() ? tr("当前积分记录草稿尚未保存") : tr("点击保存后的修改尚未保存");
            showNotification(ExInfoBar::Warning, tr("文件已保存"), message);
            return;
        }

        if(action == AfterSave::CloseWindow)
        {
            close();
        }
        else if(action == AfterSave::CloseFile)
        {
            closeFile();
            showNotification(ExInfoBar::Success, tr("保存成功"), tr("班级信息已保存到 %1").arg(QFileInfo(path).fileName()));
        }
        else
        {
            showNotification(ExInfoBar::Success, tr("保存成功"), tr("班级信息已保存到 %1").arg(currentFileName));
        }
        return;
    }

    if(fileTask == FileTask::ExportReport)
    {
        if(!error.isEmpty())
        {
            const QString message = error;
            finishFileTask();
            showNotification(ExInfoBar::Error, tr("导出失败"), tr("无法写入文件：%1").arg(message));
            return;
        }

        finishFileTask();
        manager->setPreviousSettings(pendingExportSettings);
        showNotification(ExInfoBar::Success, tr("导出成功"), tr("积分求和结果已保存到 %1").arg(QFileInfo(path).fileName()));
    }
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

void MainWindow::activateSearchResult(int row)
{
    const int entryIndex = searchModel->entryIndexAt(row);
    if(entryIndex < 0)
        return;

    switch(searchModel->scope())
    {
        case SearchScope::Student:
        {
            const int pageIndex = stackedWidget->currentIndex();
            if(pageIndex == GroupBoardPage)
            {
                groupBoardPage->revealStudent(entryIndex);
                break;
            }
            if(pageIndex != StudentTablePage)
                break;
            const int displayRow = tableModel->displayRowForStudent(entryIndex);
            if(displayRow < 0)
                break;
            QTableView* tableView = studentTablePage->tableView();
            const QModelIndex targetIndex = tableModel->index(displayRow, ClassTableModel::StudentName);
            tableView->setCurrentIndex(targetIndex);
            tableView->selectRow(displayRow);
            tableView->scrollTo(targetIndex, QAbstractItemView::PositionAtCenter);
            break;
        }
        case SearchScope::Record:
            selectListRow(recordListPage->listView(), entryIndex);
            break;
        case SearchScope::Template:
            selectListRow(templateListPage->listView(), entryIndex);
            break;
    }
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
    if(isFileTaskBusy() || recordIndices.isEmpty())
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
    const SortSettings curMode = dialog.getCurrentConfig();
    const QByteArray html = ScoreSumReport::generateHtmlString(dataList, curMode).toUtf8();

    if(!beginFileTask(FileTask::ExportReport, fileName))
        return;

    pendingExportSettings = curMode;
    if(!fileIoService->writeAll(fileName, html))
        finishFileTask();
}

void MainWindow::exportSelectedRecords()
{
    if(isFileTaskBusy())
        return;

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

/*通用确认*/
bool MainWindow::hasUnsavedChanges() const
{
    return manager->hasData() && manager->isDirty() && !manager->isEmptyNewFile();
}

MainWindow::SaveChoice MainWindow::askSaveChanges()
{
    ExContentDialog dialog(this);
    dialog.setTitle(tr("询问"));
    dialog.setContent(tr("要保存更改吗？"));
    dialog.setPrimaryButtonText(tr("保存"));
    dialog.setSecondaryButtonText(tr("不保存"));
    dialog.setCloseButtonText(tr("取消"));
    dialog.setDefaultButton(ExContentDialog::PrimaryBtn);

    const ExContentDialog::ContentDialogResult result = dialog.showDialog();
    if(result == ExContentDialog::Primary)
        return SaveChoice::Save;
    if(result == ExContentDialog::Secondary)
        return SaveChoice::Discard;
    return SaveChoice::Cancel;
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
