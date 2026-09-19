#pragma once

#include <QWidget>

// 对话框底部的“确定 / 取消”按钮组：统一两个按钮的文案、顺序与间距
class OkCancelBox : public QWidget
{
    Q_OBJECT

public:
    explicit OkCancelBox(QWidget* parent = nullptr);

signals:
    void confirmed();
    void cancelled();
};
