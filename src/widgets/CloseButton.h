#pragma once

#include <QIcon>
#include <QPushButton>

class QEvent;

// 通用关闭按钮：透明底、无边框，悬停时由浅灰图标切换为深色图标
class CloseButton : public QPushButton
{
public:
    explicit CloseButton(QWidget* parent = nullptr);

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    QIcon normalIcon;
    QIcon hoverIcon;
};
