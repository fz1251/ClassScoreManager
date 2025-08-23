#include "classtablemodal.h"

ClassTableModal::ClassTableModal(QObject *parent,ClassDataManager* dataManager)
    : QAbstractTableModel(parent)
    , manager(dataManager)
    , students(manager->getStudents())
    , templates(manager->getTemplates())
    , records(manager->getRecords())
    , templateIndex(-1)
    , recordIndex(-1)
    , activatedTemplateIndex(-1)
{
}

int ClassTableModal::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return students.size();
}

int ClassTableModal::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if(manager->hasData())
        return 6;
    else
        return 0;
}

QVariant ClassTableModal::data(const QModelIndex &index, int role) const
{
    if(!validateModalIndex(index))
        return QVariant();
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
    {
        switch(index.column())
        {
        case StudentName:
            return students.at(rowToIndex[index.row()]).name;
        case GroupNumber:
        {
            const int groupNum=students.at(rowToIndex[index.row()]).groupNumber;
            if(groupNum==StudentInfo::NoGroup)
                return QObject::tr("未分组");
            else
                return groupNum;
        }
        case CurrentRecord:
            return manager->getTempRecord().at(rowToIndex[index.row()]);
        case CurrentTemplate:
            if(activatedTemplateIndex>=0)
                return templates.at(activatedTemplateIndex).values.at(rowToIndex[index.row()]);
            else
                return 0;
        case RecordHistoryPreview:
            return records.at(recordIndex).scores.at(rowToIndex[index.row()]);
        case ScoreTemplatePreview:
            return templates.at(templateIndex).values.at(rowToIndex[index.row()]);
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
            return getScoreColor(manager->getTempRecord().value(rowToIndex[index.row()],0));
        case CurrentTemplate:
            if(activatedTemplateIndex>=0)
                return getScoreColor(templates.at(activatedTemplateIndex).values.at(rowToIndex[index.row()]));
            else
                return getScoreColor(0);
        case RecordHistoryPreview:
            return getScoreColor(records.at(recordIndex).scores.at(rowToIndex[index.row()]));
        case ScoreTemplatePreview:
            return getScoreColor(templates.at(templateIndex).values.at(rowToIndex[index.row()]));
        default:
            return QVariant();
        }
    }
    case Qt::CheckStateRole:
        if(index.column()==GroupNumber)
        {
            int groupNum=students.at(rowToIndex[index.row()]).groupNumber;
            if(groupNum==StudentInfo::NoGroup)
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

QVariant ClassTableModal::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role!=Qt::DisplayRole)
        return QVariant();
    if(orientation==Qt::Horizontal)
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
        return QString::number(section+1);
    }
}

bool ClassTableModal::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!validateModalIndex(index))
        return false;
    if(role==Qt::EditRole)
    {
        switch(index.column())
        {
        case StudentName:
        {
            manager->studentAt(rowToIndex[index.row()]).name=value.toString();
            return true;
        }
        case GroupNumber:
        {
            bool ok;
            const int val=value.toInt(&ok);
            if(ok&&val>=0)
            {
                manager->studentAt(rowToIndex[index.row()]).groupNumber=val;
                return true;
            }
            else
            {
                return false;
            }
        }
        case CurrentRecord:
        {
            bool ok;
            const int val=value.toInt(&ok);
            if(ok)
                manager->getTempRecord()[rowToIndex[index.row()]]=val;
            return ok;
        }
        case ScoreTemplatePreview:
        {
            bool ok;
            const int val=value.toInt(&ok);
            if(ok)
                manager->templateAt(templateIndex).values[rowToIndex[index.row()]]=val;
            return ok;
        }
        default:
            return false;
        }
    }
    else if(role==Qt::CheckStateRole)
    {
        int checked=value.toInt();
        int& groupNum=manager->studentAt(rowToIndex[index.row()]).groupNumber;
        if(checked)
            groupNum=0;
        else
            groupNum=StudentInfo::NoGroup;
        return true;
    }
    return false;
}

Qt::ItemFlags ClassTableModal::flags(const QModelIndex &index) const
{
    if(!validateModalIndex(index))
        return Qt::ItemIsEnabled|Qt::ItemIsDropEnabled;
    Qt::ItemFlags baseFlag=Qt::ItemIsEnabled|Qt::ItemIsSelectable|Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled;
    switch(index.column())
    {
    case StudentName:
    case CurrentRecord:
    case ScoreTemplatePreview:
        baseFlag|=Qt::ItemIsEditable;
        break;
    case GroupNumber:
        baseFlag|=Qt::ItemIsUserCheckable;
        if(students.at(rowToIndex[index.row()]).groupNumber!=StudentInfo::NoGroup)
            baseFlag|=Qt::ItemIsEditable;
        break;
    default:
        break;
    }
    return baseFlag;
}

Qt::DropActions ClassTableModal::supportedDropActions() const
{
    return Qt::MoveAction;
}

QMimeData* ClassTableModal::mimeData(const QModelIndexList &indexes) const
{
    if(indexes.isEmpty())
        return nullptr;
    QMimeData* mimeData=new QMimeData;
    QByteArray encodedData;
    QDataStream stream(&encodedData,QIODevice::WriteOnly);
    stream<<indexes.first().row();
    mimeData->setData(moveRowDataType,encodedData);
    return mimeData;
}

bool ClassTableModal::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);
    if(action==Qt::MoveAction&&data->hasFormat(moveRowDataType))
        return true;
    else
        return false;
}

bool ClassTableModal::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(column);
    if(action!=Qt::MoveAction)
        return false;
    if(row==-1)
        return false;
    if(!data->hasFormat(moveRowDataType))
        return false;
    QByteArray encodedData=data->data(moveRowDataType);
    QDataStream stream(&encodedData,QIODevice::ReadOnly);
    int sourceRow;
    stream>>sourceRow;
    beginMoveRows(parent,sourceRow,sourceRow,parent,row);
    if(row>sourceRow)
    {
        for(int i=sourceRow;i<row-1;i++)
        {
            std::swap(manager->studentAt(rowToIndex[i]).displayOrder,
                      manager->studentAt(rowToIndex[i+1]).displayOrder);
            std::swap(rowToIndex[i],rowToIndex[i+1]);
        }
    }
    else
    {
        for(int i=sourceRow;i>row;i--)
        {
            std::swap(manager->studentAt(rowToIndex[i]).displayOrder,
                      manager->studentAt(rowToIndex[i+1]).displayOrder);
            std::swap(rowToIndex[i],rowToIndex[i-1]);
        }
    }
    endMoveRows();
    return true;
}

void ClassTableModal::newScoreRecord(const QString &newRecordName)
{
    manager->newRecord(newRecordName);
    activatedTemplateIndex=-1;
    showColumnWrapper(CurrentRecord);
    showColumnWrapper(CurrentTemplate);
}

void ClassTableModal::activateTemplate(int index)
{
    activatedTemplateIndex=index;
}

void ClassTableModal::saveScoreRecord()
{
    manager->saveRecord(activatedTemplateIndex);
    activatedTemplateIndex=-1;
    hideColumnWrapper(CurrentRecord);
    hideColumnWrapper(CurrentTemplate);
}

void ClassTableModal::newScoreTemplate(const ScoreTemplate& newTemplate)
{
    manager->addTemplate(newTemplate);
    showScoreTemplate(manager->getTemplates().count()-1);
}

void ClassTableModal::setTemplateDescription(const QString &newDescription)
{
    if(templateIndex>=0)
        manager->templateAt(templateIndex).description=newDescription;
}

int ClassTableModal::getCurTemplateIndex() const
{
    return templateIndex;
}

int ClassTableModal::getCurRecordIndex() const
{
    return recordIndex;
}

void ClassTableModal::showRecordHistory(int index)
{
    recordIndex=index;
    showColumnWrapper(RecordHistoryPreview);
}

void ClassTableModal::hideRecordHistory()
{
    recordIndex=-1;
    hideColumnWrapper(RecordHistoryPreview);
}

void ClassTableModal::showScoreTemplate(int index)
{
    templateIndex=index;
    showColumnWrapper(ScoreTemplatePreview);
}

void ClassTableModal::hideScoreTemplate()
{
    templateIndex=-1;
    hideColumnWrapper(ScoreTemplatePreview);
}

void ClassTableModal::deleteRecordHistory(int index)
{
    if(index==recordIndex)
        hideRecordHistory();
    else if(index<recordIndex)
        recordIndex--;
    manager->removeRecord(index);
}

void ClassTableModal::deleteScoreTemplate(int index)
{
    if(index==templateIndex)
        hideScoreTemplate();
    else if(index<templateIndex)
        templateIndex--;
    if(activatedTemplateIndex==index)
        activatedTemplateIndex=-1;
    else if(index<activatedTemplateIndex)
        activatedTemplateIndex--;
    manager->removeTemplate(index);
}

void ClassTableModal::resetTable()
{
    beginResetModel();
    rowToIndex.resize(students.size());
    if(!students.isEmpty())
        for(int i=0;i<students.size();i++)
            rowToIndex[students.at(i).displayOrder-1]=i;
    endResetModel();
    showColumnWrapper(StudentName);
    showColumnWrapper(GroupNumber);
    hideColumnWrapper(CurrentRecord);
    hideColumnWrapper(CurrentTemplate);
    hideColumnWrapper(RecordHistoryPreview);
    hideColumnWrapper(ScoreTemplatePreview);
}

bool ClassTableModal::validateModalIndex(const QModelIndex &index) const
{
    return index.isValid()&&columnIsVisible[index.column()];
}

void ClassTableModal::showColumnWrapper(int column)
{
    columnIsVisible[column]=true;
    emit showColumnRequested(column);
}

void ClassTableModal::hideColumnWrapper(int column)
{
    columnIsVisible[column]=false;
    emit hideColumnRequested(column);
}

QBrush ClassTableModal::getScoreColor(int scoreValue)
{
    if(scoreValue>0)
        return QBrush(QColor(39,174,96));
    else if(scoreValue<0)
        return QBrush(QColor(231,76,60));
    else
        return QBrush(QColor(127,140,141));
}
