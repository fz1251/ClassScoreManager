#include "MainWindow.h"
#include "../core/ClassDataManager.h"
#include "../core/ScoreSumReport.h"
#include "../delegates/GroupSpinBoxDelegate.h"
#include "../delegates/ScoreExprDelegate.h"
#include "../delegates/TemplateSpinBoxDelegate.h"
#include "../models/ClassTableModel.h"
#include "../models/RecordListModel.h"
#include "../models/TemplateListModel.h"
#include "../pages/RecordListPage.h"
#include "../pages/StudentTablePage.h"
#include "../pages/StudentTableSidebar.h"
#include "../pages/TemplateListPage.h"
#include "../pages/WelcomePage.h"
#include "AboutInfoDialog.h"
#include "ItemSelectionDialog.h"
#include "SumExportDialog.h"
#include "fluentui3styleproperties.h"
#include <QAbstractItemView>
#include <QAction>
#include <algorithm>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabBar>
#include <QTableView>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , manager(new ClassDataManager(this))
    , tableModel(new ClassTableModel(manager, this))
{
    resize(800, 600);
    setWindowTitle(tr("班级积分管理系统"));

    // 中央区域：导航栏在左，四个页面由堆叠窗口承载
    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget = new QStackedWidget(centralWidget);
    mainLayout->addWidget(stackedWidget);

    auto *welcomePage = new ::WelcomePage(stackedWidget);
    studentTablePage = new ::StudentTablePage(stackedWidget);
    recordListPage = new RecordListPage(stackedWidget);
    templateListPage = new TemplateListPage(stackedWidget);
    stackedWidget->addWidget(welcomePage);
    stackedWidget->addWidget(studentTablePage);
    stackedWidget->addWidget(recordListPage);
    stackedWidget->addWidget(templateListPage);

    // 导航栏：文件打开后显示，切换表格、积分记录、积分模板页
    navigationTabBar = new QTabBar(centralWidget);
    navigationTabBar->setShape(QTabBar::RoundedWest);
    navigationTabBar->setDrawBase(false);
    navigationTabBar->setExpanding(false);
    navigationTabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    navigationTabBar->setProperty(TabBarStyleProperty, TabBarStyle::Navigation);
    navigationTabBar->addTab(tr("班级信息"));
    navigationTabBar->addTab(tr("积分记录"));
    navigationTabBar->addTab(tr("积分模板"));
    mainLayout->insertWidget(0, navigationTabBar);

    setCentralWidget(centralWidget);
    statusBar()->setVisible(false);

    createActions();
    createMenus();

    // 列表模型只由视图持有，MainWindow 后续不直接访问
    auto *recordModel = new RecordListModel(manager, this);
    auto *templateModel = new TemplateListModel(manager, this);
    auto *tableView = studentTablePage->tableView();
    tableView->setModel(tableModel);
    recordListPage->setListModel(recordModel);
    templateListPage->setListModel(templateModel);

    // 表格单元格委托
    tableView->setItemDelegateForColumn(ClassTableModel::CurrentRecord, new ScoreExprDelegate(this));
    tableView->setItemDelegateForColumn(ClassTableModel::GroupNumber, new GroupSpinBoxDelegate(this));
    tableView->setItemDelegateForColumn(ClassTableModel::ScoreTemplatePreview, new TemplateSpinBoxDelegate(this));

    // 表格模型与视图的列显隐联动
    connect(tableModel, &ClassTableModel::showColumnRequested, tableView, &QTableView::showColumn);
    connect(tableModel, &ClassTableModel::hideColumnRequested, tableView, &QTableView::hideColumn);

    // 欢迎页卡片请求
    connect(welcomePage, &::WelcomePage::openFileRequested, this, &MainWindow::openFileDialog);
    connect(welcomePage, &::WelcomePage::newFileRequested, this, &MainWindow::createNewFile);

    // 学生表页开关面板
    connect(studentTablePage->sidebar(), &StudentTableSidebar::dragSortChanged, this, [tableView](bool enabled) {
        if (enabled)
        {
            tableView->setDragDropMode(QAbstractItemView::InternalMove);
            tableView->setDefaultDropAction(Qt::MoveAction);
        }
        else
        {
            tableView->setDragDropMode(QAbstractItemView::NoDragDrop);
        }
    });
    connect(studentTablePage->sidebar(), &StudentTableSidebar::nameEditChanged,
            tableModel, &ClassTableModel::setStudentEditable);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::groupEditChanged,
            tableModel, &ClassTableModel::setGroupEditable);

    // 侧栏积分记录块：新建、保存、应用模板
    connect(studentTablePage->sidebar(), &StudentTableSidebar::newScoreRecordRequested,
            this, &MainWindow::createScoreRecordDraft);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::saveScoreRecordRequested,
            this, &MainWindow::saveScoreRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::applyTemplateRequested,
            this, &MainWindow::applyTemplateDialog);

    // 侧栏记录与模板预览块
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordPreviousRequested,
            this, &MainWindow::showPreviousRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordNextRequested,
            this, &MainWindow::showNextRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::recordPreviewCloseRequested,
            this, &MainWindow::hideRecord);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::newTemplateRequested,
            this, &MainWindow::newTemplateFromSidebar);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templatePreviousRequested,
            this, &MainWindow::showPreviousTemplate);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateNextRequested,
            this, &MainWindow::showNextTemplate);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templatePreviewCloseRequested,
            this, &MainWindow::hideTemplate);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateNameUpdateRequested,
            this, &MainWindow::updateTemplateName);
    connect(studentTablePage->sidebar(), &StudentTableSidebar::templateNameEditCancelled,
            this, &MainWindow::discardTemplateNameEdit);

    // 双击列表项：预览对应记录/模板并切回表格页
    connect(recordListPage->listView(), &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        showRecord(index.row());
        navigationTabBar->setCurrentIndex(0);
        stackedWidget->setCurrentIndex(StudentTablePage);
    });
    connect(templateListPage->listView(), &QListView::doubleClicked, this, [this](const QModelIndex &index) {
        showTemplate(index.row());
        navigationTabBar->setCurrentIndex(0);
        stackedWidget->setCurrentIndex(StudentTablePage);
    });

    // 导航栏切换页面
    // 记录列表页工具按钮
    connect(recordListPage, &RecordListPage::createRequested,
            this, &MainWindow::newScoreRecordFromList);
    connect(recordListPage, &RecordListPage::deleteSelectedRequested,
            this, &MainWindow::deleteSelectedRecords);
    connect(recordListPage, &RecordListPage::clearRequested,
            this, &MainWindow::clearRecordsWithConfirm);

    // 模板列表页工具按钮
    connect(recordListPage, &RecordListPage::exportRequested,
            this, &MainWindow::exportSelectedRecords);

    connect(templateListPage, &TemplateListPage::createRequested,
            this, &MainWindow::newTemplateFromList);
    connect(templateListPage, &TemplateListPage::deleteSelectedRequested,
            this, &MainWindow::deleteSelectedTemplates);
    connect(templateListPage, &TemplateListPage::clearRequested,
            this, &MainWindow::clearTemplatesWithConfirm);

    connect(navigationTabBar, &QTabBar::currentChanged, this, [this](int index) {
        stackedWidget->setCurrentIndex(index + StudentTablePage);
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
    action_NewFile = new QAction(tr("新建"), this);
    action_SaveFile = new QAction(tr("保存"), this);
    action_CloseFile = new QAction(tr("关闭"), this);
    action_ExitApp = new QAction(tr("退出程序"), this);
    connect(action_OpenFile, &QAction::triggered, this, &MainWindow::openFileDialog);
    connect(action_NewFile, &QAction::triggered, this, &MainWindow::createNewFile);
    connect(action_SaveFile, &QAction::triggered, this, &MainWindow::saveFile);
    connect(action_CloseFile, &QAction::triggered, this, &MainWindow::closeFileWithConfirm);
    connect(action_ExitApp, &QAction::triggered, this, &MainWindow::close);

    /*学生菜单动作*/
    action_AddStudent = new QAction(tr("添加学生"), this);
    action_DeleteStudent = new QAction(tr("删除一个学生"), this);
    action_ClearStudents = new QAction(tr("清空班级信息"), this);
    connect(action_AddStudent, &QAction::triggered, this, &MainWindow::addStudentsDialog);
    connect(action_DeleteStudent, &QAction::triggered, this, &MainWindow::deleteStudentDialog);
    connect(action_ClearStudents, &QAction::triggered, this, &MainWindow::clearStudentsWithConfirm);

    /*关于菜单动作*/
    action_AboutApp = new QAction(tr("关于本程序"), this);
    action_AboutQt = new QAction(tr("关于Qt"), this);
    connect(action_AboutApp, &QAction::triggered, this, &MainWindow::showAboutDialog);
    connect(action_AboutQt, &QAction::triggered, this, &MainWindow::showAboutQt);
}

void MainWindow::createMenus()
{
    QMenuBar *menubar = menuBar();

    QMenu *menu_File = menubar->addMenu(tr("文件"));
    menu_File->addAction(action_OpenFile);
    menu_File->addAction(action_NewFile);
    menu_File->addAction(action_SaveFile);
    menu_File->addAction(action_CloseFile);
    menu_File->addSeparator();
    menu_File->addAction(action_ExitApp);

    QMenu *menu_Student = menubar->addMenu(tr("学生"));
    menu_Student->addAction(action_AddStudent);
    menu_Student->addAction(action_DeleteStudent);
    menu_Student->addAction(action_ClearStudents);

    QMenu *menu_About = menubar->addMenu(tr("关于"));
    menu_About->addAction(action_AboutApp);
    menu_About->addAction(action_AboutQt);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
    else
        event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData=event->mimeData();
    if(mimeData->hasUrls())
    {
        QString fileName=mimeData->urls().front().toLocalFile();
        if(fileName.isEmpty())
            return ;
        openFile(fileName);
        event->accept();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(manager->hasData()&&manager->isDirty())
    {
        if(confirmUnsavedChanges()==QMessageBox::RejectRole)
        {
            event->ignore();
            return ;
        }
    }
    event->accept();
}

/*文件操作入口*/
void MainWindow::openFileDialog()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开班级信息文件"),
                                                    QDir::currentPath(), tr("班级信息文件(*.cls)"));
    if(fileName.isEmpty())
        return;
    openFile(fileName);
}

void MainWindow::createNewFile()
{
    currentFileName = tr("新建班级（未保存）");
    newFile();
    onFileLoaded();
}

void MainWindow::closeFileWithConfirm()
{
    if(manager->hasData() && manager->isDirty())
        if(confirmUnsavedChanges() == QMessageBox::RejectRole)
            return;
    closeFile();
}

/*学生操作入口*/
void MainWindow::addStudentsDialog()
{
    bool ok = false;
    const QString inputText = QInputDialog::getMultiLineText(
        this, tr("添加学生对话框"), tr("输入学生姓名，可用换行分隔多个学生"), QString(), &ok);
    if(!ok)
        return;

    QStringList newNames;
    const QStringList rawList = inputText.split('\n', Qt::SkipEmptyParts);
    for(const QString &rawName : rawList)
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
    ItemSelectionDialog dialog(this, tr("删除学生对话框"), tr("选择您要删除的学生"),
                               manager->getStudentList());
    if(dialog.exec() == QDialog::Accepted)
    {
        int index = dialog.selectedIndex();
        if(index != -1)
            removeStudent(index);
    }
}

void MainWindow::clearStudentsWithConfirm()
{
    if(confirmClearOperation(tr("学生(共%1名),积分模板(共%2个)与积分记录(共%3条)")
                             .arg(manager->getStudents().size())
                             .arg(manager->getTemplates().size())
                             .arg(manager->getRecords().size())))
    {
        clearStudents();
    }
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
    QStringList templateList = manager->getTemplateList();
    ItemSelectionDialog dialog(this, tr("选择积分模板对话框"), tr("选择您要使用的积分模板"),
                               templateList, templateList.size() - 1);
    if(dialog.exec() == QDialog::Accepted)
    {
        int index = dialog.selectedIndex();
        if(index != -1)
            activateTemplate(index);
    }
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

    action_AddStudent->setDisabled(false);
    action_DeleteStudent->setDisabled(false);
    action_ClearStudents->setDisabled(false);

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

    action_AddStudent->setDisabled(true);
    action_DeleteStudent->setDisabled(true);
    action_ClearStudents->setDisabled(true);

    updateRecordEditingActions();
    updateFilePreview();
}

void MainWindow::updateFilePreview()
{
    studentTablePage->sidebar()->setFileInfo(
        currentFileName, manager->isDirty(), manager->getStudents().size(),
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
        displayText = manager->getRecordList().at(recordPreviewIndex);
        positionText = tr("%1/%2").arg(recordPreviewIndex + 1).arg(recordCount);
    }
    studentTablePage->sidebar()->setRecordPreview(displayText, positionText, hasPreview,
                                                   hasPreview && recordPreviewIndex > 0,
                                                   hasPreview && recordPreviewIndex < recordCount - 1);
}

void MainWindow::updateTemplatePreviewPanel()
{
    const int templateCount = manager->getTemplates().size();
    const bool hasPreview = templatePreviewIndex >= 0 && templatePreviewIndex < templateCount;
    QString displayText;
    QString positionText;
    if(hasPreview)
    {
        displayText = manager->getTemplates().at(templatePreviewIndex).description;
        positionText = tr("%1/%2").arg(templatePreviewIndex + 1).arg(templateCount);
    }
    studentTablePage->sidebar()->setTemplatePreview(displayText, positionText, hasPreview,
                                                     hasPreview && templatePreviewIndex > 0,
                                                     hasPreview && templatePreviewIndex < templateCount - 1);
    if(hasPreview)
        studentTablePage->sidebar()->setTemplateNameDraft(
            manager->getTemplates().at(templatePreviewIndex).description);
}

/*文件操作*/
void MainWindow::openFile(const QString &filePath)
{
    QString errMsg=manager->openFile(filePath);
    if(!errMsg.isEmpty())
    {
        errMsg.prepend(tr("在解析班级信息文件时遇到错误，以下为错误信息：\n"));
        QMessageBox::critical(this,tr("错误"),errMsg);
        return ;
    }
    currentFileName = QFileInfo(filePath).fileName();
    onFileLoaded();
    resetTable();
}

void MainWindow::newFile()
{
    manager->newFile();
    resetTable();
}

void MainWindow::saveFile()
{
    if(manager->isRecordUnsaved())
    {
        QMessageBox msgBox(QMessageBox::Question,tr("询问"),tr("当前有未完成编辑的积分记录，要保存并关闭它吗？"));
        QPushButton* saveRecordBtn=msgBox.addButton(tr("保存并关闭记录"),QMessageBox::AcceptRole);
        QPushButton* saveOthersBtn=msgBox.addButton(tr("不关闭记录，保存其他"),QMessageBox::DestructiveRole);
        QPushButton* cancelBtn    =msgBox.addButton(tr("取消操作"),QMessageBox::RejectRole);
        msgBox.setDefaultButton(saveRecordBtn);
        msgBox.setEscapeButton(cancelBtn);
        msgBox.exec();
        if(msgBox.clickedButton()==saveRecordBtn)
        {
            if(!saveScoreRecord())
                return;
        }
        Q_UNUSED(saveOthersBtn);
    }
    if(manager->isNewFile())
    {
        QString fileName=QFileDialog::getSaveFileName(this,tr("保存班级信息文件"),QDir::currentPath(),tr("班级信息文件(*.cls)"));
        if(fileName.isEmpty())
            return ;
        manager->saveNewFile(fileName);
        currentFileName = QFileInfo(fileName).fileName();
        updateFilePreview();
    }
    else
    {
        manager->saveFile();
    }
}

void MainWindow::closeFile()
{
    manager->closeFile();
    currentFileName.clear();
    onFileClosed();
    resetTable();
}

/*学生操作*/
void MainWindow::importStudents(const QStringList &newNames)
{
    tableModel->importStudents(newNames);
    updateFilePreview();
}

void MainWindow::removeStudent(int displayIndex)
{
    tableModel->removeStudent(displayIndex);
    updateFilePreview();
}

void MainWindow::clearStudents()
{
    recordPreviewIndex = -1;
    templatePreviewIndex = -1;
    tableModel->clearStudents();
    updateRecordEditingActions();
    updateRecordPreviewPanel();
    updateTemplatePreviewPanel();
    updateFilePreview();
}

/*积分记录操作*/
bool MainWindow::saveScoreRecord()
{
    const QString recordName = studentTablePage->sidebar()->recordNameText();
    if(!manager->verifyRecordName(recordName))
    {
        QMessageBox::warning(this, tr("警告"), tr("描述不能为空或与之前重复"));
        return false;
    }
    manager->setRecordDraftDescription(recordName);
    tableModel->saveScoreRecord();
    studentTablePage->sidebar()->setRecordNameText(QString());
    updateRecordEditingActions();
    return true;
}

void MainWindow::activateTemplate(int index)
{
    if(index!=-1)
        tableModel->activateTemplate(index);
}

void MainWindow::exportScoreSum(const QVector<int> &recordIndices)
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
        dataList.append({stu.name, stu.displayOrder, 0, stu.groupNumber});

    for(int recordIndex : recordIndices)
        for(int i = 0; i < studentCount; i++)
            dataList[i].totalScore += manager->recordAt(recordIndex).scores.at(i);

    QString fileName = QFileDialog::getSaveFileName(
                this, tr("保存生成的文档"), QDir::currentPath(), tr("HTML Document (*.html)"));
    if(fileName.isEmpty())
        return;
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this, tr("警告"), tr("无法写入文件"));
        return;
    }
    SortSettings curMode = dialog.getCurrentConfig();
    QTextStream stream(&file);
#if QT_VERSION_MAJOR == 5
    // Qt5 下需显式指定UTF-8编码
    stream.setCodec("UTF-8");
#endif
    stream << ScoreSumReport::generateHtmlString(dataList, curMode);
    file.close();
    manager->setPreviousSettings(curMode);
}

void MainWindow::exportSelectedRecords()
{
    const QList<QModelIndex> selectedRows = recordListPage->selectedRows();
    if(selectedRows.isEmpty())
        return;
    QVector<int> recordIndices;
    recordIndices.reserve(selectedRows.size());
    for(const QModelIndex &selectedRow : selectedRows)
        recordIndices.append(selectedRow.row());
    std::sort(recordIndices.begin(), recordIndices.end());
    exportScoreSum(recordIndices);
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
    for(const QModelIndex &selectedRow : selectedRows)
        indices.append(selectedRow.row());

    if(!confirmDeleteOperation(tr("选中的%1条积分记录").arg(indices.size())))
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
}

void MainWindow::clearRecordsWithConfirm()
{
    if(confirmClearOperation(tr("积分记录(共%1条)").arg(manager->getRecords().size())))
        clearRecords();
}

void MainWindow::clearRecords()
{

    manager->clearRecords();
    recordPreviewIndex = -1;
    tableModel->setRecordPreviewIndex(-1);
    updateRecordPreviewPanel();
}

/*积分模板操作*/
void MainWindow::newTemplate()
{
    ScoreTemplate newTemplate;
    newTemplate.description=tr("未命名-%1").arg(QDateTime::currentDateTime()
                                                .toString("yy/MM/dd hh:mm:ss:zzz"));
    newTemplate.values=QVector<qint32>(manager->getStudents().count());
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

void MainWindow::updateTemplateName(const QString &newDescription)
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
        QMessageBox::warning(this, tr("警告"), tr("描述不能为空或与之前重复"));
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
    studentTablePage->sidebar()->setTemplateNameDraft(
        manager->getTemplates().at(templatePreviewIndex).description);
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
    for(const QModelIndex &selectedRow : selectedRows)
        indices.append(selectedRow.row());

    if(!confirmDeleteOperation(tr("选中的%1个积分模板").arg(indices.size())))
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
}

void MainWindow::clearTemplatesWithConfirm()
{
    if(confirmClearOperation(tr("积分模板(共%1个)").arg(manager->getTemplates().size())))
        clearTemplates();
}

void MainWindow::clearTemplates()
{
    manager->clearTemplates();
    templatePreviewIndex = -1;
    tableModel->setTemplatePreviewIndex(-1);
    updateTemplatePreviewPanel();
}

/*通用确认*/
QMessageBox::ButtonRole MainWindow::confirmUnsavedChanges()
{
    QMessageBox msgBox(QMessageBox::Question,tr("询问"),tr("要保存更改吗？"));
    QPushButton* saveBtn    =msgBox.addButton(tr("保存(&Y)"),QMessageBox::AcceptRole);
    QPushButton* dontSaveBtn=msgBox.addButton(tr("不保存(&N)"),QMessageBox::DestructiveRole);
    QPushButton* cancelBtn  =msgBox.addButton(tr("取消"),QMessageBox::RejectRole);
    msgBox.setDefaultButton(saveBtn);
    msgBox.setEscapeButton(cancelBtn);
    msgBox.exec();
    QPushButton* clickedBtn=static_cast<QPushButton*>(msgBox.clickedButton());
    if(clickedBtn==saveBtn)
    {
        if(manager->isRecordUnsaved())
            if(!saveScoreRecord())
                return QMessageBox::RejectRole;
        saveFile();
        closeFile();
        return QMessageBox::AcceptRole;
    }
    else if(clickedBtn==dontSaveBtn)
    {
        closeFile();
        return QMessageBox::DestructiveRole;
    }
    else
    {
        return QMessageBox::RejectRole;
    }
}

bool MainWindow::confirmClearOperation(const QString &description)
{
    QMessageBox msgBox(QMessageBox::Warning,tr("警告"),tr("确定要清空%1吗？").arg(description));
    QPushButton* confirmBtn=msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
    QPushButton* cancelBtn=msgBox.addButton(tr("取消"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(cancelBtn);
    msgBox.setEscapeButton(cancelBtn);
    msgBox.exec();
    return msgBox.clickedButton()==confirmBtn;
}

bool MainWindow::confirmDeleteOperation(const QString &description)
{
    QMessageBox msgBox(QMessageBox::Warning, tr("警告"), tr("确定要删除%1吗？").arg(description));
    QPushButton* confirmBtn = msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
    QPushButton* cancelBtn = msgBox.addButton(tr("取消"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(cancelBtn);
    msgBox.setEscapeButton(cancelBtn);
    msgBox.exec();
    return msgBox.clickedButton()==confirmBtn;
}
