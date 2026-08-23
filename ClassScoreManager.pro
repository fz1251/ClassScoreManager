QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

win32:LIBS += -ldwmapi
msvc: QMAKE_CXXFLAGS += /utf-8

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/core/ClassDataManager.cpp \
    src/core/ScoreSumReport.cpp \
    src/delegates/GroupSpinBoxDelegate.cpp \
    src/delegates/ScoreExprDelegate.cpp \
    src/delegates/TemplateSpinBoxDelegate.cpp \
    src/main.cpp \
    src/models/ClassTableModel.cpp \
    src/models/RecordListModel.cpp \
    src/models/TemplateListModel.cpp \
    src/ui/AboutInfoDialog.cpp \
    src/ui/ItemSelectionDialog.cpp \
    src/ui/MainWindow.cpp \
    src/ui/PasteStudentsDialog.cpp \
    src/ui/ShineLabel.cpp \
    src/ui/SumExportDialog.cpp

HEADERS += \
    src/core/ClassDataManager.h \
    src/core/DataTypes.h \
    src/core/ScoreSumReport.h \
    src/delegates/GroupSpinBoxDelegate.h \
    src/delegates/ScoreExprDelegate.h \
    src/delegates/TemplateSpinBoxDelegate.h \
    src/models/ClassTableModel.h \
    src/models/RecordListModel.h \
    src/models/TemplateListModel.h \
    src/ui/AboutInfoDialog.h \
    src/ui/ItemSelectionDialog.h \
    src/ui/MainWindow.h \
    src/ui/PasteStudentsDialog.h \
    src/ui/ShineLabel.h \
    src/ui/SumExportDialog.h

FORMS += \
    src/ui/AboutInfoDialog.ui \
    src/ui/ItemSelectionDialog.ui \
    src/ui/MainWindow.ui \
    src/ui/PasteStudentsDialog.ui \
    src/ui/SumExportDialog.ui

RC_ICONS = resources/icon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
