#include "FlowLayout.h"

#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QWidget>

FlowLayout::FlowLayout(QWidget* parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpacing(hSpacing)
    , m_vSpacing(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    while(QLayoutItem* item = takeAt(0))
        delete item;
}

void FlowLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

int FlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if(index < 0 || index >= m_items.size())
        return nullptr;
    QLayoutItem* item = m_items.takeAt(index);
    m_placed.removeAll(item);
    if(QWidget* widget = item->widget())
        stopAnimationFor(widget);
    return item;
}

void FlowLayout::insertWidget(int index, QWidget* widget)
{
    addChildWidget(widget);
    if(index < 0)
        index = m_items.size();
    m_items.insert(qBound(0, index, m_items.size()), new QWidgetItem(widget));
    invalidate();
}

void FlowLayout::removeWidget(QWidget* widget)
{
    const int index = indexOf(widget);
    if(index < 0)
        return;
    QLayoutItem* item = m_items.takeAt(index);
    m_placed.removeAll(item);
    stopAnimationFor(widget);
    delete item;
    invalidate();
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return {};
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for(const QLayoutItem* item : m_items)
        size = size.expandedTo(item->minimumSize());
    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, true);
}

int FlowLayout::horizontalSpacing() const
{
    return m_hSpacing;
}

int FlowLayout::verticalSpacing() const
{
    return m_vSpacing;
}

void FlowLayout::setSingleColumn(bool enabled)
{
    if(m_singleColumn == enabled)
        return;
    m_singleColumn = enabled;
    invalidate();
}

int FlowLayout::doLayout(const QRect& rect, bool moveItems) const
{
    const QMargins margins = contentsMargins();
    const QRect effective = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;
    bool lineIsEmpty = true;

    for(QLayoutItem* item : m_items)
    {
        //隐藏的子控件不占位置，其余子控件自动补位
        if(item->isEmpty())
            continue;

        QSize hint = item->sizeHint();
        QWidget* widget = item->widget();
        if(m_singleColumn && widget && (widget->sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag))
        {
            //单列模式下水平可扩展的子控件撑满整行，并按新宽度重算高度
            hint.setWidth(effective.width());
            const int height = widget->heightForWidth(hint.width());
            if(height > 0)
                hint.setHeight(height);
        }

        if(!lineIsEmpty && (m_singleColumn || x + hint.width() > effective.right() + 1))
        {
            x = effective.x();
            y += lineHeight + m_vSpacing;
            lineHeight = 0;
            lineIsEmpty = true;
        }

        if(moveItems)
            moveItemTo(item, QRect(x, y, hint.width(), hint.height()));
        x += hint.width() + m_hSpacing;
        lineHeight = qMax(lineHeight, hint.height());
        lineIsEmpty = false;
    }
    return y + lineHeight + margins.bottom() - rect.y();
}

void FlowLayout::moveItemTo(QLayoutItem* item, const QRect& target) const
{
    QWidget* widget = item->widget();
    if(!widget)
    {
        item->setGeometry(target);
        return;
    }

    const bool firstPlacement = !m_placed.contains(item);
    if(firstPlacement || widget->geometry() == target)
    {
        //首次落位直接到位，避免启动时所有子控件从原点飞出
        stopAnimationFor(widget);
        item->setGeometry(target);
        if(firstPlacement)
            m_placed.append(item);
        return;
    }

    //已经朝同一目标移动的动画继续跑，重排不必让它从头再来
    const QPointer<QPropertyAnimation> running = m_animations.value(widget);
    if(running && running->state() == QAbstractAnimation::Running && running->endValue().toRect() == target)
        return;

    stopAnimationFor(widget);
    //动画挂在目标控件上，控件销毁时动画随之销毁
    auto* animation = new QPropertyAnimation(widget, "geometry", widget);
    animation->setStartValue(widget->geometry());
    animation->setEndValue(target);
    animation->setDuration(m_duration);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(animation, &QPropertyAnimation::finished, animation, &QObject::deleteLater);
    m_animations.insert(widget, animation);
    animation->start();
}

void FlowLayout::stopAnimationFor(QWidget* widget) const
{
    const QPointer<QPropertyAnimation> animation = m_animations.take(widget);
    if(!animation)
        return;
    animation->stop();
    animation->deleteLater();
}
