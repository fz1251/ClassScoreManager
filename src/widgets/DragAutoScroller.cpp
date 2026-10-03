#include "DragAutoScroller.h"

#include <QAbstractScrollArea>
#include <QCursor>
#include <QScrollBar>
#include <QTimer>

DragAutoScroller::DragAutoScroller(QAbstractScrollArea* scrollArea, const Config& config, QObject* parent)
    : QObject(parent)
    , m_scrollArea(scrollArea)
    , m_timer(new QTimer(this))
    , m_config(config)
{
    m_timer->setInterval(m_config.intervalMs);
    connect(m_timer, &QTimer::timeout, this, &DragAutoScroller::step);
}

void DragAutoScroller::start()
{
    if(!m_timer->isActive())
        m_timer->start();
}

void DragAutoScroller::stop()
{
    m_timer->stop();
    m_remainder = 0.0;
}

void DragAutoScroller::step()
{
    QScrollBar* bar = m_scrollArea->verticalScrollBar();
    if(bar->maximum() <= bar->minimum())
        return;
    const QRect viewport = m_scrollArea->viewport()->rect();
    const QPoint pos = m_scrollArea->viewport()->mapFromGlobal(QCursor::pos());
    if(!viewport.contains(pos))
        return;

    //光标深入边缘区的比例：刚进边缘区是 0，贴到边框是 1
    const bool atTop = pos.y() < viewport.top() + m_config.edge;
    qreal intensity = 0.0;
    if(atTop)
        intensity = qreal(viewport.top() + m_config.edge - pos.y()) / m_config.edge;
    else if(pos.y() > viewport.bottom() - m_config.edge)
        intensity = qreal(pos.y() - viewport.bottom() + m_config.edge) / m_config.edge;
    if(intensity <= 0.0)
    {
        m_remainder = 0.0;
        return;
    }

    //速度按深入程度二次增长：边缘区里慢、贴边快，进出都连续，不需要额外的缓动
    //不足 1px 的零头累加到下一拍，否则低速段永远滚不动
    const qreal delta = m_config.maxSpeed * intensity * intensity * m_config.intervalMs / 1000.0 + m_remainder;
    const int whole = int(delta);
    m_remainder = delta - whole;
    if(whole > 0)
        bar->setValue(bar->value() + (atTop ? -whole : whole));
}
