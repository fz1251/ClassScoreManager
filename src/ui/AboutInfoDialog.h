// about_info_dialog.h
#ifndef ABOUTINFODIALOG_H
#define ABOUTINFODIALOG_H

#include <QDialog>
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
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void enableEffects();

private:
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
    static const inline int BLUR_RADIUS = 8;
};

#endif // ABOUTINFODIALOG_H
