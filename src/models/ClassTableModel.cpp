#include "ClassTableModel.h"

ClassTableModel::ClassTableModel(ClassDataManager* dataManager, QObject *parent)
    : QAbstractTableModel(parent)
    , manager(dataManager)
{
    Q_ASSERT(dataManager);
    // 模板集合变化会影响当前模板列与模板预览列的取值
    connect(manager,&ClassDataManager::templatesChanged,this,[this](){
        if(rowCount() == 0)
            return;
        const QModelIndex topLeft = index(0, CurrentTemplate);
        const QModelIndex bottomRight = index(rowCount() - 1, CurrentTemplate);
        emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::ForegroundRole});
    });
}

int ClassTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return students().size();
}

int ClassTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if(manager->hasData())
        return MaxColumnCount;
    else
        return 0;
}

QVariant ClassTableModel::data(const QModelIndex &index, int role) const
{
    if(!isValidModelIndex(index))
        return QVariant();
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        switch(index.column())
        {
        case StudentName:
            return students().at(rowToIndex[index.row()]).name;
        case GroupNumber:
        {
            const int groupNum = students().at(rowToIndex[index.row()]).groupNumber;
            if(groupNum == StudentInfo::NoGroup)
                return QObject::tr("未分组");
            else
                return groupNum;
        }
        case CurrentRecord:
            return manager->getTempRecord().at(rowToIndex[index.row()]);
        case CurrentTemplate:
            if(isValidIndex(manager->getActivatedTemplateIndex()))
                return templates().at(manager->getActivatedTemplateIndex()).values.at(rowToIndex[index.row()]);
            else
                return 0;
        case RecordHistoryPreview:
            return records().at(previewRecordIndex).scores.at(rowToIndex[index.row()]);
        case ScoreTemplatePreview:
            return templates().at(previewTemplateIndex).values.at(rowToIndex[index.row()]);
        default :
            return QVariant();
        }
    }
    case Qt::TextAlignmentRole:
        return Qt::AlignCenter;
    case Qt::ForegroundRole:
    {
        switch(index.column())
        {
        case CurrentRecord:
            // return getScoreColor(manager->getTempRecord().at(rowToIndex[index.row()]));
            return getScoreColor(manager->getTempRecord().value(rowToIndex[index.row()], 0));
        case CurrentTemplate:
            if(isValidIndex(manager->getActivatedTemplateIndex()))
                return getScoreColor(templates().at(manager->getActivatedTemplateIndex()).values.at(rowToIndex[index.row()]));
            else
                return getScoreColor(0);
        case RecordHistoryPreview:
            return getScoreColor(records().at(previewRecordIndex).scores.at(rowToIndex[index.row()]));
        case ScoreTemplatePreview:
            return getScoreColor(templates().at(previewTemplateIndex).values.at(rowToIndex[index.row()]));
        default:
            return QVariant();
        }
    }
    case Qt::CheckStateRole:
        if(index.column() == GroupNumber)
        {
            int groupNum = students().at(rowToIndex[index.row()]).groupNumber;
            if(groupNum == StudentInfo::NoGroup)
                return Qt::Unchecked;
            else
                return Qt::Checked;
        }
        else
            return QVariant();
    default:
        return QVariant();
    }
}

QVariant ClassTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant();
    if(orientation == Qt::Horizontal)
    {
        switch(section)
        {
        case StudentName:
            return tr("姓名");
        case GroupNumber:
            return tr("小组号");
        case CurrentRecord:
            return tr("积分(当前)");
        case CurrentTemplate:
            return tr("积分模板(当前)");
        case RecordHistoryPreview:
            return tr("积分(浏览)");
        case ScoreTemplatePreview:
            return tr("积分模板(浏览)");
        default:
            return QString();
        }
    }
    else
    {
        return QString::number(section + 1);
    }
}

bool ClassTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!isValidModelIndex(index))
        return false;
    if(role == Qt::EditRole)
    {
        switch(index.column())
        {
        case StudentName:
            manager->studentAt(rowToIndex[index.row()]).name = value.toString();
            break;
        case GroupNumber:
        {
            bool ok;
            const int val = value.toInt(&ok);
            if(!ok || val < 0)
                return false;
            manager->studentAt(rowToIndex[index.row()]).groupNumber = val;
            break;
        }
        case CurrentRecord:
        {
            bool ok;
            const int val = value.toInt(&ok);
            if(!ok)
                return false;
            manager->getTempRecord()[rowToIndex[index.row()]] = val;
            break;
        }
        case ScoreTemplatePreview:
        {
            bool ok;
            const int val = value.toInt(&ok);
            if(!ok)
                return false;
            manager->templateAt(previewTemplateIndex).values[rowToIndex[index.row()]] = val;
            break;
        }
        default:
            return false;
        }
        emit dataChanged(index, index);
        return true;
    }
    else if(role == Qt::CheckStateRole)
    {
        const int checked = value.toInt();
        int& groupNum = manager->studentAt(rowToIndex[index.row()]).groupNumber;
        if(checked)
            groupNum = 0;
        else
            groupNum = StudentInfo::NoGroup;
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags ClassTableModel::flags(const QModelIndex &index) const
{
    if(!isValidModelIndex(index))
        return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
    Qt::ItemFlags baseFlag = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    switch(index.column())
    {
    case StudentName:
        if(canEditStudent)
        {
            baseFlag |= Qt::ItemIsEditable;
        }
        break;
    case GroupNumber:
        if(canEditGroup)
        {
        baseFlag |= Qt::ItemIsUserCheckable;
        if(students().at(rowToIndex[index.row()]).groupNumber != StudentInfo::NoGroup)
            baseFlag |= Qt::ItemIsEditable;
        }
        break;
    case CurrentRecord:
    case ScoreTemplatePreview:
        baseFlag |= Qt::ItemIsEditable;
        break;
    default:
        break;
    }
    return baseFlag;
}

Qt::DropActions ClassTableModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QMimeData* ClassTableModel::mimeData(const QModelIndexList &indexes) const
{
    if(indexes.isEmpty())
        return nullptr;
    QMimeData* mimeData = new QMimeData;
    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    stream << indexes.first().row();
    mimeData->setData(moveRowDataType, encodedData);
    return mimeData;
}

bool ClassTableModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    if(action == Qt::MoveAction && data->hasFormat(moveRowDataType))
        return true;
    else
        return false;
}

bool ClassTableModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);
    if(action != Qt::MoveAction)
        return false;
    if(!isValidIndex(row))
        return false;
    if(!data->hasFormat(moveRowDataType))
        return false;
    QByteArray encodedData = data->data(moveRowDataType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int sourceRow;
    stream >> sourceRow;
    beginMoveRows(parent, sourceRow, sourceRow, parent, row);
    if(row > sourceRow)
    {
        for(int i = sourceRow; i < row - 1; i++)
        {
            std::swap(manager->studentAt(rowToIndex[i]).displayOrder,
                      manager->studentAt(rowToIndex[i + 1]).displayOrder);
            std::swap(rowToIndex[i], rowToIndex[i + 1]);
        }
    }
    else
    {
        for(int i = sourceRow; i > row; i--)
        {
            std::swap(manager->studentAt(rowToIndex[i]).displayOrder,
                      manager->studentAt(rowToIndex[i - 1]).displayOrder);
            std::swap(rowToIndex[i], rowToIndex[i - 1]);
        }
    }
    endMoveRows();
    return true;
}

void ClassTableModel::importStudents(const QStringList& newNames)
{
    const int beginIndex = students().size();
    const int endIndex = beginIndex + newNames.size() - 1;
    beginInsertRows(QModelIndex(), beginIndex, endIndex);
    manager->importStudents(newNames);
    for(int i = beginIndex; i <= endIndex; i++)
        rowToIndex.append(i);
    endInsertRows();
}

void ClassTableModel::removeStudent(int displayIndex)
{
    beginRemoveRows(QModelIndex(), displayIndex, displayIndex);

    // 从显示表格中的索引获得底层索引
    const int realIndex = rowToIndex.at(displayIndex);
    manager->removeStudent(realIndex);
    // 重建整个rowToIndex表
    rowToIndex.resize(students().size());
    for(int i = 0; i < students().size(); i++)
        rowToIndex[students().at(i).displayOrder - 1] = i;
    /* 原增量更新实现过于复杂，故弃用
    // 将显示顺序在其后面的学生displayOrder向前移一个
    // 要得到相同学生的索引，比原来更靠前一行
    for(int i = displayIndex; i < rowToIndex.size() - 1; i++)
        rowToIndex[i] = rowToIndex[i + 1];
    rowToIndex.removeLast();
    // 真实索引在其后面的学生索引向前移一个
    for(int& i : rowToIndex)
        if(i > realIndex)
            i--;
    */

    endRemoveRows();
}

int ClassTableModel::displayRowForStudent(int studentIndex) const
{
    return rowToIndex.indexOf(studentIndex);
}

// 积分记录编辑相关函数
void ClassTableModel::newScoreRecord(const QString &newRecordName)
{
    manager->newRecord(newRecordName);
    showColumnWrapper(CurrentRecord);
    showColumnWrapper(CurrentTemplate);
}

void ClassTableModel::activateTemplate(int index)
{
    manager->activateTemplate(index);
    if(rowCount() == 0)
        return;
    const QModelIndex topLeft = this->index(0, CurrentTemplate);
    const QModelIndex bottomRight = this->index(rowCount() - 1, CurrentTemplate);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::ForegroundRole});
}

void ClassTableModel::saveScoreRecord()
{
    manager->saveRecord();
    hideColumnWrapper(CurrentRecord);
    hideColumnWrapper(CurrentTemplate);
}

// 预览状态(由主窗口传入，模型只负责按此渲染)
void ClassTableModel::setRecordPreviewIndex(int index)
{
    previewRecordIndex = index;
    if(!isValidIndex(index))
    {
        hideColumnWrapper(RecordHistoryPreview);
        return;
    }
    showColumnWrapper(RecordHistoryPreview);
    if(rowCount() == 0)
        return;
    const QModelIndex topLeft = this->index(0, RecordHistoryPreview);
    const QModelIndex bottomRight = this->index(rowCount() - 1, RecordHistoryPreview);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::ForegroundRole});
}

void ClassTableModel::setTemplatePreviewIndex(int index)
{
    previewTemplateIndex = index;
    if(!isValidIndex(index))
    {
        hideColumnWrapper(ScoreTemplatePreview);
        return;
    }
    showColumnWrapper(ScoreTemplatePreview);
    if(rowCount() == 0)
        return;
    const QModelIndex topLeft = this->index(0, ScoreTemplatePreview);
    const QModelIndex bottomRight = this->index(rowCount() - 1, ScoreTemplatePreview);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole, Qt::ForegroundRole});
}

void ClassTableModel::resetTable()
{
    beginResetModel();
    rowToIndex.resize(students().size());
    // displayOrder取值为[1,size]，将其减一变成在[0,size-1]之间
    if(!students().isEmpty())
        for(int i = 0; i < students().size(); i++)
            rowToIndex[students().at(i).displayOrder - 1] = i;
    endResetModel();
    showColumnWrapper(StudentName);
    showColumnWrapper(GroupNumber);
    hideColumnWrapper(CurrentRecord);
    hideColumnWrapper(CurrentTemplate);
    hideColumnWrapper(RecordHistoryPreview);
    hideColumnWrapper(ScoreTemplatePreview);
    previewRecordIndex = INVALID_INDEX;
    previewTemplateIndex = INVALID_INDEX;
}

void ClassTableModel::setStudentEditable(bool editable)
{
    canEditStudent = editable;
    emit dataChanged(index(0,StudentName),index(rowCount()-1,StudentName),{Qt::EditRole});
}

void ClassTableModel::setGroupEditable(bool editable)
{
    canEditGroup = editable;
    emit dataChanged(index(0,GroupNumber),index(rowCount()-1,GroupNumber),{Qt::EditRole});
}

bool ClassTableModel::isValidModelIndex(const QModelIndex &index) const
{
    return index.isValid() &&
           columnIsVisible[index.column()] &&
           index.row() >= 0 &&
           index.row() <= students().size();
}

bool ClassTableModel::isValidIndex(int index) const
{
    return index != INVALID_INDEX;
}

void ClassTableModel::showColumnWrapper(int column)
{
    columnIsVisible[column] = true;
    emit showColumnRequested(column);
}

void ClassTableModel::hideColumnWrapper(int column)
{
    columnIsVisible[column] = false;
    emit hideColumnRequested(column);
}

QBrush ClassTableModel::getScoreColor(int scoreValue)
{
    if(scoreValue > 0)
        return QBrush(QColor(39, 174, 96));
    else if(scoreValue < 0)
        return QBrush(QColor(231, 76, 60));
    else
        return QBrush(QColor(127, 140, 141));
}