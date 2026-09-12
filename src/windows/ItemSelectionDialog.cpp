#include "ItemSelectionDialog.h"
#include "excontentdialog.h"
#include <QComboBox>

int ItemSelectionDialog::selectIndex(QWidget *parent, const QString &title, const QStringList &items,
                                     int current)
{
    auto *selectionComboBox = new QComboBox;
    selectionComboBox->addItems(items);
    if(current >= 0 && current < selectionComboBox->count())
        selectionComboBox->setCurrentIndex(current);

    ExContentDialog dialog(parent);
    dialog.setTitle(title);
    dialog.setContentWidget(selectionComboBox);
    dialog.setPrimaryButtonText(tr("确定"));
    dialog.setCloseButtonText(tr("取消"));
    dialog.setDefaultButton(ExContentDialog::PrimaryBtn);

    if(dialog.showDialog() != ExContentDialog::Primary)
        return -1;
    return selectionComboBox->currentIndex();
}
