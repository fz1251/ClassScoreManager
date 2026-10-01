#include "GroupCard.h"

#include "CardStyle.h"
#include "FlowLayout.h"
#include "StudentCard.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

namespace
{
constexpr int kHighlightDurationMs = 160;
constexpr int kCornerRadius = 12;
constexpr int kCardSpacing = 4;
constexpr int kEdgeMargin = 12;
constexpr qreal kGroupWash = 0.24;       //静止时组色在组容器底色中的占比
constexpr qreal kGroupWashHover = 0.38;  //拖拽落入时的占比
constexpr qreal kGroupBorder = 0.45;     //描边向组色靠拢的程度
constexpr qreal kPlaceholderFill = 0.55; //占位槽填充：组色的不透明度
//落点判定线的死区：光标越过中线后多走这么多才换位，避免压在中线上时来回弹
constexpr int kDropSlack = CardStyle::hoverMargin + kCardSpacing / 2;
//行与行之间的可见留白，等于纵向迟滞带的宽度：换行要真正踩进目标行的可见范围
constexpr int kRowWhitespace = 2 * CardStyle::hoverMargin + kCardSpacing;

//卡片在布局矩形里的可见范围：去掉四周供悬停放大用的留白
QRect visibleBand(const QRect& area)
{
    return area.adjusted(0, CardStyle::hoverMargin, 0, -CardStyle::hoverMargin);
}

//插入位置指示：与目标组卡片同色同尺寸的空位，预览卡片落下去之后的样子
class DropPlaceholder : public QWidget
{
public:
    DropPlaceholder(const QColor& color, QWidget* parent)
        : QWidget(parent)
        , m_color(color)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing);
        //实心组色 + 与学生卡片相同的浅灰描边
        QColor fill = m_color;
        fill.setAlphaF(kPlaceholderFill);
        painter.setPen(QPen(QColor(0, 0, 0, 40), 1.0));
        painter.setBrush(fill);
        const QRectF box =
            QRectF(rect()).adjusted(CardStyle::hoverMargin, CardStyle::hoverMargin, -CardStyle::hoverMargin, -CardStyle::hoverMargin);
        painter.drawRoundedRect(box, CardStyle::studentCardRadius, CardStyle::studentCardRadius);
    }

private:
    QColor m_color;
};
}

GroupCard::GroupCard(qint32 groupNumber, QWidget* parent)
    : QWidget(parent)
    , m_groupNumber(groupNumber)
    , m_groupColor(CardStyle::groupColor(groupNumber))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAcceptDrops(true);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(kEdgeMargin, 10, kEdgeMargin, kEdgeMargin);
    rootLayout->setSpacing(8);

    //标题行：左侧组名，右侧成员数
    auto* headerWidget = new QWidget(this);
    auto* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(8);

    m_titleLabel = new QLabel(headerWidget);
    QFont titleFont = m_titleLabel->font();
    titleFont.setBold(true);
    m_titleLabel->setFont(titleFont);
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_countLabel = new QLabel(headerWidget);
    QPalette countPalette = m_countLabel->palette();
    countPalette.setColor(QPalette::WindowText, countPalette.color(QPalette::PlaceholderText));
    m_countLabel->setPalette(countPalette);
    headerLayout->addWidget(m_countLabel);
    rootLayout->addWidget(headerWidget);

    //组内流式布局交给外层布局认领：先挂到widget再addLayout会因布局已有父对象而失败
    m_cardLayout = new FlowLayout(nullptr, 0, kCardSpacing, kCardSpacing);
    rootLayout->addLayout(m_cardLayout);

    m_highlightAnimation = new QVariantAnimation(this);
    m_highlightAnimation->setDuration(kHighlightDurationMs);
    m_highlightAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_highlightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value)
            {
                m_highlightProgress = value.toReal();
                update();
            });

    refreshHeader();
}

qint32 GroupCard::groupNumber() const
{
    return m_groupNumber;
}

int GroupCard::cardCount() const
{
    int count = 0;
    for(int i = 0; i < m_cardLayout->count(); i++)
        if(qobject_cast<StudentCard*>(m_cardLayout->itemAt(i)->widget()))
            count++;
    return count;
}

FlowLayout* GroupCard::cardLayout() const
{
    return m_cardLayout;
}

QColor GroupCard::cardColor() const
{
    return m_groupColor;
}

void GroupCard::insertCard(StudentCard* card, int index)
{
    //占位槽让出的位置正是卡片要插入的位置：先按占位槽的矩形落位再显示，卡片不会从原位置飞过来
    const QRect slot = m_placeholder ? m_placeholder->geometry() : QRect();
    clearPlaceholder();
    m_cardLayout->insertWidget(index, card);
    card->setHost(this);
    if(!slot.isNull())
        card->setGeometry(slot);
    //重新认领父控件会让卡片转为隐藏，插入后要显式恢复显示
    card->show();
    refreshHeader();
}

void GroupCard::removeCard(StudentCard* card)
{
    //卡片搬走时源小组的占位槽一并撤除，不留空位
    clearPlaceholder();
    m_cardLayout->removeWidget(card);
    refreshHeader();
}

void GroupCard::beginDrag(StudentCard* card)
{
    //占位槽先在卡片原地顶住位置，卡片再隐藏，布局在拖动起步时不动
    const int index = m_cardLayout->indexOf(card);
    if(index >= 0)
        setPlaceholder(card->sizeHint(), index);
}

void GroupCard::endDrag()
{
    //落下时占位槽已随插入撤除，取消拖动时才会留到这里
    clearPlaceholder();
}

void GroupCard::refreshHeader()
{
    m_titleLabel->setText(tr("第 %1 组").arg(m_groupNumber));
    m_countLabel->setText(tr("%1 人").arg(cardCount()));
}

void GroupCard::paintEvent(QPaintEvent*)
{
    //静止时是组色的淡雾，拖拽落入时底色加深、描边变实
    const qreal wash = kGroupWash + (kGroupWashHover - kGroupWash) * m_highlightProgress;
    const qreal borderMix = kGroupBorder + (1.0 - kGroupBorder) * m_highlightProgress;
    const QColor fill = CardStyle::mix(palette().base().color(), m_groupColor, wash);
    const QColor border = CardStyle::mix(QColor(0, 0, 0, 30), m_groupColor, borderMix);

    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setBrush(fill);
    painter.setPen(QPen(border, 1.0));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kCornerRadius, kCornerRadius);
}

//插入点由光标所在的行决定，行内只按横坐标定位；光标在卡片区之上取下第一行，之下取下最后一行
int GroupCard::dropIndexAt(const QPoint& pos) const
{
    //判定用布局算出的目标矩形：动画中途的 geometry 会让中线跟着漂移，落点来回跳
    const QVector<QRect> rects = m_cardLayout->plannedRects();
    //已提交的插入点就是占位槽当前的位置，同时充当纵向迟滞的参照
    const int committedIndex = m_placeholder ? m_cardLayout->indexOf(m_placeholder) : -1;

    //按行汇总卡片的可见范围：同一行的子项纵坐标相同
    QVector<QRect> rows;
    for(int i = 0; i < m_cardLayout->count(); i++)
    {
        if(rects.value(i).isNull())
            continue;
        const QRect band = visibleBand(rects.value(i));
        if(!rows.isEmpty() && rows.constLast().top() == band.top())
            rows.last().setBottom(band.bottom());
        else
            rows.append(band);
    }

    //参考行：光标还在已提交行的迟滞带里就不换行，否则取纵向最近的一行——
    //在卡片区之上落在第一行，之下落在最后一行，行间留白落在下一行
    int refRowTop = -1;
    if(committedIndex >= 0 && !rects.value(committedIndex).isNull())
    {
        const QRect band = visibleBand(rects.value(committedIndex));
        if(pos.y() >= band.top() - kRowWhitespace && pos.y() <= band.bottom() + kRowWhitespace)
            refRowTop = band.top();
    }
    for(int i = 0; refRowTop < 0 && i < rows.size(); i++)
        if(rows.at(i).bottom() >= pos.y())
            refRowTop = rows.at(i).top();
    if(refRowTop < 0 && !rows.isEmpty())
        refRowTop = rows.constLast().top();

    int index = 0;
    for(int i = 0; i < m_cardLayout->count(); i++)
    {
        QWidget* widget = m_cardLayout->itemAt(i)->widget();
        if(!widget || widget == m_placeholder)
            continue;
        const QRect rect = rects.value(i);
        if(rect.isNull())
        {
            //隐藏项（被拖动的卡片）没有排布结果，但它仍占一个插入下标
            index++;
            continue;
        }
        const QRect band = visibleBand(rect);
        //参考行之前的卡片都排在插入点之前，参考行之后的整行跳过，行内第一张判定线在光标右侧的卡片决定插入点
        if(band.top() > refRowTop)
            break;
        if(band.top() == refRowTop)
        {
            //横向迟滞：已提交的插入点在这张卡片之后时判定线左移，往回退要多走 kDropSlack 才换位；
            //还没有占位槽（拖动进入组的第一个事件）时按中线判定
            int line = band.center().x();
            if(committedIndex >= 0)
                line += committedIndex > index ? -kDropSlack : kDropSlack;
            if(pos.x() < line)
                break;
        }
        index++;
    }
    return index;
}

void GroupCard::setPlaceholder(const QSize& cardSize, int index)
{
    if(!m_placeholder)
    {
        m_placeholder = new DropPlaceholder(m_groupColor, this);
        m_placeholder->setFixedSize(cardSize);
        m_cardLayout->insertWidget(index, m_placeholder);
        return;
    }
    if(m_placeholder->size() != cardSize)
        m_placeholder->setFixedSize(cardSize);
    if(m_cardLayout->indexOf(m_placeholder) == index)
        return;
    m_cardLayout->removeWidget(m_placeholder);
    m_cardLayout->insertWidget(index, m_placeholder);
}

void GroupCard::clearPlaceholder()
{
    if(!m_placeholder)
        return;
    m_cardLayout->removeWidget(m_placeholder);
    delete m_placeholder;
    m_placeholder = nullptr;
}

void GroupCard::startHighlightAnimation(bool highlighted)
{
    m_highlightAnimation->stop();
    m_highlightAnimation->setStartValue(m_highlightProgress);
    m_highlightAnimation->setEndValue(highlighted ? 1.0 : 0.0);
    m_highlightAnimation->start();
}

void GroupCard::dragEnterEvent(QDragEnterEvent* event)
{
    int studentIndex = -1;
    QSize cardSize;
    if(!StudentCard::decodeDragPayload(event->mimeData(), studentIndex, cardSize))
    {
        event->ignore();
        return;
    }
    setPlaceholder(cardSize, dropIndexAt(event->pos()));
    startHighlightAnimation(true);
    event->acceptProposedAction();
}

void GroupCard::dragMoveEvent(QDragMoveEvent* event)
{
    int studentIndex = -1;
    QSize cardSize;
    if(!StudentCard::decodeDragPayload(event->mimeData(), studentIndex, cardSize))
    {
        event->ignore();
        return;
    }
    setPlaceholder(cardSize, dropIndexAt(event->pos()));
    event->acceptProposedAction();
}

void GroupCard::dragLeaveEvent(QDragLeaveEvent* event)
{
    clearPlaceholder();
    startHighlightAnimation(false);
    event->accept();
}

void GroupCard::dropEvent(QDropEvent* event)
{
    int studentIndex = -1;
    QSize cardSize;
    if(!StudentCard::decodeDragPayload(event->mimeData(), studentIndex, cardSize))
    {
        event->ignore();
        return;
    }
    //占位槽留到卡片真正插入时再撤除，位置索引才对得上
    const int placeholderIndex = m_placeholder ? m_cardLayout->indexOf(m_placeholder) : dropIndexAt(event->pos());
    //插入位置一律按“不含被拖卡片”的顺序算：拖动中的卡片只是隐藏，仍占着一个布局项
    int insertIndex = placeholderIndex;
    for(int i = 0; i < placeholderIndex; i++)
    {
        QWidget* item = m_cardLayout->itemAt(i)->widget();
        if(item && item != m_placeholder && item->isHidden())
            insertIndex--;
    }
    startHighlightAnimation(false);
    event->acceptProposedAction();
    emit studentDropped(studentIndex, insertIndex);
}
