#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <QString>
#include <QList>
#include <QDateTime>
#include <QDataStream>

// 文件头部信息(校验用)
struct HeaderData
{
    quint64 fileHeader;       //文件头
    qsizetype studentCount;   //学生数
    qsizetype templateCount;  //积分模板数
    qsizetype recordCount;    //积分记录数
};
// QDataStream 序列化支持
inline QDataStream &operator >>(QDataStream &in, HeaderData &data)
{
    in >> data.fileHeader
       >> data.studentCount
       >> data.templateCount
       >> data.recordCount;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const HeaderData &data)
{
    out << data.fileHeader
        << data.studentCount
        << data.templateCount
        << data.recordCount;
    return out;
}


// 积分求和排序设置
// 排序字段
enum class SortField : quint8
{
    StudentNumber,  // 学生编号
    StudentSum,     // 学生总分
    GroupNumber,    // 小组编号
    GroupSum        // 小组总分
};

// 排序顺序
enum class SortOrder : quint8
{
    Ascending,      // 升序
    Descending      // 降序
};

// 排序规格结构体
struct SortSpec
{
    SortField field;
    SortOrder order;
    // 二级排序约定：学生按学号升序，小组按组号升序（无需存储）
};

// QDataStream 序列化支持
inline QDataStream& operator<<(QDataStream& out, const SortSpec& spec)
{
    out << static_cast<quint8>(spec.field)
        << static_cast<quint8>(spec.order);
    return out;
}

inline QDataStream& operator>>(QDataStream& in, SortSpec& spec) {
    quint8 pf, po;
    in >> pf >> po;
    spec.field = static_cast<SortField>(pf);
    spec.order = static_cast<SortOrder>(po);
    return in;
}

struct SortSettings
{
    SortSettings()
    {
        defaultSettings();
    }

    void defaultSettings()
    {
        showStudentNumberFlag=false;
        useGroupModeFlag=false;
        studentSpec.field=SortField::StudentNumber;
        studentSpec.order=SortOrder::Ascending;
        groupSpec.field=SortField::GroupNumber;
        groupSpec.order=SortOrder::Ascending;
        groupStuSpec.field=SortField::StudentNumber;
        groupStuSpec.order=SortOrder::Ascending;
    }

    bool     showStudentNumberFlag;// 是否显示学生编号
    bool     useGroupModeFlag;     // 是否使用小组模式
    SortSpec studentSpec;          // 学生列表排序规则
    SortSpec groupSpec;            // 小组列表排序规则
    SortSpec groupStuSpec;         // 小组内学生排序规则
};

// QDataStream 序列化支持
inline QDataStream& operator<<(QDataStream& out, const SortSettings& data)
{
    out << data.showStudentNumberFlag
        << data.useGroupModeFlag
        << data.studentSpec
        << data.groupSpec
        << data.groupStuSpec;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, SortSettings& data)
{
    in >> data.showStudentNumberFlag
       >> data.useGroupModeFlag
       >> data.studentSpec
       >> data.groupSpec
       >> data.groupStuSpec;
    return in;
}


//学生信息
struct StudentInfo
{
    QString name;                       //学生姓名
    qint32 displayOrder;                //显示学生信息列表时的顺序
    qint32 groupNumber;                 //学生的小组号
    static constexpr qint32 NoGroup=-1; //未指定小组时为-1
};

// QDataStream 序列化支持
inline QDataStream &operator >>(QDataStream &in, StudentInfo &data)
{
    in >> data.name
       >> data.displayOrder
       >> data.groupNumber;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const StudentInfo &data)
{
    out << data.name
        << data.displayOrder
        << data.groupNumber;
    return out;
}


//积分模板信息
struct ScoreTemplate
{
    QString description;     //对积分模板的描述
    QList<qint32> values;    //对每一个学生（按索引）应用的分数修改值
};
// QDataStream 序列化支持
inline QDataStream &operator >>(QDataStream &in, ScoreTemplate &data)
{
    in >> data.description
       >> data.values;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const ScoreTemplate &data)
{
    out << data.description
        << data.values;
    return out;
}


//积分记录信息
struct ScoreRecord
{
    QDateTime saveTime;      //录入本次记录的时间
    QString description;     //对本次分数统计的描述
    QList<qint32> scores;    //每一个学生（按索引）的分数值
};
// QDataStream 序列化支持
inline QDataStream &operator >>(QDataStream &in, ScoreRecord &data)
{
    in >> data.saveTime
       >> data.description
       >> data.scores;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const ScoreRecord &data)
{
    out << data.saveTime
        << data.description
        << data.scores;
    return out;
}

#endif // DATA_TYPES_H
