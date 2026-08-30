#include "TemplateListPage.h"
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QListView>
#include <QPushButton>
#include <QVBoxLayout>

TemplateListPage::TemplateListPage(QWidget *parent)
    : QWidget(parent)
{
    // 页面根布局：顶部工具按钮行，下方列表
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // 顶部工具行：新建、删除选中、清空
    auto *toolbarWidget = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);
    toolbarLayout->setSpacing(6);

    createButton = new QPushButton(tr("新建模板"), toolbarWidget);
    deleteButton = new QPushButton(tr("删除选中"), toolbarWidget);
    clearButton = new QPushButton(tr("清空"), toolbarWidget);
    toolbarLayout->addWidget(createButton);
    toolbarLayout->addWidget(deleteButton);
    toolbarLayout->addWidget(clearButton);
    toolbarLayout->addStretch();
    rootLayout->addWidget(toolbarWidget);

    // 积分模板列表：支持多选
    m_listView = new QListView(this);
    m_listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(m_listView, 1);

    connect(createButton, &QPushButton::clicked, this, &TemplateListPage::createRequested);
    connect(deleteButton, &QPushButton::clicked, this, &TemplateListPage::deleteSelectedRequested);
    connect(clearButton, &QPushButton::clicked, this, &TemplateListPage::clearRequested);
}

QListView *TemplateListPage::listView() const
{
    return m_listView;
}

QList<QModelIndex> TemplateListPage::selectedRows() const
{
    if(!m_listView->selectionModel())
        return {};
    return m_listView->selectionModel()->selectedRows();
}

void TemplateListPage::setListModel(QAbstractItemModel *model)
{
    m_listView->setModel(model);
    connect(m_listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &TemplateListPage::updateToolButtonStates);
    updateToolButtonStates();
}

void TemplateListPage::updateToolButtonStates()
{
    const bool hasSelection = !selectedRows().isEmpty();
    deleteButton->setEnabled(hasSelection);
}
