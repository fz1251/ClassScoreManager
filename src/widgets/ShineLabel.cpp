#include "ShineLabel.h"

#include <QFontMetrics>
#include <QHideEvent>
#include <QLinearGradient>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>
#include <QtMath>

ShineLabel::ShineLabel(QWidget* parent) : QLabel(parent)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::WindowText, QColor(kDefaultTextColor));
    setPalette(palette);
    setTextFormat(Qt::PlainText);

    m_frameTimer = new QTimer(this);
    m_frameTimer->setInterval(kShineFrameIntervalMs);
    m_frameTimer->setTimerType(Qt::PreciseTimer);
    connect(m_frameTimer, &QTimer::timeout, this, [this]
    {
        const int cycleMs = kShineSweepMs + kShinePauseMs;
        const qint64 elapsedInCycle = m_elapsed.elapsed() % cycleMs;
        const bool sweeping = elapsedInCycle < kShineSweepMs;
        m_sweepProgress = sweeping ? qreal(elapsedInCycle) / kShineSweepMs : 1.0;
        if (sweeping || m_bandVisible)
            update();
        m_bandVisible = sweeping;
    });
}

void ShineLabel::paintEvent(QPaintEvent* event)
{
    const QString currentText = text();
    const bool supported = textFormat() == Qt::PlainText && !wordWrap()
        && !currentText.contains(QLatin1Char('\n')) && !currentText.contains(QLatin1Char('\r'));
    Q_ASSERT(supported);
    if (!supported)
    {
        QLabel::paintEvent(event);
        return;
    }

    updateTextPathIfNeeded();
    if (m_textPath.isEmpty())
    {
        QLabel::paintEvent(event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    painter.setBrush(palette().color(foregroundRole()));
    painter.drawPath(m_textPath);

    if (m_sweepProgress <= 0.0 || m_sweepProgress >= 1.0)
        return;

    // InOutQuad 缓动：头尾放慢、中段加速，让光带的起收更自然
    const qreal easedProgress = m_sweepProgress < 0.5
        ? 2.0 * m_sweepProgress * m_sweepProgress
        : 1.0 - 2.0 * (1.0 - m_sweepProgress) * (1.0 - m_sweepProgress);
    const qreal sweepCenterX = m_sweepStartX + easedProgress * (m_sweepEndX - m_sweepStartX);
    const QPointF bandCenter(sweepCenterX, m_textRect.center().y());
    const qreal gradientAngle = qDegreesToRadians(kShineGradientAngle);
    const QPointF gradientAxis(qCos(gradientAngle), qSin(gradientAngle));
    const qreal halfBandWidth = m_bandWidth / 2.0;

    const QPointF gradientStart = bandCenter - gradientAxis * halfBandWidth;
    const QPointF gradientEnd = bandCenter + gradientAxis * halfBandWidth;
    QLinearGradient shineGradient(gradientStart, gradientEnd);
    shineGradient.setColorAt(0.00, QColor(255, 255, 255, 0));
    shineGradient.setColorAt(0.25, QColor(255, 255, 255, 20));
    shineGradient.setColorAt(0.40, QColor(255, 255, 255, 90));
    shineGradient.setColorAt(0.50, QColor(255, 255, 255, kShinePeakAlpha));
    shineGradient.setColorAt(0.60, QColor(255, 255, 255, 90));
    shineGradient.setColorAt(0.75, QColor(255, 255, 255, 20));
    shineGradient.setColorAt(1.00, QColor(255, 255, 255, 0));
    painter.setBrush(shineGradient);
    painter.drawPath(m_textPath);
}

void ShineLabel::showEvent(QShowEvent* event)
{
    QLabel::showEvent(event);
    m_elapsed.start();
    m_sweepProgress = 0.0;
    m_bandVisible = false;
    m_frameTimer->start();
}

void ShineLabel::hideEvent(QHideEvent* event)
{
    QLabel::hideEvent(event);
    m_frameTimer->stop();
}

void ShineLabel::updateTextPathIfNeeded()
{
    const QString currentText = QLabel::text();
    const QFont currentFont = font();
    const QRect currentContents = contentsRect();
    const Qt::Alignment currentAlignment = alignment();

    if (currentText == m_cachedText && currentFont == m_cachedFont && currentContents == m_cachedContentsRect
        && currentAlignment == m_cachedAlignment)
    {
        return;
    }

    m_cachedText = currentText;
    m_cachedFont = currentFont;
    m_cachedContentsRect = currentContents;
    m_cachedAlignment = currentAlignment;
    updateTextPath();
}

void ShineLabel::updateTextPath()
{
    m_textPath = QPainterPath();
    m_textPath.setFillRule(Qt::WindingFill);
    m_textRect = QRectF();
    m_bandWidth = 0.0;
    m_sweepStartX = 0.0;
    m_sweepEndX = 0.0;

    if (m_cachedText.isEmpty())
        return;

    const QRectF available = m_cachedContentsRect;
    if (available.width() <= 0.0)
        return;

    const QFontMetricsF metrics(m_cachedFont);
    const qreal textWidth = metrics.horizontalAdvance(m_cachedText);

    qreal x = available.left();
    if (m_cachedAlignment.testFlag(Qt::AlignHCenter))
        x += (available.width() - textWidth) / 2.0;
    else if (m_cachedAlignment.testFlag(Qt::AlignRight))
        x += available.width() - textWidth;

    qreal y = available.top();
    if (m_cachedAlignment.testFlag(Qt::AlignVCenter))
        y += (available.height() - metrics.height()) / 2.0;
    else if (m_cachedAlignment.testFlag(Qt::AlignBottom))
        y += available.height() - metrics.height();

    QPainterPath linePath;
    linePath.addText(QPointF(x, y + metrics.ascent()), m_cachedFont, m_cachedText);
    m_textPath.addPath(linePath);
    m_textRect = linePath.boundingRect();
    if (m_textRect.isEmpty())
        return;

    m_bandWidth = metrics.height() * kShineBandHeightRatio;

    // 按文字四角在渐变轴上的投影计算光带起止位置，保证开始和结束时整条光带都在文字之外
    const qreal gradientAngle = qDegreesToRadians(kShineGradientAngle);
    const QPointF gradientAxis(qCos(gradientAngle), qSin(gradientAngle));
    const QPointF centerOffset(0.0, m_textRect.center().y());
    const QPointF textCorners[] = { m_textRect.topLeft(), m_textRect.topRight(), m_textRect.bottomRight(), m_textRect.bottomLeft() };
    qreal minProjection = QPointF::dotProduct(textCorners[0] - centerOffset, gradientAxis);
    qreal maxProjection = minProjection;
    for (int i = 1; i < 4; ++i)
    {
        const qreal projection = QPointF::dotProduct(textCorners[i] - centerOffset, gradientAxis);
        minProjection = qMin(minProjection, projection);
        maxProjection = qMax(maxProjection, projection);
    }

    const qreal halfBandWidth = m_bandWidth / 2.0;
    m_sweepStartX = (minProjection - halfBandWidth) / gradientAxis.x();
    m_sweepEndX = (maxProjection + halfBandWidth) / gradientAxis.x();
}
