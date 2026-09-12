#pragma once

#include "../core/ClassDataManager.h"
#include <QString>
#include <QVector>
#include <QAbstractListModel>

// 学生搜索列表模型：按姓名匹配学生，结果按表格显示顺序排列
class StudentSearchModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit StudentSearchModel(ClassDataManager* dataManager, QObject* parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // 结果行对应的学生索引；越界时返回-1
    int studentIndexAt(int row) const;

public slots:
    void setFilter(const QString &text);
    void refresh();

private:
    ClassDataManager* manager;
    QString filter;
    QVector<int> matchedIndices; // 学生索引，按displayOrder排序
};
