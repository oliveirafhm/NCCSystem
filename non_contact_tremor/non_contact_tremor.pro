#-------------------------------------------------
#
# Project created by QtCreator 2016-02-02T09:02:20
#
#-------------------------------------------------

QT       += core gui serialport multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = NCSystem
TEMPLATE = app

# https://wiki.qt.io/Build_Standalone_Qt_Application_for_Windows
#CONFIG += static

SOURCES += main.cpp\
        mainwindow.cpp \
    trialsetup.cpp \
    settingsdialog.cpp \
    patient.cpp \
    qcustomplot.cpp \
    plot.cpp

HEADERS  += mainwindow.h \
    trialsetup.h \
    settingsdialog.h \
    patient.h \
    qcustomplot.h \
    plot.h

FORMS    += mainwindow.ui \
    trialsetup.ui \
    settingsdialog.ui \
    patient.ui

RESOURCES += \
    non_contact_tremor.qrc

