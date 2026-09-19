#include "StudentSearchModel.h"

#include <algorithm>

StudentSearchModel::StudentSearchModel(ClassDataManager* dataManager, QObject* parent)
    : QAbstractListModel(parent)
    , manager(dataManager)
{
    Q_ASSERT(dataManager);
}

int StudentSearchModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return matchedIndices.size();
}

QVariant StudentSearchModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= matchedIndices.size())
        return QVariant();
    if(role == Qt::DisplayRole)
        return manager->getStudents().at(matchedIndices.at(index.row())).name;
    return QVariant();
}

int StudentSearchModel::studentIndexAt(int row) const
{
    if(row < 0 || row >= matchedIndices.size())
        return -1;
    return matchedIndices.at(row);
}

void StudentSearchModel::setFilter(const QString& text)
{
    filter = text;
    refresh();
}

void StudentSearchModel::refresh()
{
    beginResetModel();
    matchedIndices.clear();
    const QVector<StudentInfo>& students = manager->getStudents();
    const QString keyword = filter.trimmed();
    if(!keyword.isEmpty())
        for(int i = 0; i < students.size(); i++)
            if(students.at(i).name.contains(keyword, Qt::CaseInsensitive))
                matchedIndices.append(i);
    // 结果顺序与表格显示顺序保持一致
    std::sort(matchedIndices.begin(), matchedIndices.end(),
              [&students](int left, int right)
              {
                  return students.at(left).displayOrder < students.at(right).displayOrder;
              });
    endResetModel();
}
