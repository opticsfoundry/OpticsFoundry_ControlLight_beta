#-------------------------------------------------
#
# Project created by QtCreator 2017-10-01T20:47:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QTelnet
TEMPLATE = app


SOURCES += main.cpp\
    ControlLightAPI.cpp \
    QTCPIP.cpp \
    QCmdWidget.cpp \
    QTCPIPControlTester.cpp \
    TestSequence.cpp

HEADERS  += \
    ControlLightAPI.h \
    QCmdWidget.h \
    QTCPIP.h \
    QTCPIPControlTester.h \
    TestSequence.h \
    main.h

FORMS    += \
    QTCPIPControlTester.ui

RESOURCES += \
    resources.qrc

DISTFILES += \
    ControlHardwareConfig.json
