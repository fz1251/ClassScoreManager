#include "CardStyle.h"

#include <QtMath>
#include <cmath>

namespace
{
//组色在 OKLCH 上的取点参数
constexpr qreal kLightness = 0.87;                 //明度取真实马卡龙色值的统计中位
constexpr qreal kChromaMax = 0.085;                //彩度上限，蓝紫区再按色域余量压低
constexpr qreal kFirstHue = 231.5;                 //第 1 组的色相：浅蓝
constexpr qreal kGoldenAngle = 137.50776405003785; //360°/φ²：相邻组色相相距最远，任意组数都不重复
constexpr int kGamutSearchSteps = 16;

qreal srgbEncode(qreal value)
{
    return value <= 0.0031308 ? 12.92 * value : 1.055 * qPow(value, 1.0 / 2.4) - 0.055;
}

//OKLCH 转线性 sRGB，越界值原样输出供色域判断；变换矩阵取自 Björn Ottosson 的 Oklab 定义
void toLinearRgb(qreal lightness, qreal chroma, qreal hueDegrees, qreal rgb[3])
{
    const qreal a = chroma * qCos(qDegreesToRadians(hueDegrees));
    const qreal b = chroma * qSin(qDegreesToRadians(hueDegrees));
    const qreal l = lightness + 0.3963377774 * a + 0.2158037573 * b;
    const qreal m = lightness - 0.1055613458 * a - 0.0638541728 * b;
    const qreal s = lightness - 0.0894841775 * a - 1.2914855480 * b;
    const qreal lCube = l * l * l;
    const qreal mCube = m * m * m;
    const qreal sCube = s * s * s;
    rgb[0] = 4.0767416621 * lCube - 3.3077115913 * mCube + 0.2309699292 * sCube;
    rgb[1] = -1.2684380046 * lCube + 2.6097574011 * mCube - 0.3413193965 * sCube;
    rgb[2] = -0.0041960863 * lCube - 0.7034186147 * mCube + 1.7076147010 * sCube;
}

bool inSrgbGamut(const qreal rgb[3])
{
    for(int i = 0; i < 3; i++)
        if(rgb[i] < 0.0 || rgb[i] > 1.0)
            return false;
    return true;
}

//该色相在 sRGB 里能取到的最大彩度：超出就压低彩度，而不是截断通道（截断会让颜色发灰偏色）
qreal fitChroma(qreal lightness, qreal hueDegrees)
{
    qreal low = 0.0;
    qreal high = 0.5;
    for(int i = 0; i < kGamutSearchSteps; i++)
    {
        const qreal middle = (low + high) / 2.0;
        qreal rgb[3];
        toLinearRgb(lightness, middle, hueDegrees, rgb);
        if(inSrgbGamut(rgb))
            low = middle;
        else
            high = middle;
    }
    return low;
}

QColor toColor(qreal lightness, qreal chroma, qreal hueDegrees)
{
    qreal rgb[3];
    toLinearRgb(lightness, qMin(chroma, fitChroma(lightness, hueDegrees)), hueDegrees, rgb);
    const int red = qRound(srgbEncode(qBound(0.0, rgb[0], 1.0)) * 255.0);
    const int green = qRound(srgbEncode(qBound(0.0, rgb[1], 1.0)) * 255.0);
    const int blue = qRound(srgbEncode(qBound(0.0, rgb[2], 1.0)) * 255.0);
    return QColor(red, green, blue);
}
}

QColor CardStyle::groupColor(qint32 groupNumber)
{
    const qreal hue = std::fmod(kFirstHue + (groupNumber - 1) * kGoldenAngle, 360.0);
    return toColor(kLightness, kChromaMax, hue < 0.0 ? hue + 360.0 : hue);
}
