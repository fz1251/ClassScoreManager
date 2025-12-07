#ifndef SUM_EXPORT_DIALOG_H
#define SUM_EXPORT_DIALOG_H

#include <data_types.h>
#include <QDialog>
#include <QStringList>

namespace Ui {
class ExportSumDialog;
}

class SumExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SumExportDialog(QWidget *parent = nullptr);
    ~SumExportDialog();
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

#endif // SUM_EXPORT_DIALOG_H
