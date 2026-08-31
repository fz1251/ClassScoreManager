#pragma once

#include <QCloseEvent>
#include <QMainWindow>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QVector>

class QAction;
class QStackedWidget;
class QTabBar;
class ClassDataManager;
class ClassTableModel;
class RecordListPage;
class StudentTablePage;
class TemplateListPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

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
    void updateFilePreview();
    void resetTable();
    void updateRecordEditingActions();
    void updateRecordPreviewPanel();
    void updateTemplatePreviewPanel();

    /*文件操作*/
    void openFileDialog();
    void createNewFile();
    void openFile(const QString& filePath);
    void newFile();
    void saveFile();
    void closeFileWithConfirm();
    void closeFile();

    /*学生操作*/
    void addStudentsDialog();
    void deleteStudentDialog();
    void importStudents(const QStringList& newNames);
    void removeStudent(int displayIndex);

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
    QMessageBox::ButtonRole confirmUnsavedChanges();
    bool confirmClearOperation(const QString& description);
    bool confirmDeleteOperation(const QString& description);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QStackedWidget* stackedWidget = nullptr;
    QTabBar* navigationTabBar = nullptr;
    QAction* action_OpenFile = nullptr;
    QAction* action_NewFile = nullptr;
    QAction* action_SaveFile = nullptr;
    QAction* action_CloseFile = nullptr;
    QAction* action_AboutApp = nullptr;
    QAction* action_AboutQt = nullptr;

    ClassDataManager* manager = nullptr;
    ClassTableModel* tableModel = nullptr;
    ::StudentTablePage* studentTablePage = nullptr;
    RecordListPage* recordListPage = nullptr;
    TemplateListPage* templateListPage = nullptr;
    QString currentFileName;
    int recordPreviewIndex = -1;
    int templatePreviewIndex = -1;
};
