#ifndef SCORESUMREPORT_H
#define SCORESUMREPORT_H

#include "DataTypes.h"

namespace ScoreSumReport {

struct SummaryData
{
    QString name;   //学生姓名
    int studentNo;  //学生编号
    int totalScore; //学生总分
    int groupNo;    //小组号
};

QString generateHtmlString(QVector<SummaryData> dataList,SortSettings mode);

} // namespace ScoreSumReport

#endif // SCORESUMREPORT_H
