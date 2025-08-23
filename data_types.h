#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QList>
#include <QDateTime>
#include <QDataStream>

//文件头部信息(校验用)
struct HeaderData
{
    quint64 fileHeader;           //文件头
    qsizetype studentCount;       //学生数
    qsizetype templateCount;      //积分模板数
    qsizetype recordCount;        //积分记录数
};
//序列化/反序列化
inline QDataStream &operator >>(QDataStream &in, HeaderData &obj)
{
    in>>obj.fileHeader>>obj.studentCount>>obj.templateCount>>obj.recordCount;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const HeaderData &obj)
{
    out<<obj.fileHeader<<obj.studentCount<<obj.templateCount<<obj.recordCount;
    return out;
}


//积分求和排序设置
struct SortSetting
{
    enum SortMode : quint8
    {
        hideStudentNumber     = 1<<0,    //隐藏学生编号(默认显示)
        sortByStudentSum      = 1<<1,    //按学生总分排序(默认按学生编号)
        flipStudentSorting    = 1<<2,       //反转学生信息排序顺序(默认编号升序，总分降序)
        sortAsGroup           = 1<<3,    //进行小组积分总和显示(默认按学生个人显示)
        hideGroupPreview      = 1<<4,    //隐藏小组积分排行榜(默认显示)
        sortByGroupSum        = 1<<5,    //按小组总分排序(默认按小组编号)
        flipGroupSorting      = 1<<6,       //反转小组信息排序顺序(默认组号升序，总分降序)
    };

    /* student:以学生为单位排序
     * group:以小组为单位排序
     * default:默认排序设置
     * previous:上一次使用的排序设置
     */
    quint8 studentDefault;
    quint8 studentPrevious;
    quint8 groupDefault;
    quint8 groupPrevious;

    SortSetting()
    {
        defaultSettings();
    }

    void defaultSettings()
    {
        studentPrevious=studentDefault=0;
        groupPrevious=groupDefault=sortAsGroup;
    }
};
//序列化/反序列化
inline QDataStream &operator >>(QDataStream &in, SortSetting &obj)
{
    in>>obj.studentDefault>>obj.studentPrevious>>obj.groupDefault>>obj.groupPrevious;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const SortSetting &obj)
{
    out<<obj.studentDefault<<obj.studentPrevious<<obj.groupDefault<<obj.groupPrevious;
    return out;
}


//学生信息
struct StudentInfo
{
    QString name;                       //学生姓名
    qint32 displayOrder;                //显示学生信息列表时的顺序
    qint32 groupNumber;                 //学生的小组号
    static constexpr qint32 NoGroup=-1; //未指定小组时为-1
};

//序列化/反序列化
inline QDataStream &operator >>(QDataStream &in, StudentInfo &obj)
{
    in>>obj.name>>obj.displayOrder>>obj.groupNumber;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const StudentInfo &obj)
{
    out<<obj.name<<obj.displayOrder<<obj.groupNumber;
    return out;
}


//积分模板信息
struct ScoreTemplate
{
    QString description;     //对积分模板的描述
    QList<qint32> values;    //对每一个学生（按索引）应用的分数修改值
};
//序列化/反序列化
inline QDataStream &operator >>(QDataStream &in, ScoreTemplate &obj)
{
    in>>obj.description>>obj.values;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const ScoreTemplate &obj)
{
    out<<obj.description<<obj.values;
    return out;
}


//积分记录信息
struct ScoreRecord
{
    QDateTime saveTime;      //录入本次记录的时间
    QString description;     //对本次分数统计的描述
    QList<qint32> scores;    //每一个学生（按索引）的分数值
};
//序列化/反序列化
inline QDataStream &operator >>(QDataStream &in, ScoreRecord &obj)
{
    in>>obj.saveTime>>obj.description>>obj.scores;
    return in;
}

inline QDataStream &operator <<(QDataStream &out, const ScoreRecord &obj)
{
    out<<obj.saveTime<<obj.description<<obj.scores;
    return out;
}

#endif // DATATYPES_H
