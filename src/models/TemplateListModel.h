#pragma once

#include "../core/ClassDataManager.h"

#include <QAbstractListModel>

// 积分模板列表模型：作为 ClassDataManager 中模板数据的只读投影
class TemplateListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit TemplateListModel(ClassDataManager* dataManager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

private:
    void resetModel();

    ClassDataManager* manager;
};
