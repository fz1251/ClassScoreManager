#ifndef SHINELABEL_H
#define SHINELABEL_H

#include <QElapsedTimer>
#include <QFont>
#include <QLabel>
#include <QPainterPath>
#include <QRect>

class QHideEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

// 仅用于单行纯文本大标题的扫光标签；不支持富文本、换行与自动换行，不满足条件时回退为普通 QLabel 绘制
class ShineLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ShineLabel(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void updateTextPathIfNeeded();
    void updateTextPath();

    // 光带从文字左侧完全扫到右侧所用的时间（毫秒），数值越大扫得越慢
    static constexpr int kShineSweepMs = 1500;
    // 两次扫光之间的静止间隔（毫秒），数值越大停顿越久
    static constexpr int kShinePauseMs = 5000;
    // 动画重绘的帧间隔（毫秒），33ms 约等于 30fps；停顿期间不重绘，控制 CPU 开销
    static constexpr int kShineFrameIntervalMs = 33;
    // 光带中心白色的峰值透明度（0~255），数值越大高光越亮
    static constexpr int kShinePeakAlpha = 200;
    // 光带完整渐变宽度与字高的比值，数值越大光带越宽；字高取 QFontMetricsF::height()
    static constexpr qreal kShineBandHeightRatio = 2.0;
    // 光带相对竖直方向的倾斜角（度）：0 为竖直光带，负值由左上向右下倾斜，正值由右上向左下倾斜
    static constexpr qreal kShineGradientAngle = -17.0;
    // 文字默认颜色（0xAARRGGBB），当前为不透明黑色
    static constexpr QRgb kDefaultTextColor = 0xFF000000;

    QTimer* m_frameTimer = nullptr;
    QElapsedTimer m_elapsed;
    QPainterPath m_textPath;
    QRectF m_textRect;
    qreal m_bandWidth = 0.0;
    qreal m_sweepStartX = 0.0;
    qreal m_sweepEndX = 0.0;
    qreal m_sweepProgress = 0.0;
    bool m_bandVisible = false;

    QString m_cachedText;
    QFont m_cachedFont;
    QRect m_cachedContentsRect;
    Qt::Alignment m_cachedAlignment = Qt::AlignLeft | Qt::AlignTop;
};

#endif // SHINELABEL_H
