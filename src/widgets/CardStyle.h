#pragma once

#include <QColor>

//小组管理页卡片共用的尺寸与配色
namespace CardStyle
{
constexpr int studentCardHeight = 34;
constexpr int studentCardRadius = 17;
constexpr int studentCardMinWidth = 64;
constexpr int studentCardMaxWidth = 150;
constexpr int studentCardTextPadding = 14;
constexpr int hoverMargin = 4; //卡片四周的留白，供悬停放大与描边使用

inline QColor mix(const QColor& from, const QColor& to, qreal progress)
{
    const qreal red = from.redF() + (to.redF() - from.redF()) * progress;
    const qreal green = from.greenF() + (to.greenF() - from.greenF()) * progress;
    const qreal blue = from.blueF() + (to.blueF() - from.blueF()) * progress;
    const qreal alpha = from.alphaF() + (to.alphaF() - from.alphaF()) * progress;
    return QColor::fromRgbF(red, green, blue, alpha);
}

//按组号取组色：同一组号恒定同色，组号相邻的小组色相相距最远
QColor groupColor(qint32 groupNumber);
}
