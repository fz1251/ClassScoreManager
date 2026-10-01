#include "UngroupedPanel.h"

#include "FlowLayout.h"
#include "StudentCard.h"
#include "exfonticon.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
constexpr int kPanelMinWidth = 120; //够放 3 个字的名字，再窄长名字就会被裁切
constexpr int kEdgeMargin = 12;
constexpr int kHeaderSpacing = 8;
constexpr int kMaskDurationMs = 140;
constexpr int kRailRadius = 12;               //与右侧小组卡片、学生胶囊同一套圆角
constexpr QColor kRailSurface(238, 240, 242); //左栏栏面：比页面略深，比右栏明显更深
constexpr QColor kRailBorder(0, 0, 0, 30);
constexpr QColor kMaskColor(211, 47, 47);     //蒙版
constexpr QColor kMaskTextColor(155, 28, 28); //深红图标与提示文字
constexpr char16_t kDeleteGlyph = static_cast<char16_t>(SegoeIcon::Delete);
constexpr int kMaskIconPixelSize = 48;
constexpr int kMaskTextPointSize = 20;
constexpr int kIconTextSpacing = 10;
constexpr qreal kMaskTintAlpha = 0.16;

//覆盖在列表区之上的红色蒙版，提示拖到此处即取消分组
class DropMask : public QWidget
{
public:
    DropMask(const QString& text, QWidget* parent)
        : QWidget(parent)
        , m_text(text)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setVisible(false);
    }

    void setProgress(qreal progress)
    {
        m_progress = progress;
        setVisible(progress > 0.002);
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

        //铺满整块栏面，圆角与栏自身的边框对齐
        const QRectF area = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
        QColor tint = kMaskColor;
        tint.setAlphaF(kMaskTintAlpha * m_progress);
        painter.setPen(Qt::NoPen);
        painter.setBrush(tint);
        painter.drawRoundedRect(area, kRailRadius, kRailRadius);

        //深红的垃圾桶图标与提示文字，在栏面内垂直居中
        QColor contentColor = kMaskTextColor;
        contentColor.setAlphaF(m_progress);

        QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
        iconFont.setPixelSize(kMaskIconPixelSize);
        QFont textFont = font();
        textFont.setPointSize(kMaskTextPointSize);

        const QFontMetricsF iconMetrics(iconFont);
        const QFontMetricsF textMetrics(textFont);
        const qreal iconHeight = iconMetrics.height();
        const qreal textHeight = textMetrics.height();
        const qreal top = area.top() + (area.height() - iconHeight - kIconTextSpacing - textHeight) / 2.0;

        painter.setPen(contentColor);
        painter.setFont(iconFont);
        painter.drawText(QRectF(area.left(), top, area.width(), iconHeight), Qt::AlignCenter, QString(QChar(kDeleteGlyph)));
        painter.setFont(textFont);
        painter.drawText(QRectF(area.left(), top + iconHeight + kIconTextSpacing, area.width(), textHeight), Qt::AlignCenter, m_text);
    }

private:
    QString m_text;
    qreal m_progress = 0.0;
};
}

UngroupedPanel::UngroupedPanel(QWidget* parent)
    : QWidget(parent)
{
    setAcceptDrops(true);
    setMinimumWidth(kPanelMinWidth);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(kEdgeMargin, kEdgeMargin, kEdgeMargin, kEdgeMargin);
    rootLayout->setSpacing(kHeaderSpacing);

    //表头：未分组人数
    m_headerLabel = new QLabel(this);
    QFont headerFont = m_headerLabel->font();
    headerFont.setBold(true);
    m_headerLabel->setFont(headerFont);
    rootLayout->addWidget(m_headerLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_cardContainer = new QWidget;
    m_cardLayout = new FlowLayout(m_cardContainer, 0, 0, 2);
    m_cardLayout->setSingleColumn(true);
    m_scrollArea->setWidget(m_cardContainer);

    //滚动区、视口与容器都不填底色，让栏面整体透上来（setWidget 会把容器设成自填背景）
    m_scrollArea->setAutoFillBackground(false);
    m_scrollArea->viewport()->setAutoFillBackground(false);
    m_cardContainer->setAutoFillBackground(false);
    rootLayout->addWidget(m_scrollArea, 1);

    m_mask = new DropMask(tr("取消分组"), this);

    m_maskAnimation = new QVariantAnimation(this);
    m_maskAnimation->setDuration(kMaskDurationMs);
    m_maskAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_maskAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value)
            {
                updateMaskProgress(value.toReal());
            });

    refreshHeader();
}

FlowLayout* UngroupedPanel::cardLayout() const
{
    return m_cardLayout;
}

QColor UngroupedPanel::cardColor() const
{
    //未分组学生保持中性色，与有色小组形成对照
    return palette().base().color();
}

QScrollArea* UngroupedPanel::scrollArea() const
{
    return m_scrollArea;
}

int UngroupedPanel::cardCount() const
{
    int count = 0;
    for(int i = 0; i < m_cardLayout->count(); i++)
        if(qobject_cast<StudentCard*>(m_cardLayout->itemAt(i)->widget()))
            count++;
    return count;
}

void UngroupedPanel::insertCard(StudentCard* card, int index)
{
    m_cardLayout->insertWidget(index, card);
    card->setHost(this);
    //重新认领父控件会让卡片转为隐藏，插入后要显式恢复显示
    card->show();
    refreshHeader();
}

void UngroupedPanel::removeCard(StudentCard* card)
{
    m_cardLayout->removeWidget(card);
    refreshHeader();
}

void UngroupedPanel::clearCards()
{
    while(m_cardLayout->count() > 0)
    {
        QLayoutItem* item = m_cardLayout->takeAt(0);
        QWidget* widget = item->widget();
        delete item;
        delete widget;
    }
    refreshHeader();
}

void UngroupedPanel::refreshHeader()
{
    m_headerLabel->setText(tr("未分组学生 (%1)").arg(cardCount()));
}

void UngroupedPanel::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setPen(QPen(kRailBorder, 1.0));
    painter.setBrush(kRailSurface);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kRailRadius, kRailRadius);
}

void UngroupedPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_mask->setGeometry(rect());
    m_mask->raise();
}

void UngroupedPanel::updateMaskProgress(qreal progress)
{
    m_maskProgress = progress;
    //蒙版是本文件内的DropMask实例，此处按已知类型直接驱动进度
    static_cast<DropMask*>(m_mask)->setProgress(progress);
}

void UngroupedPanel::startMaskAnimation(bool visible)
{
    m_maskAnimation->stop();
    m_maskAnimation->setStartValue(m_maskProgress);
    m_maskAnimation->setEndValue(visible ? 1.0 : 0.0);
    m_maskAnimation->start();
}

void UngroupedPanel::dragEnterEvent(QDragEnterEvent* event)
{
    int studentIndex = -1;
    QSize cardSize;
    if(!StudentCard::decodeDragPayload(event->mimeData(), studentIndex, cardSize))
    {
        event->ignore();
        return;
    }
    startMaskAnimation(true);
    event->acceptProposedAction();
}

void UngroupedPanel::dragMoveEvent(QDragMoveEvent* event)
{
    int studentIndex = -1;
    QSize cardSize;
    if(!StudentCard::decodeDragPayload(event->mimeData(), studentIndex, cardSize))
    {
        event->ignore();
        return;
    }
    event->acceptProposedAction();
}

void UngroupedPanel::dragLeaveEvent(QDragLeaveEvent* event)
{
    startMaskAnimation(false);
    event->accept();
}

void UngroupedPanel::dropEvent(QDropEvent* event)
{
    int studentIndex = -1;
    QSize cardSize;
    if(!StudentCard::decodeDragPayload(event->mimeData(), studentIndex, cardSize))
    {
        event->ignore();
        return;
    }
    startMaskAnimation(false);
    event->acceptProposedAction();
    emit studentDropped(studentIndex);
}
