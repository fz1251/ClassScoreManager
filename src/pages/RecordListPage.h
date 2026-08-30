#pragma once

#include <QModelIndex>
#include <QWidget>

class QAbstractItemModel;
class QListView;
class QPushButton;

class RecordListPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecordListPage(QWidget *parent = nullptr);

    QListView *listView() const;
    QList<QModelIndex> selectedRows() const;
    void setListModel(QAbstractItemModel *model);
    void setNewRecordEnabled(bool enabled);

signals:
    void createRequested();
    void exportRequested();
    void deleteSelectedRequested();
    void clearRequested();

private:
    void updateToolButtonStates();

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QListView *m_listView = nullptr;
    QPushButton *createButton = nullptr;
    QPushButton *exportButton = nullptr;
    QPushButton *deleteButton = nullptr;
    QPushButton *clearButton = nullptr;
};
