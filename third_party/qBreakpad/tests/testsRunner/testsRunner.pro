TEMPLATE = app
TARGET = testsRunner

QT -= gui
QT += testlib

CONFIG -= app_bundle
CONFIG += console c++11 testcase

win32 {
    DESTDIR = $$OUT_PWD
    DEFINES += TEST_RUNNER=\\\"$$OUT_PWD/../duplicates/duplicates_test.exe\\\"
} else {
    DEFINES += TEST_RUNNER=\\\"$$OUT_PWD/../duplicates/duplicates_test\\\"
}

SOURCES += \
    tst_duplicates.cpp
