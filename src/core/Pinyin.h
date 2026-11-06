#pragma once

#include <QChar>
#include <QString>
#include <QVector>

namespace Pinyin
{
// 查询归一化：兼容分解、去掉组合记号、转小写，只保留字母、数字与汉字
QString normalizeQuery(const QString& text);
}

// 一条文本的读音格自动机：汉字取全部读音/首字母/原字，英文按整词与首字母，数字逐位。
// 匹配时在格上做 NFA 子串可达性判定，不生成读音组合串。
class PinyinMatcher
{
public:
    PinyinMatcher() = default;
    explicit PinyinMatcher(const QString& text);

    bool isEmpty() const;
    // normalizedQuery 必须已经过 Pinyin::normalizeQuery
    bool matches(const QString& normalizedQuery) const;

private:
    struct State
    {
        QChar ch;
        QVector<int> targets; // 吃掉本字符后可走到的状态
    };

    QVector<State> m_states;
};
