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
    explicit SumExportDialog(QWidget *parent = nullptr, SortSettings settings = SortSettings());
    ~SumExportDialog();
    void setRecordText(QStringList records);
    int getBeginIndex();
    int getEndIndex();
    SortSettings getCurrentConfig();
private slots:
    void updateRecordList(int index_);
    void setGroupModeVisibility(bool f);
private:
    Ui::ExportSumDialog *ui;
    QStringList recordList;
};

#endif // SUM_EXPORT_DIALOG_H
