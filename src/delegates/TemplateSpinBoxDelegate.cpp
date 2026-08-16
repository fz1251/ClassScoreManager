#include "TemplateSpinBoxDelegate.h"

#include <FluentUI3Style/fluentui3styleproperties.h>
#include <QSpinBox>
#include <limits>

TemplateSpinBoxDelegate::TemplateSpinBoxDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget *TemplateSpinBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto *editor = new QSpinBox(parent);
    editor->setRange(std::numeric_limits<int>::min(),
                     std::numeric_limits<int>::max());
    editor->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);
    editor->setProperty(NoRoundedCorners, true);
    return editor;
}
