#pragma once

#include "../core/ClassDataManager.h"

#include <QAbstractListModel>

// 积分记录列表模型：作为 ClassDataManager 中记录数据的只读投影
class RecordListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RecordListModel(ClassDataManager* dataManager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
    void resetModel();

    ClassDataManager* manager;
};
