#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QLabel>
#include <QPaintEvent>
#include <QTimer>
#include <QVBoxLayout>

class LineSpacedLabel;
class QPropertyAnimation;

namespace QWK
{
class WidgetWindowAgent;
}

class AboutInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutInfoDialog(QWidget* parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

private slots:
    void enableEffects();

private:
    void createFramelessTitleBar();
    void enableBackdrop();
    void setBackdrop(const QString& key);
    void windowBounce(int dx, int dy);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QVBoxLayout* outerLayout = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* subtitleLabel = nullptr;
    LineSpacedLabel* contentLabel = nullptr;
    QHBoxLayout* titleBarLayout = nullptr;
    QWK::WidgetWindowAgent* windowAgent = nullptr;

    // 关闭按钮的 Normal/Hover 图标，eventFilter 中切换
    QIcon m_closeIconNormal;
    QIcon m_closeIconHover;
    // 启用特效的按键序列
    static const inline QVector<int> m_keySequence = {Qt::Key_Up,   Qt::Key_Up,   Qt::Key_Down,  Qt::Key_Down,
                                                      Qt::Key_Left, Qt::Key_Left, Qt::Key_Right, Qt::Key_Right};
    // 按键序列当前匹配到的位置
    int m_keyIndex = 0;
    // 是否已启用特效标识
    bool m_effectsEnabled = false;
    // 是否已启用背景透明标识
    bool m_translucentBackground = false;
    // 文字淡出后是否已为外层布局补过弹簧
    bool m_stretchAdded = false;
    // 系统版本：按发布顺序排列，用 >= 比较表达"某某及以上"
    // 非 Windows、Win7 之前与 Win8/8.1 都归入 Unadapted，沿用 Qt 默认对话框
    enum class WindowsVersion
    {
        Unadapted,
        Win7,
        Win10,
        Win11,
        Win11_22H2,
    };
    WindowsVersion m_winVersion = WindowsVersion::Unadapted;
    // 方向键弹跳动画：连按时以当前位置为起点重新起跳，落点固定为一段弹跳结束时的位置
    QPropertyAnimation* m_bounceAnimation = nullptr;
    QPoint m_bounceHome;
};
