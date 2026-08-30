#include "HomeActionCard.h"

#include <QEasingCurve>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>

namespace
{
constexpr int kWidgetWidth{250};
constexpr int kWidgetHeight{170};
constexpr int kCardLeft{10};
constexpr int kCardTop{16};
constexpr int kCardWidth{230};
constexpr int kCardHeight{138};
constexpr int kCardRadius{14};
constexpr int kHoverLift{6};
constexpr int kHoverDurationMs{150};
constexpr int kShadowMargin{10};
constexpr int kShadowRingCount{10};
constexpr qreal kShadowRingPeakAlpha{0.45};
constexpr qreal kShadowRestOpacity{0.35};
constexpr qreal kShadowHoverOpacity{0.9};
constexpr int kIconSize{44};
constexpr int kIconTextSpacing{8};
constexpr int kTextHeight{24};

qreal mixChannel(qreal from, qreal to, qreal progress)
{
    return from + (to - from) * progress;
}

QColor mixColor(const QColor &from, const QColor &to, qreal progress)
{
    return QColor::fromRgbF(mixChannel(from.redF(), to.redF(), progress),
                            mixChannel(from.greenF(), to.greenF(), progress),
                            mixChannel(from.blueF(), to.blueF(), progress),
                            mixChannel(from.alphaF(), to.alphaF(), progress));
}
}

HomeActionCard::HomeActionCard(QWidget *parent)
    : QAbstractButton(parent)
{
    setFixedSize(kWidgetWidth, kWidgetHeight);
    setCursor(Qt::PointingHandCursor);
    setFocusPolicy(Qt::StrongFocus);

    QFont cardFont = font();
    cardFont.setPointSize(14);
    cardFont.setBold(true);
    setFont(cardFont);

    m_hoverAnimation = new QVariantAnimation(this);
    m_hoverAnimation->setDuration(kHoverDurationMs);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverProgress = value.toReal();
        update();
    });
}

HomeActionCard::HomeActionCard(const QString &text, QWidget *parent)
    : HomeActionCard(parent)
{
    setText(text);
}

QSize HomeActionCard::sizeHint() const
{
    return QSize(kWidgetWidth, kWidgetHeight);
}

void HomeActionCard::paintEvent(QPaintEvent*)
{
    ensureShadowCache();

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    const QRectF card = cardRect();

    //阶段1：阴影——直接贴自绘好的环形阴影缓存，按进度调整透明度与位置
    const qreal shadowOpacity = isDown() ? kShadowRestOpacity : kShadowRestOpacity + (kShadowHoverOpacity - kShadowRestOpacity) * m_hoverProgress;
    painter.setOpacity(shadowOpacity);
    painter.drawPixmap(QPointF(card.x() - kShadowMargin, card.y() - kShadowMargin), m_shadowCache);
    painter.setOpacity(1.0);

    //阶段2：卡片本体与边框——获得焦点时边框固定深灰，否则随悬停加深
    painter.setBrush(fillColor());
    QColor borderColor(150, 150, 150);
    if (!hasFocus())
        borderColor = mixColor(QColor(0, 0, 0, 35), QColor(0, 0, 0, 72), m_hoverProgress);
    painter.setPen(QPen(borderColor, 1.0));
    painter.drawRoundedRect(card, kCardRadius, kCardRadius);

    //阶段3：图标与标题——共用 contentTop 定位，整体随卡片一起上浮
    const qreal contentTop = card.top() + (card.height() - kIconSize - kIconTextSpacing - kTextHeight) / 2.0;
    const QRectF iconRect(card.center().x() - kIconSize / 2.0, contentTop, kIconSize, kIconSize);
    if (!icon().isNull())
        icon().paint(&painter, iconRect.toRect(), Qt::AlignCenter, isEnabled() ? QIcon::Normal : QIcon::Disabled, QIcon::Off);

    painter.setPen(palette().buttonText().color());
    const QRectF textRect(card.left(), contentTop + kIconSize + kIconTextSpacing, card.width(), kTextHeight);
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, text());

}

void HomeActionCard::enterEvent(QEvent *event)
{
    startHoverAnimation(1.0);
    QWidget::enterEvent(event);
}

void HomeActionCard::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    QWidget::leaveEvent(event);
}

void HomeActionCard::mousePressEvent(QMouseEvent *event)
{
    QAbstractButton::mousePressEvent(event);
    update();
}

void HomeActionCard::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    update();
}

void HomeActionCard::focusInEvent(QFocusEvent *event)
{
    QAbstractButton::focusInEvent(event);
    update();
}

void HomeActionCard::focusOutEvent(QFocusEvent *event)
{
    QAbstractButton::focusOutEvent(event);
    update();
}

void HomeActionCard::startHoverAnimation(qreal endProgress)
{
    //悬停进度统一驱动上浮、阴影与填充色过渡
    m_hoverAnimation->stop();
    m_hoverAnimation->setStartValue(m_hoverProgress);
    m_hoverAnimation->setEndValue(endProgress);
    m_hoverAnimation->start();
}

void HomeActionCard::ensureShadowCache()
{
    const QSize cacheSize(kCardWidth + 2 * kShadowMargin, kCardHeight + 2 * kShadowMargin);
    if (m_shadowCache.size() == cacheSize)
        return;

    m_shadowCache = QPixmap(cacheSize);
    m_shadowCache.fill(Qt::transparent);

    QPainter painter(&m_shadowCache);
    painter.setRenderHint(QPainter::Antialiasing);
    //环形阴影只生成一次：从卡片边缘向外叠 10 圈描边，外圈更淡
    const QRectF baseRect(kShadowMargin, kShadowMargin, kCardWidth, kCardHeight);
    QColor ringColor(0, 0, 0);
    for (int i = 0; i < kShadowRingCount; ++i)
    {
        QPainterPath ringPath;
        ringPath.addRoundedRect(baseRect.adjusted(-i, -i, i, i), kCardRadius + i, kCardRadius + i);
        const qreal fade = qreal(i) / kShadowRingCount;
        ringColor.setAlphaF(kShadowRingPeakAlpha * (1.0 - fade) * (1.0 - fade));
        painter.setPen(ringColor);
        painter.drawPath(ringPath);
    }
}

QRectF HomeActionCard::cardRect() const
{
    //悬停时卡片在固定控件范围内上浮，按下回落到原位
    const qreal lift = isDown() ? 0.0 : kHoverLift * m_hoverProgress;
    return QRectF(kCardLeft, kCardTop - lift, kCardWidth, kCardHeight);
}

QColor HomeActionCard::fillColor() const
{
    //悬停时普通色向浅色插值，按下直接用深色
    const QColor base = palette().button().color();
    if (isDown())
        return base.darker(108);
    return mixColor(base, base.lighter(103), m_hoverProgress);
}
