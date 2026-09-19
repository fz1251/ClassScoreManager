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
    void enableBackgroundBlurEffect();
    void windowBounce(int dx, int dy, int duration_ms = 350);

    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QVBoxLayout* outerLayout = nullptr;
    QLabel* titleLabel = nullptr;
    QLabel* subtitleLabel = nullptr;
    LineSpacedLabel* contentLabel = nullptr;
    QHBoxLayout* titleBarLayout = nullptr;

    // 关闭按钮的 Normal/Hover 图标，eventFilter 中切换
    QIcon m_closeIconNormal;
    QIcon m_closeIconHover;
    // 启用特效的按键序列
    static const inline QVector<int> m_keySequence = {Qt::Key_Up,   Qt::Key_Up,   Qt::Key_Down,  Qt::Key_Down,
                                                      Qt::Key_Left, Qt::Key_Left, Qt::Key_Right, Qt::Key_Right};
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
