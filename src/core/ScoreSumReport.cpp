#include "ScoreSumReport.h"

#include <QObject>
#include <QStringList>
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <vector>

// 文件内部实现
namespace
{
using namespace ScoreSumReport;

struct StudentSumData
{
    QString name;
    qint32 number, groupOrder, score;
    int rank;
};

struct GroupSumData
{
    std::vector<StudentSumData> members;
    qint32 number, score;
    int rank;
};

// 次序依据：个人模式用学号；小组模式用组内编号，未分组学生没有组内编号，退回学号
qint32 orderKey(const StudentSumData& student, bool byGroupOrder)
{
    if(byGroupOrder && student.groupOrder >= 0)
        return student.groupOrder;
    return student.number;
}

// 学生比较器：先按排序字段，同值时统一按次序依据升序收尾
auto getStudentComparator(const SortSpec& spec, bool byGroupOrder)
{
    return [spec, byGroupOrder](const StudentSumData& a, const StudentSumData& b)
    {
        if(spec.field == SortField::StudentNumber)
        {
            return spec.order == SortOrder::Ascending ? orderKey(a, byGroupOrder) < orderKey(b, byGroupOrder)
                                                      : orderKey(a, byGroupOrder) > orderKey(b, byGroupOrder);
        }
        if(a.score != b.score)
            return spec.order == SortOrder::Ascending ? a.score < b.score : a.score > b.score;
        return orderKey(a, byGroupOrder) < orderKey(b, byGroupOrder);
    };
}

// 小组比较器：先按排序字段，同值时统一按组号升序收尾
auto getGroupComparator(const SortSpec& spec)
{
    return [spec](const GroupSumData& a, const GroupSumData& b)
    {
        if(spec.field == SortField::GroupNumber)
            return spec.order == SortOrder::Ascending ? a.number < b.number : a.number > b.number;
        if(a.score != b.score)
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
                  if(a.score != b.score)
                      return a.score > b.score;
                  return orderKey(a, byGroupOrder) < orderKey(b, byGroupOrder);
              });
    int curRank = 1;
    for(auto& studentData : students)
        studentData.rank = curRank++;
    // 按照传入的排序规格进行排序
    std::sort(students.begin(), students.end(), getStudentComparator(spec, byGroupOrder));
}

void sortGroups(std::vector<GroupSumData>& groups, SortSpec groupSpec, SortSpec stuSpec)
{
    // 组内学生按小组中的编号定次序
    for(auto& groupData : groups)
        sortStudents(groupData.members, stuSpec, true);
    // 排名按小组总分降序，同分时按组号定序
    std::sort(groups.begin(), groups.end(),
              [](const GroupSumData& a, const GroupSumData& b)
              {
                  if(a.score != b.score)
                      return a.score > b.score;
                  return a.number < b.number;
              });
    int curRank = 1;
    for(auto& groupData : groups)
        groupData.rank = curRank++;
    // 按照传入的排序规格进行排序
    std::sort(groups.begin(), groups.end(), getGroupComparator(groupSpec));
}

// 去掉模板源码中只为人眼阅读的缩进与空行
QString stripIndent(const QString& html)
{
    QStringList lines = html.split(QLatin1Char('\n'));
    for(QString& line : lines)
        line = line.trimmed();
    lines.removeAll(QString());
    return lines.join(QLatin1Char('\n'));
}

namespace student
{
const QString HTMLTemplate = stripIndent(QStringLiteral(R"(
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

    .positive-number { color: #15803d !important; }
    .negative-number { color: #dc2626 !important; }
    .zero-number     { color: #6b7280 !important; }
  </style>
</head>
<body>
  <div class="page-title">积分求和结果</div>
  <div class="students-grid">
    %1
  </div>
</body>
</html>
)"));
const QString HTMLStudentItem = stripIndent(QStringLiteral(R"(
<div class="student-item">
%4
  <div class="student-name">%1</div>
  <div class="score-area">
    <span class="student-score %3">%2</span>
    <span class="score-unit">分</span>
  </div>
</div>
)"));
const QString HTMLStudentNumber = stripIndent(QStringLiteral(R"(
<span class="student-no">%1</span>
)"));
QString generate(std::vector<StudentSumData> dataList, bool showStudentNumber)
{
    auto getSumTextClass = [](qint32 num) -> QString
    {
        if(num > 0)
            return "positive-number";
        if(num < 0)
            return "negative-number";
        return "zero-number";
    };

    QString bodyContent;
    for(const auto& studentData : dataList)
    {
        bodyContent += HTMLStudentItem.arg(studentData.name.toHtmlEscaped())
                           .arg(studentData.score)
                           .arg(getSumTextClass(studentData.score))
                           .arg(showStudentNumber ? HTMLStudentNumber.arg(studentData.number) : "");
    }
    return HTMLTemplate.arg(bodyContent);
}
} // namespace student

namespace group
{
const QString HTMLTemplate = stripIndent(QStringLiteral(R"(
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
      font-family: system-ui, "Microsoft YaHei", "PingFang SC", sans-serif;
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

    .positive-number { color: #15803d !important; }
    .negative-number { color: #dc2626 !important; }
    .zero-number     { color: #6b7280 !important; }
  </style>
</head>
<body>
  <div class="groups-row">
    %1
  </div>
</body>
</html>
)"));
const QString HTMLGroupCard = stripIndent(QStringLiteral(R"(
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
)"));
const QString HTMLStudentItem = stripIndent(QStringLiteral(R"(
<div class="stat-item">
%4
  <div class="stat-head">%1</div>
  <div class="stat-content">
    <div class="stat-number %3">%2</div>
    <div class="stat-unit">分</div>
  </div>
</div>
)"));
const QString HTMLStudentNumber = stripIndent(QStringLiteral(R"(
<span class="student-no">%1</span>
)"));
QString generate(std::vector<GroupSumData> dataList, bool showStudentNumber)
{
    auto getGroupName = [](qint32 num) -> QString
    {
        if(num == StudentInfo::NoGroup)
            return QObject::tr("未分组");
        return QObject::tr("%1组").arg(num);
    };

    auto getSumTextClass = [](qint32 num) -> QString
    {
        if(num > 0)
            return "positive-number";
        if(num < 0)
            return "negative-number";
        return "zero-number";
    };

    QString HTMLBody, groupContent;
    for(const auto& groupData : dataList)
    {
        for(const auto& member : groupData.members)
        {
            groupContent += HTMLStudentItem.arg(member.name.toHtmlEscaped())
                                .arg(member.score)
                                .arg(getSumTextClass(member.score))
                                .arg(showStudentNumber ? HTMLStudentNumber.arg(member.number) : "");
        }
        HTMLBody += HTMLGroupCard.arg(getGroupName(groupData.number))
                        .arg(groupData.score)
                        .arg(getSumTextClass(groupData.score))
                        .arg(groupData.rank)
                        .arg(groupContent);
        groupContent.clear();
    }
    return HTMLTemplate.arg(HTMLBody);
}
} // namespace group
} // namespace

namespace ScoreSumReport
{
QString generateHtmlString(const QVector<SummaryData>& dataList, const SortSettings& mode)
{
    if(mode.useGroupModeFlag)
    {
        std::unordered_map<qint32, GroupSumData> groupMap;
        for(const auto& stu : dataList)
        {
            GroupSumData& curData = groupMap[stu.groupNo];
            curData.members.push_back(StudentSumData{stu.name, stu.studentNo, stu.groupOrder, stu.totalScore, 0});
            curData.number = stu.groupNo;
            curData.score += stu.totalScore;
        }
        std::vector<GroupSumData> groupList;
        groupList.reserve(groupMap.size());
        for(auto& pair : groupMap)
            groupList.push_back(std::move(pair.second));
        sortGroups(groupList, mode.groupSpec, mode.groupStuSpec);
        return group::generate(groupList, mode.showStudentNumberFlag);
    }
    else
    {
        std::vector<StudentSumData> studentList;
        studentList.reserve(dataList.size());
        for(const auto& stu : dataList)
            studentList.push_back({stu.name, stu.studentNo, stu.groupOrder, stu.totalScore, 0});
        sortStudents(studentList, mode.studentSpec);
        return student::generate(studentList, mode.showStudentNumberFlag);
    }
}
} // namespace ScoreSumReport
