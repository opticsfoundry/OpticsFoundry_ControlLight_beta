#-------------------------------------------------
#
# Project created by QtCreator 2017-10-01T20:47:40
#
#-------------------------------------------------

#QT       += core gui network
QT -= gui
CONFIG += console
CONFIG -= app_bundle

SOURCES += main.cpp
SOURCES += ControlLightAPI.h
SOURCES += ControlLightAPI.cpp

TARGET = QTCPIP
TEMPLATE = app

#INCLUDEPATH += $$PWD
#LIBS += -L$$PWD/lib -lControlLightAPI

#DEFINES += USING_DLL

# If needed (to find the DLL at runtime)
# QMAKE_POST_LINK += $$quote(cmd /c copy /y $$shell_path($$PWD/ControlAPI.dll) $$shell_path($$OUT_PWD))


#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#TARGET = QTelnet
#TEMPLATE = app

#INCLUDEPATH += $$PWD/include
#LIBS += -L$$PWD/lib -lControlAPI


#SOURCES += main.cpp\
#   ControlAPI.cpp \
#    QTCPIP.cpp \
#    QCmdWidget.cpp \
#    QTCPIPControlTester.cpp \
#    TestSequence.cpp

#HEADERS  += \
#    ControlAPI.h \
#    QCmdWidget.h \
#    QTCPIP.h \
#    QTCPIPControlTester.h \
#    TestSequence.h \
#    main.h

#FORMS    += \
#    QTCPIPControlTester.ui

#RESOURCES += \
#    resources.qrc

