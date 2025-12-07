#include "sum_export_dialog.h"
#include "ui_sum_export_dialog.h"

SumExportDialog::SumExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ExportSumDialog)
{
    ui->setupUi(this);
    connect(ui->beginTimeComboBox,&QComboBox::currentIndexChanged,this,&SumExportDialog::updateRecordList);
    connect(ui->endTimeComboBox,&QComboBox::currentIndexChanged,this,&SumExportDialog::updateRecordList);
}

SumExportDialog::~SumExportDialog()
{
    delete ui;
}

void SumExportDialog::setRecordText(QStringList records)
{
    recordList=records;
    ui->beginTimeComboBox->addItems(recordList);
    ui->endTimeComboBox->addItems(recordList);
}

void SumExportDialog::setSortingConfig(SortSetting settings)
{
    sortConfig=settings;
}

int SumExportDialog::getBeginIndex()
{
    return ui->beginTimeComboBox->currentIndex();
}

int SumExportDialog::getEndIndex()
{
    return ui->endTimeComboBox->currentIndex();
}

quint8 SumExportDialog::getCurrentConfig()
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

void SumExportDialog::updateRecordList(int unused_index)
{
    Q_UNUSED(unused_index);
    ui->recordListWidget->clear();
    for(int i=getBeginIndex();i<=getEndIndex();i++)
        new QListWidgetItem(recordList.at(i),ui->recordListWidget);
}

void SumExportDialog::setConfig(quint8 mode)
{
    ui->box_HideStudentNumber->setChecked(mode&SortSetting::hideStudentNumber);
    ui->box_SortByStudentSum->setChecked((mode&SortSetting::sortByStudentSum));
    ui->box_FlipStudentSortOrder->setChecked(mode&SortSetting::flipStudentSorting);
    ui->box_SortAsGroup->setChecked(mode&SortSetting::sortAsGroup);
    ui->box_HideGroupPreview->setChecked(mode&SortSetting::hideGroupPreview);
    ui->box_SortByGroupSum->setChecked(mode&SortSetting::sortByGroupSum);
    ui->box_FlipGroupSortOrder->setChecked(mode&SortSetting::flipGroupSorting);
}


void SumExportDialog::on_studentDefaultBtn_clicked()
{
    setConfig(sortConfig.studentDefault);
}


void SumExportDialog::on_groupDefaultBtn_clicked()
{
    setConfig(sortConfig.groupDefault);
}


void SumExportDialog::on_studentPreviousBtn_clicked()
{
    setConfig(sortConfig.studentPrevious);
}


void SumExportDialog::on_groupPreviousBtn_clicked()
{
    setConfig(sortConfig.groupPrevious);
}

