#pragma once

#include <QHash>
#include <QWidget>

class CardHost;
class ClassDataManager;
class DragAutoScroller;
class FlowLayout;
class GroupCard;
class StudentCard;
class UngroupedPanel;
class QScrollArea;

//小组管理页：左侧按组号排列的小组容器，右侧未分组学生，拖放即完成分组
class GroupBoardPage : public QWidget
{
    Q_OBJECT
public:
    explicit GroupBoardPage(ClassDataManager* manager, QWidget* parent = nullptr);

    //搜索结果跳转：滚动到学生卡片并播放入场动画，未分组或组内都适用
    void revealStudent(int studentIndex);

public slots:
    //外部改动（打开/关闭文件、学生增删、表格页改小组号）后按数据重建
    void reload();

private:
    StudentCard* createCard(int studentIndex);
    GroupCard* ensureGroupCard(qint32 groupNumber);
    void removeGroupCardIfEmpty(GroupCard* card);
    void moveCardToHost(StudentCard* card, CardHost* target, int index);
    void handleGroupNumberEdit(int studentIndex);
    void handleGroupDrop(int studentIndex, GroupCard* target, int insertIndex);
    void handleUngroupDrop(int studentIndex);

    ClassDataManager* m_manager = nullptr;
    UngroupedPanel* m_ungroupedPanel = nullptr;
    QScrollArea* m_groupScrollArea = nullptr;
    QWidget* m_groupContainer = nullptr;
    FlowLayout* m_groupLayout = nullptr;
    QHash<qint32, GroupCard*> m_groupCards;
    QHash<int, StudentCard*> m_cards;
    DragAutoScroller* m_groupAutoScroller = nullptr;
};
