#include "exportsumdialog.h"
#include "ui_exportsumdialog.h"

ExportSumDialog::ExportSumDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportSumDialog)
{
    ui->setupUi(this);
    connect(ui->beginTimeComboBox,&QComboBox::currentIndexChanged,this,&ExportSumDialog::updateRecordList);
    connect(ui->endTimeComboBox,&QComboBox::currentIndexChanged,this,&ExportSumDialog::updateRecordList);
}

ExportSumDialog::~ExportSumDialog()
{
    delete ui;
}

void ExportSumDialog::setRecordText(QStringList records)
{
    recordList=records;
    ui->beginTimeComboBox->addItems(recordList);
    ui->endTimeComboBox->addItems(recordList);
}

void ExportSumDialog::setSortingConfig(SortSetting settings)
{
    sortConfig=settings;
}

int ExportSumDialog::getBeginIndex()
{
    return ui->beginTimeComboBox->currentIndex();
}

int ExportSumDialog::getEndIndex()
{
    return ui->endTimeComboBox->currentIndex();
}

quint8 ExportSumDialog::getCurrentConfig()
{
    quint8 mode=0;
    if(ui->box_HideStudentNumber->isChecked())
        mode|=SortSetting::hideStudentNumber;
    if(ui->box_SortByStudentSum->isChecked())
        mode|=SortSetting::sortByStudentSum;
    if(ui->box_FlipStudentSortOrder->isChecked())
        mode|=SortSetting::flipStudentSorting;
    if(ui->box_SortAsGroup->isChecked())
        mode|=SortSetting::sortAsGroup;
    if(ui->box_HideGroupPreview->isChecked())
        mode|=SortSetting::hideGroupPreview;
    if(ui->box_SortByGroupSum->isChecked())
        mode|=SortSetting::sortByGroupSum;
    if(ui->box_FlipGroupSortOrder->isChecked())
        mode|=SortSetting::flipGroupSorting;
    return mode;
}

void ExportSumDialog::updateRecordList(int unused_index)
{
    Q_UNUSED(unused_index);
    ui->recordListWidget->clear();
    for(int i=getBeginIndex();i<=getEndIndex();i++)
        new QListWidgetItem(recordList.at(i),ui->recordListWidget);
}

void ExportSumDialog::setConfig(quint8 mode)
{
    ui->box_HideStudentNumber->setChecked(mode&SortSetting::hideStudentNumber);
    ui->box_SortByStudentSum->setChecked((mode&SortSetting::sortByStudentSum));
    ui->box_FlipStudentSortOrder->setChecked(mode&SortSetting::flipStudentSorting);
    ui->box_SortAsGroup->setChecked(mode&SortSetting::sortAsGroup);
    ui->box_HideGroupPreview->setChecked(mode&SortSetting::hideGroupPreview);
    ui->box_SortByGroupSum->setChecked(mode&SortSetting::sortByGroupSum);
    ui->box_FlipGroupSortOrder->setChecked(mode&SortSetting::flipGroupSorting);
}


void ExportSumDialog::on_studentDefaultBtn_clicked()
{
    setConfig(sortConfig.studentDefault);
}


void ExportSumDialog::on_groupDefaultBtn_clicked()
{
    setConfig(sortConfig.groupDefault);
}


void ExportSumDialog::on_studentPreviousBtn_clicked()
{
    setConfig(sortConfig.studentPrevious);
}


void ExportSumDialog::on_groupPreviousBtn_clicked()
{
    setConfig(sortConfig.groupPrevious);
}

