#pragma once

#include "CardHost.h"

#include <QColor>
#include <QVariantAnimation>
#include <QWidget>

class FlowLayout;
class StudentCard;
class QLabel;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;

//小组容器：标题行加组内卡片的流式排列，同时作为拖放落点
class GroupCard : public QWidget, public CardHost
{
    Q_OBJECT
public:
    explicit GroupCard(qint32 groupNumber, QWidget* parent = nullptr);

    qint32 groupNumber() const;
    int cardCount() const;

    FlowLayout* cardLayout() const override;
    QColor cardColor() const override;
    void insertCard(StudentCard* card, int index) override;
    void removeCard(StudentCard* card) override;
    void refreshHeader() override;

    //拖动开始与结束：占位槽先在卡片原地顶住位置，落下或取消后再撤除
    void beginDrag(StudentCard* card);
    void endDrag();

signals:
    void studentDropped(int studentIndex, int insertIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    int dropIndexAt(const QPoint& pos) const;
    void setPlaceholder(const QSize& cardSize, int index);
    void clearPlaceholder();
    void startHighlightAnimation(bool highlighted);

    qint32 m_groupNumber;
    QColor m_groupColor; //小组固有颜色，构造时算一次
    QLabel* m_titleLabel = nullptr;
    QLabel* m_countLabel = nullptr;
    FlowLayout* m_cardLayout = nullptr;
    QWidget* m_placeholder = nullptr;
    QVariantAnimation* m_highlightAnimation = nullptr;
    qreal m_highlightProgress = 0.0;
};
