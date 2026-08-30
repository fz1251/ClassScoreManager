#include "ItemSelectionDialog.h"
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

ItemSelectionDialog::ItemSelectionDialog(QWidget *parent, const QString &title, const QString &label,
                                         const QStringList &items, int current) :
    QDialog(parent)
{
    resize(215, 98);
    setWindowTitle(title);

    // 对话框根布局：提示文字、下拉框、确认按钮从上到下排列
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(2);

    // 提示文字
    auto *promptLabel = new QLabel(label, this);
    mainLayout->addWidget(promptLabel);

    // 选择下拉框
    selectionComboBox = new QComboBox(this);
    selectionComboBox->addItems(items);
    if(current >= 0 && current < selectionComboBox->count())
        selectionComboBox->setCurrentIndex(current);
    mainLayout->addWidget(selectionComboBox);

    // 确定与取消按钮
    auto *buttonBox = new QDialogButtonBox(this);
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

int ItemSelectionDialog::selectedIndex() const
{
    return selectionComboBox->currentIndex();
}
