TEMPLATE = app
TARGET = ReporterExample

QT += core network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets
}

CONFIG -= app_bundle
CONFIG += debug_and_release warn_on
CONFIG += thread exceptions rtti stl

# without c++11 & AppKit library compiler can't solve address for symbols
CONFIG += c++11
macx: LIBS += -framework AppKit

# link qBreakpad library
include($$PWD/../../qBreakpad.pri)

# source code
HEADERS += \
    $$PWD/../program/TestThread.h \
    $$PWD/reporter.h

SOURCES += \
    $$PWD/../program/TestThread.cpp \
    $$PWD/reporter.cpp

FORMS += \
    $$PWD/reporter.ui

OBJECTS_DIR = _build/obj
MOC_DIR = _build
