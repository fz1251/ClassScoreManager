#pragma once

#include <QLabel>

class QEvent;
class QMouseEvent;
class QPaintEvent;

// 可点击标签：按当前宽度自动省略文本，未查看时整行作为灰底按钮提示
class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget* parent = nullptr);

    void setClickable(bool clickable);

signals:
    void clicked();

protected:
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    bool clickable = false;
    bool hovered = false;
};
