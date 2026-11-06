#include "SearchPane.h"

#include "CloseButton.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QModelIndex>
#include <QPalette>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace
{
QColor dimTextColor(const QWidget* widget)
{
    const QPalette palette = widget->palette();
    QColor color = palette.color(QPalette::PlaceholderText);
    if(!color.isValid())
        color = palette.color(QPalette::Disabled, QPalette::WindowText);
    return color;
}

void applyDimPalette(QLabel* label)
{
    QPalette palette = label->palette();
    palette.setColor(QPalette::WindowText, dimTextColor(label));
    label->setPalette(palette);
}
} // namespace

SearchPane::SearchPane(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);
    setMaximumWidth(420);

    auto* rootLayout = new QVBoxLayout(this);
    // 顶部与右侧页面内容共用 4px 基线
    rootLayout->setContentsMargins(12, 4, 12, 12);
    rootLayout->setSpacing(8);

    // 标题行：当前搜索范围的标题与关闭按钮
    auto* headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(6);

    titleLabel = new QLabel(this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    closeButton = new CloseButton(this);
    closeButton->setToolTip(tr("关闭搜索窗格"));

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(closeButton);
    rootLayout->addLayout(headerLayout);

    // 搜索框：全项目唯一一个搜索输入
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("使用名称、拼音或首字母查找内容"));
    searchEdit->setClearButtonEnabled(true);
    rootLayout->addWidget(searchEdit);

    // 结果区：有结果时显示列表，无结果时显示提示
    resultStack = new QStackedWidget(this);
    resultView = new QListView(resultStack);
    resultView->setFrameShape(QFrame::NoFrame);
    resultView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultView->setSelectionMode(QAbstractItemView::SingleSelection);
    resultView->setUniformItemSizes(true);
    resultView->setResizeMode(QListView::Adjust);
    resultView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    resultView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    resultView->setMouseTracking(true);
    emptyLabel = new QLabel(resultStack);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);
    applyDimPalette(emptyLabel);
    resultStack->addWidget(resultView);
    resultStack->addWidget(emptyLabel);
    rootLayout->addWidget(resultStack, 1);

    connect(searchEdit, &QLineEdit::textChanged, this, &SearchPane::searchTextChanged);
    connect(searchEdit, &QLineEdit::returnPressed, this, &SearchPane::activateCurrentResult);
    connect(resultView, &QListView::clicked, this,
            [this](const QModelIndex& index)
            {
                emit resultActivated(index.row());
            });
    connect(resultView, &QListView::activated, this,
            [this](const QModelIndex& index)
            {
                emit resultActivated(index.row());
            });
    connect(closeButton, &QPushButton::clicked, this, &SearchPane::closeRequested);

    updateEmptyState();
}

void SearchPane::setModel(QAbstractItemModel* model)
{
    resultView->setModel(model);
    if(model)
    {
        connect(model, &QAbstractItemModel::modelReset, this, &SearchPane::updateEmptyState);
        connect(model, &QAbstractItemModel::rowsInserted, this,
                [this](const QModelIndex&, int, int)
                {
                    updateEmptyState();
                });
        connect(model, &QAbstractItemModel::rowsRemoved, this,
                [this](const QModelIndex&, int, int)
                {
                    updateEmptyState();
                });
    }
    updateEmptyState();
}

void SearchPane::setSearchContext(const QString& title, const QString& emptyText, const QString& noMatchText)
{
    titleLabel->setText(title);
    this->emptyText = emptyText;
    this->noMatchText = noMatchText;
    updateEmptyState();
}

void SearchPane::clearSearch()
{
    searchEdit->clear();
}

void SearchPane::focusSearch()
{
    // 用非键盘焦点原因：打开窗格不应该进入 Tab 的键盘焦点态，否则会带出系统焦点框
    searchEdit->setFocus(Qt::OtherFocusReason);
    searchEdit->selectAll();
}

void SearchPane::updateEmptyState()
{
    const int resultCount = resultView->model() ? resultView->model()->rowCount() : 0;
    const bool searching = !searchEdit->text().trimmed().isEmpty();
    emptyLabel->setText(searching ? noMatchText : emptyText);
    if(resultCount > 0)
        resultStack->setCurrentWidget(resultView);
    else
        resultStack->setCurrentWidget(emptyLabel);
}

void SearchPane::activateCurrentResult()
{
    if(!resultView->model())
        return;
    const QModelIndex current = resultView->currentIndex();
    if(current.isValid() && current.row() < resultView->model()->rowCount())
    {
        emit resultActivated(current.row());
        return;
    }
    if(resultView->model()->rowCount() > 0)
    {
        resultView->setCurrentIndex(resultView->model()->index(0, 0));
        emit resultActivated(0);
    }
}
