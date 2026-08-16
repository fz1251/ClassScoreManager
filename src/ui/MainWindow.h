#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "../core/DataTypes.h"
#include "../models/ClassTableModel.h"
#include "../models/RecordListModel.h"
#include "../models/TemplateListModel.h"
#include <QMainWindow>
#include <QLabel>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
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
    void on_action_ImportStudents_triggered();
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
    /*界面状态维护*/
    void onFileLoaded();
    void onFileClosed();
    void updateWindowTitle(const QString& fileName);
    void resetTable();
    void updateRecordPreviewActions();
    void updateTemplatePreviewActions();

    /*文件操作*/
    void openFile(const QString& filePath);
    void newFile();
    void saveFile();
    void closeFile();

    /*学生操作*/
    void addStudent(const QString& newName);
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
    Ui::MainWindow *ui;
    ClassDataManager* manager;
    ClassTableModel* tableModel;
    RecordListModel* recordModel;
    TemplateListModel* templateModel;
};
#endif // MAINWINDOW_H
