#include "RecordListPage.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

RecordListPage::RecordListPage(QWidget* parent)
    : QWidget(parent)
{
    // 页面根布局：顶部工具按钮行，下方列表
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 顶部工具行：新建、导出、删除选中、清空
    auto* toolbarWidget = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);
    toolbarLayout->setSpacing(6);

    createButton = new QPushButton(tr("新建记录"), toolbarWidget);
    exportButton = new QPushButton(tr("导出"), toolbarWidget);
    deleteButton = new QPushButton(tr("删除选中"), toolbarWidget);
    clearButton = new QPushButton(tr("清空"), toolbarWidget);
    toolbarLayout->addWidget(createButton);
    toolbarLayout->addWidget(exportButton);
    toolbarLayout->addWidget(deleteButton);
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addStretch();
    rootLayout->addWidget(toolbarWidget);

    // 积分记录列表：支持多选
    m_listView = new QListView(this);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(m_listView, 1);

    connect(createButton, &QPushButton::clicked, this, &RecordListPage::createRequested);
    connect(exportButton, &QPushButton::clicked, this, &RecordListPage::exportRequested);
    connect(deleteButton, &QPushButton::clicked, this, &RecordListPage::deleteSelectedRequested);
    connect(clearButton, &QPushButton::clicked, this, &RecordListPage::clearRequested);
}

QListView* RecordListPage::listView() const
{
    return m_listView;
}

QList<QModelIndex> RecordListPage::selectedRows() const
{
    if(!m_listView->selectionModel())
        return {};
    return m_listView->selectionModel()->selectedRows();
}

void RecordListPage::setListModel(QAbstractItemModel* model)
{
    m_listView->setModel(model);
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &RecordListPage::updateToolButtonStates);
    //列表模型每次变化都是整体重置，重置后选中可能被静默清空，这里按模型自身的变化刷新
    connect(model, &QAbstractItemModel::modelReset, this, &RecordListPage::updateToolButtonStates);
    updateToolButtonStates();
}

void RecordListPage::setNewRecordEnabled(bool enabled)
{
    createButton->setEnabled(enabled);
}

void RecordListPage::updateToolButtonStates()
{
    const QAbstractItemModel* model = m_listView->model();
    const bool hasRows = model && model->rowCount() > 0;
    const bool hasSelection = !selectedRows().isEmpty();
    exportButton->setEnabled(hasSelection);
    deleteButton->setEnabled(hasSelection);
    clearButton->setEnabled(hasRows);
}
