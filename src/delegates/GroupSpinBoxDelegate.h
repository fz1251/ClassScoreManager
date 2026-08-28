#pragma once

#include <QStyledItemDelegate>

class GroupSpinBoxDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit GroupSpinBoxDelegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

