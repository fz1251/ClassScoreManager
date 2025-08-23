#ifndef CLASSTABLEMODAL_H
#define CLASSTABLEMODAL_H

#include "classdatamanager.h"
#include <QMap>
#include <QBrush>
#include <QMimeData>
#include <QByteArray>
#include <QAbstractTableModel>

enum TableColumnType
{
    StudentName=0,
    GroupNumber,
    CurrentRecord,
    CurrentTemplate,
    RecordHistoryPreview,
    ScoreTemplatePreview
};

class ClassTableModal : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ClassTableModal(QObject *parent = nullptr,ClassDataManager* dataManager=nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    void newScoreRecord(const QString& newRecordName);
    void activateTemplate(int index);
    void saveScoreRecord();
    void newScoreTemplate(const ScoreTemplate& newTemplate);
    void setTemplateDescription(const QString& newDescription);

    int getCurTemplateIndex() const;
    int getCurRecordIndex()   const;
    void showRecordHistory(int index);
    void hideRecordHistory();
    void showScoreTemplate(int index);
    void hideScoreTemplate();
    void deleteRecordHistory(int index);
    void deleteScoreTemplate(int index);

    void resetTable();

signals:
    void showColumnRequested(int column);
    void hideColumnRequested(int column);
private:
    bool validateModalIndex(const QModelIndex& index) const;
    bool columnIsVisible[8];
    void showColumnWrapper(int column);
    void hideColumnWrapper(int column);
    static const inline QString moveRowDataType="application/row_data";
    static QBrush getScoreColor(int scoreValue);
    ClassDataManager* manager;
    const QList<StudentInfo>& students;
    const QList<ScoreTemplate>& templates;
    const QList<ScoreRecord>& records;
    QList<int> rowToIndex;
    int templateIndex,recordIndex;
    int activatedTemplateIndex;
};

#endif // CLASSTABLEMODAL_H
