#-------------------------------------------------
#
# Project created by QtCreator 2019-01-14T15:14:30
#
#-------------------------------------------------
include("../3rdparty/ZeroMQ/zmq.pri")

QT       += core gui xml network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VHF
TEMPLATE = app
DESTDIR = ../bin

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QT_MESSAGELOGCONTEXT
DEFINES -= CD_TEST



# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    comtextaudiothread.cpp \
    dbservices.cpp \
    javasubinfothread.cpp \
        main.cpp \
    networker.cpp \
    logindialog.cpp \
    controlwidget.cpp \
    controlwindow.cpp \
    extension.cpp \
    quploadfilethread.cpp \
    relaycontroller.cpp \
    groupcontrolwidget.cpp \
    inifile.cpp \
    tcpserver.cpp \
    tcpsocket.cpp \
    vhfdbdatabase.cpp \
    volume.cpp \
    yeastarnetwork.cpp \
    zchxbroadcasttaskmgr.cpp \
    zchxdataasyncworker.cpp \
    zchxvhfconvertthread.cpp \
    centercontrolthread.cpp \
    zchxvhfaudioplaythread.cpp \
    watchdogthread.cpp \
    rxbutton.cpp

HEADERS += \
    comtextaudiothread.h \
    dbservices.h \
    javasubinfothread.h \
    networker.h \
    logindialog.h \
    controlwidget.h \
    controlwindow.h \
    extension.h \
    quploadfilethread.h \
    relaycontroller.h \
    groupcontrolwidget.h \
    inifile.h \
    tcpserver.h \
    tcpsocket.h \
    vhfdatadefines.h \
    vhfdbdatabase.h \
    volume.h \
    yeastarnetwork.h \
    zchxbroadcasttaskmgr.h \
    zchxdataasyncworker.h \
    zchxvhfconvertthread.h \
    centercontrolthread.h \
    zchxvhfaudioplaythread.h \
    watchdogthread.h \
    rxbutton.h \
    vhfdata.h

FORMS += \
    logindialog.ui \
    controlwidget.ui \
    controlwindow.ui \
    volume.ui

TRANSLATIONS += $$PWD/Resource/zh_cn.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32: QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
win32: QMAKE_CFLAGS_RELEASE -= -Zc:strictStrings
win32: QMAKE_CFLAGS -= -Zc:strictStrings
win32: QMAKE_CXXFLAGS -= -Zc:strictStrings

RESOURCES += \
    resource.qrc

