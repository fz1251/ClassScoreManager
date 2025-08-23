QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutinfodialog.cpp \
    classdatamanager.cpp \
    classtablemodal.cpp \
    exportsumdialog.cpp \
    itemselectiondialog.cpp \
    main.cpp \
    mainwindow.cpp \
    pastestudentsdialog.cpp \
    scoreexprdelegate.cpp \
    scoresumreport.cpp

HEADERS += \
    aboutinfodialog.h \
    classdatamanager.h \
    classtablemodal.h \
    datatypes.h \
    exportsumdialog.h \
    itemselectiondialog.h \
    mainwindow.h \
    pastestudentsdialog.h \
    scoreexprdelegate.h \
    scoresumreport.h

FORMS += \
    aboutinfodialog.ui \
    exportsumdialog.ui \
    itemselectiondialog.ui \
    mainwindow.ui \
    pastestudentsdialog.ui

RC_ICONS = icon.ico

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
