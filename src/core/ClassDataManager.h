#pragma once

#include "DataTypes.h"

#include <QFile>
#include <QObject>

//班级信息管理和接口类
class ClassDataManager : public QObject
{
    Q_OBJECT
signals:
    void dirtyStateChanged(bool isDirty);
    void recordsChanged();
    void templatesChanged();
    void studentsChanged();

public:
    //打开文件的结果：解析失败时message为失败原因；解析成功但修正过数据时message为修正说明，正常打开时为空
    struct OpenResult
    {
        bool succeeded = false;
        QString message;
    };
    explicit ClassDataManager(QObject* parent = nullptr);
    virtual ~ClassDataManager() = default;
    //文件读写
    OpenResult openFile(const QString& openFilePath);
    void newFile();
    bool saveNewFile(const QString& saveFilePath);
    bool saveFile();
    void closeFile();
    //文件状态
    bool hasData() const;
    bool isNewFile() const;
    bool isDirty() const;
    bool isRecordUnsaved() const;
    bool isEmptyNewFile() const;
    //信息获取接口(读写&只读)
    void setPreviousSettings(SortSettings curSettings);
    StudentInfo& studentAt(int index);
    ScoreTemplate& templateAt(int index);
    void setTemplateDescription(int index, const QString& description);
    const ScoreRecord& recordAt(int index);
    const SortSettings& getSettings() const;
    const QVector<StudentInfo>& getStudents() const;
    const QVector<ScoreTemplate>& getTemplates() const;
    const QVector<ScoreRecord>& getRecords() const;
    QStringList getStudentList() const;
    QString getTemplateDisplayName(int index) const;
    QString getRecordDisplayName(int index) const;
    int getActivatedTemplateIndex() const;
    bool verifyTemplateName(const QString& templateName);
    bool verifyRecordName(const QString& recordName);
    //数据增删与清空
    void addTemplate(const ScoreTemplate& newTemplate);
    void newRecord(const QString& recordName);
    void setRecordDraftDescription(const QString& description);
    void activateTemplate(int index);
    void saveRecord();
    QVector<qint32>& getTempRecord();
    void importStudents(const QStringList& newNames);
    void removeStudent(int index);
    //小组分配：position为-1表示追加到组末；groupNumber为NoGroup表示取消分组
    //返回是否真的发生了改变，原样放回时返回false且不置脏
    bool moveStudentToGroup(int studentIndex, qint32 groupNumber, int position = -1);
    QVector<qint32> groupNumbers() const;
    QVector<int> groupStudentIndices(qint32 groupNumber) const;
    void removeTemplate(int index);
    void removeRecord(int index);
    void clearTemplates();
    void clearRecords();

private:
    static constexpr quint64 CorrectFileHeader = 0x0123456789abcdefull;
    //把indices指到的学生按(顺序字段, 学生索引)递增排序后从base起重新编号，返回是否真的改动过
    static bool renumberOrder(QVector<StudentInfo>& students, QVector<int> indices, qint32 base, qint32 StudentInfo::* field);
    //修正读入的数据：顺序字段归一化、分数向量长度对齐，返回修正说明（无修正时为空）
    static QStringList repairLoadedData(QVector<StudentInfo>& newStudents, QVector<ScoreTemplate>& newTemplates,
                                        QVector<ScoreRecord>& newRecords);
    void setDirty(bool isDirty);
    void clearRecordDraft();
    bool m_hasData = false;
    bool m_isNewFile = false;
    bool m_isDirty = false;
    ScoreRecord tempRecord;
    bool m_isRecordEditing = false;
    int activatedTemplateIndex = -1;
    QString filePath;
    SortSettings settings;
    QVector<StudentInfo> students;
    QVector<ScoreTemplate> templates;
    QVector<ScoreRecord> records;
};
