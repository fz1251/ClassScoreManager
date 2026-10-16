#pragma once

#include "DataTypes.h"

namespace ScoreSumReport
{

struct SummaryData
{
    QString name;          //学生姓名
    qint32 studentNo = 0;  //学生编号
    qint32 totalScore = 0; //学生总分
    qint32 groupNo = 0;    //小组号
    qint32 groupOrder = 0; //小组中的编号
};

QString generateHtmlString(const QVector<SummaryData>& dataList, const SortSettings& mode);

} // namespace ScoreSumReport
