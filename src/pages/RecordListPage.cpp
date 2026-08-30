#include "RecordListPage.h"
#include <QAbstractItemView>
#include <QListView>
#include <QVBoxLayout>

RecordListPage::RecordListPage(QWidget *parent)
    : QWidget(parent)
{
    // 页面根布局：积分记录列表占满整页
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    m_listView = new QListView(this);
    m_listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    rootLayout->addWidget(m_listView);
}

QListView *RecordListPage::listView() const
{
    return m_listView;
}
