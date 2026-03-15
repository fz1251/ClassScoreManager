#include "score_sum_report.h"
#include <algorithm>
#include <map>

//FIXME:移除不再有效的状态，优化小组信息传参

//前向声明
namespace
{
using namespace ScoreSumReport;
namespace student
{
QString generate(QList<SummaryData> dataList, SortSetting::SortMode mode);
} // namespace student
namespace group
{
QString generate(QList<SummaryData> dataList, SortSetting::SortMode mode);
} // namespace group
} // namespace anonymous

namespace ScoreSumReport
{

Sorter::Sorter(SortSetting::SortMode mode)
{
    auto setStudentCmp = [mode](getFunc & get_, cmpFunc & cmp_) -> void
    {
        bool sortBySum = mode & SortSetting::sortByStudentSum;
        bool filpSorting = mode & SortSetting::flipStudentSorting;
        if(sortBySum)
            get_ = studentSum;
        else
            get_ = studentNumber;
        if(sortBySum ^ filpSorting)
            cmp_ = bigToLittle;
        else
            cmp_ = littleToBig;
    };
    if(mode & SortSetting::sortAsGroup)
    {
        bool sortBySum = mode & SortSetting::sortByGroupSum;
        bool filpSorting = mode & SortSetting::flipGroupSorting;
        if(sortBySum)
            get1 = groupSum;
        else
            get1 = groupNumber;
        if(sortBySum ^ filpSorting)
            cmp1 = bigToLittle;
        else
            cmp1 = littleToBig;
        setStudentCmp(get2, cmp2);
    }
    else
    {
        setStudentCmp(get1, cmp1);
        get2 = studentNumber;
        cmp2 = littleToBig;
    }
}

bool Sorter::operator()(const SummaryData &lhs, const SummaryData &rhs)
{
    if(get1(lhs) == get1(rhs))
        return cmp2(get2(lhs), get2(rhs));
    else
        return cmp1(get1(lhs), get1(rhs));
}

qint32 studentNumber(const SummaryData &obj)
{
    return obj.studentNum;
}

qint32 groupNumber(const SummaryData &obj)
{
    return obj.groupNum;
}

qint32 studentSum(const SummaryData &obj)
{
    return obj.studentScore;
}

qint32 groupSum(const SummaryData &obj)
{
    return obj.groupScore;
}

qint32 emptyGet(const SummaryData &obj)
{
    Q_UNUSED(obj);
    return 0;
}

bool littleToBig(qint32 lhs, qint32 rhs)
{
    return lhs < rhs;
}

bool bigToLittle(qint32 lhs, qint32 rhs)
{
    return lhs > rhs;
}

bool emptyCmp(qint32 lhs, qint32 rhs)
{
    Q_UNUSED(lhs);
    Q_UNUSED(rhs);
    return true;
}

QString generateHtmlString(QList<SummaryData> dataList, SortSetting::SortMode mode)
{
    if(mode & SortSetting::sortAsGroup)
        return group::generate(dataList, mode);
    else
        return student::generate(dataList, mode);
}
} // namespace ScoreSumReport
namespace
{
namespace student
{
const QString HTMLTemplate = QStringLiteral(R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>积分求和导出表格</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Segoe UI', 'Microsoft YaHei', 'sans-serif';
            font-size: 16px;
            background: linear-gradient(135deg, #f5f7fa 0%, #e4e7eb 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }

        .table-container {
            background-color: #ffffff;
            border-radius: 16px;
            box-shadow: 0 15px 40px rgba(0, 0, 0, 0.12);
            padding: 40px;
            width: 95%;
            max-width: 1000px;
            position: relative;
            overflow: hidden;
        }

        h1 {
            text-align: center;
            color: #2c3e50;
            margin-bottom: 35px;
            font-size: 2.4rem;
        }

        .data-table {
            width: 100%;
            border-collapse: collapse;
            font-size: 18px;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0 5px 15px rgba(0, 0, 0, 0.06);
            table-layout: fixed;
        }

        .data-table thead {
            background: linear-gradient(135deg, #6c8fa9 0%, #5a7b95 100%);
            color: #ffffff;
        }

        .data-table th {
            padding: 18px 20px;
            text-align: left;
            font-weight: 600;
            font-size: 1.3rem;
        }

        .data-table th:first-child {
            border-top-left-radius: 10px;
        }

        .data-table th:last-child {
            border-top-right-radius: 10px;
        }

        .data-table tbody tr:nth-child(even) {
            background-color: #f8f9fa;
        }

        .data-table td {
            padding: 16px 20px;
            border-bottom: 1px solid #e1e8ed;
            color: #333333;
        }

        .data-table tbody tr:last-child td {
            border-bottom: none;
        }

        .positive-sum {
            color: #27ae60 !important;
            font-weight: 600;
        }

        .negative-sum {
            color: #e74c3c !important;
            font-weight: 600;
        }

        .zero-sum {
            color: #7f8c8d !important;
            font-weight: 600;
        }
    </style>
</head>
<body>
    <div class="table-container">
        <h1>积分求和结果</h1>
            <table class="data-table">
                <thead> <tr> %1 </tr> </thead>
                <tbody> %2 </tbody>
            </table>
    </div>
</body>
</html>
)");

const QString getSumTextClass(int score)
{
    if(score>0)
        return "positive-sum";
    else if(score<0)
        return "negative-sum";
    else
        return "zero-sum";
}

const QString HTMLTableHeadBase=QStringLiteral(R"(
<th>姓名</th>
<th>积分</th>
)");

const QString HTMLTableHeadStudent=QStringLiteral(R"(
<th>编号</th>
)");

const QString HTMLTableRowTemplate=QStringLiteral(R"(
<tr> %1 </tr>
)");

const QString HTMLTableItemBase=QStringLiteral(R"(
<td> %1 </td>
<td class="%3"> %2 </td>
)");

const QString HTMLTableItemStudent=QStringLiteral(R"(
<td> %1 </td>
)");

QString generate(QList<SummaryData> dataList, SortSetting::SortMode mode)
{
    QString tableHead,tableBody;
    if(!(mode&SortSetting::hideStudentNumber))
        tableHead+=HTMLTableHeadStudent;
    tableHead+=HTMLTableHeadBase;
    for(const SummaryData& i:dataList)
    {
        QString tableItem;
        if(!(mode&SortSetting::hideStudentNumber))
            tableItem+=HTMLTableItemStudent.arg(i.studentNum);
        tableItem+=HTMLTableItemBase.arg(i.name).arg(i.studentScore).arg(getSumTextClass(i.studentScore));

        tableBody+=HTMLTableRowTemplate.arg(tableItem);
    }
    return HTMLTemplate.arg(tableHead,tableBody);
}
} // namespace student

namespace group {
const QString HTMLTemplate=QStringLiteral(R"(
<!doctype html>
<html lang="zh-CN">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>积分求和导出表格</title>
  <style>
    * {
      box-sizing: border-box;
    }

    body {
      font-family: system-ui, -apple-system, "Microsoft YaHei", "PingFang SC", sans-serif;
      background: #f5f7fa;
      margin: 20px;
      color: #333;
      line-height: 1.2;
    }

    .groups-row {
      display: flex;
      flex-wrap: wrap;
      gap: 16px;
      width: 100%;
    }

    .group-card {
      background: #fff;
      border-radius: 12px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.06);
      padding: 20px;
      flex: 1 1 auto;
      min-width: 300px;
      display: flex;
      flex-direction: column;
    }

    .card-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      border-bottom: 2px solid #f0f2f5;
      padding-bottom: 10px;
      margin-bottom: 20px;
      flex-shrink: 0;
    }

    .header-left {
      display: flex;
      align-items: baseline;
      gap: 16px;
      overflow: hidden;
    }

    .group-name {
      font-size: 1.5rem;
      color: #2c3e50;
      font-weight: 800;
      white-space: nowrap;
    }

    .group-summary {
      font-size: 1rem;
      color: #7f8c8d;
      font-weight: 500;
      white-space: nowrap;
    }

    .group-index {
      font-size: 1.5rem;
      font-weight: 700;
      color: #3498db;
      white-space: nowrap;
      background: #ebf5fb;
      padding: 8px 16px;
      border-radius: 20px;
    }

    .card-body {
      display: flex;
      flex-direction: column;
      gap: 16px;
      flex: 1;
      min-height: 0;
    }

    .stats-grid {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
    }

    .stat-item {
      display: flex;
      align-items: center;
      flex: 1 0 auto;
      padding: 16px 20px;
      background: #fafbfc;
      border: 1px solid #eef2f7;
      border-radius: 8px;
    }

    .stat-head {
      font-size: 1.25rem;
      color: #2c3e50;
      font-weight: 600;
      flex-shrink: 0;
      white-space: nowrap;
      margin-right: auto;
    }

    .stat-content {
      display: flex;
      align-items: baseline;
      gap: 6px;
      flex-shrink: 0;
    }

    .stat-number {
      font-size: 1.8rem;
      font-weight: 1000;
      white-space: nowrap;
      font-variant-numeric: tabular-nums;
      line-height: 1;
    }

    .stat-unit {
      flex-shrink: 0;
      color: #95a5a6;
      font-size: 1rem;
      font-weight: 400;
      line-height: 1.2;
    }

    .positive-number { color: #27ae60 !important; }
    .negative-number { color: #e74c3c !important; }
    .zero-number { color: #95a5a6 !important; }
  </style>
</head>
<body>
  <div class="groups-row">
    %1
  </div>
</body>
</html>
)");
const QString HTMLGroupCard=QStringLiteral(R"(
<div class="group-card">
  <div class="card-header">
    <div class="header-left">
      <div class="group-name">%1</div>
      <div class="group-summary">总分: <span class="stat-number %3">%2</span></div>
    </div>
    <div class="group-index">No.%4</div>
  </div>
  <div class="card-body">
    <div class="stats-grid">
      %5
    </div>
  </div>
</div>
)");
const QString HTMLStudentItem=QStringLiteral(R"(
<div class="stat-item">
  <div class="stat-head">%1</div>
  <div class="stat-content">
    <div class="stat-number %3">%2</div>
    <div class="stat-unit">分</div>
  </div>
</div>
)");
QString generate(QList<SummaryData> dataList, SortSetting::SortMode mode)
{
    auto getGroupName=[](int num) -> QString
    {
        if(num==StudentInfo::NoGroup)
            return QObject::tr("未分组");
        else
            return QObject::tr("%1组").arg(num);
    };
    auto getSumTextClass=[](int num) -> QString
    {
        if(num>0)
            return "positive-number";
        else if(num<0)
            return "negative-number";
        else
            return "zero-number";
    };

    std::map<int,int> groupCount,groupScoreSum;
    std::multimap<int,int,std::greater<int>> groupRank;
    if(mode&SortSetting::sortAsGroup)
    {
        //Step 1:记录各小组学生个数和总分
        for(const SummaryData& i:dataList)
        {
            groupCount[i.groupNum]++;
            groupScoreSum[i.groupNum]+=i.studentScore;
        }
        //Step 2:写入各学生对应的小组总分
        for(SummaryData& i:dataList)
            i.groupScore=groupScoreSum[i.groupNum];
        //Step 3:写入小组总分排名
        for(auto i:groupScoreSum)
            groupRank.insert({i.second,i.first});
    }
    std::sort(dataList.begin(),dataList.end(),Sorter(mode));

    QString HTMLBody,groupContent,groupStudents;
    int curGroupIndex=INT_MIN,groupRankCnt=0;
    for(const SummaryData& i:dataList)
    {
        //暂时忽略未分组
        if(i.groupNum==StudentInfo::NoGroup) continue;

        if(curGroupIndex!=i.groupNum)
        {
            if(curGroupIndex!=INT_MIN)
            {
                //填充%5，完成上一个小组的设置
                HTMLBody+=groupContent.arg(groupStudents);
                groupContent.clear();
                groupStudents.clear();
            }
            //填充新的小组
            curGroupIndex=i.groupNum;
            groupContent=HTMLGroupCard.arg(getGroupName(i.groupNum))
                         .arg(i.groupScore)
                         .arg(getSumTextClass(i.groupScore))
                         .arg(++groupRankCnt);

        }
        groupStudents+=HTMLStudentItem.arg(i.name).arg(i.studentScore).arg(getSumTextClass(i.studentScore));
    }
    HTMLBody+=groupContent.arg(groupStudents);
    return HTMLTemplate.arg(HTMLBody);
}
} // namespace group
} // namespace anonymous
