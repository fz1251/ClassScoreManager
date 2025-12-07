#include "class_data_manager.h"

ClassDataManager::ClassDataManager(QObject *parent)
    : QObject(parent)
{

}

#ifdef QT_DEBUG
void ClassDataManager::debugPrint()
{
    qDebug() << "===== Class Data Manager Debug Information =====";

    // 1. 文件路径
    qDebug() << "[File Path]: " << filePath;
    qDebug() << "-----------------------------------------------";

    // 2. 排序设置
    qDebug() << "[Sort Settings]";
    qDebug() << "  Student Default: " << QString::number(settings.studentDefault, 2).rightJustified(8, '0');
    qDebug() << "  Student Previous:" << QString::number(settings.studentPrevious, 2).rightJustified(8, '0');
    qDebug() << "  Group Default:   " << QString::number(settings.groupDefault, 2).rightJustified(8, '0');
    qDebug() << "  Group Previous:  " << QString::number(settings.groupPrevious, 2).rightJustified(8, '0');
    qDebug() << "-----------------------------------------------";

    // 3. 学生信息列表
    qDebug() << "[Student List] (" << students.size() << "students)";
    for (int i = 0; i < students.size(); ++i) {
        const auto& s = students[i];
        qDebug().nospace()
            << "  #" << i
            << " | Name: " << s.name
            << " | Order: " << s.displayOrder
            << " | Group: " << (s.groupNumber == StudentInfo::NoGroup ? "No" : QString::number(s.groupNumber));
    }
    qDebug() << "-----------------------------------------------";

    // 4. 积分模板列表
    qDebug() << "[Score Templates] (" << templates.size() << "templates)";
    for (int i = 0; i < templates.size(); ++i) {
        const auto& t = templates[i];
        qDebug() << "  Template #" << i;
        qDebug() << "    Description: " << t.description;
        qDebug() << "    Values: " << t.values;
    }
    qDebug() << "-----------------------------------------------";

    // 5. 积分记录列表
    qDebug() << "[Score Records] (" << records.size() << "records)";
    for (int i = 0; i < records.size(); ++i) {
        const auto& r = records[i];
        qDebug() << "  Record #" << i;
        qDebug() << "    Time: " << r.saveTime.toString("yyyy-MM-dd hh:mm:ss");
        qDebug() << "    Description: " << r.description;
        qDebug() << "    Scores (" << r.scores.size() << "entries):";
        qDebug() << "    "<<r.scores;
    }
    qDebug() << "===== End of Debug Information =====";
}
#endif

QString ClassDataManager::openFile(const QString& openFilePath)
{
    filePath=openFilePath;
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
    {
        return QObject::tr("无法打开名为%1的文件")
                .arg(filePath);
    }
    QDataStream stream(&file);
    stream>>header;
    if(header.fileHeader!=CorrectFileHeader)
    {
        return QObject::tr("错误的文件头，文件格式不正确或已损坏");
    }
    stream>>settings;
    stream>>students;
    if(header.studentCount!=students.size())
    {
        return QObject::tr("学生信息不正确，应为%1条，实际为%2条")
                .arg(header.studentCount)
                .arg(students.size());
    }
    stream>>templates;
    if(header.templateCount!=templates.size())
    {
        return QObject::tr("积分模板信息不正确，应为%1条，实际为%2条")
                .arg(header.templateCount)
                .arg(templates.size());
    }
    stream>>records;
    if(header.recordCount!=records.size())
    {
        return QObject::tr("积分记录信息不正确，应为%1条，实际为%2条")
                .arg(header.recordCount)
                .arg(records.size());
    }
    file.close();
    m_hasData=true;
    return QString();
}

void ClassDataManager::newFile()
{
    m_hasData=true;
    m_isNewFile=true;
    m_isDirty=true;
}

void ClassDataManager::saveNewFile(const QString &saveFilePath)
{
    filePath=saveFilePath;
    saveFile();

    m_isNewFile=false;
}

void ClassDataManager::saveFile()
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
        return;
    QDataStream stream(&file);
    stream<<HeaderData{CorrectFileHeader,students.size(),templates.size(),records.size()};
    stream<<settings;
    stream<<students;
    stream<<templates;
    stream<<records;
    file.close();

    if(!this->isRecordUnsaved())
        setDirty(false);
}

void ClassDataManager::closeFile()
{
    filePath.clear();
    settings.defaultSettings();
    students.clear();
    templates.clear();
    records.clear();

    m_hasData=false;
    m_isNewFile=false;
    setDirty(false);
}

bool ClassDataManager::hasData() const
{
    return m_hasData;
}

bool ClassDataManager::isNewFile() const
{
    return m_isNewFile;
}

bool ClassDataManager::isDirty() const
{
    return m_isDirty;
}

bool ClassDataManager::isRecordUnsaved() const
{
    return !tempRecord.scores.isEmpty();
}

void ClassDataManager::setPreviousSettings(quint8 mode)
{
    if(mode&SortSetting::sortAsGroup)
        settings.groupPrevious=mode;
    else
        settings.studentPrevious=mode;
}

StudentInfo& ClassDataManager::studentAt(int index)
{
    setDirty(true);
    return students[index];
}

ScoreTemplate& ClassDataManager::templateAt(int index)
{
    setDirty(true);
    return templates[index];
}

const ScoreRecord& ClassDataManager::recordAt(int index)
{
    return records[index];
}

const SortSetting &ClassDataManager::getSettings() const
{
    return settings;
}

const QList<StudentInfo> &ClassDataManager::getStudents() const
{
    return students;
}

const QList<ScoreTemplate> &ClassDataManager::getTemplates() const
{
    return templates;
}

const QList<ScoreRecord> &ClassDataManager::getRecords() const
{
    return records;
}

QStringList ClassDataManager::getStudentList() const
{
    std::map<qint32,QString> sortedList;
    for(const auto& i:students)
        sortedList.insert({i.displayOrder,i.name});
    QStringList studentList;
    studentList.reserve(students.size());
    for(const auto& i:sortedList)
        studentList.append(tr("[%1号] %2").arg(i.first).arg(i.second));
    return studentList;
}

QStringList ClassDataManager::getTemplateList() const
{
    QStringList templateList;
    templateList.reserve(templates.size());
    for(const auto& i:templates)
        templateList.append(i.description);
    return templateList;
}

QStringList ClassDataManager::getRecordList() const
{
    QStringList recordList;
    recordList.reserve(records.size());
    for(const auto& i:records)
        recordList.append(tr("%1 [%2]").arg(i.description).arg(i.saveTime.toString("yyyy/M/d")));
    return recordList;
}

bool ClassDataManager::verifyTemplateName(const QString &templateName)
{
    if(templateName.isEmpty())
        return false;
    for(const ScoreTemplate& i:templates)
        if(i.description==templateName)
            return false;
    return true;
}

bool ClassDataManager::verifyRecordName(const QString &recordName)
{
    if(recordName.isEmpty())
        return false;
    for(const ScoreRecord& i:records)
        if(i.description==recordName)
            return false;
    return true;
}

void ClassDataManager::addStudent(const QString &newName)
{
    const int newId=students.size()+1;
    students.append({newName,newId,StudentInfo::NoGroup});
    for(auto& i:records)
        i.scores.append(0);
    for(auto& i:templates)
        i.values.append(0);
    setDirty(true);
}

void ClassDataManager::addTemplate(const ScoreTemplate &newTemplate)
{
    templates.append(newTemplate);
    setDirty(true);
}

void ClassDataManager::newRecord(const QString &recordName)
{
    tempRecord.description=recordName;
    tempRecord.scores.resize(students.size());
    tempRecord.scores.fill(0);
    setDirty(true);
}

void ClassDataManager::saveRecord(int templateIndex)
{
    tempRecord.saveTime=QDateTime::currentDateTime();
    if(templateIndex>=0&&templateIndex<templates.size())
        for(int i=0;i<tempRecord.scores.size();i++)
            tempRecord.scores[i]+=templates.at(templateIndex).values[i];
    records.append(tempRecord);
    tempRecord.scores.clear();
}

QList<qint32> &ClassDataManager::getTempRecord()
{
    return tempRecord.scores;
}

void ClassDataManager::importStudents(const QStringList &newNames)
{
    const int beginId=students.size()+1;
    QList<StudentInfo> newStudents;
    newStudents.reserve(newNames.size());
    for(int i=0;i<newNames.size();i++)
        newStudents.append({newNames.at(i),beginId+i,StudentInfo::NoGroup});
    students.append(std::move(newStudents));
    QList<qint32> zeros(newNames.size(),0);
    for(auto& i:records)
        i.scores.append(zeros);
    for(auto& i:templates)
        i.values.append(zeros);
    setDirty(true);
}

void ClassDataManager::removeStudent(int index)
{
    const int removedPlace=students.at(index).displayOrder;
    students.removeAt(index);
    for(auto& i:students)
        if(i.displayOrder>removedPlace)
            i.displayOrder--;
    for(auto& i:records)
        i.scores.removeAt(index);
    for(auto& i:templates)
        i.values.removeAt(index);
    setDirty(true);
}

void ClassDataManager::removeTemplate(int index)
{
    templates.removeAt(index);
    setDirty(true);
}

void ClassDataManager::removeRecord(int index)
{
    records.removeAt(index);
    setDirty(true);
}

void ClassDataManager::clearStudents()
{
    students.clear();
    templates.clear();
    records.clear();
    setDirty(true);
}

void ClassDataManager::clearTemplates()
{
    templates.clear();
    setDirty(true);
}

void ClassDataManager::clearRecords()
{
    records.clear();
    setDirty(true);
}

void ClassDataManager::setDirty(bool isDirty)
{
    if(m_isDirty!=isDirty)
    {
        m_isDirty=isDirty;
        emit dirtyStateChanged(isDirty);
    }
}
