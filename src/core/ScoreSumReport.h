#pragma once

#include "DataTypes.h"

namespace ScoreSumReport
{

struct SummaryData
{
    QString name;      //学生姓名
    qint32 studentNo;  //学生编号
    qint32 totalScore; //学生总分
    qint32 groupNo;    //小组号
    qint32 groupOrder; //小组中的编号
};

QString generateHtmlString(QVector<SummaryData> dataList, SortSettings mode);

} // namespace ScoreSumReport
