#include "fluentui3style.h"
#include "windows/MainWindow.h"

#include <QApplication>
#include <QEvent>
#include <QFontInfo>
#include <QScreen>
#include <QWidget>

namespace
{
// Qt 在 Tab/快捷键改变焦点时会给顶层窗口置上 WA_KeyboardFocusChange，而且只在窗口重新显示时清除。
// 之后即使鼠标点击，焦点框仍会以“键盘焦点”绘制。这里在鼠标按下时清掉该属性，让点击回到指针模式；
// 键盘 Tab 导航时仍会重新置上，焦点框只留给键盘操作。
class KeyboardFocusResetFilter final : public QObject
{
public:
    explicit KeyboardFocusResetFilter(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if(event->type() == QEvent::MouseButtonPress)
        {
            if(auto* widget = qobject_cast<QWidget*>(watched))
            {
                QWidget* window = widget->window();
                if(window->testAttribute(Qt::WA_KeyboardFocusChange))
                {
                    window->setAttribute(Qt::WA_KeyboardFocusChange, false);
                    window->update();
                }
            }
        }
        return QObject::eventFilter(watched, event);
    }
};

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
    a.installEventFilter(new KeyboardFocusResetFilter(&a));

    // FluentUI3 样式库随项目一起构建，直接实例化，不依赖 Qt 样式插件目录
    a.setStyle(new FluentUI3Style);
    setUpDefaultFont(a);

    MainWindow w;
    w.show();
    return a.exec();
}
