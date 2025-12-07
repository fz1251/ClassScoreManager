#include "score_expr_delegate.h"
#include <QRegularExpression>
#include <QRegularExpressionValidator>

ScoreParseResult parseScoreExpression(const QString &expr)
{
    ScoreParseResult res;
    if(expr.isEmpty())
        return res;
    static QRegularExpression termRegExp("(?=[+-])");
    QStringList terms=expr.split(termRegExp,Qt::SkipEmptyParts);
    long long total=0;
    for(const QString& term:terms)
    {
        bool ok;
        int curValue=term.toInt(&ok);
        if(!ok)
        {
            res.isSuccess=false;
            return res;
        }
        total+=curValue;
        if(total>INT_MAX||total<INT_MIN)
        {
            res.isSuccess=false;
            return res;
        }
    }
    res.value=static_cast<int>(total);
    return res;
}

ScoreExprDelegate::ScoreExprDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{

}

QWidget *ScoreExprDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    QLineEdit* lineEdit=new QLineEdit(parent);
    QRegularExpression scoreRegExp(scoreRegExpStr);
    QRegularExpressionValidator* validator=new QRegularExpressionValidator(scoreRegExp,parent);
    lineEdit->setValidator(validator);
    return lineEdit;
}

void ScoreExprDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    static_cast<QLineEdit*>(editor)->setText(index.model()->data(index).toString());
}

void ScoreExprDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString scoreExpr=static_cast<QLineEdit*>(editor)->text();
    QRegularExpression scoreRegExp(scoreRegExpStr);
    if(scoreRegExp.match(scoreExpr).hasMatch())
    {
        ScoreParseResult res=parseScoreExpression(scoreExpr);
        if(res.isSuccess)
            model->setData(index,res.value);
    }
}
