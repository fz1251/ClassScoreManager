#include "GroupBoardPage.h"

#include "../core/ClassDataManager.h"
#include "../widgets/CardHost.h"
#include "../widgets/DragAutoScroller.h"
#include "../widgets/FlowLayout.h"
#include "../widgets/GroupCard.h"
#include "../widgets/StudentCard.h"
#include "../widgets/UngroupedPanel.h"
#include "../windows/GroupNumberDialog.h"

#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>
#include <algorithm>

namespace
{
constexpr int kUngroupedPanelWidth = 208; //右侧固定宽度：最长卡片、悬停留白与滚动条都能放下
constexpr int kGroupAreaMinWidth = 220;   //小组区至少放得下一个小组容器和一张卡片
//卡片信息密度低，自动滚动的速度要更高
constexpr DragAutoScroller::Config kAutoScrollConfig{16, 60, 800};
}

GroupBoardPage::GroupBoardPage(ClassDataManager* manager, QWidget* parent)
    : QWidget(parent)
    , m_manager(manager)
{
    //右侧留白让未分组栏成为一张与其他卡片同套圆角的栏；小组区贴靠左侧
    auto* rootLayout = new QHBoxLayout(this);
    //顶部 4px 与搜索窗格、记录页工具栏保持同一内容基线；底部不留白，交给窗口裁剪
    rootLayout->setContentsMargins(0, 4, 12, 0);
    rootLayout->setSpacing(0);

    m_ungroupedPanel = new UngroupedPanel(this);
    connect(m_ungroupedPanel, &UngroupedPanel::studentDropped, this, &GroupBoardPage::handleUngroupDrop);

    //小组区：小组容器自上而下排列，每个容器占满整行
    m_groupScrollArea = new QScrollArea(this);
    m_groupScrollArea->setWidgetResizable(true);
    m_groupScrollArea->setFrameShape(QFrame::NoFrame);
    m_groupScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_groupScrollArea->setMinimumWidth(kGroupAreaMinWidth);

    m_groupContainer = new QWidget;
    m_groupLayout = new FlowLayout(m_groupContainer, 12, 10, 10);
    m_groupLayout->setSingleColumn(true);
    //顶部不再叠加 12px，卡片顶边由页面根布局的 4px 决定；底部不留白，交给窗口裁剪
    m_groupLayout->setContentsMargins(12, 0, 12, 0);
    m_groupScrollArea->setWidget(m_groupContainer);

    //小组区占满剩余宽度；未分组栏固定贴右，顶边与小组区共用页面根布局的 4px
    rootLayout->addWidget(m_groupScrollArea, 1);
    auto* ungroupedColumn = new QWidget(this);
    ungroupedColumn->setFixedWidth(kUngroupedPanelWidth);
    auto* ungroupedLayout = new QVBoxLayout(ungroupedColumn);
    ungroupedLayout->setContentsMargins(0, 0, 0, 0);
    ungroupedLayout->setSpacing(0);
    m_ungroupedPanel->setFixedWidth(kUngroupedPanelWidth);
    ungroupedLayout->addWidget(m_ungroupedPanel, 1);
    rootLayout->addWidget(ungroupedColumn);

    //小组列表在拖动时贴边自动滚动，与主界面表格共用同一套手感；未分组栏只是容器，不需要滚动
    m_groupAutoScroller = new DragAutoScroller(m_groupScrollArea, kAutoScrollConfig, this);
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

void GroupBoardPage::revealStudent(int studentIndex)
{
    StudentCard* card = m_cards.value(studentIndex);
    if(!card)
        return;
    if(auto* panel = dynamic_cast<UngroupedPanel*>(card->host()))
        panel->revealCard(card);
    else
        m_groupScrollArea->ensureWidgetVisible(card, 0, 0);
    card->playEnterAnimation();
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
                m_groupAutoScroller->start();
            });
    connect(card, &StudentCard::dragFinished, this,
            [this, card]()
            {
                if(auto* group = dynamic_cast<GroupCard*>(card->host()))
                    group->endDrag();
                m_groupAutoScroller->stop();
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
