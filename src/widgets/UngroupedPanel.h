#pragma once

#include "CardHost.h"

#include <QVariantAnimation>
#include <QWidget>

class FlowLayout;
class StudentCard;
class QLabel;
class QScrollArea;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;

//左栏：未分组学生列表，同时是"取消分组"的拖放落点；栏面、表头与分隔线自绘
class UngroupedPanel : public QWidget, public CardHost
{
    Q_OBJECT
public:
    explicit UngroupedPanel(QWidget* parent = nullptr);

    FlowLayout* cardLayout() const override;
    QColor cardColor() const override;
    void insertCard(StudentCard* card, int index) override;
    void removeCard(StudentCard* card) override;
    void refreshHeader() override;
    void clearCards();

    int cardCount() const;

signals:
    void studentDropped(int studentIndex);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void startMaskAnimation(bool visible);
    void updateMaskProgress(qreal progress);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QLabel* m_headerLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_cardContainer = nullptr;
    FlowLayout* m_cardLayout = nullptr;
    QWidget* m_mask = nullptr;

    QVariantAnimation* m_maskAnimation = nullptr;
    qreal m_maskProgress = 0.0;
};
