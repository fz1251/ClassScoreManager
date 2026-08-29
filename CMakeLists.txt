cmake_minimum_required(VERSION 3.16)

project(ClassScoreManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 优先找 Qt6，找不到就用 Qt5；实际使用哪个由当前 Kit 的 CMAKE_PREFIX_PATH 决定
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

# 所有动态库与主程序输出到同一目录，运行时只需把该目录加入 PATH
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# 先把 QWindowKit 单独加到较浅的构建目录；
# ExWidgets 检测到 QWindowKit::Widgets 已存在后，不会再从过深的子目录重复添加
set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_QUICK OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_WIDGETS ON CACHE BOOL "" FORCE)
set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
set(QWINDOWKIT_INSTALL ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/FluentUIStyle/3rd/qwindowkit "${CMAKE_BINARY_DIR}/qwindowkit")

# FluentUI3 样式库与 ExWidgets 扩展控件，作为子项目随主程序一起构建
set(BUILD_LIBRARY ON CACHE BOOL "" FORCE)
set(BUILD_PLUGIN OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(EXWIDGETS_BUILD_FRAMELESS ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/FluentUIStyle)

set(APP_SOURCES
    src/core/ClassDataManager.cpp
    src/core/ScoreSumReport.cpp
    src/delegates/GroupSpinBoxDelegate.cpp
    src/delegates/ScoreExprDelegate.cpp
    src/delegates/TemplateSpinBoxDelegate.cpp
    src/main.cpp
    src/models/ClassTableModel.cpp
    src/models/RecordListModel.cpp
    src/models/TemplateListModel.cpp
    src/ui/AboutInfoDialog.cpp
    src/ui/HomeActionCard.cpp
    src/ui/ItemSelectionDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/PasteStudentsDialog.cpp
    src/ui/ShineLabel.cpp
    src/ui/SumExportDialog.cpp
)

set(APP_HEADERS
    src/core/ClassDataManager.h
    src/core/DataTypes.h
    src/core/ScoreSumReport.h
    src/delegates/GroupSpinBoxDelegate.h
    src/delegates/ScoreExprDelegate.h
    src/delegates/TemplateSpinBoxDelegate.h
    src/models/ClassTableModel.h
    src/models/RecordListModel.h
    src/models/TemplateListModel.h
    src/ui/AboutInfoDialog.h
    src/ui/HomeActionCard.h
    src/ui/ItemSelectionDialog.h
    src/ui/MainWindow.h
    src/ui/PasteStudentsDialog.h
    src/ui/ShineLabel.h
    src/ui/SumExportDialog.h
)

set(APP_FORMS
    src/ui/AboutInfoDialog.ui
    src/ui/ItemSelectionDialog.ui
    src/ui/MainWindow.ui
    src/ui/PasteStudentsDialog.ui
    src/ui/SumExportDialog.ui
)

set(APP_RESOURCES
    resources.qrc
)

if(WIN32)
    enable_language(RC)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.ico" APP_ICON_PATH)
    # rc.exe 的字符串里反斜杠是转义符，路径中的 \ 必须写成 \\
    string(REPLACE "\\" "\\\\" APP_RC_ICON_PATH "${APP_ICON_PATH}")
    set(APP_RC_FILE "${CMAKE_CURRENT_BINARY_DIR}/ClassScoreManager.rc")
    file(WRITE "${APP_RC_FILE}" "IDI_ICON1 ICON DISCARDABLE \"${APP_RC_ICON_PATH}\"\n")
    list(APPEND APP_SOURCES "${APP_RC_FILE}")
endif()

add_executable(ClassScoreManager WIN32
    ${APP_SOURCES}
    ${APP_HEADERS}
    ${APP_FORMS}
    ${APP_RESOURCES}
)

# MainWindow.ui 的 customwidget 头文件写作 .\src\ui\xxx.h，
# AUTOUIC 生成的头文件需要从项目根目录解析该相对路径（qmake 默认带 -I 项目根目录）
target_include_directories(ClassScoreManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ClassScoreManager PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    FluentUI3Style
    ExWidgets::ExWidgets
    ExWidgets::Frameless
)

if(WIN32)
    target_link_libraries(ClassScoreManager PRIVATE dwmapi)
endif()

if(MSVC)
    target_compile_options(ClassScoreManager PRIVATE /utf-8)
endif()
