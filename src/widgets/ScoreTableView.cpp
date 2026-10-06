#include "ScoreTableView.h"

#include <QAbstractItemDelegate>
#include <QKeyEvent>
#include <QModelIndex>

ScoreTableView::ScoreTableView(QWidget* parent)
    : QTableView(parent)
{
}

void ScoreTableView::setInlineEditColumns(const QVector<int>& columns)
{
    m_inlineEditColumns = columns;
}

bool ScoreTableView::isInlineColumn(const QModelIndex& index) const
{
    return index.isValid() && m_inlineEditColumns.contains(index.column());
}

// 输入即编辑只对指定列放行；返回false后基类会回退到按输入内容跳转到匹配项
bool ScoreTableView::edit(const QModelIndex& index, EditTrigger trigger, QEvent* event)
{
    if(trigger == AnyKeyPressed && !isInlineColumn(index))
        return false;
    if(!QTableView::edit(index, trigger, event))
        return false;
    if(state() == EditingState)
        m_editIndex = index;
    return true;
}

void ScoreTableView::keyPressEvent(QKeyEvent* event)
{
    // 基类在非 macOS 平台把回车当作“激活”而不打开编辑器，这里自行接管，事件不再下传
    const bool isEnterKey = event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter;
    if(isEnterKey && state() != EditingState && isInlineColumn(currentIndex()) && edit(currentIndex(), AnyKeyPressed, nullptr))
        return;
    QTableView::keyPressEvent(event);
}

void ScoreTableView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint)
{
    QTableView::closeEditor(editor, hint);
    // 只有回车提交才往后跳；Tab 的跳转由基类按 EditNextItem 处理，Esc 不提交也不跳
    if(hint != QAbstractItemDelegate::SubmitModelCache)
        return;
    // 编辑中切换当前行时基类也用这个 hint 关编辑器，此时当前行已不是被编辑的那一行，不能跟着跳
    const QModelIndex current = currentIndex();
    if(current != m_editIndex)
        return;
    m_editIndex = QModelIndex();
    if(!isInlineColumn(current) || !model())
        return;
    const QModelIndex next = model()->index(current.row() + 1, current.column());
    if(!next.isValid())
        return;
    setCurrentIndex(next);
    scrollTo(next);
}
