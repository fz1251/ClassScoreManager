#include "GroupSpinBoxDelegate.h"

#include <FluentUI3Style/fluentui3styleproperties.h>
#include <QSpinBox>
#include <limits>

GroupSpinBoxDelegate::GroupSpinBoxDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *GroupSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto *editor = new QSpinBox(parent);
    editor->setAutoFillBackground(true);
    editor->setFrame(false);
    editor->setRange(0, std::numeric_limits<int>::max());
    editor->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);
    return editor;
}
