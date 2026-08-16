#ifndef GROUPSPINBOXDELEGATE_H
#define GROUPSPINBOXDELEGATE_H

#include <QStyledItemDelegate>

class GroupSpinBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit GroupSpinBoxDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif // GROUPSPINBOXDELEGATE_H
