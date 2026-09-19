#include "GroupNumberDialog.h"

#include "../widgets/OkCancelBox.h"
#include "fluentui3styleproperties.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <limits>

GroupNumberDialog::GroupNumberDialog(const QString& studentName, int currentGroup, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("设置小组"));

    // 对话框根布局：提示文字、小组号输入与确定取消按钮
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(10);

    rootLayout->addWidget(new QLabel(tr("把「%1」分到第几组？").arg(studentName), this));

    // 小组号输入行：与表格中编辑小组号的委托保持同一套外观和取值规则
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);
    inputRow->addWidget(new QLabel(tr("小组号："), this));

    groupSpinBox = new QSpinBox(this);
    groupSpinBox->setAutoFillBackground(true);
    groupSpinBox->setFrame(false);
    groupSpinBox->setRange(0, std::numeric_limits<int>::max());
    groupSpinBox->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);
    groupSpinBox->setValue(currentGroup);
    inputRow->addWidget(groupSpinBox);
    inputRow->addStretch();
    rootLayout->addLayout(inputRow);

    auto* buttonBox = new OkCancelBox(this);
    connect(buttonBox, &OkCancelBox::confirmed, this, &QDialog::accept);
    connect(buttonBox, &OkCancelBox::cancelled, this, &QDialog::reject);
    rootLayout->addWidget(buttonBox);

    groupSpinBox->setFocus();
    groupSpinBox->selectAll();
}

int GroupNumberDialog::getGroupNumber() const
{
    return groupSpinBox->value();
}
