#ifndef CLASSDATAMANAGER_H
#define CLASSDATAMANAGER_H

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
    const ScoreRecord& recordAt  (int index);
    const SortSettings&          getSettings() const;
    const QVector<StudentInfo>&   getStudents() const;
    const QVector<ScoreTemplate>& getTemplates()const;
    const QVector<ScoreRecord>&   getRecords()  const;
    QStringList getStudentList () const;
    QStringList getTemplateList() const;
    QStringList getRecordList  () const;
    bool verifyTemplateName(const QString& templateName);
    bool verifyRecordName  (const QString& recordName);
    //数据增删与清空
    void addStudent (const QString      & newName);
    void addTemplate(const ScoreTemplate& newTemplate);
    void newRecord  (const QString      & recordName);
    void saveRecord (int templateIndex);
    QVector<qint32>& getTempRecord();
    void importStudents(const QStringList& newNames);
    void removeStudent (int index);
    void removeTemplate(int index);
    void removeRecord  (int index);
    void clearStudents();
    void clearTemplates();
    void clearRecords();

private:
    static constexpr quint64 CorrectFileHeader=0x0123456789abcdefull;
    void setDirty(bool isDirty);
    bool m_hasData   = false;
    bool m_isNewFile = false;
    bool m_isDirty   = false;
    ScoreRecord tempRecord;
    QString filePath;
    HeaderData header;
    SortSettings settings;
    QVector<StudentInfo> students;
    QVector<ScoreTemplate> templates;
    QVector<ScoreRecord> records;
};

#endif // CLASSDATAMANAGER_H
