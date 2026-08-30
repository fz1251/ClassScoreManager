#include "StudentTablePage.h"
#include "fluentui3styleproperties.h"
#include <QAbstractItemView>
#include <QCheckBox>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

StudentTablePage::StudentTablePage(QWidget *parent)
    : QWidget(parent)
{
    // 页面根布局：表格与开关面板左右分栏
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *splitter = new QSplitter(this);
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

    // 右侧开关面板：拖动排序、姓名编辑、小组编辑
    auto *rightPanelWidget = new QWidget(splitter);
    auto *rightPanelLayout = new QVBoxLayout(rightPanelWidget);
    rightPanelLayout->setContentsMargins(-1, -1, 13, -1);

    auto *dragStudentSwitch = new QCheckBox(tr("拖动排序学生"), rightPanelWidget);
    dragStudentSwitch->setCheckState(Qt::Checked);
    dragStudentSwitch->setProperty(SwitchStyleProperty, true);
    rightPanelLayout->addWidget(dragStudentSwitch);

    auto *editNameSwitch = new QCheckBox(tr("学生姓名编辑"), rightPanelWidget);
    editNameSwitch->setCheckState(Qt::Checked);
    editNameSwitch->setProperty(SwitchStyleProperty, true);
    rightPanelLayout->addWidget(editNameSwitch);

    auto *editGroupSwitch = new QCheckBox(tr("小组信息编辑"), rightPanelWidget);
    editGroupSwitch->setCheckState(Qt::Checked);
    editGroupSwitch->setProperty(SwitchStyleProperty, true);
    rightPanelLayout->addWidget(editGroupSwitch);

    rightPanelLayout->addStretch();
    splitter->addWidget(rightPanelWidget);

    connect(dragStudentSwitch, &QCheckBox::stateChanged, this, [this](int state) {
        emit dragSortChanged(state == Qt::Checked);
    });
    connect(editNameSwitch, &QCheckBox::stateChanged, this, [this](int state) {
        emit nameEditChanged(state == Qt::Checked);
    });
    connect(editGroupSwitch, &QCheckBox::stateChanged, this, [this](int state) {
        emit groupEditChanged(state == Qt::Checked);
    });
}

QTableView *StudentTablePage::tableView() const
{
    return m_tableView;
}
