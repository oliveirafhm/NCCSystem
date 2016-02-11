#-------------------------------------------------
#
# Project created by QtCreator 2016-02-02T09:02:20
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TremorDataLogger
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    trialsetup.cpp \
    settingsdialog.cpp \
    patientdata.cpp

HEADERS  += mainwindow.h \
    trialsetup.h \
    settingsdialog.h \
    patientdata.h

FORMS    += mainwindow.ui \
    trialsetup.ui \
    settingsdialog.ui \
    patientdata.ui

RESOURCES += \
    non_contact_tremor.qrc

