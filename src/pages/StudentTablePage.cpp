#include "StudentTablePage.h"
#include "StudentTableSidebar.h"
#include <QAbstractItemView>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

StudentTablePage::StudentTablePage(QWidget *parent)
    : QWidget(parent)
{
    // 页面根布局：表格与右侧边栏左右分栏
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(this);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    rootLayout->addWidget(splitter);

    // 学生表格：支持内部拖动排序与整行选择
    m_tableView = new QTableView(splitter);
    m_tableView->setAcceptDrops(true);
    m_tableView->setDragEnabled(true);
    m_tableView->setDragDropOverwriteMode(false);
    m_tableView->setDragDropMode(QAbstractItemView::InternalMove);
    m_tableView->setDefaultDropAction(Qt::MoveAction);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    splitter->addWidget(m_tableView);

    // 右侧边栏：文件信息、表格选项、学生操作、记录与模板折叠块
    m_sidebar = new StudentTableSidebar(splitter);
    splitter->addWidget(m_sidebar);
}

QTableView *StudentTablePage::tableView() const
{
    return m_tableView;
}

StudentTableSidebar *StudentTablePage::sidebar() const
{
    return m_sidebar;
}
