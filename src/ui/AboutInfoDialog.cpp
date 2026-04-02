// about_info_dialog.cpp
#include "AboutInfoDialog.h"
#include "ui_AboutInfoDialog.h"
#include <QPushButton>
#include <QButtonGroup>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QOperatingSystemVersion>

AboutInfoDialog::AboutInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutInfoDialog)
{
    //检测系统版本
#ifdef Q_OS_WIN
    const QOperatingSystemVersion osVer = QOperatingSystemVersion::current();
    if(osVer >= QOperatingSystemVersion::Windows10)
    {
        m_isWindows10or11 = true;
        if(osVer >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 17134))
        {
            //自Windows 10 1803 (Build 17134)后支持亚克力特效，否则只有模糊特效
            m_windowsSupportsAcrylic = true;
        }
    }
    else if(osVer >= QOperatingSystemVersion::Windows7 && osVer < QOperatingSystemVersion::Windows8)
    {
        m_isWindows7 = true;
    }
#endif
    ui->setupUi(this);
    if(m_isWindows7 || m_isWindows10or11)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        m_translucentBackground = true;
    }
    if(m_isWindows10or11)
    {
        setWindowFlag(Qt::FramelessWindowHint);
        setMouseTracking(true);
        m_enableDragging = true;

        //手动模拟窗口标题栏
        titleBarLayout = new QHBoxLayout();
        titleBarLayout->setObjectName("titleBarLayout");
        titleBarLayout->setSpacing(0);
        titleBarLayout->setContentsMargins(BLUR_RADIUS, BLUR_RADIUS, BLUR_RADIUS, BLUR_RADIUS);
        titleBarLayout->addStretch();

        //因窗口无边框，添加一个关闭按钮
        QPushButton* closeButton = new QPushButton(this);
        closeButton->setObjectName("closeButton");
        closeButton->setFixedSize({24, 24});
        closeButton->setFocusPolicy(Qt::ClickFocus);
        titleBarLayout->addWidget(closeButton);

        ui->OuterVLayout->insertLayout(0, titleBarLayout);

        connect(closeButton, &QPushButton::clicked, this, &AboutInfoDialog::close);
        setStyleSheet(R"(
#closeButton {
    background: transparent;
    border: none;
    padding: 2px;
    background-image: url(:/icons/resources/dialog-close.png);
    background-repeat: no-repeat;
    background-position: center;
}

#closeButton:hover {
    background-image: url(:/icons/resources/dialog-close-hovered.png);
}

#acrylicButton, #blurButton {
    background-color: transparent;
    border: 1px solid #ccc;
    padding: 3px 5px;
    color: #666;
    font: normal 14pt;
    min-width: 80px;
}

#acrylicButton {
    border-right: none;
}

#blurButton {
    border-left: 1px solid #ccc;
}

#acrylicButton:checked, #blurButton:checked {
    font: bold 14pt;
    color: #000;
}
    )");

    }
    m_timer.start();
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
            //立即标记，不在进行按键序列判断，防止越界访问
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
        QPainter painter(this);
        if(m_effectsEnabled)
        {
            painter.fillRect(rect(), QColor(255, 255, 255, 1));
        }
        else
        {
            if(m_isWindows10or11)
            {
                const int w = this->width();
                const int h = this->height();
                const int blur = BLUR_RADIUS;
                const QRect contentRect = rect().adjusted(blur, blur, -blur, -blur);

                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(Qt::NoPen);

                painter.fillRect(contentRect, Qt::white);

                // 裁剪：挖去中间内容区，仅保留边缘区域用于绘制阴影
                QPainterPath clip;
                clip.addRect(rect());
                clip.addRect(blur, blur, w - 2 * blur, h - 2 * blur);
                painter.setClipPath(clip);

                QPainterPath path;

                // --- 绘制4个角落 (扇形) ---
                QRadialGradient radialGradient;
                QPointF center;

                // --- 左上角 ---
                center = QPointF(blur, blur);
                radialGradient.setCenter(center);
                radialGradient.setFocalPoint(center);
                radialGradient.setRadius(blur);
                radialGradient.setColorAt(0, shadowStart);
                radialGradient.setColorAt(1, shadowEnd);
                painter.setBrush(radialGradient);

                path.moveTo(center);
                // 左上角：从90度开始，逆时针画90度
                path.arcTo(0, 0, blur * 2, blur * 2, 90, 90);
                painter.drawPath(path);
                path.clear();

                // --- 右上角 ---
                center = QPointF(w - blur, blur);
                radialGradient.setCenter(center);
                radialGradient.setFocalPoint(center);
                radialGradient.setRadius(blur);
                radialGradient.setColorAt(0, shadowStart);
                radialGradient.setColorAt(1, shadowEnd);
                painter.setBrush(radialGradient);

                path.moveTo(center);
                // 右上角：从0度开始，逆时针画90度
                path.arcTo(w - blur * 2, 0, blur * 2, blur * 2, 0, 90);
                painter.drawPath(path);
                path.clear();

                // --- 左下角 ---
                center = QPointF(blur, h - blur);
                radialGradient.setCenter(center);
                radialGradient.setFocalPoint(center);
                radialGradient.setRadius(blur);
                radialGradient.setColorAt(0, shadowStart);
                radialGradient.setColorAt(1, shadowEnd);
                painter.setBrush(radialGradient);

                path.moveTo(center);
                // 左下角：从180度开始，逆时针画90度
                path.arcTo(0, h - blur * 2, blur * 2, blur * 2, 180, 90);
                painter.drawPath(path);
                path.clear();

                // --- 右下角 ---
                center = QPointF(w - blur, h - blur);
                radialGradient.setCenter(center);
                radialGradient.setFocalPoint(center);
                radialGradient.setRadius(blur);
                radialGradient.setColorAt(0, shadowStart);
                radialGradient.setColorAt(1, shadowEnd);
                painter.setBrush(radialGradient);

                path.moveTo(center);
                // 右下角：从270度开始，逆时针画90度
                path.arcTo(w - blur * 2, h - blur * 2, blur * 2, blur * 2, 270, 90);
                painter.drawPath(path);
                path.clear();


                // --- 绘制四条边（内缩 blur，避免与角重叠）---
                QLinearGradient linear;
                linear.setColorAt(0, shadowStart);
                linear.setColorAt(1, shadowEnd);

                // 上边
                linear.setStart(w / 2, blur);
                linear.setFinalStop(w / 2, 0);
                painter.setBrush(linear);
                path.addRect(blur, 0, w - 2 * blur, blur);
                painter.drawPath(path);
                path.clear();

                // 下边
                linear.setStart(w / 2, h - blur);
                linear.setFinalStop(w / 2, h);
                painter.setBrush(linear);
                path.addRect(blur, h - blur, w - 2 * blur, blur);
                painter.drawPath(path);
                path.clear();

                // 左边
                linear.setStart(blur, h / 2);
                linear.setFinalStop(0, h / 2);
                painter.setBrush(linear);
                path.addRect(0, blur, blur, h - 2 * blur);
                painter.drawPath(path);
                path.clear();

                // 右边
                linear.setStart(w - blur, h / 2);
                linear.setFinalStop(w, h / 2);
                painter.setBrush(linear);
                path.addRect(w - blur, blur, blur, h - 2 * blur);
                painter.drawPath(path);
            }
            else
            {
                painter.fillRect(rect(), Qt::white);
            }
        }
    }
    QDialog::paintEvent(event);
}

void AboutInfoDialog::mousePressEvent(QMouseEvent *event)
{
    if (!m_enableDragging)
        return;

    if (event->button() == Qt::LeftButton && m_currentRegion != NoEdge)
    {
        m_isResizing = true;
        m_drag_start = event->globalPosition().toPoint();
        m_windowRect = geometry();
    }
    else if (event->button() == Qt::LeftButton)
    {
        // 不在边缘区域，开始拖动
        m_isDragging = true;
        m_drag_start = event->globalPosition().toPoint() - this->pos();
        m_timer.restart();
    }

    event->accept();
}

void AboutInfoDialog::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_enableDragging)
        return;

    if (m_isDragging && (event->buttons() & Qt::LeftButton))
    {
        if (m_timer.elapsed() >= 20)
        {
            m_timer.restart();
            // 拖动逻辑：新位置 = 当前全局鼠标位置 - 初始偏移
            move(event->globalPosition().toPoint() - m_drag_start);
            event->accept();
        }
    }
    else if (m_isResizing && (event->buttons() & Qt::LeftButton))
    {
        if (m_timer.elapsed() >= 20)
        {
            m_timer.restart();

            QPoint delta = event->globalPosition().toPoint() - m_drag_start;
            QRect newGeom = m_windowRect;
            switch (m_currentRegion)
            {
            case Left:
                newGeom.setLeft(m_windowRect.left() + delta.x());
                break;
            case Right:
                newGeom.setRight(m_windowRect.right() + delta.x());
                break;
            case Top:
                newGeom.setTop(m_windowRect.top() + delta.y());
                break;
            case Bottom:
                newGeom.setBottom(m_windowRect.bottom() + delta.y());
                break;
            case TopLeft:
                newGeom.setTopLeft(m_windowRect.topLeft() + delta);
                break;
            case TopRight:
                newGeom.setTopRight(m_windowRect.topRight() + delta);
                break;
            case BottomLeft:
                newGeom.setBottomLeft(m_windowRect.bottomLeft() + delta);
                break;
            case BottomRight:
                newGeom.setBottomRight(m_windowRect.bottomRight() + delta);
                break;
            default:
                break;
            }
            if(newGeom.height() > ui->OuterVLayout->minimumHeightForWidth(newGeom.width()))
            {
                setGeometry(newGeom);
            }
        }
    }
    else
    {
        // 设置鼠标光标形状
        ResizeRegion region = getResizeRegion(event->pos());
        m_currentRegion = region;
        switch (region)
        {
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            unsetCursor();
            break;
        }
    }

    event->accept();
}

void AboutInfoDialog::mouseReleaseEvent(QMouseEvent *event)
{
    if(!m_enableDragging)
        return ;
    if (event->button() == Qt::LeftButton)
    {
        m_isDragging = false;
        m_isResizing = false;
        event->accept();
    }
}

#ifdef Q_OS_WIN

#include <windows.h>
#include <dwmapi.h>

enum AccentState
{
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4
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
#endif

/* 旧版关闭按钮样式：
QPushButton#pushButton {
    color: #fff;
    border-radius: 6px;
    padding: 8px 20px;
    font-weight: 500;
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 rgba(6,14,131,255),stop:1 rgba(12,25,180,255));
    border: none;
}

QPushButton#pushButton:hover {
    background: qlineargradient(x1:0, y1:0, y2:1,stop:0 rgba(0,3,255,255),stop:1 rgba(2,126,251,255));
} */
void AboutInfoDialog::enableEffects()
{
    repaint();//重绘窗口，使背景更新，显示透明
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
    if(m_isWindows10or11)
    {
        //设置背景模糊的lambda
        auto setWindowComposition = [this](AccentState effectState)
        {
            HWND hwnd = reinterpret_cast<HWND>(winId());
            static pSetWindowCompositionAttribute fn = nullptr;
            if (!fn)
            {
                HMODULE hMod = GetModuleHandleW(L"user32.dll");
                if (hMod)
                {
                    fn = reinterpret_cast<pSetWindowCompositionAttribute>(
                             reinterpret_cast<void*>(
                                 GetProcAddress(hMod, "SetWindowCompositionAttribute")));
                }
            }
            if (fn)
            {
                AccentPolicy policy = { effectState, 2, 0x01FFFFFF, 0 };
                WindowCompositionAttributeData data = { 19, &policy, sizeof(policy) };
                fn(hwnd, &data);

            }
        };

        if(m_windowsSupportsAcrylic)
        {
            //添加亚克力和模糊模式的切换按钮
            QPushButton* acrylicButton = new QPushButton(this);
            QPushButton* blurButton = new QPushButton(this);
            acrylicButton->setObjectName("acrylicButton");
            blurButton->setObjectName("blurButton");
            acrylicButton->setText("Acrylic");
            blurButton->setText("Blur");
            acrylicButton->setCheckable(true);
            blurButton->setCheckable(true);
            acrylicButton->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            blurButton->setSizePolicy({QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed});
            acrylicButton->setFocusPolicy(Qt::ClickFocus);
            blurButton->setFocusPolicy(Qt::ClickFocus);
            titleBarLayout->insertWidget(0, acrylicButton);
            titleBarLayout->insertWidget(1, blurButton);

            QButtonGroup* group = new QButtonGroup(this);
            group->addButton(acrylicButton, 1);
            group->addButton(blurButton, 2);
            acrylicButton->setChecked(true);

            connect(acrylicButton, &QPushButton::clicked, this, [setWindowComposition](bool checked)
            {
                if(checked)
                    setWindowComposition(ACCENT_ENABLE_ACRYLICBLURBEHIND);
            });
            connect(blurButton, &QPushButton::clicked, this, [setWindowComposition](bool checked)
            {
                if(checked)
                    setWindowComposition(ACCENT_ENABLE_BLURBEHIND);
            });
        }

        //设置背景模糊的初始状态
        if(m_windowsSupportsAcrylic)
        {
            setWindowComposition(ACCENT_ENABLE_ACRYLICBLURBEHIND);
        }
        else
        {
            setWindowComposition(ACCENT_ENABLE_BLURBEHIND);
        }
        return ;
    }
    else if(m_isWindows7)

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
    //不符合要求的Windows和其它系统什么也不做
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

AboutInfoDialog::ResizeRegion AboutInfoDialog::getResizeRegion(const QPoint& pos)
{
    bool onLeft = pos.x() <= BLUR_RADIUS;
    bool onRight = pos.x() >= width() - BLUR_RADIUS;
    bool onTop = pos.y() <= BLUR_RADIUS;
    bool onBottom = pos.y() >= height() - BLUR_RADIUS;

    if (onTop && onLeft) return TopLeft;
    if (onTop && onRight) return TopRight;
    if (onBottom && onLeft) return BottomLeft;
    if (onBottom && onRight) return BottomRight;
    if (onTop) return Top;
    if (onBottom) return Bottom;
    if (onLeft) return Left;
    if (onRight) return Right;
    return NoEdge;
}
