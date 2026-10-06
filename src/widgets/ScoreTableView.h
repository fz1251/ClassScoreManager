#pragma once

#include <QTableView>
#include <QVector>

class QKeyEvent;

// 学生表格视图：在指定列上提供“输入即编辑、回车进入编辑、回车提交后移到下一行同一列”的录入体验
class ScoreTableView : public QTableView
{
    Q_OBJECT
public:
    explicit ScoreTableView(QWidget* parent = nullptr);

    // 指定允许上述内联编辑行为的列；其余列保持只能双击或按 F2 编辑
    void setInlineEditColumns(const QVector<int>& columns);

protected:
    bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) override;

private:
    bool isInlineColumn(const QModelIndex& index) const;

    QVector<int> m_inlineEditColumns;
    QModelIndex m_editIndex; // 当前打开的编辑器所在单元格：区分“回车提交”与“编辑中切换当前行”两种关闭
};
