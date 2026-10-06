#include "AboutInfoDialog.h"

#include "../widgets/LineSpacedLabel.h"
#include "app_build_info.h"

#include <QButtonGroup>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QOperatingSystemVersion>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QSize>
#include <QStringList>
#include <QWKWidgets/widgetwindowagent.h>
#include <QWindow>

#ifdef Q_OS_WIN
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>
#endif

namespace
{
// 方向键弹跳动画时长，也是彩蛋触发前等待动画结束的时间
constexpr int kBounceDurationMs = 350;
// 标题栏到窗口边缘的留白：缩放边界厚度在 100% DPI 下为 SM_CXSIZEFRAME(4) + SM_CXPADDEDBORDER(4)，
// Win32 度量随 DPI 放大、Qt 再除以 DPR，因此逻辑像素下恒为 8
constexpr int kTitleBarMargin = 8;

const QString closeButtonStyle = QStringLiteral(R"(
#closeButton {
    border: none;
    background-color: transparent;
}
)");

const QString modeButtonStyle = QStringLiteral(R"(
#acrylicButton, #micaButton, #micaAltButton, #blurButton {
    min-width: 80px;
    background-color: transparent;
    border: 1px solid rgba(20, 20, 20, 0.25);
    padding: 3px 6px;
    color: #4a4a4a;
    font: normal 14pt;
}

#acrylicButton {
    border-right: none;
    border-top-left-radius: 8px;
    border-bottom-left-radius: 8px;
}

#micaButton, #micaAltButton {
    border-right: none;
}

#blurButton {
    border-top-right-radius: 8px;
    border-bottom-right-radius: 8px;
}

#acrylicButton:checked, #micaButton:checked, #micaAltButton:checked, #blurButton:checked {
    color: #000;
}
)");
} // namespace

AboutInfoDialog::AboutInfoDialog(QWidget* parent)
    : QDialog(parent)
{
#ifdef Q_OS_WIN
    // 从新到旧的阶梯：每档只判断一次，档位之间的包含关系由枚举顺序表达
    const QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
    if(osVer >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 22621))
        m_winVersion = WindowsVersion::Win11_22H2;
    else if(osVer >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 22000))
        m_winVersion = WindowsVersion::Win11;
    else if(osVer >= QOperatingSystemVersion::Windows10)
        m_winVersion = WindowsVersion::Win10;
    else if(osVer >= QOperatingSystemVersion::Windows7 && osVer < QOperatingSystemVersion::Windows8)
        m_winVersion = WindowsVersion::Win7;
#endif
    resize(550, 300);
    setWindowTitle(tr("关于"));

    // 对话框根布局：容纳无边框窗口标题栏与下方文字区
    outerLayout = new QVBoxLayout(this);
    outerLayout->setObjectName(QStringLiteral("outerLayout"));
    outerLayout->setSpacing(0);
    outerLayout->setContentsMargins(0, 0, 0, 0);

    /*
     * 窗口样式取舍说明
     *
     * 【Win10 及以上】不使用 Qt::FramelessWindowHint。QWindowKit 保留 QDialog 默认的
     * WS_OVERLAPPEDWINDOW（含 WS_THICKFRAME | WS_CAPTION），仅去掉 WS_SYSMENU，再以
     * WM_NCCALCSIZE 把整窗声明为客户区：
     *  — WS_THICKFRAME 使命中测试返回的 HT* 缩放值生效。
     *  — WS_CAPTION 使 DWM 按有标题栏的坐标系对齐，确保 Qt mapFromGlobal 按钮命中无偏移。
     *
     * 【Win7】保留原生标题栏与系统帧：Aero 玻璃与四边缩放都由系统帧提供，去掉标题栏会一并失去，
     * 因此只在 nativeEvent 中把客户区补成拖动区。
     *
     * 【背景透明】WA_TranslucentBackground → WS_EX_LAYERED，系统玻璃与窗口材质得以透过显示；
     * paintEvent 填充近乎不可见的底色，避免全透明导致点击穿透。
     */
    if(m_winVersion != WindowsVersion::Unadapted)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        m_translucentBackground = true;
    }
    if(m_winVersion >= WindowsVersion::Win10)
        createFramelessTitleBar();

    // 方向键弹跳动画：复用同一对象，重复触发时以当前位置重新起跳
    m_bounceAnimation = new QPropertyAnimation(this, "pos", this);
    m_bounceAnimation->setDuration(kBounceDurationMs);
    m_bounceAnimation->setEasingCurve(QEasingCurve::OutInCirc);

    // 文字区垂直布局：标题、副标题与内容按 1:1:4 分配纵向空间
    auto* textLayout = new QVBoxLayout;
    textLayout->setObjectName(QStringLiteral("textLayout"));
    textLayout->setSpacing(12);
    // 有无边框标题栏时顶部空间由标题栏占据，文字区不再重复留白
    textLayout->setContentsMargins(25, m_winVersion >= WindowsVersion::Win10 ? 0 : 20, 25, 20);

    // 标题：应用名称
    titleLabel = new QLabel(this);
    titleLabel->setObjectName(QStringLiteral("titleLabel"));
    //标题与欢迎页大标题统一，直接用应用字体；不要再 setBold，合成粗体会把笔画糊成一团
    QFont titleFont = font();
    titleFont.setPointSize(24);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setText(tr("班级积分管理系统"));
    textLayout->addWidget(titleLabel);

    // 副标题：构建日期
    subtitleLabel = new QLabel(this);
    subtitleLabel->setObjectName(QStringLiteral("subtitleLabel"));
    //副标题：日期数字指定 Segoe UI（数字更规整）；纯西文字体不含汉字，只对数字用富文本局部指定
    QFont subtitleFont = font();
    subtitleFont.setPointSize(16);
    subtitleLabel->setFont(subtitleFont);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setTextFormat(Qt::RichText);
    subtitleLabel->setText(tr("构建日期：<span style=\"font-family:'Segoe UI'\">%1</span>").arg(QStringLiteral(APP_BUILD_DATE)));
    textLayout->addWidget(subtitleLabel);

    // 内容：技术栈、版权与仓库信息
    contentLabel = new LineSpacedLabel(this);
    contentLabel->setObjectName(QStringLiteral("contentLabel"));
    contentLabel->setLineHeight(1.4);
    //必须从应用字体派生：默认构造的 QFont 族名为空，会被解析成宋体
    QFont contentFont = font();
    contentFont.setPointSize(12);
    contentLabel->setFont(contentFont);
    contentLabel->setWordWrap(false);
    const QString contentText = tr("Made with C++\n"
                                   "Powered by Qt %1\n"
                                   "Copyright© 2025-2026 mengfanzhen0501@163.com\n"
                                   "本程序以MIT协议开源\n"
                                   "仓库链接: github.com/fz1251/ClassScoreManager")
                                    .arg(QStringLiteral(QT_VERSION_STR));
    contentLabel->setText(contentText);
    contentLabel->setFixedWidth(contentLabel->sizeHint().width());
    textLayout->setAlignment(contentLabel, Qt::AlignLeft);
    textLayout->addWidget(contentLabel);

    textLayout->setStretch(0, 1);
    textLayout->setStretch(1, 1);
    textLayout->setStretch(2, 4);

    // 文字区占满剩余空间，标题栏保持自身高度
    outerLayout->addLayout(textLayout, 1);
}

void AboutInfoDialog::createFramelessTitleBar()
{
    // QWindowKit 窗口代理：接管 WM_NCCALCSIZE、WM_NCHITTEST、边缘缩放与 DWM 帧扩展
    windowAgent = new QWK::WidgetWindowAgent(this);
    windowAgent->setup(this);
    // 整个对话框都是拖动区，可交互的控件再用 setHitTestVisible 逐个放行
    windowAgent->setTitleBar(this);

    // 标题栏布局：只有右侧按钮是实体，其余空白即拖动区
    titleBarLayout = new QHBoxLayout;
    titleBarLayout->setObjectName(QStringLiteral("titleBarLayout"));
    titleBarLayout->setSpacing(0);
    // 四周留出与缩放边界等宽的空隙，避免按钮压在缩放判定区上
    titleBarLayout->setContentsMargins(kTitleBarMargin, kTitleBarMargin, kTitleBarMargin, kTitleBarMargin);
    titleBarLayout->addStretch();

    // 无边框窗口的关闭按钮
    const QSize buttonSize = {30, 30};
    auto* closeButton = new QPushButton(this);
    closeButton->setObjectName(QStringLiteral("closeButton"));
    closeButton->setFocusPolicy(Qt::ClickFocus);
    closeButton->setFixedSize(buttonSize);
    closeButton->setIconSize(buttonSize);
    m_closeIconNormal.addFile(":/icons/resources/about-dialog-close.svg");
    m_closeIconHover.addFile(":/icons/resources/about-dialog-close-hovered.svg");
    closeButton->setIcon(m_closeIconNormal);
    closeButton->installEventFilter(this);
    windowAgent->setHitTestVisible(closeButton, true);
    titleBarLayout->addWidget(closeButton);
    connect(closeButton, &QPushButton::clicked, this, &AboutInfoDialog::close);

    outerLayout->addLayout(titleBarLayout);

    setStyleSheet(m_winVersion >= WindowsVersion::Win11 ? closeButtonStyle + modeButtonStyle : closeButtonStyle);
}

void AboutInfoDialog::keyPressEvent(QKeyEvent* event)
{
    if(m_effectsEnabled)
        return;
    const int key = event->key();
    switch(key)
    {
        case Qt::Key_Up:
            windowBounce(0, -30);
            break;
        case Qt::Key_Down:
            windowBounce(0, 30);
            break;
        case Qt::Key_Left:
            windowBounce(-30, 0);
            break;
        case Qt::Key_Right:
            windowBounce(30, 0);
            break;
        default:
            QDialog::keyPressEvent(event);
            break;
    }
    if(m_keySequence.at(m_keyIndex) == key)
    {
        m_keyIndex++;
        if(m_keyIndex == m_keySequence.size())
        {
            // 立即标记，不再进行按键序列判断，防止越界访问
            m_effectsEnabled = true;
            QTimer::singleShot(kBounceDurationMs + 1, this, &AboutInfoDialog::enableEffects);
        }
    }
    else
    {
        m_keyIndex = 0;
    }

    event->accept();
}

bool AboutInfoDialog::eventFilter(QObject* watched, QEvent* event)
{
    if(watched->objectName() == "closeButton")
    {
        QPushButton* btn = qobject_cast<QPushButton*>(watched);
        if(event->type() == QEvent::Enter)
            btn->setIcon(m_closeIconHover);
        else if(event->type() == QEvent::Leave)
            btn->setIcon(m_closeIconNormal);
    }
    return QDialog::eventFilter(watched, event);
}

void AboutInfoDialog::paintEvent(QPaintEvent* event)
{
    // WS_EX_LAYERED 下背景全透明会点击穿透到下层窗口，必须填充底色
    if(m_translucentBackground)
    {
        QPainter painter(this);
        painter.fillRect(rect(), m_effectsEnabled ? QColor(255, 255, 255, 1) : QColor(Qt::white));
    }
    QDialog::paintEvent(event);
}

bool AboutInfoDialog::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
#ifdef Q_OS_WIN
    // QWidget::nativeEvent 的 eventType 在 Windows 上是 QPA 插件写死的常量，没有第二种取值，无需判断
    Q_UNUSED(eventType);

    // Win10 及以上的命中测试由 QWindowKit 处理；Win7 保留原生标题栏与系统帧，
    // 这里只把客户区补成拖动区（标题栏与边框不在 rect() 内，交回系统处理）
    if(m_winVersion == WindowsVersion::Win7)
    {
        MSG* msg = static_cast<MSG*>(message);
        if(msg->message == WM_NCHITTEST)
        {
            // GET_X/Y_LPARAM 正确处理多显示器负坐标，不可用 LOWORD/HIWORD 替代
            const int xPos = GET_X_LPARAM(msg->lParam);
            const int yPos = GET_Y_LPARAM(msg->lParam);
            const QPoint clientPos = mapFromGlobal((QPointF(xPos, yPos) / windowHandle()->devicePixelRatio()).toPoint());
            if(rect().contains(clientPos))
            {
                *result = HTCAPTION;
                return true;
            }
        }
    }
#endif
    return false;
}

void AboutInfoDialog::enableEffects()
{
    repaint(); // 重绘窗口，使背景更新，显示透明
    auto animateWidgetFadeOut = [this](QWidget* widget, int duration_ms = 2000)
    {
        if(!widget)
            return;

        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);

        QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
        animation->setDuration(duration_ms);
        animation->setStartValue(1.0);
        animation->setEndValue(0.0);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        connect(animation, &QAbstractAnimation::finished, widget, &QWidget::hide);
        connect(animation, &QAbstractAnimation::finished, this,
                [this]()
                {
                    // 文字隐藏后补一个弹簧，避免剩余纵向空间被重新分摊
                    if(!m_stretchAdded)
                    {
                        outerLayout->addStretch();
                        m_stretchAdded = true;
                    }
                });
    };
    animateWidgetFadeOut(titleLabel);
    animateWidgetFadeOut(subtitleLabel);
    animateWidgetFadeOut(contentLabel);
    enableBackdrop();
}

void AboutInfoDialog::enableBackdrop()
{
#ifdef Q_OS_WIN
    // Win7：原生标题栏与系统帧保留，把 DWM 帧扩展到整个客户区，得到全窗 Aero 玻璃
    if(m_winVersion == WindowsVersion::Win7)
    {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        BOOL dwmEnabled = FALSE;
        if(SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled)
        {
            const MARGINS margins = {-1, -1, -1, -1};
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
        return;
    }

    // Win11：Acrylic / Mica / MicaAlt / Blur 按钮组
    if(m_winVersion >= WindowsVersion::Win11)
    {
        auto* acrylicButton = new QPushButton(this);
        auto* micaButton = new QPushButton(this);
        auto* blurButton = new QPushButton(this);
        acrylicButton->setObjectName("acrylicButton");
        micaButton->setObjectName("micaButton");
        blurButton->setObjectName("blurButton");
        acrylicButton->setText("Acrylic");
        micaButton->setText("Mica");
        blurButton->setText("Blur");
        for(QPushButton* button : {acrylicButton, micaButton, blurButton})
        {
            button->setCheckable(true);
            button->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            button->setFocusPolicy(Qt::ClickFocus);
        }

        QButtonGroup* group = new QButtonGroup(this);

        // 按钮顺序：Acrylic, Mica, (MicaAlt), Blur
        titleBarLayout->insertWidget(0, acrylicButton);
        titleBarLayout->insertWidget(1, micaButton);
        group->addButton(acrylicButton, 1);
        group->addButton(micaButton, 2);

        // MicaAlt 从 Win11 22H2 起提供
        const bool hasMicaAlt = m_winVersion >= WindowsVersion::Win11_22H2;
        QPushButton* micaAltButton = nullptr;
        if(hasMicaAlt)
        {
            micaAltButton = new QPushButton(this);
            micaAltButton->setObjectName("micaAltButton");
            micaAltButton->setText("MicaAlt");
            micaAltButton->setCheckable(true);
            micaAltButton->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            micaAltButton->setFocusPolicy(Qt::ClickFocus);
            titleBarLayout->insertWidget(2, micaAltButton);
            group->addButton(micaAltButton, 3);
        }

        titleBarLayout->insertWidget(hasMicaAlt ? 3 : 2, blurButton);
        group->addButton(blurButton, hasMicaAlt ? 4 : 3);
        acrylicButton->setChecked(true);

        // 材质按钮必须登记为可命中，否则会被整窗拖动区吞掉点击
        windowAgent->setHitTestVisible(acrylicButton, true);
        windowAgent->setHitTestVisible(micaButton, true);
        windowAgent->setHitTestVisible(blurButton, true);
        if(micaAltButton)
            windowAgent->setHitTestVisible(micaAltButton, true);

        const auto connectBackdrop = [this](QPushButton* button, const QString& key)
        {
            connect(button, &QPushButton::clicked, this,
                    [this, key](bool checked)
                    {
                        if(checked)
                            setBackdrop(key);
                    });
        };
        connectBackdrop(acrylicButton, QStringLiteral("acrylic-material"));
        connectBackdrop(micaButton, QStringLiteral("mica"));
        if(micaAltButton)
            connectBackdrop(micaAltButton, QStringLiteral("mica-alt"));
        connectBackdrop(blurButton, QStringLiteral("dwm-blur"));

        setBackdrop(QStringLiteral("acrylic-material"));
        return;
    }

    // Win10：只用 Blur。Win10 的 ACCENT_ENABLE_ACRYLICBLURBEHIND 存在性能缺陷：高轮询率鼠标
    // （250Hz 及以上）移动或调整窗口大小时，后台合成/渲染在高频输入下形成瓶颈，窗口响应延迟
    // 甚至出现"幽灵光标"——鼠标释放后窗口仍持续移动，仿佛在消化积压的事件队列。
    if(m_winVersion == WindowsVersion::Win10)
        setBackdrop(QStringLiteral("dwm-blur"));
#endif
}

void AboutInfoDialog::setBackdrop(const QString& key)
{
    // 四个材质键互斥，先清掉其余键再设置目标键
    static const QStringList backdropKeys = {QStringLiteral("dwm-blur"), QStringLiteral("acrylic-material"), QStringLiteral("mica"),
                                             QStringLiteral("mica-alt")};
    for(const QString& backdropKey : backdropKeys)
        windowAgent->setWindowAttribute(backdropKey, false);
    windowAgent->setWindowAttribute(key, true);
}

void AboutInfoDialog::windowBounce(int dx, int dy)
{
    // 落点：一段弹跳结束后才重新记录，连按期间终点保持不变，窗口最终回到起跳前的位置
    if(m_bounceAnimation->state() != QAbstractAnimation::Running)
        m_bounceHome = pos();
    // 起点与峰值都相对当前位置：连按时从当前位置续接，弹幅在上一段的基础上叠加
    const QPoint origin = pos();
    m_bounceAnimation->stop();
    m_bounceAnimation->setStartValue(origin);
    m_bounceAnimation->setKeyValueAt(0.5, origin + QPoint(dx, dy));
    m_bounceAnimation->setEndValue(m_bounceHome);
    m_bounceAnimation->start();
}
