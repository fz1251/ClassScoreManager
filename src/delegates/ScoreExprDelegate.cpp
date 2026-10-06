#include "ScoreExprDelegate.h"

#include "exinfobar.h"
#include "exinfobarhost.h"

#include <QLineEdit>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStringList>

ScoreParseResult parseScoreExpression(const QString& expr)
{
    ScoreParseResult res;
    if(expr.isEmpty())
        return res;
    static QRegularExpression termRegExp("(?=[+-])");
    QStringList terms = expr.split(termRegExp, Qt::SkipEmptyParts);
    long long total = 0;
    for(const QString& term : terms)
    {
        bool ok;
        int curValue = term.toInt(&ok);
        if(!ok)
        {
            res.isSuccess = false;
            return res;
        }
        total += curValue;
        if(total > INT_MAX || total < INT_MIN)
        {
            res.isSuccess = false;
            return res;
        }
    }
    res.value = static_cast<qint32>(total);
    return res;
}

ScoreExprDelegate::ScoreExprDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* ScoreExprDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    auto* lineEdit = new QLineEdit(parent);
    // //部分控件底色带透明，防止把单元格文本显示出来
    lineEdit->setAutoFillBackground(true);
    // 让控件不绘制边框
    lineEdit->setFrame(false);
    // 设置正则表达式验证器，父对象取行编辑本身，随编辑器一起销毁
    lineEdit->setValidator(new QRegularExpressionValidator(QRegularExpression(scoreRegExpStr), lineEdit));
    return lineEdit;
}

void ScoreExprDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QString scoreExpr = static_cast<QLineEdit*>(editor)->text();
    QRegularExpression scoreRegExp(scoreRegExpStr);
    if(!scoreRegExp.match(scoreExpr).hasMatch())
        return;
    ScoreParseResult res = parseScoreExpression(scoreExpr);
    if(!res.isSuccess)
    {
        //表达式合法但结果超出 int 范围，保留原值并提示
        ExInfoBarHost::defaultHost()->showInfoBar(ExInfoBar::Warning, tr("警告"), tr("积分超出可表示的范围，本次输入未生效"),
                                                  ExInfoBarHost::TopRight);
        return;
    }
    model->setData(index, res.value);
}
