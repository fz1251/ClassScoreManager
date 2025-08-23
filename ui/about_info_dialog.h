#ifndef ABOUTINFODIALOG_H
#define ABOUTINFODIALOG_H

#include <QDialog>

namespace Ui {
class AboutInfoDialog;
}

class AboutInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutInfoDialog(QWidget *parent = nullptr);
    ~AboutInfoDialog();

private:
    Ui::AboutInfoDialog *ui;
};

#endif // ABOUTINFODIALOG_H
