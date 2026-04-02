#include "ItemSelectionDialog.h"
#include "ui_ItemSelectionDialog.h"

ItemSelectionDialog::ItemSelectionDialog(QWidget *parent, const QString &title, const QString &label, const QStringList &items, int current) :
    QDialog(parent),
    ui(new Ui::ItemSelectionDialog)
{
    ui->setupUi(this);
    this->setWindowTitle(title);
    ui->label->setText(label);
    ui->comboBox->addItems(items);
    if(current>=0&&current<ui->comboBox->count())
        ui->comboBox->setCurrentIndex(current);
}

ItemSelectionDialog::~ItemSelectionDialog()
{
    delete ui;
}

int ItemSelectionDialog::selectedIndex()
{
    return ui->comboBox->currentIndex();
}
