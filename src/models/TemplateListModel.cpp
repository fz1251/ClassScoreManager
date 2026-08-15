#include "TemplateListModel.h"

TemplateListModel::TemplateListModel(ClassDataManager* dataManager, QObject* parent)
    : QAbstractListModel(parent)
    , manager(dataManager)
{
    Q_ASSERT(dataManager);
    connect(manager, &ClassDataManager::templatesChanged,
            this, [this](){ resetModel(); });
}

int TemplateListModel::rowCount(const QModelIndex &parent) const
{
    if(parent.isValid())
        return 0;
    return manager->getTemplates().size();
}

QVariant TemplateListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= rowCount())
        return QVariant();
    if(role == Qt::DisplayRole)
        return manager->getTemplates().at(index.row()).description;
    return QVariant();
}

void TemplateListModel::resetModel()
{
    beginResetModel();
    endResetModel();
}
