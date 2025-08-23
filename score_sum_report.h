#ifndef SCORESUMREPORT_H
#define SCORESUMREPORT_H

#include "datatypes.h"
#include <algorithm>
#include <map>

namespace ScoreSumReport {

struct SummaryData
{
    QString name;
    qint32 studentNum,groupNum,studentScore,groupScore;
};

qint32 studentNumber(const SummaryData& obj);
qint32 groupNumber(const SummaryData& obj);
qint32 studentSum(const SummaryData& obj);
qint32 groupSum(const SummaryData& obj);
qint32 emptyGet(const SummaryData& obj);

bool littleToBig(qint32 lhs,qint32 rhs);
bool bigToLittle(qint32 lhs,qint32 rhs);
bool emptyCmp(qint32 lhs,qint32 rhs);

using getFunc=qint32(*)(const SummaryData& );
using cmpFunc=bool(*)(qint32,qint32);

class Sorter
{
public:
    Sorter(SortSetting::SortMode mode);
    bool operator()(const SummaryData& lhs,const SummaryData& rhs);
private:
    getFunc get1,get2;
    cmpFunc cmp1,cmp2;
};

QString generateHtmlString(QList<SummaryData> dataList,SortSetting::SortMode mode);

} // namespace ScoreSumReport

#endif // SCORESUMREPORT_H
