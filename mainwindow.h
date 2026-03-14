#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "data_types.h"
#include "class_table_modal.h"
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


    void on_recordToggleBtn_toggled(bool checked);

    void on_templateToggleBtn_toggled(bool checked);

private:
    enum StackedPageIndex
    {
        WelcomePage,
        StudentTablePage,
        // ScoreRecordPage,
        // ScoreTemplatePage
    };
    void onFileLoaded();
    void onFileClosed();
    void updateWindowTitle(const QString& fileName);
    void openFile(const QString& filePath);
    void saveFile();
    void closeFile();
    QMessageBox::ButtonRole confirmUnsavedChanges();
    bool confirmClearOperation(const QString& description);
    Ui::MainWindow *ui;
    ClassDataManager* manager;
    ClassTableModal* modal;
};
#endif // MAINWINDOW_H
