#include "FlowLayout.h"

#include <QTimer>
#include <QWidget>
#include <QtMath>

namespace
{
constexpr qreal kOmega = 18.0;            //弹簧角频率：首次到位约 180ms，再回弹收尾
constexpr qreal kDampingReorder = 0.70;   //拖动重排：约 5% 过冲，收尾回弹一下，就是手机那种弹
constexpr qreal kDampingStable = 0.98;    //增删卡片与窗口缩放：不过冲
constexpr int kTickIntervalMs = 16;       //约 60Hz
constexpr qreal kMaxFrameSeconds = 0.032; //掉帧时一帧最多推进 32ms
constexpr qreal kFixedStepSeconds = 1.0 / 120.0;
constexpr int kMaxSteps = 4;
constexpr qreal kRestDistance = 0.5; //距目标不足半个像素且速度低于 5px/s 就吸附收尾
constexpr qreal kRestSpeed = 5.0;
}

FlowLayout::FlowLayout(QWidget* parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpacing(hSpacing)
    , m_vSpacing(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);

    m_timer = new QTimer(this);
    m_timer->setInterval(kTickIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &FlowLayout::advanceSprings);
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
    m_springs.remove(item);
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
    m_springs.remove(item);
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
    QVector<QRect> rects;
    return computeRects(QRect(0, 0, width, 0), rects);
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
    if(rect.width() > 0)
        m_layoutRect = rect;
    QVector<QRect> rects;
    computeRects(rect, rects);

    //阻尼按这一次重排改变了什么决定：尺寸或子项数变了是增删卡片与窗口缩放，其余是拖动重排
    const bool resized = rect.size() != m_lastGeometrySize;
    const bool countChanged = m_items.size() != m_lastItemCount;
    m_dampingRatio = (resized || countChanged) ? kDampingStable : kDampingReorder;
    m_lastGeometrySize = rect.size();
    m_lastItemCount = m_items.size();

    for(int i = 0; i < m_items.size(); i++)
    {
        //隐藏项不占位置，动画也一并停掉，卡片重新显示时直接落到新位置
        if(rects.at(i).isNull())
            m_springs.remove(m_items.at(i));
        else
            setItemTarget(m_items.at(i), rects.at(i));
    }
}

QVector<QRect> FlowLayout::plannedRects() const
{
    QVector<QRect> rects;
    computeRects(m_layoutRect, rects);
    return rects;
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

int FlowLayout::computeRects(const QRect& rect, QVector<QRect>& rects) const
{
    rects.clear();
    rects.resize(m_items.size());
    const QMargins margins = contentsMargins();
    const QRect effective = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());
    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;
    bool lineIsEmpty = true;

    for(int i = 0; i < m_items.size(); i++)
    {
        QLayoutItem* item = m_items.at(i);
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

        //单列模式下每行只放一个；其余情况放不下就换行（QRect::right() 含端点，+1 才是实际右边界）
        if(!lineIsEmpty && (m_singleColumn || x + hint.width() > effective.right() + 1))
        {
            x = effective.x();
            y += lineHeight + m_vSpacing;
            lineHeight = 0;
            lineIsEmpty = true;
        }

        rects[i] = QRect(x, y, hint.width(), hint.height());
        x += hint.width() + m_hSpacing;
        lineHeight = qMax(lineHeight, hint.height());
        lineIsEmpty = false;
    }
    return y + lineHeight + margins.bottom() - rect.y();
}

void FlowLayout::setItemTarget(QLayoutItem* item, const QRect& target)
{
    if(!m_springs.contains(item))
    {
        //首次落位直接到位，避免启动时所有子控件从原点飞出
        item->setGeometry(target);
        Spring& created = m_springs[item];
        created.target = target;
        created.damping = m_dampingRatio;
        created.value[0] = target.x();
        created.value[1] = target.y();
        created.value[2] = target.width();
        created.value[3] = target.height();
        return;
    }

    //目标变化只改目标，位置与速度都留着，新目标从当前运动接着走；阻尼跟着这一次重排的场景
    Spring& spring = m_springs[item];
    if(spring.target == target)
        return;
    spring.target = target;
    spring.damping = m_dampingRatio;
    //宽度直接到位、不参与弹簧：组变宽变窄时它里面的卡片会重新换行，动画期间会反复换行
    spring.value[2] = target.width();
    spring.speed[2] = 0.0;
    //目标掉头时清掉速度：否则那股还没走完的向外速度会在原位置打颤（占位槽扫过卡片时最明显）
    const qreal goal[4] = {qreal(target.x()), qreal(target.y()), qreal(target.width()), qreal(target.height())};
    for(int i = 0; i < 4; i++)
        if((goal[i] - spring.value[i]) * spring.speed[i] < 0.0)
            spring.speed[i] = 0.0;
    if(springAtRest(spring))
    {
        item->setGeometry(target);
        spring.value[0] = target.x();
        spring.value[1] = target.y();
        spring.value[2] = target.width();
        spring.value[3] = target.height();
        return;
    }
    if(!m_timer->isActive())
    {
        m_clock.start();
        m_timer->start();
    }
}

bool FlowLayout::springAtRest(const Spring& spring) const
{
    const qreal goal[4] = {qreal(spring.target.x()), qreal(spring.target.y()), qreal(spring.target.width()), qreal(spring.target.height())};
    for(int i = 0; i < 4; i++)
        if(qAbs(spring.value[i] - goal[i]) >= kRestDistance || qAbs(spring.speed[i]) >= kRestSpeed)
            return false;
    return true;
}

void FlowLayout::advanceSprings()
{
    //固定步长积分：一帧最多按 32ms 推进、单步不超过 1/120s，掉帧时也不会把弹簧积分炸开
    const qreal frame = qMin(m_clock.restart() / 1000.0, kMaxFrameSeconds);
    const int steps = qBound(1, qCeil(frame / kFixedStepSeconds), kMaxSteps);
    const qreal h = frame / steps;
    const qreal stiffness = kOmega * kOmega;

    bool moving = false;
    for(auto it = m_springs.begin(); it != m_springs.end(); ++it)
    {
        Spring& spring = it.value();
        const qreal friction = 2.0 * spring.damping * kOmega;
        const qreal goal[4] = {qreal(spring.target.x()), qreal(spring.target.y()), qreal(spring.target.width()),
                               qreal(spring.target.height())};
        for(int step = 0; step < steps; step++)
            for(int i = 0; i < 4; i++)
            {
                //半隐式欧拉：先用当前加速度更新速度，再用新速度更新位置
                spring.speed[i] += (stiffness * (goal[i] - spring.value[i]) - friction * spring.speed[i]) * h;
                spring.value[i] += spring.speed[i] * h;
            }

        if(springAtRest(spring))
        {
            //吸附收尾不留看不见的微动；条目留着，它同时标记"这个子项已经落过位"，
            //下次目标变化才继续走弹簧，不会被当成首次落位直接跳过去
            it.key()->setGeometry(spring.target);
            continue;
        }
        it.key()->setGeometry(QRect(qRound(spring.value[0]), qRound(spring.value[1]), qRound(spring.value[2]), qRound(spring.value[3])));
        moving = true;
    }

    //没有子项还在动就停掉定时器，等下一次目标变化再启动
    if(!moving)
        m_timer->stop();
}
