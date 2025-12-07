QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

win32:LIBS += -ldwmapi

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    class_data_manager.cpp \
    class_table_modal.cpp \
    main.cpp \
    mainwindow.cpp \
    score_expr_delegate.cpp \
    score_sum_report.cpp \
    ui/about_info_dialog.cpp \
    ui/item_selection_dialog.cpp \
    ui/paste_students_dialog.cpp \
    ui/sum_export_dialog.cpp

HEADERS += \
    class_data_manager.h \
    class_table_modal.h \
    data_types.h \
    mainwindow.h \
    score_expr_delegate.h \
    score_sum_report.h \
    ui/about_info_dialog.h \
    ui/item_selection_dialog.h \
    ui/paste_students_dialog.h \
    ui/sum_export_dialog.h

FORMS += \
    mainwindow.ui \
    ui/about_info_dialog.ui \
    ui/item_selection_dialog.ui \
    ui/paste_students_dialog.ui \
    ui/sum_export_dialog.ui

RC_ICONS = icon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
