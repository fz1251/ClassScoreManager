#include "CloseButton.h"

#include <QEvent>
#include <QSize>

CloseButton::CloseButton(QWidget* parent)
    : QPushButton(parent)
    , normalIcon(QStringLiteral(":/icons/resources/about-dialog-close.svg"))
    , hoverIcon(QStringLiteral(":/icons/resources/about-dialog-close-hovered.svg"))
{
    constexpr int kSide = 30;
    setFocusPolicy(Qt::ClickFocus);
    setFixedSize(kSide, kSide);
    setIconSize(QSize(kSide, kSide));
    setIcon(normalIcon);
    setStyleSheet(QStringLiteral("QPushButton { border: none; background: transparent; }"));
}

void CloseButton::enterEvent(QEvent* event)
{
    setIcon(hoverIcon);
    QPushButton::enterEvent(event);
}

void CloseButton::leaveEvent(QEvent* event)
{
    setIcon(normalIcon);
    QPushButton::leaveEvent(event);
}
