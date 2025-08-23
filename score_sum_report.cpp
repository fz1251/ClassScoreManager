#include "scoresumreport.h"


namespace ScoreSumReport {

namespace
{
const QString HTMLTemplate=QStringLiteral(R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>积分求和导出表格</title>
    <style>
    %1
    %2
    </style>
</head>
<body>
    <div class="table-container">
        <h1>积分求和结果</h1>
        %3
    </div>
</body>
</html>
)");

const QString CSSBasic=QStringLiteral(R"(
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
)");

const QString CSSStudent=QStringLiteral(R"(
        .data-table tbody tr {
            transition: all 0.25s ease;
        }

        .data-table tbody tr:hover {
            background-color: #f0f7ff;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(50, 89, 105, 0.1);
        }

        .data-table tbody tr:last-child td {
            border-bottom: none;
        }
)");

const QString CSSGroup=QStringLiteral(R"(
        .group-divider {
            border-right: 2px solid #e1e8ed;
        }

        .data-table th:nth-child(1),
        .data-table td:nth-child(1) {
            width: 15%;
        }

        .data-table th:nth-child(2),
        .data-table td:nth-child(2) {
            width: 15%;
        }

        .ranking-section {
            margin-bottom: 30px;
            background: white;
            border-radius: 12px;
            box-shadow: 0 8px 25px rgba(0, 0, 0, 0.08);
            padding: 25px 30px;
        }

        .ranking-title {
            text-align: center;
            font-size: 1.6rem;
            color: #2c3e50;
            margin-bottom: 20px;
            padding-bottom: 12px;
            border-bottom: 2px solid #e1e8ed;
            position: relative;
        }

        .ranking-title:after {
            content: '';
            position: absolute;
            bottom: -2px;
            left: 50%;
            transform: translateX(-50%);
            width: 144px;
            height: 3px;
            background: linear-gradient(135deg, #6c8fa9 0%, #5a7b95 100%);
            border-radius: 3px;
        }

        .ranking-list {
            counter-reset: ranking;
            list-style: none;
            padding: 0;
        }

        .ranking-list li {
            counter-increment: ranking;
            padding: 16px 20px;
            margin-bottom: 12px;
            background: #f8f9fa;
            border-radius: 8px;
            font-size: 1.3rem;
            display: flex;
            align-items: center;
            transition: all 0.3s ease;
            border-left: 4px solid transparent;
        }

        .ranking-list li:hover {
            transform: translateX(5px);
            background: #f0f7ff;
            border-left: 4px solid #5a7b95;
            box-shadow: 0 4px 10px rgba(0, 0, 0, 0.05);
        }

        .ranking-list li::before {
            content: counter(ranking);
            display: inline-flex;
            align-items: center;
            justify-content: center;
            width: 32px;
            height: 32px;
            background: linear-gradient(135deg, #6c8fa9 0%, #5a7b95 100%);
            color: white;
            border-radius: 50%;
            margin-right: 15px;
            font-weight: 600;
            font-size: 1.1rem;
        }
)");

const QString getSumTextColor(int score)
{
    if(score>0)
        return "positive-sum";
    else if(score<0)
        return "negative-sum";
    else
        return "zero-sum";
}

const QString HTMLListTemplate=QStringLiteral(R"(
        <div class="ranking-section">
            <h2 class="ranking-title">小组积分排名一览</h2>
            <ol class="ranking-list">
            %1
            </ol>
        </div>
)");

const QString HTMLListItem=QStringLiteral(R"(
                <li>%1组(%2分)</li>
)");

const QString HTMLTableTemplate=QStringLiteral(R"(
<table class="data-table">
    <thead>
        <tr>
            %1
        </tr>
    </thead>
    <tbody>
        %2
    </tbody>
</table>
)");

const QString HTMLTableHeadBase=QStringLiteral(R"(
            <th>姓名</th>
            <th>积分</th>
)");

const QString HTMLTableHeadStudent=QStringLiteral(R"(
            <th>编号</th>
)");

const QString HTMLTableHeadGroup=QStringLiteral(R"(
            <th>小组号</th>
            <th>小组总分</th>
)");

const QString HTMLTableRowTemplate=QStringLiteral(R"(
        <tr>
            %1
        </tr>
)");

const QString HTMLTableItemBase=QStringLiteral(R"(
            <td>%1</td>
            <td class="%3">%2</td>
)");

const QString HTMLTableItemStudent=QStringLiteral(R"(
            <td>%1</td>
)");

const QString HTMLTableItemGroup=QStringLiteral(R"(
            <td rowspan="%1" class="group-divider">%2</td>
            <td rowspan="%1" class="%4 group-divider">%3</td>
)");
}

Sorter::Sorter(SortSetting::SortMode mode)
{
    auto setStudentCmp = [mode](getFunc& get_,cmpFunc& cmp_) -> void
    {
        bool sortBySum=mode&SortSetting::sortByStudentSum;
        bool filpSorting=mode&SortSetting::flipStudentSorting;
        if(sortBySum)
            get_=studentSum;
        else
            get_=studentNumber;
        if(sortBySum^filpSorting)
            cmp_=bigToLittle;
        else
            cmp_=littleToBig;
    };
    if(mode&SortSetting::sortAsGroup)
    {
        bool sortBySum=mode&SortSetting::sortByGroupSum;
        bool filpSorting=mode&SortSetting::flipGroupSorting;
        if(sortBySum)
            get1=groupSum;
        else
            get1=groupNumber;
        if(sortBySum^filpSorting)
            cmp1=bigToLittle;
        else
            cmp1=littleToBig;
        setStudentCmp(get2,cmp2);
    }
    else
    {
        setStudentCmp(get1,cmp1);
        get2=studentNumber;
        cmp2=littleToBig;
    }
}

bool Sorter::operator()(const SummaryData &lhs, const SummaryData &rhs)
{
    if(get1(lhs)==get1(rhs))
        return cmp2(get2(lhs),get2(rhs));
    else
        return cmp1(get1(lhs),get1(rhs));
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
    return lhs<rhs;
}

bool bigToLittle(qint32 lhs, qint32 rhs)
{
    return lhs>rhs;
}

bool emptyCmp(qint32 lhs, qint32 rhs)
{
    Q_UNUSED(lhs);
    Q_UNUSED(rhs);
    return true;
}

QString generateHtmlString(QList<SummaryData> dataList, SortSetting::SortMode mode)
{
    auto getGroupName=[](int num) -> QString
    {
        if(num==StudentInfo::NoGroup)
            return QObject::tr("未分组");
        else
            return QString::number(num);
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

    QString tempHTMLBody,tempTableHeadStr,tempTableBodyStr;
    if(mode&SortSetting::sortAsGroup)
        tempTableHeadStr+=HTMLTableHeadGroup;
    if(!(mode&SortSetting::hideStudentNumber))
        tempTableHeadStr+=HTMLTableHeadStudent;
    tempTableHeadStr+=HTMLTableHeadBase;

    if(mode&SortSetting::sortAsGroup)
    {
        if(!(mode&SortSetting::hideGroupPreview))
        {
            QString tempListStr;
            for(auto i:groupRank)
                if(i.second!=StudentInfo::NoGroup)
                    tempListStr+=HTMLListItem.arg(i.second).arg(i.first);
            tempHTMLBody+=HTMLListTemplate.arg(tempListStr);
        }
        int curGroupIndex=INT_MIN;
        for(const SummaryData& i:dataList)
        {
            QString tempTableItemStr;
            if(curGroupIndex!=i.groupNum)
            {
                curGroupIndex=i.groupNum;
                tempTableItemStr+=HTMLTableItemGroup.arg(groupCount[curGroupIndex])
                                                    .arg(getGroupName(curGroupIndex))
                                                    .arg(groupScoreSum[curGroupIndex])
                                                    .arg(getSumTextColor(groupScoreSum[curGroupIndex]));
            }
            if(!(mode&SortSetting::hideStudentNumber))
                tempTableItemStr+=HTMLTableItemStudent.arg(i.studentNum);
            tempTableItemStr+=HTMLTableItemBase.arg(i.name).arg(i.studentScore).arg(getSumTextColor(i.studentScore));

            tempTableBodyStr+=HTMLTableRowTemplate.arg(tempTableItemStr);
        }
    }
    else
    {
        for(const SummaryData& i:dataList)
        {
            QString tempTableItemStr;
            if(!(mode&SortSetting::hideStudentNumber))
                tempTableItemStr+=HTMLTableItemStudent.arg(i.studentNum);
            tempTableItemStr+=HTMLTableItemBase.arg(i.name).arg(i.studentScore).arg(getSumTextColor(i.studentScore));

            tempTableBodyStr+=HTMLTableRowTemplate.arg(tempTableItemStr);
        }

    }
    tempHTMLBody+=HTMLTableTemplate.arg(tempTableHeadStr,tempTableBodyStr);
    return HTMLTemplate.arg(CSSBasic,
                            ((mode&SortSetting::sortAsGroup)?CSSGroup:CSSStudent),
                            tempHTMLBody);
}

} // namespace ScoreSumReport
