#include "UngroupedPanel.h"

#include "FlowLayout.h"
#include "StudentCard.h"
#include "exfonticon.h"

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QGraphicsBlurEffect>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRadialGradient>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
constexpr int kPanelMinWidth = 120; //够放 3 个字的名字，再窄长名字就会被裁切
constexpr int kEdgeMargin = 12;
constexpr int kHeaderSpacing = 8;
constexpr int kMaskEnterMs = 500; //出现要慢慢化开
constexpr int kMaskLeaveMs = 300; //收起要干脆
constexpr int kRailRadius = 12;   //与右侧小组卡片、学生胶囊同一套圆角
constexpr QColor kRailBorder(0, 0, 0, 30);
constexpr char16_t kDeleteGlyph = static_cast<char16_t>(SegoeIcon::Delete);
constexpr int kMaskIconPixelSize = 48;
constexpr int kMaskTextPointSize = 20;
constexpr int kIconTextSpacing = 10;
constexpr qreal kMaskIconDelay = 0.25;  //图标与文字比底色晚这么多进度起淡入，做出错峰
constexpr qreal kMaskIconSpread = 0.55; //错峰占用的进度区间

//淡红薄膜：只给毛玻璃染上一层红味，不压住底下的模糊。两档透明度决定"红到什么程度"
constexpr QColor kMaskFilmColor(211, 47, 47);
constexpr int kMaskTintAlphaCenter = 25;
constexpr int kMaskTintAlphaEdge = 0;
constexpr QColor kMaskInkColor(179, 38, 30); //深红：垃圾桶图标与提示文字

//毛玻璃：用 Qt 的图形模糊效果生成一张缓存图，整段拖动只算一次
constexpr qreal kBackdropBlurRadius = 8.0;

QImage blurredImage(const QImage& source, qreal radius)
{
    QGraphicsScene scene;
    auto* item = new QGraphicsPixmapItem(QPixmap::fromImage(source));
    auto* effect = new QGraphicsBlurEffect;
    effect->setBlurRadius(radius);
    item->setGraphicsEffect(effect);
    scene.addItem(item);

    QImage result(source.size(), QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    scene.render(&painter);
    return result;
}

//覆盖在栏面上的毛玻璃蒙版，提示拖到此处即取消分组
class DropMask : public QWidget
{
public:
    DropMask(QScrollArea* area, const QString& text, QWidget* parent)
        : QWidget(parent)
        , m_area(area)
        , m_text(text)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setVisible(false);
    }

    void setProgress(qreal progress)
    {
        const bool wasHidden = m_progress <= 0.0;
        m_progress = progress;
        if(wasHidden && progress > 0.0)
            rebuildBackdrop();
        //透明度归零才隐藏：逐帧反复切换可见性会白白走一遍显隐流程
        const bool visible = progress > 0.002;
        if(isVisible() != visible)
            setVisible(visible);
        update();
    }

    //列表内容变化后模糊底已过期，下次显形时重新采样
    void invalidateBackdrop()
    {
        m_backdrop = QPixmap();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

        //整块栏面都在圆角里，蒙版的每一层都不越出栏面
        QPainterPath rail;
        rail.addRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kRailRadius, kRailRadius);
        painter.setClipPath(rail);
        painter.setOpacity(m_progress);

        //模糊底只铺列表区：表头在模糊之外保持清晰
        if(!m_backdrop.isNull())
            painter.drawPixmap(m_backdropRect, m_backdrop);

        //淡红薄膜铺满整块栏面，中心略深、四角略浅
        const QPointF center(m_backdropRect.isNull() ? rect().center() : QPointF(m_backdropRect.center()));
        const qreal reach = qMax(rect().width(), rect().height()) * 0.75;
        QRadialGradient tint(center, reach);
        tint.setColorAt(0.0, QColor(kMaskFilmColor.red(), kMaskFilmColor.green(), kMaskFilmColor.blue(), kMaskTintAlphaCenter));
        tint.setColorAt(1.0, QColor(kMaskFilmColor.red(), kMaskFilmColor.green(), kMaskFilmColor.blue(), kMaskTintAlphaEdge));
        painter.setPen(Qt::NoPen);
        painter.setBrush(tint);
        painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kRailRadius, kRailRadius);

        //垃圾桶图标与提示文字在整块栏面里居中，深红压在淡红的毛玻璃上
        QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
        iconFont.setPixelSize(kMaskIconPixelSize);
        //图标字体只走自带的图标字族，否则会掉进合并字体里画出个不相干的汉字
        iconFont.setStyleStrategy(QFont::NoFontMerging);
        QFont textFont = font();
        textFont.setPointSize(kMaskTextPointSize);
        textFont.setLetterSpacing(QFont::AbsoluteSpacing, 2.0);

        const QFontMetricsF iconMetrics(iconFont);
        const QFontMetricsF textMetrics(textFont);
        const qreal iconHeight = iconMetrics.height();
        const qreal textHeight = textMetrics.height();
        const qreal blockTop = (rect().height() - iconHeight - kIconTextSpacing - textHeight) / 2.0;
        const QRectF iconRect(0, blockTop, rect().width(), iconHeight);
        const QRectF textRect(0, iconRect.bottom() + kIconTextSpacing, rect().width(), textHeight);

        //前景比底色晚起淡入，错峰出现
        painter.setOpacity(m_progress * qBound(0.0, (m_progress - kMaskIconDelay) / kMaskIconSpread, 1.0));
        painter.setFont(iconFont);
        painter.setPen(kMaskInkColor);
        painter.drawText(iconRect, Qt::AlignCenter, QString(QChar(kDeleteGlyph)));
        painter.setFont(textFont);
        painter.setPen(kMaskInkColor);
        painter.drawText(textRect, Qt::AlignCenter, m_text);
        painter.setOpacity(1.0);
    }

private:
    void rebuildBackdrop()
    {
        m_backdropRect = m_area->geometry();
        if(m_backdropRect.isEmpty())
            return;

        const qreal ratio = devicePixelRatioF();
        //采样面板自身：此刻蒙版刚显形、进度为 0，抓进来的是一张干净的面板
        const QImage shot = parentWidget()->grab(m_backdropRect).toImage();
        if(shot.isNull())
            return;

        const QImage blurred = blurredImage(shot, kBackdropBlurRadius);

        QPixmap result(blurred.size());
        result.setDevicePixelRatio(ratio);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(QRectF(0, 0, m_backdropRect.width(), m_backdropRect.height()), blurred);
        m_backdrop = result;
    }

    QScrollArea* m_area = nullptr;
    QString m_text;
    QPixmap m_backdrop;
    QRect m_backdropRect;
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

    m_mask = new DropMask(m_scrollArea, tr("取消分组"), this);

    m_maskAnimation = new QVariantAnimation(this);
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
    static_cast<DropMask*>(m_mask)->invalidateBackdrop();
    refreshHeader();
}

void UngroupedPanel::removeCard(StudentCard* card)
{
    m_cardLayout->removeWidget(card);
    static_cast<DropMask*>(m_mask)->invalidateBackdrop();
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
    static_cast<DropMask*>(m_mask)->invalidateBackdrop();
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
    //栏面用页面的全局底色，左栏与其余界面同底，靠圆角与描边立出栏面
    painter.setBrush(palette().color(QPalette::Window));
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), kRailRadius, kRailRadius);
}

void UngroupedPanel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    m_mask->setGeometry(rect());
    m_mask->raise();
    //尺寸变了，缓存下来的模糊底不再对应列表区
    static_cast<DropMask*>(m_mask)->invalidateBackdrop();
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
    const int duration = visible ? kMaskEnterMs : kMaskLeaveMs;
    if(m_maskAnimation->duration() != duration)
    {
        //时长不同必须重设：只在变化时改，避免打断正在跑的补间
        m_maskAnimation->setDuration(duration);
        m_maskAnimation->setEasingCurve(visible ? QEasingCurve::OutCubic : QEasingCurve::InCubic);
    }
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
