#pragma once

#include "CardHost.h"

#include <QGroupBox>
#include <QVariantAnimation>

class FlowLayout;
class StudentCard;
class QScrollArea;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;

//左栏：未分组学生列表，同时是"取消分组"的拖放落点；边框与标题由全局样式绘制
class UngroupedPanel : public QGroupBox, public CardHost
{
    Q_OBJECT
public:
    explicit UngroupedPanel(QWidget* parent = nullptr);

    FlowLayout* cardLayout() const override;
    void insertCard(StudentCard* card, int index) override;
    void removeCard(StudentCard* card) override;
    void refreshHeader() override;
    void clearCards();

    QScrollArea* scrollArea() const;
    int cardCount() const;

signals:
    void studentDropped(int studentIndex);

protected:
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void startMaskAnimation(bool visible);
    void updateMaskProgress(qreal progress);

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_cardContainer = nullptr;
    FlowLayout* m_cardLayout = nullptr;
    QWidget* m_mask = nullptr;
    QVariantAnimation* m_maskAnimation = nullptr;
    qreal m_maskProgress = 0.0;
};
