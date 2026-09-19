#include "UngroupedPanel.h"

#include "FlowLayout.h"
#include "StudentCard.h"
#include "exfonticon.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOptionGroupBox>
#include <QVBoxLayout>

namespace
{
constexpr int kPanelMinWidth = 200; //刚好放得下最宽的胶囊、边距与滚动条
constexpr int kMaskDurationMs = 140;
constexpr qreal kFrameInset = 1.5;            //与样式库绘制GroupBox边框时的内缩一致
constexpr int kDefaultRoundingRadius = 4;     //样式库 secondLevelRoundingRadius 的默认值
constexpr QColor kMaskColor(211, 47, 47);     //蒙版与虚线
constexpr QColor kMaskTextColor(155, 28, 28); //深红图标与提示文字
constexpr char16_t kDeleteGlyph = static_cast<char16_t>(SegoeIcon::Delete);
constexpr int kMaskIconPixelSize = 48;
constexpr int kMaskTextPointSize = 20;
constexpr int kIconTextSpacing = 10;

//边框圆角半径取自样式库的全局属性，取不到就退回它的默认值
qreal frameRoundingRadius()
{
    const QVariant radius = qApp->property("secondLevelRoundingRadius");
    return radius.isValid() ? radius.toDouble() : kDefaultRoundingRadius;
}

//覆盖在面板之上的红色蒙版，提示拖到此处即取消分组
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

        //蒙版与虚线都取GroupBox边框的矩形和圆角，正好盖在边框上
        const QRectF frame = frameRect();
        const qreal radius = frameRoundingRadius();

        QColor tint = kMaskColor;
        tint.setAlphaF(0.30 * m_progress);
        painter.setPen(Qt::NoPen);
        painter.setBrush(tint);
        painter.drawRoundedRect(frame, radius, radius);

        QColor border = kMaskColor;
        border.setAlphaF(0.85 * m_progress);
        painter.setPen(QPen(border, 1.0, Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(frame, radius, radius);

        //深红的垃圾桶图标与提示文字，整体在框内垂直居中
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
        const qreal top = frame.top() + (frame.height() - iconHeight - kIconTextSpacing - textHeight) / 2.0;

        painter.setPen(contentColor);
        painter.setFont(iconFont);
        painter.drawText(QRectF(frame.left(), top, frame.width(), iconHeight), Qt::AlignCenter, QString(QChar(kDeleteGlyph)));
        painter.setFont(textFont);
        painter.drawText(QRectF(frame.left(), top + iconHeight + kIconTextSpacing, frame.width(), textHeight), Qt::AlignCenter, m_text);
    }

private:
    //按GroupBox当前样式算出边框矩形，与样式库绘制时用的是同一个子控件
    QRectF frameRect() const
    {
        const auto* groupBox = qobject_cast<const QGroupBox*>(parentWidget());
        if(!groupBox)
            return QRectF(rect()).marginsRemoved(QMarginsF(kFrameInset, kFrameInset, kFrameInset, kFrameInset));

        QStyleOptionGroupBox option;
        option.initFrom(groupBox);
        option.text = groupBox->title();
        option.textAlignment = groupBox->alignment();
        option.subControls = QStyle::SC_GroupBoxFrame;
        option.lineWidth = groupBox->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, groupBox);
        option.midLineWidth = 0;
        const QRect frame = groupBox->style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxFrame, groupBox);
        return QRectF(frame).marginsRemoved(QMarginsF(kFrameInset, kFrameInset, kFrameInset, kFrameInset));
    }

    QString m_text;
    qreal m_progress = 0.0;
};
}

UngroupedPanel::UngroupedPanel(QWidget* parent)
    : QGroupBox(parent)
{
    setAcceptDrops(true);
    setMinimumWidth(kPanelMinWidth);

    //内容区边距由QGroupBox按样式给出的标题高度自动计算，这里只控制间距
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(8);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->viewport()->setAutoFillBackground(false);

    m_cardContainer = new QWidget;
    m_cardLayout = new FlowLayout(m_cardContainer, 0, 0, 2);
    m_cardLayout->setSingleColumn(true);
    m_scrollArea->setWidget(m_cardContainer);
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
    setTitle(tr("未分组学生 (%1)").arg(cardCount()));
}

void UngroupedPanel::resizeEvent(QResizeEvent* event)
{
    QGroupBox::resizeEvent(event);
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
