#include "ui/MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{

// 调整 Qt5 默认行为，使其与原 Qt6 正确行为相符
#if QT_VERSION_MAJOR == 5
    // Qt5 无内置 DPI 感知，需手动开启
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    // Qt5 默认将 scale factor 取整（125%→100%），设为 PassThrough 保留原始值
    QApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

    QApplication a(argc, argv);

#if QT_VERSION_MAJOR == 5
    QFont defaultFont = a.font();
#ifdef Q_OS_WIN
    // Qt 5 在高 DPI 环境下默认 font family 为空，属于 Windows 平台问题
    // 导致字体回退至 SimSun（宋体），需手动设置，以避免显示问题
    defaultFont.setFamily("Microsoft YaHei UI");
#endif
    // Qt5 默认 weight 为 50（Light），需与 Qt6 下 400（Normal）统一
    defaultFont.setWeight(QFont::Normal);
    // Qt5 PreferDefaultHinting 走 GDI 强像素对齐路径，文字生硬，需禁用
    defaultFont.setHintingPreference(QFont::PreferNoHinting);
    a.setFont(defaultFont);
#endif

    MainWindow w;
    w.show();
    return a.exec();
}
