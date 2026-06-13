#ifndef CLASSTABLEMODAL_H
#define CLASSTABLEMODAL_H

#include "../core/ClassDataManager.h"
#include <QMap>
#include <QBrush>
#include <QMimeData>
#include <QByteArray>
#include <QAbstractTableModel>

class ClassTableModal : public QAbstractTableModel
{
    Q_OBJECT
public:
    // 表格列枚举
    enum TableColumnType
    {
        StudentName = 0,
        GroupNumber,
        CurrentRecord,
        CurrentTemplate,
        RecordHistoryPreview,
        ScoreTemplatePreview,
        MaxColumnCount
    };

    // 表格模型所用函数
    explicit ClassTableModal(ClassDataManager* dataManager, QObject *parent = nullptr);
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

    // 学生信息相关函数
    void addStudent (const QString& newName);
    void importStudents(const QStringList& newNames);
    void removeStudent (int displayIndex);
    void clearStudents();

    // 积分记录相关函数
    int  getCurRecordIndex() const;
    void showRecordHistory(int index);
    void hideRecordHistory();
    void newScoreRecord(const QString& newRecordName);
    void activateTemplate(int index);
    void saveScoreRecord();
    void deleteRecordHistory(int index);
    void clearScoreRecord();

    // 积分模板相关函数
    int  getCurTemplateIndex() const;
    void showScoreTemplate(int index);
    void hideScoreTemplate();
    void newScoreTemplate(const ScoreTemplate& newTemplate);
    void setTemplateDescription(const QString& newDescription);
    void deleteScoreTemplate(int index);
    void clearScoreTemplate();

    // 刷新全表格函数
    void resetTable();
public slots:
    void setStudentEditable(bool editable);
    void setGroupEditable(bool editable);
signals:
    void showColumnRequested(int column);
    void hideColumnRequested(int column);
private:
    static const inline QString moveRowDataType = "application/row_data";
    static const inline int INVALID_INDEX = -1;

    // 辅助获取manager中内容
    const QVector<StudentInfo>&   students()  const { return manager->getStudents(); };
    const QVector<ScoreTemplate>& templates() const { return manager->getTemplates(); };
    const QVector<ScoreRecord>&   records()   const { return manager->getRecords(); };

    // 验证索引有效性
    bool isValidModalIndex(const QModelIndex& index) const;
    bool isValidIndex(int index) const;

    // 包装显示/隐藏列功能
    void showColumnWrapper(int column);
    void hideColumnWrapper(int column);

    // 获取HTML表格中积分颜色
    static QBrush getScoreColor(int scoreValue);

    ClassDataManager* manager;
    bool columnIsVisible[MaxColumnCount];
    bool canEditStudent = true;
    bool canEditGroup   = true;
    QVector<int> rowToIndex; // rowIndex[0,size-1]=>students[0,size-1]
    int templateIndex, recordIndex;
    int activatedTemplateIndex;
};

#endif // CLASSTABLEMODAL_H
