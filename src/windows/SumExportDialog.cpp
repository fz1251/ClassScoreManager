#include "SumExportDialog.h"

#include "../widgets/OkCancelBox.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SumExportDialog::SumExportDialog(QWidget* parent, SortSettings settings)
    : QDialog(parent)
{
    setWindowTitle(tr("积分求和统计导出"));

    // 对话框根布局：只保留排序配置，不再选择记录范围
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(10);

    // 排序模式行：个人/小组互斥
    auto* modeRow = new QHBoxLayout;
    modeRow->setSpacing(6);
    modeRow->addWidget(new QLabel(tr("排序模式："), this));
    studentModeButton = new QPushButton(tr("个人"), this);
    groupModeButton = new QPushButton(tr("小组"), this);
    studentModeButton->setCheckable(true);
    groupModeButton->setCheckable(true);
    modeRow->addWidget(studentModeButton);
    modeRow->addWidget(groupModeButton);
    modeRow->addStretch();
    rootLayout->addLayout(modeRow);

    showStudentNumberBox = new QCheckBox(tr("显示学生编号"), this);
    rootLayout->addWidget(showStudentNumberBox);

    // 排序行创建辅助：标签 + 两个互斥按钮
    auto createSortRow = [this](const QString& label, QPushButton*& firstButton, QPushButton*& secondButton)
    {
        auto* row = new QWidget(this);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(6);
        rowLayout->addWidget(new QLabel(label, row));
        firstButton = new QPushButton(row);
        secondButton = new QPushButton(row);
        firstButton->setCheckable(true);
        secondButton->setCheckable(true);
        rowLayout->addWidget(firstButton);
        rowLayout->addWidget(secondButton);
        rowLayout->addStretch();
        return row;
    };

    // 学生排序依据与顺序
    auto* studentFieldRow = createSortRow(tr("学生排序依据："), studentNumberButton, studentScoreButton);
    studentNumberButton->setText(tr("编号"));
    studentScoreButton->setText(tr("总分"));
    rootLayout->addWidget(studentFieldRow);

    auto* studentOrderRow = createSortRow(tr("学生排序顺序："), studentAscButton, studentDescButton);
    studentAscButton->setText(tr("升序"));
    studentDescButton->setText(tr("降序"));
    rootLayout->addWidget(studentOrderRow);

    // 小组排序依据与顺序：小组模式下才显示
    groupFieldRow = createSortRow(tr("小组排序依据："), groupNumberButton, groupScoreButton);
    groupNumberButton->setText(tr("编号"));
    groupScoreButton->setText(tr("总分"));
    rootLayout->addWidget(groupFieldRow);

    groupOrderRow = createSortRow(tr("小组排序顺序："), groupAscButton, groupDescButton);
    groupAscButton->setText(tr("升序"));
    groupDescButton->setText(tr("降序"));
    rootLayout->addWidget(groupOrderRow);

    // 确定与取消按钮
    auto* buttonBox = new OkCancelBox(this);
    connect(buttonBox, &OkCancelBox::confirmed, this, &QDialog::accept);
    connect(buttonBox, &OkCancelBox::cancelled, this, &QDialog::reject);
    rootLayout->addWidget(buttonBox);

    // 互斥按钮组
    auto makeExclusive = [this](QPushButton* firstButton, QPushButton* secondButton)
    {
        auto* group = new QButtonGroup(this);
        group->addButton(firstButton);
        group->addButton(secondButton);
    };
    makeExclusive(studentModeButton, groupModeButton);
    makeExclusive(studentNumberButton, studentScoreButton);
    makeExclusive(studentAscButton, studentDescButton);
    makeExclusive(groupNumberButton, groupScoreButton);
    makeExclusive(groupAscButton, groupDescButton);

    connect(groupModeButton, &QPushButton::toggled, this,
            [this](bool checked)
            {
                setGroupModeVisibility(checked);
            });

    // 按保存的配置恢复控件状态
    showStudentNumberBox->setChecked(settings.showStudentNumberFlag);
    if(settings.useGroupModeFlag)
    {
        groupModeButton->setChecked(true);
        if(settings.groupStuSpec.field == SortField::StudentNumber)
            studentNumberButton->setChecked(true);
        else if(settings.groupStuSpec.field == SortField::StudentSum)
            studentScoreButton->setChecked(true);
        if(settings.groupStuSpec.order == SortOrder::Ascending)
            studentAscButton->setChecked(true);
        else if(settings.groupStuSpec.order == SortOrder::Descending)
            studentDescButton->setChecked(true);
    }
    else
    {
        studentModeButton->setChecked(true);
        setGroupModeVisibility(false);
        if(settings.studentSpec.field == SortField::StudentNumber)
            studentNumberButton->setChecked(true);
        else if(settings.studentSpec.field == SortField::StudentSum)
            studentScoreButton->setChecked(true);
        if(settings.studentSpec.order == SortOrder::Ascending)
            studentAscButton->setChecked(true);
        else if(settings.studentSpec.order == SortOrder::Descending)
            studentDescButton->setChecked(true);
    }

    if(settings.groupSpec.field == SortField::GroupNumber)
        groupNumberButton->setChecked(true);
    else if(settings.groupSpec.field == SortField::GroupSum)
        groupScoreButton->setChecked(true);
    if(settings.groupSpec.order == SortOrder::Ascending)
        groupAscButton->setChecked(true);
    else if(settings.groupSpec.order == SortOrder::Descending)
        groupDescButton->setChecked(true);
}

SortSettings SumExportDialog::getCurrentConfig() const
{
    SortSettings settings;
    settings.showStudentNumberFlag = showStudentNumberBox->isChecked();
    if(groupModeButton->isChecked())
    {
        settings.useGroupModeFlag = true;
        if(studentNumberButton->isChecked())
            settings.groupStuSpec.field = SortField::StudentNumber;
        else if(studentScoreButton->isChecked())
            settings.groupStuSpec.field = SortField::StudentSum;
        if(studentAscButton->isChecked())
            settings.groupStuSpec.order = SortOrder::Ascending;
        else if(studentDescButton->isChecked())
            settings.groupStuSpec.order = SortOrder::Descending;
        if(groupNumberButton->isChecked())
            settings.groupSpec.field = SortField::GroupNumber;
        else if(groupScoreButton->isChecked())
            settings.groupSpec.field = SortField::GroupSum;
        if(groupAscButton->isChecked())
            settings.groupSpec.order = SortOrder::Ascending;
        else if(groupDescButton->isChecked())
            settings.groupSpec.order = SortOrder::Descending;
    }
    else
    {
        settings.useGroupModeFlag = false;
        if(studentNumberButton->isChecked())
            settings.studentSpec.field = SortField::StudentNumber;
        else if(studentScoreButton->isChecked())
            settings.studentSpec.field = SortField::StudentSum;
        if(studentAscButton->isChecked())
            settings.studentSpec.order = SortOrder::Ascending;
        else if(studentDescButton->isChecked())
            settings.studentSpec.order = SortOrder::Descending;
    }
    return settings;
}

void SumExportDialog::setGroupModeVisibility(bool visible)
{
    groupFieldRow->setVisible(visible);
    groupOrderRow->setVisible(visible);
}
