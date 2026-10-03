#pragma once

#include <QObject>

class QAbstractScrollArea;
class QTimer;

// 拖动时贴近滚动区上下边缘自动滚动：速度按光标深入边缘区的比例平方增长，参数由每个实例各自配置
// 速度以像素/秒计，要求滚动区按像素滚动：QScrollArea 天然如此，QAbstractItemView 需设为 ScrollPerPixel
class DragAutoScroller : public QObject
{
    Q_OBJECT
public:
    // 信息密度高的列表给更低的最高速度，否则一贴边就冲过头
    struct Config
    {
        int intervalMs; //节拍，16ms 约 60Hz，看不出台阶
        int edge;       //光标进入边缘这么多像素内开始滚动
        qreal maxSpeed; //贴到边框时的滚动速度（px/s）
    };

    explicit DragAutoScroller(QAbstractScrollArea* scrollArea, const Config& config, QObject* parent = nullptr);

    void start();
    void stop();

private:
    void step();

    QAbstractScrollArea* m_scrollArea = nullptr;
    QTimer* m_timer = nullptr;
    Config m_config;
    qreal m_remainder = 0.0; //不足 1px 的滚动零头，累加到下一拍
};
