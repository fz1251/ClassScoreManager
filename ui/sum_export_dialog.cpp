#include "sum_export_dialog.h"
#include "ui_sum_export_dialog.h"
#include <QButtonGroup>

SumExportDialog::SumExportDialog(QWidget *parent, SortSettings settings) :
    QDialog(parent),
    ui(new Ui::ExportSumDialog)
{
    ui->setupUi(this);

    connect(ui->beginTimeComboBox,&QComboBox::currentIndexChanged,this,&SumExportDialog::updateRecordList);
    connect(ui->endTimeComboBox,&QComboBox::currentIndexChanged,this,&SumExportDialog::updateRecordList);
    connect(ui->btn_groupMode,&QPushButton::toggled,this,&SumExportDialog::setGroupModeVisibility);

    auto addToButtonGroup = [this](QPushButton* btn1, QPushButton* btn2)
    {
        QButtonGroup* group = new QButtonGroup(this);
        group->addButton(btn1);
        group->addButton(btn2);
    };
    // 设置每组的QButtonGroup
    addToButtonGroup(ui->btn_studentMode,ui->btn_groupMode);
    addToButtonGroup(ui->btn_studentNo,ui->btn_studentScore);
    addToButtonGroup(ui->btn_studentAsc,ui->btn_studentDesc);
    addToButtonGroup(ui->btn_groupNo,ui->btn_groupScore);
    addToButtonGroup(ui->btn_groupAsc,ui->btn_groupDesc);

    // 根据传入的SortSettings设置控件的选中状态
    ui->box_showStudentNumber->setChecked(settings.showStudentNumberFlag);
    if(settings.useGroupModeFlag)
    {
        // 标记使用小组模式
        ui->btn_groupMode->setChecked(true);
        // 设置学生排序依据
        if(settings.groupStuSpec.field==SortField::StudentNumber)
            ui->btn_studentNo->setChecked(true);
        else if(settings.groupStuSpec.field==SortField::StudentSum)
            ui->btn_studentScore->setChecked(true);
        // 设置学生排序顺序
        if(settings.groupStuSpec.order==SortOrder::Ascending)
            ui->btn_studentAsc->setChecked(true);
        else if(settings.groupStuSpec.order==SortOrder::Descending)
            ui->btn_studentDesc->setChecked(true);
        // 设置小组排序依据
        if(settings.groupSpec.field==SortField::GroupNumber)
            ui->btn_groupNo->setChecked(true);
        else if(settings.groupSpec.field==SortField::GroupSum)
            ui->btn_groupScore->setChecked(true);
        // 设置小组排序顺序
        if(settings.groupSpec.order==SortOrder::Ascending)
            ui->btn_groupAsc->setChecked(true);
        else if(settings.groupSpec.order==SortOrder::Descending)
            ui->btn_groupDesc->setChecked(true);
    }
    else
    {
        // 标记不使用小组模式，而使用学生模式
        ui->btn_studentMode->setChecked(true);
        setGroupModeVisibility(false);
        // 设置学生排序依据
        if(settings.studentSpec.field==SortField::StudentNumber)
            ui->btn_studentNo->setChecked(true);
        else if(settings.studentSpec.field==SortField::StudentSum)
            ui->btn_studentScore->setChecked(true);
        // 设置学生排序顺序
        if(settings.studentSpec.order==SortOrder::Ascending)
            ui->btn_studentAsc->setChecked(true);
        else if(settings.studentSpec.order==SortOrder::Descending)
            ui->btn_studentDesc->setChecked(true);
        // 仍要进行小组相关的设置，防止互斥按钮均未被选中
        // 设置小组排序依据
        if(settings.groupSpec.field==SortField::GroupNumber)
            ui->btn_groupNo->setChecked(true);
        else if(settings.groupSpec.field==SortField::GroupSum)
            ui->btn_groupScore->setChecked(true);
        // 设置小组排序顺序
        if(settings.groupSpec.order==SortOrder::Ascending)
            ui->btn_groupAsc->setChecked(true);
        else if(settings.groupSpec.order==SortOrder::Descending)
            ui->btn_groupDesc->setChecked(true);
    }
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

int SumExportDialog::getBeginIndex()
{
    return ui->beginTimeComboBox->currentIndex();
}

int SumExportDialog::getEndIndex()
{
    return ui->endTimeComboBox->currentIndex();
}

SortSettings SumExportDialog::getCurrentConfig()
{
    SortSettings settings;
    // 设置结果是否显示学生编号
    settings.showStudentNumberFlag=ui->box_showStudentNumber->isChecked();
    if(ui->btn_groupMode->isChecked())
    {
        // 标记使用小组模式
        settings.useGroupModeFlag=true;
        // 设置学生排序依据
        if(ui->btn_studentNo->isChecked())
            settings.groupStuSpec.field=SortField::StudentNumber;
        else if(ui->btn_studentScore->isChecked())
            settings.groupStuSpec.field=SortField::StudentSum;
        // 设置学生排序顺序
        if(ui->btn_studentAsc->isChecked())
            settings.groupStuSpec.order=SortOrder::Ascending;
        else if(ui->btn_studentDesc->isChecked())
            settings.groupStuSpec.order=SortOrder::Descending;
        // 设置小组排序依据
        if(ui->btn_groupNo->isChecked())
            settings.groupSpec.field=SortField::GroupNumber;
        else if(ui->btn_groupScore->isChecked())
            settings.groupSpec.field=SortField::GroupSum;
        // 设置小组排序顺序
        if(ui->btn_groupAsc->isChecked())
            settings.groupSpec.order=SortOrder::Ascending;
        else if(ui->btn_groupDesc->isChecked())
            settings.groupSpec.order=SortOrder::Descending;
    }
    else
    {
        // 标记不使用小组模式，而使用学生模式
        settings.useGroupModeFlag=false;
        // 设置学生排序依据
        if(ui->btn_studentNo->isChecked())
            settings.studentSpec.field=SortField::StudentNumber;
        else if(ui->btn_studentScore->isChecked())
        // 设置学生排序顺序
            settings.studentSpec.field=SortField::StudentSum;
        if(ui->btn_studentAsc->isChecked())
            settings.studentSpec.order=SortOrder::Ascending;
        else if(ui->btn_studentDesc->isChecked())
            settings.studentSpec.order=SortOrder::Descending;
    }
    return settings;
}

void SumExportDialog::updateRecordList(int index_)
{
    Q_UNUSED(index_);
    ui->recordListWidget->clear();
    for(int i=getBeginIndex();i<=getEndIndex();i++)
        new QListWidgetItem(recordList.at(i),ui->recordListWidget);
}

void SumExportDialog::setGroupModeVisibility(bool f)
{
    ui->label_3->setVisible(f);
    ui->label_4->setVisible(f);
    ui->btn_groupNo->setVisible(f);
    ui->btn_groupScore->setVisible(f);
    ui->btn_groupAsc->setVisible(f);
    ui->btn_groupDesc->setVisible(f);
}