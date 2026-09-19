#include "TemplateSpinBoxDelegate.h"

#include "fluentui3styleproperties.h"

#include <QSpinBox>
#include <limits>

TemplateSpinBoxDelegate::TemplateSpinBoxDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* TemplateSpinBoxDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    auto* editor = new QSpinBox(parent);
    editor->setAutoFillBackground(true);
    editor->setFrame(false);
    editor->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    editor->setProperty("spinBoxButtonLayout", PlusMinusHorizontalSides);
    return editor;
}
