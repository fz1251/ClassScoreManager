#include "GroupBoardPage.h"

#include "../core/ClassDataManager.h"
#include "../widgets/CardHost.h"
#include "../widgets/FlowLayout.h"
#include "../widgets/GroupCard.h"
#include "../widgets/StudentCard.h"
#include "../widgets/UngroupedPanel.h"
#include "../windows/GroupNumberDialog.h"

#include <QCursor>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTimer>
#include <algorithm>

namespace
{
constexpr int kUngroupedInitialWidth = 140; //左栏初始宽度，最小宽度由 UngroupedPanel 自己约束
constexpr int kGroupAreaMinWidth = 220;     //右栏至少放得下一个小组容器和一张卡片
constexpr int kAutoScrollIntervalMs = 16;   //约 60Hz，看不出台阶
constexpr int kAutoScrollEdge = 60;         //光标进入边缘这么多像素内开始自动滚动
constexpr qreal kAutoScrollMaxSpeed = 800;  //贴到边框时的滚动速度（px/s）
}

GroupBoardPage::GroupBoardPage(ClassDataManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_manager(manager)
{
    //左边给导航栏留出间距；四周留白让左栏成为一张与其他卡片同套圆角的栏
    auto* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 0, 12);
    rootLayout->setSpacing(0);

    m_ungroupedPanel = new UngroupedPanel(this);
    connect(m_ungroupedPanel, &UngroupedPanel::studentDropped, this, &GroupBoardPage::handleUngroupDrop);

    //右侧：小组容器自上而下排列，每个容器占满整行
    m_groupScrollArea = new QScrollArea(this);
    m_groupScrollArea->setWidgetResizable(true);
    m_groupScrollArea->setFrameShape(QFrame::NoFrame);
    m_groupScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_groupScrollArea->setMinimumWidth(kGroupAreaMinWidth);

    m_groupContainer = new QWidget;
    m_groupLayout = new FlowLayout(m_groupContainer, 12, 10, 10);
    m_groupLayout->setSingleColumn(true);
    m_groupScrollArea->setWidget(m_groupContainer);

    //左右分栏可拖动，两侧都不会被压死
    auto* splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(m_ungroupedPanel);
    splitter->addWidget(m_groupScrollArea);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({kUngroupedInitialWidth, kGroupAreaMinWidth * 2});
    rootLayout->addWidget(splitter);

    m_autoScrollTimer = new QTimer(this);
    m_autoScrollTimer->setInterval(kAutoScrollIntervalMs);
    connect(m_autoScrollTimer, &QTimer::timeout, this, &GroupBoardPage::updateAutoScroll);
}

void GroupBoardPage::reload()
{
    m_cards.clear();
    m_ungroupedPanel->clearCards();
    //组内卡片的父对象已在 insertCard 时被布局认领为小组卡片，删容器会连带删掉卡片
    for(GroupCard* groupCard : m_groupCards)
        delete groupCard;
    m_groupCards.clear();

    //先按组号建容器，容器顺序即组号顺序
    for(qint32 groupNumber : m_manager->groupNumbers())
        ensureGroupCard(groupNumber);

    const QVector<StudentInfo>& students = m_manager->getStudents();
    for(int i = 0; i < students.size(); i++)
        m_cards.insert(i, createCard(i));

    //组内顺序按数据中的小组内顺序排列
    for(qint32 groupNumber : m_manager->groupNumbers())
    {
        GroupCard* groupCard = m_groupCards.value(groupNumber);
        for(int studentIndex : m_manager->groupStudentIndices(groupNumber))
            groupCard->insertCard(m_cards.value(studentIndex), -1);
    }
    //未分组卡片按显示顺序排列，与表格行序保持一致（students 向量顺序在表格拖动排序后不再等于显示顺序）
    QVector<int> ungrouped;
    for(int i = 0; i < students.size(); i++)
        if(students.at(i).groupNumber == StudentInfo::NoGroup)
            ungrouped.append(i);
    std::sort(ungrouped.begin(), ungrouped.end(),
              [&students](int left, int right)
              {
                  return students.at(left).displayOrder < students.at(right).displayOrder;
              });
    for(int studentIndex : ungrouped)
        m_ungroupedPanel->insertCard(m_cards.value(studentIndex), -1);
}

StudentCard* GroupBoardPage::createCard(int studentIndex)
{
    auto* card = new StudentCard(studentIndex, m_manager->getStudents().at(studentIndex).name, this);
    connect(card, &StudentCard::groupNumberEditRequested, this, &GroupBoardPage::handleGroupNumberEdit);
    connect(card, &StudentCard::dragStarted, this,
            [this, card]()
            {
                //拿起卡片：源小组先用占位槽顶住它的位置，避免起步时布局先收拢再展开
                if(auto* group = dynamic_cast<GroupCard*>(card->host()))
                    group->beginDrag(card);
                m_autoScrollTimer->start();
            });
    connect(card, &StudentCard::dragFinished, this,
            [this, card]()
            {
                if(auto* group = dynamic_cast<GroupCard*>(card->host()))
                    group->endDrag();
                m_autoScrollTimer->stop();
            });
    return card;
}

GroupCard* GroupBoardPage::ensureGroupCard(qint32 groupNumber)
{
    if(m_groupCards.contains(groupNumber))
        return m_groupCards.value(groupNumber);

    auto* groupCard = new GroupCard(groupNumber, m_groupContainer);
    connect(groupCard, &GroupCard::studentDropped, this,
            [this, groupCard](int studentIndex, int insertIndex)
            {
                handleGroupDrop(studentIndex, groupCard, insertIndex);
            });

    //按组号升序插到已有的小组之间
    int index = 0;
    while(index < m_groupLayout->count())
    {
        auto* other = qobject_cast<GroupCard*>(m_groupLayout->itemAt(index)->widget());
        if(other && other->groupNumber() > groupNumber)
            break;
        index++;
    }
    m_groupLayout->insertWidget(index, groupCard);
    m_groupCards.insert(groupNumber, groupCard);
    return groupCard;
}

void GroupBoardPage::removeGroupCardIfEmpty(GroupCard* card)
{
    if(card->cardCount() > 0)
        return;
    //组里没有成员时小组不再存在
    m_groupCards.remove(card->groupNumber());
    m_groupLayout->removeWidget(card);
    card->hide();
    card->deleteLater();
}

void GroupBoardPage::moveCardToHost(StudentCard* card, CardHost* target, int index)
{
    CardHost* source = card->host();
    if(source == target)
        target->cardLayout()->removeWidget(card);
    else if(source)
        source->removeCard(card);
    target->insertCard(card, index);
}

void GroupBoardPage::handleGroupDrop(int studentIndex, GroupCard* target, int insertIndex)
{
    StudentCard* card = m_cards.value(studentIndex);
    if(!card)
        return;
    auto* sourceGroup = dynamic_cast<GroupCard*>(card->host());

    //落点与原来一致时只把卡片放回去，不播入场动画也不清理来源小组
    const bool moved = m_manager->moveStudentToGroup(studentIndex, target->groupNumber(), insertIndex);
    moveCardToHost(card, target, insertIndex);
    if(!moved)
        return;
    card->playEnterAnimation();
    if(sourceGroup)
        removeGroupCardIfEmpty(sourceGroup);
}

void GroupBoardPage::handleUngroupDrop(int studentIndex)
{
    StudentCard* card = m_cards.value(studentIndex);
    if(!card)
        return;
    auto* sourceGroup = dynamic_cast<GroupCard*>(card->host());

    if(!m_manager->moveStudentToGroup(studentIndex, StudentInfo::NoGroup))
        return;
    moveCardToHost(card, m_ungroupedPanel, -1);
    card->playEnterAnimation();
    if(sourceGroup)
        removeGroupCardIfEmpty(sourceGroup);
}

void GroupBoardPage::handleGroupNumberEdit(int studentIndex)
{
    const QVector<StudentInfo>& students = m_manager->getStudents();
    if(studentIndex < 0 || studentIndex >= students.size())
        return;
    StudentCard* card = m_cards.value(studentIndex);
    if(!card)
        return;
    const QString name = students.at(studentIndex).name;
    const qint32 currentGroup = students.at(studentIndex).groupNumber;

    GroupNumberDialog dialog(name, currentGroup == StudentInfo::NoGroup ? 0 : currentGroup, this);
    if(dialog.exec() != QDialog::Accepted)
        return;
    const qint32 groupNumber = dialog.getGroupNumber();
    if(groupNumber == currentGroup)
        return;

    auto* sourceGroup = dynamic_cast<GroupCard*>(card->host());
    GroupCard* target = ensureGroupCard(groupNumber);
    m_manager->moveStudentToGroup(studentIndex, groupNumber);
    moveCardToHost(card, target, -1);
    card->playEnterAnimation();

    if(sourceGroup)
        removeGroupCardIfEmpty(sourceGroup);
}

//光标贴近滚动区上下边缘时自动滚动：越靠边越快，便于拖到还没露出来的小组
void GroupBoardPage::scrollNearEdge(QScrollArea* scrollArea)
{
    QScrollBar* bar = scrollArea->verticalScrollBar();
    if(bar->maximum() <= bar->minimum())
        return;
    const QRect viewport = scrollArea->viewport()->rect();
    const QPoint pos = scrollArea->viewport()->mapFromGlobal(QCursor::pos());
    if(!viewport.contains(pos))
        return;

    //光标深入边缘区的比例：刚进边缘区是 0，贴到边框是 1
    const bool atTop = pos.y() < viewport.top() + kAutoScrollEdge;
    qreal intensity = 0.0;
    if(atTop)
        intensity = qreal(viewport.top() + kAutoScrollEdge - pos.y()) / kAutoScrollEdge;
    else if(pos.y() > viewport.bottom() - kAutoScrollEdge)
        intensity = qreal(pos.y() - viewport.bottom() + kAutoScrollEdge) / kAutoScrollEdge;
    if(intensity <= 0.0)
    {
        m_autoScrollRemainder = 0.0;
        return;
    }

    //速度按深入程度二次增长：边缘区里慢、贴边快，进出都连续，不需要额外的缓动
    //不足 1px 的零头累加到下一拍，否则低速段永远滚不动
    const qreal delta = kAutoScrollMaxSpeed * intensity * intensity * kAutoScrollIntervalMs / 1000.0 + m_autoScrollRemainder;
    const int whole = int(delta);
    m_autoScrollRemainder = delta - whole;
    if(whole > 0)
        bar->setValue(bar->value() + (atTop ? -whole : whole));
}

void GroupBoardPage::updateAutoScroll()
{
    scrollNearEdge(m_groupScrollArea);
    scrollNearEdge(m_ungroupedPanel->scrollArea());
}
