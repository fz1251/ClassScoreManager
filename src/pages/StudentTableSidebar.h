#pragma once

#include <QString>
#include <QWidget>

class ExExpander;
class QLabel;
class QLineEdit;
class QPushButton;
class QResizeEvent;

// 班级表格页右侧边栏：由多个 ExExpander 折叠块组成，只负责展示与发信号
class StudentTableSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit StudentTableSidebar(QWidget *parent = nullptr);

    void setFileInfo(const QString &fileName, bool dirty, int studentCount,
                     int recordCount, int templateCount);
    void setRecordPreview(const QString &displayText, const QString &positionText,
                          bool hasPreview, bool canView, bool canPrevious, bool canNext);
    void setTemplatePreview(const QString &displayText, const QString &positionText,
                            bool hasPreview, bool canView, bool canPrevious, bool canNext);
    void setTemplateNameDraft(const QString &name);
    void setRecordEditingInProgress(bool isEditing);
    void setStudentActionsEnabled(bool enabled);
    QString recordNameText() const;
    void setRecordNameText(const QString &text);
    void focusRecordNameEdit();
    void focusTemplateNameEdit();

signals:
    void dragSortChanged(bool enabled);
    void nameEditChanged(bool enabled);
    void groupEditChanged(bool enabled);
    void addStudentsRequested();
    void deleteStudentRequested();
    void recordViewRequested();
    void recordPreviousRequested();
    void recordNextRequested();
    void recordPreviewCloseRequested();
    void newScoreRecordRequested();
    void saveScoreRecordRequested();
    void applyTemplateRequested();
    void newTemplateRequested();
    void templateViewRequested();
    void templatePreviousRequested();
    void templateNextRequested();
    void templatePreviewCloseRequested();
    void templateNameUpdateRequested(const QString &name);
    void templateNameEditCancelled();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateRecordPreviewTitle();
    void updateTemplatePreviewTitle();

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QLabel *fileNameLabel = nullptr;
    QLabel *studentCountLabel = nullptr;
    QLabel *recordCountLabel = nullptr;
    QLabel *templateCountLabel = nullptr;
    QPushButton *addStudentButton = nullptr;
    QPushButton *deleteStudentButton = nullptr;
    ExExpander *recordExpander = nullptr;
    QLabel *recordPreviewLabel = nullptr;
    QLabel *recordPositionLabel = nullptr;
    QPushButton *recordViewButton = nullptr;
    QPushButton *recordPreviousButton = nullptr;
    QPushButton *recordNextButton = nullptr;
    QPushButton *recordCloseButton = nullptr;
    QPushButton *newRecordButton = nullptr;
    QPushButton *applyTemplateButton = nullptr;
    QLineEdit *recordNameEdit = nullptr;
    QPushButton *saveRecordButton = nullptr;
    ExExpander *templateExpander = nullptr;
    QLabel *templatePreviewLabel = nullptr;
    QLabel *templatePositionLabel = nullptr;
    QPushButton *templateViewButton = nullptr;
    QPushButton *templatePreviousButton = nullptr;
    QPushButton *templateNextButton = nullptr;
    QPushButton *templateCloseButton = nullptr;
    QPushButton *newTemplateButton = nullptr;
    QLineEdit *templateNameEdit = nullptr;
    QPushButton *templateUpdateButton = nullptr;
    QPushButton *templateDiscardButton = nullptr;

    QString recordPreviewTitle;
    QString templatePreviewTitle;
};
