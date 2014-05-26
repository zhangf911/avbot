CONFIG += console

QT += gui
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

TARGET = avbot-gui

CONFIG   += console
CONFIG   -= app_bundle

SOURCES += \
    test.cpp \
    dialog.cpp

FORMS += \
    dialog.ui

HEADERS += \
    dialog.h

TEMPLATE = app
