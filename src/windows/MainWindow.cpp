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
#include "../pages/TemplateListPage.h"
#include "../pages/WelcomePage.h"
#include "AboutInfoDialog.h"
#include "ItemSelectionDialog.h"
#include "PasteStudentsDialog.h"
#include "SumExportDialog.h"
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
#include <QLineEdit>
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
    auto *studentTablePage = new ::StudentTablePage(stackedWidget);
    auto *recordListPage = new RecordListPage(stackedWidget);
    auto *templateListPage = new TemplateListPage(stackedWidget);
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
    recordListPage->listView()->setModel(recordModel);
    templateListPage->listView()->setModel(templateModel);

    // 表格单元格委托
    tableView->setItemDelegateForColumn(ClassTableModel::CurrentRecord, new ScoreExprDelegate(this));
    tableView->setItemDelegateForColumn(ClassTableModel::GroupNumber, new GroupSpinBoxDelegate(this));
    tableView->setItemDelegateForColumn(ClassTableModel::ScoreTemplatePreview, new TemplateSpinBoxDelegate(this));

    // 表格模型与视图的列显隐联动
    connect(tableModel, &ClassTableModel::showColumnRequested, tableView, &QTableView::showColumn);
    connect(tableModel, &ClassTableModel::hideColumnRequested, tableView, &QTableView::hideColumn);

    // 欢迎页卡片请求
    connect(welcomePage, &::WelcomePage::openFileRequested, this, &MainWindow::on_action_OpenFile_triggered);
    connect(welcomePage, &::WelcomePage::newFileRequested, this, &MainWindow::on_action_NewFile_triggered);

    // 学生表页开关面板
    connect(studentTablePage, &::StudentTablePage::dragSortChanged, this, [tableView](bool enabled) {
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
    connect(studentTablePage, &::StudentTablePage::nameEditChanged, tableModel, &ClassTableModel::setStudentEditable);
    connect(studentTablePage, &::StudentTablePage::groupEditChanged, tableModel, &ClassTableModel::setGroupEditable);

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
    connect(navigationTabBar, &QTabBar::currentChanged, this, [this](int index) {
        stackedWidget->setCurrentIndex(index + StudentTablePage);
    });

    connect(manager, &ClassDataManager::dirtyStateChanged, this, &MainWindow::setWindowModified);

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
    connect(action_OpenFile, &QAction::triggered, this, &MainWindow::on_action_OpenFile_triggered);
    connect(action_NewFile, &QAction::triggered, this, &MainWindow::on_action_NewFile_triggered);
    connect(action_SaveFile, &QAction::triggered, this, &MainWindow::on_action_SaveFile_triggered);
    connect(action_CloseFile, &QAction::triggered, this, &MainWindow::on_action_CloseFile_triggered);
    connect(action_ExitApp, &QAction::triggered, this, &MainWindow::on_action_ExitApp_triggered);

    /*学生菜单动作*/
    action_AddStudent = new QAction(tr("添加学生"), this);
    action_ImportStudents = new QAction(tr("批量导入学生"), this);
    action_DeleteStudent = new QAction(tr("删除一个学生"), this);
    action_ClearStudents = new QAction(tr("清空班级信息"), this);
    connect(action_AddStudent, &QAction::triggered, this, &MainWindow::on_action_AddStudent_triggered);
    connect(action_ImportStudents, &QAction::triggered, this, &MainWindow::on_action_ImportStudents_triggered);
    connect(action_DeleteStudent, &QAction::triggered, this, &MainWindow::on_action_DeleteStudent_triggered);
    connect(action_ClearStudents, &QAction::triggered, this, &MainWindow::on_action_ClearStudents_triggered);

    /*积分菜单动作*/
    action_NewScoreRecord = new QAction(tr("新建积分记录"), this);
    action_SaveScoreRecord = new QAction(tr("保存积分记录"), this);
    action_UseScoreTemplate = new QAction(tr("应用积分模板"), this);
    action_ExportScoreSum = new QAction(tr("积分求和统计"), this);
    action_ShowRecordHistory = new QAction(tr("查看历史记录"), this);
    action_HideRecordHistory = new QAction(tr("关闭历史记录预览"), this);
    action_DeleteRecordHistory = new QAction(tr("删除一项历史记录"), this);
    action_ClearRecords = new QAction(tr("清空积分记录"), this);
    connect(action_NewScoreRecord, &QAction::triggered, this, &MainWindow::on_action_NewScoreRecord_triggered);
    connect(action_SaveScoreRecord, &QAction::triggered, this, &MainWindow::on_action_SaveScoreRecord_triggered);
    connect(action_UseScoreTemplate, &QAction::triggered, this, &MainWindow::on_action_UseScoreTemplate_triggered);
    connect(action_ExportScoreSum, &QAction::triggered, this, &MainWindow::on_action_ExportScoreSum_triggered);
    connect(action_ShowRecordHistory, &QAction::triggered, this, &MainWindow::on_action_ShowRecordHistory_triggered);
    connect(action_HideRecordHistory, &QAction::triggered, this, &MainWindow::on_action_HideRecordHistory_triggered);
    connect(action_DeleteRecordHistory, &QAction::triggered, this, &MainWindow::on_action_DeleteRecordHistory_triggered);
    connect(action_ClearRecords, &QAction::triggered, this, &MainWindow::on_action_ClearRecords_triggered);

    /*积分模板菜单动作*/
    action_NewTemplate = new QAction(tr("新建积分模板"), this);
    action_EditTemplate = new QAction(tr("更改积分模板描述"), this);
    action_ShowTemplate = new QAction(tr("查看积分模板"), this);
    action_HideTemplate = new QAction(tr("关闭积分模板预览"), this);
    action_DeleteTemplate = new QAction(tr("删除一个积分模板"), this);
    action_ClearTemplates = new QAction(tr("清空积分模板"), this);
    connect(action_NewTemplate, &QAction::triggered, this, &MainWindow::on_action_NewTemplate_triggered);
    connect(action_EditTemplate, &QAction::triggered, this, &MainWindow::on_action_EditTemplate_triggered);
    connect(action_ShowTemplate, &QAction::triggered, this, &MainWindow::on_action_ShowTemplate_triggered);
    connect(action_HideTemplate, &QAction::triggered, this, &MainWindow::on_action_HideTemplate_triggered);
    connect(action_DeleteTemplate, &QAction::triggered, this, &MainWindow::on_action_DeleteTemplate_triggered);
    connect(action_ClearTemplates, &QAction::triggered, this, &MainWindow::on_action_ClearTemplates_triggered);

    /*关于菜单动作*/
    action_AboutApp = new QAction(tr("关于本程序"), this);
    action_AboutQt = new QAction(tr("关于Qt"), this);
    connect(action_AboutApp, &QAction::triggered, this, &MainWindow::on_action_AboutApp_triggered);
    connect(action_AboutQt, &QAction::triggered, this, &MainWindow::on_action_AboutQt_triggered);
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
    menu_Student->addAction(action_ImportStudents);
    menu_Student->addAction(action_DeleteStudent);
    menu_Student->addAction(action_ClearStudents);

    QMenu *menu_Score = menubar->addMenu(tr("积分"));
    menu_Score->addAction(action_NewScoreRecord);
    menu_Score->addAction(action_UseScoreTemplate);
    menu_Score->addAction(action_SaveScoreRecord);
    menu_Score->addAction(action_ExportScoreSum);
    menu_Score->addSeparator();
    menu_Score->addAction(action_ShowRecordHistory);
    menu_Score->addAction(action_HideRecordHistory);
    menu_Score->addAction(action_DeleteRecordHistory);
    menu_Score->addAction(action_ClearRecords);

    QMenu *menu_ScoreTemplate = menubar->addMenu(tr("积分模板"));
    menu_ScoreTemplate->addAction(action_NewTemplate);
    menu_ScoreTemplate->addAction(action_EditTemplate);
    menu_ScoreTemplate->addAction(action_ShowTemplate);
    menu_ScoreTemplate->addAction(action_HideTemplate);
    menu_ScoreTemplate->addAction(action_DeleteTemplate);
    menu_ScoreTemplate->addAction(action_ClearTemplates);

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

/*文件菜单(menu_File)功能实现*/
void MainWindow::on_action_OpenFile_triggered()
{
    QString fileName=QFileDialog::getOpenFileName(this,tr("打开班级信息文件"),
                                                  QDir::currentPath(),tr("班级信息文件(*.cls)"));
    if(fileName.isEmpty())
        return ;
    openFile(fileName);
}

void MainWindow::on_action_NewFile_triggered()
{
    newFile();
    onFileLoaded();
    updateWindowTitle(tr("新建班级（未保存）"));
}

void MainWindow::on_action_SaveFile_triggered()
{
    saveFile();
}

void MainWindow::on_action_CloseFile_triggered()
{
    if(manager->hasData()&&manager->isDirty())
        if(confirmUnsavedChanges()==QMessageBox::RejectRole)
            return ;
    closeFile();
}

void MainWindow::on_action_ExitApp_triggered()
{
    this->close();
}

/*学生菜单(menu_Student)功能实现*/
void MainWindow::on_action_AddStudent_triggered()
{
    bool ok;
    QString studentName=QInputDialog::getText(this,tr("添加学生对话框"),tr("输入新的学生姓名"),
                                              QLineEdit::Normal,QString(),&ok);
    if(ok)
        addStudent(studentName);
}

void MainWindow::on_action_ImportStudents_triggered()
{
    PasteStudentsDialog dialog;
    if(dialog.exec()==QDialog::Accepted)
    {
        importStudents(dialog.getNameList());
    }
}

void MainWindow::on_action_DeleteStudent_triggered()
{
    ItemSelectionDialog dialog(this,tr("删除学生对话框"),tr("选择您要删除的学生"),manager->getStudentList());
    if(dialog.exec()==QDialog::Accepted)
    {
        int index=dialog.selectedIndex();
        if(index!=-1)
        {
            removeStudent(index);
        }
    }
}

void MainWindow::on_action_ClearStudents_triggered()
{
    if(confirmClearOperation(tr("学生(共%1名),积分模板(共%2个)与积分记录(共%3条)")
                             .arg(manager->getStudents().size())
                             .arg(manager->getTemplates().size())
                             .arg(manager->getRecords().size())))
    {
        clearStudents();
    }
}

/*积分菜单(menu_Score)功能实现*/
void MainWindow::on_action_NewScoreRecord_triggered()
{
    bool ok;
    QString descriptionText=QInputDialog::getText(this,tr("新建积分记录对话框"),tr("输入新的积分记录名"),
                                                  QLineEdit::Normal,QString(),&ok);
    if(ok)
    {
        if(!newScoreRecord(descriptionText))
        {
            QMessageBox::warning(this,tr("警告"),tr("描述不能为空或与之前重复"));
            return ;
        }

        updateRecordEditingActions();
    }
}

void MainWindow::on_action_SaveScoreRecord_triggered()
{
    saveScoreRecord();
    updateRecordEditingActions();
}

void MainWindow::on_action_UseScoreTemplate_triggered()
{
    QStringList templateList=manager->getTemplateList();
    ItemSelectionDialog dialog(this,tr("选择积分模板对话框"),tr("选择您要使用的积分模板"),
                               templateList,templateList.size()-1);
    if(dialog.exec()==QDialog::Accepted)
    {
        int index=dialog.selectedIndex();
        if(index!=-1)
        {
            activateTemplate(index);
        }
    }
}

void MainWindow::on_action_ExportScoreSum_triggered()
{
    exportScoreSum();
}

void MainWindow::on_action_ShowRecordHistory_triggered()
{
    QStringList recordList=manager->getRecordList();
    ItemSelectionDialog dialog(this,tr("选择积分记录对话框"),tr("选择您要查看的积分记录"),recordList,recordList.size()-1);
    if(dialog.exec()==QDialog::Accepted)
        showRecord(dialog.selectedIndex());
}

void MainWindow::on_action_HideRecordHistory_triggered()
{
    hideRecord();
}

void MainWindow::on_action_DeleteRecordHistory_triggered()
{
    QStringList recordList=manager->getRecordList();
    ItemSelectionDialog dialog(this,tr("选择积分记录对话框"),tr("选择您要删除的积分记录"),
                                    recordList,recordPreviewIndex);
    if(dialog.exec()==QDialog::Accepted)
    {
        int index=dialog.selectedIndex();
        if(index!=-1)
            deleteRecord(index);
    }
}

void MainWindow::on_action_ClearRecords_triggered()
{
    if(confirmClearOperation(tr("积分记录(共%1条)").arg(manager->getRecords().size())))
    {
        clearRecords();
    }
}

/*积分模板菜单(menu_ScoreTemplate)功能实现*/
void MainWindow::on_action_NewTemplate_triggered()
{
    newTemplate();
}

void MainWindow::on_action_EditTemplate_triggered()
{
    bool ok;
    QString descriptionText=QInputDialog::getText(this,tr("修改积分模板对话框"),tr("输入新的积分模板名"),
                            QLineEdit::Normal,
                            manager->getTemplates().at(templatePreviewIndex).description,&ok);
    if(ok)
    {
        if(!renameTemplate(descriptionText))
            QMessageBox::warning(this,tr("警告"),tr("描述不能为空或与之前重复"));
    }
}

void MainWindow::on_action_ShowTemplate_triggered()
{
    QStringList templateList=manager->getTemplateList();
    ItemSelectionDialog dialog(this,tr("选择积分模板对话框"),tr("选择您要查看的积分模板"),
                                    templateList,templateList.size()-1);
    if(dialog.exec()==QDialog::Accepted)
        showTemplate(dialog.selectedIndex());
}

void MainWindow::on_action_HideTemplate_triggered()
{
    hideTemplate();
}

void MainWindow::on_action_DeleteTemplate_triggered()
{
    QStringList templateList=manager->getTemplateList();
    ItemSelectionDialog dialog(this,tr("选择积分模板对话框"),tr("选择您要查看的积分模板"),
                                    templateList,templatePreviewIndex);
    if(dialog.exec()==QDialog::Accepted)
    {
        int index=dialog.selectedIndex();
        if(index!=-1)
            deleteTemplate(index);
    }
}

void MainWindow::on_action_ClearTemplates_triggered()
{
    if(confirmClearOperation(tr("积分模板(共%1个)").arg(manager->getTemplates().size())))
    {
        clearTemplates();
    }
}

/*关于菜单(menu_About)功能实现*/
void MainWindow::on_action_AboutApp_triggered()
{
    AboutInfoDialog* dialog=new AboutInfoDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::on_action_AboutQt_triggered()
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
    action_ImportStudents->setDisabled(false);
    action_DeleteStudent->setDisabled(false);
    action_ClearStudents->setDisabled(false);

    updateRecordEditingActions();
    action_ExportScoreSum->setDisabled(false);
    action_ShowRecordHistory->setDisabled(false);
    action_DeleteRecordHistory->setDisabled(false);
    action_ClearRecords->setDisabled(false);

    action_NewTemplate->setDisabled(false);
    action_ShowTemplate->setDisabled(false);
    action_DeleteTemplate->setDisabled(false);
    action_ClearTemplates->setDisabled(false);
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
    action_ImportStudents->setDisabled(true);
    action_DeleteStudent->setDisabled(true);
    action_ClearStudents->setDisabled(true);

    updateRecordEditingActions();
    action_ExportScoreSum->setDisabled(true);
    action_ShowRecordHistory->setDisabled(true);
    action_HideRecordHistory->setDisabled(true);
    action_DeleteRecordHistory->setDisabled(true);
    action_ClearRecords->setDisabled(true);

    action_NewTemplate->setDisabled(true);
    action_EditTemplate->setDisabled(true);
    action_ShowTemplate->setDisabled(true);
    action_HideTemplate->setDisabled(true);
    action_DeleteTemplate->setDisabled(true);
    action_ClearTemplates->setDisabled(true);
}

void MainWindow::updateWindowTitle(const QString &fileName)
{
    if(fileName.isEmpty())
        this->setWindowTitle(tr("班级积分管理系统"));
    else
        this->setWindowTitle(tr("[*]%1 - 班级积分管理系统").arg(fileName));
}

void MainWindow::updateRecordEditingActions()
{
    const bool isEditing = manager->isRecordUnsaved();
    action_NewScoreRecord->setDisabled(isEditing);
    action_SaveScoreRecord->setDisabled(!isEditing);
    action_UseScoreTemplate->setDisabled(!isEditing);
}

void MainWindow::resetTable()
{
    recordPreviewIndex = -1;
    templatePreviewIndex = -1;
    tableModel->resetTable();
    updateRecordPreviewActions();
    updateTemplatePreviewActions();
}

void MainWindow::updateRecordPreviewActions()
{
    action_HideRecordHistory->setDisabled(recordPreviewIndex == -1);
}

void MainWindow::updateTemplatePreviewActions()
{
    const bool hasPreview = templatePreviewIndex != -1;
    action_HideTemplate->setDisabled(!hasPreview);
    action_EditTemplate->setDisabled(!hasPreview);
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
    onFileLoaded();
    updateWindowTitle(QFileInfo(filePath).fileName());

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
            saveScoreRecord();
        }
        Q_UNUSED(saveOthersBtn);
    }
    if(manager->isNewFile())
    {
        QString fileName=QFileDialog::getSaveFileName(this,tr("保存班级信息文件"),QDir::currentPath(),tr("班级信息文件(*.cls)"));
        if(fileName.isEmpty())
            return ;
        manager->saveNewFile(fileName);
    }
    else
    {
        manager->saveFile();
    }
}

void MainWindow::closeFile()
{
    manager->closeFile();
    onFileClosed();
    updateWindowTitle("");
    resetTable();
}

/*学生操作*/
void MainWindow::addStudent(const QString &newName)
{
    tableModel->addStudent(newName);
}

void MainWindow::importStudents(const QStringList &newNames)
{
    tableModel->importStudents(newNames);
}

void MainWindow::removeStudent(int displayIndex)
{
    tableModel->removeStudent(displayIndex);
}

void MainWindow::clearStudents()
{
    recordPreviewIndex = -1;
    templatePreviewIndex = -1;
    tableModel->clearStudents();
    updateRecordEditingActions();
    updateRecordPreviewActions();
    updateTemplatePreviewActions();
}

/*积分记录操作*/
bool MainWindow::newScoreRecord(const QString &recordName)
{
    if(!manager->verifyRecordName(recordName))
        return false;
    tableModel->newScoreRecord(recordName);
    return true;
}

void MainWindow::saveScoreRecord()
{
    tableModel->saveScoreRecord();
}

void MainWindow::activateTemplate(int index)
{
    if(index!=-1)
        tableModel->activateTemplate(index);
}

void MainWindow::exportScoreSum()
{
    SumExportDialog dialog(this,manager->getSettings());
    dialog.setRecordText(manager->getRecordList());
    if(dialog.exec()!=QDialog::Accepted)
        return ;
    const int studentCount=manager->getStudents().count();
    QVector<ScoreSumReport::SummaryData> dataList;
    dataList.reserve(studentCount);
    for(const StudentInfo& stu:manager->getStudents())
        dataList.append({stu.name,stu.displayOrder,0,stu.groupNumber});
    for(int idx=dialog.getBeginIndex();idx<=dialog.getEndIndex();idx++)
        for(int i=0;i<studentCount;i++)
            dataList[i].totalScore+=manager->recordAt(idx).scores.at(i);

    QString fileName=QFileDialog::getSaveFileName(
                this,tr("保存生成的文档"),QDir::currentPath(),tr("HTML Document (*.html)"));
    if(fileName.isEmpty())
        return ;
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly))
    {
        QMessageBox::warning(this,tr("警告"),tr("无法写入文件"));
        return ;
    }
    SortSettings curMode=dialog.getCurrentConfig();
    QTextStream stream(&file);
#if QT_VERSION_MAJOR == 5
    // Qt5 下需显式指定UTF-8编码
    stream.setCodec("UTF-8");
#endif
    stream << ScoreSumReport::generateHtmlString(dataList,curMode);
    file.close();
    manager->setPreviousSettings(curMode);
}

void MainWindow::showRecord(int index)
{
    if(index == -1)
        return;
    recordPreviewIndex = index;
    tableModel->setRecordPreviewIndex(index);
    updateRecordPreviewActions();
}

void MainWindow::hideRecord()
{
    recordPreviewIndex = -1;
    tableModel->setRecordPreviewIndex(-1);
    updateRecordPreviewActions();
}

void MainWindow::deleteRecord(int index)
{
    if(index == -1)
        return;
    manager->removeRecord(index);
    if(index == recordPreviewIndex)
        recordPreviewIndex = -1;
    else if(index < recordPreviewIndex)
        --recordPreviewIndex;
    tableModel->setRecordPreviewIndex(recordPreviewIndex);
    updateRecordPreviewActions();
}

void MainWindow::clearRecords()
{
    manager->clearRecords();
    recordPreviewIndex = -1;
    tableModel->setRecordPreviewIndex(-1);
    updateRecordPreviewActions();
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

bool MainWindow::renameTemplate(const QString &newDescription)
{
    if(templatePreviewIndex == -1)
        return false;
    if(!manager->verifyTemplateName(newDescription))
        return false;
    manager->setTemplateDescription(templatePreviewIndex, newDescription);
    return true;
}

void MainWindow::showTemplate(int index)
{
    if(index == -1)
        return;
    templatePreviewIndex = index;
    tableModel->setTemplatePreviewIndex(index);
    updateTemplatePreviewActions();
}

void MainWindow::hideTemplate()
{
    templatePreviewIndex = -1;
    tableModel->setTemplatePreviewIndex(-1);
    updateTemplatePreviewActions();
}

void MainWindow::deleteTemplate(int index)
{
    if(index == -1)
        return;
    manager->removeTemplate(index);
    if(index == templatePreviewIndex)
        templatePreviewIndex = -1;
    else if(index < templatePreviewIndex)
        --templatePreviewIndex;
    tableModel->setTemplatePreviewIndex(templatePreviewIndex);
    updateTemplatePreviewActions();
}

void MainWindow::clearTemplates()
{
    manager->clearTemplates();
    templatePreviewIndex = -1;
    tableModel->setTemplatePreviewIndex(-1);
    updateTemplatePreviewActions();
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
            saveScoreRecord();
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
