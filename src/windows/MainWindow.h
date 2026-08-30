#pragma once

#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QStringList>

class QAction;
class QStackedWidget;
class QTabBar;
class ClassDataManager;
class ClassTableModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    /*文件菜单(menu_File)操作*/
    void on_action_OpenFile_triggered();
    void on_action_NewFile_triggered();
    void on_action_SaveFile_triggered();
    void on_action_CloseFile_triggered();
    void on_action_ExitApp_triggered();

    /*学生菜单(menu_Student)操作*/
    void on_action_AddStudent_triggered();
    void on_action_DeleteStudent_triggered();
    void on_action_ClearStudents_triggered();

    /*积分菜单(menu_Score)操作*/
    void on_action_NewScoreRecord_triggered();
    void on_action_SaveScoreRecord_triggered();
    void on_action_UseScoreTemplate_triggered();
    void on_action_ExportScoreSum_triggered();
    /*---------------分割线---------------*/
    void on_action_ShowRecordHistory_triggered();
    void on_action_HideRecordHistory_triggered();
    void on_action_DeleteRecordHistory_triggered();
    void on_action_ClearRecords_triggered();

    /*积分模板菜单(menu_ScoreTemplate)操作*/
    void on_action_NewTemplate_triggered();
    void on_action_EditTemplate_triggered();
    void on_action_ShowTemplate_triggered();
    void on_action_HideTemplate_triggered();
    void on_action_DeleteTemplate_triggered();
    void on_action_ClearTemplates_triggered();

    /*关于菜单(menu_About)操作*/
    void on_action_AboutApp_triggered();
    void on_action_AboutQt_triggered();

private:
    enum StackedPageIndex
    {
        WelcomePage,
        StudentTablePage,
        ScoreRecordPage,
        ScoreTemplatePage
    };

    void createActions();
    void createMenus();

    /*界面状态维护*/
    void onFileLoaded();
    void onFileClosed();
    void updateWindowTitle(const QString& fileName);
    void resetTable();
    void updateRecordEditingActions();
    void updateRecordPreviewActions();
    void updateTemplatePreviewActions();

    /*文件操作*/
    void openFile(const QString& filePath);
    void newFile();
    void saveFile();
    void closeFile();

    /*学生操作*/
    void importStudents(const QStringList& newNames);
    void removeStudent(int displayIndex);
    void clearStudents();

    /*积分记录操作*/
    bool newScoreRecord(const QString& recordName);
    void saveScoreRecord();
    void activateTemplate(int index);
    void exportScoreSum();
    void showRecord(int index);
    void hideRecord();
    void deleteRecord(int index);
    void clearRecords();

    /*积分模板操作*/
    void newTemplate();
    bool renameTemplate(const QString& newDescription);
    void showTemplate(int index);
    void hideTemplate();
    void deleteTemplate(int index);
    void clearTemplates();

    /*通用确认*/
    QMessageBox::ButtonRole confirmUnsavedChanges();
    bool confirmClearOperation(const QString& description);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QStackedWidget* stackedWidget = nullptr;
    QTabBar* navigationTabBar = nullptr;
    QAction* action_OpenFile = nullptr;
    QAction* action_NewFile = nullptr;
    QAction* action_SaveFile = nullptr;
    QAction* action_CloseFile = nullptr;
    QAction* action_ExitApp = nullptr;
    QAction* action_AddStudent = nullptr;
    QAction* action_DeleteStudent = nullptr;
    QAction* action_ClearStudents = nullptr;
    QAction* action_NewScoreRecord = nullptr;
    QAction* action_SaveScoreRecord = nullptr;
    QAction* action_UseScoreTemplate = nullptr;
    QAction* action_ExportScoreSum = nullptr;
    QAction* action_ShowRecordHistory = nullptr;
    QAction* action_HideRecordHistory = nullptr;
    QAction* action_DeleteRecordHistory = nullptr;
    QAction* action_ClearRecords = nullptr;
    QAction* action_NewTemplate = nullptr;
    QAction* action_EditTemplate = nullptr;
    QAction* action_ShowTemplate = nullptr;
    QAction* action_HideTemplate = nullptr;
    QAction* action_DeleteTemplate = nullptr;
    QAction* action_ClearTemplates = nullptr;
    QAction* action_AboutApp = nullptr;
    QAction* action_AboutQt = nullptr;

    ClassDataManager* manager = nullptr;
    ClassTableModel* tableModel = nullptr;
    int recordPreviewIndex = -1;
    int templatePreviewIndex = -1;
};
