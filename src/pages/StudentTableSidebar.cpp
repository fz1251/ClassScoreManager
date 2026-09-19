#include "StudentTableSidebar.h"

#include "exexpander.h"
#include "exfonticon.h"
#include "fluentui3styleproperties.h"

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariant>

StudentTableSidebar::StudentTableSidebar(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(250);

    // 侧栏根布局：滚动区域承载全部折叠块，避免全部展开时挤压表格
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rootLayout->addWidget(scrollArea);

    auto* contentWidget = new QWidget;
    auto* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);
    scrollArea->setWidget(contentWidget);

    // 文件信息折叠块：显示文件名与三个数量，默认展开
    auto* fileInfoExpander = new ExExpander(contentWidget);
    fileInfoExpander->setHeader(tr("文件信息"));
    fileInfoExpander->setExpandDirection(ExExpander::Down);

    auto* fileInfoBlock = new QWidget;
    auto* fileInfoLayout = new QVBoxLayout(fileInfoBlock);
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
    auto* tableOptionsExpander = new ExExpander(contentWidget);
    tableOptionsExpander->setHeader(tr("表格选项"));
    tableOptionsExpander->setExpandDirection(ExExpander::Down);

    auto* tableOptionsBlock = new QWidget;
    auto* tableOptionsLayout = new QVBoxLayout(tableOptionsBlock);
    tableOptionsLayout->setContentsMargins(0, 0, 0, 0);
    tableOptionsLayout->setSpacing(6);

    auto* dragStudentSwitch = new QCheckBox(tr("拖动排序学生"), tableOptionsBlock);
    dragStudentSwitch->setCheckState(Qt::Checked);
    dragStudentSwitch->setProperty(SwitchStyleProperty, QVariant(true));
    tableOptionsLayout->addWidget(dragStudentSwitch);

    auto* editNameSwitch = new QCheckBox(tr("学生姓名编辑"), tableOptionsBlock);
    editNameSwitch->setCheckState(Qt::Checked);
    editNameSwitch->setProperty(SwitchStyleProperty, QVariant(true));
    tableOptionsLayout->addWidget(editNameSwitch);

    auto* editGroupSwitch = new QCheckBox(tr("小组信息编辑"), tableOptionsBlock);
    editGroupSwitch->setCheckState(Qt::Checked);
    editGroupSwitch->setProperty(SwitchStyleProperty, QVariant(true));
    tableOptionsLayout->addWidget(editGroupSwitch);

    tableOptionsExpander->addContentWidget(tableOptionsBlock);
    contentLayout->addWidget(tableOptionsExpander);

    // 学生操作折叠块：添加/删除学生按钮与按姓名搜索学生的结果列表，默认展开
    auto* studentActionsExpander = new ExExpander(contentWidget);
    studentActionsExpander->setHeader(tr("学生操作"));
    studentActionsExpander->setExpandDirection(ExExpander::Down);

    auto* studentActionsBlock = new QWidget;
    auto* studentActionsLayout = new QVBoxLayout(studentActionsBlock);
    studentActionsLayout->setContentsMargins(0, 0, 0, 0);
    studentActionsLayout->setSpacing(6);

    auto* studentButtonLayout = new QHBoxLayout;
    studentButtonLayout->setContentsMargins(0, 0, 0, 0);
    studentButtonLayout->setSpacing(6);
    addStudentButton = new QPushButton(tr("添加学生"), studentActionsBlock);
    deleteStudentButton = new QPushButton(tr("删除学生"), studentActionsBlock);
    studentButtonLayout->addWidget(addStudentButton);
    studentButtonLayout->addWidget(deleteStudentButton);
    studentActionsLayout->addLayout(studentButtonLayout);

    // 搜索框：输入即时过滤，匹配结果由 StudentSearchModel 提供
    studentSearchEdit = new QLineEdit(studentActionsBlock);
    studentSearchEdit->setPlaceholderText(tr("输入姓名搜索学生"));
    studentSearchEdit->setClearButtonEnabled(true);
    studentActionsLayout->addWidget(studentSearchEdit);

    // 结果列表：高度随结果数变化，无结果时整块隐藏
    studentSearchView = new QListView(studentActionsBlock);
    studentSearchView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    studentSearchView->setVisible(false);
    studentActionsLayout->addWidget(studentSearchView);

    studentSearchHintLabel = new QLabel(tr("未找到匹配的学生"), studentActionsBlock);
    studentSearchHintLabel->setVisible(false);
    studentActionsLayout->addWidget(studentSearchHintLabel);

    studentActionsExpander->addContentWidget(studentActionsBlock);
    studentActionsExpander->setExpanded(true);
    contentLayout->addWidget(studentActionsExpander);

    // 积分记录折叠块：当前查看的记录与上一条/下一条/退出查看，默认展开
    recordExpander = new ExExpander(contentWidget);
    recordExpander->setHeader(tr("积分记录"));
    recordExpander->setExpandDirection(ExExpander::Down);

    auto* recordBlock = new QWidget;
    auto* recordLayout = new QVBoxLayout(recordBlock);
    recordLayout->setContentsMargins(0, 0, 0, 0);
    recordLayout->setSpacing(6);

    // 查看与退出查看按钮行：查看通过选择框指定要查看的记录，退出结束查看
    auto* recordViewLayout = new QHBoxLayout;
    recordViewLayout->setContentsMargins(0, 0, 0, 0);
    recordViewLayout->setSpacing(6);
    recordViewButton = new QPushButton(tr("查看积分记录"), recordBlock);
    recordCloseButton = new QPushButton(tr("退出查看"), recordBlock);
    recordViewLayout->addWidget(recordViewButton);
    recordViewLayout->addWidget(recordCloseButton);
    recordViewLayout->addStretch();
    recordLayout->addLayout(recordViewLayout);

    // 上一项/下一项按钮使用 Segoe Fluent Icons 字体；该字体已由 FluentUI3Style 加载
    constexpr char16_t chromeBackGlyph = static_cast<char16_t>(SegoeIcon::ChromeBack);
    constexpr char16_t chromeBackMirroredGlyph = static_cast<char16_t>(SegoeIcon::ChromeBackMirrored);
    const auto createNavIconButton = [](char16_t glyph, const QString& tooltip, QWidget* parent)
    {
        auto* button = new QPushButton(parent);
        QFont iconFont(QStringLiteral("Segoe Fluent Icons"));
        iconFont.setPixelSize(14);
        button->setFont(iconFont);
        button->setText(QString(QChar(glyph)));
        button->setToolTip(tooltip);
        const int buttonSide = button->sizeHint().height();
        button->setFixedSize(buttonSide, buttonSide);
        return button;
    };
    const auto createFluentTextButton = [](char16_t glyph, const QString& text, QWidget* parent)
    {
        auto* button = new QPushButton(parent);
        QFont buttonFont = button->font();
        QStringList fontFamilies;
        fontFamilies << buttonFont.family() << QStringLiteral("Segoe Fluent Icons");
        buttonFont.setFamilies(fontFamilies);
        button->setFont(buttonFont);
        button->setText(QString(QChar(glyph)) + QStringLiteral("  ") + text);
        return button;
    };

    // 查看信息行：标题占满剩余空间，右侧依次为上一项、当前位置、下一项
    auto* recordNavLayout = new QHBoxLayout;
    recordNavLayout->setContentsMargins(0, 0, 0, 0);
    recordNavLayout->setSpacing(6);
    recordPreviewLabel = new QLabel(recordBlock);
    recordPreviewLabel->setMinimumWidth(0);
    recordPreviewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    recordPreviewLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    recordPreviousButton = createNavIconButton(chromeBackGlyph, tr("上一项"), recordBlock);
    recordPositionLabel = new QLabel(recordBlock);
    recordPositionLabel->setAlignment(Qt::AlignCenter);
    recordNextButton = createNavIconButton(chromeBackMirroredGlyph, tr("下一项"), recordBlock);
    recordNavLayout->addWidget(recordPreviewLabel, 1);
    recordNavLayout->addWidget(recordPreviousButton);
    recordNavLayout->addWidget(recordPositionLabel);
    recordNavLayout->addWidget(recordNextButton);
    recordLayout->addLayout(recordNavLayout);

    auto* recordActionLayout = new QHBoxLayout;
    recordActionLayout->setContentsMargins(0, 0, 0, 0);
    recordActionLayout->setSpacing(6);
    newRecordButton = new QPushButton(tr("新建积分记录"), recordBlock);
    applyTemplateButton = new QPushButton(tr("应用积分模板"), recordBlock);
    recordActionLayout->addWidget(newRecordButton);
    recordActionLayout->addWidget(applyTemplateButton);
    recordLayout->addLayout(recordActionLayout);

    auto* recordNameLayout = new QHBoxLayout;
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

    // 积分模板折叠块：当前查看的模板、导航、名称编辑与更新/放弃，默认展开
    templateExpander = new ExExpander(contentWidget);
    templateExpander->setHeader(tr("积分模板"));
    templateExpander->setExpandDirection(ExExpander::Down);

    auto* templateBlock = new QWidget;
    auto* templateLayout = new QVBoxLayout(templateBlock);
    templateLayout->setContentsMargins(0, 0, 0, 0);
    templateLayout->setSpacing(6);

    // 查看与退出查看按钮行：查看通过选择框指定要查看的模板，退出结束查看
    auto* templateViewLayout = new QHBoxLayout;
    templateViewLayout->setContentsMargins(0, 0, 0, 0);
    templateViewLayout->setSpacing(6);
    templateViewButton = new QPushButton(tr("查看积分模板"), templateBlock);
    templateCloseButton = new QPushButton(tr("退出查看"), templateBlock);
    templateViewLayout->addWidget(templateViewButton);
    templateViewLayout->addWidget(templateCloseButton);
    templateViewLayout->addStretch();
    templateLayout->addLayout(templateViewLayout);

    // 查看信息行：标题占满剩余空间，右侧依次为上一项、当前位置、下一项
    auto* templateNavLayout = new QHBoxLayout;
    templateNavLayout->setContentsMargins(0, 0, 0, 0);
    templateNavLayout->setSpacing(6);
    templatePreviewLabel = new QLabel(templateBlock);
    templatePreviewLabel->setMinimumWidth(0);
    templatePreviewLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    templatePreviewLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    templatePreviousButton = createNavIconButton(chromeBackGlyph, tr("上一项"), templateBlock);
    templatePositionLabel = new QLabel(templateBlock);
    templatePositionLabel->setAlignment(Qt::AlignCenter);
    templateNextButton = createNavIconButton(chromeBackMirroredGlyph, tr("下一项"), templateBlock);
    templateNavLayout->addWidget(templatePreviewLabel, 1);
    templateNavLayout->addWidget(templatePreviousButton);
    templateNavLayout->addWidget(templatePositionLabel);
    templateNavLayout->addWidget(templateNextButton);
    templateLayout->addLayout(templateNavLayout);

    newTemplateButton = new QPushButton(tr("新建积分模板"), templateBlock);
    templateLayout->addWidget(newTemplateButton);

    templateNameEdit = new QLineEdit(templateBlock);
    templateNameEdit->setPlaceholderText(tr("输入新的积分模板名称"));
    templateLayout->addWidget(templateNameEdit);

    auto* templateNameButtonLayout = new QHBoxLayout;
    templateNameButtonLayout->setContentsMargins(0, 0, 0, 0);
    templateNameButtonLayout->setSpacing(6);
    constexpr char16_t syncGlyph = static_cast<char16_t>(SegoeIcon::Sync);
    constexpr char16_t revToggleKeyGlyph = static_cast<char16_t>(SegoeIcon::RevToggleKey);
    templateUpdateButton = createFluentTextButton(syncGlyph, tr("更新名称"), templateBlock);
    templateDiscardButton = createFluentTextButton(revToggleKeyGlyph, tr("放弃修改"), templateBlock);
    templateNameButtonLayout->addWidget(templateUpdateButton);
    templateNameButtonLayout->addWidget(templateDiscardButton);
    templateLayout->addLayout(templateNameButtonLayout);

    templateExpander->addContentWidget(templateBlock);
    templateExpander->setExpanded(true);
    contentLayout->addWidget(templateExpander);

    contentLayout->addStretch();

    connect(dragStudentSwitch, &QCheckBox::stateChanged, this,
            [this](int state)
            {
                emit dragSortChanged(state == Qt::Checked);
            });
    connect(editNameSwitch, &QCheckBox::stateChanged, this,
            [this](int state)
            {
                emit nameEditChanged(state == Qt::Checked);
            });
    connect(editGroupSwitch, &QCheckBox::stateChanged, this,
            [this](int state)
            {
                emit groupEditChanged(state == Qt::Checked);
            });

    connect(addStudentButton, &QPushButton::clicked, this, &StudentTableSidebar::addStudentsRequested);
    connect(deleteStudentButton, &QPushButton::clicked, this, &StudentTableSidebar::deleteStudentRequested);
    connect(studentSearchEdit, &QLineEdit::textChanged, this, &StudentTableSidebar::studentSearchTextChanged);
    connect(studentSearchView, &QListView::clicked, this,
            [this](const QModelIndex& index)
            {
                emit studentSearchResultActivated(index.row());
            });

    connect(recordViewButton, &QPushButton::clicked, this, &StudentTableSidebar::recordViewRequested);
    connect(recordPreviousButton, &QPushButton::clicked, this, &StudentTableSidebar::recordPreviousRequested);
    connect(recordNextButton, &QPushButton::clicked, this, &StudentTableSidebar::recordNextRequested);
    connect(recordCloseButton, &QPushButton::clicked, this, &StudentTableSidebar::recordPreviewCloseRequested);
    connect(newRecordButton, &QPushButton::clicked, this, &StudentTableSidebar::newScoreRecordRequested);
    connect(saveRecordButton, &QPushButton::clicked, this, &StudentTableSidebar::saveScoreRecordRequested);
    connect(applyTemplateButton, &QPushButton::clicked, this, &StudentTableSidebar::applyTemplateRequested);

    connect(templateViewButton, &QPushButton::clicked, this, &StudentTableSidebar::templateViewRequested);
    connect(templatePreviousButton, &QPushButton::clicked, this, &StudentTableSidebar::templatePreviousRequested);
    connect(templateNextButton, &QPushButton::clicked, this, &StudentTableSidebar::templateNextRequested);
    connect(templateCloseButton, &QPushButton::clicked, this, &StudentTableSidebar::templatePreviewCloseRequested);
    connect(newTemplateButton, &QPushButton::clicked, this, &StudentTableSidebar::newTemplateRequested);
    connect(templateNameEdit, &QLineEdit::returnPressed, this,
            [this]
            {
                emit templateNameUpdateRequested(templateNameEdit->text());
            });
    connect(templateUpdateButton, &QPushButton::clicked, this,
            [this]
            {
                emit templateNameUpdateRequested(templateNameEdit->text());
            });
    connect(templateDiscardButton, &QPushButton::clicked, this, &StudentTableSidebar::templateNameEditCancelled);
}

void StudentTableSidebar::setFileInfo(const QString& fileName, bool dirty, int studentCount, int recordCount, int templateCount)
{
    QString displayName = fileName.isEmpty() ? tr("未打开") : fileName;
    if(dirty)
        displayName += tr("（未保存）");
    fileNameLabel->setText(tr("文件名：%1").arg(displayName));
    studentCountLabel->setText(tr("学生数量：%1").arg(studentCount));
    recordCountLabel->setText(tr("积分记录：%1").arg(recordCount));
    templateCountLabel->setText(tr("积分模板：%1").arg(templateCount));
}

void StudentTableSidebar::setRecordPreview(const QString& displayText, const QString& positionText, bool hasPreview, bool canView,
                                           bool canPrevious, bool canNext)
{
    if(hasPreview)
        recordPreviewTitle = displayText;
    else
        recordPreviewTitle = canView ? tr("尚未选择要查看的记录") : tr("暂无可查看的记录");
    recordPreviewLabel->setToolTip(recordPreviewTitle);
    updateRecordPreviewTitle();
    recordPositionLabel->setText(hasPreview ? positionText : tr("-/-"));
    recordViewButton->setEnabled(canView);
    recordPreviousButton->setEnabled(hasPreview && canPrevious);
    recordNextButton->setEnabled(hasPreview && canNext);
    recordCloseButton->setEnabled(hasPreview);
    if(hasPreview && !recordExpander->isExpanded())
        recordExpander->setExpanded(true);
}

void StudentTableSidebar::setTemplatePreview(const QString& displayText, const QString& positionText, bool hasPreview, bool canView,
                                             bool canPrevious, bool canNext)
{
    if(hasPreview)
        templatePreviewTitle = displayText;
    else
        templatePreviewTitle = canView ? tr("尚未选择要查看的模板") : tr("暂无可查看的模板");
    templatePreviewLabel->setToolTip(templatePreviewTitle);
    updateTemplatePreviewTitle();
    templatePositionLabel->setText(hasPreview ? positionText : tr("-/-"));
    templateViewButton->setEnabled(canView);
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

void StudentTableSidebar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateRecordPreviewTitle();
    updateTemplatePreviewTitle();
}

void StudentTableSidebar::updateRecordPreviewTitle()
{
    if(recordPreviewTitle.isEmpty())
        recordPreviewTitle = tr("暂无可查看的记录");
    recordPreviewLabel->setText(
        recordPreviewLabel->fontMetrics().elidedText(recordPreviewTitle, Qt::ElideRight, qMax(0, recordPreviewLabel->width())));
}

void StudentTableSidebar::updateTemplatePreviewTitle()
{
    if(templatePreviewTitle.isEmpty())
        templatePreviewTitle = tr("暂无可查看的模板");
    templatePreviewLabel->setText(
        templatePreviewLabel->fontMetrics().elidedText(templatePreviewTitle, Qt::ElideRight, qMax(0, templatePreviewLabel->width())));
}

void StudentTableSidebar::updateStudentSearchPanel()
{
    const QAbstractItemModel* model = studentSearchView->model();
    const int resultCount = model ? model->rowCount() : 0;
    const bool searching = !studentSearchEdit->text().trimmed().isEmpty();
    studentSearchView->setVisible(resultCount > 0);
    studentSearchHintLabel->setVisible(searching && resultCount == 0);
    if(resultCount > 0)
    {
        // 列表高度跟随结果数，超过5项后由列表内部滚动
        constexpr int maximumVisibleRows = 5;
        const int rowHeight = studentSearchView->sizeHintForRow(0);
        studentSearchView->setFixedHeight(rowHeight * qMin(resultCount, maximumVisibleRows) + 2 * studentSearchView->frameWidth());
    }
}

void StudentTableSidebar::setTemplateNameDraft(const QString& name)
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

void StudentTableSidebar::setStudentSearchModel(QAbstractItemModel* model)
{
    studentSearchView->setModel(model);
    connect(model, &QAbstractItemModel::modelReset, this, &StudentTableSidebar::updateStudentSearchPanel);
    updateStudentSearchPanel();
}

QString StudentTableSidebar::recordNameText() const
{
    return recordNameEdit->text();
}

void StudentTableSidebar::setRecordNameText(const QString& text)
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
