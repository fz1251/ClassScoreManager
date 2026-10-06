#include "ClassDataManager.h"

#include <QSaveFile>
#include <algorithm>
#include <qglobal.h>
#include <utility>

ClassDataManager::ClassDataManager(QObject* parent)
    : QObject(parent)
{
}

ClassDataManager::OpenResult ClassDataManager::openFile(const QString& openFilePath)
{
    clearRecordDraft();
    QFile file(openFilePath);
    if(!file.open(QIODevice::ReadOnly))
        return {false, QObject::tr("无法打开名为%1的文件").arg(openFilePath)};
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);

    //先解析到局部变量，全部校验通过后再提交，避免解析失败留下半份数据与错误的文件路径
    HeaderData newHeader;
    SortSettings newSettings;
    QVector<StudentInfo> newStudents;
    QVector<ScoreTemplate> newTemplates;
    QVector<ScoreRecord> newRecords;
    stream >> newHeader;
    if(newHeader.fileHeader != CorrectFileHeader)
        return {false, QObject::tr("错误的文件头，文件格式不正确或已损坏")};
    stream >> newSettings;
    stream >> newStudents;
    stream >> newTemplates;
    stream >> newRecords;
    //读取中断说明文件被截断，容器已被清空，先于数量校验拦下
    if(stream.status() != QDataStream::Ok)
        return {false, QObject::tr("文件不完整，读取失败")};
    //文件头的计数与数据由同一次保存写出，对不上说明文件已被外部改动，任何一侧都不可信
    if(newHeader.studentCount != newStudents.size())
        return {false, QObject::tr("学生信息不正确，应为%1条，实际为%2条").arg(newHeader.studentCount).arg(newStudents.size())};
    if(newHeader.templateCount != newTemplates.size())
        return {false, QObject::tr("积分模板信息不正确，应为%1条，实际为%2条").arg(newHeader.templateCount).arg(newTemplates.size())};
    if(newHeader.recordCount != newRecords.size())
        return {false, QObject::tr("积分记录信息不正确，应为%1条，实际为%2条").arg(newHeader.recordCount).arg(newRecords.size())};
    file.close();

    //结构完整的前提下，字段级的不一致都可以就地修正
    const QStringList repairs = repairLoadedData(newStudents, newTemplates, newRecords);
    filePath = openFilePath;
    settings = newSettings;
    students = std::move(newStudents);
    templates = std::move(newTemplates);
    records = std::move(newRecords);
    m_hasData = true;
    setDirty(!repairs.isEmpty());
    emit recordsChanged();
    emit templatesChanged();
    emit studentsChanged();

    return {true, repairs.join('\n')};
}

bool ClassDataManager::renumberOrder(QVector<StudentInfo>& students, QVector<int> indices, qint32 base, qint32 StudentInfo::* field)
{
    std::sort(indices.begin(), indices.end(),
              [&students, field](int left, int right)
              {
                  const qint32 leftOrder = students.at(left).*field;
                  const qint32 rightOrder = students.at(right).*field;
                  //相同顺序值时按学生索引决定先后，保证结果确定
                  return leftOrder != rightOrder ? leftOrder < rightOrder : left < right;
              });
    bool changed = false;
    for(int i = 0; i < indices.size(); i++)
    {
        StudentInfo& student = students[indices.at(i)];
        if(student.*field != base + i)
            changed = true;
        student.*field = base + i;
    }
    return changed;
}

QStringList ClassDataManager::repairLoadedData(QVector<StudentInfo>& newStudents, QVector<ScoreTemplate>& newTemplates,
                                               QVector<ScoreRecord>& newRecords)
{
    QStringList repairs;

    //显示顺序必须是1..size的排列；它在students向量中的先后无关，拖动排序只交换该字段
    QVector<int> allIndices;
    allIndices.reserve(newStudents.size());
    for(int i = 0; i < newStudents.size(); i++)
        allIndices.append(i);
    if(renumberOrder(newStudents, allIndices, 1, &StudentInfo::displayOrder))
        repairs.append(QObject::tr("学生的显示顺序不正确"));

    //组内顺序每组各自是1..组内人数的排列，未分组的学生没有组内顺序
    QVector<qint32> groupList;
    for(const StudentInfo& student : newStudents)
        if(student.groupNumber != StudentInfo::NoGroup && !groupList.contains(student.groupNumber))
            groupList.append(student.groupNumber);
    bool groupOrderChanged = false;
    for(qint32 groupNumber : groupList)
    {
        QVector<int> members;
        for(int i = 0; i < newStudents.size(); i++)
            if(newStudents.at(i).groupNumber == groupNumber)
                members.append(i);
        if(renumberOrder(newStudents, members, 1, &StudentInfo::groupOrder))
            groupOrderChanged = true;
    }
    for(StudentInfo& student : newStudents)
        if(student.groupNumber == StudentInfo::NoGroup && student.groupOrder != StudentInfo::NoOrder)
        {
            student.groupOrder = StudentInfo::NoOrder;
            groupOrderChanged = true;
        }
    if(groupOrderChanged)
        repairs.append(QObject::tr("小组内的顺序不正确"));

    //分数向量的长度必须与学生数一致：长于学生数的截断，短于学生数的补零
    bool scoreSizeChanged = false;
    for(ScoreTemplate& scoreTemplate : newTemplates)
    {
        if(scoreTemplate.values.size() != newStudents.size())
            scoreSizeChanged = true;
        scoreTemplate.values.resize(newStudents.size());
    }
    for(ScoreRecord& record : newRecords)
    {
        if(record.scores.size() != newStudents.size())
            scoreSizeChanged = true;
        record.scores.resize(newStudents.size());
    }
    if(scoreSizeChanged)
        repairs.append(QObject::tr("积分模板与积分记录中的分数个数与学生数不一致"));

    return repairs;
}

void ClassDataManager::newFile()
{
    clearRecordDraft();
    m_hasData = true;
    m_isNewFile = true;
    setDirty(true);
    emit recordsChanged();
    emit templatesChanged();
}

bool ClassDataManager::saveNewFile(const QString& saveFilePath)
{
    filePath = saveFilePath;
    if(!saveFile())
        return false;

    m_isNewFile = false;
    return true;
}

bool ClassDataManager::saveFile()
{
    //先写同目录的临时文件，commit成功才替换目标文件，写失败时磁盘上的旧文件保持原样
    QSaveFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return false;
    QDataStream stream(&file);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << HeaderData{CorrectFileHeader, students.size(), templates.size(), records.size()};
    stream << settings;
    stream << students;
    stream << templates;
    stream << records;
    if(stream.status() != QDataStream::Ok || !file.commit())
        return false;

    if(!this->isRecordUnsaved())
        setDirty(false);
    return true;
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
    //配置没有变化时不置脏，避免只是导出一次就把文件标成未保存
    if(settings == curSettings)
        return;
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
    //启用的模板只在保存时并入正式记录，表格的“积分(当前)”列显示的是不含模板的草稿值
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
    const qint32 beginId = students.size() + 1;
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
    const qint32 removedPlace = students.at(index).displayOrder;
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

    //目标组内按插入位置重排为连续的1..n
    if(groupNumber != StudentInfo::NoGroup)
    {
        QVector<int> members = groupStudentIndices(groupNumber);
        members.removeAll(studentIndex);
        if(position < 0 || position > members.size())
            position = members.size();
        members.insert(position, studentIndex);
        for(int i = 0; i < members.size(); i++)
            students[members.at(i)].groupOrder = i + 1;
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
