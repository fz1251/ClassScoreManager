#include "ClassDataManager.h"

#include <algorithm>
#include <qglobal.h>

ClassDataManager::ClassDataManager(QObject* parent)
    : QObject(parent)
{
}

QString ClassDataManager::openFile(const QString& openFilePath)
{
    clearRecordDraft();
    filePath = openFilePath;
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return QObject::tr("无法打开名为%1的文件").arg(filePath);
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);
    stream >> header;
    if(header.fileHeader != CorrectFileHeader)
        return QObject::tr("错误的文件头，文件格式不正确或已损坏");
    stream >> settings;
    stream >> students;
    if(header.studentCount != students.size())
        return QObject::tr("学生信息不正确，应为%1条，实际为%2条").arg(header.studentCount).arg(students.size());
    stream >> templates;
    if(header.templateCount != templates.size())
        return QObject::tr("积分模板信息不正确，应为%1条，实际为%2条").arg(header.templateCount).arg(templates.size());
    stream >> records;
    if(header.recordCount != records.size())
        return QObject::tr("积分记录信息不正确，应为%1条，实际为%2条").arg(header.recordCount).arg(records.size());
    file.close();
    m_hasData = true;
    emit recordsChanged();
    emit templatesChanged();
    emit studentsChanged();

    return QString();
}

void ClassDataManager::newFile()
{
    clearRecordDraft();
    m_hasData = true;
    m_isNewFile = true;
    m_isDirty = true;
    emit recordsChanged();
    emit templatesChanged();
}

void ClassDataManager::saveNewFile(const QString& saveFilePath)
{
    filePath = saveFilePath;
    saveFile();

    m_isNewFile = false;
}

void ClassDataManager::saveFile()
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << HeaderData{CorrectFileHeader, students.size(), templates.size(), records.size()};
    stream << settings;
    stream << students;
    stream << templates;
    stream << records;
    file.close();

    if(!this->isRecordUnsaved())
        setDirty(false);
}

void ClassDataManager::closeFile()
{
    clearRecordDraft();
    filePath.clear();
    settings.defaultSettings();
    students.clear();
    templates.clear();
    records.clear();

    m_hasData = false;
    m_isNewFile = false;
    setDirty(false);
    emit recordsChanged();
    emit templatesChanged();
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
    return m_isRecordEditing;
}

bool ClassDataManager::isEmptyNewFile() const
{
    return m_isNewFile && students.isEmpty() && records.isEmpty() && templates.isEmpty();
}

void ClassDataManager::setPreviousSettings(SortSettings curSettings)
{
    settings = curSettings;
    setDirty(true);
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

void ClassDataManager::setTemplateDescription(int index, const QString& description)
{
    if(index < 0 || index >= templates.size())
        return;
    templates[index].description = description;
    setDirty(true);
    emit templatesChanged();
}

const ScoreRecord& ClassDataManager::recordAt(int index)
{
    return records[index];
}

const SortSettings& ClassDataManager::getSettings() const
{
    return settings;
}

const QVector<StudentInfo>& ClassDataManager::getStudents() const
{
    return students;
}

const QVector<ScoreTemplate>& ClassDataManager::getTemplates() const
{
    return templates;
}

const QVector<ScoreRecord>& ClassDataManager::getRecords() const
{
    return records;
}

QStringList ClassDataManager::getStudentList() const
{
    std::map<qint32, QString> sortedList;
    for(const auto& i : students)
        sortedList.insert({i.displayOrder, i.name});
    QStringList studentList;
    studentList.reserve(students.size());
    for(const auto& i : sortedList)
        studentList.append(tr("[%1号] %2").arg(i.first).arg(i.second));
    return studentList;
}

QString ClassDataManager::getTemplateDisplayName(int index) const
{
    return templates.at(index).description;
}

QString ClassDataManager::getRecordDisplayName(int index) const
{
    const ScoreRecord& record = records.at(index);
    return tr("%1 [%2]").arg(record.description).arg(record.saveTime.toString("yyyy/M/d"));
}

bool ClassDataManager::verifyTemplateName(const QString& templateName)
{
    if(templateName.isEmpty())
        return false;
    for(const ScoreTemplate& i : templates)
        if(i.description == templateName)
            return false;
    return true;
}

bool ClassDataManager::verifyRecordName(const QString& recordName)
{
    if(recordName.isEmpty())
        return false;
    for(const ScoreRecord& i : records)
        if(i.description == recordName)
            return false;
    return true;
}

void ClassDataManager::addTemplate(const ScoreTemplate& newTemplate)
{
    templates.append(newTemplate);
    setDirty(true);
    emit templatesChanged();
}

void ClassDataManager::newRecord(const QString& recordName)
{
    tempRecord.description = recordName;
    tempRecord.scores.resize(students.size());
    tempRecord.scores.fill(0);
    activatedTemplateIndex = -1;
    m_isRecordEditing = true;
    setDirty(true);
}

void ClassDataManager::saveRecord()
{
    tempRecord.saveTime = QDateTime::currentDateTime();
    if(activatedTemplateIndex >= 0 && activatedTemplateIndex < templates.size())
        for(int i = 0; i < tempRecord.scores.size(); i++)
            tempRecord.scores[i] += templates.at(activatedTemplateIndex).values[i];
    records.append(tempRecord);
    tempRecord.scores.clear();
    activatedTemplateIndex = -1;
    m_isRecordEditing = false;
    emit recordsChanged();
}

int ClassDataManager::getActivatedTemplateIndex() const
{
    return activatedTemplateIndex;
}

void ClassDataManager::setRecordDraftDescription(const QString& description)
{
    if(!m_isRecordEditing)
        return;
    tempRecord.description = description;
}

void ClassDataManager::activateTemplate(int index)
{
    if(index >= 0 && index < templates.size())
        activatedTemplateIndex = index;
}

QVector<qint32>& ClassDataManager::getTempRecord()
{
    return tempRecord.scores;
}

void ClassDataManager::importStudents(const QStringList& newNames)
{
    const int beginId = students.size() + 1;
    QVector<StudentInfo> newStudents;
    newStudents.reserve(newNames.size());
    for(int i = 0; i < newNames.size(); i++)
        newStudents.append({newNames.at(i), beginId + i, StudentInfo::NoGroup, StudentInfo::NoOrder});
    students.append(std::move(newStudents));
    QVector<qint32> zeros(newNames.size(), 0);
    for(auto& i : records)
        i.scores.append(zeros);
    for(auto& i : templates)
        i.values.append(zeros);
    if(m_isRecordEditing)
        tempRecord.scores.append(zeros);
    setDirty(true);
}

void ClassDataManager::removeStudent(int index)
{
    const int removedPlace = students.at(index).displayOrder;
    const qint32 removedGroup = students.at(index).groupNumber;
    const qint32 removedOrder = students.at(index).groupOrder;
    students.removeAt(index);
    for(auto& i : students)
        if(i.displayOrder > removedPlace)
            i.displayOrder--;
    if(removedGroup != StudentInfo::NoGroup)
        for(auto& i : students)
            if(i.groupNumber == removedGroup && i.groupOrder > removedOrder)
                i.groupOrder--;
    for(auto& i : records)
        i.scores.removeAt(index);
    for(auto& i : templates)
        i.values.removeAt(index);
    if(m_isRecordEditing)
        tempRecord.scores.removeAt(index);
    setDirty(true);
}

bool ClassDataManager::moveStudentToGroup(int studentIndex, qint32 groupNumber, int position)
{
    if(studentIndex < 0 || studentIndex >= students.size())
        return false;
    const qint32 oldGroup = students[studentIndex].groupNumber;
    const qint32 oldOrder = students[studentIndex].groupOrder;

    //分组和组内位置都没变时直接返回，避免原样放回也把文件置脏
    if(groupNumber == oldGroup)
    {
        if(groupNumber == StudentInfo::NoGroup)
            return false;
        const QVector<int> current = groupStudentIndices(groupNumber);
        QVector<int> reordered = current;
        reordered.removeAll(studentIndex);
        const int target = (position < 0 || position > reordered.size()) ? reordered.size() : position;
        reordered.insert(target, studentIndex);
        if(reordered == current)
            return false;
    }

    //从原组摘除：排在其后的成员顺序前移
    if(oldGroup != StudentInfo::NoGroup)
        for(StudentInfo& student : students)
            if(student.groupNumber == oldGroup && student.groupOrder > oldOrder)
                student.groupOrder--;

    students[studentIndex].groupNumber = groupNumber;
    students[studentIndex].groupOrder = StudentInfo::NoOrder;

    //目标组内按插入位置重排为连续的0..n-1
    if(groupNumber != StudentInfo::NoGroup)
    {
        QVector<int> members = groupStudentIndices(groupNumber);
        members.removeAll(studentIndex);
        if(position < 0 || position > members.size())
            position = members.size();
        members.insert(position, studentIndex);
        for(int i = 0; i < members.size(); i++)
            students[members.at(i)].groupOrder = i;
    }
    setDirty(true);
    emit studentsChanged();
    return true;
}

QVector<qint32> ClassDataManager::groupNumbers() const
{
    QVector<qint32> numbers;
    for(const StudentInfo& student : students)
        if(student.groupNumber != StudentInfo::NoGroup && !numbers.contains(student.groupNumber))
            numbers.append(student.groupNumber);
    std::sort(numbers.begin(), numbers.end());
    return numbers;
}

QVector<int> ClassDataManager::groupStudentIndices(qint32 groupNumber) const
{
    QVector<int> members;
    for(int i = 0; i < students.size(); i++)
        if(students.at(i).groupNumber == groupNumber)
            members.append(i);
    std::sort(members.begin(), members.end(),
              [this](int left, int right)
              {
                  return students.at(left).groupOrder < students.at(right).groupOrder;
              });
    return members;
}

void ClassDataManager::removeTemplate(int index)
{
    if(activatedTemplateIndex == index)
        activatedTemplateIndex = -1;
    else if(index < activatedTemplateIndex)
        activatedTemplateIndex--;
    templates.removeAt(index);
    setDirty(true);
    emit templatesChanged();
}

void ClassDataManager::removeRecord(int index)
{
    records.removeAt(index);
    setDirty(true);
    emit recordsChanged();
}

void ClassDataManager::clearTemplates()
{
    activatedTemplateIndex = -1;
    templates.clear();
    setDirty(true);
    emit templatesChanged();
}

void ClassDataManager::clearRecords()
{
    records.clear();
    setDirty(true);
    emit recordsChanged();
}

void ClassDataManager::clearRecordDraft()
{
    tempRecord = ScoreRecord();
    m_isRecordEditing = false;
    activatedTemplateIndex = -1;
}

void ClassDataManager::setDirty(bool isDirty)
{
    if(m_isDirty != isDirty)
    {
        m_isDirty = isDirty;
        emit dirtyStateChanged(isDirty);
    }
}
