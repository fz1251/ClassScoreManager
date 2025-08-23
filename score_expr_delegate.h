#ifndef SCOREEXPRDELEGATE_H
#define SCOREEXPRDELEGATE_H

#include <QString>
#include <QStringList>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

// 解析结果结构体
struct ScoreParseResult {
    bool isSuccess=true;       // 解析状态
    int value = 0;             // 计算结果
};

ScoreParseResult parseScoreExpression(const QString& expr);

class ScoreExprDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    ScoreExprDelegate(QObject* parent=nullptr);
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
private:
    static const inline QString scoreRegExpStr="^(?:[+-]?\\d+)(?:[+-]\\d+)*$";
};

#endif // SCOREEXPRDELEGATE_H
