#include "StudentTablePage.h"

#include "../widgets/DragAutoScroller.h"
#include "StudentTableSidebar.h"

#include <QAbstractItemView>
#include <QEvent>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

namespace
{
//表格行矮、信息密度高，最大滚动速度降低，触发区稍宽
constexpr DragAutoScroller::Config kAutoScrollConfig{16, 80, 400};
}

StudentTablePage::StudentTablePage(QWidget* parent)
    : QWidget(parent)
{
    // 页面根布局：表格与右侧边栏左右分栏
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* splitter = new QSplitter(this);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setChildrenCollapsible(false);
    rootLayout->addWidget(splitter);

    // 学生表格：支持内部拖动排序与整行选择
    m_tableView = new QTableView(splitter);
    m_tableView->setMinimumWidth(320);
    m_tableView->setAcceptDrops(true);
    m_tableView->setDragEnabled(true);
    m_tableView->setDragDropOverwriteMode(false);
    m_tableView->setDragDropMode(QAbstractItemView::InternalMove);
    m_tableView->setDefaultDropAction(Qt::MoveAction);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    // 选中单元格后直接敲键盘不再进入编辑，避免误覆盖内容；双击或按 F2/回车才编辑
    m_tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    splitter->addWidget(m_tableView);

    // 表格滚动：按像素滚动；右侧滚动条不显示，滚动交给滚轮与拖动时的边缘自动滚动
    // item view 默认按整行滚动，滚动条的值是行号，同样的速度数字含义会变成“行/秒”，改成按像素后单位才和小组页一致
    m_tableView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_tableView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 关掉 Qt 自带的自动滚动，避免两套机制叠加
    m_tableView->setAutoScroll(false);
    m_dragAutoScroller = new DragAutoScroller(m_tableView, kAutoScrollConfig, this);
    m_tableView->viewport()->installEventFilter(this);

    // 右侧边栏：文件信息、表格选项、学生操作、记录与模板折叠块
    m_sidebar = new StudentTableSidebar(splitter);
    splitter->addWidget(m_sidebar);
}

QTableView* StudentTablePage::tableView() const
{
    return m_tableView;
}

StudentTableSidebar* StudentTablePage::sidebar() const
{
    return m_sidebar;
}

// 拖动进入表格时启动自动滚动，移出视口或落下时停止；每拍滚多少由计时器按光标位置决定
bool StudentTablePage::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == m_tableView->viewport())
    {
        switch(event->type())
        {
            case QEvent::DragEnter:
            case QEvent::DragMove:
                m_dragAutoScroller->start();
                break;
            case QEvent::DragLeave:
            case QEvent::Drop:
                m_dragAutoScroller->stop();
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
}
