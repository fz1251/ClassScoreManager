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
    int number,groupOrder,score,rank;
};

struct GroupSumData
{
    std::vector<StudentSumData> members;
    int number,score,rank;
};

// 次序依据：个人模式用学号，小组模式用小组中的编号
int orderKey(const StudentSumData& student, bool byGroupOrder)
{
    return byGroupOrder ? student.groupOrder : student.number;
}

// 学生比较器：先按排序字段，同值时统一按次序依据升序收尾
auto getStudentComparator(const SortSpec& spec, bool byGroupOrder)
{
    return [spec, byGroupOrder](const StudentSumData& a, const StudentSumData& b)
    {
        if (spec.field == SortField::StudentNumber)
        {
            return spec.order == SortOrder::Ascending
                       ? orderKey(a, byGroupOrder) < orderKey(b, byGroupOrder)
                       : orderKey(a, byGroupOrder) > orderKey(b, byGroupOrder);
        }
        if (a.score != b.score)
            return spec.order == SortOrder::Ascending ? a.score < b.score : a.score > b.score;
        return orderKey(a, byGroupOrder) < orderKey(b, byGroupOrder);
    };
}

// 小组比较器：先按排序字段，同值时统一按组号升序收尾
auto getGroupComparator(const SortSpec& spec)
{
    return [spec](const GroupSumData& a, const GroupSumData& b)
    {
        if (spec.field == SortField::GroupNumber)
        {
            return spec.order == SortOrder::Ascending
                       ? a.number < b.number
                       : a.number > b.number;
        }
        if (a.score != b.score)
            return spec.order == SortOrder::Ascending ? a.score < b.score : a.score > b.score;
        return a.number < b.number;
    };
}

void sortStudents(std::vector<StudentSumData>& students, SortSpec spec, bool byGroupOrder = false)
{
    // 排名按总分降序，同分时按次序依据定序
    std::sort(students.begin(), students.end(),
              [byGroupOrder](const StudentSumData& a, const StudentSumData& b)
    {
        if (a.score != b.score)
            return a.score > b.score;
        return orderKey(a, byGroupOrder) < orderKey(b, byGroupOrder);
    });
    int curRank=1;
    for(auto& i:students)
    {
        i.rank=curRank;
        curRank++;
    }
    // 按照传入的排序规格进行排序
    std::sort(students.begin(), students.end(), getStudentComparator(spec, byGroupOrder));
}

void sortGroups(std::vector<GroupSumData>& groups, SortSpec groupSpec, SortSpec stuSpec)
{
    // 组内学生按小组中的编号定次序
    for(auto& i:groups)
    {
        sortStudents(i.members, stuSpec, true);
    }
    // 排名按小组总分降序，同分时按组号定序
    std::sort(groups.begin(), groups.end(),
              [](const GroupSumData& a, const GroupSumData& b)
    {
        if (a.score != b.score)
            return a.score > b.score;
        return a.number < b.number;
    });
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
            curData.members.push_back(StudentSumData{i.name,i.studentNo,i.groupOrder,i.totalScore,0});
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
            studentList.push_back({i.name,i.studentNo,i.groupOrder,i.totalScore,0});
        sortStudents(studentList, mode.studentSpec);
        return student::generate(studentList, mode.showStudentNumberFlag);
    }
}
} // namespace ScoreSumReport
namespace
{
namespace student
{
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
      margin: 0;
      padding: 0;
    }

    body {
      font-family: system-ui, "Microsoft YaHei", "PingFang SC", sans-serif;
      background: #e8ecf1;
      padding: 20px;
      color: #333;
      line-height: 1.2;
    }

    .page-title {
      font-size: 1.5rem;
      font-weight: 700;
      color: #1a1a2e;
      margin-bottom: 14px;
    }

    .students-grid {
      display: flex;
      flex-wrap: wrap;
      gap: 8px;
    }

    .student-item {
      flex: 0 0 200px;
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 10px 14px;
      background: #f3f5f7;
      border: 1px solid #e8eaef;
      border-radius: 6px;
      box-shadow: 0 1px 3px rgba(0,0,0,0.04);
    }

    .student-no {
      font-size: 1rem;
      font-weight: 600;
      color: #2563b0;
      font-variant-numeric: tabular-nums;
      flex-shrink: 0;
    }

    .student-name {
      font-size: 1.2rem;
      font-weight: 600;
      color: #1e293b;
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
      flex: 1;
    }

    .score-area {
      display: flex;
      align-items: baseline;
      gap: 3px;
      flex-shrink: 0;
    }

    .student-score {
      font-size: 1.55rem;
      font-weight: 800;
      white-space: nowrap;
      font-variant-numeric: tabular-nums;
      line-height: 1;
    }

    .score-unit {
      font-size: 0.95rem;
      color: #94a3b8;
      font-weight: 400;
    }

    .positive-number { color: #16a34a !important; }
    .negative-number { color: #dc2626 !important; }
    .zero-number     { color: #9ca3af !important; }
  </style>
</head>
<body>
  <div class="page-title">积分求和结果</div>
  <div class="students-grid">
    %1
  </div>
</body>
</html>
)");
const QString HTMLStudentItem=QStringLiteral(R"(
<div class="student-item">
%4
  <div class="student-name">%1</div>
  <div class="score-area">
    <span class="student-score %3">%2</span>
    <span class="score-unit">分</span>
  </div>
</div>
)");
const QString HTMLStudentNumber=QStringLiteral(R"(
<span class="student-no">%1</span>
)");
QString generate(std::vector<StudentSumData> dataList, bool showStudentNumber)
{
    auto getSumTextClass=[](int num) -> QString
    {
        if(num>0)
            return "positive-number";
        else if(num<0)
            return "negative-number";
        else
            return "zero-number";
    };

    QString bodyContent;
    for(const auto& i:dataList)
    {
        bodyContent+=HTMLStudentItem.arg(i.name)
                .arg(i.score)
                .arg(getSumTextClass(i.score))
                .arg(showStudentNumber?HTMLStudentNumber.arg(i.number):"");
    }
    return HTMLTemplate.arg(bodyContent);
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
