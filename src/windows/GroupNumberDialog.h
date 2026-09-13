#pragma once

#include <QDialog>

class QSpinBox;

// 设置学生小组号的对话框：单个非负整数输入
class GroupNumberDialog : public QDialog
{
    Q_OBJECT

public:
    GroupNumberDialog(const QString& studentName, int currentGroup, QWidget *parent = nullptr);

    int getGroupNumber() const;

private:
    // 界面元素指针：构造函数中统一创建，供其他成员函数访问
    QSpinBox *groupSpinBox = nullptr;
};
