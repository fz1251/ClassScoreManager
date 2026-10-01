#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QLayout>
#include <QList>
#include <QRect>
#include <QVector>

class QTimer;

//流式布局：子控件从左到右排列、放不下就换行；重排时用弹簧积分把子控件平滑移到新位置
class FlowLayout : public QLayout
{
    Q_OBJECT
public:
    explicit FlowLayout(QWidget* parent = nullptr, int margin = 0, int hSpacing = 8, int vSpacing = 8);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect& rect) override;

    int horizontalSpacing() const;
    int verticalSpacing() const;

    //每行只放一个子控件
    void setSingleColumn(bool enabled);
    void insertWidget(int index, QWidget* widget);
    void removeWidget(QWidget* widget);

    //算出各子项的目标矩形，长度等于 count()，不参与排布的项为空矩形；不改动任何子控件，供落点判定使用
    QVector<QRect> plannedRects() const;

private:
    //一个子项的弹簧状态：位置与尺寸（x、y、宽、高）各存一份当前值与速度，另存目标矩形与阻尼比
    struct Spring
    {
        qreal value[4] = {};
        qreal speed[4] = {};
        QRect target;
        qreal damping = 1.0;
    };

    //按当前参数算出各子项的目标矩形并返回内容总高度
    int computeRects(const QRect& rect, QVector<QRect>& rects) const;
    void setItemTarget(QLayoutItem* item, const QRect& target);
    bool springAtRest(const Spring& spring) const;
    void advanceSprings();

    QList<QLayoutItem*> m_items;
    QHash<QLayoutItem*, Spring> m_springs; //有条目的子项已经落过位，此后的目标变化都走弹簧；新子项直接到位
    QTimer* m_timer = nullptr;
    QElapsedTimer m_clock;
    QRect m_layoutRect; //最后一次有效的布局矩形：布局被 invalidate 后 geometry() 会被清零，落点判定不能用它
    QSize m_lastGeometrySize;
    int m_lastItemCount = 0;
    qreal m_dampingRatio = 1.0;
    int m_hSpacing;
    int m_vSpacing;
    bool m_singleColumn = false;
};
