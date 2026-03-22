#include "score_sum_report.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

//FIXME:移除不再有效的状态，优化小组信息传参

//前向声明
namespace
{
using namespace ScoreSumReport;

struct StudentSumData
{
    QString name;
    int number,score,rank;
};
struct GroupSumData
{
    QList<StudentSumData> members;
    int number,score,rank;
};

namespace student
{
QString generate(std::vector<StudentSumData> dataList, bool showStudentNumber = false);
} // namespace student
namespace group
{
QString generate(std::vector<GroupSumData> dataList, bool showStudentNumber = false);

} // namespace group
} // namespace anonymous

namespace ScoreSumReport
{
/*using getFunc=int(*)(const StudentSumData& );
using cmpFunc=bool(*)(int,int);
class Sorter
{
public:
    Sorter(SortSetting::SortMode mode);
    bool operator()(const StudentSumData& lhs,const StudentSumData& rhs);
private:
    getFunc get1,get2;
    cmpFunc cmp1,cmp2;
};

int studentNumber(const StudentSumData& obj)
{
    return obj.number;
}

int studentSum(const StudentSumData& obj)
{
    return obj.score;
}

bool littleToBig(int lhs, int rhs)
{
    return lhs < rhs;
}

bool bigToLittle(int lhs, int rhs)
{
    return lhs > rhs;
}

Sorter::Sorter(SortSetting::SortMode mode)
{
    if(mode & SortSetting::sortByStudentSum)
        get1 = studentSum;
    else
        get1 = studentNumber;
    if(mode & SortSetting::studentAsc2Desc)
        cmp1 = bigToLittle;
    else
        cmp1 = littleToBig;

    get2 = studentNumber;
    cmp2 = littleToBig;
}

bool Sorter::operator()(const StudentSumData &lhs, const StudentSumData &rhs)
{
    if(get1(lhs) == get1(rhs))
        return cmp2(get2(lhs), get2(rhs));
    else
        return cmp1(get1(lhs), get1(rhs));
}*/

QString generateHtmlString(QList<SummaryData> dataList, SortSettings mode)
{
    if(mode.useGroupModeFlag)
    {
        std::unordered_map<int,GroupSumData> groupMap;
        for(const auto& i:dataList)
        {
            GroupSumData& curData=groupMap[i.groupNo];
            curData.members.append(StudentSumData{i.name,i.studentNo,i.totalScore,0});
            curData.number=i.groupNo;
            curData.score+=i.totalScore;
        }
        std::vector<GroupSumData> groupList;
        groupList.reserve(groupMap.size());
        for(const auto& pair:groupMap)
        {
            groupList.push_back(pair.second);
        }
        //排序以计算排名
        std::sort(groupList.begin(),groupList.end(),[](const GroupSumData& lhs,const GroupSumData& rhs)
        {
            return lhs.score>rhs.score;
        });
        int curRank=1;
        for(auto& i:groupList)
        {
            i.rank=curRank;
            curRank++;
        }
        return group::generate(groupList, mode.showStudentNumberFlag);
    }
    else
    {
        std::vector<StudentSumData> studentList;
        studentList.reserve(dataList.size());
        for(const auto& i:dataList)
            studentList.push_back({i.name,i.studentNo,i.totalScore,0});
        //排序以计算排名
        std::sort(studentList.begin(),studentList.end(),[](const StudentSumData& lhs,const StudentSumData& rhs)
        {
            return lhs.number>rhs.number;
        });
        int curRank=1;
        for(auto& i:studentList)
        {
            i.rank=curRank;
            curRank++;
        }
        // std::sort(studentList.begin(),studentList.end(),Sorter(mode));
        return student::generate(studentList, mode.showStudentNumberFlag);
    }
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

QString generate(std::vector<StudentSumData> dataList, bool showStudentNumber)
{
    QString tableHead,tableBody;
    if(showStudentNumber)
        tableHead+=HTMLTableHeadStudent;
    tableHead+=HTMLTableHeadBase;
    for(const auto& i:dataList)
    {
        QString tableItem;
        if(showStudentNumber)
            tableItem+=HTMLTableItemStudent.arg(i.number);
        tableItem+=HTMLTableItemBase.arg(i.name).arg(i.score).arg(getSumTextClass(i.score));
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
QString generate(std::vector<GroupSumData> dataList, bool showStudentNumber)
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

    QString HTMLBody,groupContent;
    for(const auto& i:dataList)
    {
        for(const auto& j:i.members)
        {
            groupContent+=HTMLStudentItem.arg(j.name)
                    .arg(j.score)
                    .arg(getSumTextClass(j.score));
        }
        HTMLBody+=HTMLGroupCard.arg(getGroupName(i.number))
                .arg(i.score)
                .arg(getSumTextClass(i.score))
                .arg(i.rank)
                .arg(groupContent);
        groupContent.clear();
    }
    return HTMLTemplate.arg(HTMLBody);
}
} // namespace group
} // namespace anonymous
