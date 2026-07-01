#include "ScoreSumReport.h"
#include <vector>
#include <unordered_map>
#include <algorithm>

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
    std::vector<StudentSumData> members;
    int number,score,rank;
};

// 学生比较器
bool studentNumberAsc(const StudentSumData& a, const StudentSumData& b) {
    return a.number < b.number;
}

bool studentNumberDesc(const StudentSumData& a, const StudentSumData& b) {
    return a.number > b.number;
}

bool studentSumAsc(const StudentSumData& a, const StudentSumData& b) {
    if (a.score != b.score) return a.score < b.score;
    return a.number < b.number;   // 总分相同时按学号升序
}

bool studentSumDesc(const StudentSumData& a, const StudentSumData& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.number < b.number;
}

// 小组比较器
bool groupNumberAsc(const GroupSumData& a, const GroupSumData& b) {
    return a.number < b.number;
}

bool groupNumberDesc(const GroupSumData& a, const GroupSumData& b) {
    return a.number > b.number;
}

bool groupSumAsc(const GroupSumData& a, const GroupSumData& b) {
    if (a.score != b.score) return a.score < b.score;
    return a.number < b.number;
}

bool groupSumDesc(const GroupSumData& a, const GroupSumData& b) {
    if (a.score != b.score) return a.score > b.score;
    return a.number < b.number;
}

auto getStudentComparator(const SortSpec& spec) {
    if (spec.field == SortField::StudentNumber) {
        return spec.order == SortOrder::Ascending ? studentNumberAsc : studentNumberDesc;
    } else {
        return spec.order == SortOrder::Ascending ? studentSumAsc : studentSumDesc;
    }
}

auto getGroupComparator(const SortSpec& spec) {
    if (spec.field == SortField::GroupNumber) {
        return spec.order == SortOrder::Ascending ? groupNumberAsc : groupNumberDesc;
    } else {
        return spec.order == SortOrder::Ascending ? groupSumAsc : groupSumDesc;
    }
}

void sortStudents(std::vector<StudentSumData>& students, SortSpec spec)
{
    // 排序以计算排名
    std::sort(students.begin(),students.end(),studentSumDesc);
    int curRank=1;
    for(auto& i:students)
    {
        i.rank=curRank;
        curRank++;
    }
    // 按照传入的排序规格进行排序
    std::sort(students.begin(), students.end(), getStudentComparator(spec));
}

void sortGroups(std::vector<GroupSumData>& groups, SortSpec groupSpec, SortSpec stuSpec)
{
    // 对每个小组中的学生进行排序
    for(auto& i:groups)
    {
        sortStudents(i.members, stuSpec);
    }
    // 排序以计算排名
    std::sort(groups.begin(),groups.end(),groupSumDesc);
    int curRank=1;
    for(auto& i:groups)
    {
        i.rank=curRank;
        curRank++;
    }
    // 按照传入的排序规格进行排序
    std::sort(groups.begin(), groups.end(), getGroupComparator(groupSpec));
}

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
QString generateHtmlString(QVector<SummaryData> dataList, SortSettings mode)
{
    if(mode.useGroupModeFlag)
    {
        std::unordered_map<int,GroupSumData> groupMap;
        for(const auto& i:dataList)
        {
            GroupSumData& curData=groupMap[i.groupNo];
            curData.members.push_back(StudentSumData{i.name,i.studentNo,i.totalScore,0});
            curData.number=i.groupNo;
            curData.score+=i.totalScore;
        }
        std::vector<GroupSumData> groupList;
        groupList.reserve(groupMap.size());
        for(const auto& pair:groupMap)
        {
            groupList.push_back(pair.second);
        }
        sortGroups(groupList, mode.groupSpec, mode.groupStuSpec);
        return group::generate(groupList, mode.showStudentNumberFlag);
    }
    else
    {
        std::vector<StudentSumData> studentList;
        studentList.reserve(dataList.size());
        for(const auto& i:dataList)
            studentList.push_back({i.name,i.studentNo,i.totalScore,0});
        sortStudents(studentList, mode.studentSpec);
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
      background: #e8ecf1;
      margin: 20px;
      color: #333;
      line-height: 1.2;
    }

    .groups-row {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      width: 100%;
    }

    .group-card {
      background: #f3f5f7;
      border-radius: 0;
      box-shadow: 0 1px 3px rgba(0,0,0,0.04);
      padding: 12px;
      flex: 1 1 auto;
      min-width: 300px;
      display: flex;
      flex-direction: column;
    }

    .card-header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      border-bottom: 1px solid #e2e6ea;
      padding-bottom: 6px;
      margin-bottom: 8px;
      flex-shrink: 0;
    }

    .header-left {
      display: flex;
      align-items: center;
      gap: 12px;
    }

    .group-rank-badge {
      font-size: 1.4rem;
      font-weight: 700;
      color: #3498db;
      background: #ebf5fb;
      border: 1.5px solid #c5ddf5;
      padding: 8px 18px;
      border-radius: 50px;
      line-height: 1.2;
      white-space: nowrap;
      flex-shrink: 0;
    }

    .group-name {
      font-size: 1.5rem;
      color: #1a1a2e;
      font-weight: 700;
      white-space: nowrap;
    }

    .group-summary {
      font-size: 1.05rem;
      color: #6b7280;
      font-weight: 500;
      white-space: nowrap;
    }

    .card-body {
      flex: 1;
      min-height: 0;
    }

    .stats-grid {
      display: flex;
      flex-wrap: wrap;
      gap: 4px;
    }

    .stat-item {
      display: flex;
      align-items: center;
      flex: 1 0 auto;
      padding: 8px 10px;
      background: rgba(255,255,255,0.7);
      border: 1px solid #e8eaef;
      border-radius: 0;
      gap: 8px;
    }

    .student-no {
      font-size: 1rem;
      font-weight: 600;
      color: #2563b0;
      font-variant-numeric: tabular-nums;
      flex-shrink: 0;
    }

    .stat-head {
      font-size: 1.2rem;
      color: #1e293b;
      font-weight: 600;
      white-space: nowrap;
    }

    .stat-content {
      display: flex;
      align-items: baseline;
      gap: 4px;
      flex-shrink: 0;
    }

    .stat-number {
      font-size: 1.55rem;
      font-weight: 800;
      white-space: nowrap;
      font-variant-numeric: tabular-nums;
      line-height: 1;
    }

    .group-total {
      font-size: 1.8rem;
      font-weight: 800;
      white-space: nowrap;
      font-variant-numeric: tabular-nums;
      line-height: 1;
    }

    .stat-unit {
      font-size: 0.95rem;
      color: #94a3b8;
      font-weight: 400;
      flex-shrink: 0;
    }

    .positive-number { color: #16a34a !important; }
    .negative-number { color: #dc2626 !important; }
    .zero-number     { color: #9ca3af !important; }
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
      <div class="group-rank-badge">No.%4</div>
      <div class="group-name">%1</div>
    </div>
    <div class="group-summary">总分 <span class="group-total %3">%2</span></div>
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
%4
  <div class="stat-head">%1</div>
  <div class="stat-content">
    <div class="stat-number %3">%2</div>
    <div class="stat-unit">分</div>
  </div>
</div>
)");
const QString HTMLStudentNumber=QStringLiteral(R"(
<span class="student-no">%1</span>
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
                    .arg(getSumTextClass(j.score))
                    .arg(showStudentNumber?HTMLStudentNumber.arg(j.number):"");
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
