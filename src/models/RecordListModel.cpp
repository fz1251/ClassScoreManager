#include "RecordListModel.h"

RecordListModel::RecordListModel(ClassDataManager* dataManager, QObject* parent)
    : QAbstractListModel(parent)
    , manager(dataManager)
{
    Q_ASSERT(dataManager);
    connect(manager, &ClassDataManager::recordsChanged,
            this, [this](){ resetModel(); });
}

int RecordListModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;
    return manager->getRecords().size();
}

QVariant RecordListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    if(role == Qt::DisplayRole)
        return manager->getRecordList().at(index.row());
    return QVariant();
}

void RecordListModel::resetModel()
{
    beginResetModel();
    endResetModel();
}
