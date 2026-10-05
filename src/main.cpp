#include "fluentui3style.h"
#include "windows/MainWindow.h"

#include <QApplication>
#include <QFontInfo>
#include <QScreen>

namespace
{
// 物理字号够大时才用无 hinting 的柔和字形，否则汉字笔画会散成灰雾（实测数据见 docs/font-rendering.md）
constexpr qreal kNoHintingMinPhysicalPixelSize = 15.0;

QFont::HintingPreference defaultHintingPreference(const QFont& font)
{
    const QScreen* screen = QGuiApplication::primaryScreen();
    const qreal physicalPixelSize = QFontInfo(font).pixelSize() * (screen ? screen->devicePixelRatio() : 1.0);
    return physicalPixelSize >= kNoHintingMinPhysicalPixelSize ? QFont::PreferNoHinting : QFont::PreferFullHinting;
}

void setUpDefaultFont(QApplication& app)
{
    QFont font = app.font();
    // Qt5 在中文 Windows 上默认落到宋体，改用微软雅黑；"Microsoft YaHei UI" 自 Windows 8 起提供，按顺序回退
    font.setFamilies({QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei"), QStringLiteral("微软雅黑")});
    font.setHintingPreference(defaultHintingPreference(font));
    app.setFont(font);
}
} // namespace

int main(int argc, char* argv[])
{
#ifdef Q_OS_WIN
    // 固定用 FreeType 光栅器，避免 GDI 与不同版本 DirectWrite 在各 Windows 版本上字形不一致（见 docs/font-rendering.md）
    if(qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "windows:fontengine=freetype");
#endif

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication a(argc, argv);

    // FluentUI3 样式库随项目一起构建，直接实例化，不依赖 Qt 样式插件目录
    a.setStyle(new FluentUI3Style);
    setUpDefaultFont(a);

    MainWindow w;
    w.show();
    return a.exec();
}
