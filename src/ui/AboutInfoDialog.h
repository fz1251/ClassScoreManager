// about_info_dialog.h
#ifndef ABOUTINFODIALOG_H
#define ABOUTINFODIALOG_H

#include <QDialog>
#include <QIcon>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QTimer>
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
    bool eventFilter(QObject *watched, QEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

private slots:
    void enableEffects();

private:
    void enableBackgroundBlurEffect();
    void windowBounce(int dx, int dy, int duration_ms = 350);
    Ui::AboutInfoDialog* ui;
    // 动态创建的窗口控件
    QHBoxLayout* titleBarLayout = nullptr;
    // 关闭按钮的 Normal/Hover 图标，eventFilter 中切换
    QIcon m_closeIconNormal;
    QIcon m_closeIconHover;
    // 启用特效的按键序列
    static const inline QVector<int> m_keySequence =
    {
        Qt::Key_Up, Qt::Key_Up, Qt::Key_Down, Qt::Key_Down,
        Qt::Key_Left, Qt::Key_Left, Qt::Key_Right, Qt::Key_Right
    };
    // 是否已启用特效标识
    bool m_effectsEnabled = false;
    // 是否已启用背景透明标识
    bool m_translucentBackground = false;
    // 系统版本标识
    bool m_isWin7 = false;
    bool m_isWin10or11 = false;
    bool m_isWin11 = false;
    bool m_gtWin11_22H2 = false;
    // 8 = SM_CXFRAME 或 SM_CYFRAME(4) + SM_CXPADDEDBORDER(4)，用作 WM_NCHITTEST 边缘检测的缩放边界厚度
    static const inline int BLUR_RADIUS = 8;
};

#endif // ABOUTINFODIALOG_H
