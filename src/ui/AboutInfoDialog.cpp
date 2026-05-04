// about_info_dialog.cpp
#include "AboutInfoDialog.h"
#include "ui_AboutInfoDialog.h"
#include <QWindow>
#include <QPushButton>
#include <QButtonGroup>
#include <QPixmap>
#include <QPainter>

#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QOperatingSystemVersion>

#ifdef Q_OS_WIN
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif
#endif

#ifdef Q_OS_WIN
namespace {
const QString closeButtonStyle = QStringLiteral(R"(
#closeButton {
    border: none;
    padding: 2px;
    background: transparent url(:/icons/resources/dialog-close.png) no-repeat center;
}

#closeButton:hover {
    background-image: url(:/icons/resources/dialog-close-hovered.png);
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
}
#endif

// TODO: Win11 DWM 材料效果验证 — 当前实现在 Win10 已验证，Win11 待实测
//
// 【当前实现】
// Win11 材料：DwmExtendFrameIntoClientArea({65536,0,0,0}) + DWMWA_SYSTEMBACKDROP_TYPE
//              (Acrylic→DWMSBT_TRANSIENTWINDOW, Mica→DWMSBT_MAINWINDOW, MicaAlt→DWMSBT_TABBEDWINDOW)
// Win11 Blur：SetWindowCompositionAttribute(ACCENT_ENABLE_BLURBEHIND)，沿用 Win10 路径
// 背景透明：  WA_TranslucentBackground → WS_EX_LAYERED，paintEvent 填 QColor(255,255,255,1) 防点击穿透
// 窗口阴影：  DwmExtendFrameIntoClientArea({1,0,0,0}) 左侧 1px 扩展
// 标题栏：    无 FramelessWindowHint，QDialog 默认 WS_THICKFRAME|WS_CAPTION，WM_NCCALCSIZE 整窗为客户区
//
// 【争议：WS_EX_LAYERED 是否破坏 DWMWA_SYSTEMBACKDROP_TYPE？】
// 说法 A（某 WinUI3 文章）：SetWindowRgn→WS_EX_LAYERED 触发 DWM "降级熔断"，Mica 变纯白或 fallback Acrylic。
//                          要求 WS_EX_LAYERED=FALSE、禁用自定义区域、WS_THICKFRAME 必须启用。
// 说法 B（B站视频演示）：  WA_TranslucentBackground + {-1,-1,-1,-1} 全窗扩展 + DWMWA_SYSTEMBACKDROP_TYPE，
//                          Mica/Acrylic 正常渲染。Qt 窗口需设背景透明。
// 分析：我们未使用 SetWindowRgn，窗口区域完整。SetWindowRgn 与 WA_TranslucentBackground 虽然都设
//       WS_EX_LAYERED，但前者还破坏窗口区域拓扑（这是真正的致命因素）。后者仅分层化，区域完整，
//       理论上 DWM 合成管线仍可正常运作。说法 A 的警告可能不适用于我们。
//
// 【争议：帧扩展边距】
// 视频方案：{-1,-1,-1,-1} 全窗扩展 → 材料覆盖整个窗口
// 当前方案：{65536,0,0,0} 仅左侧大幅扩展（源自 ElaWidgetTools / WinUI3 源码）
// 待测：两种边距在材料渲染上有无差异？左侧单边扩展是否影响四边阴影均匀性？
//
// 【Win11 测试清单】
// □ Acrylic / Mica / MicaAlt / Blur 四种模式切换是否即时生效
// □ 背景材料是否正常渲染（非纯白/灰色，非明显降级）
// □ Mica 是否正确采样桌面壁纸（而非实时模糊）
// □ DWM 原生阴影在四种模式下是否均正常
// □ 尝试 {-1,-1,-1,-1} 替代 {65536,0,0,0}，对比材料渲染差异
// □ 若材料失效，尝试临时去掉 WA_TranslucentBackground，验证是否因 WS_EX_LAYERED 被 DWM 拒绝
// □ Win11 22H2+ 的 MicaAlt 是否与 Mica 有可见差异
//
// 【已知问题：DWM 帧扩展的 1px 白线 — 已选择顶部方案】
// 测试结果：{0,0,0,0}=无阴影；{1,0,0,0}=阴影在左，左侧多 1px 白线；{0,1,0,0}=同理在右；
//          {0,0,1,0}=阴影在上方，新增白线与 WS_CAPTION 残留的顶部白线重叠，只显一条。
// 结论：扩展方向决定了白线位置，无论哪边都会有一条。选择顶部是因为 WS_CAPTION 本身在顶部
//       已有 1px 渲染残留，DWM 帧扩展的白线与之重叠，不会在额外边缘引入新线条，视觉干扰最小。

AboutInfoDialog::AboutInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutInfoDialog)
{
#ifdef Q_OS_WIN
    const QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
    if(osVer >= QOperatingSystemVersion::Windows10)
    {
        m_isWin10or11 = true;
        if(osVer >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 22000))
            m_isWin11 = true;
        if(osVer >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 22621))
            m_gtWin11_22H2 = true;
    }
    else if(osVer >= QOperatingSystemVersion::Windows7 && osVer < QOperatingSystemVersion::Windows8)
    {
        m_isWin7 = true;
    }
#endif
    ui->setupUi(this);
    if(m_isWin7 || m_isWin10or11)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        m_translucentBackground = true;
        if(m_isWin10or11)
        {
            // setWindowFlag(Qt::FramelessWindowHint);
            /*
             * 窗口样式取舍说明：
             * — 不使用 Qt::FramelessWindowHint，QDialog 默认 WS_OVERLAPPEDWINDOW 含 WS_THICKFRAME|WS_CAPTION。
             * — WS_THICKFRAME 使 WM_NCHITTEST 返回的 HT* 缩放值生效。
             * — WS_CAPTION 使 DWM 正确对齐窗口坐标系，确保 Qt 的 mapFromGlobal 按钮命中无偏移。
             *   WM_NCCALCSIZE 将整窗声明为客户区，标题栏不渲染但 DWM 坐标系仍按有标题栏对齐。
             * — DwmExtendFrameIntoClientArea 顶部扩展 1px DWM 帧启用原生阴影，白线与 WS_CAPTION 残留重叠。
             * — WA_TranslucentBackground 引入 WS_EX_LAYERED，供 Blur/Acrylic/Mica 材料透过显示。
             * — Win10 仅提供 Blur 不提供 Acrylic：全窗亚克力 GPU 开销高且旧 API 路径质量差，Blur 纯净通透、与 Win7 行为一致。
             */
            HWND hwnd = reinterpret_cast<HWND>(winId());

            // 顶部扩展 1px DWM 帧以启用原生阴影，白线与 WS_CAPTION 残留重叠，视觉干扰最小
            const MARGINS shadowMargins = {0, 0, 1, 0};
            DwmExtendFrameIntoClientArea(hwnd, &shadowMargins);

            // 手动模拟窗口标题栏
            titleBarLayout = new QHBoxLayout();
            titleBarLayout->setObjectName("titleBarLayout");
            titleBarLayout->setSpacing(0);
            titleBarLayout->setContentsMargins(BLUR_RADIUS, BLUR_RADIUS, BLUR_RADIUS, BLUR_RADIUS);
            titleBarLayout->addStretch();

            // 无边框窗口需添加一个关闭按钮
            QPushButton* closeButton = new QPushButton(this);
            closeButton->setObjectName("closeButton");
            closeButton->setFixedSize({24, 24});
            closeButton->setFocusPolicy(Qt::ClickFocus);
            titleBarLayout->addWidget(closeButton);

            ui->OuterVLayout->insertLayout(0, titleBarLayout);

            connect(closeButton, &QPushButton::clicked, this, &AboutInfoDialog::close);
            if(m_isWin11)
                setStyleSheet(closeButtonStyle + modeButtonStyle);
            else
                setStyleSheet(closeButtonStyle);
        }
    }
}

AboutInfoDialog::~AboutInfoDialog()
{
    delete ui;
}

void AboutInfoDialog::keyPressEvent(QKeyEvent *event)
{
    static int m_curKeyIndex = 0;
    if(m_effectsEnabled)
        return ;
    int key = event->key();
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
    if(m_keySequence.at(m_curKeyIndex) == key)
    {
        m_curKeyIndex++;
        if(m_curKeyIndex == m_keySequence.size())
        {
            // 立即标记，不在进行按键序列判断，防止越界访问
            m_effectsEnabled = true;
            QTimer::singleShot(351, this, &AboutInfoDialog::enableEffects);
        }
    }
    else
    {
        m_curKeyIndex = 0;
    }

    event->accept();
}

void AboutInfoDialog::paintEvent(QPaintEvent *event)
{
    if(m_translucentBackground)
    {
        // 防止 WS_EX_LAYERED + 全透明导致点击穿透到下层窗口
        QPainter painter(this);
        if(m_effectsEnabled)
        {
            painter.fillRect(rect(), QColor(255, 255, 255, 1));
        }
        else
        {
            painter.fillRect(rect(), Qt::white);
        }
    }
    QDialog::paintEvent(event);
}

bool AboutInfoDialog::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
#ifdef Q_OS_WIN
    if (!m_isWin10or11)
        return false;

    if (eventType == "windows_generic_MSG")
    {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_NCCALCSIZE)
        {
            if (msg->wParam == TRUE)
            {
                // 整窗声明为客户区，WS_CAPTION 标题栏不渲染，但其 DWM 坐标系对齐仍生效
                *result = 0;
                return true;
            }
        }
        else if (msg->message == WM_NCHITTEST)
        {
            // GET_X/Y_LPARAM 正确处理多显示器负坐标，不可用 LOWORD/HIWORD 替代
            const int xPos = GET_X_LPARAM(msg->lParam);
            const int yPos = GET_Y_LPARAM(msg->lParam);

            // 放行按钮区域以免被 HTCAPTION 覆盖
            const QPoint clientPos = mapFromGlobal(
                QPointF(xPos, yPos) / windowHandle()->devicePixelRatio()).toPoint();
            QWidget *child = childAt(clientPos);
            if (child && qobject_cast<QPushButton *>(child))
                return false;

            // 边缘检测用 Win32 屏幕坐标（Qt 无法感知 WS_THICKFRAME 边框区域）
            RECT wr;
            GetWindowRect(reinterpret_cast<HWND>(winId()), &wr);
            const int hitMargin = qRound(BLUR_RADIUS * windowHandle()->devicePixelRatio());
            const int rx = xPos - wr.left;
            const int ry = yPos - wr.top;
            const int ww = wr.right - wr.left;
            const int wh = wr.bottom - wr.top;

            const bool onLeft   = rx <= hitMargin;
            const bool onRight  = rx >= ww - hitMargin;
            const bool onTop    = ry <= hitMargin;
            const bool onBottom = ry >= wh - hitMargin;

            if (onTop && onLeft)     { *result = HTTOPLEFT;     return true; }
            if (onTop && onRight)    { *result = HTTOPRIGHT;    return true; }
            if (onBottom && onLeft)  { *result = HTBOTTOMLEFT;  return true; }
            if (onBottom && onRight) { *result = HTBOTTOMRIGHT; return true; }
            if (onTop)               { *result = HTTOP;         return true; }
            if (onBottom)            { *result = HTBOTTOM;      return true; }
            if (onLeft)              { *result = HTLEFT;        return true; }
            if (onRight)             { *result = HTRIGHT;       return true; }

            *result = HTCAPTION; // 剩余区域均可拖动
            return true;
        }
    }
#endif
    return false;
}

#ifdef Q_OS_WIN

enum AccentState
{
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOST_BACKDROP = 5,
    ACCENT_INVALID_STATE = 6
};

struct AccentPolicy
{
    AccentState AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
};

struct WindowCompositionAttributeData
{
    int Attribute;
    PVOID Data;
    ULONG DataSize;
};

typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WindowCompositionAttributeData*);

enum DWM_SYSTEMBACKDROP_TYPE
{
    DWMSBT_AUTO = 0,
    DWMSBT_NONE = 1,
    DWMSBT_MAINWINDOW = 2,
    DWMSBT_TRANSIENTWINDOW = 3,
    DWMSBT_TABBEDWINDOW = 4
};

const DWORD DWMWA_SYSTEMBACKDROP_TYPE = 38;
const DWORD DWMWA_MICA_EFFECT = 1029;

typedef HRESULT (WINAPI *pDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
#endif

void AboutInfoDialog::enableEffects()
{
    repaint(); // 重绘窗口，使背景更新，显示透明
    auto animateWidgetFadeOut = [this](QWidget * widget, int duration_ms = 2000)
    {
        if (!widget) return;

        QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);

        QPropertyAnimation* animation = new QPropertyAnimation(effect, "opacity");
        animation->setDuration(duration_ms);
        animation->setStartValue(1.0);
        animation->setEndValue(0.0);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
        connect(animation, &QAbstractAnimation::finished, widget, &QWidget::hide);

        static bool firstEntered = true;
        if(firstEntered)
        {
            connect(animation, &QAbstractAnimation::finished, this, [this]()
            {
                ui->OuterVLayout->addStretch();
            });
            firstEntered = false;
        }
    };
    animateWidgetFadeOut(ui->label);
    animateWidgetFadeOut(ui->label_2);
    animateWidgetFadeOut(ui->label_3);
    enableBackgroundBlurEffect();
}

void AboutInfoDialog::enableBackgroundBlurEffect()
{
#ifdef Q_OS_WIN
    if(m_isWin10or11)
    {
        HWND hwnd = reinterpret_cast<HWND>(winId());

        // 通过 DwmSetWindowAttribute 设置 Win11 原生材料
        auto dwmSetAttr = [hwnd](DWORD attr, LPCVOID data, DWORD size)
        {
            static pDwmSetWindowAttribute fn = nullptr;
            if (!fn)
            {
                HMODULE hMod = GetModuleHandleW(L"dwmapi.dll");
                if (hMod)
                    fn = reinterpret_cast<pDwmSetWindowAttribute>(
                         reinterpret_cast<void*>(
                        GetProcAddress(hMod, "DwmSetWindowAttribute")));
            }
            if (fn)
                fn(hwnd, attr, data, size);
        };

        // 通过 SetWindowCompositionAttribute 设置 Blur（Win10/Win11 通用）
        auto setWindowComposition = [this](AccentState effectState)
        {
            HWND hwnd = reinterpret_cast<HWND>(winId());
            static pSetWindowCompositionAttribute fn = nullptr;
            if (!fn)
            {
                HMODULE hMod = GetModuleHandleW(L"user32.dll");
                if (hMod)
                    fn = reinterpret_cast<pSetWindowCompositionAttribute>(
                         reinterpret_cast<void*>(
                        GetProcAddress(hMod, "SetWindowCompositionAttribute")));
            }
            if (fn)
            {
                AccentPolicy policy = { effectState, 2, 0x01FFFFFF, 0 };
                WindowCompositionAttributeData data = { 19, &policy, sizeof(policy) };
                fn(hwnd, &data);
            }
        };

        // 为 Win11 原生材料扩展 DWM 绘制区域
        auto extendMargins = [hwnd]()
        {
            static const MARGINS margins = {65536, 0, 0, 0};
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        };

        if(m_isWin11)
        {
            // Win11：Acrylic / Mica / MicaAlt / Blur 按钮组
            QPushButton* acrylicButton = new QPushButton(this);
            QPushButton* micaButton = new QPushButton(this);
            QPushButton* blurButton = new QPushButton(this);
            acrylicButton->setObjectName("acrylicButton");
            micaButton->setObjectName("micaButton");
            blurButton->setObjectName("blurButton");
            acrylicButton->setText("Acrylic");
            micaButton->setText("Mica");
            blurButton->setText("Blur");
            acrylicButton->setCheckable(true);
            micaButton->setCheckable(true);
            blurButton->setCheckable(true);
            acrylicButton->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            micaButton->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            blurButton->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            acrylicButton->setFocusPolicy(Qt::ClickFocus);
            micaButton->setFocusPolicy(Qt::ClickFocus);
            blurButton->setFocusPolicy(Qt::ClickFocus);

            QButtonGroup* group = new QButtonGroup(this);

            // 按钮顺序：Acrylic, Mica, (MicaAlt), Blur
            titleBarLayout->insertWidget(0, acrylicButton);
            titleBarLayout->insertWidget(1, micaButton);
            group->addButton(acrylicButton, 1);
            group->addButton(micaButton, 2);

            QPushButton* micaAltButton = nullptr;
            if(m_gtWin11_22H2)
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

            titleBarLayout->insertWidget(m_gtWin11_22H2 ? 3 : 2, blurButton);
            group->addButton(blurButton, m_gtWin11_22H2 ? 4 : 3);
            acrylicButton->setChecked(true);

            // Acrylic — 通过 DWMWA_SYSTEMBACKDROP_TYPE
            connect(acrylicButton, &QPushButton::clicked, this, [extendMargins, dwmSetAttr](bool checked)
            {
                if(checked)
                {
                    extendMargins();
                    const DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_TRANSIENTWINDOW;
                    dwmSetAttr(DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
                }
            });

            // Mica — 22H2 用 DWMWA_SYSTEMBACKDROP_TYPE，旧版用 DWMWA_MICA_EFFECT
            connect(micaButton, &QPushButton::clicked, this, [this, extendMargins, dwmSetAttr](bool checked)
            {
                if(checked)
                {
                    extendMargins();
                    if(m_gtWin11_22H2)
                    {
                        const DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_MAINWINDOW;
                        dwmSetAttr(DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
                    }
                    else
                    {
                        const BOOL enable = TRUE;
                        dwmSetAttr(DWMWA_MICA_EFFECT, &enable, sizeof(enable));
                    }
                }
            });

            // MicaAlt — 22H2+ 通过 DWMWA_SYSTEMBACKDROP_TYPE
            if(micaAltButton)
            {
                connect(micaAltButton, &QPushButton::clicked, this, [extendMargins, dwmSetAttr](bool checked)
                {
                    if(checked)
                    {
                        extendMargins();
                        const DWM_SYSTEMBACKDROP_TYPE type = DWMSBT_TABBEDWINDOW;
                        dwmSetAttr(DWMWA_SYSTEMBACKDROP_TYPE, &type, sizeof(type));
                    }
                });
            }

            // Blur — 沿用 SetWindowCompositionAttribute
            connect(blurButton, &QPushButton::clicked, this, [setWindowComposition](bool checked)
            {
                if(checked)
                    setWindowComposition(ACCENT_ENABLE_BLURBEHIND);
            });

            // 默认选中 Acrylic
            extendMargins();
            const DWM_SYSTEMBACKDROP_TYPE defaultType = DWMSBT_TRANSIENTWINDOW;
            dwmSetAttr(DWMWA_SYSTEMBACKDROP_TYPE, &defaultType, sizeof(defaultType));
        }
        else
        {
            // Win10：仅 Blur，不创建切换按钮
            setWindowComposition(ACCENT_ENABLE_BLURBEHIND);
        }
        return ;
    }
    else if(m_isWin7)
    {
        HWND hwnd = reinterpret_cast<HWND>(winId());
        BOOL dwmEnabled = FALSE;
        if (SUCCEEDED(DwmIsCompositionEnabled(&dwmEnabled)) && dwmEnabled)
        {
            MARGINS margins = { -1, -1, -1, -1 };
            DwmExtendFrameIntoClientArea(hwnd, &margins);
        }
        return ;
    }
#endif
    // 不符合要求的Windows和其它系统什么也不做
}

void AboutInfoDialog::windowBounce(int dx, int dy, int duration_ms)
{
    QPoint startPos = pos();
    QPoint endPos(startPos.x() + dx, startPos.y() + dy);
    QPropertyAnimation *animation = new QPropertyAnimation(this, "pos");
    animation->setDuration(duration_ms);
    animation->setStartValue(startPos);
    animation->setKeyValueAt(0.5, endPos);
    animation->setEndValue(startPos);
    animation->setEasingCurve(QEasingCurve::OutInCirc);
    animation->start(QPropertyAnimation::DeleteWhenStopped);
}