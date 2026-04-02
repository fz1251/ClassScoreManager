#ifndef PASTE_STUDENTS_DIALOG_H
#define PASTE_STUDENTS_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QListWidget>
#include <QListWidgetItem>

namespace Ui {
class PasteStudentsDialog;
}

class PasteStudentsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PasteStudentsDialog(QWidget *parent = nullptr);
    ~PasteStudentsDialog();
    QStringList getNameList();

private slots:
    void on_parseTextButton_clicked(bool checked);

    void on_editListButton_clicked(bool checked);

private:
    Ui::PasteStudentsDialog *ui;
};

#endif // PASTE_STUDENTS_DIALOG_H
