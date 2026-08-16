#ifndef TEMPLATESPINBOXDELEGATE_H
#define TEMPLATESPINBOXDELEGATE_H

#include <QStyledItemDelegate>

class TemplateSpinBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TemplateSpinBoxDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // TEMPLATESPINBOXDELEGATE_H
