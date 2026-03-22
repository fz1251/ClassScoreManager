#ifndef SCORE_SUM_REPORT_H
#define SCORE_SUM_REPORT_H

#include "data_types.h"

namespace ScoreSumReport {

struct SummaryData
{
    QString name;   //学生姓名
    int studentNo;  //学生编号
    int totalScore; //学生总分
    int groupNo;    //小组号
};

QString generateHtmlString(QList<SummaryData> dataList,SortSettings mode);

} // namespace ScoreSumReport

#endif // SCORE_SUM_REPORT_H
