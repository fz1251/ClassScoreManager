#include "aboutinfodialog.h"
#include "ui_aboutinfodialog.h"

AboutInfoDialog::AboutInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutInfoDialog)
{
    ui->setupUi(this);
}

AboutInfoDialog::~AboutInfoDialog()
{
    delete ui;
}
