#pragma once

#include <QColor>

class FlowLayout;
class StudentCard;

//卡片容器的统一接口：未分组面板与小组卡片都实现它，小组管理页据此搬移卡片
class CardHost
{
public:
    virtual ~CardHost() = default;
    virtual FlowLayout* cardLayout() const = 0;
    //容器内学生卡片的底色：小组用组色，未分组面板用中性色
    virtual QColor cardColor() const = 0;
    //index为插入位置，负数表示追加到末尾
    virtual void insertCard(StudentCard* card, int index) = 0;
    virtual void removeCard(StudentCard* card) = 0;
    //成员增减后更新标题
    virtual void refreshHeader() = 0;
};
