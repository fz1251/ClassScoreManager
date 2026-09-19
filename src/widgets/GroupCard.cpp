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

//插入位置指示：与拖动卡片同尺寸的虚线空位
class DropPlaceholder : public QWidget
{
public:
    explicit DropPlaceholder(QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing);
        const QColor accent = palette().highlight().color();
        QColor fill = accent;
        fill.setAlpha(46);
        painter.setPen(QPen(accent, 1.5, Qt::DashLine));
        painter.setBrush(fill);
        const QRectF box =
            QRectF(rect()).adjusted(CardStyle::hoverMargin, CardStyle::hoverMargin, -CardStyle::hoverMargin, -CardStyle::hoverMargin);
        painter.drawRoundedRect(box, CardStyle::studentCardRadius, CardStyle::studentCardRadius);
    }
};
}

GroupCard::GroupCard(qint32 groupNumber, QWidget* parent)
    : QWidget(parent)
    , m_groupNumber(groupNumber)
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

void GroupCard::insertCard(StudentCard* card, int index)
{
    //占位槽让出的位置正是卡片要插入的位置
    clearPlaceholder();
    m_cardLayout->insertWidget(index, card);
    card->setHost(this);
    //重新认领父控件会让卡片转为隐藏，插入后要显式恢复显示
    card->show();
    refreshHeader();
}

void GroupCard::removeCard(StudentCard* card)
{
    m_cardLayout->removeWidget(card);
    refreshHeader();
}

void GroupCard::clearCards()
{
    m_placeholder = nullptr;
    while(m_cardLayout->count() > 0)
    {
        QLayoutItem* item = m_cardLayout->takeAt(0);
        QWidget* widget = item->widget();
        delete item;
        delete widget;
    }
    refreshHeader();
}

void GroupCard::refreshHeader()
{
    m_titleLabel->setText(tr("第 %1 组").arg(m_groupNumber));
    m_countLabel->setText(tr("%1 人").arg(cardCount()));
}

void GroupCard::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    const QColor accent = palette().highlight().color();
    const QColor fill = CardStyle::mix(palette().base().color(), accent, 0.12 * m_highlightProgress);
    const QColor border = CardStyle::mix(QColor(0, 0, 0, 30), accent, m_highlightProgress);
    painter.setBrush(fill);
    painter.setPen(QPen(border, 1.0));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kCornerRadius, kCornerRadius);
}

int GroupCard::dropIndexAt(const QPoint& pos) const
{
    int index = 0;
    for(int i = 0; i < m_cardLayout->count(); i++)
    {
        QWidget* widget = m_cardLayout->itemAt(i)->widget();
        if(!widget || widget == m_placeholder)
            continue;
        const QRect area = widget->geometry();
        if(pos.y() < area.center().y())
            break;
        if(pos.y() <= area.bottom() && pos.x() < area.center().x())
            break;
        index++;
    }
    return index;
}

void GroupCard::setPlaceholder(const QSize& cardSize, int index)
{
    if(!m_placeholder)
    {
        m_placeholder = new DropPlaceholder(this);
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
    const int insertIndex = m_placeholder ? m_cardLayout->indexOf(m_placeholder) : dropIndexAt(event->pos());
    startHighlightAnimation(false);
    event->acceptProposedAction();
    emit studentDropped(studentIndex, insertIndex);
}
