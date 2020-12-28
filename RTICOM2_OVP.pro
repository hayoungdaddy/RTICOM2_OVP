#-------------------------------------------------
#
# Project created by QtCreator 2019-03-05T06:04:24
#
#-------------------------------------------------

QT       += core gui quick location charts sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RTICOM2_OVP
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    eventlist.cpp \
    filtersta.cpp \
    newregend.cpp

HEADERS += \
        mainwindow.h \
    common.h \
    eventlist.h \
    filtersta.h \
    newregend.h

FORMS += \
        mainwindow.ui \
    eventlist.ui \
    filtersta.ui \
    newregend.ui

RESOURCES += \
    RTICOM2_OVP.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
