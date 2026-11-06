#include "Pinyin.h"

#include <QFile>
#include <QHash>
#include <QList>
#include <QString>

namespace
{
struct PinyinTable
{
    QHash<ushort, QVector<QString>> readings;
    bool loaded = false;
};

PinyinTable& pinyinTable()
{
    static PinyinTable instance;
    if(instance.loaded)
        return instance;
    instance.loaded = true;

    QFile file(QStringLiteral(":/pinyin/pinyin.txt"));
    if(!file.open(QIODevice::ReadOnly))
        return instance;

    // 字典每行“码点(十六进制) 音节[,音节]”，注释与空行跳过
    const QList<QByteArray> lines = file.readAll().split('\n');
    instance.readings.reserve(lines.size());
    for(const QByteArray& rawLine : lines)
    {
        const QByteArray line = rawLine.trimmed();
        if(line.isEmpty() || line.startsWith('#'))
            continue;
        const int spaceIndex = line.indexOf(' ');
        if(spaceIndex <= 0)
            continue;
        bool ok = false;
        const uint codePoint = line.left(spaceIndex).toUInt(&ok, 16);
        if(!ok || codePoint > 0xFFFF)
            continue;
        QVector<QString> syllables;
        for(const QByteArray& rawSyllable : line.mid(spaceIndex + 1).split(','))
        {
            const QString syllable = QString::fromLatin1(rawSyllable).trimmed();
            if(!syllable.isEmpty())
                syllables.append(syllable);
        }
        if(!syllables.isEmpty())
            instance.readings.insert(static_cast<ushort>(codePoint), syllables);
    }
    return instance;
}

bool isHanCodePoint(uint codePoint)
{
    return (codePoint >= 0x3400 && codePoint <= 0x4DBF) || (codePoint >= 0x4E00 && codePoint <= 0x9FFF) ||
           (codePoint >= 0xF900 && codePoint <= 0xFAFF) || (codePoint >= 0x20000 && codePoint <= 0x2FA1F);
}

void appendCodePoint(QString& text, uint codePoint)
{
    if(codePoint <= 0xFFFF)
        text.append(QChar(static_cast<ushort>(codePoint)));
    else
        text.append(QString::fromUcs4(&codePoint, 1));
}

QString codePointString(uint codePoint)
{
    QString text;
    appendCodePoint(text, codePoint);
    return text;
}

// 统一文本与查询的形态：兼容分解、去组合记号（声调、变音符）、小写
QString normalizeText(const QString& text)
{
    const QString decomposed = text.normalized(QString::NormalizationForm_KD).toLower();
    QString result;
    result.reserve(decomposed.size());
    for(uint codePoint : decomposed.toUcs4())
        if(QChar::combiningClass(codePoint) == 0)
            appendCodePoint(result, codePoint);
    return result;
}

void appendUnique(QVector<QString>& values, const QString& value)
{
    if(!value.isEmpty() && !values.contains(value))
        values.append(value);
}

// 把文本切成“字/词/数字”的写法集合：汉字有读音、首字母与原字，英文有整词与首字母，数字逐位
QVector<QVector<QString>> tokenize(const QString& text)
{
    const QVector<uint> codePoints = normalizeText(text).toUcs4();
    QVector<QVector<QString>> tokens;
    int index = 0;
    while(index < codePoints.size())
    {
        const uint codePoint = codePoints.at(index);
        if(isHanCodePoint(codePoint))
        {
            QVector<QString> alternatives;
            if(codePoint <= 0xFFFF)
            {
                const auto it = pinyinTable().readings.constFind(static_cast<ushort>(codePoint));
                if(it != pinyinTable().readings.constEnd())
                {
                    for(const QString& reading : it.value())
                    {
                        appendUnique(alternatives, reading);
                        if(reading.contains(QChar('v')))
                            appendUnique(alternatives, QString(reading).replace(QChar('v'), QChar('u')));
                        appendUnique(alternatives, reading.left(1));
                    }
                }
            }
            // 字典未收录的字仍可用原字匹配
            appendUnique(alternatives, codePointString(codePoint));
            tokens.append(alternatives);
            ++index;
        }
        else if(QChar::isLetter(codePoint))
        {
            QString word;
            while(index < codePoints.size())
            {
                const uint wordCodePoint = codePoints.at(index);
                if(isHanCodePoint(wordCodePoint) || !QChar::isLetter(wordCodePoint))
                    break;
                appendCodePoint(word, wordCodePoint);
                ++index;
            }
            if(!word.isEmpty())
            {
                QVector<QString> alternatives;
                appendUnique(alternatives, word);
                appendUnique(alternatives, word.left(1));
                tokens.append(alternatives);
            }
        }
        else if(QChar::isNumber(codePoint))
        {
            QVector<QString> alternatives;
            alternatives.append(codePointString(codePoint));
            tokens.append(alternatives);
            ++index;
        }
        else
        {
            ++index;
        }
    }
    return tokens;
}
} // namespace

QString Pinyin::normalizeQuery(const QString& text)
{
    const QString normalized = normalizeText(text);
    QString result;
    result.reserve(normalized.size());
    for(uint codePoint : normalized.toUcs4())
        if(QChar::isLetterOrNumber(codePoint))
            appendCodePoint(result, codePoint);
    return result;
}

PinyinMatcher::PinyinMatcher(const QString& text)
{
    const QVector<QVector<QString>> tokens = tokenize(text);
    if(tokens.isEmpty())
        return;

    QVector<QVector<QVector<int>>> stateIds(tokens.size());
    QVector<QVector<int>> firstStates(tokens.size());
    for(int tokenIndex = 0; tokenIndex < tokens.size(); ++tokenIndex)
    {
        const QVector<QString>& alternatives = tokens.at(tokenIndex);
        stateIds[tokenIndex].resize(alternatives.size());
        firstStates[tokenIndex].resize(alternatives.size());
        for(int alternativeIndex = 0; alternativeIndex < alternatives.size(); ++alternativeIndex)
        {
            const QString& alternative = alternatives.at(alternativeIndex);
            stateIds[tokenIndex][alternativeIndex].resize(alternative.size());
            for(int charIndex = 0; charIndex < alternative.size(); ++charIndex)
            {
                State state;
                state.ch = alternative.at(charIndex);
                const int stateId = m_states.size();
                m_states.append(state);
                stateIds[tokenIndex][alternativeIndex][charIndex] = stateId;
                if(charIndex == 0)
                    firstStates[tokenIndex][alternativeIndex] = stateId;
                else
                    m_states[stateIds[tokenIndex][alternativeIndex][charIndex - 1]].targets.append(stateId);
            }
        }
    }

    // 一种写法结束后，ε 转移到下一个字/词的所有写法；最后一个 token 不再转移
    for(int tokenIndex = 0; tokenIndex + 1 < tokens.size(); ++tokenIndex)
    {
        for(int alternativeIndex = 0; alternativeIndex < tokens.at(tokenIndex).size(); ++alternativeIndex)
        {
            const QVector<int>& ids = stateIds.at(tokenIndex).at(alternativeIndex);
            if(!ids.isEmpty())
                m_states[ids.last()].targets += firstStates.at(tokenIndex + 1);
        }
    }
}

bool PinyinMatcher::isEmpty() const
{
    return m_states.isEmpty();
}

bool PinyinMatcher::matches(const QString& normalizedQuery) const
{
    if(normalizedQuery.isEmpty())
        return true;
    if(m_states.isEmpty())
        return false;

    // 任意状态都可以作为子串起点；每读入一个字符，只在字符相等的状态上继续
    QVector<int> active;
    active.reserve(m_states.size());
    for(int i = 0; i < m_states.size(); ++i)
        active.append(i);

    QVector<int> marks(m_states.size(), 0);
    int generation = 0;
    for(int queryIndex = 0; queryIndex < normalizedQuery.size(); ++queryIndex)
    {
        const QChar ch = normalizedQuery.at(queryIndex);
        ++generation;
        QVector<int> next;
        next.reserve(active.size());
        bool matched = false;
        for(int stateIndex : active)
        {
            const State& state = m_states.at(stateIndex);
            if(state.ch != ch)
                continue;
            matched = true;
            for(int target : state.targets)
            {
                if(marks.at(target) == generation)
                    continue;
                marks[target] = generation;
                next.append(target);
            }
        }
        if(!matched)
            return false;
        // 查询可以在音节中间结束；最后一个字符命中即成功
        if(queryIndex + 1 == normalizedQuery.size())
            return true;
        active.swap(next);
        if(active.isEmpty())
            return false;
    }
    return true;
}
