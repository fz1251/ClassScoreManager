#include "ClickableLabel.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>

namespace
{
constexpr int kHorizontalPadding = 10;
constexpr int kCornerRadius = 4;
}

ClickableLabel::ClickableLabel(QWidget* parent)
    : QLabel(parent)
{
    setMinimumWidth(0);
    setFixedHeight(36);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
}

void ClickableLabel::setClickable(bool clickable)
{
    if(this->clickable == clickable)
        return;
    this->clickable = clickable;
    hovered = false;
    setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
    update();
}

void ClickableLabel::enterEvent(QEvent* event)
{
    if(clickable)
    {
        hovered = true;
        update();
    }
    QLabel::enterEvent(event);
}

void ClickableLabel::leaveEvent(QEvent* event)
{
    if(hovered)
    {
        hovered = false;
        update();
    }
    QLabel::leaveEvent(event);
}

void ClickableLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if(clickable && event->button() == Qt::LeftButton && rect().contains(event->pos()))
    {
        emit clicked();
        event->accept();
        return;
    }
    QLabel::mouseReleaseEvent(event);
}

void ClickableLabel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if(clickable)
    {
        const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        painter.setPen(QPen(QColor(128, 128, 128, hovered ? 100 : 60), 1.0));
        painter.setBrush(QColor(128, 128, 128, hovered ? 46 : 22));
        painter.drawRoundedRect(box, kCornerRadius, kCornerRadius);
        painter.setPen(palette().color(QPalette::WindowText));
    }
    else
    {
        painter.setPen(palette().color(QPalette::PlaceholderText));
    }

    const int textWidth = qMax(0, width() - 2 * kHorizontalPadding);
    const QString elidedText = fontMetrics().elidedText(text(), Qt::ElideRight, textWidth);
    const QRect textRect = rect().adjusted(kHorizontalPadding, 0, -kHorizontalPadding, 0);
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, elidedText);
}
