#pragma once

#include "DataTypes.h"

namespace ScoreSumReport
{

struct SummaryData
{
    QString name;   //学生姓名
    int studentNo;  //学生编号
    int totalScore; //学生总分
    int groupNo;    //小组号
    int groupOrder; //小组中的编号
};

QString generateHtmlString(QVector<SummaryData> dataList, SortSettings mode);

} // namespace ScoreSumReport
