#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "AboutInfoDialog.h"
#include "ItemSelectionDialog.h"
#include "PasteStudentsDialog.h"
#include "SumExportDialog.h"
#include "../delegates/GroupSpinBoxDelegate.h"
#include "../delegates/TemplateSpinBoxDelegate.h"
#include "../delegates/ScoreExprDelegate.h"
#include "../core/ScoreSumReport.h"
#include <FluentUI3Style/fluentui3styleproperties.h>
#include <QDir>
#include <QListView>
#include <QPushButton>
#include <QSizePolicy>
#include <QTabBar>
#include <QInputDialog>
#include <QFileDialog>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , manager(new ClassDataManager(this))
    , tableModel(new ClassTableModel(manager,this))
    , recordModel(new RecordListModel(manager,this))
    , templateModel(new TemplateListModel(manager,this))
{
    ui->setupUi(this);

    ui->tableView->setModel(tableModel);
    ui->recordListView->setModel(recordModel);
    ui->templateListView->setModel(templateModel);

    // ui->statusbar->setSizeGripEnabled(false);
    ui->statusbar->setVisible(false);

    navigationTabBar = new QTabBar(ui->centralwidget);
    navigationTabBar->setObjectName(QStringLiteral("navigationTabBar"));
    navigationTabBar->setShape(QTabBar::RoundedWest);
    navigationTabBar->setDrawBase(false);
    navigationTabBar->setExpanding(false);
    navigationTabBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    navigationTabBar->setProperty(TabBarStyleProperty, TabBarStyle::Navigation);
    navigationTabBar->addTab(tr("班级信息"));
    navigationTabBar->addTab(tr("积分记录"));
    navigationTabBar->addTab(tr("积分模板"));
    ui->mainLayout->insertWidget(0, navigationTabBar);

    ui->stackedWidget->setCurrentIndex(0);

    ui->tableView->setItemDelegateForColumn(ClassTableModel::CurrentRecord,new ScoreExprDelegate(this));
    ui->tableView->setItemDelegateForColumn(ClassTableModel::GroupNumber,new GroupSpinBoxDelegate(this));
    ui->tableView->setItemDelegateForColumn(ClassTableModel::ScoreTemplatePreview,new TemplateSpinBoxDelegate(this));

    onFileClosed();

    //设置表格右侧操作面板
    QWidget* secondPage = ui->stackedWidget->widget(StudentTablePage);
    ui->rightPanelLayout->addStretch();
    QSplitter* splitter = new QSplitter(secondPage);
    splitter->addWidget(ui->tableView);
    splitter->addWidget(ui->rightPanelWidget);
    QVBoxLayout* centralLayout = new QVBoxLayout(secondPage);
    centralLayout->addWidget(splitter);
    centralLayout->setContentsMargins({0,0,0,0});
    secondPage->setLayout(centralLayout);


    ui->dragStudentSwitch->setCheckState(Qt::Checked);
    ui->editNameSwitch->setCheckState(Qt::Checked);
    ui->editGroupSwitch->setCheckState(Qt::Checked);


    connect(manager,&ClassDataManager::dirtyStateChanged,this,&MainWindow::setWindowModified);
    connect(tableModel,&ClassTableModel::showColumnRequested,ui->tableView,&QTableView::showColumn);
    connect(tableModel,&ClassTableModel::hideColumnRequested,ui->tableView,&QTableView::hideColumn);

    connect(navigationTabBar,&QTabBar::currentChanged,this,[this](int index){
        ui->stackedWidget->setCurrentIndex(index + StudentTablePage);
    });

    ui->dragStudentSwitch->setProperty(SwitchStyleProperty, true);
    ui->editNameSwitch->setProperty(SwitchStyleProperty, true);
    ui->editGroupSwitch->setProperty(SwitchStyleProperty, true);
    connect(ui->dragStudentSwitch,&QCheckBox::stateChanged,this,[this](int state){
        bool checked=state==Qt::Checked;
        if(checked)
        {
            ui->tableView->setDragDropMode(QAbstractItemView::InternalMove);
            ui->tableView->setDefaultDropAction(Qt::MoveAction);
        }
        else
        {
            ui->tableView->setDragDropMode(QAbstractItemView::NoDragDrop);
        }
    });
    connect(ui->editNameSwitch,&QCheckBox::stateChanged,this,[this](int state){
        bool checked=(state==Qt::Checked);
        tableModel->setStudentEditable(checked);
    });
    connect(ui->editGroupSwitch,&QCheckBox::stateChanged,this,[this](int state){
        bool checked=(state==Qt::Checked);
        tableModel->setGroupEditable(checked);
    });

    //双击跳转：The signal is only emitted when the index is valid.
    connect(ui->recordListView,&QListView::doubleClicked,this,[this](const QModelIndex &index){
        showRecord(index.row());
        navigationTabBar->setCurrentIndex(0);
        ui->stackedWidget->setCurrentIndex(StudentTablePage);
    });
    connect(ui->templateListView,&QListView::doubleClicked,this,[this](const QModelIndex &index){
        showTemplate(index.row());
        navigationTabBar->setCurrentIndex(0);
        ui->stackedWidget->setCurrentIndex(StudentTablePage);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
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

        ui->action_NewScoreRecord->setDisabled(true);
        ui->action_SaveScoreRecord->setDisabled(false);
        ui->action_UseScoreTemplate->setDisabled(false);
    }
}

void MainWindow::on_action_SaveScoreRecord_triggered()
{
    saveScoreRecord();
    ui->action_NewScoreRecord->setDisabled(false);
    ui->action_SaveScoreRecord->setDisabled(true);
    ui->action_UseScoreTemplate->setDisabled(true);
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
                                    recordList,tableModel->getCurRecordIndex());
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
                            manager->getTemplates().at(tableModel->getCurTemplateIndex()).description,&ok);
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
                                    templateList,tableModel->getCurTemplateIndex());
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
    AboutInfoDialog* dialog=new AboutInfoDialog();
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
    ui->stackedWidget->setCurrentIndex(StudentTablePage);
    navigationTabBar->setCurrentIndex(0);
    navigationTabBar->setVisible(true);

    ui->action_OpenFile->setDisabled(true);
    ui->action_NewFile->setDisabled(true);
    ui->action_SaveFile->setDisabled(false);
    ui->action_CloseFile->setDisabled(false);

    ui->action_AddStudent->setDisabled(false);
    ui->action_ImportStudents->setDisabled(false);
    ui->action_DeleteStudent->setDisabled(false);
    ui->action_ClearStudents->setDisabled(false);

    ui->action_NewScoreRecord->setDisabled(false);
    ui->action_ExportScoreSum->setDisabled(false);
    ui->action_ShowRecordHistory->setDisabled(false);
    ui->action_DeleteRecordHistory->setDisabled(false);
    ui->action_ClearRecords->setDisabled(false);

    ui->action_NewTemplate->setDisabled(false);
    ui->action_ShowTemplate->setDisabled(false);
    ui->action_DeleteTemplate->setDisabled(false);
    ui->action_ClearTemplates->setDisabled(false);
}

void MainWindow::onFileClosed()
{
    ui->stackedWidget->setCurrentIndex(WelcomePage);
    navigationTabBar->setVisible(false);

    ui->action_OpenFile->setDisabled(false);
    ui->action_NewFile->setDisabled(false);
    ui->action_SaveFile->setDisabled(true);
    ui->action_CloseFile->setDisabled(true);

    ui->action_AddStudent->setDisabled(true);
    ui->action_ImportStudents->setDisabled(true);
    ui->action_DeleteStudent->setDisabled(true);
    ui->action_ClearStudents->setDisabled(true);

    ui->action_NewScoreRecord->setDisabled(true);
    ui->action_UseScoreTemplate->setDisabled(true);
    ui->action_SaveScoreRecord->setDisabled(true);
    ui->action_ExportScoreSum->setDisabled(true);
    ui->action_ShowRecordHistory->setDisabled(true);
    ui->action_HideRecordHistory->setDisabled(true);
    ui->action_DeleteRecordHistory->setDisabled(true);
    ui->action_ClearRecords->setDisabled(true);

    ui->action_NewTemplate->setDisabled(true);
    ui->action_EditTemplate->setDisabled(true);
    ui->action_ShowTemplate->setDisabled(true);
    ui->action_HideTemplate->setDisabled(true);
    ui->action_DeleteTemplate->setDisabled(true);
    ui->action_ClearTemplates->setDisabled(true);
}

void MainWindow::updateWindowTitle(const QString &fileName)
{
    if(fileName.isEmpty())
        this->setWindowTitle(tr("班级积分管理系统"));
    else
        this->setWindowTitle(tr("[*]%1 - 班级积分管理系统").arg(fileName));
}

void MainWindow::resetTable()
{
    tableModel->resetTable();
    updateRecordPreviewActions();
    updateTemplatePreviewActions();
}

void MainWindow::updateRecordPreviewActions()
{
    ui->action_HideRecordHistory->setDisabled(tableModel->getCurRecordIndex()==-1);
}

void MainWindow::updateTemplatePreviewActions()
{
    const bool hasPreview=tableModel->getCurTemplateIndex()!=-1;
    ui->action_HideTemplate->setDisabled(!hasPreview);
    ui->action_EditTemplate->setDisabled(!hasPreview);
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
    tableModel->clearStudents();
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
    if(index!=-1)
    {
        tableModel->showRecordHistory(index);
        updateRecordPreviewActions();
    }
}

void MainWindow::hideRecord()
{
    tableModel->hideRecordHistory();
    updateRecordPreviewActions();
}

void MainWindow::deleteRecord(int index)
{
    if(index!=-1)
    {
        tableModel->deleteRecordHistory(index);
        updateRecordPreviewActions();
    }
}

void MainWindow::clearRecords()
{
    tableModel->clearScoreRecord();
    updateRecordPreviewActions();
}

/*积分模板操作*/
void MainWindow::newTemplate()
{
    ScoreTemplate newTemplate;
    newTemplate.description=tr("未命名-%1").arg(QDateTime::currentDateTime()
                                                .toString("yy/MM/dd hh:mm:ss:zzz"));
    newTemplate.values=QVector<qint32>(manager->getStudents().count());
    tableModel->newScoreTemplate(newTemplate);
    updateTemplatePreviewActions();
}

bool MainWindow::renameTemplate(const QString &newDescription)
{
    if(tableModel->getCurTemplateIndex()==-1)
        return false;
    if(!manager->verifyTemplateName(newDescription))
        return false;
    tableModel->setTemplateDescription(newDescription);
    return true;
}

void MainWindow::showTemplate(int index)
{
    if(index!=-1)
    {
        tableModel->showScoreTemplate(index);
        updateTemplatePreviewActions();
    }
}

void MainWindow::hideTemplate()
{
    tableModel->hideScoreTemplate();
    updateTemplatePreviewActions();
}

void MainWindow::deleteTemplate(int index)
{
    if(index!=-1)
    {
        tableModel->deleteScoreTemplate(index);
        updateTemplatePreviewActions();
    }
}

void MainWindow::clearTemplates()
{
    tableModel->clearScoreTemplate();
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
