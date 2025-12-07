// about_info_dialog.h
#ifndef ABOUT_INFO_DIALOG_H
#define ABOUT_INFO_DIALOG_H

#include <QDialog>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QElapsedTimer>
#include <QHBoxLayout>

namespace Ui
{
class AboutInfoDialog;
}

class AboutInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutInfoDialog(QWidget *parent = nullptr);
    ~AboutInfoDialog();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void enableEffects();

private:
    enum ResizeRegion
    {
        NoEdge = 0,
        Left = 1,
        Top = 2,
        Right = 4,
        Bottom = 8,
        TopLeft = Top | Left,
        TopRight = Top | Right,
        BottomLeft = Bottom | Left,
        BottomRight = Bottom | Right
    };
    void enableBackgroundBlurEffect();
    void windowBounce(int dx, int dy, int duration_ms = 350);
    Ui::AboutInfoDialog* ui;
    //动态创建的窗口控件
    QHBoxLayout* titleBarLayout = nullptr;
    //启用特效的按键序列
    static const inline QList<int> m_keySequence =
    {
        Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down,
        Qt::Key_Left, Qt::Key_Left, Qt::Key_Right, Qt::Key_Right
    };
    //是否已启用特效标识
    bool m_effectsEnabled = false;
    //是否已启用背景透明标识
    bool m_translucentBackground = false;
    //系统版本标识
    bool m_isWindows7 = false;
    bool m_isWindows10or11 = false;
    bool m_windowsSupportsAcrylic = false;
    //拖动相关支持
    bool m_enableDragging = false;
    bool m_isDragging = false;
    bool m_isResizing = false;
    QPoint m_drag_start;
    QElapsedTimer m_timer;
    //拉伸相关支持
    static const inline int BLUR_RADIUS = 6; //边缘检测宽度
    static const inline QColor shadowStart{0, 0, 0, 70}; //阴影渐变起始颜色
    static const inline QColor shadowEnd  {0, 0, 0, 0};  //阴影渐变结束颜色

    QRect m_windowRect;
    ResizeRegion m_currentRegion = NoEdge;
    ResizeRegion getResizeRegion(const QPoint& pos);
};

#endif // ABOUT_INFO_DIALOG_H
