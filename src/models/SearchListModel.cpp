#include "SearchListModel.h"

#include <algorithm>

SearchListModel::SearchListModel(ClassDataManager* dataManager, QObject* parent)
    : QAbstractListModel(parent)
    , manager(dataManager)
{
    Q_ASSERT(dataManager);
    // 学生条目的增删/排序/改名由表格模型的信号驱动（见 MainWindow）：manager 的 studentsChanged
    // 也会在只改小组号时发出，若在这里刷新，拖放分组会把搜索结果重置。
    // 记录与模板没有对应的表格模型，直接在模型内跟随 manager。
    connect(manager, &ClassDataManager::recordsChanged, this,
            [this]()
            {
                if(currentScope == SearchScope::Record)
                    refresh();
            });
    connect(manager, &ClassDataManager::templatesChanged, this,
            [this]()
            {
                if(currentScope == SearchScope::Template)
                    refresh();
            });

    rebuildEntries();
    applyFilter();
}

int SearchListModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid())
        return 0;
    return visibleRows.size();
}

QVariant SearchListModel::data(const QModelIndex& index, int role) const
{
    if(!index.isValid() || index.row() < 0 || index.row() >= visibleRows.size())
        return QVariant();
    const Entry& entry = entries.at(visibleRows.at(index.row()));
    if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
        return entry.text;
    return QVariant();
}

SearchScope SearchListModel::scope() const
{
    return currentScope;
}

void SearchListModel::setScope(SearchScope scope)
{
    if(currentScope == scope)
        return;
    beginResetModel();
    currentScope = scope;
    rebuildEntries();
    applyFilter();
    endResetModel();
}

void SearchListModel::setFilter(const QString& text)
{
    if(filterText == text)
        return;
    filterText = text;
    normalizedFilter = Pinyin::normalizeQuery(text);
    beginResetModel();
    applyFilter();
    endResetModel();
}

void SearchListModel::refresh()
{
    beginResetModel();
    rebuildEntries();
    applyFilter();
    endResetModel();
}

int SearchListModel::entryIndexAt(int row) const
{
    if(row < 0 || row >= visibleRows.size())
        return -1;
    return entries.at(visibleRows.at(row)).index;
}

void SearchListModel::rebuildEntries()
{
    entries.clear();
    switch(currentScope)
    {
        case SearchScope::Student:
        {
            const QVector<StudentInfo>& students = manager->getStudents();
            QVector<int> indices;
            indices.reserve(students.size());
            for(int i = 0; i < students.size(); ++i)
                indices.append(i);
            // 结果顺序与表格显示顺序一致；students 向量顺序在表格拖动排序后不再等于显示顺序
            std::sort(indices.begin(), indices.end(),
                      [&students](int left, int right)
                      {
                          return students.at(left).displayOrder < students.at(right).displayOrder;
                      });
            entries.reserve(indices.size());
            for(int index : indices)
            {
                Entry entry;
                entry.text = students.at(index).name;
                entry.index = index;
                entries.append(entry);
            }
            break;
        }
        case SearchScope::Record:
        {
            const int recordCount = manager->getRecords().size();
            entries.reserve(recordCount);
            for(int i = 0; i < recordCount; ++i)
            {
                Entry entry;
                entry.text = manager->getRecordDisplayName(i);
                entry.index = i;
                entries.append(entry);
            }
            break;
        }
        case SearchScope::Template:
        {
            const int templateCount = manager->getTemplates().size();
            entries.reserve(templateCount);
            for(int i = 0; i < templateCount; ++i)
            {
                Entry entry;
                entry.text = manager->getTemplateDisplayName(i);
                entry.index = i;
                entries.append(entry);
            }
            break;
        }
    }
}

void SearchListModel::applyFilter()
{
    visibleRows.clear();
    visibleRows.reserve(entries.size());
    if(normalizedFilter.isEmpty())
    {
        for(int i = 0; i < entries.size(); ++i)
            visibleRows.append(i);
        return;
    }

    for(int i = 0; i < entries.size(); ++i)
    {
        Entry& entry = entries[i];
        if(!entry.matcherReady)
        {
            entry.matcher = PinyinMatcher(entry.text);
            entry.matcherReady = true;
        }
        if(entry.matcher.matches(normalizedFilter))
            visibleRows.append(i);
    }
}
