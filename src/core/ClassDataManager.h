#pragma once

#include "DataTypes.h"
#include <QObject>
#include <QDebug>
#include <QFile>

//班级信息管理和接口类
class ClassDataManager : public QObject
{
    Q_OBJECT
signals:
    void dirtyStateChanged(bool isDirty);
    void recordsChanged();
    void templatesChanged();
public:
    explicit ClassDataManager(QObject *parent = nullptr);
    virtual ~ClassDataManager() = default;
#ifdef QT_DEBUG
    void debugPrint();
#endif
    //文件读写
    QString openFile(const QString& openFilePath);
    void newFile();
    void saveNewFile(const QString& saveFilePath);
    void saveFile();
    void closeFile();
    //文件状态
    bool hasData() const;
    bool isNewFile() const;
    bool isDirty() const;
    bool isRecordUnsaved() const;
    //信息获取接口(读写&只读)
    void setPreviousSettings(SortSettings curSettings);
    StudentInfo&       studentAt (int index);
    ScoreTemplate&     templateAt(int index);
    void setTemplateDescription(int index, const QString& description);
    const ScoreRecord& recordAt  (int index);
    const SortSettings&          getSettings() const;
    const QVector<StudentInfo>&   getStudents() const;
    const QVector<ScoreTemplate>& getTemplates()const;
    const QVector<ScoreRecord>&   getRecords()  const;
    QStringList getStudentList () const;
    QString getTemplateDisplayName(int index) const;
    QString getRecordDisplayName(int index) const;
    int  getActivatedTemplateIndex() const;
    bool verifyTemplateName(const QString& templateName);
    bool verifyRecordName  (const QString& recordName);
    //数据增删与清空
    void addTemplate(const ScoreTemplate& newTemplate);
    void newRecord  (const QString      & recordName);
    void setRecordDraftDescription(const QString& description);
    void activateTemplate(int index);
    void saveRecord ();
    QVector<qint32>& getTempRecord();
    void importStudents(const QStringList& newNames);
    void removeStudent (int index);
    void removeTemplate(int index);
    void removeRecord  (int index);
    void clearTemplates();
    void clearRecords();

private:
    static constexpr quint64 CorrectFileHeader=0x0123456789abcdefull;
    void setDirty(bool isDirty);
    void clearRecordDraft();
    bool m_hasData   = false;
    bool m_isNewFile = false;
    bool m_isDirty   = false;
    ScoreRecord tempRecord;
    bool m_isRecordEditing = false;
    int activatedTemplateIndex = -1;
    QString filePath;
    HeaderData header;
    SortSettings settings;
    QVector<StudentInfo> students;
    QVector<ScoreTemplate> templates;
    QVector<ScoreRecord> records;
};

