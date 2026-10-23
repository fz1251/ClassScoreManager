#pragma once

#include "../core/DataTypes.h"
#include "exinfobar.h"

#include <QByteArray>
#include <QCloseEvent>
#include <QMainWindow>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVector>

class QAction;
class QModelIndex;
class QStackedWidget;
class QTabBar;
class ClassDataManager;
class ClassTableModel;
class FileIoService;
class GroupBoardPage;
class RecordListPage;
class StudentSearchModel;
class StudentTablePage;
class TemplateListPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    enum StackedPageIndex
    {
        WelcomePage,
        StudentTablePage,
        GroupBoardPage,
        ScoreRecordPage,
        ScoreTemplatePage
    };

    enum class FileTask
    {
        None,
        OpenClass,
        SaveClass,
        ExportReport
    };

    enum class AfterSave
    {
        None,
        CloseWindow,
        CloseFile
    };

    enum class SaveChoice
    {
        Save,
        Discard,
        Cancel
    };

    void createActions();
    void createMenus();

    /*界面状态维护*/
    void onFileLoaded();
    void onFileClosed();
    void updateFilePreview();
    void resetTable();
    void updateRecordEditingActions();
    void updateRecordPreviewPanel();
    void updateTemplatePreviewPanel();
    void syncListPageSelection();

    /*文件操作*/
    void openFileDialog();
    void createNewFile();
    void openFile(const QString& filePath);
    void newFile();
    bool saveFile();
    bool startSave(AfterSave after);
    void closeFileWithConfirm();
    void closeFile();

    /*文件任务与提示*/
    bool isFileTaskBusy() const;
    bool beginFileTask(FileTask task, const QString& path);
    void finishFileTask();
    void showFileNotice();
    void hideFileNotice();
    QString fileNoticeTitle() const;
    void onFileReadFinished(const QString& path, const QByteArray& data, const QString& error);
    void onFileWriteFinished(const QString& path, const QString& error);

    /*学生操作*/
    void addStudentsDialog();
    void deleteStudentDialog();
    void importStudents(const QStringList& newNames);
    void removeStudent(int displayIndex, const QString& studentName);
    void editStudentGroup(const QModelIndex& index);
    void selectStudentFromSearch(int resultRow);

    /*积分记录操作*/
    void createScoreRecordDraft();
    void newScoreRecordFromList();
    bool saveScoreRecord();
    void applyTemplateDialog();
    void activateTemplate(int index);
    void exportScoreSum(const QVector<int>& recordIndices);
    void exportSelectedRecords();
    void selectRecordToView();
    void showRecord(int index);
    void hideRecord();
    void showPreviousRecord();
    void showNextRecord();
    void deleteSelectedRecords();
    void clearRecords();
    void clearRecordsWithConfirm();

    /*积分模板操作*/
    void newTemplate();
    void newTemplateFromList();
    void newTemplateFromSidebar();
    void updateTemplateName(const QString& newDescription);
    void discardTemplateNameEdit();
    void selectTemplateToView();
    void showTemplate(int index);
    void hideTemplate();
    void showPreviousTemplate();
    void showNextTemplate();
    void deleteSelectedTemplates();
    void clearTemplates();
    void clearTemplatesWithConfirm();

    /*关于*/
    void showAboutDialog();
    void showAboutQt();

    /*通用确认*/
    bool hasUnsavedChanges() const;
    SaveChoice askSaveChanges();
    bool confirmDestructiveOperation(const QString& action, const QString& description);

    /*提示通知*/
    void showNotification(ExInfoBar::Severity severity, const QString& title, const QString& message);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QStackedWidget* stackedWidget = nullptr;
    QTabBar* navigationTabBar = nullptr;
    QAction* action_OpenFile = nullptr;
    QAction* action_NewFile = nullptr;
    QAction* action_SaveFile = nullptr;
    QAction* action_CloseFile = nullptr;
    QAction* action_AboutApp = nullptr;
    QAction* action_AboutQt = nullptr;
    QPointer<ExInfoBar> fileNoticeBar;

    FileIoService* fileIoService = nullptr;
    ClassDataManager* manager = nullptr;
    ClassTableModel* tableModel = nullptr;
    StudentSearchModel* searchModel = nullptr;
    ::StudentTablePage* studentTablePage = nullptr;
    ::GroupBoardPage* groupBoardPage = nullptr;
    RecordListPage* recordListPage = nullptr;
    TemplateListPage* templateListPage = nullptr;
    QString currentFileName;
    FileTask fileTask = FileTask::None;
    AfterSave afterSave = AfterSave::None;
    quint64 fileTaskId = 0;
    QString taskPath;
    QByteArray pendingSaveSnapshot;
    SortSettings pendingExportSettings;
    bool groupEditEnabled = true;
    int recordPreviewIndex = -1;
    int templatePreviewIndex = -1;
};
