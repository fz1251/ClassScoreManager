#include "paste_students_dialog.h"
#include "ui_paste_students_dialog.h"

PasteStudentsDialog::PasteStudentsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasteStudentsDialog)
{
    ui->setupUi(this);
    ui->tableTextEdit->setFocus();
    ui->editListButton->setDisabled(true);
}

PasteStudentsDialog::~PasteStudentsDialog()
{
    delete ui;
}

QStringList PasteStudentsDialog::getNameList()
{
    QStringList nameList;
    for(int i=0;i<ui->parseResultList->count();i++)
        nameList.append(ui->parseResultList->item(i)->text());
    return nameList;
}

void PasteStudentsDialog::on_parseTextButton_clicked(bool checked)
{
    if(checked)
    {
        ui->parseTextButton->setText(tr("修改"));
        QString inputString=ui->tableTextEdit->toPlainText();
        QStringList rawList=inputString.split('\n',Qt::SkipEmptyParts);
        for(const QString& rawStr:rawList)
        {
            QString studentName=rawStr.simplified();
            if(!studentName.isEmpty())
                new QListWidgetItem(studentName,ui->parseResultList);
        }
        ui->editListButton->setDisabled(false);
        ui->parseStatusLable->setText(tr("解析得到%1名学生").arg(ui->parseResultList->count()));
        ui->tableTextEdit->setDisabled(true);
    }
    else
    {
        ui->parseTextButton->setText(tr("重新解析"));
        ui->parseResultList->clear();
        ui->editListButton->setChecked(false);
        ui->editListButton->setDisabled(true);
        ui->editListButton->setText(tr("手动编辑列表"));
        ui->parseStatusLable->setText(tr("重新解析以查看结果"));
        ui->tableTextEdit->setDisabled(false);
    }
}


void PasteStudentsDialog::on_editListButton_clicked(bool checked)
{
    if(checked)
    {
        ui->editListButton->setText(tr("关闭编辑"));
        for(int i=0;i<ui->parseResultList->count();i++)
                ui->parseResultList->item(i)->setFlags(
                    Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled);
    }
    else
    {
        ui->editListButton->setText(tr("开启编辑"));
        for(int i=0;i<ui->parseResultList->count();i++)
                ui->parseResultList->item(i)->setFlags(
                    Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    }
}

