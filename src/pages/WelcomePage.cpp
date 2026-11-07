#include "WelcomePage.h"

#include "../widgets/HomeActionCard.h"
#include "../widgets/ShineLabel.h"

#include <QFont>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHideEvent>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QRadialGradient>
#include <QRectF>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSpacerItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

namespace
{
// 光斑纹理边长（物理像素）：略大于窗口短边，缩放绘制时边缘仍然柔和
constexpr int kTextureSide = 1024;

// 渐变抖动幅度：极淡的 8 位渐变只有几级色阶差，±2 的抖动足以打散条带
constexpr int kDitherAmplitude = 2;

// 动画帧间隔（毫秒）：10fps 下漂移无明显卡顿，提高帧率只增加 CPU 占用
constexpr int kFrameIntervalMs = 100;

// 单帧最大步进（秒）：限制休眠恢复或长时间阻塞后的单帧位移
constexpr qreal kMaxFrameDeltaSeconds = 0.2;

// 光斑尺寸起伏的幅度与频率：直径在 ±12% 内变化，周期约四十秒
constexpr qreal kBreathAmplitude = 0.12;
constexpr qreal kBreathSpeed = 0.15;

struct GlowSpec
{
    QColor color;
    QPointF baseCenter;
    qreal diameterFactor;
    qreal scaleX;
    qreal scaleY;
    qreal phaseX;
    qreal phaseY;
    qreal speedX;
    qreal speedY;
    qreal driftX;
    qreal driftY;
};

// 四团大光斑：左灰蓝、中偏右下粉、最右灰紫、下偏左青；青与粉保持饱和度，蓝与紫用低饱和度铺底
// scaleX/scaleY 将正圆拉伸为椭圆且面积保持约 1.0，避免规则正圆的几何感
const GlowSpec kGlowSpecs[] = {
    {QColor(206, 218, 234, 50), QPointF(0.15, 0.45), 1.45, 1.15, 0.87, 0.0, 1.7, 0.420, 0.500, 0.12, 0.08},
    {QColor(246, 190, 214, 70), QPointF(0.60, 0.65), 1.35, 0.92, 1.09, 2.3, 0.4, 0.720, 0.550, 0.14, 0.10},
    {QColor(202, 190, 238, 58), QPointF(0.92, 0.40), 1.30, 1.10, 0.91, 4.1, 3.0, 0.540, 0.660, 0.07, 0.08},
    {QColor(170, 222, 242, 52), QPointF(0.30, 0.85), 1.55, 1.20, 0.83, 1.2, 5.1, 0.600, 0.520, 0.12, 0.08},
};

QColor scaledAlpha(const QColor& color, qreal factor)
{
    QColor result = color;
    result.setAlpha(qRound(color.alpha() * factor));
    return result;
}

int ditherNoise(int x, int y)
{
    quint32 hash = quint32(x) * 374761393u + quint32(y) * 668265263u;
    hash = (hash ^ (hash >> 13)) * 1274126177u;
    return int((hash >> 16) % (2 * kDitherAmplitude + 1)) - kDitherAmplitude;
}

// 逐像素抖动：极淡的渐变在 8 位色深下只有几级色阶差，不抖动会形成可见条带
void applyDither(QImage& image)
{
    for(int y = 0; y < image.height(); ++y)
    {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for(int x = 0; x < image.width(); ++x)
        {
            const QRgb pixel = line[x];
            if(qAlpha(pixel) == 0)
                continue;
            const int noise = ditherNoise(x, y);
            line[x] = qRgba(qBound(0, qRed(pixel) + noise, 255), qBound(0, qGreen(pixel) + noise, 255),
                            qBound(0, qBlue(pixel) + noise, 255), qAlpha(pixel));
        }
    }
}

QPixmap makeGlowPixmap(int side, const QColor& color)
{
    QImage image(side, side, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal radius = side / 2.0;
    QRadialGradient gradient(radius, radius, radius);
    gradient.setColorAt(0.00, color);
    gradient.setColorAt(0.30, scaledAlpha(color, 0.75));
    gradient.setColorAt(0.65, scaledAlpha(color, 0.22));
    gradient.setColorAt(1.00, Qt::transparent);
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawEllipse(QRectF(0.0, 0.0, side, side));
    painter.end();

    applyDither(image);
    return QPixmap::fromImage(image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
}
} // namespace

WelcomePage::WelcomePage(QWidget* parent)
    : QWidget(parent)
{
    for(int i = 0; i < kGlowCount; ++i)
    {
        const GlowSpec& spec = kGlowSpecs[i];
        GlowLayer& glow = m_glows[i];
        glow.color = spec.color;
        glow.baseCenter = spec.baseCenter;
        glow.diameterFactor = spec.diameterFactor;
        glow.scaleX = spec.scaleX;
        glow.scaleY = spec.scaleY;
        glow.phaseX = spec.phaseX;
        glow.phaseY = spec.phaseY;
        glow.speedX = spec.speedX;
        glow.speedY = spec.speedY;
        glow.driftX = spec.driftX;
        glow.driftY = spec.driftY;
        glow.pixmap = makeGlowPixmap(kTextureSide, glow.color);
    }

    m_frameTimer = new QTimer(this);
    m_frameTimer->setInterval(kFrameIntervalMs);
    m_frameTimer->setTimerType(Qt::PreciseTimer);
    connect(m_frameTimer, &QTimer::timeout, this,
            [this]
            {
                const qreal delta = qMin(m_clock.restart() / 1000.0, kMaxFrameDeltaSeconds);
                // 失焦或最小化时定时器照常触发，但不推进相位、不重绘
                if(!isVisible() || window()->isMinimized() || QGuiApplication::applicationState() != Qt::ApplicationActive)
                    return;
                m_phase += delta;
                update();
            });

    // 页面根布局：标题固定在顶部，其余空间全部给卡片区域
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(0);
    rootLayout->setContentsMargins(40, 40, 40, 40);

    // 标题：应用名称
    auto* titleLabel = new ShineLabel(this);
    //必须从应用字体派生：默认构造的 QFont 族名为空，会被解析成宋体
    QFont titleFont = font();
    titleFont.setPointSize(42);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setText(tr("班级积分管理系统"));
    rootLayout->addWidget(titleLabel, 0);

    // 底部区域：占据全部剩余空间，卡片组在其中横向、纵向居中
    auto* cardAreaLayout = new QVBoxLayout;
    cardAreaLayout->setSpacing(0);
    cardAreaLayout->addStretch();

    // 操作卡片横向布局：两侧弹簧保持卡片组水平居中
    auto* cardLayout = new QHBoxLayout;
    cardLayout->setSpacing(0);
    cardLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    // 打开文件卡片
    auto* openFileCard = new HomeActionCard(tr("打开文件"), this);
    openFileCard->setIcon(QIcon(QStringLiteral(":/icons/resources/icons/open-file.svg")));
    cardLayout->addWidget(openFileCard);

    // 两张卡片之间的固定间距
    cardLayout->addSpacing(80);

    // 新建文件卡片
    auto* newFileCard = new HomeActionCard(tr("新建文件"), this);
    newFileCard->setIcon(QIcon(QStringLiteral(":/icons/resources/icons/new-file.svg")));
    cardLayout->addWidget(newFileCard);

    cardLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    cardAreaLayout->addLayout(cardLayout);
    cardAreaLayout->addStretch();
    rootLayout->addLayout(cardAreaLayout, 1);

    connect(openFileCard, &QAbstractButton::clicked, this, &WelcomePage::openFileRequested);
    connect(newFileCard, &QAbstractButton::clicked, this, &WelcomePage::newFileRequested);
}

void WelcomePage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.fillRect(rect(), palette().color(QPalette::Window));

    const qreal shortSide = qMin(width(), height());
    for(const GlowLayer& glow : m_glows)
    {
        const QPointF center = glowCenter(glow);
        const qreal breath = 1.0 + kBreathAmplitude * qSin(m_phase * kBreathSpeed + glow.phaseX);
        const qreal diameter = shortSide * glow.diameterFactor * breath;
        const qreal targetWidth = diameter * glow.scaleX;
        const qreal targetHeight = diameter * glow.scaleY;
        const QRectF target(center.x() - targetWidth / 2.0, center.y() - targetHeight / 2.0, targetWidth, targetHeight);
        painter.drawPixmap(target, glow.pixmap, QRectF(glow.pixmap.rect()));
    }
}

void WelcomePage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    m_clock.start();
    m_frameTimer->start();
}

void WelcomePage::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_frameTimer->stop();
}

QPointF WelcomePage::glowCenter(const GlowLayer& layer) const
{
    const qreal x = layer.baseCenter.x() + layer.driftX * qSin(m_phase * layer.speedX + layer.phaseX);
    const qreal y = layer.baseCenter.y() + layer.driftY * qCos(m_phase * layer.speedY + layer.phaseY);
    return QPointF(x * width(), y * height());
}
