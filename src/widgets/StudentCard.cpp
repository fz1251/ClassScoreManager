#include "StudentCard.h"

#include "CardHost.h"
#include "CardStyle.h"

#include <QApplication>
#include <QDrag>
#include <QFontMetrics>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>

namespace
{
constexpr int kHoverDurationMs = 150;
constexpr int kEnterDurationMs = 180;
constexpr qreal kHoverScale = 1.06;
constexpr qreal kPressedScale = 0.97;
constexpr qreal kEnterScale = 0.86;
constexpr int kDragOpacity = 190;
constexpr qreal kDragHoverProgress = 0.6;
}

StudentCard::StudentCard(int studentIndex, const QString& name, QWidget* parent)
    : QWidget(parent)
    , m_studentIndex(studentIndex)
    , m_name(name)
{
    setCursor(Qt::OpenHandCursor);
    setToolTip(m_name);

    m_hoverAnimation = new QVariantAnimation(this);
    m_hoverAnimation->setDuration(kHoverDurationMs);
    m_hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_hoverAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value)
            {
                m_hoverProgress = value.toReal();
                update();
            });

    m_enterAnimation = new QVariantAnimation(this);
    m_enterAnimation->setDuration(kEnterDurationMs);
    m_enterAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_enterAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value)
            {
                m_enterProgress = value.toReal();
                update();
            });
}

int StudentCard::studentIndex() const
{
    return m_studentIndex;
}

CardHost* StudentCard::host() const
{
    return m_host;
}

void StudentCard::setHost(CardHost* host)
{
    m_host = host;
}

void StudentCard::playEnterAnimation()
{
    m_enterAnimation->stop();
    m_enterAnimation->setStartValue(0.0);
    m_enterAnimation->setEndValue(1.0);
    m_enterAnimation->start();
}

bool StudentCard::decodeDragPayload(const QMimeData* mimeData, int& studentIndex, QSize& cardSize)
{
    if(!mimeData || !mimeData->hasFormat(MimeType))
        return false;
    QByteArray encoded = mimeData->data(MimeType);
    QDataStream stream(&encoded, QIODevice::ReadOnly);
    stream >> studentIndex >> cardSize;
    return studentIndex >= 0;
}

QSize StudentCard::sizeHint() const
{
    const QSize capsule = capsuleSize();
    return QSize(capsule.width() + 2 * CardStyle::hoverMargin, capsule.height() + 2 * CardStyle::hoverMargin);
}

QSize StudentCard::capsuleSize() const
{
    const QFontMetrics metrics(font());
    const int width = qBound(CardStyle::studentCardMinWidth, metrics.horizontalAdvance(m_name) + 2 * CardStyle::studentCardTextPadding,
                             CardStyle::studentCardMaxWidth);
    return QSize(width, CardStyle::studentCardHeight);
}

QRect StudentCard::capsuleRect() const
{
    const QSize capsule = capsuleSize();
    return QRect((width() - capsule.width()) / 2, (height() - capsule.height()) / 2, capsule.width(), capsule.height());
}

bool StudentCard::event(QEvent* event)
{
    if(event->type() == QEvent::Enter)
        startHoverAnimation(1.0);
    else if(event->type() == QEvent::Leave)
        startHoverAnimation(0.0);
    return QWidget::event(event);
}

void StudentCard::paintEvent(QPaintEvent*)
{
    qreal scale = 1.0 + (kHoverScale - 1.0) * m_hoverProgress;
    if(m_pressed)
        scale = kPressedScale;
    scale *= kEnterScale + (1.0 - kEnterScale) * m_enterProgress;

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    drawCapsule(painter, scale, m_enterProgress, m_hoverProgress);
}

void StudentCard::drawCapsule(QPainter& painter, qreal scale, qreal opacity, qreal hoverProgress) const
{
    const QRectF capsule = capsuleRect();
    painter.setOpacity(opacity);
    painter.save();
    painter.translate(capsule.center());
    painter.scale(scale, scale);
    painter.translate(-capsule.center());

    const QColor base = palette().button().color();
    painter.setBrush(CardStyle::mix(base, base.lighter(112), hoverProgress));
    painter.setPen(QPen(CardStyle::mix(QColor(0, 0, 0, 40), QColor(0, 0, 0, 90), hoverProgress), 1.0));
    painter.drawRoundedRect(capsule.adjusted(0.5, 0.5, -0.5, -0.5), CardStyle::studentCardRadius, CardStyle::studentCardRadius);

    const QFontMetrics metrics(font());
    const int textWidth = capsule.width() - 2 * CardStyle::studentCardTextPadding;
    painter.setPen(palette().buttonText().color());
    painter.drawText(capsule, Qt::AlignCenter, metrics.elidedText(m_name, Qt::ElideMiddle, textWidth));
    painter.restore();
    painter.setOpacity(1.0);
}

QPixmap StudentCard::renderCard() const
{
    QPixmap pixmap(size());
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    drawCapsule(painter, 1.0, 1.0, kDragHoverProgress);
    painter.end();

    //整体压成半透明，作为跟随光标的拖动缩略图
    QPainter fade(&pixmap);
    fade.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    fade.fillRect(pixmap.rect(), QColor(0, 0, 0, kDragOpacity));
    return pixmap;
}

void StudentCard::startHoverAnimation(qreal endValue)
{
    m_hoverAnimation->stop();
    m_hoverAnimation->setStartValue(m_hoverProgress);
    m_hoverAnimation->setEndValue(endValue);
    m_hoverAnimation->start();
}

void StudentCard::mousePressEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    m_pressPos = event->pos();
    setCursor(Qt::ClosedHandCursor);
    update();
}

void StudentCard::mouseMoveEvent(QMouseEvent* event)
{
    if(!m_pressed || m_dragging || !(event->buttons() & Qt::LeftButton))
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    if((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
        return;
    m_dragging = true;
    startDrag();
}

void StudentCard::mouseReleaseEvent(QMouseEvent* event)
{
    if(event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const bool clicked = m_pressed && !m_dragging && rect().contains(event->pos());
    m_pressed = false;
    setCursor(Qt::OpenHandCursor);
    update();
    if(clicked)
        emit groupNumberEditRequested(m_studentIndex);
}

void StudentCard::startDrag()
{
    QByteArray encoded;
    QDataStream stream(&encoded, QIODevice::WriteOnly);
    stream << m_studentIndex << sizeHint();

    auto* mimeData = new QMimeData;
    mimeData->setData(MimeType, encoded);

    const QPixmap snapshot = renderCard();
    auto* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(snapshot);
    drag->setHotSpot(QPoint(snapshot.width() / 2, snapshot.height() / 2));

    //拿起卡片：同容器内其余卡片自动补位
    hide();
    emit dragStarted();
    drag->exec(Qt::MoveAction);
    emit dragFinished();
    m_dragging = false;
    //落点没有接收时卡片仍属于原容器，恢复显示
    if(isHidden())
        show();
}
