#ifndef ITEMSELECTIONDIALOG_H
#define ITEMSELECTIONDIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

namespace Ui {
class ItemSelectionDialog;
}

class ItemSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ItemSelectionDialog(QWidget *parent, const QString &title, const QString &label,const QStringList &items, int current = 0);
    ~ItemSelectionDialog();
    int selectedIndex();

private:
    Ui::ItemSelectionDialog *ui;
};

#endif // ITEMSELECTIONDIALOG_H
