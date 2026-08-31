#include "StudentTableSidebar.h"
#include "exexpander.h"
#include "fluentui3styleproperties.h"
#include <QCheckBox>
#include <QVariant>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

StudentTableSidebar::StudentTableSidebar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(230);

    // 侧栏根布局：滚动区域承载全部折叠块，避免全部展开时挤压表格
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rootLayout->addWidget(scrollArea);

    auto *contentWidget = new QWidget;
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);
    scrollArea->setWidget(contentWidget);

    // 文件信息折叠块：显示文件名与三个数量，默认展开
    auto *fileInfoExpander = new ExExpander(contentWidget);
    fileInfoExpander->setHeader(tr("文件信息"));
    fileInfoExpander->setExpandDirection(ExExpander::Down);

    auto *fileInfoBlock = new QWidget;
    auto *fileInfoLayout = new QVBoxLayout(fileInfoBlock);
    fileInfoLayout->setContentsMargins(0, 0, 0, 0);
    fileInfoLayout->setSpacing(6);

    fileNameLabel = new QLabel(fileInfoBlock);
    fileNameLabel->setWordWrap(true);
    fileInfoLayout->addWidget(fileNameLabel);

    studentCountLabel = new QLabel(fileInfoBlock);
    fileInfoLayout->addWidget(studentCountLabel);

    recordCountLabel = new QLabel(fileInfoBlock);
    fileInfoLayout->addWidget(recordCountLabel);

    templateCountLabel = new QLabel(fileInfoBlock);
    fileInfoLayout->addWidget(templateCountLabel);

    fileInfoExpander->addContentWidget(fileInfoBlock);
    fileInfoExpander->setExpanded(true);
    contentLayout->addWidget(fileInfoExpander);

    // 表格开关折叠块：拖动排序、姓名编辑、小组编辑，默认收起
    auto *tableOptionsExpander = new ExExpander(contentWidget);
    tableOptionsExpander->setHeader(tr("表格选项"));
    tableOptionsExpander->setExpandDirection(ExExpander::Down);

    auto *tableOptionsBlock = new QWidget;
    auto *tableOptionsLayout = new QVBoxLayout(tableOptionsBlock);
    tableOptionsLayout->setContentsMargins(0, 0, 0, 0);
    tableOptionsLayout->setSpacing(6);

    auto *dragStudentSwitch = new QCheckBox(tr("拖动排序学生"), tableOptionsBlock);
    dragStudentSwitch->setCheckState(Qt::Checked);
    dragStudentSwitch->setProperty(SwitchStyleProperty, QVariant(true));
    tableOptionsLayout->addWidget(dragStudentSwitch);

    auto *editNameSwitch = new QCheckBox(tr("学生姓名编辑"), tableOptionsBlock);
    editNameSwitch->setCheckState(Qt::Checked);
    editNameSwitch->setProperty(SwitchStyleProperty, QVariant(true));
    tableOptionsLayout->addWidget(editNameSwitch);

    auto *editGroupSwitch = new QCheckBox(tr("小组信息编辑"), tableOptionsBlock);
    editGroupSwitch->setCheckState(Qt::Checked);
    editGroupSwitch->setProperty(SwitchStyleProperty, QVariant(true));
    tableOptionsLayout->addWidget(editGroupSwitch);

    tableOptionsExpander->addContentWidget(tableOptionsBlock);
    contentLayout->addWidget(tableOptionsExpander);

    // 学生操作折叠块：添加学生与删除学生，默认展开
    auto *studentActionsExpander = new ExExpander(contentWidget);
    studentActionsExpander->setHeader(tr("学生操作"));
    studentActionsExpander->setExpandDirection(ExExpander::Down);

    auto *studentActionsBlock = new QWidget;
    auto *studentActionsLayout = new QHBoxLayout(studentActionsBlock);
    studentActionsLayout->setContentsMargins(0, 0, 0, 0);
    studentActionsLayout->setSpacing(6);
    addStudentButton = new QPushButton(tr("添加学生"), studentActionsBlock);
    deleteStudentButton = new QPushButton(tr("删除学生"), studentActionsBlock);
    studentActionsLayout->addWidget(addStudentButton);
    studentActionsLayout->addWidget(deleteStudentButton);

    studentActionsExpander->addContentWidget(studentActionsBlock);
    studentActionsExpander->setExpanded(true);
    contentLayout->addWidget(studentActionsExpander);

    // 积分记录折叠块：当前查看的记录与上一条/下一条/关闭，默认展开
    recordExpander = new ExExpander(contentWidget);
    recordExpander->setHeader(tr("积分记录"));
    recordExpander->setExpandDirection(ExExpander::Down);

    auto *recordBlock = new QWidget;
    auto *recordLayout = new QVBoxLayout(recordBlock);
    recordLayout->setContentsMargins(0, 0, 0, 0);
    recordLayout->setSpacing(6);

    recordPreviewLabel = new QLabel(recordBlock);
    recordPreviewLabel->setWordWrap(true);
    recordLayout->addWidget(recordPreviewLabel);

    auto *recordNavLayout = new QHBoxLayout;
    recordNavLayout->setContentsMargins(0, 0, 0, 0);
    recordNavLayout->setSpacing(6);
    recordPreviousButton = new QPushButton(tr("前"), recordBlock);
    recordPositionLabel = new QLabel(recordBlock);
    recordPositionLabel->setAlignment(Qt::AlignCenter);
    recordNextButton = new QPushButton(tr("后"), recordBlock);
    recordCloseButton = new QPushButton(tr("关闭预览"), recordBlock);
    recordNavLayout->addWidget(recordPreviousButton);
    recordNavLayout->addWidget(recordPositionLabel);
    recordNavLayout->addWidget(recordNextButton);
    recordNavLayout->addWidget(recordCloseButton);
    recordNavLayout->addStretch();
    recordLayout->addLayout(recordNavLayout);

    auto *recordActionLayout = new QHBoxLayout;
    recordActionLayout->setContentsMargins(0, 0, 0, 0);
    recordActionLayout->setSpacing(6);
    newRecordButton = new QPushButton(tr("新建积分记录"), recordBlock);
    applyTemplateButton = new QPushButton(tr("应用积分模板"), recordBlock);
    recordActionLayout->addWidget(newRecordButton);
    recordActionLayout->addWidget(applyTemplateButton);
    recordLayout->addLayout(recordActionLayout);

    auto *recordNameLayout = new QHBoxLayout;
    recordNameLayout->setContentsMargins(0, 0, 0, 0);
    recordNameLayout->setSpacing(6);
    recordNameEdit = new QLineEdit(recordBlock);
    recordNameEdit->setPlaceholderText(tr("填写积分记录名称"));
    saveRecordButton = new QPushButton(tr("保存积分记录"), recordBlock);
    recordNameLayout->addWidget(recordNameEdit, 1);
    recordNameLayout->addWidget(saveRecordButton);
    recordLayout->addLayout(recordNameLayout);

    recordExpander->addContentWidget(recordBlock);
    recordExpander->setExpanded(true);
    contentLayout->addWidget(recordExpander);

    // 积分模板折叠块：当前查看的模板、导航、名称编辑与更新/放弃，默认收起
    templateExpander = new ExExpander(contentWidget);
    templateExpander->setHeader(tr("积分模板"));
    templateExpander->setExpandDirection(ExExpander::Down);

    auto *templateBlock = new QWidget;
    auto *templateLayout = new QVBoxLayout(templateBlock);
    templateLayout->setContentsMargins(0, 0, 0, 0);
    templateLayout->setSpacing(6);

    templatePreviewLabel = new QLabel(templateBlock);
    templatePreviewLabel->setWordWrap(true);
    templateLayout->addWidget(templatePreviewLabel);

    auto *templateNavLayout = new QHBoxLayout;
    templateNavLayout->setContentsMargins(0, 0, 0, 0);
    templateNavLayout->setSpacing(6);
    templatePreviousButton = new QPushButton(tr("前"), templateBlock);
    templatePositionLabel = new QLabel(templateBlock);
    templatePositionLabel->setAlignment(Qt::AlignCenter);
    templateNextButton = new QPushButton(tr("后"), templateBlock);
    templateCloseButton = new QPushButton(tr("关闭预览"), templateBlock);
    templateNavLayout->addWidget(templatePreviousButton);
    templateNavLayout->addWidget(templatePositionLabel);
    templateNavLayout->addWidget(templateNextButton);
    templateNavLayout->addWidget(templateCloseButton);
    templateNavLayout->addStretch();
    templateLayout->addLayout(templateNavLayout);

    newTemplateButton = new QPushButton(tr("新建积分模板"), templateBlock);
    templateLayout->addWidget(newTemplateButton);

    templateNameEdit = new QLineEdit(templateBlock);
    templateNameEdit->setPlaceholderText(tr("输入新的积分模板名称"));
    templateLayout->addWidget(templateNameEdit);

    auto *templateNameButtonLayout = new QHBoxLayout;
    templateNameButtonLayout->setContentsMargins(0, 0, 0, 0);
    templateNameButtonLayout->setSpacing(6);
    templateUpdateButton = new QPushButton(tr("更新名称"), templateBlock);
    templateDiscardButton = new QPushButton(tr("放弃修改"), templateBlock);
    templateNameButtonLayout->addWidget(templateUpdateButton);
    templateNameButtonLayout->addWidget(templateDiscardButton);
    templateLayout->addLayout(templateNameButtonLayout);

    templateExpander->addContentWidget(templateBlock);
    contentLayout->addWidget(templateExpander);

    contentLayout->addStretch();

    connect(dragStudentSwitch, &QCheckBox::stateChanged, this, [this](int state) {
        emit dragSortChanged(state == Qt::Checked);
    });
    connect(editNameSwitch, &QCheckBox::stateChanged, this, [this](int state) {
        emit nameEditChanged(state == Qt::Checked);
    });
    connect(editGroupSwitch, &QCheckBox::stateChanged, this, [this](int state) {
        emit groupEditChanged(state == Qt::Checked);
    });

    connect(addStudentButton, &QPushButton::clicked,
            this, &StudentTableSidebar::addStudentsRequested);
    connect(deleteStudentButton, &QPushButton::clicked,
            this, &StudentTableSidebar::deleteStudentRequested);

    connect(recordPreviousButton, &QPushButton::clicked,
            this, &StudentTableSidebar::recordPreviousRequested);
    connect(recordNextButton, &QPushButton::clicked,
            this, &StudentTableSidebar::recordNextRequested);
    connect(recordCloseButton, &QPushButton::clicked,
            this, &StudentTableSidebar::recordPreviewCloseRequested);
    connect(newRecordButton, &QPushButton::clicked,
            this, &StudentTableSidebar::newScoreRecordRequested);
    connect(saveRecordButton, &QPushButton::clicked,
            this, &StudentTableSidebar::saveScoreRecordRequested);
    connect(applyTemplateButton, &QPushButton::clicked,
            this, &StudentTableSidebar::applyTemplateRequested);

    connect(templatePreviousButton, &QPushButton::clicked,
            this, &StudentTableSidebar::templatePreviousRequested);
    connect(templateNextButton, &QPushButton::clicked,
            this, &StudentTableSidebar::templateNextRequested);
    connect(templateCloseButton, &QPushButton::clicked,
            this, &StudentTableSidebar::templatePreviewCloseRequested);
    connect(newTemplateButton, &QPushButton::clicked,
            this, &StudentTableSidebar::newTemplateRequested);
    connect(templateNameEdit, &QLineEdit::returnPressed, this, [this] {
        emit templateNameUpdateRequested(templateNameEdit->text());
    });
    connect(templateUpdateButton, &QPushButton::clicked, this, [this] {
        emit templateNameUpdateRequested(templateNameEdit->text());
    });
    connect(templateDiscardButton, &QPushButton::clicked,
            this, &StudentTableSidebar::templateNameEditCancelled);
}

void StudentTableSidebar::setFileInfo(const QString &fileName, bool dirty, int studentCount,
                                      int recordCount, int templateCount)
{
    fileNameLabel->setText(tr("文件名：%1%2").arg(fileName).arg(dirty ? tr(" *") : QString()));
    studentCountLabel->setText(tr("学生数量：%1").arg(studentCount));
    recordCountLabel->setText(tr("积分记录：%1").arg(recordCount));
    templateCountLabel->setText(tr("积分模板：%1").arg(templateCount));
}

void StudentTableSidebar::setRecordPreview(const QString &displayText,
                                           const QString &positionText, bool hasPreview,
                                           bool canPrevious, bool canNext)
{
    recordPreviewLabel->setText(hasPreview ? tr("当前记录：%1").arg(displayText)
                                           : tr("当前记录：未查看"));
    recordPositionLabel->setText(hasPreview ? positionText : tr("-/-"));
    recordPreviousButton->setEnabled(hasPreview && canPrevious);
    recordNextButton->setEnabled(hasPreview && canNext);
    recordCloseButton->setEnabled(hasPreview);
    if(hasPreview && !recordExpander->isExpanded())
        recordExpander->setExpanded(true);
}

void StudentTableSidebar::setTemplatePreview(const QString &displayText,
                                             const QString &positionText, bool hasPreview,
                                             bool canPrevious, bool canNext)
{
    templatePreviewLabel->setText(hasPreview ? tr("当前模板：%1").arg(displayText)
                                             : tr("当前模板：未查看"));
    templatePositionLabel->setText(hasPreview ? positionText : tr("-/-"));
    templatePreviousButton->setEnabled(hasPreview && canPrevious);
    templateNextButton->setEnabled(hasPreview && canNext);
    templateCloseButton->setEnabled(hasPreview);
    templateNameEdit->setEnabled(hasPreview);
    templateUpdateButton->setEnabled(hasPreview);
    templateDiscardButton->setEnabled(hasPreview);
    if(hasPreview)
    {
        if(!templateExpander->isExpanded())
            templateExpander->setExpanded(true);
    }
    else
    {
        templateNameEdit->clear();
    }
}

void StudentTableSidebar::setTemplateNameDraft(const QString &name)
{
    templateNameEdit->setText(name);
}

void StudentTableSidebar::setRecordEditingInProgress(bool isEditing)
{
    newRecordButton->setEnabled(!isEditing);
    saveRecordButton->setEnabled(isEditing);
    applyTemplateButton->setEnabled(isEditing);
}

void StudentTableSidebar::setStudentActionsEnabled(bool enabled)
{
    addStudentButton->setEnabled(enabled);
    deleteStudentButton->setEnabled(enabled);
}

QString StudentTableSidebar::recordNameText() const
{
    return recordNameEdit->text();
}

void StudentTableSidebar::setRecordNameText(const QString &text)
{
    recordNameEdit->setText(text);
}

void StudentTableSidebar::focusRecordNameEdit()
{
    recordNameEdit->setFocus();
}

void StudentTableSidebar::focusTemplateNameEdit()
{
    templateNameEdit->setFocus();
    templateNameEdit->selectAll();
}
