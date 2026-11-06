#pragma once

#include "../core/ClassDataManager.h"
#include "../core/Pinyin.h"

#include <QAbstractListModel>
#include <QString>
#include <QVector>

enum class SearchScope
{
    Student,
    Record,
    Template
};

// 搜索列表模型：一个实例承载一个搜索范围；范围切换时重建条目，只有非空查询才构建匹配器并过滤
class SearchListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit SearchListModel(ClassDataManager* dataManager, QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    SearchScope scope() const;
    // 切换搜索范围；同一范围直接返回
    void setScope(SearchScope scope);
    // 结果行对应的学生/记录/模板索引；越界时返回-1
    int entryIndexAt(int row) const;

public slots:
    void setFilter(const QString& text);
    void refresh();

private:
    struct Entry
    {
        QString text;
        int index = -1;
        PinyinMatcher matcher; // 首次非空查询时构建，清空关键字后仍保留
        bool matcherReady = false;
    };

    void rebuildEntries();
    void applyFilter();

    ClassDataManager* manager = nullptr;
    SearchScope currentScope = SearchScope::Student;
    QVector<Entry> entries;   // 当前范围的全部条目
    QVector<int> visibleRows; // 命中条目在 entries 中的下标
    QString filterText;
    QString normalizedFilter;
};
