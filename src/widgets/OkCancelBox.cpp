#include "OkCancelBox.h"
#include <QHBoxLayout>
#include <QPushButton>

OkCancelBox::OkCancelBox(QWidget *parent)
    : QWidget(parent)
{
    // 按钮组布局：确定与取消靠右排列
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch();

    auto *confirmButton = new QPushButton(tr("确定"), this);
    confirmButton->setDefault(true);
    layout->addWidget(confirmButton);

    auto *cancelButton = new QPushButton(tr("取消"), this);
    layout->addWidget(cancelButton);

    connect(confirmButton, &QPushButton::clicked, this, &OkCancelBox::confirmed);
    connect(cancelButton, &QPushButton::clicked, this, &OkCancelBox::cancelled);
}
