#ifndef EXPORTSUMDIALOG_H
#define EXPORTSUMDIALOG_H

#include <scoresumreport.h>
#include <QDialog>
#include <QStringList>

namespace Ui {
class ExportSumDialog;
}

class ExportSumDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportSumDialog(QWidget *parent = nullptr);
    ~ExportSumDialog();
    void setRecordText(QStringList records);
    void setSortingConfig(SortSetting settings);
    int getBeginIndex();
    int getEndIndex();
    quint8 getCurrentConfig();
private slots:
    void updateRecordList(int unused_index);

    void on_studentDefaultBtn_clicked();

    void on_groupDefaultBtn_clicked();

    void on_studentPreviousBtn_clicked();

    void on_groupPreviousBtn_clicked();

private:
    void setConfig(quint8 mode);
    Ui::ExportSumDialog *ui;
    QStringList recordList;
    SortSetting sortConfig;
};

#endif // EXPORTSUMDIALOG_H
