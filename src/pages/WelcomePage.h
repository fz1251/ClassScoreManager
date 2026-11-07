#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QPixmap>
#include <QPointF>
#include <QWidget>
#include <array>

class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget* parent = nullptr);

signals:
    void openFileRequested();
    void newFileRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    // 一团预渲染的柔和光斑：动画阶段只按目标矩形缩放平移，不重新计算渐变
    struct GlowLayer
    {
        QPixmap pixmap;
        QColor color;
        QPointF baseCenter;
        qreal diameterFactor = 1.0; // 直径相对窗口短边的倍数
        qreal scaleX = 1.0;         // 椭圆长宽比，面积保持约 1.0
        qreal scaleY = 1.0;
        qreal phaseX = 0.0;
        qreal phaseY = 0.0;
        qreal speedX = 0.04;
        qreal speedY = 0.03;
        qreal driftX = 0.10;
        qreal driftY = 0.08;
    };

    static constexpr int kGlowCount = 4;

    QPointF glowCenter(const GlowLayer& layer) const;

    std::array<GlowLayer, kGlowCount> m_glows;
    QTimer* m_frameTimer = nullptr;
    QElapsedTimer m_clock;
    qreal m_phase = 0.0;
};
